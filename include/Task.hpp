#pragma once

#include <functional>
#include <chrono>

namespace Alchyme {
	struct Task {
		const std::function<void()> function;
		std::chrono::steady_clock::time_point at;

		// a period of 0 will denote no repeat
		const std::chrono::milliseconds period;

		bool Repeats();
	};
}
