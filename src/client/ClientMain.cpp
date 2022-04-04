// AlchymeClient
//

#include "Game.h"
#include "Utils.h"

INITIALIZE_EASYLOGGINGPP

int main(int argc, char** argv)
{
    initLogger();

    Alchyme::Game::RunClient();

	return 0;
}