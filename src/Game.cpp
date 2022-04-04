#include "Game.hpp"
#include "Client.hpp"
#include "Server.hpp"

namespace Alchyme {
	static std::unique_ptr<Game> impl;

	Game* Game::Get() {
		return impl.get();
	}

	void Game::RunClient() {
		impl = std::make_unique<Client>();
		impl->Start();
	}

	void Game::RunServer() {
		impl = std::make_unique<Server>();
		impl->Start();
	}

	Game::Game(bool isServer)
		: m_isServer(isServer)
	{}

	void Game::StartIOThread() {
		m_ctxThread = std::thread([this]() {
			el::Helpers::setThreadName("io");
			m_ctx.run();
		});

#if defined(_WIN32)// && !defined(_NDEBUG)
		void* pThr = m_ctxThread.native_handle();
		SetThreadDescription(pThr, L"IO Thread");
#endif
	}

	void Game::StopIOThread() {
		m_ctx.stop();

		if (m_ctxThread.joinable())
			m_ctxThread.join();

		m_ctx.restart();
	}

	void Game::Start() {
		m_running = true;

		while (m_running) {
			auto now = std::chrono::steady_clock::now();
			static auto last_tick = now; // Initialized to this once
			auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - last_tick).count();
			last_tick = now;

			{
				std::scoped_lock lock(m_taskMutex);
				const auto now = std::chrono::steady_clock::now();
				for (auto itr = m_tasks.begin(); itr != m_tasks.end();) {
					if (itr->at < now) {
						itr->function();
						if (itr->Repeats()) {
							itr->at += itr->period;
							++itr;
						}
						else
							itr = m_tasks.erase(itr);
					}
					else
						++itr;
				}
			}

			// UPDATE
			Update(elapsed / 1000000.f);

			// could instead create a performance analyzer that will
			// not delay when tps is low
			// Removing this will make cpu usage go to a lot more
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	void Game::Stop() {
		m_running = false;
	}

	void Game::RunTask(std::function<void()> task) {
		RunTaskLater(task, 0ms);
	}

	void Game::RunTaskLater(std::function<void()> task, std::chrono::milliseconds after) {
		RunTaskLaterRepeat(task, after, 0ms);
	}

	void Game::RunTaskAt(std::function<void()> task, std::chrono::steady_clock::time_point at) {
		RunTaskAtRepeat(task, at, 0ms);
	}

	void Game::RunTaskLaterRepeat(std::function<void()> task, std::chrono::milliseconds after, std::chrono::milliseconds period) {
		RunTaskAtRepeat(task, std::chrono::steady_clock::now() + after, period);
	}

	void Game::RunTaskAtRepeat(std::function<void()> task, std::chrono::steady_clock::time_point at, std::chrono::milliseconds period) {
		std::scoped_lock lock(m_taskMutex);
		m_tasks.push_back({ task, at, period });
	}

	void Game::Disconnect(Net::Peer* peer) {
		peer->m_socket.reset();
	}

	void Game::DisconnectLater(Net::Peer* peer) {
		RunTaskLater([peer]() {
			peer->Disconnect();
		}, 1s);
	}

}
