#pragma once

#include <string>
#include <memory>
#include <ankerl/unordered_dense.h>
#include <Method.h>
#include <Socket.h>
#include <avledet/util/UserID.h>
#include <avledet/util/Vector.h>
#include <avledet/sync/ZDOID.h>
#include <NetAccepter.h>
#include <avledet/util/Types.h>

namespace avledet::procedure {
    template<class T>
    class RpcBase {
    protected:
        void internal_invoke(T handle, avledet::util::Hash hash, avledet::util::Reader& reader) {
            auto&& find = m_methods.find(hash);
            if (find != m_methods.end()) {
                find->second->invoke(handle, reader);
            }
        }

    public:
        // *note: registering a rpc by assign ([]) *might* break things if called from within a invoked function
        template<typename F>
        void register_method(avledet::util::Hash hash, F func) {
            m_methods.emplace(hash, std::make_unique<avledet::util::Method<T, F>>(func));
        }

        //template<typename F>
        //void register_method(std::string_view name, F func) {            
        //    m_methods.emplace(hash, std::make_unique<avledet::util::Method<T, F>>(func));
        //}

    private:
        ankerl::unordered_dense::map<avledet::util::Hash, std::unique_ptr<avledet::util::MethodBase<T>>> m_methods;
    };

}// namespace avledet::stream

