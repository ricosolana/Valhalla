#pragma once

#include <tuple>
#include <Stream.h>
#include <Traits.h>

namespace avledet::util {

    /* https://godbolt.org/z/MMGsa8rhr
    * to implement deduction guides
    * (static functions, no class object needed)
    */

    // Thanks @Fux
    template<class T>
    class MethodBase {
    public:
        // Calls a locally stored function
        //  Expects a passthrough parameter and serialized package
        //  Returns false if the call requested unsubscription
        virtual void invoke(T handle, Reader& reader) = 0;
    };


    // Package lambda invoker
    template<class T, typename F>
    class Method : public MethodBase<T> {
        using args_type = typename avledet::util::traits::func_traits<F>::args_type;

        template<class Tuple, size_t... Is>
        auto impl_tail(Reader& reader, std::index_sequence<Is...>) {
            return Reader::deserialize<std::tuple_element_t<Is + 1u, Tuple>...>(reader);
        }

    private:
        const F m_func;

    public:
        Method(F func) : m_func(func) { }

        void invoke(T handle, Reader& reader) override {
            // This covers too little data received for fn deserialize
            auto tuple = std::tuple_cat(std::forward_as_tuple(handle),
                impl_tail<args_type>(reader,
                    (std::make_index_sequence < std::tuple_size<args_type>{} - 1 > {})));

            // If too much data received for fn deserialize
            if (reader.get_pos() != reader.size()) {
                //throw
                //warn
            }

            std::apply(m_func, tuple);
        }
    };

    template<typename F>
    Method(F) -> Method<
        std::tuple_element_t<0, typename avledet::util::traits::func_traits<F>::args_type>,
        F
    >;

}// namespace avledet::util