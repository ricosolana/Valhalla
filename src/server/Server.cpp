#include <algorithm>
#include "Server.hpp"
#include <string>
#ifdef _WIN32
#include <windows.h>
#endif

namespace Alchyme {
	using namespace std::chrono_literals;

	static bool loadSettings(robin_hood::unordered_map<std::string, std::string>& settings) {
		std::ifstream file;

		//file.open("C:\\Users\\Rico\\Documents\\VisualStudio2019\\Projects\\Alchyme\\AlchymeServer\\data\\settings.txt");
		file.open("./data/server_data/settings.txt");
		//file.open("data\\settings.txt");
		if (file.is_open()) {
			std::string line;
			while (std::getline(file, line)) {
				size_t index = line.find(':');

				std::string key = line.substr(0, index);
				std::string value = line.substr(index + 2);
				settings.insert({ key, value });
			}

			file.close();
			return true;
		}
		else {
			return false;
		}
	}

#define SETTING(key, def) (m_settings.emplace(key, def).first->second)

	Server* Server::Get() {
		return static_cast<Server*>(Game::Get());
	}

	Server::Server()
		: Game(true) {}

	static std::string consoleInput() {
#ifdef _WIN32
		SetThreadDescription(GetCurrentThread(), L"ConsoleInputThread");
#endif
		std::string in;
		std::getline(std::cin, in);
		return in;
	}

	void Server::Start() {
		consoleFuture = std::async(consoleInput);

		if (!loadSettings(m_settings))
			throw std::runtime_error("could not load settings.txt");

		m_useWhitelist = SETTING("use-whitelist", "false") == "true";
		m_maxPeers = std::stoi(SETTING("max-peers", "10"));
		m_passwordHash = Utils::StrHash(SETTING("password", "10").c_str());

		m_serverTitle = SETTING("server-title", "My Alchyme server");
		m_serverDesc = SETTING("server-desc", "the best server");
		m_serverCreateTime = std::chrono::seconds(std::stoi(SETTING("server-creation-time", "0")));
		m_serverStartTime = std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::steady_clock::now().time_since_epoch());
		m_serverPrevUpDur = std::chrono::seconds(std::stoi(SETTING("server-up-duration", "0")));

		LoadUsers();

		asio::ip::port_type port = std::stoi(SETTING("port", "8001"));
		m_acceptor = std::make_unique<tcp::acceptor>(m_ctx, tcp::endpoint(tcp::v4(), port));
		LOG(INFO) << "Starting server on *:" << port;
		DoAccept();

		RunTaskLaterRepeat([this]() {
			for (auto& peer : m_peers) {

				// test for online and nullifying or deleting
				// once peer has been validated and is 'playing' on the server
				if (peer->m_authorized) {
					PeerResult res;
					if ((res = PeerAllowedToStay(peer.get())) != PeerResult::OK)
						Kick(peer.get());
				}
			}
		}, 0s, 5s);

		Game::StartIOThread();
		Game::Start(); // Will begin loop
	}

	void Server::Stop() {
		Game::Stop();
	}



	void Server::DoAccept() {
		m_acceptor->async_accept(
			[this](const asio::error_code& ec, tcp::socket socket) {
			// Ran on io thread
			if (!ec) {
				auto sock = std::make_shared<Net::AsioSocket>(m_ctx, std::move(socket));

				sock->Accept();

				RunTask([this, sock]() {
					auto peer = std::make_unique<Net::Peer>(sock);
					ConnectCallback(peer.get());
					m_peers.push_back(std::move(peer));
				});
			}
			else {
				LOG(ERROR) << ec.message();
			}

			DoAccept();
		});
	}

	/**
	 * @brief Matches expected arguments determined at compile time aainst a runtime command and related arguments
	 * @param least | Minimum required argument count
	 * @param args | The arguments to test against
	 * @param expected | The acceptable args
	 * @return
	*/
	static int CMD_MATCH_ARG(int least, std::vector<std::string_view>& args, std::initializer_list<std::string_view> expected) {
		assert(least >= 1 && "Expects at least 1 arguments");

		if (args.size() < least) {
			LOG(INFO) << "Input at least " << least - 1 << " arguments";
			//LOG(INFO) << "Input more arguments: " << Utils::join(", ", expected);
			return -1;
		}

		const auto size = expected.size();

		for (auto i = (decltype(size))0; i < size; i++) {
			if ((expected.begin() + i)->_Equal(args[least - 1])) {
				return i;
			}
		}

		//if (least == 1) {
		//	LOG(INFO) << "Unknown command";
		//} else 
		//	LOG(INFO) << "Illegal argument";

		LOG(ERROR) << Utils::join(", ", expected);

		return -1;
	}

	static bool CMD_MIN_ARG(int least, std::vector<std::string_view>& args) {
		assert(least >= 1 && "Expects at least 1 arguments");

		if (args.size() < least) {
			LOG(INFO) << "Input at least " << least - 1 << " arguments";
			return false;
		}

		return true;
	}

	void Server::Update(float dt) {
		for (auto& peer : m_peers) {
			peer->Update();
		}

		if (consoleFuture.wait_for(0s) == std::future_status::ready) {
			std::string in = consoleFuture.get();

			auto split = Utils::split(in, ' ');

			int index = CMD_MATCH_ARG(1, split, { "ipban", "unipban", "whitelist" });
			switch (index)
			{
			case 0: if (CMD_MIN_ARG(2, split)) if (Utils::isAddress(split[1])) { LOG(INFO) << "Banned ip " << split[1]; Ban(std::string(split[1])); }
				  else LOG(ERROR) << "Not an address"; break;
			case 1: if (CMD_MIN_ARG(2, split)) if (Utils::isAddress(split[1])) { if (UnBan(std::string(split[1]))) LOG(INFO) << "Unbanned ip " << split[1]; else LOG(INFO) << split[1] << " was never ip banned"; }
				  else LOG(ERROR) << "Not an address"; break;
			case 2: {
				index = CMD_MATCH_ARG(2, split, { "on", "off", "add", "remove" });
				switch (index) {
				case 0: LOG(INFO) << "Enabled the whitelist"; m_useWhitelist = true; break;
				case 1: LOG(INFO) << "Disabled the whitelist"; m_useWhitelist = true; break;
				case 2: if (CMD_MIN_ARG(3, split)) { AddToWhitelist(std::string(split[2])); LOG(INFO) << "Added " << split[2] << " to the whitelist"; } break;
				case 3: if (CMD_MIN_ARG(3, split)) { if (RemoveFromWhitelist(std::string(split[2]))) LOG(INFO) << "Removed " << split[2] << " from the whitelist"; else LOG(INFO) << split[2] << " was never whitelisted"; } break;
				}
			}
			default:
				break;
			}

			consoleFuture = std::async(consoleInput);
		}
	}



	void Server::ConnectCallback(NOTNULL Net::Peer* peer) {
		LOG(INFO) << peer->m_socket->GetHostName() << " has connected";

		LOG(INFO) << "Registered ServerHandshake()";
		peer->m_rpc->Register("ServerHandshake", new Net::Method(this, &Server::RPC_ServerHandshake));

		peer->m_rpc->Invoke("ClientHandshake", MAGIC);
	}

	void Server::DisconnectCallback(NOTNULL Net::Peer* peer) {
		LOG(INFO) << peer->m_socket->GetHostName() << " has disconnected";
		// experiment with this
	}

	NULLABLE Net::Peer* Server::GetPeer(size_t uid) {
		for (auto&& peer : m_peers) {
			if (peer->m_uid == uid)
				return peer.get();
		}
		return nullptr;
	}

	NULLABLE Net::Peer* Server::GetPeer(std::string_view name) {
		for (auto&& peer : m_peers) {
			if (peer->m_name == name)
				return peer.get();
		}
		return nullptr;
	}

	NOTNULL Net::Peer* Server::GetPeer(NOTNULL Net::Rpc* rpc) {
		assert(rpc && "Rpc must not be null");
		for (auto&& peer : m_peers) {
			if (peer->m_rpc && peer->m_rpc.get() == rpc)
				return peer.get();
		}
		// throw
		throw std::runtime_error("This point should not be reached");
	}



	PeerResult Server::PeerAllowedToJoin(Net::Peer* peer,
		std::string version,
		size_t passwordHash) {

		// check the most common conditions first

		if (passwordHash != m_passwordHash)
			return PeerResult::WRONG_PASSWORD;

		if (version != VERSION)
			return PeerResult::WRONG_VERSION;

		if (m_peers.size() >= m_maxPeers)
			return PeerResult::MAX_PEERS;

		if (m_useWhitelist && !m_whitelist.contains(peer->m_name))
			return PeerResult::NOT_WHITELISTED;

		if (m_banned.contains(peer->m_socket->GetHostName()))
			return PeerResult::BANNED;

		if (GetPeer(peer->m_uid))
			return PeerResult::UID_ALREADY_ONLINE;

		// on the border of whether duplicate names should be allowed
		// probably shouldnt be allowed
		if (GetPeer(peer->m_name))
			return PeerResult::NAME_ALREADY_ONLINE;

		return PeerResult::OK;
	}

	PeerResult Server::PeerAllowedToStay(Net::Peer* peer) {
		if (m_useWhitelist && !m_whitelist.contains(peer->m_name))
			return PeerResult::NOT_WHITELISTED;

		if (m_banned.contains(peer->m_socket->GetHostName()))
			return PeerResult::BANNED;

		return PeerResult::OK;
	}



	void Server::Ban(const std::string& host) {
		m_banned.insert(host);
	}

	void Server::AddToWhitelist(const std::string& name) {
		m_whitelist.insert(name);
	}

	bool Server::UnBan(const std::string& host) {
		return static_cast<bool>(m_banned.erase(host));
	}

	bool Server::RemoveFromWhitelist(const std::string& name) {
		return static_cast<bool>(m_whitelist.erase(name));
	}

	bool Server::IsBanned(const std::string& host) {
		return m_banned.contains(host);
	}

	/// TODO Move this to AlchymeServer
	void Server::Kick(Net::Peer* peer, std::string reason) {
		peer->m_rpc->Invoke("KickNotify", reason);
		DisconnectLater(peer);
	}



	void Server::SaveUsers() {
		std::ofstream myfile;
		myfile.open("data\\banned.txt", std::ios::out);
		if (myfile.is_open()) {
			for (auto&& s : m_banned) {
				myfile << s << "\n";
			}
			myfile.close();
		}

		myfile.open("data\\whitelist.txt", std::ios::out);
		if (myfile.is_open()) {
			for (auto&& s : m_whitelist) {
				myfile << s << "\n";
			}
			myfile.close();
		}
	}

	void Server::LoadUsers() {
		std::ifstream myfile;
		myfile.open("data\\banned.txt", std::ios::in);
		if (myfile.is_open()) {
			std::string line;
			while (std::getline(myfile, line)) {
				m_banned.insert(line);
			}
			myfile.close();
		}

		myfile.open("data\\whitelist.txt", std::ios::in);
		if (myfile.is_open()) {
			std::string line;
			while (std::getline(myfile, line)) {
				m_whitelist.insert(line);
			}
			myfile.close();
		}
	}
}
