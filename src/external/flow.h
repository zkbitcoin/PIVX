// external/flow.h
// ============================================================================
// Flow tracing for UIX
//
// PURPOSE
// --------
// • Semantic execution tracing (NOT logging)
// • Drives diagrams (Mermaid), timelines, tooltips
// • UIX filters by scope + domain
// • Outputs JSON format for easy frontend parsing
//
// DESIGN
// ------
// • INIT / ENV are shared scopes
// • EXEC is per-feature (MN / POS / SHIELD)
// • Source metadata is OPTIONAL and DESCRIPTIVE
// • File output is valid JSON array
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
// Scope/Domain string conversion
// ---------------------------------------------------------------------------
inline const char* ScopeToString(FlowScope scope) {
    switch (scope) {
        case FlowScope::INIT: return "INIT";
        case FlowScope::ENV:  return "ENV";
        case FlowScope::EXEC: return "EXEC";
    }
    return "UNKNOWN";
}

inline const char* DomainToString(FlowDomain domain) {
    switch (domain) {
        case FlowDomain::SHARED: return "SHARED";
        case FlowDomain::MN:     return "MN";
        case FlowDomain::POS:    return "POS";
        case FlowDomain::SHIELD: return "SHIELD";
    }
    return "UNKNOWN";
}

// ---------------------------------------------------------------------------
// Flow step
// ---------------------------------------------------------------------------
struct FlowStep {
    FlowScope   scope;              // INIT / ENV / EXEC
    FlowDomain  domain;             // SHARED / MN / POS / SHIELD

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

    // Clear in-memory steps (per EXEC run)
    static void Clear();

    // -----------------------------------------------------------------------
    // Reset flow state (SAFE execution boundary)
    //
    // • Clears in-memory steps
    // • Truncates JSON flow file immediately
    // • Guarantees UI sees empty flow before first EXEC step
    //
    // This is stronger than Clear() and should be used at the start of
    // each external execution (e.g. pivx_external_init).
    // -----------------------------------------------------------------------
    static void Reset();

    // Get all steps as JSON string
    static std::string ToJson();

private:
    static std::mutex                m_mutex;
    static std::vector<FlowStep>     m_steps;
    static FlowSink                  m_sink;
    static std::string               m_filePath;
    static bool                      m_initialized;

    // Internal helpers (caller must hold mutex)
    static std::string buildJsonUnlocked();
    static void writeJsonFileUnlocked();

    // JSON formatting
    static std::string escapeJson(const std::string& str);
    static std::string stepToJson(const FlowStep& step);
};
