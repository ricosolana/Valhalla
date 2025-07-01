#include <memory>
#include <quill/core/LogLevel.h>
#include <quill/LogMacros.h>
#include <string_view>

#include <gtl/btree.hpp>
#include <gtl/gtl_base.hpp>

#include "Hashes.h"
#include "Method.h"
#include "ModManager.h"
#include "Types.h"
#include "ValhallaServer.h"

//#include "Peer.h"//TODO remove!!

namespace avledet::rpc {

    template<class T>
    class RpcBase
    {
      public:
        using Method = IMethod<T>;// keep consistent for now...

      protected:
        void internal_invoke(T handle, avledet::util::Hash hash, avledet::util::Reader &reader)
        {
            auto &&find = m_methods.find(hash);
            if (find != m_methods.end()) {
                //bool keep_mapped = find->get()->Invoke(std::move(handle), reader);//(1)
                bool keep_mapped = find->second->Invoke(std::move(handle), reader);//(1)
                if (!keep_mapped) {
                    LOG_TRACE_L1(AVL_LOGGER, "method {} unsubscribed", hash);
                    m_methods.erase(
                            hash);// do not erase using iterator; changes between (1) and here can result in invalid iterator
                }
            }
        }

      public:
        // TODO later, for custom LUA
        void register_method(std::unique_ptr<Method> method)
        {
            auto hash       = method->m_hash;
            m_methods[hash] = std::move(method);
        }

        // *note: registering a rpc by assign ([]) *might* break things if called from within a invoked function
        template<typename F>
        void register_method(avledet::util::Hash hash, F func)
        {
            this->register_method(std::make_unique<MethodImpl<T, F>>(hash, std::move(func)));
        }

        template<typename F>
        void register_method(std::string_view name, F func)
        {
            this->register_method(avledet::util::get_stable_hash(name), std::move(func));
        }

      public:
        //gtl::btree_set<MethodPtr, std::less<>> m_methods;
        avledet::util::Map<avledet::util::Hash, std::unique_ptr<Method>> m_methods;
    };

}// namespace avledet::rpc
