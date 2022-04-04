#pragma once

#include <tuple>
#include <functional>
#include "NetPacket.hpp"

/* https://godbolt.org/z/MMGsa8rhr
* to implement deduction guides
* (static functions, no class object needed)
*/

namespace Alchyme {
    namespace Net {

        class Peer;

        // Thanks @Fux
        template <class C, class Tuple, class F, size_t... Is>
        constexpr auto invoke_tuple_impl(F f, C& c, Peer* pao, Tuple t,
            std::index_sequence<Is...>) {
            return std::invoke(f, c, pao, std::get<Is>(t)...);
        }

        template <class C, class Tuple, class F>
        constexpr auto invoke_tuple(F f, C& c, Peer* pao, Tuple t) {
            return invoke_tuple_impl(f, c, pao, t, std::make_index_sequence < std::tuple_size<Tuple>{} > {});
        }

        class IMethod
        {
        public:
            virtual void Invoke(Peer* pao, Packet* packet) = 0;
        };

        template<class C, class...Args>
        class Method final : public IMethod {
            using Lambda = void(C::*)(Peer*, Args...);

            C* object;
            Lambda lambda;

            template<class F>
            auto Invoke_impl(Packet* p) {
                F f; p->Read(f);
                std::tuple<F> a{ f };
                return a;
            }

            // Split a param,
            // Add that param from Packet into tuple
            template<class F, class S, class...R>
            auto Invoke_impl(Packet* p) {
                F f; p->Read(f);
                std::tuple<F> a{ f };
                std::tuple<S, R...> b = Invoke_impl<S, R...>(p);
                return std::tuple_cat(a, b);
            }

        public:
            Method(C* object, Lambda lam) : object(object), lambda(lam) {}

            void Invoke(Peer* pao, Packet* p) {
                // Invoke_impl returns a tuple of types by recursion
                if constexpr (sizeof...(Args))
                {
                    auto tupl = Invoke_impl<Args...>(p);
                    invoke_tuple(lambda, object, pao, tupl);
                }
                else
                {
                    // works like ~magic~
                    std::invoke(lambda, object, pao);
                }
            }
        };
    }
}
