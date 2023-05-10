// main.cpp
#define SOL_ALL_SAFETIES_ON 1

//#define WIN32_LEAN_AND_MEAN
//#include <WinSock2.h>

#include "VUtils.h"

#include "VUtilsResource.h"
#include "ValhallaServer.h"
#include "VUtilsRandom.h"
#include "CompileSettings.h"
#include "PrefabManager.h"

#include "Tests.h"

/*
* Example command line args:
*   .\Valhalla.exe -vmodule=VUtilsResource=1
*   .\Valhalla.exe --no-colors "-vmodule=Peer=2,PrefabManager=2"
*   .\Valhalla.exe --no-log-backup --v=2
*   .\Valhalla.exe -v
*/
int main(int argc, char **argv) {
#ifdef RUN_TESTS
    Tests().RunTests();
#else // !RUN_TESTS
    tracy::SetThreadName("main");
    
    fs::current_path("./data/");
    
    {
        std::string path = (fs::current_path() / VH_LUA_PATH).string();
        std::string path2 = (fs::current_path() / VH_MOD_PATH).string();
        std::string env = "LUA_PATH=" 
            + path + "/?.lua;" 
            + path + "/?/?.lua;"
            + path2 + "/?.lua;"
            + path2 + "/?/?.lua";
        putenv(env.c_str());
    }

    {
        std::string path = (fs::current_path() / VH_LUA_CPATH).string();
        std::string env = "LUA_CPATH=" 
            + path + "/?.dll;" 
            + path + "/?/?.dll";
        putenv(env.c_str());
    }

#ifndef _DEBUG
    try {
#endif // _DEBUG
        Valhalla()->Start();
#ifndef _DEBUG
    }
    catch (const std::exception& e) {
        LOG(ERROR) << e.what();
        return 1;
    }
#endif // _DEBUG

    return 0;
#endif // !RUN_TESTS
}
