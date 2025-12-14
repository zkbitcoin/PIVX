// external/external_api.h
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#define PIVX_EXTERNAL_API __declspec(dllexport)
#elif defined(__GNUC__)
#define PIVX_EXTERNAL_API __attribute__((visibility("default")))
#else
#define PIVX_EXTERNAL_API
#endif

    // Process lifecycle
    PIVX_EXTERNAL_API void pivx_external_init(void);
    PIVX_EXTERNAL_API void pivx_external_shutdown(void);

    // Request lifecycle
    PIVX_EXTERNAL_API void pivx_external_begin_request(void);
    PIVX_EXTERNAL_API void pivx_external_end_request(void);

    // Demo steps
    PIVX_EXTERNAL_API const char* pivx_external_mn_step(void);
    PIVX_EXTERNAL_API const char* pivx_external_pos_step(void);
    PIVX_EXTERNAL_API const char* pivx_external_shield_step(void);

#ifdef __cplusplus
}
#endif