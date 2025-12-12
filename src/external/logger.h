#pragma once

#include <string>
#include <mutex>
#include <fstream>

enum class LogLevel {
    INFO,
    WARN,
    ERROR
};

enum class LogSink {
    CONSOLE,
    FILE,
    BOTH
};

class Logger {
public:
    // Initialize once (loader)
    static void Init(
        LogSink sink,
        LogLevel level,
        const std::string& filePath,
        bool append
    );

    static void Shutdown();

    static void Log(
        LogLevel level,
        const std::string& module,
        const std::string& message
    );

private:
    static const char* LevelToString(LogLevel level);

    static std::mutex        m_mutex;
    static std::ofstream     m_file;
    static LogLevel          m_level;
    static LogSink           m_sink;
    static bool              m_initialized;
};

#define LOG_INFO(module, msg)  Logger::Log(LogLevel::INFO,  module, msg)
#define LOG_WARN(module, msg)  Logger::Log(LogLevel::WARN,  module, msg)
#define LOG_ERROR(module, msg) Logger::Log(LogLevel::ERROR, module, msg)
