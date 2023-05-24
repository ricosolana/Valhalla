#pragma once

//#include "rpc/detail/bool.h"
// Borrowed from
//  https://github.com/rpclib/rpclib/blob/master/include/rpc/detail/func_traits.h

#include <tuple>
#include <variant>
#include <concepts>

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
            // typename fails on msvc
            // using type = std::variant<typename Ts...>;
            using type = std::variant<Ts...>;
        };



        //untested
        /*
        // https://stackoverflow.com/questions/30736242/how-can-i-get-the-index-of-a-type-in-a-variadic-class-template
        template <typename... >
        struct variadic_index_of_type;

        // found it
        template <typename T, typename... R>
        struct variadic_index_of_type<T, T, R...>
            : std::integral_constant<size_t, 0>
        { };

        // still looking
        template <typename T, typename F, typename... R>
        struct variadic_index_of_type<T, F, R...>
            : std::integral_constant<size_t, 1 + variadic_index_of_type<T, R...>::value>
        { };*/

        
        
        template <size_t index, size_t... >
        struct variadic_value_at_index_reversed;

        // found it
        template <size_t index, size_t F, size_t... R>
            requires (index == sizeof...(R))
        struct variadic_value_at_index_reversed<index, F, R...>
            : std::integral_constant<size_t, F>
        { };

        // still looking
        template <size_t index, size_t F, size_t... R>
            requires (index != sizeof...(R))
        struct variadic_value_at_index_reversed<index, F, R...>
            : std::integral_constant<size_t, variadic_value_at_index_reversed<index, R...>::value>
        { };

        template <size_t index, size_t... R>
        struct variadic_value_at_index
            : std::integral_constant<size_t, variadic_value_at_index_reversed<sizeof...(R) - index - 1, R...>::value>
        { };



        template <size_t index, size_t... >
        struct variadic_accumulate_values_until_index_reversed;

        template <size_t index, size_t F, size_t...R>
            requires (index == sizeof...(R) || sizeof...(R) == 0)
        struct variadic_accumulate_values_until_index_reversed<index, F, R...>
            : std::integral_constant<size_t, F>
        { };

        template <size_t index, size_t F, size_t... R>
            requires (index < sizeof...(R) && sizeof...(R) > 0)
        struct variadic_accumulate_values_until_index_reversed<index, F, R...>
            : std::integral_constant<size_t, F + variadic_accumulate_values_until_index_reversed<index, R...>::value>
        { };

        template <size_t index, size_t... R>
        struct variadic_accumulate_values_until_index
            : std::integral_constant<size_t, variadic_accumulate_values_until_index_reversed<sizeof...(R) - index, R...>::value>
        { };
    }
}
