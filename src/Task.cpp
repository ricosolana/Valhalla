#include "Task.h"

using namespace std::chrono_literals;

bool Task::Repeats() {
    return period > 0ms;
}

void Task::Cancel() {
    period = std::chrono::milliseconds::min();
}
