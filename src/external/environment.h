#pragma once

#include <stdint.h>

class CWallet;

// ============================================================================
// HYBRID INITIALIZATION MODEL
//
// Process-level (once per library load):
//   - ECC contexts (secp256k1)
//   - Chain parameters
//   - EvoDB + deterministic MN manager
//   - Mock block index
//
// Request-level (per HTTP request):
//   - Logger reset
//   - Flow reset
//   - Masternode list clear
// ============================================================================

// Process-level init (idempotent, thread-safe)
void init_process();

// Request lifecycle
void begin_request();
void end_request();

// Process-level shutdown (call at process exit only)
void shutdown_process();

// Utilities
void advance_time(int64_t sec);
CWallet& wallet();