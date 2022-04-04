#include "Task.hpp"

namespace Alchyme {
	bool Task::Repeats() {
		using namespace std::chrono_literals;
		return period > 0ms;
	}
}
