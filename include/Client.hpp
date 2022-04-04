#pragma once

#include "Game.hpp"
#include "NetPeer.hpp"
#include "MyRenderInterface.hpp"
#include "MySystemInterface.hpp"
#include "MyFileInterface.hpp"

namespace Alchyme {
	class Client : public Game {
		SDL_Window* m_sdlWindow = nullptr;
		SDL_GLContext* m_sdlGLContext = nullptr;
		SDL_Renderer* m_sdlRenderer = nullptr;

		Rml::Context* m_rmlContext = nullptr;
		std::unique_ptr<MyRenderInterface> m_renderInterface;
		std::unique_ptr<MySystemInterface> m_systemInterface;
		std::unique_ptr<MyFileInterface> m_fileInterface;

		bool serverAwaitingLogin = false;

		std::unique_ptr<Net::Peer> m_peer;

	public:
		ConnectMode m_mode = ConnectMode::STATUS;

	public:
		static Client* Get();

		Client();

		void Start() override;
		void Stop() override;

		void Connect(std::string host, std::string port);

		void SendLogin(std::string username, std::string key);

		void Disconnect();

	private:
		void Update(float delta) override;
		void ConnectCallback(Net::Peer* peer) override;
		void DisconnectCallback(Net::Peer* peer) override;

		void InitSDL();
		void InitGLEW();
		void InitRML();

		void RPC_ClientHandshake(Net::Peer* peer, int magic);
		void RPC_ModeStatus(Net::Peer* peer, std::string svTitle, std::string svDesc, std::chrono::seconds svCreateTime, std::chrono::seconds svStartTime, std::chrono::seconds svPrevUpDur, std::string svVer, size_t svConnections);
		void RPC_ModeLogin(Net::Peer* peer, std::string serverVersion);
		void RPC_PeerResult(Net::Peer* peer, PeerResult result);
		void RPC_PeerInfo(Net::Peer* peer, size_t peerUid, size_t worldSeed, size_t worldTime);
		void RPC_Print(Net::Peer* peer, std::string s);
		void RPC_Error(Net::Peer* peer, std::string s);
	};
}
