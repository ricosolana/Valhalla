#pragma once

#include <thread>
#include <asio.hpp>
#include "AsyncDeque.hpp"
#include <semaphore>
#include "Task.hpp"
#include "NetRpc.hpp"

using namespace asio::ip;

namespace Alchyme {
	// contain within Net namespace
	enum class PeerResult : uint8_t {
		OK,
		WRONG_VERSION,
		WRONG_PASSWORD,
		MAX_PEERS,
		NOT_WHITELISTED,
		BANNED,	// ip ban is checked very early
		UID_ALREADY_ONLINE,
		NAME_ALREADY_ONLINE,
		BAD_MAGIC,
		//STEAM_INVALID_SESSION_TICKET,
	};

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
		const bool m_isServer;

	public:
		static Game* Get();
		static void RunClient();
		static void RunServer();

		/*constexpr*/ Game(bool isServer);

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
