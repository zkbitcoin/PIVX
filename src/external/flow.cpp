// external/flow.cpp
// ============================================================================
// Flow tracing implementation - JSON output
// ============================================================================

#include "external/flow.h"

#include <fstream>
#include <sstream>
#include <iomanip>

std::mutex                Flow::m_mutex;
std::vector<FlowStep>     Flow::m_steps;
FlowSink                  Flow::m_sink = FlowSink::MEMORY;
std::string               Flow::m_filePath;
bool                      Flow::m_initialized = false;

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
void Flow::Init(FlowSink sink,
                const std::string& filePath,
                bool append)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) return;

    m_sink = sink;
    m_filePath = filePath;

    // For JSON output, we always write the complete file on shutdown
    // append mode is ignored for JSON (we rebuild the full array each time)
    (void)append;

    m_initialized = true;
}

// ---------------------------------------------------------------------------
// Shutdown - write final JSON file
// ---------------------------------------------------------------------------
void Flow::Shutdown()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_sink == FlowSink::FILE || m_sink == FlowSink::BOTH) {
        writeJsonFileUnlocked();
    }

    m_initialized = false;
}

// ---------------------------------------------------------------------------
// Clear in-memory steps (per EXEC run)
// ---------------------------------------------------------------------------
void Flow::Clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_steps.clear();
}

// ---------------------------------------------------------------------------
// Record a step
// ---------------------------------------------------------------------------
void Flow::Step(const FlowStep& step)
{
    if (!m_initialized) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_steps.push_back(step);

    // Write incrementally to file for real-time updates
    if (m_sink == FlowSink::FILE || m_sink == FlowSink::BOTH) {
        writeJsonFileUnlocked();
    }
}

// ---------------------------------------------------------------------------
// Accessor (UIX reads this)
// ---------------------------------------------------------------------------
const std::vector<FlowStep>& Flow::Steps()
{
    return m_steps;
}

// ---------------------------------------------------------------------------
// Escape string for JSON
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Convert single step to JSON object string
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Build JSON string from current steps (caller must hold mutex)
// ---------------------------------------------------------------------------
std::string Flow::buildJsonUnlocked()
{
    std::ostringstream o;
    o << "{\n";
    o << "  \"steps\": [\n";

    for (size_t i = 0; i < m_steps.size(); ++i) {
        o << stepToJson(m_steps[i]);
        if (i < m_steps.size() - 1) {
            o << ",";
        }
        o << "\n";
    }

    o << "  ]\n";
    o << "}\n";

    return o.str();
}

// ---------------------------------------------------------------------------
// Get all steps as JSON string (public, acquires lock)
// ---------------------------------------------------------------------------
std::string Flow::ToJson()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return buildJsonUnlocked();
}

// ---------------------------------------------------------------------------
// Write complete JSON file (caller must hold mutex)
// ---------------------------------------------------------------------------
void Flow::writeJsonFileUnlocked()
{
    if (m_filePath.empty()) return;

    std::ofstream file(m_filePath, std::ios::out | std::ios::trunc);
    if (!file.is_open()) return;

    file << buildJsonUnlocked();
    file.close();
}