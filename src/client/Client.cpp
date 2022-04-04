#include "Client.hpp"
#include <SDL.h>
#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include "ScriptManager.hpp"

namespace Alchyme {
	Client* Client::Get() {
		return static_cast<Client*>(Game::Get());
	}

	Client::Client() : Game(false) {}

	void Client::Stop() {
		Rml::Shutdown();
		SDL_DestroyRenderer(m_sdlRenderer);
		SDL_GL_DeleteContext(m_sdlGLContext);
		SDL_DestroyWindow(m_sdlWindow);
		SDL_Quit();

		StopIOThread();
		Game::Stop();
	}

	void Client::Start() {
		InitSDL();
		InitGLEW();
		InitRML();
		ScriptManager::Init();

		Game::Start();
	}

	void Client::InitSDL() {

#ifdef RMLUI_PLATFORM_WIN32
		AllocConsole();
#endif

		int window_width = 1024;
		int window_height = 768;

		SDL_Init(SDL_INIT_VIDEO);

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
		m_sdlRenderer = SDL_CreateRenderer(m_sdlWindow, oglIdx, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	}

	void Client::InitGLEW() {
		GLenum err = glewInit();

		if (err != GLEW_OK)
			fprintf(stderr, "GLEW ERROR: %s\n", glewGetErrorString(err));

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

	void Client::InitRML() {

		m_renderInterface = std::make_unique<MyRenderInterface>(m_sdlRenderer, m_sdlWindow);
		m_systemInterface = std::make_unique<MySystemInterface>();
		m_fileInterface = std::make_unique<MyFileInterface>("./data/client_data/");

		Rml::SetRenderInterface(m_renderInterface.get());
		Rml::SetSystemInterface(m_systemInterface.get());
		Rml::SetFileInterface(m_fileInterface.get());

		if (!Rml::Initialise())
			throw std::runtime_error("Rml failed to initialize");

		int w = 0, h = 0;
		SDL_GetWindowSize(m_sdlWindow, &w, &h);

		m_rmlContext = Rml::CreateContext("default",
			Rml::Vector2i(w, h));

#ifndef NDEBUG
		Rml::Debugger::Initialise(m_rmlContext);
#endif

	}



	void Client::SendLogin(std::string username, std::string key) {
		if (serverAwaitingLogin) {
			m_peer->m_name = username;
			m_peer->m_rpc->Invoke("LoginInfo", m_peer->m_name, key);
			serverAwaitingLogin = false;
		}
	}

	void Client::Disconnect() {
		if (m_peer)
			m_peer->Disconnect();
	}



	void Client::Connect(std::string host, std::string port) {
		if (m_peer)
			return;

		m_peer = std::make_unique<Net::Peer>(std::make_shared<Net::AsioSocket>(m_ctx));

		LOG(INFO) << "Non garbage: " << m_peer->m_rpc->not_garbage;

		asio::ip::tcp::resolver resolver(m_ctx);
		auto endpoints = resolver.resolve(asio::ip::tcp::v4(), host, port);

		m_peer->m_rpc->Register("ClientHandshake", new Net::Method(this, &Client::RPC_ClientHandshake));

		LOG(INFO) << "Connecting...";

		asio::async_connect(
			m_peer->m_rpc->m_socket->GetSocket(), endpoints.begin(), endpoints.end(),
			[this](const asio::error_code& ec, asio::ip::tcp::resolver::results_type::iterator it) {

			if (!ec) {
				m_peer->m_socket->Accept();

				RunTask([this] {
					ConnectCallback(m_peer.get());
				});
			}
			else {
				if (ec == asio::error::timed_out) {
					LOG(ERROR) << "Server took too long to respond";
				}
				else if (ec == asio::error::operation_aborted) {
					LOG(ERROR) << "Connect operation aborted";
				}
				else if (ec == asio::error::not_found) {
					LOG(ERROR) << "Cannot locate host";
				}
				else if (ec.value() == 10061) { // might be win32 only
					LOG(ERROR) << "Cannot locate host (win32)";
				}
				else {
					LOG(ERROR) << std::string("Unknown connection error: ") + ec.message();
				}

				RunTask([this]() {
					m_peer.reset();
					StopIOThread();
				});
			}
		});

		Game::StartIOThread();
	}

	void Client::Update(float delta) {
		// This is important to processing RPC remote invocations
		if (m_peer) {
			if (m_peer->IsOnline()) {
				m_peer->Update();
			}
			else {
				DisconnectCallback(m_peer.get());
				m_peer.reset();
			}
		}

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

	void Client::ConnectCallback(Net::Peer* peer) {
		LOG(INFO) << "Connected";
	}

	void Client::DisconnectCallback(Net::Peer* peer) {
		LOG(INFO) << "Disconnected";
		//StopIOThread();
		StopIOThread();
	}
}
