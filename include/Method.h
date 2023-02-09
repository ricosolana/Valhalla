#pragma once

#include <tuple>
#include <functional>
#include <type_traits>
#include <concepts>

#include "VUtils.h"
#include "VUtilsTraits.h"
#include "DataReader.h"
#include "ModManager.h"

/* https://godbolt.org/z/MMGsa8rhr
* to implement deduction guides
* (static functions, no class object needed)
*/

// Thanks @Fux
template<class T>
class IMethod
{
public:
    // Invoke a function with variadic parameters
    // A copy of the package is made
    // You may perform a move on the package as needed
    // TODO accept a different stream type, readonly, with referenced vector
    virtual void Invoke(T t, DataReader reader) = 0;
};

// Base forward declaration
//template<class T, class...V> class MethodImpl;


/*
// Package lambda invoker
template<class T, typename F> 
    //requires std::same_as<T,
    //std::tuple_element_t<0, typename VUtils::Traits::func_traits<F>::args_type>>
class MethodImplPre
    : public IMethod<T> 
{
    //using VUtils::Traits::func_traits;
    //using args_type = typename func_traits<F>::args_type;
    using args_type = typename VUtils::Traits::func_traits<F>::args_type;

    //static_assert(std::is_same<T, std::tuple_element_t<0, args_type>>, "Lambda first type does not match declared type");



    template<class Tuple, size_t... Is> //requires (sizeof...(Is))
    auto impl_tail(DataReader& reader, std::index_sequence<Is...>) {
        //using Tail = std::tuple<std::tuple_element_t<Is + 1, Tuple>...>;
        return DataReader::Deserialize<std::tuple_element_t<Is + 1u, Tuple>...>(reader);
        //return NetPackage::Deserialize<Tail>(pkg);
    }

private:
    const F m_func;

public:
    explicit MethodImplPre(F func) 
        : m_func(func) {}

    // shouldnt pass around a vector in container
    // wherre only pos/marker increases
    void Invoke(T t, DataReader reader) override {
        auto tuple = std::tuple_cat(std::forward_as_tuple(t),
                                    //NetPackage::Deserialize<Args...>(pkg));
                                    impl_tail<args_type>(reader,
                                        (std::make_index_sequence < std::tuple_size<args_type>{} - 1> {})));

        if (reader.Position() != reader.Length())
            LOG(WARNING) << "Peer Rpc Invoke has more data than expected " 
                << reader.Length() << "/" << reader.Position();

        std::apply(m_func, tuple);
    }
};

template<typename F>
MethodImplPre(F) -> MethodImplPre<
    std::tuple_element_t<0, typename VUtils::Traits::func_traits<F>::args_type>, 
    F
>;*/



// Package lambda invoker
template<class T, typename F>
//requires std::same_as<T,
//std::tuple_element_t<0, typename VUtils::Traits::func_traits<F>::args_type>>
class MethodImpl
    : public IMethod<T>
{
    //using VUtils::Traits::func_traits;
    //using args_type = typename func_traits<F>::args_type;
    using args_type = typename VUtils::Traits::func_traits<F>::args_type;

    //static_assert(std::is_same<T, std::tuple_element_t<0, args_type>>, "Lambda first type does not match declared type");



    template<class Tuple, size_t... Is> //requires (sizeof...(Is))
    auto impl_tail(DataReader& reader, std::index_sequence<Is...>) {
        //using Tail = std::tuple<std::tuple_element_t<Is + 1, Tuple>...>;
        return DataReader::Deserialize<std::tuple_element_t<Is + 1u, Tuple>...>(reader);
        //return NetPackage::Deserialize<Tail>(pkg);
    }

private:
    const F m_func;

    const HASH_t m_categoryHash;
    const HASH_t m_methodHash;

public:
    explicit MethodImpl(F func, HASH_t categoryHash, HASH_t methodHash)
        : m_func(func), m_categoryHash(categoryHash), m_methodHash(methodHash) {}

    // shouldnt pass around a vector in container
    // wherre only pos/marker increases
    void Invoke(T t, DataReader reader) override {
        auto tuple = std::tuple_cat(std::forward_as_tuple(t),
            //NetPackage::Deserialize<Args...>(pkg));
            impl_tail<args_type>(reader,
                (std::make_index_sequence < std::tuple_size<args_type>{} - 1 > {})));

        if (reader.Position() != reader.Length())
            LOG(WARNING) << "Peer Rpc Invoke has more data than expected "
            << reader.Length() << "/" << reader.Position();

        // Prefix
        // category catch
        //ModManager()->CallEventTuple(m_categoryHash, tuple);
        // specific catch
        if (ModManager()->CallEventTuple(m_categoryHash ^ m_methodHash, tuple) != EventStatus::CANCEL);
            std::apply(m_func, tuple);

        // Postfix
        ModManager()->CallEventTuple(m_categoryHash ^ m_methodHash ^ EVENT_HASH_POST, tuple);
    }
};

template<typename F>
MethodImpl(F, HASH_t, HASH_t) -> MethodImpl<
    std::tuple_element_t<0, typename VUtils::Traits::func_traits<F>::args_type>,
    F
>;



// TODO streamline this into MethodImpl somehow
//  all rpc calls CAN be unpacked into a (sender, PKG), then funnelled further down to whatever 
//  rpc handlers with args to unpack
// Lua callbacks
template<class T>
class MethodImplLua : public IMethod<T> {
    sol::function m_func;
    std::vector<DataType> m_types;

public:
    explicit MethodImplLua(sol::function func, std::vector<DataType> types)
        : m_func(std::move(func)), m_types(std::move(types)) {}

    void Invoke(T t, DataReader reader) override {
        sol::variadic_results results;
        auto&& state(m_func.lua_state());

        // Pass the rpc always
        results.push_back(sol::make_object(state, t));

        for (auto&& type : m_types) {
            switch (type) {
            case DataType::BYTES:
                // Will be interpreted as sol container type
                // see https://sol2.readthedocs.io/en/latest/containers.html
                results.push_back(sol::make_object(state, reader.Read<BYTES_t>()));
                break;
            case DataType::STRING:
                // Primitive: string
                results.push_back(sol::make_object(state, reader.Read<std::string>()));
                break;
            case DataType::ZDOID:
                results.push_back(sol::make_object(state, reader.Read<NetID>()));
                break;
            case DataType::VECTOR3:
                results.push_back(sol::make_object(state, reader.Read<Vector3>()));
                break;
            case DataType::VECTOR2i:
                results.push_back(sol::make_object(state, reader.Read<Vector2i>()));
                break;
            case DataType::QUATERNION:
                results.push_back(sol::make_object(state, reader.Read<Quaternion>()));
                break;
            case DataType::STRINGS:
                // Container type of Primitive: string
                results.push_back(sol::make_object(state, reader.Read<std::vector<std::string>>()));
                break;
            case DataType::BOOL:
                // Primitive: boolean
                results.push_back(sol::make_object(state, reader.Read<bool>()));
                break;
            case DataType::INT8:
                // Primitive: number
                results.push_back(sol::make_object(state, reader.Read<int8_t>()));
                break;
            case DataType::INT16:
                // Primitive: number
                results.push_back(sol::make_object(state, reader.Read<int16_t>()));
                break;
            case DataType::INT32:
                // Primitive: number
                results.push_back(sol::make_object(state, reader.Read<int32_t>()));
                break;
            case DataType::INT64:
                // Primitive: number
                results.push_back(sol::make_object(state, reader.Read<uint64_t>()));
                break;
            case DataType::FLOAT:
                // Primitive: number
                results.push_back(sol::make_object(state, reader.Read<float>()));
                break;
            case DataType::DOUBLE:
                // Primitive: number
                results.push_back(sol::make_object(state, reader.Read<double>()));
                break;
            default:
                LOG(ERROR) << "LUA MethodImpl bad";
            }
        }

        m_func(results);

        //auto args(sol::as_args(results));
        //m_func(args);
    }
};

template<typename T>
MethodImplLua(sol::function, std::vector<DataType>) -> MethodImplLua<T>;
