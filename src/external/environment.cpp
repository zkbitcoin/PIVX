// external/environment.cpp
// ============================================================================
// Minimal mock environment for UIX demos
// - Initializes BOTH ECC contexts (VERIFY + SIGN)
// - Sets chain params (REGTEST)
// - Creates fake chainActive tip
// - Sets up EvoDB + deterministicMNManager
// ============================================================================

#include "environment.h"
#include "external/logger.h"

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

static CBlockIndex* g_genesis = nullptr;

// ============================================================================
// MOCK BLOCKCHAIN
// ============================================================================
static void mock_blockindex()
{
    LOG_INFO("ENV", "Creating mock blockchain");

    if (g_genesis) {
        LOG_INFO("ENV", "Mock blockchain already initialized");
        return;
    }

    // ------------------------------------------------------------
    // Fake previous block
    // ------------------------------------------------------------
    LOG_INFO("ENV", "Creating synthetic previous block");

    CBlockIndex* prev = new CBlockIndex();
    prev->nHeight = -1;
    prev->nTime   = GetTime() - 200;
    prev->nBits   = 0x1f00ffff;
    prev->nChainWork = 1;
    prev->pprev = nullptr;

    prev->SetStakeModifier(0x1111111111111111ULL, true);

    static uint256 prevHash = uint256S(
        "00000000000000000000000000000000000000000000000000000000000000"
    );
    prev->phashBlock = &prevHash;
    mapBlockIndex[prevHash] = prev;

    // ------------------------------------------------------------
    // Fake genesis block
    // ------------------------------------------------------------
    LOG_INFO("ENV", "Creating synthetic genesis block");

    g_genesis = new CBlockIndex();
    g_genesis->nHeight = 0;
    g_genesis->nTime   = GetTime() - 100;
    g_genesis->nBits   = prev->nBits;
    g_genesis->nChainWork = 2;
    g_genesis->pprev = prev;

    g_genesis->SetStakeModifier(0x2222222222222222ULL, true);

    static uint256 ghash = uint256S(
        "00000000000000000000000000000000000000000000000000000000000001"
    );
    g_genesis->phashBlock = &ghash;
    mapBlockIndex[ghash] = g_genesis;

    chainActive.SetTip(g_genesis);
    pindexBestHeader = g_genesis;

    LOG_INFO("ENV", "Mock blockchain tip set successfully");
}

// ============================================================================
// SPORK MOCK (no-op)
// ============================================================================
static void mock_sporks()
{
    LOG_WARN("ENV", "Sporks are disabled (demo environment)");
}

// ============================================================================
// MN MOCK (no-op)
// ============================================================================
static void mock_mn()
{
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

    // Logger should already be initialized by loader or test harness
    LOG_INFO("ENV", "Initializing mock environment");

    // ------------------------------------------------------------
    // ECC INITIALIZATION
    // ------------------------------------------------------------
    // VERIFY context: handled by static ECCVerifyHandle
    // SIGN context: MUST be initialized manually
    LOG_INFO("ENV", "Initializing ECC SIGN context");
    ECC_Start();

    // ------------------------------------------------------------
    // Chain parameters
    // ------------------------------------------------------------
    LOG_INFO("ENV", "Selecting REGTEST chain parameters");
    SelectParams(CBaseChainParams::REGTEST);
    gArgs.SoftSetArg("-datadir", "/tmp/pivx_mock_env");

    // ------------------------------------------------------------
    // EvoDB + deterministic MN manager
    // ------------------------------------------------------------
    LOG_INFO("ENV", "Initializing EvoDB and deterministic MN manager");

    g_evoDb = std::make_unique<CEvoDB>(1 << 20); // 1 MB cache
    deterministicMNManager = std::make_unique<CDeterministicMNManager>(*g_evoDb);

    // ------------------------------------------------------------
    // Mock subsystems
    // ------------------------------------------------------------
    mock_blockindex();
    mock_sporks();
    mock_mn();

    g_env_ready = true;
    LOG_INFO("ENV", "Environment initialization complete");
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
    LOG_WARN("ENV", "Dummy wallet accessed (linker stub only)");
    return *(CWallet*)&dummyWallet;
}
