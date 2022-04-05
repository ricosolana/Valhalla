#pragma once

#include <thread>
#include <asio.hpp>
#include "AsyncDeque.hpp"
#include <semaphore>
#include "Task.hpp"
#include "NetRpc.hpp"

using namespace asio::ip;

namespace Valhalla {

	class Game {
	private:
		bool m_running = false;

		// perfect structure for this job
		// https://stackoverflow.com/questions/2209224/vector-vs-list-in-stl
		std::list<Task> m_tasks;

		std::recursive_mutex m_taskMutex;

	protected:
		static constexpr int MAGIC = 0xABCDEF69;
		static constexpr const char* VERSION = "1.0.0";

		std::thread m_ctxThread;
		asio::io_context m_ctx;

	public:
		virtual void Start();
		virtual void Stop();

		void StartIOThread();
		void StopIOThread();

		void RunTask(std::function<void()> task);
		void RunTaskLater(std::function<void()> task, std::chrono::milliseconds after);
		void RunTaskAt(std::function<void()> task, std::chrono::steady_clock::time_point at);
		void RunTaskLaterRepeat(std::function<void()> task, std::chrono::milliseconds after, std::chrono::milliseconds period);
		void RunTaskAtRepeat(std::function<void()> task, std::chrono::steady_clock::time_point at, std::chrono::milliseconds period);

		void Disconnect(Net::Peer *peer);

		void DisconnectLater(Net::Peer* peer);

	private:
		virtual void Update(float delta) = 0;

		virtual void ConnectCallback(Net::Peer* peer) = 0;
		virtual void DisconnectCallback(Net::Peer* peer) = 0;
	};
}
