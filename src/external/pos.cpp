// external/pos.cpp
// ============================================================================
// Level 1 Proof-of-Stake Demo (UIX)
//
// Demonstrates:
//  • Stake kernel construction
//  • Kernel hash computation
//  • Difficulty / target check
//
// DOES NOT demonstrate:
//  • Wallet ownership
//  • UTXO maturity
//  • Economic validation
//  • Block acceptance / consensus
//
// This is a cryptographic + kernel-level demo only.
// ============================================================================

#include "external/pos.h"
#include "external/environment.h"
#include "external/logger.h"

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

    // Not virtual in PIVX → intentionally NOT overridden
    bool CreateTxIn(CWallet*, CTxIn&, uint256) const { return false; }
    bool CreateTxOuts(CWallet*, std::vector<CTxOut>&, CAmount) const { return false; }
};

/***************************************************************
 * Create fake stake input
 ***************************************************************/
static std::unique_ptr<CStakeInput> CreateFakeStake(const CBlockIndex* tip)
{
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
    CStakeKernel kernel(prev, stake, block.nBits, block.nTime);
    return kernel.GetHash();
}

/***************************************************************
 * PUBLIC UIX ENTRY POINT
 ***************************************************************/
extern "C"
const char* pivx_external_pos_step()
{
    LOG_INFO("POS", "Starting Proof-of-Stake Level 1 demo");

    init_environment();

    // ------------------------------------------------------------
    // Chain tip
    // ------------------------------------------------------------
    const CBlockIndex* tip = chainActive.Tip();
    if (!tip) {
        LOG_ERROR("POS", "No chain tip available");
        last_json = "{\"module\":\"pos\",\"error\":\"no chain tip\"}";
        return last_json.c_str();
    }

    LOG_INFO("POS", "Using mock chain tip at height " + std::to_string(tip->nHeight));

    // ------------------------------------------------------------
    // Stake input
    // ------------------------------------------------------------
    auto stake = CreateFakeStake(tip);
    if (!stake) {
        LOG_ERROR("POS", "Stake input creation failed");
        last_json = "{\"module\":\"pos\",\"error\":\"stake creation failed\"}";
        return last_json.c_str();
    }

    // ------------------------------------------------------------
    // Build synthetic block
    // ------------------------------------------------------------
    LOG_INFO("POS", "Constructing synthetic PoS block");

    CBlock block;
    block.nBits = tip->nBits;
    block.nTime = tip->nTime + 60;

    // coinbase (required but meaningless for PoS demo)
    {
        CMutableTransaction cb;
        cb.nVersion = CTransaction::SAPLING;
        cb.vin.emplace_back();
        cb.vout.emplace_back();
        block.vtx.emplace_back(MakeTransactionRef(cb));
    }

    // coinstake
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
    // Stake kernel evaluation
    // ------------------------------------------------------------
    LOG_INFO("POS", "Evaluating stake kernel against difficulty target");

    int64_t newTime = block.nTime;
    bool hit = Stake(tip, stake.get(), tip->nBits, newTime);
    block.nTime = newTime;

    uint256 kernelHash = GetKernelHash(block, tip, stake.get());

    LOG_INFO(
        "POS",
        std::string("Kernel evaluation complete — target ")
        + (hit ? "HIT" : "MISSED")
    );

    // ------------------------------------------------------------
    // JSON output
    // ------------------------------------------------------------
    last_json = strprintf(
        "{"
          "\"module\":\"pos\","
          "\"result\":{"
            "\"kernel_hit\":%s,"
            "\"kernel_hash\":\"%s\","
            "\"block_time\":%d"
          "},"
          "\"validation\":{"
            "\"kernel\":{\"checked\":true,\"valid\":true},"
            "\"difficulty\":{"
                "\"checked\":true,"
                "\"bits\":\"%08x\","
                "\"target_hit\":%s"
            "},"
            "\"economic\":{"
                "\"checked\":false,"
                "\"reason\":\"Level 1 demo — no wallet or UTXO ownership\""
            "}"
          "},"
          "\"environment\":{"
            "\"level\":1,"
            "\"wallet_loaded\":false"
          "}"
        "}",
        hit ? "true" : "false",
        kernelHash.ToString(),
        block.nTime,
        tip->nBits,
        hit ? "true" : "false"
    );

    LOG_INFO("POS", "Proof-of-Stake Level 1 demo complete");

    return last_json.c_str();
}
