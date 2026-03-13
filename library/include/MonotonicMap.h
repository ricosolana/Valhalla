#pragma once


#include <tuple>
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

        struct parallel_tuple_order
        {
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
        };

        struct strict_unique_policy
        {
            template<size_t I=0,class Tuple>
            static bool unique_impl(const Tuple& a,const Tuple& b)
            {
                if constexpr (I==std::tuple_size_v<Tuple>)
                    return true;
                else
                {
                    if (std::get<I>(a)==std::get<I>(b))
                        return false;

                    return unique_impl<I+1>(a,b);
                }
            }

            template<class Tuple>
            static bool unique(const Tuple& a,const Tuple& b)
            {
                return unique_impl(a,b);
            }
        };

        struct weak_unique_policy
        {
            template<class Tuple>
            static bool unique(const Tuple& a,const Tuple& b)
            {
                return !(a==b);
            }
        };

        struct allow_duplicates_policy
        {
            template<class Tuple>
            static bool unique(const Tuple& a, const Tuple& b)
            {
                // Always returns true: duplicates are allowed
                return true;
            }
        };

        template<class Value, class KeyTuple, class OrderPolicy, class UniquePolicy>
        class parallel_vector_map
        {
            using entry = std::pair<KeyTuple, Value>;
            std::vector<entry> data;

        public:
            using key_type = KeyTuple;
            using iterator = typename std::vector<entry>::iterator;
            using size_type = std::size_t;

            iterator lower_bound(const key_type& k)
            {
                return std::lower_bound(
                    data.begin(),
                    data.end(),
                    k,
                    [](const entry& e, const key_type& k)
                    {
                        return OrderPolicy::less(e.first,k);
                    });
            }

            std::pair<iterator,bool> insert(const key_type& k, const Value& v)
            {
                auto pos = lower_bound(k);

                // ---- check previous neighbor ----
                if (pos != data.begin())
                {
                    auto& prev = std::prev(pos)->first;

                    if (!OrderPolicy::valid(prev,k))
                        throw std::logic_error("parallel key ordering violated");

                    if (!UniquePolicy::unique(prev,k))
                        return {std::prev(pos),false};
                }

                // ---- check next neighbor ----
                if (pos != data.end())
                {
                    auto& next = pos->first;

                    if (!OrderPolicy::valid(k,next))
                        throw std::logic_error("parallel key ordering violated");

                    if (!UniquePolicy::unique(k,next))
                        return {pos,false};
                }

                pos = data.insert(pos,{k,v});
                return {pos,true};
            }

            iterator find(const key_type& k)
            {
                auto it = lower_bound(k);
                if (it!=data.end() && it->first==k)
                    return it;
                return data.end();
            }

            iterator begin(){ return data.begin(); }
            iterator end(){ return data.end(); }
            iterator begin() const { return data.begin(); }
            iterator end() const { return data.end(); }

            size_type size() const { return data.size(); }
            bool empty() const { return data.empty(); }
        };

    }

    template<class Value,class... Keys>
    using parallel_strict_map =
        priv::parallel_vector_map<
            Value,
            std::tuple<Keys...>,
            priv::parallel_tuple_order,
            priv::strict_unique_policy
        >;

    template<class Value,class... Keys>
    using parallel_weak_map =
        priv::parallel_vector_map<
            Value,
            std::tuple<Keys...>,
            priv::parallel_tuple_order,
            priv::weak_unique_policy
        >;

    template<class Value,class... Keys>
    using parallel_multi_map =
        priv::parallel_vector_map<
            Value,
            std::tuple<Keys...>,
            priv::parallel_tuple_order,
            priv::allow_duplicates_policy
        >;
}