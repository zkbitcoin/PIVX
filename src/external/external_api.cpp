// src/external/external_api.cpp
#include "external_api.h"
#include <string>

// NOTE: stub implementation for wiring & build testing.
// We'll replace internals with real MN / PoS / Shield logic later.

static std::string g_last;

static const char* ret(const std::string& s) {
    g_last = s;
    return g_last.c_str();
}

void pivx_external_init(void) {
    // Later: SelectParams(REGTEST), minimal init, etc.
}

const char* pivx_external_mn_step(void) {
    return ret(R"({"ok":true,"module":"masternode","step":"stub"})");
}

const char* pivx_external_pos_step(void) {
    return ret(R"({"ok":true,"module":"pos","step":"stub"})");
}

const char* pivx_external_shield_step(void) {
    return ret(R"({"ok":true,"module":"shielded","step":"stub"})");
}

void pivx_external_shutdown(void) {
    // Later: cleanup if we allocate anything
}
