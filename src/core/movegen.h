#pragma once

#include "board.h"
#include "move.h"
#include <vector>

namespace chess {

enum GenType { ALL_MOVES, CAPTURES_ONLY };

struct MoveList {
    Move moves[256];
    int count = 0;

    void push(Move m) { moves[count++] = m; }
    Move* begin() { return moves; }
    Move* end()   { return moves + count; }
    const Move* begin() const { return moves; }
    const Move* end()   const { return moves + count; }
    int size() const { return count; }
};

template<GenType GT>
void generate_moves(const Board& board, MoveList& list);

// Legal move generation (filters out moves leaving king in check)
void generate_legal_moves(const Board& board, MoveList& list);
void generate_legal_captures(const Board& board, MoveList& list);

// Perft
uint64_t perft(Board& board, int depth, std::vector<StateInfo>& states);

} // namespace chess
