// external/environment.cpp
// ============================================================================
// Hybrid initialization model for concurrent request safety
//
// Process-level (once):
//   - ECC contexts
//   - Chain parameters
//   - EvoDB + MN manager
//   - Mock block index
//
// Request-level (per HTTP request):
//   - Logger/Flow reset
//   - MN list clear
// ============================================================================

#include "environment.h"
#include "external/logger.h"
#include "external/flow.h"

#include "chainparams.h"
#include "chain.h"
#include "timedata.h"
#include "streams.h"
#include "spork.h"
#include "evo/evodb.h"
#include "evo/deterministicmns.h"
#include "masternodeman.h"
#include "validation.h"
#include "pubkey.h"
#include "key.h"

#include <memory>
#include <mutex>

// ============================================================================
// STATE
// ============================================================================
static std::mutex g_mutex;
static bool g_ecc_init = false;
static bool g_env_init = false;

static std::unique_ptr<ECCVerifyHandle> g_verify_handle;
static std::unique_ptr<CEvoDB> g_evoDb;
extern std::unique_ptr<CDeterministicMNManager> deterministicMNManager;

static CBlockIndex* g_genesis = nullptr;
static uint256 g_genesisHash;

// ============================================================================
// INTERNAL HELPERS
// ============================================================================
static void setup_mock_chain()
{
    if (g_genesis) return;

    Flow::Step({
        FlowScope::INIT,
        FlowDomain::SHARED,
        "INIT_MOCK_CHAIN",
        "Create mock blockchain",
        "Synthetic genesis block and chain tip",
        "environment.cpp"
    });

    g_genesis = new CBlockIndex();
    g_genesis->nHeight = 0;
    g_genesis->nTime = GetTime() - 100;
    g_genesis->nBits = 0x1f00ffff;
    g_genesis->nChainWork = 1;
    g_genesis->pprev = nullptr;
    g_genesis->SetStakeModifier(0x2222222222222222ULL, true);

    g_genesisHash = uint256S("0000000000000000000000000000000000000000000000000000000000000001");
    g_genesis->phashBlock = &g_genesisHash;
    mapBlockIndex[g_genesisHash] = g_genesis;

    chainActive.SetTip(g_genesis);
    pindexBestHeader = g_genesis;

    LOG_INFO("ENV", "Mock chain initialized");
}

// ============================================================================
// PROCESS-LEVEL INIT (idempotent, thread-safe)
// ============================================================================
void init_process()
{
    std::lock_guard<std::mutex> lock(g_mutex);

    if (!g_ecc_init) {
        Flow::Step({
            FlowScope::INIT,
            FlowDomain::SHARED,
            "INIT_ECC",
            "Initialize ECC",
            "secp256k1 sign/verify contexts",
            "key.h"
        });

        ECC_Start();
        g_verify_handle = std::make_unique<ECCVerifyHandle>();
        g_ecc_init = true;

        LOG_INFO("ENV", "ECC initialized");
    }

    if (!g_env_init) {
        Flow::Step({
            FlowScope::INIT,
            FlowDomain::SHARED,
            "INIT_ENV",
            "Initialize environment",
            "Chain params, EvoDB, mock chain",
            "environment.cpp"
        });

        SelectParams(CBaseChainParams::REGTEST);

        const std::string datadir = "/tmp/pivx_mock_env";
        fs::create_directories(datadir);
        gArgs.ForceSetArg("-datadir", datadir);
        GetDataDir(true);

        g_evoDb = std::make_unique<CEvoDB>(1 << 20);
        deterministicMNManager = std::make_unique<CDeterministicMNManager>(*g_evoDb);

        setup_mock_chain();

        g_env_init = true;

        LOG_INFO("ENV", "Environment initialized");
    }
}

// ============================================================================
// REQUEST LIFECYCLE
// ============================================================================
void begin_request()
{
    Logger::Reset();
    Flow::Reset();

    LOG_INFO("REQUEST", "Begin");

    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::SHARED,
        "REQUEST_BEGIN",
        "Request started",
        "Logger/Flow reset, MN list cleared",
        "environment.cpp"
    });

    mnodeman.Clear();
}

void end_request()
{
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::SHARED,
        "REQUEST_END",
        "Request complete",
        "Request processing finished",
        "environment.cpp"
    });

    LOG_INFO("REQUEST", "End");
}

// ============================================================================
// PROCESS-LEVEL SHUTDOWN
// ============================================================================
void shutdown_process()
{
    std::lock_guard<std::mutex> lock(g_mutex);

    LOG_INFO("ENV", "Shutdown started");

    if (g_env_init) {
        chainActive.SetTip(nullptr);
        pindexBestHeader = nullptr;

        if (!g_genesisHash.IsNull()) {
            mapBlockIndex.erase(g_genesisHash);
        }

        delete g_genesis;
        g_genesis = nullptr;
        g_genesisHash.SetNull();

        deterministicMNManager.reset();
        g_evoDb.reset();

        g_env_init = false;
    }

    if (g_ecc_init) {
        g_verify_handle.reset();
        ECC_Stop();
        g_ecc_init = false;
    }

    LOG_INFO("ENV", "Shutdown complete");
}

// ============================================================================
// UTILITIES
// ============================================================================
void advance_time(int64_t sec)
{
    std::lock_guard<std::mutex> lock(g_mutex);

    if (!g_genesis) {
        LOG_WARN("ENV", "advance_time: no genesis");
        return;
    }

    g_genesis->nTime += sec;
    LOG_INFO("ENV", "Time advanced by " + std::to_string(sec) + "s");
}

struct DummyWallet { int unused = 0; };
static DummyWallet g_dummy_wallet;

CWallet& wallet()
{
    LOG_WARN("ENV", "Dummy wallet accessed");
    return *(CWallet*)&g_dummy_wallet;
}