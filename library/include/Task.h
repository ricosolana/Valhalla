#pragma once

#include <chrono>
#include <functional>

using namespace std::chrono_literals;

class Task
{
  public:
    using F = std::function<void(Task &)>;

  public:
    F const m_func;
    std::chrono::steady_clock::time_point m_at;
    std::chrono::milliseconds m_period;// 0 = no repeat

  public:
    Task(F func, std::chrono::steady_clock::time_point at, std::chrono::milliseconds period) :
        m_func(func),
        m_at(at),
        m_period(period)
    {
    }

    bool Repeats() const
    {
        return m_period >= 0ms;
    }

    void Cancel()
    {
        //m_period = milliseconds::min();
        m_period = -1ms;
    }
};
