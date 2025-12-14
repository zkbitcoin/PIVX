// external/environment.cpp
// ============================================================================
// Minimal mock environment for UIX demos
// - Initializes BOTH ECC contexts (VERIFY + SIGN)
// - Sets chain params (REGTEST)
// - Creates fake chainActive tip
// - Sets up EvoDB + deterministicMNManager
//
// Emits FLOW steps in INIT / ENV scope shared by:
//   • Masternode
//   • Proof-of-Stake
//   • Shielded (Sapling)
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
#include "validation.h"

#include "pubkey.h"   // ECCVerifyHandle
#include "key.h"      // ECC_Start / ECC_Stop

#include <memory>

// ============================================================================
// ECC (VERIFY context must be static lifetime)
// ============================================================================
static ECCVerifyHandle g_verify_handle;

// ============================================================================
// GLOBALS
// ============================================================================
static bool g_env_ready = false;

static std::unique_ptr<CEvoDB> g_evoDb;
extern std::unique_ptr<CDeterministicMNManager> deterministicMNManager;

// Track our mock blocks for cleanup
static CBlockIndex* g_genesis = nullptr;
static uint256 g_genesisHash;

// ============================================================================
// MOCK BLOCKCHAIN
// ============================================================================
static void mock_blockindex()
{
    Flow::Step({
        FlowScope::ENV,
        FlowDomain::SHARED,
        "ENV_CHAIN_INIT",
        "Initialize mock blockchain",
        "Create synthetic block index and chainActive tip",
        "external/environment.cpp"
    });

    LOG_INFO("ENV", "Creating mock blockchain");

    if (g_genesis) {
        LOG_INFO("ENV", "Mock blockchain already initialized");
        return;
    }

    // ------------------------------------------------------------
    // Fake genesis block (height 0, no pprev)
    // ------------------------------------------------------------
    Flow::Step({
        FlowScope::ENV,
        FlowDomain::SHARED,
        "ENV_GENESIS_BLOCK",
        "Create genesis block",
        "Create synthetic genesis block for mock chain",
        "chain.h"
    });

    g_genesis = new CBlockIndex();
    g_genesis->nHeight = 0;
    g_genesis->nTime   = GetTime() - 100;
    g_genesis->nBits   = 0x1f00ffff;
    g_genesis->nChainWork = 1;
    g_genesis->pprev = nullptr;  // Genesis has no parent!

    g_genesis->SetStakeModifier(0x2222222222222222ULL, true);

    g_genesisHash = uint256S(
        "00000000000000000000000000000000000000000000000000000000000001"
    );
    g_genesis->phashBlock = &g_genesisHash;
    mapBlockIndex[g_genesisHash] = g_genesis;

    // SetTip walks back via pprev - with pprev=nullptr it stops at genesis
    chainActive.SetTip(g_genesis);
    pindexBestHeader = g_genesis;

    LOG_INFO("ENV", "Mock blockchain tip set successfully");
}

// ============================================================================
// SPORK MOCK (no-op)
// ============================================================================
static void mock_sporks()
{
    Flow::Step({
        FlowScope::ENV,
        FlowDomain::SHARED,
        "ENV_SPORKS_DISABLED",
        "Disable sporks",
        "Spork subsystem disabled in demo environment",
        "spork.h"
    });

    LOG_WARN("ENV", "Sporks are disabled (demo environment)");
}

// ============================================================================
// MN MOCK (no-op)
// ============================================================================
static void mock_mn()
{
    Flow::Step({
        FlowScope::ENV,
        FlowDomain::SHARED,
        "ENV_MN_MOCK",
        "Mock masternode subsystem",
        "Masternode networking and wallet disabled",
        "masternodeman.h"
    });

    LOG_INFO("ENV", "Masternode subsystem mocked (no network / no wallet)");
}

// ============================================================================
// INIT ENVIRONMENT (IDEMPOTENT, SAFE)
// ============================================================================
void init_environment()
{
    if (g_env_ready) {
        LOG_INFO("ENV", "Environment already initialized");
        return;
    }

    Flow::Step({
        FlowScope::INIT,
        FlowDomain::SHARED,
        "ENV_INIT_START",
        "Initialize environment",
        "Begin mock environment initialization",
        "external/environment.cpp"
    });

    LOG_INFO("ENV", "Initializing mock environment");

    // ------------------------------------------------------------
    // ECC INITIALIZATION
    // ------------------------------------------------------------
    Flow::Step({
        FlowScope::INIT,
        FlowDomain::SHARED,
        "ENV_ECC_INIT",
        "Initialize ECC",
        "Initialize secp256k1 VERIFY and SIGN contexts",
        "key.h"
    });

    ECC_Start();

    // ------------------------------------------------------------
    // Chain parameters
    // ------------------------------------------------------------
    Flow::Step({
        FlowScope::INIT,
        FlowDomain::SHARED,
        "ENV_CHAIN_PARAMS",
        "Select REGTEST params",
        "Select REGTEST chain parameters",
        "chainparams.h"
    });

    SelectParams(CBaseChainParams::REGTEST);

    // ------------------------------------------------------------
    // Force datadir resolution BEFORE EvoDB
    // ------------------------------------------------------------
    const std::string datadir = "/tmp/pivx_mock_env";
    fs::create_directories(datadir);

    // IMPORTANT: set BEFORE GetDataDir() is used
    gArgs.ForceSetArg("-datadir", datadir);

    // Forces internal path caching
    GetDataDir(true);

    LOG_INFO("ENV", "Resolved datadir: " + GetDataDir().string());

    // ------------------------------------------------------------
    // EvoDB + deterministic MN manager
    // ------------------------------------------------------------
    Flow::Step({
        FlowScope::INIT,
        FlowDomain::SHARED,
        "ENV_EVODB_INIT",
        "Initialize EvoDB",
        "Initialize EvoDB and deterministic masternode manager",
        "evo/evodb.h"
    });

    g_evoDb = std::make_unique<CEvoDB>(1 << 20); // 1 MB cache
    deterministicMNManager = std::make_unique<CDeterministicMNManager>(*g_evoDb);

    // ------------------------------------------------------------
    // Mock subsystems
    // ------------------------------------------------------------
    mock_blockindex();
    mock_sporks();
    mock_mn();

    g_env_ready = true;

    Flow::Step({
        FlowScope::INIT,
        FlowDomain::SHARED,
        "ENV_INIT_DONE",
        "Environment ready",
        "Mock environment initialization complete",
        "external/environment.cpp"
    });

    LOG_INFO("ENV", "Environment initialization complete");
}

// ============================================================================
// CLEANUP ENVIRONMENT
// Must be called before process exit to prevent double-free in CMainCleanup
// ============================================================================
void cleanup_environment()
{
    if (!g_env_ready) {
        return;
    }

    LOG_INFO("ENV", "Cleaning up mock environment");

    // Clear chain active tip first
    chainActive.SetTip(nullptr);
    pindexBestHeader = nullptr;

    // Remove our entry from mapBlockIndex (don't let CMainCleanup delete it)
    mapBlockIndex.erase(g_genesisHash);

    // Delete our block index ourselves
    delete g_genesis;
    g_genesis = nullptr;

    // Clear hash
    g_genesisHash.SetNull();

    // Reset MN manager and EvoDB
    deterministicMNManager.reset();
    g_evoDb.reset();

    g_env_ready = false;

    LOG_INFO("ENV", "Environment cleanup complete");
}

// ============================================================================
// ADVANCE TIME (OPTIONAL)
// ============================================================================
void advance_time(int64_t sec)
{
    if (!g_genesis) {
        LOG_WARN("ENV", "advance_time called before genesis initialization");
        return;
    }

    Flow::Step({
        FlowScope::ENV,
        FlowDomain::SHARED,
        "ENV_TIME_ADVANCE",
        "Advance mock time",
        "Advance mock chain time",
        "chain.h"
    });

    g_genesis->nTime += sec;

    LOG_INFO("ENV", "Advanced mock chain time by " + std::to_string(sec) + " seconds");
}

// ============================================================================
// DUMMY WALLET STUB (LINKER ONLY)
// ============================================================================
struct DummyWalletStruct { int unused = 0; };
static DummyWalletStruct dummyWallet;

CWallet& wallet()
{
    Flow::Step({
        FlowScope::ENV,
        FlowDomain::SHARED,
        "ENV_WALLET_STUB",
        "Access dummy wallet",
        "Wallet stub accessed (linker-only, no real wallet)",
        "wallet.h"
    });

    LOG_WARN("ENV", "Dummy wallet accessed (linker stub only)");
    return *(CWallet*)&dummyWallet;
}