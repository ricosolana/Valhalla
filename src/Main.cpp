#include "Client.hpp"
#include "Server.hpp"

#include <iostream>

INITIALIZE_EASYLOGGINGPP

// Time to make seperate client and server executables 
// once again
int main(int argc, char **argv) {

	// open file impl.txt

	Alchyme::Utils::initLogger();

	std::fstream f;
	f.open("./data/impl.txt");
	if (f.is_open()) {
		std::string line;
		std::getline(f, line);
		f.close();
		if (line == "client") {
			Alchyme::Game::RunClient();
		}
		else if (line == "server")
			Alchyme::Game::RunServer();
		else
			LOG(ERROR) << "Unknown impl in impl.txt (must be server or client)";
	} else
		LOG(ERROR) << "./data/impl.txt not found";

	return 0;
}
