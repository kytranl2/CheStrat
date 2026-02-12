#pragma once

#include "../core/board.h"
#include "../core/move.h"
#include "../search/search.h"
#include <vector>

namespace chess {

class Engine {
public:
    Engine();

    void new_game();
    void set_position(const std::string& fen);
    void set_startpos();
    bool apply_move(Move m);
    bool apply_uci_move(const std::string& uci);

    Move think(const SearchLimits& limits, InfoCallback on_info = nullptr);
    void stop_thinking();

    const Board& board() const { return board_; }
    Board& board() { return board_; }

    // Get legal moves for GUI
    MoveList legal_moves() const;

    bool is_game_over() const;
    bool is_checkmate() const;
    bool is_stalemate() const;
    bool is_draw() const;

private:
    Board board_;
    Searcher searcher_;
    std::vector<StateInfo> states_;

    void ensure_state();
};

} // namespace chess
