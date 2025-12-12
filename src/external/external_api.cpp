// src/external/external_api.cpp
#include "external_api.h"
#include "environment.h"
#include "logger.h"

// Direct functions implemented in other files:
extern "C" const char* pivx_external_mn_step();
extern "C" const char* pivx_external_pos_step();
extern "C" const char* pivx_external_shield_step();

extern "C" {

    PIVX_EXTERNAL_API void pivx_external_init()
    {
        static bool once = false;
        if (!once) {
            // ------------------------------------------------------------
            // Resolve log path
            // ------------------------------------------------------------
            const char* pathEnv = getenv("PIVX_UIX_LOG_PATH");
            std::string logPath = pathEnv ? pathEnv : "/tmp/pivx-uix.log";

            // ------------------------------------------------------------
            // Append vs truncate
            // ------------------------------------------------------------
            const char* appendEnv = getenv("PIVX_UIX_LOG_APPEND");
            bool append = true;  // default = append

            if (appendEnv && std::string(appendEnv) == "0") {
                append = false;
            }

            // ------------------------------------------------------------
            // Log level
            // ------------------------------------------------------------
            LogLevel level = LogLevel::INFO;
            const char* levelEnv = getenv("PIVX_UIX_LOG_LEVEL");
            if (levelEnv) {
                std::string lvl(levelEnv);
                if (lvl == "WARN")  level = LogLevel::WARN;
                if (lvl == "ERROR") level = LogLevel::ERROR;
            }

            // ------------------------------------------------------------
            // Init logger
            // ------------------------------------------------------------
            Logger::Init(
                LogSink::BOTH,
                level,
                logPath,
                append
            );

            LOG_INFO("INIT", std::string("Logger initialized (")
                               + (append ? "append" : "truncate")
                               + ") at " + logPath);

            once = true;
        }

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
