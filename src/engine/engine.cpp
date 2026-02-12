#include "engine.h"
#include "../core/movegen.h"

namespace chess {

Engine::Engine() {
    new_game();
}

void Engine::ensure_state() {
    if (states_.empty()) {
        states_.emplace_back();
    }
    board_.set_state(&states_.back());
}

void Engine::new_game() {
    states_.clear();
    states_.emplace_back();
    board_.set_state(&states_.back());
    board_.set_startpos();
}

void Engine::set_position(const std::string& fen) {
    states_.clear();
    states_.emplace_back();
    board_.set_state(&states_.back());
    board_.set_fen(fen);
}

void Engine::set_startpos() {
    new_game();
}

bool Engine::apply_move(Move m) {
    // Verify move is legal
    MoveList legal;
    generate_legal_moves(board_, legal);
    bool found = false;
    for (int i = 0; i < legal.count; ++i) {
        if (legal.moves[i] == m) { found = true; break; }
    }
    if (!found) return false;

    states_.emplace_back();
    board_.make_move(m, states_.back());
    return true;
}

bool Engine::apply_uci_move(const std::string& uci) {
    Move m = Move::from_uci(uci, board_);
    return apply_move(m);
}

Move Engine::think(const SearchLimits& limits, InfoCallback on_info) {
    return searcher_.search(board_, limits, states_, on_info);
}

void Engine::stop_thinking() {
    searcher_.stop();
}

MoveList Engine::legal_moves() const {
    MoveList list;
    generate_legal_moves(board_, list);
    return list;
}

bool Engine::is_checkmate() const {
    MoveList moves;
    generate_legal_moves(board_, moves);
    return moves.count == 0 && board_.in_check();
}

bool Engine::is_stalemate() const {
    MoveList moves;
    generate_legal_moves(board_, moves);
    return moves.count == 0 && !board_.in_check();
}

bool Engine::is_draw() const {
    return is_stalemate() || board_.halfmove_clock() >= 100;
}

bool Engine::is_game_over() const {
    MoveList moves;
    generate_legal_moves(board_, moves);
    return moves.count == 0 || board_.halfmove_clock() >= 100;
}

} // namespace chess
