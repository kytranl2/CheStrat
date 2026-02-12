#pragma once

#include "types.h"
#include "bitboard.h"
#include "move.h"
#include <string>
#include <vector>
#include <random>

namespace chess {

// ── Zobrist hashing ─────────────────────────────────────────────────────
namespace zobrist {
    extern uint64_t PieceSquare[PIECE_NB][SQUARE_NB];
    extern uint64_t Castling[16];
    extern uint64_t EnPassant[8]; // file
    extern uint64_t Side;
    void init();
}

// ── State info (for undo) ───────────────────────────────────────────────
struct StateInfo {
    CastlingRight castling;
    Square        ep_square;
    int           halfmove_clock;
    Piece         captured;
    uint64_t      hash;
    int           plies_from_null;
};

// ── Board ───────────────────────────────────────────────────────────────
class Board {
public:
    Board();

    void set_startpos();
    void set_fen(const std::string& fen);
    std::string to_fen() const;

    void make_move(Move m, StateInfo& new_si);
    void undo_move(Move m);

    // Accessors
    Color     side_to_move() const { return side_; }
    Piece     piece_on(Square s) const { return mailbox_[s]; }
    Bitboard  pieces() const { return by_color_[WHITE] | by_color_[BLACK]; }
    Bitboard  pieces(Color c) const { return by_color_[c]; }
    Bitboard  pieces(PieceType pt) const { return by_type_[pt]; }
    Bitboard  pieces(Color c, PieceType pt) const { return by_color_[c] & by_type_[pt]; }
    Bitboard  pieces(PieceType pt1, PieceType pt2) const { return by_type_[pt1] | by_type_[pt2]; }
    Bitboard  pieces(Color c, PieceType pt1, PieceType pt2) const { return by_color_[c] & (by_type_[pt1] | by_type_[pt2]); }
    Square    king_square(Color c) const { return bb::lsb(pieces(c, KING)); }

    CastlingRight castling_rights() const { return state_->castling; }
    Square        ep_square() const { return state_->ep_square; }
    int           halfmove_clock() const { return state_->halfmove_clock; }
    uint64_t      hash() const { return state_->hash; }
    int           fullmove_number() const { return fullmove_; }

    // Attack queries
    bool is_square_attacked(Square s, Color by) const;
    bool in_check() const { return is_square_attacked(king_square(side_), ~side_); }
    Bitboard attackers_to(Square s, Bitboard occupied) const;

    // State stack management
    void set_state(StateInfo* si) { state_ = si; }
    StateInfo* state() const { return state_; }

    // For move generation
    Bitboard checkers() const;

    int game_ply() const { return game_ply_; }

private:
    void put_piece(Piece p, Square s);
    void remove_piece(Square s);
    void move_piece(Square from, Square to);
    void compute_hash();

    // Dual representation
    Bitboard  by_type_[PIECE_TYPE_NB] = {};
    Bitboard  by_color_[COLOR_NB] = {};
    Piece     mailbox_[SQUARE_NB] = {};

    Color     side_ = WHITE;
    int       fullmove_ = 1;
    int       game_ply_ = 0;
    StateInfo* state_ = nullptr;

    // Castling rights mask per square (which rights to clear when piece moves from/to)
    static constexpr CastlingRight CastlingMask[SQUARE_NB] = {
        // a1       b1              c1              d1              e1                    f1              g1              h1
        WHITE_OOO, NO_CASTLING, NO_CASTLING, NO_CASTLING, CastlingRight(WHITE_OO|WHITE_OOO), NO_CASTLING, NO_CASTLING, WHITE_OO,
        NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
        NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
        NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
        NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
        NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
        NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
        // a8       b8              c8              d8              e8                    f8              g8              h8
        BLACK_OOO, NO_CASTLING, NO_CASTLING, NO_CASTLING, CastlingRight(BLACK_OO|BLACK_OOO), NO_CASTLING, NO_CASTLING, BLACK_OO,
    };
};

} // namespace chess
