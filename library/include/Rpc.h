#include <memory>
#include <string_view>

#include <gtl/btree.hpp>
#include <gtl/gtl_base.hpp>

#include "Hashes.h"
#include "Method.h"

//#include "Peer.h"//TODO remove!!

namespace avledet::rpc {

    template<class T>
    class RpcBase
    {
      public:
        //using T = std::shared_ptr<Peer>;
        //using MethodPtr = std::unique_ptr<IMethod<T>>;
        using Method = IMethod<T>;// keep consistent for now...

      protected:
        void internal_invoke(T handle, avledet::util::Hash hash, avledet::util::Reader &reader)
        {
            auto &&find = m_methods.find(hash);
            if (find != m_methods.end()) {
                //bool keep_mapped = find->get()->Invoke(std::move(handle), reader);//(1)
                bool keep_mapped = find->second->Invoke(std::move(handle), reader);//(1)
                if (!keep_mapped) {
                    m_methods.erase(
                            hash);// do not erase using iterator; changes between (1) and here can result in invalid iterator
                }
            }
        }

      public:
        // TODO later, for custom LUA
        //void register_method(MethodPtr method)
        //{
        //    m_methods.emplace(std::move(method));
        //}

        // *note: registering a rpc by assign ([]) *might* break things if called from within a invoked function
        template<typename F>
        void register_method(avledet::util::Hash hash, F func)
        {
            //m_methods.emplace(std::make_unique<MethodImpl<T, F>>(hash, std::move(func)));
            m_methods[hash] = std::make_unique<MethodImpl<T, F>>(std::move(func));
        }

        template<typename F>
        void register_method(std::string_view name, F func)
        {
            this->register_method(avledet::util::get_stable_hash(name), std::move(func));
        }

      private:
        //gtl::btree_set<MethodPtr, std::less<>> m_methods;
        avledet::util::Map<avledet::util::Hash, std::unique_ptr<Method>> m_methods;
    };

}// namespace avledet::rpc
