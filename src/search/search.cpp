#include "search.h"
#include "../core/movegen.h"
#include "../eval/evaluation.h"
#include "../eval/pst.h"
#include <chrono>
#include <algorithm>

namespace chess {

static int64_t now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

Searcher::Searcher() : tt_(64), stop_flag_(false), nodes_(0), start_time_(0), time_limit_(0) {}

bool Searcher::should_stop() {
    if (stop_flag_.load(std::memory_order_relaxed)) return true;
    if ((nodes_ & (CHECK_NODES - 1)) == 0) {
        if (now_ms() - start_time_ >= time_limit_) {
            stop_flag_.store(true);
            return true;
        }
    }
    return false;
}

// MVV-LVA scoring for captures
static int mvv_lva_score(const Board& board, Move m) {
    if (!m.is_capture()) return 0;
    PieceType victim = piece_type(board.piece_on(m.to()));
    PieceType attacker = piece_type(board.piece_on(m.from()));
    if (m.flags() == EP_CAPTURE) victim = PAWN;
    return pst::PieceValue[victim] * 10 - pst::PieceValue[attacker];
}

void Searcher::order_moves(const Board& board, MoveList& moves, Move tt_move) {
    int scores[256];
    for (int i = 0; i < moves.count; ++i) {
        if (moves.moves[i] == tt_move) {
            scores[i] = 1000000; // TT move first
        } else if (moves.moves[i].is_capture()) {
            scores[i] = 100000 + mvv_lva_score(board, moves.moves[i]);
        } else if (moves.moves[i].is_promotion()) {
            scores[i] = 90000;
        } else {
            scores[i] = 0;
        }
    }

    // Simple selection sort (good enough for ~30 moves)
    for (int i = 0; i < moves.count - 1; ++i) {
        int best = i;
        for (int j = i + 1; j < moves.count; ++j) {
            if (scores[j] > scores[best]) best = j;
        }
        if (best != i) {
            std::swap(moves.moves[i], moves.moves[best]);
            std::swap(scores[i], scores[best]);
        }
    }
}

int Searcher::quiescence(Board& board, int alpha, int beta, int ply,
                         std::deque<StateInfo>& states) {
    ++nodes_;

    int stand_pat = evaluate(board);
    if (stand_pat >= beta) return beta;
    if (stand_pat > alpha) alpha = stand_pat;

    MoveList moves;
    generate_legal_captures(board, moves);
    order_moves(board, moves, Move::none());

    StateInfo* prev_state = board.state();
    size_t si = states.size();
    states.emplace_back();

    for (int i = 0; i < moves.count; ++i) {
        if (should_stop()) break;

        board.make_move(moves.moves[i], states[si]);
        int score = -quiescence(board, -beta, -alpha, ply + 1, states);
        board.undo_move(moves.moves[i]);
        board.set_state(prev_state);

        if (score >= beta) {
            states.pop_back();
            return beta;
        }
        if (score > alpha) alpha = score;
    }

    states.pop_back();
    return alpha;
}

int Searcher::alpha_beta(Board& board, int alpha, int beta, int depth, int ply,
                         std::deque<StateInfo>& states) {
    if (should_stop()) return 0;

    // Check transposition table
    TTEntry tt_entry;
    Move tt_move = Move::none();
    if (tt_.probe(board.hash(), tt_entry)) {
        tt_move = tt_entry.get_move();
        if (tt_entry.depth >= depth) {
            int tt_score = tt_entry.score;
            if (tt_entry.flag == TT_EXACT) return tt_score;
            if (tt_entry.flag == TT_ALPHA && tt_score <= alpha) return alpha;
            if (tt_entry.flag == TT_BETA && tt_score >= beta) return beta;
        }
    }

    if (depth <= 0) {
        return quiescence(board, alpha, beta, ply, states);
    }

    ++nodes_;

    MoveList moves;
    generate_legal_moves(board, moves);

    // Checkmate or stalemate
    if (moves.count == 0) {
        if (board.in_check())
            return -VALUE_MATE + ply; // Checkmate
        return VALUE_DRAW; // Stalemate
    }

    // Draw by 50-move rule
    if (board.halfmove_clock() >= 100) return VALUE_DRAW;

    order_moves(board, moves, tt_move);

    Move best_move = moves.moves[0];
    TTFlag flag = TT_ALPHA;

    StateInfo* prev_state = board.state();
    size_t si = states.size();
    states.emplace_back();

    for (int i = 0; i < moves.count; ++i) {
        board.make_move(moves.moves[i], states[si]);
        int score = -alpha_beta(board, -beta, -alpha, depth - 1, ply + 1, states);
        board.undo_move(moves.moves[i]);
        board.set_state(prev_state);

        if (stop_flag_.load(std::memory_order_relaxed)) {
            states.pop_back();
            return 0;
        }

        if (score >= beta) {
            tt_.store(board.hash(), beta, depth, TT_BETA, moves.moves[i]);
            states.pop_back();
            return beta;
        }
        if (score > alpha) {
            alpha = score;
            best_move = moves.moves[i];
            flag = TT_EXACT;
        }
    }

    tt_.store(board.hash(), alpha, depth, flag, best_move);
    states.pop_back();
    return alpha;
}

Move Searcher::search(Board& board, const SearchLimits& limits,
                      std::deque<StateInfo>& states,
                      InfoCallback on_info) {
    stop_flag_.store(false);
    nodes_ = 0;
    start_time_ = now_ms();
    time_limit_ = limits.time_ms;
    info_cb_ = on_info;

    Move best_move = Move::none();

    // Iterative deepening
    for (int depth = 1; depth <= limits.max_depth; ++depth) {
        int alpha = -VALUE_INFINITE;
        int beta = VALUE_INFINITE;

        MoveList moves;
        generate_legal_moves(board, moves);
        if (moves.count == 0) break;

        // Get TT move for ordering
        TTEntry tt_entry;
        Move tt_move = Move::none();
        if (tt_.probe(board.hash(), tt_entry))
            tt_move = tt_entry.get_move();
        order_moves(board, moves, tt_move);

        Move iter_best = moves.moves[0];
        int iter_score = -VALUE_INFINITE;

        StateInfo* prev_state = board.state();
        size_t si = states.size();
        states.emplace_back();

        for (int i = 0; i < moves.count; ++i) {
            board.make_move(moves.moves[i], states[si]);
            int score = -alpha_beta(board, -beta, -alpha, depth - 1, 1, states);
            board.undo_move(moves.moves[i]);
            board.set_state(prev_state);

            if (stop_flag_.load(std::memory_order_relaxed)) break;

            if (score > iter_score) {
                iter_score = score;
                iter_best = moves.moves[i];
            }
            if (score > alpha) alpha = score;
        }

        states.pop_back();

        if (!stop_flag_.load(std::memory_order_relaxed)) {
            best_move = iter_best;
            if (info_cb_) {
                SearchInfo info{depth, iter_score, best_move, nodes_};
                info_cb_(info);
            }
        }

        if (stop_flag_.load(std::memory_order_relaxed)) break;

        // If we found a mate, no need to search deeper
        if (is_mate_score(alpha)) break;
    }

    return best_move;
}

} // namespace chess
