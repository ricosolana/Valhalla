#pragma once

#include <thread>
#include <asio.hpp>
#include "Task.hpp"
#include "NetPeer.hpp"
#include "MyRenderInterface.hpp"
#include "MySystemInterface.hpp"
#include "MyFileInterface.hpp"
#include "NetClient.hpp"

using namespace asio::ip;

class Game {
	bool m_running = false;

	// perfect structure for this job
	// https://stackoverflow.com/questions/2209224/vector-vs-list-in-stl
	std::list<Task> m_tasks;

	std::recursive_mutex m_taskMutex;

	SDL_Window* m_sdlWindow = nullptr;
	SDL_GLContext* m_sdlGLContext = nullptr;
	SDL_Renderer* m_sdlRenderer = nullptr;

	Rml::Context* m_rmlContext = nullptr;
	std::unique_ptr<MyRenderInterface> m_renderInterface;
	std::unique_ptr<MySystemInterface> m_systemInterface;
	std::unique_ptr<MyFileInterface> m_fileInterface;

public:
	std::unique_ptr<Client> m_client;

	static constexpr const char* VERSION = "1.0.0";

public:
	static Game& Get();
	static void Run();

	void Stop();

	void RunTask(std::function<void()> task);
	void RunTaskLater(std::function<void()> task, std::chrono::milliseconds after);
	void RunTaskAt(std::function<void()> task, std::chrono::steady_clock::time_point at);
	void RunTaskLaterRepeat(std::function<void()> task, std::chrono::milliseconds after, std::chrono::milliseconds period);
	void RunTaskAtRepeat(std::function<void()> task, std::chrono::steady_clock::time_point at, std::chrono::milliseconds period);

private:
	void Start();

	void Update(float delta);

	void InitSDL();
	void InitGLEW();
	void InitRML();
};
