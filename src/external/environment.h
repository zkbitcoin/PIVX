#pragma once

#include <stdint.h>

// Forward declaration
class CWallet;

//
// Initialize a minimal internal environment so that
// legacy masternode code works with *real PIVX logic*.
//
// This does NOT build a full node, blockchain, PoS kernel,
// or networking. It only mocks enough global state so that
// mnodeman.Add(), .Copy(), .Rank() etc function normally.
//
void init_environment();

//
// Cleanup mock environment before exit.
// Removes mock block indices from mapBlockIndex to prevent
// double-free in CMainCleanup destructor.
//
void cleanup_environment();

//
// Advance chain time (for PoS or ping simulation)
//
void advance_time(int64_t sec);

//
// Access mock wallet (created in-memory)
//
CWallet& wallet();