#pragma once

#include "../core/types.h"
#include "../core/move.h"
#include <vector>
#include <cstring>

namespace chess {

enum TTFlag : uint8_t {
    TT_NONE  = 0,
    TT_EXACT = 1,
    TT_ALPHA = 2, // Upper bound (failed low)
    TT_BETA  = 3, // Lower bound (failed high)
};

struct TTEntry {
    uint64_t key;
    int16_t  score;
    int16_t  depth;
    uint8_t  flag;
    uint16_t best_move; // raw move data

    Move get_move() const {
        Move m;
        // Reconstruct from raw - Move is just a uint16_t wrapper
        if (best_move == 0) return Move::none();
        return *reinterpret_cast<const Move*>(&best_move);
    }
};

class TranspositionTable {
public:
    TranspositionTable(size_t mb = 64) {
        resize(mb);
    }

    void resize(size_t mb) {
        size_t bytes = mb * 1024 * 1024;
        num_entries_ = bytes / sizeof(TTEntry);
        table_.resize(num_entries_);
        clear();
    }

    void clear() {
        std::memset(table_.data(), 0, table_.size() * sizeof(TTEntry));
    }

    bool probe(uint64_t key, TTEntry& entry) const {
        size_t idx = key % num_entries_;
        entry = table_[idx];
        return entry.key == key && entry.flag != TT_NONE;
    }

    void store(uint64_t key, int score, int depth, TTFlag flag, Move best) {
        size_t idx = key % num_entries_;
        TTEntry& e = table_[idx];
        // Always replace (simple scheme)
        // Prefer deeper entries unless new is exact
        if (e.key == key && e.depth > depth && flag != TT_EXACT && e.flag == TT_EXACT)
            return;
        e.key = key;
        e.score = int16_t(score);
        e.depth = int16_t(depth);
        e.flag = flag;
        e.best_move = best.raw();
    }

private:
    std::vector<TTEntry> table_;
    size_t num_entries_ = 0;
};

} // namespace chess
