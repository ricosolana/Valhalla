#pragma once

//#include "rpc/detail/bool.h"
// Borrowed from
//  https://github.com/rpclib/rpclib/blob/master/include/rpc/detail/func_traits.h

namespace VUtils {
    namespace Traits {

        template<typename T>
        using invoke = typename T::type;

        //template <int N>
        //using is_zero = invoke<std::conditional<(N == 0), true_, false_>>;

        template <int N, typename... Ts>
        using nth_type = invoke<std::tuple_element<N, std::tuple<Ts...>>>;

        namespace tags {

            // tags for the function traits, used for tag dispatching
            struct zero_arg {};
            struct nonzero_arg {};
            struct void_result {};
            struct nonvoid_result {};

            template <int N> struct arg_count_trait { typedef nonzero_arg type; };

            template <> struct arg_count_trait<0> { typedef zero_arg type; };

            template <typename T> struct result_trait { typedef nonvoid_result type; };

            template <> struct result_trait<void> { typedef void_result type; };
        }

        //! \brief Provides a small function traits implementation that
        //! works with a reasonably large set of functors.
        template <typename T>
        struct func_traits : func_traits<decltype(&T::operator())> {};

        template <typename C, typename R, typename... Args>
        struct func_traits<R(C::*)(Args...)> : func_traits<R(*)(Args...)> {};

        template <typename C, typename R, typename... Args>
        struct func_traits<R(C::*)(Args...) const> : func_traits<R(*)(Args...)> {};

        template <typename R, typename... Args> struct func_traits<R(*)(Args...)> {
            using result_type = R;
            using arg_count = std::integral_constant<std::size_t, sizeof...(Args)>;
            using args_type = std::tuple<typename std::decay<Args>::type...>;
        };



        // get type in lambda
        //template <typename F>
        //struct lam_nth_arg



        template <typename T>
        struct func_kind_info : func_kind_info<decltype(&T::operator())> {};

        template <typename C, typename R, typename... Args>
        struct func_kind_info<R(C::*)(Args...)> : func_kind_info<R(*)(Args...)> {};

        template <typename C, typename R, typename... Args>
        struct func_kind_info<R(C::*)(Args...) const>
            : func_kind_info<R(*)(Args...)> {};

        template <typename R, typename... Args> struct func_kind_info<R(*)(Args...)> {
            typedef typename tags::arg_count_trait<sizeof...(Args)>::type args_kind;
            typedef typename tags::result_trait<R>::type result_kind;
        };

        //template <typename F> using is_zero_arg = is_zero<func_traits<F>::arg_count>;

        //template <typename F>
        //using is_single_arg =
            //invoke<std::conditional<func_traits<F>::arg_count == 1, true_, false_>>;

        template <typename F>
        using is_void_result = std::is_void<typename func_traits<F>::result_type>;



        template <typename T, typename = void>
        struct is_iterable : std::false_type {};

        // this gets used only when we can call std::begin() and std::end() on that type
        template <typename T>
        struct is_iterable<T, std::void_t<decltype(std::begin(std::declval<T>())),
            decltype(std::end(std::declval<T>()))
        >
        > : std::true_type {};

        // Here is a helper:
        template <typename T>
        constexpr bool is_iterable_v = is_iterable<T>::value;



        template <typename T, typename = void>
        struct has_value_type : std::false_type {};

        template <typename T>
        struct has_value_type<T, std::void_t<typename T::value_type
        >
        > : std::true_type {};

        // Here is a helper:
        template <typename T>
        constexpr bool has_value_type_v = has_value_type<T>::value;



        template <typename T, typename = void>
        struct has_key_type : std::false_type {};

        template <typename T>
        struct has_key_type<T, std::void_t<typename T::key_type
        >
        > : std::true_type {};

        // Here is a helper:
        template <typename T>
        constexpr bool has_key_type_v = has_key_type<T>::value;



        template <typename T, typename = void>
        struct has_traits_type : std::false_type {};

        template <typename T>
        struct has_traits_type<T, std::void_t<typename T::traits_type
        >
        > : std::true_type {};

        // Here is a helper:
        template <typename T>
        constexpr bool has_traits_type_v = has_traits_type<T>::value;



        template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
        template<class... Ts> overload(Ts...)->overload<Ts...>; // line not needed in C++20...



        // Get the index of a type in a tuple
        template <class T, class Tuple>
        struct tuple_index;
        
        // Get the index of a type in a tuple
        template <class T, class... Types>
        struct tuple_index<T, std::tuple<T, Types...>> {
            static const std::size_t value = 0;
        };
        
        // Get the index of a type in a tuple
        template <class T, class U, class... Types>
        struct tuple_index<T, std::tuple<U, Types...>> {
            static const std::size_t value = 1 + tuple_index<T, std::tuple<Types...>>::value;
        };



        // https://stackoverflow.com/questions/25958259/how-do-i-find-out-if-a-tuple-contains-a-type
        template <typename T, typename Tuple>
        struct tuple_has_type;

        template <typename T, typename... Us>
        struct tuple_has_type<T, std::tuple<Us...>> : std::disjunction<std::is_same<T, Us>...> {}; 



        template <typename Tuple>
        struct tuple_to_variant;

        template <typename... Ts>
        struct tuple_to_variant<std::tuple<Ts...>>
        {
            using type = std::variant<typename Ts...>;
        };
    }
}
