#pragma once

//#include "rpc/detail/bool.h"
// Borrowed from
//  https://github.com/rpclib/rpclib/blob/master/include/rpc/detail/func_traits.h

#include <tuple>
#include <variant>
#include <type_traits>

namespace avledet::util::traits {

    template<typename T>
    using invoke = typename T::type;

    template <int N, typename... Ts>
    using nth_type = invoke<std::tuple_element<N, std::tuple<Ts...>>>;

    namespace tags {

        // tags for the function traits, used for tag dispatching
        struct zero_arg { };
        struct nonzero_arg { };
        struct void_result { };
        struct nonvoid_result { };

        template <int N> struct arg_count_trait { typedef nonzero_arg type; };

        template <> struct arg_count_trait<0> { typedef zero_arg type; };

        template <typename T> struct result_trait { typedef nonvoid_result type; };

        template <> struct result_trait<void> { typedef void_result type; };
    }

    //untested idea
    //template <class T>
    //concept is_callable = requires(T a) {
    //    a();
    //};
    
    //template<typename Tuple, std::size_t Index>
    //    requires (Index < std::tuple_size_v<Tuple>)
    //using safe_tuple_element_t = std::tuple_element_t<Index, Tuple>;
    //
    //template<typename Tuple, std::size_t Index>
    //    requires (Index >= std::tuple_size_v<Tuple>)
    //using safe_tuple_element_t = void;



    //! \brief Provides a small function traits implementation that
    //! works with a reasonably large set of functors.
    template <typename T>
    struct func_traits : func_traits<decltype(&T::operator())> { };

    template <typename C, typename R, typename... Args>
    struct func_traits<R(C::*)(Args...)> : func_traits<R(*)(Args...)> { };

    template <typename C, typename R, typename... Args>
    struct func_traits<R(C::*)(Args...) const> : func_traits<R(*)(Args...)> { };

    template <typename R, typename... Args> struct func_traits<R(*)(Args...)> {
        using result_type = R;
        using arg_count = std::integral_constant<std::size_t, sizeof...(Args)>;
        //the arguments of the function, without qualifiers, ie, no & or const
        using args_type = std::tuple<typename std::decay<Args>::type...>;
        //the arguments of the function, with qualifiers, ie, & or const
        using raw_args_type = std::tuple<Args...>;
        //std::conditional_t<bool Cond, typename Iftrue, typename Iffalse>
        //TODO create a simple getter for first arg, second arg, third arg...
        //  but only enable IF those args are found
        //template
        //requires (Index < std::tuple_size_v<Tuple>)
        //using first_arg = safe_tuple_element_t<raw_args_type, 0>;
        //using second_arg = safe_tuple_element_t<raw_args_type, 1>;
        //using third_arg = safe_tuple_element_t<raw_args_type, 2>;
        //using first_arg = std::conditional_t<
        //    (std::tuple_size_v<raw_args_type> <= 0), 
        //    void, std::tuple_element_t<0, raw_args_type>>;
        //using second_arg = std::conditional_t<
        //    (std::tuple_size_v<raw_args_type> <= 1), 
        //    void, std::tuple_element_t<1, raw_args_type>>;
        //using third_arg = std::conditional_t<
        //    (std::tuple_size_v<raw_args_type> <= 2), 
        //    void, std::tuple_element_t<2, raw_args_type>>;
        //... create more as needed
    };



    template <typename T>
    struct func_kind_info : func_kind_info<decltype(&T::operator())> { };

    template <typename C, typename R, typename... Args>
    struct func_kind_info<R(C::*)(Args...)> : func_kind_info<R(*)(Args...)> { };

    template <typename C, typename R, typename... Args>
    struct func_kind_info<R(C::*)(Args...) const>
        : func_kind_info<R(*)(Args...)> {
    };

    template <typename R, typename... Args> struct func_kind_info<R(*)(Args...)> {
        typedef typename tags::arg_count_trait<sizeof...(Args)>::type args_kind;
        typedef typename tags::result_trait<R>::type result_kind;
    };



    template <class T>
    concept is_iterable = requires {
        std::begin(std::declval<T>());
        std::end(std::declval<T>());
    };

    template <class T>
    concept has_key_type = requires {
        typename T::key_type;
    };

    template <class T>
    concept has_value_type = requires {
        typename T::value_type;
    };

    template <class T>
    concept has_traits_type = requires {
        typename T::traits_type;
    };



    template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
    template<class... Ts> overload(Ts...) -> overload<Ts...>; // line not needed in C++20...



    // disabled (and unused, how lucky) due to .clangd being dumb
    // Get the index of a type in a tuple
    //template <class T, class Tuple>
    //struct tuple_index;
    //
    //// Get the index of a type in a tuple
    //template <class T, class... Types>
    //struct tuple_index<T, std::tuple<T, Types...>> {
    //    static const std::size_t value = 0;
    //};
    //
    //// Get the index of a type in a tuple
    //template <class T, class U, class... Types>
    //struct tuple_index<T, std::tuple<U, Types...>> {
    //    static const std::size_t value = 1 + tuple_index<T, std::tuple<Types...>>::value;
    //};

    //template<class T, class U, class... Types>
    //constexpr std::size_t tuple_index_v = tuple_index<T, U, Types...>::value;



    // https://stackoverflow.com/questions/25958259/how-do-i-find-out-if-a-tuple-contains-a-type
    template <typename T, typename Tuple>
    struct tuple_has_type;

    template <typename T, typename... Us>
    struct tuple_has_type<T, std::tuple<Us...>> : std::disjunction<std::is_same<T, Us>...> { };

    template<typename T, typename... Us>
    constexpr bool tuple_has_type_v = tuple_has_type<T, Us...>::value;



    template <typename Tuple>
    struct tuple_to_variant;

    template <typename... Ts>
    struct tuple_to_variant<std::tuple<Ts...>> {
        // typename fails on msvc
        // using type = std::variant<typename Ts...>;
        using type = std::variant<Ts...>;
    };

    template<typename... T>
    using tuple_to_variant_t = tuple_to_variant<T...>::type;



    //runtime value at tuple index



    //untested
    /*
    // https://stackoverflow.com/questions/30736242/how-can-i-get-the-index-of-a-type-in-a-variadic-class-template
    template <typename... >
    struct variadic_index_of_type;

    // found it
    template <typename T, typename... R>
    struct variadic_index_of_type<T, T, R...>
        : std::integral_constant<std::size_t, 0>
    { };

    // still looking
    template <typename T, typename F, typename... R>
    struct variadic_index_of_type<T, F, R...>
        : std::integral_constant<std::size_t, 1 + variadic_index_of_type<T, R...>::value>
    { };*/



    template <std::size_t index, std::size_t...>
    struct variadic_value_at_index;

    template <std::size_t index, std::size_t F, std::size_t... R>
        requires (index == 0)
    struct variadic_value_at_index<index, F, R...>
        : std::integral_constant<std::size_t, F> {
    };

    template <std::size_t index, std::size_t F, std::size_t... R>
        requires (index > 0)
    struct variadic_value_at_index<index, F, R...>
        : variadic_value_at_index<index - 1, R...> {
    };



    template <std::size_t index, std::size_t... >
    struct variadic_accumulate_values_to_index;

    template <std::size_t index, std::size_t F, std::size_t...R>
        requires (index == 0)
    struct variadic_accumulate_values_to_index<index, F, R...>
        : std::integral_constant<std::size_t, F> {
    };

    template <std::size_t index, std::size_t F, std::size_t... R>
        requires (index > 0)
    struct variadic_accumulate_values_to_index<index, F, R...>
        : std::integral_constant<std::size_t, F + variadic_accumulate_values_to_index<index - 1, R...>::value> {
    };

}// namespace avledet::util::traits

//backward compatability
namespace VUtils {
    namespace Traits = avledet::util::traits;
}