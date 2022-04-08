#pragma once

#include <thread>
#include <asio.hpp>
#include "Task.hpp"
#include "ZNetPeer.hpp"
#include "MyRenderInterface.hpp"
#include "MySystemInterface.hpp"
#include "MyFileInterface.hpp"
#include "ZNet.hpp"
#include "Settings.hpp"

using namespace asio::ip;

class Game {
	bool m_running = false;

	// perfect structure for this job
	// https://stackoverflow.com/questions/2209224/vector-vs-list-in-stl
	std::list<std::unique_ptr<Task>> m_tasks;

	std::recursive_mutex m_taskMutex;

	SDL_Window* m_sdlWindow = nullptr;
	SDL_GLContext* m_sdlGLContext = nullptr;
	SDL_Renderer* m_sdlRenderer = nullptr;

	Rml::Context* m_rmlContext = nullptr;
	std::unique_ptr<MyRenderInterface> m_renderInterface;
	std::unique_ptr<MySystemInterface> m_systemInterface;
	std::unique_ptr<MyFileInterface> m_fileInterface;

	Settings m_settings;

public:
	static constexpr const char* VERSION = "1.0.0";
	std::unique_ptr<ZNet> m_znet;

	static Game *Get();
	static void Run();

	void Stop();

	Task* RunTask(Task::F f);
	Task* RunTaskLater(Task::F f, std::chrono::milliseconds after);
	Task* RunTaskAt(Task::F f, std::chrono::steady_clock::time_point at);
	Task* RunTaskLaterRepeat(Task::F f, std::chrono::milliseconds after, std::chrono::milliseconds period);
	Task* RunTaskAtRepeat(Task::F f, std::chrono::steady_clock::time_point at, std::chrono::milliseconds period);

private:
	void Start();

	void Update(float delta);

	void InitSDL();
	void InitGLEW();
	void InitRML();
};
