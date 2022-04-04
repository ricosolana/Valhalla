// AlchymeServer
//

#include "AlchymeGame.h"
#include "Utils.h"

INITIALIZE_EASYLOGGINGPP

int main(int argc, char** argv) 
{
    initLogger();

    AlchymeGame::RunServer();

	return 0;
}
