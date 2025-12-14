// external/external_api.cpp

#include "external_api.h"
#include "environment.h"
#include "logger.h"
#include "flow.h"

#include <cstdlib>
#include <mutex>
#include <string>

// Demo step functions
extern "C" const char* pivx_external_mn_step();
extern "C" const char* pivx_external_pos_step();
extern "C" const char* pivx_external_shield_step();

// ============================================================================
// STATE
// ============================================================================
static std::once_flag g_init_flag;
static std::mutex g_request_mutex;

// ============================================================================
// INTERNAL: One-time logger/flow setup
// ============================================================================
static void init_logging()
{
    const char* logPathEnv = getenv("PIVX_UIX_LOG_PATH");
    std::string logPath = logPathEnv ? logPathEnv : "/tmp/pivx-uix.log";

    LogLevel level = LogLevel::INFO;
    if (const char* lvl = getenv("PIVX_UIX_LOG_LEVEL")) {
        std::string s(lvl);
        if (s == "WARN")  level = LogLevel::WARN;
        if (s == "ERROR") level = LogLevel::ERROR;
    }

    LogSink logSink = LogSink::BOTH;
    if (const char* s = getenv("PIVX_UIX_LOG_SINK")) {
        std::string v(s);
        if (v == "CONSOLE") logSink = LogSink::CONSOLE;
        else if (v == "FILE") logSink = LogSink::FILE;
    }

    // append=false so Reset() truncates
    Logger::Init(logSink, level, logPath, false);

    const char* flowPathEnv = getenv("PIVX_UIX_FLOW_PATH");
    std::string flowPath = flowPathEnv ? flowPathEnv : "/tmp/pivx-uix.flow";

    FlowSink flowSink = FlowSink::BOTH;
    if (const char* s = getenv("PIVX_UIX_FLOW_SINK")) {
        std::string v(s);
        if (v == "MEMORY") flowSink = FlowSink::MEMORY;
        else if (v == "FILE") flowSink = FlowSink::FILE;
    }

    Flow::Init(flowSink, flowPath, false);
}

// ============================================================================
// EXPORTED API
// ============================================================================
extern "C" {

PIVX_EXTERNAL_API void pivx_external_init()
{
    std::call_once(g_init_flag, []() {
        init_logging();
        init_process();
    });
}

PIVX_EXTERNAL_API void pivx_external_shutdown()
{
    shutdown_process();
    Flow::Shutdown();
    Logger::Shutdown();
}

PIVX_EXTERNAL_API void pivx_external_begin_request()
{
    g_request_mutex.lock();
    begin_request();
}

PIVX_EXTERNAL_API void pivx_external_end_request()
{
    end_request();
    g_request_mutex.unlock();
}

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