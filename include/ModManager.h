#pragma once 

#include <sol/sol.hpp>

class IModManager {
    struct Mod {
        std::string m_name;
        sol::environment m_env;

        Mod(std::string name,
            sol::environment env) 
            : m_name(name), m_env(std::move(env)) {}
    };

private:
    sol::state m_state;

public:
    void Init();

    void Uninit();

};
