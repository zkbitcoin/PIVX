#include "external/logger.h"

#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

// ---------------------------------------------------------------------------
// Static members
// ---------------------------------------------------------------------------
std::mutex    Logger::m_mutex;
std::ofstream Logger::m_file;
LogLevel      Logger::m_level = LogLevel::INFO;
LogSink       Logger::m_sink  = LogSink::CONSOLE;
bool          Logger::m_initialized = false;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::string PadRight(const std::string& s, size_t width)
{
    if (s.size() >= width) return s.substr(0, width);
    return s + std::string(width - s.size(), ' ');
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
void Logger::Init(LogSink sink,
                  LogLevel level,
                  const std::string& filePath,
                  bool append)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized) return;

    m_sink  = sink;
    m_level = level;

    if (sink == LogSink::FILE || sink == LogSink::BOTH) {
        std::ios::openmode mode = std::ios::out;
        if (append) {
            mode |= std::ios::app;
        } else {
            mode |= std::ios::trunc;
        }

        m_file.open(filePath, mode);
        if (!m_file.is_open()) {
            std::cerr << "[LOGGER][ERROR] Cannot open log file: "
                      << filePath << std::endl;
        }
    }

    m_initialized = true;
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------
void Logger::Shutdown()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_file.is_open()) m_file.close();
    m_initialized = false;
}

// ---------------------------------------------------------------------------
// Level → string
// ---------------------------------------------------------------------------
const char* Logger::LevelToString(LogLevel level)
{
    switch (level) {
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
    }
    return "UNKNOWN";
}

// ---------------------------------------------------------------------------
// Log
// ---------------------------------------------------------------------------
void Logger::Log(LogLevel level,
                 const std::string& module,
                 const std::string& message)
{
    if (!m_initialized || level < m_level) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    // Timestamp
    auto now   = std::chrono::system_clock::now();
    auto now_t = std::chrono::system_clock::to_time_t(now);

    char timebuf[20];
    std::strftime(
        timebuf,
        sizeof(timebuf),
        "%Y-%m-%d %H:%M:%S",
        std::localtime(&now_t)
    );

    // Fixed-width columns for clean UI alignment
    std::string lvl = PadRight(LevelToString(level), 5);  // INFO / WARN
    std::string mod = PadRight(module, 6);                // ENV / INIT / MN / POS / SHIELD

    std::ostringstream line;
    line << "[" << timebuf << "] "
         << "[" << lvl << "] "
         << "[" << mod << "] "
         << message << "\n";

    if (m_sink == LogSink::CONSOLE || m_sink == LogSink::BOTH) {
        std::cerr << line.str();
    }

    if ((m_sink == LogSink::FILE || m_sink == LogSink::BOTH) && m_file.is_open()) {
        m_file << line.str();
        m_file.flush();
    }
}
