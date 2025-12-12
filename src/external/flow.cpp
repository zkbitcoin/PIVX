// external/flow.cpp
// ============================================================================
// Flow tracing implementation
// ============================================================================

#include "external/flow.h"

#include <fstream>

std::mutex                Flow::m_mutex;
std::vector<FlowStep>     Flow::m_steps;
FlowSink                  Flow::m_sink = FlowSink::MEMORY;
bool                      Flow::m_initialized = false;

static std::ofstream g_flowFile;

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

    if (sink == FlowSink::FILE || sink == FlowSink::BOTH) {
        std::ios::openmode mode = std::ios::out;
        mode |= append ? std::ios::app : std::ios::trunc;
        g_flowFile.open(filePath, mode);
    }

    m_initialized = true;
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------
void Flow::Shutdown()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (g_flowFile.is_open()) g_flowFile.close();
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

    if (m_sink == FlowSink::FILE || m_sink == FlowSink::BOTH) {
        writeToFile(step);
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
// File sink (line-based, UIX friendly)
// ---------------------------------------------------------------------------
void Flow::writeToFile(const FlowStep& step)
{
    if (!g_flowFile.is_open()) return;

    g_flowFile
        << static_cast<int>(step.scope)  << " | "
        << static_cast<int>(step.domain) << " | "
        << step.id          << " | "
        << step.label       << " | "
        << step.description << " | "
        << step.source_file << " | "
        << step.source_func
        << "\n";

    g_flowFile.flush();
}
