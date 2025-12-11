// src/external/external_api.h
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

    // Platform-aware export macro so symbols end up in the dynamic symbol table
#if defined(_WIN32)
#define PIVX_EXTERNAL_API __declspec(dllexport)
#elif defined(__GNUC__)
#define PIVX_EXTERNAL_API __attribute__((visibility("default")))
#else
#define PIVX_EXTERNAL_API
#endif

    // Must be called before any others
    PIVX_EXTERNAL_API void pivx_external_init(void);

    // Masternode lifecycle step – returns JSON (owned by PIVX, do NOT free)
    PIVX_EXTERNAL_API const char* pivx_external_mn_step(void);

    // PoS staking kernel step – returns JSON
    PIVX_EXTERNAL_API const char* pivx_external_pos_step(void);

    // Shielded tx step – returns JSON
    PIVX_EXTERNAL_API const char* pivx_external_shield_step(void);

    // Optional shutdown/cleanup
    PIVX_EXTERNAL_API void pivx_external_shutdown(void);

#ifdef __cplusplus
}
#endif
