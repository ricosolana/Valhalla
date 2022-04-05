#include "Task.hpp"

namespace Valhalla {
	bool Task::Repeats() {
		using namespace std::chrono_literals;
		return period > 0ms;
	}
}
