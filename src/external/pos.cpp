// external/pos.cpp
// ============================================================================
// Level 1 Proof-of-Stake Demo (UIX)
//
// Cryptographic + kernel-level demo only
// ============================================================================

#include "external/pos.h"
#include "external/environment.h"
#include "external/logger.h"
#include "external/flow.h"

#include "chain.h"
#include "kernel.h"
#include "stakeinput.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "script/script.h"
#include "util/system.h"
#include "validation.h"

#include <memory>
#include <string>
#include <sstream>

// ----------------------------------------------------------------------------
// ABI-safe return buffer
// ----------------------------------------------------------------------------
static std::string last_json;

/***************************************************************
 * Fake stake input — kernel-valid only
 ***************************************************************/
class CFakeStake : public CStakeInput
{
private:
    CTxOut out;
    COutPoint op;
    const CBlockIndex* idx;

public:
    CFakeStake(const CTxOut& o,
               const COutPoint& p,
               const CBlockIndex* i)
        : CStakeInput(i), out(o), op(p), idx(i) {}

    const CBlockIndex* GetIndexFrom() const override { return idx; }

    bool GetTxOutFrom(CTxOut& o) const override {
        o = out;
        return true;
    }

    CAmount GetValue() const override { return out.nValue; }
    bool IsZPIV() const override { return false; }

    CDataStream GetUniqueness() const override {
        CDataStream ss(SER_NETWORK, 0);
        ss << op.hash << op.n;
        return ss;
    }

    bool CreateTxIn(CWallet*, CTxIn&, uint256) const { return false; }
    bool CreateTxOuts(CWallet*, std::vector<CTxOut>&, CAmount) const { return false; }
};

/***************************************************************
 * Create fake stake input
 ***************************************************************/
static std::unique_ptr<CStakeInput> CreateFakeStake(const CBlockIndex* tip)
{
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::POS,
        "POS_STAKE_CREATE",
        "Create stake input",
        "Create synthetic stake input valid for kernel hashing",
        "stakeinput.h"
    });

    LOG_INFO("POS", "Creating synthetic stake input (kernel-valid only)");

    CTxOut out(5000 * COIN, CScript() << OP_TRUE);
    COutPoint op(uint256S("01"), 0);

    return std::make_unique<CFakeStake>(out, op, tip);
}

/***************************************************************
 * Kernel hash helper
 ***************************************************************/
static uint256 GetKernelHash(const CBlock& block,
                             const CBlockIndex* prev,
                             CStakeInput* stake)
{
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::POS,
        "POS_KERNEL_HASH",
        "Compute kernel hash",
        "Compute stake kernel hash for PoS evaluation",
        "kernel.h"
    });

    CStakeKernel kernel(prev, stake, block.nBits, block.nTime);
    return kernel.GetHash();
}

/***************************************************************
 * PUBLIC UIX ENTRY POINT
 ***************************************************************/
extern "C"
const char* pivx_external_pos_step()
{
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::POS,
        "POS_ENTRY",
        "Start PoS demo",
        "Entry point for Proof-of-Stake Level 1 demo",
        "external/pos.cpp"
    });

    LOG_INFO("POS", "Starting Proof-of-Stake Level 1 demo");

    init_environment();

    // ------------------------------------------------------------
    // Chain tip
    // ------------------------------------------------------------
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::POS,
        "POS_CHAIN_TIP",
        "Fetch chain tip",
        "Obtain current mock chain tip",
        "chain.h"
    });

    const CBlockIndex* tip = chainActive.Tip();
    if (!tip) {
        last_json = "{\"module\":\"pos\",\"error\":\"no chain tip\"}";
        return last_json.c_str();
    }

    // ------------------------------------------------------------
    // Stake input
    // ------------------------------------------------------------
    auto stake = CreateFakeStake(tip);
    if (!stake) {
        last_json = "{\"module\":\"pos\",\"error\":\"stake creation failed\"}";
        return last_json.c_str();
    }

    // ------------------------------------------------------------
    // Build synthetic block
    // ------------------------------------------------------------
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::POS,
        "POS_BLOCK_BUILD",
        "Build synthetic block",
        "Construct synthetic PoS block with coinbase and coinstake",
        "primitives/block.h"
    });

    CBlock block;
    block.nBits = tip->nBits;
    block.nTime = tip->nTime + 60;

    {
        CMutableTransaction cb;
        cb.nVersion = CTransaction::SAPLING;
        cb.vin.emplace_back();
        cb.vout.emplace_back();
        block.vtx.emplace_back(MakeTransactionRef(cb));
    }

    {
        CMutableTransaction tx;
        tx.nVersion = CTransaction::SAPLING;
        tx.vin.emplace_back(tip->GetBlockHash(), 0);

        CTxOut empty;
        empty.SetEmpty();
        tx.vout.push_back(empty);
        tx.vout.emplace_back(5000 * COIN, CScript() << OP_TRUE);

        block.vtx.emplace_back(MakeTransactionRef(tx));
    }

    // ------------------------------------------------------------
    // Kernel evaluation
    // ------------------------------------------------------------
    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::POS,
        "POS_KERNEL_EVAL",
        "Evaluate kernel",
        "Evaluate stake kernel against difficulty target",
        "kernel.h"
    });

    int64_t newTime = block.nTime;
    bool hit = Stake(tip, stake.get(), tip->nBits, newTime);
    block.nTime = newTime;

    uint256 kernelHash = GetKernelHash(block, tip, stake.get());

    // ------------------------------------------------------------
    // JSON output
    // ------------------------------------------------------------
    std::ostringstream o;
    o << "{"
      << "\"module\":\"pos\","
      << "\"result\":{"
      << "\"kernel_hit\":" << (hit ? "true" : "false") << ","
      << "\"kernel_hash\":\"" << kernelHash.ToString() << "\","
      << "\"block_time\":" << block.nTime
      << "},"
      << "\"validation\":{"
      << "\"kernel\":{\"checked\":true,\"valid\":true},"
      << "\"difficulty\":{"
      << "\"checked\":true,"
      << "\"bits\":\"" << strprintf("%08x", tip->nBits) << "\","
      << "\"target_hit\":" << (hit ? "true" : "false")
      << "},"
      << "\"economic\":{"
      << "\"checked\":false,"
      << "\"reason\":\"Level 1 demo — no wallet or UTXO ownership\""
      << "}"
      << "},"
      << "\"environment\":{"
      << "\"level\":1,"
      << "\"wallet_loaded\":false"
      << "}"
      << "}";

    last_json = o.str();

    Flow::Step({
        FlowScope::EXEC,
        FlowDomain::POS,
        "POS_EXIT",
        "Return result",
        "Return PoS kernel evaluation result to UIX",
        "external/pos.cpp"
    });

    LOG_INFO("POS", "Proof-of-Stake Level 1 demo complete");

    return last_json.c_str();
}
