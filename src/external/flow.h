// external/flow.h
#pragma once

#include <string>
#include <vector>
#include <mutex>

enum class FlowSink { MEMORY, FILE, BOTH };
enum class FlowScope { INIT, ENV, EXEC };
enum class FlowDomain { SHARED, MN, POS, SHIELD };

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

struct FlowStep {
    FlowScope   scope;
    FlowDomain  domain;
    std::string id;
    std::string label;
    std::string description;
    std::string source_file;
    std::string source_func = "";
};

class Flow {
public:
    static void Init(FlowSink sink, const std::string& filePath, bool append);
    static void Shutdown();
    static void Step(const FlowStep& step);
    static void Clear();
    static void Reset();
    static const std::vector<FlowStep>& Steps();
    static std::string ToJson();

private:
    static std::mutex            m_mutex;
    static std::vector<FlowStep> m_steps;
    static FlowSink              m_sink;
    static std::string           m_filePath;
    static bool                  m_initialized;

    static std::string buildJsonUnlocked();
    static void writeJsonFileUnlocked();
    static std::string escapeJson(const std::string& str);
    static std::string stepToJson(const FlowStep& step);
};