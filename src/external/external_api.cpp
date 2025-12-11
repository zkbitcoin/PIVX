// src/external/external_api.cpp
#include "external_api.h"
#include "environment.h"

// Direct functions implemented in other files:
extern "C" const char* pivx_external_mn_step();
extern "C" const char* pivx_external_pos_step();
extern "C" const char* pivx_external_shield_step();

extern "C" {

    PIVX_EXTERNAL_API void pivx_external_init()
    {
        init_environment();
    }

    PIVX_EXTERNAL_API void pivx_external_shutdown()
    {
        // Nothing to do for regtest/in-memory environment
    }

    // ------------------- EXPORTED API -------------------

    PIVX_EXTERNAL_API const char* pivx_external_mn()
    {
        return pivx_external_mn_step();
    }

    PIVX_EXTERNAL_API const char* pivx_external_pos()
    {
        return pivx_external_pos_step();
    }

    PIVX_EXTERNAL_API const char* pivx_external_shield()
    {
        return pivx_external_shield_step();
    }

} // extern "C"
