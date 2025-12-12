// external/flow.h
// ============================================================================
// Flow tracing for UIX
//
// PURPOSE
// --------
// • Semantic execution tracing (NOT logging)
// • Drives diagrams (Mermaid), timelines, tooltips
// • UIX filters by scope + domain
//
// DESIGN
// ------
// • INIT / ENV are shared scopes
// • EXEC is per-feature (MN / POS / SHIELD)
// • Source metadata is OPTIONAL and DESCRIPTIVE
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <mutex>

// ---------------------------------------------------------------------------
// Sink
// ---------------------------------------------------------------------------
enum class FlowSink {
    MEMORY,
    FILE,
    BOTH
};

// ---------------------------------------------------------------------------
// Scope (when does this step happen?)
// ---------------------------------------------------------------------------
enum class FlowScope {
    INIT,   // one-time process init
    ENV,    // shared runtime environment
    EXEC    // execution of MN / POS / SHIELD
};

// ---------------------------------------------------------------------------
// Domain (what feature area?)
// ---------------------------------------------------------------------------
enum class FlowDomain {
    SHARED,      // shared / cross-cutting infrastructure (ENV, INIT)
    MN,
    POS,
    SHIELD
};


// ---------------------------------------------------------------------------
// Flow step
// ---------------------------------------------------------------------------
struct FlowStep {
    FlowScope  scope;               // INIT / ENV / EXEC
    FlowDomain domain;              // GLOBAL / MN / POS / SHIELD

    std::string id;                 // Stable semantic ID (e.g. MN_KEYGEN)
    std::string label;              // Short UI label
    std::string description;        // Tooltip / explanation

    // Optional metadata (NOT identity)
    std::string source_file;        // e.g. external/mn.cpp
    std::string source_func = "";   // e.g. generate_keys
};

// ---------------------------------------------------------------------------
// Flow controller
// ---------------------------------------------------------------------------
class Flow {
public:
    static void Init(FlowSink sink,
                     const std::string& filePath,
                     bool append);

    static void Shutdown();

    static void Step(const FlowStep& step);

    static const std::vector<FlowStep>& Steps();
    static void Clear();

private:
    static std::mutex                m_mutex;
    static std::vector<FlowStep>     m_steps;
    static FlowSink                  m_sink;
    static bool                      m_initialized;

    static void writeToFile(const FlowStep& step);
};
