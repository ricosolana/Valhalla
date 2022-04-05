#include "Client.hpp"

#include <iostream>
#include "BinaryWriter.hpp"
#include <fstream>

INITIALIZE_EASYLOGGINGPP

// Time to make seperate client and server executables 
// once again
int main(int argc, char **argv) {

	// open file impl.txt

	Valhalla::Utils::initLogger();
	//Valhalla::Client::Run();

	std::string s = "Hello, world!";

	std::vector<unsigned char> vec;
	auto writer = BinaryWriter(vec);
	writer.Write(s);

	std::ofstream wf("testc.dat", std::ios::out | std::ios::binary);

	wf.write(reinterpret_cast<const char*>(vec.data()), vec.size());

	wf.close();

	return 0;
}
