// src/external/external_api.cpp

#include "external_api.h"
#include "environment.h"
#include "logger.h"
#include "flow.h"

#include <cstdlib>
#include <string>

// Direct functions implemented in other files:
extern "C" const char* pivx_external_mn_step();
extern "C" const char* pivx_external_pos_step();
extern "C" const char* pivx_external_shield_step();

extern "C" {

    PIVX_EXTERNAL_API void pivx_external_init()
    {
        static bool once = false;

        // ============================================================
        // LOGGER (one-time init)
        // ============================================================
        if (!once) {
            const char* pathEnv = getenv("PIVX_UIX_LOG_PATH");
            std::string logPath = pathEnv ? pathEnv : "/tmp/pivx-uix.log";

            const char* appendEnv = getenv("PIVX_UIX_LOG_APPEND");
            bool logAppend = !(appendEnv && std::string(appendEnv) == "0");

            LogLevel level = LogLevel::INFO;
            if (const char* lvl = getenv("PIVX_UIX_LOG_LEVEL")) {
                std::string s(lvl);
                if (s == "WARN")  level = LogLevel::WARN;
                if (s == "ERROR") level = LogLevel::ERROR;
            }

            LogSink sink = LogSink::BOTH;
            if (const char* s = getenv("PIVX_UIX_LOG_SINK")) {
                std::string v(s);
                if (v == "MEMORY") sink = LogSink::CONSOLE;
                else if (v == "FILE") sink = LogSink::FILE;
                else if (v == "BOTH") sink = LogSink::BOTH;
            }

            Logger::Init(sink, level, logPath, logAppend);

            LOG_INFO(
                "INIT",
                std::string("Logger initialized (")
                + (logAppend ? "append" : "truncate")
                + ") sink="
                + (sink == LogSink::FILE ? "FILE" :
                   sink == LogSink::CONSOLE ? "MEMORY" : "BOTH")
                + " path=" + logPath
            );

            once = true;
        }

        // ============================================================
        // FLOW (per-process, but configurable per run)
        // ============================================================
        const char* flowPathEnv = getenv("PIVX_UIX_FLOW_PATH");
        std::string flowPath = flowPathEnv ? flowPathEnv : "/tmp/pivx-uix.flow";

        const char* flowAppendEnv = getenv("PIVX_UIX_FLOW_APPEND");
        bool flowAppend = !(flowAppendEnv && std::string(flowAppendEnv) == "0");

        FlowSink flowSink = FlowSink::BOTH;
        if (const char* s = getenv("PIVX_UIX_FLOW_SINK")) {
            std::string v(s);
            if (v == "MEMORY") flowSink = FlowSink::MEMORY;
            else if (v == "FILE") flowSink = FlowSink::FILE;
            else if (v == "BOTH") flowSink = FlowSink::BOTH;
        }

        Flow::Init(flowSink, flowPath, flowAppend);

        // ============================================================
        // Environment (idempotent)
        // ============================================================
        init_environment();
    }

    PIVX_EXTERNAL_API void pivx_external_shutdown()
    {
        Flow::Shutdown();
        Logger::Shutdown();
    }

    // ------------------- EXPORTED API -------------------

    PIVX_EXTERNAL_API const char* pivx_external_mn()
    {
        Flow::Clear();
        return pivx_external_mn_step();
    }

    PIVX_EXTERNAL_API const char* pivx_external_pos()
    {
        Flow::Clear();
        return pivx_external_pos_step();
    }

    PIVX_EXTERNAL_API const char* pivx_external_shield()
    {
        Flow::Clear();
        return pivx_external_shield_step();
    }
} // extern "C"
