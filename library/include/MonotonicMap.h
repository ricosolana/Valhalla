#pragma once


#include <tuple>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace avledet::util::mono {

    namespace priv {
        template<size_t I=0, class Tuple>
        int compare_direction(const Tuple& a, const Tuple& b)
        {
            if constexpr (I == std::tuple_size_v<Tuple>)
                return 0;
            else
            {
                if (std::get<I>(a) < std::get<I>(b)) return -1;
                if (std::get<I>(a) > std::get<I>(b)) return +1;
                return compare_direction<I+1>(a,b);
            }
        }

        template<size_t I=0, class Tuple>
        bool parallel_order_valid(const Tuple& a, const Tuple& b, int dir)
        {
            if constexpr (I == std::tuple_size_v<Tuple>)
                return true;
            else
            {
                if (dir < 0 && !(std::get<I>(a) <= std::get<I>(b)))
                    return false;

                if (dir > 0 && !(std::get<I>(a) >= std::get<I>(b)))
                    return false;

                return parallel_order_valid<I+1>(a,b,dir);
            }
        }

        template<size_t I=0, class Tuple>
        bool tuple_dimension_unique(const Tuple& a, const Tuple& b)
        {
            if constexpr (I == std::tuple_size_v<Tuple>)
                return true;
            else
            {
                if (std::get<I>(a) == std::get<I>(b))
                    return false;

                return tuple_dimension_unique<I+1>(a,b);
            }
        }



        // Entire tuple must be unique to another tuple
        struct all_unique
        {
            template<size_t I=0,class Tuple>
            static bool unique(const Tuple& a,const Tuple& b)
            {
                if constexpr (I==std::tuple_size_v<Tuple>)
                    return true;
                else
                {
                    if (std::get<I>(a)==std::get<I>(b))
                        return false;

                    return unique<I+1>(a,b);
                }
            }
        };

        // at least 1 tuple element must differ from another tuple
        struct one_unique
        {
            template<class Tuple>
            static bool unique(const Tuple& a,const Tuple& b)
            {
                return !(a==b);
            }
        };

        // Keys are free to fully clash
        struct none_unique
        {
            template<class Tuple>
            static bool unique(const Tuple& a, const Tuple& b)
            {
                // Always returns true: duplicates are allowed
                return true;
            }
        };



        template<class _Value, class _KeyTuple, class _Unique>
        class parallel_vector_map
        {
        public:
            using key_type = _KeyTuple;
            using mapped_type = _Value;
            using value_type = std::pair<key_type, mapped_type>;
            using iterator = typename std::vector<value_type>::iterator;
            using size_type = std::size_t;

        private:
            std::vector<value_type> data;

        private:
            template<class Tuple>
            static bool less(const Tuple& a,const Tuple& b)
            {
                return a < b;
            }

            template<size_t I=0,class Tuple>
            static int direction(const Tuple& a,const Tuple& b)
            {
                if constexpr (I==std::tuple_size_v<Tuple>)
                    return 0;
                else
                {
                    if (std::get<I>(a) < std::get<I>(b)) return -1;
                    if (std::get<I>(a) > std::get<I>(b)) return 1;
                    return direction<I+1>(a,b);
                }
            }

            template<size_t I=0,class Tuple>
            static bool monotonic(const Tuple& a,const Tuple& b,int dir)
            {
                if constexpr (I==std::tuple_size_v<Tuple>)
                    return true;
                else
                {
                    if (dir < 0 && !(std::get<I>(a) <= std::get<I>(b)))
                        return false;

                    if (dir > 0 && !(std::get<I>(a) >= std::get<I>(b)))
                        return false;

                    return monotonic<I+1>(a,b,dir);
                }
            }

            template<class Tuple>
            static bool valid(const Tuple& a,const Tuple& b)
            {
                int d = direction(a,b);
                return monotonic(a,b,d);
            }

        public:
            iterator lower_bound(const key_type& k)
            {
                return std::lower_bound(
                    data.begin(),
                    data.end(),
                    k,
                    [](const value_type& e, const key_type& k)
                    {
                        return less(e.first,k);
                    });
            }

            //std::pair<iterator, bool> insert(const key_type& k, mapped_type&& v)
            std::pair<iterator, bool> insert(value_type&& v)
            {
                //std::unordered_map<int, int>().insert()

                //std::unordered_map<int, int>()[0];
                
                //.insert( { 0, 1 });

                return try_emplace(v.first, std::forward<decltype(v.second)>(v.second));

                /*
                auto pos = lower_bound(k);

                // ---- check previous neighbor ----
                if (pos != data.begin())
                {
                    auto& prev = std::prev(pos)->first;

                    if (!valid(prev,k))
                        throw std::logic_error("parallel key ordering violated");

                    if (!_Unique::unique(prev,k))
                        return {std::prev(pos),false};
                }

                // ---- check next neighbor ----
                if (pos != data.end())
                {
                    auto& next = pos->first;

                    if (!valid(k,next))
                        throw std::logic_error("parallel key ordering violated");

                    if (!_Unique::unique(k,next))
                        return {pos,false};
                }

                pos = data.insert(pos, { k, std::move(v) });
                return {pos,true};*/
            }

            template<typename... Args>
            std::pair<iterator, bool>
            try_emplace(const key_type& key, Args&&... args)
            {
                auto pos = lower_bound(key);

                // ---- check previous neighbor ----
                if (pos != data.begin())
                {
                    auto& prev = std::prev(pos)->first;

                    if (!valid(prev, key))
                        throw std::logic_error("parallel key ordering violated");

                    if (!_Unique::unique(prev, key))
                        return {std::prev(pos),false};
                }

                // ---- check next neighbor ----
                if (pos != data.end())
                {
                    auto& next = pos->first;

                    if (!valid(key, next))
                        throw std::logic_error("parallel key ordering violated");

                    if (!_Unique::unique(key, next))
                        return {pos,false};
                }

                //pos = data.insert(pos, { k, std::move(v) });
                pos = data.emplace(
                    pos,
                    std::piecewise_construct,
                    std::forward_as_tuple(key),
                    std::forward_as_tuple(std::forward<Args>(args)...)
                );

                return { pos, true };
            }

            iterator find(const key_type& k)
            {
                auto it = lower_bound(k);
                if (it!=data.end() && it->first==k)
                    return it;
                return data.end();
            }

            // TODO
            mapped_type& operator[](const key_type& key)
            {
                //return *try_emplace(key);
                return try_emplace(key).first->second;

                //auto it = get_monotonic(key);
//
                //if (it != data.end() && keys_equal(it->first, key))
                //    return it->second;
//
                //it = data.emplace(
                //    it,
                //    std::piecewise_construct,
                //    std::forward_as_tuple(key),
                //    std::forward_as_tuple()
                //);
//
                //return it->second;
            }

            /*
            // TODO 
            template<size_t I, typename K>
            auto find(const K& key)
            {
                static_assert(I < std::tuple_size_v<key_type>,
                    "monotonic_tree::find<I>: key index out of range");

                auto it = std::lower_bound(
                    data.begin(),
                    data.end(),
                    key,
                    [](const value_type& v, const K& k)
                    {
                        auto cmp = comparator<I>();

                        return cmp(std::get<I>(v.first), k);
                    }
                );

                if (it == data.end())
                    return it;

                const auto& val = std::get<I>(it->first);

                auto cmp = comparator<I>();
                if (!cmp(val,key) && !cmp(key,val))
                    return it;

                return data.end();
            }

            // TODO
            template<size_t I, typename K>
            bool erase(const K& key)
            {
                static_assert(I < std::tuple_size_v<key_type>,
                    "monotonic_tree::erase<I, K>: key index out of range");

                auto it = find<I>(key);
                if (it == end())
                    return false;

                data.erase(it);
                return true;
            }*/

            // could be renamed
            //  erase_tuple
            //  erase_tied
            bool erase(const key_type& key)
            {
                auto it = find(key);
                if (it == end())
                    return false;

                data.erase(it);
                return true;
            }

            iterator begin(){ return data.begin(); }
            iterator end(){ return data.end(); }
            iterator begin() const { return data.begin(); }
            iterator end() const { return data.end(); }

            size_type size() const { return data.size(); }
            bool empty() const { return data.empty(); }
        };

    }

    template<class _Value,class... Keys>
    using parallel_strict_map =
        priv::parallel_vector_map<
            _Value,
            std::tuple<Keys...>,
            priv::all_unique
        >;

    template<class _Value,class... Keys>
    using parallel_weak_map =
        priv::parallel_vector_map<
            _Value,
            std::tuple<Keys...>,
            priv::one_unique
        >;

    template<class _Value,class... Keys>
    using parallel_multi_map =
        priv::parallel_vector_map<
            _Value,
            std::tuple<Keys...>,
            priv::none_unique
        >;
}