#pragma once

#include "../core/board.h"
#include "../core/move.h"
#include "../core/movegen.h"
#include "ttable.h"
#include <atomic>
#include <functional>
#include <vector>

namespace chess {

struct SearchLimits {
    int max_depth = 64;
    int time_ms = 5000;   // milliseconds
};

struct SearchInfo {
    int depth;
    int score;
    Move best_move;
    uint64_t nodes;
};

using InfoCallback = std::function<void(const SearchInfo&)>;

class Searcher {
public:
    Searcher();

    Move search(Board& board, const SearchLimits& limits,
                std::vector<StateInfo>& states,
                InfoCallback on_info = nullptr);

    void stop() { stop_flag_.store(true); }
    uint64_t nodes() const { return nodes_; }

private:
    int alpha_beta(Board& board, int alpha, int beta, int depth, int ply,
                   std::vector<StateInfo>& states);
    int quiescence(Board& board, int alpha, int beta, int ply,
                   std::vector<StateInfo>& states);
    void order_moves(const Board& board, MoveList& moves, Move tt_move);

    TranspositionTable tt_;
    std::atomic<bool> stop_flag_;
    uint64_t nodes_;
    int64_t start_time_;
    int64_t time_limit_;
    InfoCallback info_cb_;

    static constexpr int CHECK_NODES = 2048;
    bool should_stop();
};

} // namespace chess
