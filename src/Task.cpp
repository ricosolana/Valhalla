#include "Task.hpp"

using namespace std::chrono_literals;

bool Task::Repeats() {
	return period > 0ms;
}

void Task::Cancel() {
	period = -1ms;
}
