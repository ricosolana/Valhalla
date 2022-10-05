#pragma once

#include <memory>
#include <thread>
#include <mutex>
#include <deque>
#include <optional>
#include <vector>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cstdint>

//#define USE_DEQUE_WAIT

template<typename T>
class AsyncDeque
{
public:
	AsyncDeque() = default;
	AsyncDeque(const AsyncDeque<T>&) = delete;
	virtual ~AsyncDeque() { clear(); }

public:
	// Returns and maintains item at front of Queue
	T& front()
	{
		std::scoped_lock lock(m_mutex);
		return m_deque.front();
	}

	// Returns and maintains item at back of Queue
	T& back()
	{
		std::scoped_lock lock(m_mutex);
		return m_deque.back();
	}

	// Removes and returns item from front of Queue
	T pop_front()
	{
		std::scoped_lock lock(m_mutex);
		auto t = std::move(m_deque.front()); // prevents copying
		m_deque.pop_front();
		return t;
	}

	// Removes and returns item from back of Queue
	T pop_back()
	{
		std::scoped_lock lock(m_mutex);
		auto t = std::move(m_deque.back()); // prevents copying
		m_deque.pop_back();
		return t;
	}

	// Adds an item to back of Queue
	void push_back(const T &item)
	{
		std::scoped_lock lock(m_mutex);
		m_deque.push_back(item);

#ifdef USE_DEQUE_WAIT
		std::unique_lock<std::mutex> ul(muxBlocking);
		m_cv.notify_one();
#endif
	}

	// Adds an item to front of Queue
	void push_front(const T &item)
	{
		std::scoped_lock lock(m_mutex);
		m_deque.push_front(item);

#ifdef USE_DEQUE_WAIT
		std::unique_lock<std::mutex> ul(muxBlocking);
		m_cv.notify_one();
#endif
	}

	// https://stackoverflow.com/questions/4303513/push-back-vs-emplace-back
	// emplace_back does not work as expected in msvc
	// so use push_back with a move
	void push_back(T&& item)
	{
		std::scoped_lock lock(m_mutex);
		//m_deque.emplace_back((item));
		m_deque.push_back(std::move(item));

#ifdef USE_DEQUE_WAIT
		std::unique_lock<std::mutex> ul(muxBlocking);
		m_cv.notify_one();
#endif
	}

	// Adds an item to front of Queue
	void push_front(T&& item)
	{
		std::scoped_lock lock(m_mutex);
		//m_deque.emplace_front((item));
		m_deque.push_front(std::move(item));

#ifdef USE_DEQUE_WAIT
		std::unique_lock<std::mutex> ul(muxBlocking);
		m_cv.notify_one();
#endif
	}



	// Returns true if Queue has no items
	bool empty()
	{
		std::scoped_lock lock(m_mutex);
		return m_deque.empty();
	}

	// Returns number of items in Queue
	size_t count()
	{
		std::scoped_lock lock(m_mutex);
		return m_deque.size();
	}

	// Clears Queue
	void clear()
	{
		std::scoped_lock lock(m_mutex);
		m_deque.clear();
	}

#ifdef USE_DEQUE_WAIT
	bool wait()
	{
		while (empty())
		{
			std::unique_lock<std::mutex> ul(muxBlocking);
			m_cv.wait(ul);
			if (notified) {
				notified = false;
				return true;
			}
		}
		return false;
	}

	void notify()
	{
		std::unique_lock<std::mutex> ul(muxBlocking);
		notified = true;
		m_cv.notify_one();
	}
#endif

protected:
	std::mutex m_mutex;
	std::deque<T> m_deque;

#ifdef USE_DEQUE_WAIT
	std::condition_variable m_cv;
	std::mutex muxBlocking;
	bool notified = false;
#endif
};
