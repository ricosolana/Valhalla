#pragma once


#include <tuple>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace avledet::util::mono1 {

    template<typename Tuple>
    struct default_compare_tuple;

    template<typename... Keys>
    struct default_compare_tuple<std::tuple<Keys...>>
    {
        using type = std::tuple<std::less<Keys>...>;
    };

    template<
        typename KeyTuple,
        typename Value,
        typename CompareTuple = typename default_compare_tuple<KeyTuple>::type
    >
    class monotonic_tree;

    template<typename... Keys, typename Value, typename... Compares>
    class monotonic_tree<std::tuple<Keys...>, Value, std::tuple<Compares...>>
    {
        static_assert(sizeof...(Keys) == sizeof...(Compares),
            "Each key must have a comparator");

    public:
        using key_type = std::tuple<Keys...>;
        using mapped_type = Value;
        using value_type = std::pair<key_type, mapped_type>;
        using size_type = std::size_t;

    private:
        std::vector<value_type> data_;

        template<size_t I>
        static constexpr auto comparator()
        {
            return std::tuple_element_t<I, std::tuple<Compares...>>{};
        }

        static bool keys_equal(const key_type& a, const key_type& b)
        {
            return !keys_less(a,b) && !keys_less(b,a);
        }

        template<std::size_t I = 0>
        static bool keys_less(const key_type& lhs, const key_type& rhs)
        {
            if constexpr (I == sizeof...(Keys)) {
                return false;
            } else
            {
                auto cmp = comparator<I>();

                const auto& key_lhs = std::get<I>(lhs);
                const auto& key_rhs = std::get<I>(rhs);

                if (cmp(key_lhs, key_rhs)) return true;
                if (cmp(key_rhs, key_lhs)) return false;

                return keys_less<I + 1>(lhs, rhs);
            }
        }

        auto lower_bound(const key_type& key)
        {
            return std::lower_bound(
                data_.begin(),
                data_.end(),
                key,
                [](const value_type& a, const key_type& b)
                {
                    return keys_less(a.first, b);
                }
            );
        }

        void check_monotonic(
            const key_type& key,
            typename std::vector<value_type>::iterator it)
        {
            // previous element
            if (it != data_.begin())
            {
                auto prev = std::prev(it);
                if (keys_less(key, prev->first))
                    throw std::logic_error(
                        "monotonic_tree: key violates monotonic ordering (previous)");
            }

            // next element
            if (it != data_.end())
            {
                if (keys_less(it->first, key))
                    throw std::logic_error(
                        "monotonic_tree: key violates monotonic ordering (next)");
            }
        }

        auto get_monotonic(const key_type& key)
        {
            auto it = lower_bound(key);
            
            check_monotonic(key, it);

            return it; // iterator to insertion position
        }

        //void check_monotonic(const key_type& k)
        //{
        //    if (data_.empty()) return;

        //    const auto& last = data_.back().first;

        //    if (keys_less(k, last))
        //        throw std::logic_error("monotonic_tree: keys are not monotonic");
        //}

        template<size_t I, typename K>
        auto find_impl(const K& key)
        {
            static_assert(I < sizeof...(Keys),
                "monotonic_tree::find<I>: key index out of range");

            auto it = std::lower_bound(
                data_.begin(),
                data_.end(),
                key,
                [](const value_type& v, const K& k)
                {
                    auto cmp = comparator<I>();
                    return cmp(std::get<I>(v.first), k);
                }
            );

            if (it == data_.end())
                return it;

            const auto& val = std::get<I>(it->first);

            auto cmp = comparator<I>();
            if (!cmp(val,key) && !cmp(key,val))
                return it;

            return data_.end();
        }

    public:
        monotonic_tree() = default;

        size_type size() const { return data_.size(); }
        bool empty() const { return data_.empty(); }

        auto begin() { return data_.begin(); }
        auto end() { return data_.end(); }

        auto begin() const { return data_.begin(); }
        auto end() const { return data_.end(); }

        template<size_t I, typename K>
        auto find(const K& key)
        {
            return find_impl<I>(key);
        }

        template<size_t I, typename K>
        auto find(const K& key) const
        {
            return find_impl<I>(key);
        }

        auto find_tuple(const key_type& key)
        {
            auto it = lower_bound_position(key);

            if (it != data_.end() && keys_equal(it->first, key))
                return it;

            return data_.end();
        }

        //template<typename... Args>
        //auto try_emplace(const key_type& key, Args&&... args)
        //{
        //    check_monotonic(key);
//
        //    data_.emplace_back(
        //        key,
        //        mapped_type(std::forward<Args>(args)...)
        //    );
//
        //    return std::prev(data_.end());
        //}
/*
        template<typename... Args>
        std::pair<typename std::vector<value_type>::iterator, bool>
        try_emplace(const key_type& key, Args&&... args)
        {
            auto it = std::lower_bound(
                data_.begin(),
                data_.end(),
                key,
                [](const value_type& a, const key_type& b)
                {
                    return keys_less(a.first, b);
                });

            // key already exists
            if (it != data_.end() && keys_equal(it->first, key))
                return { it, false };

            it = data_.emplace(
                it,
                key,
                mapped_type(std::forward<Args>(args)...)
            );

            return { it, true };
        }*/

        template<typename... Args>
        std::pair<typename std::vector<value_type>::iterator,bool>
        try_emplace(const key_type& key, Args&&... args)
        {
            auto it = lower_bound(*this, key);

            if (it != data_.end() && keys_equal(it->first, key))
                return { it, false };

            check_monotonic(key, it);

            it = data_.emplace(
                it,
                std::piecewise_construct,
                std::forward_as_tuple(key),
                std::forward_as_tuple(std::forward<Args>(args)...)
            );

            return { it, true };
        }

        std::pair<typename std::vector<value_type>::iterator, bool>
        insert(value_type v)
        {
            auto it = get_monotonic(v.first);

            if (it != data_.end() && keys_equal(it->first, v.first))
                return { it, false };

            it = data_.emplace(it, std::move(v));
            return { it, true };
        }

        mapped_type& operator[](const key_type& key)
        {
            auto it = get_monotonic(key);

            if (it != data_.end() && keys_equal(it->first, key))
                return it->second;

            it = data_.emplace(
                it,
                std::piecewise_construct,
                std::forward_as_tuple(key),
                std::forward_as_tuple()
            );

            return it->second;
        }

        template<size_t I, typename K>
        bool erase(const K& key)
        {
            static_assert(I < sizeof...(Keys),
                "monotonic_tree::erase<I, K>: key index out of range");

            auto it = find_impl<I>(key);
            if (it == data_.end())
                return false;

            data_.erase(it);
            return true;
        }

        // could be renamed
        //  erase_tuple
        //  erase_tied
        bool erase(const key_type& key)
        {
            auto it = find(key);
            if (it == data_.end())
                return false;

            data_.erase(it);
            return true;
        }
    };
}