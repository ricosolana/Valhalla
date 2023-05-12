#pragma once

#include <functional>
#include <chrono>

#include "VUtils.h"

class Task {
public:
    using F = std::function<void(Task&)>;

public:
    const F m_func;
    std::chrono::steady_clock::time_point m_at;
    std::chrono::milliseconds m_period; // 0 = no repeat

public:
    Task(F func, steady_clock::time_point at, milliseconds period) 
        : m_func(func), m_at(at), m_period(period) {}

    bool Repeats() const {
        return m_period.count() == 0;
    }

    void Cancel() {
        m_period = milliseconds::min();
    }
};
