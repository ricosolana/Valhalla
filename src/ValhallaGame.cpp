#include "ValhallaGame.hpp"
#include <SDL.h>
#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include "ScriptManager.hpp"

static Game GAME;
Game& Game::Get() {
	return GAME;
}

void Game::Run() {
	GAME.Start();
}



void Game::Start() {
	InitSDL();
	InitGLEW();
	InitRML();
	ScriptManager::Init();

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
	Rml::Shutdown();
	SDL_DestroyRenderer(m_sdlRenderer);
	SDL_GL_DeleteContext(m_sdlGLContext);
	SDL_DestroyWindow(m_sdlWindow);
	SDL_Quit();
}

void Game::InitSDL() {
#ifdef RMLUI_PLATFORM_WIN32
	AllocConsole();
#endif

	int window_width = 1024;
	int window_height = 768;

	if (SDL_Init(SDL_INIT_VIDEO) != 0)
		throw std::runtime_error("failed to initialize sdl");

	m_sdlWindow = SDL_CreateWindow("AlchymeClient",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		window_width, window_height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	SDL_GLContext glcontext = SDL_GL_CreateContext(m_sdlWindow);
	int oglIdx = -1;
	int nRD = SDL_GetNumRenderDrivers();
	for (int i = 0; i < nRD; i++)
	{
		SDL_RendererInfo info;
		if (!SDL_GetRenderDriverInfo(i, &info))
		{
			if (!strcmp(info.name, "opengl"))
			{
				oglIdx = i;
			}
		}
	}

	if (oglIdx == -1)
		throw std::runtime_error("failed to initialize render drivers");

	m_sdlRenderer = SDL_CreateRenderer(m_sdlWindow, oglIdx, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
}

void Game::InitGLEW() {
	GLenum err = glewInit();

	if (err != GLEW_OK)
		throw std::runtime_error("failed to initialize glew");
		//fprintf(stderr, "GLEW ERROR: %s\n", glewGetErrorString(err));

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	glMatrixMode(GL_PROJECTION | GL_MODELVIEW);
	glLoadIdentity();

	int w = 0, h = 0;
	SDL_GetWindowSize(m_sdlWindow, &w, &h);

	//glOrtho(0, w, h, 0, 0, 1);
	glViewport(0, 0,
		w, h);

	// VSync Refresh rate, designated by 1
	SDL_GL_SetSwapInterval(1);
}

void Game::InitRML() {
	m_renderInterface = std::make_unique<MyRenderInterface>(m_sdlRenderer, m_sdlWindow);
	m_systemInterface = std::make_unique<MySystemInterface>();
	m_fileInterface = std::make_unique<MyFileInterface>("./data/client_data/");

	Rml::SetRenderInterface(m_renderInterface.get());
	Rml::SetSystemInterface(m_systemInterface.get());
	Rml::SetFileInterface(m_fileInterface.get());

	if (!Rml::Initialise())
		throw std::runtime_error("failed to initialize rml");

	int w = 0, h = 0;
	SDL_GetWindowSize(m_sdlWindow, &w, &h);

	m_rmlContext = Rml::CreateContext("default",
		Rml::Vector2i(w, h));

#ifndef NDEBUG
	if (!Rml::Debugger::Initialise(m_rmlContext))
		throw std::runtime_error("failed to initialize rml debugger");
#endif
}



void Game::Update(float delta) {
	// This is important to processing RPC remote invocations

	ScriptManager::Event::OnUpdate(delta);

	SDL_Event event;

	SDL_SetRenderDrawColor(m_sdlRenderer, 255, 255, 255, 255);
	SDL_RenderClear(m_sdlRenderer);

	m_rmlContext->Render();
	SDL_RenderPresent(m_sdlRenderer);

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_QUIT:
			Stop();
			return;

		case SDL_MOUSEMOTION:
			m_rmlContext->ProcessMouseMove(event.motion.x, event.motion.y, m_systemInterface->GetKeyModifiers());
			break;
		case SDL_MOUSEBUTTONDOWN:
			m_rmlContext->ProcessMouseButtonDown(m_systemInterface->TranslateMouseButton(event.button.button), m_systemInterface->GetKeyModifiers());
			break;

		case SDL_MOUSEBUTTONUP:
			m_rmlContext->ProcessMouseButtonUp(m_systemInterface->TranslateMouseButton(event.button.button), m_systemInterface->GetKeyModifiers());
			break;

		case SDL_MOUSEWHEEL:
			m_rmlContext->ProcessMouseWheel(float(event.wheel.y), m_systemInterface->GetKeyModifiers());
			break;

		case SDL_KEYDOWN: {
			// Intercept F8 key stroke to toggle RmlUi's visual debugger tool
			if (event.key.keysym.sym == SDLK_F8)
			{
				Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
				break;
			}

			auto k(m_systemInterface->TranslateKey(event.key.keysym.sym));
			auto m(m_systemInterface->GetKeyModifiers());

			m_rmlContext->ProcessKeyDown(k, m);
			break;
		}
		case SDL_KEYUP: {
			auto k(m_systemInterface->TranslateKey(event.key.keysym.sym));
			auto m(m_systemInterface->GetKeyModifiers());

			m_rmlContext->ProcessKeyUp(k, m);
			break;
		}
		case SDL_TEXTINPUT: {
			m_rmlContext->ProcessTextInput(Rml::String(event.text.text));
			break;
		}
		case SDL_WINDOWEVENT: {
			switch (event.window.event) {
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				auto w = event.window.data1;
				auto h = event.window.data2;
				m_rmlContext->SetDimensions(Rml::Vector2i(w, h));
				break;
			}
			break;
		}

		default:
			break;
		}
	}
	m_rmlContext->Update();
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
