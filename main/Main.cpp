#ifdef ESP_PLATFORM
#include <esp_pthread.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#endif

#include "VUtils.h"
#include "VUtilsResource.h"
#include "ValhallaServer.h"
#include "VUtilsRandom.h"
#include "CompileSettings.h"

#ifdef ESP_PLATFORM
extern "C" void app_main(void)
#else
int main(int argc, char **argv)
#endif
{
#ifdef RUN_TESTS
    static_assert("Tests not implemented!")
    //Tests().RunTests();
#else // !RUN_TESTS
    std::filesystem::current_path("./data/");

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
