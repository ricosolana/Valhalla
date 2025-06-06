#pragma once

/*
#include "VUtils.h"

class IEventManager {
    avledet::util::Map<avledet::util::Hash, std::vector<void*>> m_functions;

public:
    template<typename Func>
    void Register(std::string_view name, Func func) {

    }

    template<typename ...Args>
    void Dispatch(avledet::util::Hash hash, Args... args) {
        auto&& find = m_functions.find(hash);
        if (find != m_functions.end()) {
            auto&& vec = find->second;
            for (auto&& raw : vec) {
                // cast to a function ptr
                auto&& func = ()
            }
        }

    }
};

IEventManager* EventManager();
*/