#pragma once

#include <functional>
#include <chrono>

struct Task {
	using F = std::function<void(Task*)>;
	const F function;
	std::chrono::steady_clock::time_point at;

	// a period of 0 will denote no repeat
	std::chrono::milliseconds period;

	bool Repeats();
	void Cancel();
};
