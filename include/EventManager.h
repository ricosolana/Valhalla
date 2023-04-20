#pragma once

/*
#include "VUtils.h"

class IEventManager {
    UNORDERED_MAP_t<HASH_t, std::vector<void*>> m_functions;

public:
    template<typename Func>
    void Register(std::string_view name, Func func) {

    }

    template<typename ...Args>
    void Dispatch(HASH_t hash, Args... args) {
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