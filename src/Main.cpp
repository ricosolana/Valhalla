// main.cpp
#define SOL_ALL_SAFETIES_ON 1

#include "VUtils.h"

#include "VUtilsResource.h"
#include "ValhallaServer.h"
#include "VUtilsRandom.h"
#include "CompileSettings.h"
#include "PrefabManager.h"

#include "Tests.h"

#ifdef ESP_PLATFORM
void app_main() {
#else
int main(int argc, char **argv) {
#endif

#ifdef RUN_TESTS
    Tests().RunTests();
#else // !RUN_TESTS
    fs::current_path("./data/");

#ifndef _DEBUG
    try {
#endif // _DEBUG
        Valhalla()->Start();
#ifndef _DEBUG
    }
    catch (const std::exception& e) {
        LOG_ERROR(LOGGER, "{}", e.what());
        return 1;
    }
#endif // _DEBUG

    return 0;
#endif // !RUN_TESTS
}
