// external/flow.cpp

#include "external/flow.h"

#include <fstream>
#include <sstream>
#include <iomanip>

std::mutex            Flow::m_mutex;
std::vector<FlowStep> Flow::m_steps;
FlowSink              Flow::m_sink = FlowSink::MEMORY;
std::string           Flow::m_filePath;
bool                  Flow::m_initialized = false;

void Flow::Init(FlowSink sink, const std::string& filePath, bool append)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) return;

    m_sink = sink;
    m_filePath = filePath;
    (void)append;

    m_initialized = true;
}

void Flow::Shutdown()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_sink == FlowSink::FILE || m_sink == FlowSink::BOTH) {
        writeJsonFileUnlocked();
    }
}

void Flow::Clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_steps.clear();
}

void Flow::Reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_steps.clear();

    if (!m_filePath.empty()) {
        std::ofstream file(m_filePath, std::ios::out | std::ios::trunc);
        if (file.is_open()) {
            file << "{\n  \"steps\": []\n}\n";
            file.close();
        }
    }
}

void Flow::Step(const FlowStep& step)
{
    if (!m_initialized) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_steps.push_back(step);

    if (m_sink == FlowSink::FILE || m_sink == FlowSink::BOTH) {
        writeJsonFileUnlocked();
    }
}

const std::vector<FlowStep>& Flow::Steps()
{
    return m_steps;
}

std::string Flow::escapeJson(const std::string& str)
{
    std::ostringstream out;
    for (char c : str) {
        switch (c) {
            case '"':  out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b";  break;
            case '\f': out << "\\f";  break;
            case '\n': out << "\\n";  break;
            case '\r': out << "\\r";  break;
            case '\t': out << "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    out << "\\u" << std::hex << std::setfill('0')
                        << std::setw(4) << static_cast<int>(c);
                } else {
                    out << c;
                }
        }
    }
    return out.str();
}

std::string Flow::stepToJson(const FlowStep& step)
{
    std::ostringstream o;
    o << "    {\n";
    o << "      \"scope\": \""       << ScopeToString(step.scope)   << "\",\n";
    o << "      \"domain\": \""      << DomainToString(step.domain) << "\",\n";
    o << "      \"id\": \""          << escapeJson(step.id)          << "\",\n";
    o << "      \"label\": \""       << escapeJson(step.label)       << "\",\n";
    o << "      \"description\": \"" << escapeJson(step.description) << "\",\n";
    o << "      \"source\": \""      << escapeJson(step.source_file) << "\",\n";
    o << "      \"func\": \""        << escapeJson(step.source_func) << "\"\n";
    o << "    }";
    return o.str();
}

std::string Flow::buildJsonUnlocked()
{
    std::ostringstream o;
    o << "{\n  \"steps\": [\n";

    for (size_t i = 0; i < m_steps.size(); ++i) {
        o << stepToJson(m_steps[i]);
        if (i < m_steps.size() - 1) o << ",";
        o << "\n";
    }

    o << "  ]\n}\n";
    return o.str();
}

std::string Flow::ToJson()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return buildJsonUnlocked();
}

void Flow::writeJsonFileUnlocked()
{
    if (m_filePath.empty()) return;

    std::ofstream file(m_filePath, std::ios::out | std::ios::trunc);
    if (!file.is_open()) return;

    file << buildJsonUnlocked();
    file.close();
}