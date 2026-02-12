#pragma once

#include <cstdint>
#include <string>
#include <cassert>

namespace chess {

// ── Bitboard ────────────────────────────────────────────────────────────
using Bitboard = uint64_t;

// ── Color ───────────────────────────────────────────────────────────────
enum Color : int { WHITE = 0, BLACK = 1, COLOR_NB = 2 };

constexpr Color operator~(Color c) { return Color(c ^ 1); }

// ── Piece types ─────────────────────────────────────────────────────────
enum PieceType : int {
    NO_PIECE_TYPE = 0,
    PAWN = 1, KNIGHT = 2, BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6,
    PIECE_TYPE_NB = 7
};

// ── Piece (color | type) ────────────────────────────────────────────────
enum Piece : int {
    NO_PIECE = 0,
    W_PAWN = 1, W_KNIGHT = 2, W_BISHOP = 3, W_ROOK = 4, W_QUEEN = 5, W_KING = 6,
    B_PAWN = 9, B_KNIGHT = 10, B_BISHOP = 11, B_ROOK = 12, B_QUEEN = 13, B_KING = 14,
    PIECE_NB = 16
};

constexpr Piece make_piece(Color c, PieceType pt) {
    return Piece((c << 3) | pt);
}
constexpr Color piece_color(Piece p) { return Color(p >> 3); }
constexpr PieceType piece_type(Piece p) { return PieceType(p & 7); }

// ── Square ──────────────────────────────────────────────────────────────
enum Square : int {
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
    SQ_NONE = 64, SQUARE_NB = 64
};

constexpr int file_of(Square s) { return s & 7; }
constexpr int rank_of(Square s) { return s >> 3; }
constexpr Square make_square(int file, int rank) { return Square(rank * 8 + file); }

// Relative rank from a color's perspective
constexpr int relative_rank(Color c, Square s) {
    return c == WHITE ? rank_of(s) : 7 - rank_of(s);
}

inline std::string square_to_string(Square s) {
    return std::string(1, char('a' + file_of(s))) + std::string(1, char('1' + rank_of(s)));
}

inline Square string_to_square(const std::string& str) {
    return make_square(str[0] - 'a', str[1] - '1');
}

// ── Directions ──────────────────────────────────────────────────────────
enum Direction : int {
    NORTH =  8, SOUTH = -8, EAST =  1, WEST = -1,
    NORTH_EAST = 9, NORTH_WEST = 7, SOUTH_EAST = -7, SOUTH_WEST = -9
};

constexpr Square operator+(Square s, Direction d) { return Square(int(s) + int(d)); }
constexpr Square operator-(Square s, Direction d) { return Square(int(s) - int(d)); }
constexpr Square& operator+=(Square& s, Direction d) { return s = s + d; }

// ── Castling rights ─────────────────────────────────────────────────────
enum CastlingRight : int {
    NO_CASTLING = 0,
    WHITE_OO  = 1,
    WHITE_OOO = 2,
    BLACK_OO  = 4,
    BLACK_OOO = 8,
    ALL_CASTLING = 15
};

constexpr CastlingRight operator|(CastlingRight a, CastlingRight b) {
    return CastlingRight(int(a) | int(b));
}
constexpr CastlingRight operator&(CastlingRight a, CastlingRight b) {
    return CastlingRight(int(a) & int(b));
}
constexpr CastlingRight operator~(CastlingRight c) {
    return CastlingRight(~int(c) & 15);
}
constexpr CastlingRight& operator|=(CastlingRight& a, CastlingRight b) { return a = a | b; }
constexpr CastlingRight& operator&=(CastlingRight& a, CastlingRight b) { return a = a & b; }

// ── Move flags ──────────────────────────────────────────────────────────
enum MoveFlag : int {
    NORMAL     = 0,
    DOUBLE_PUSH = 1,
    KING_CASTLE = 2,
    QUEEN_CASTLE = 3,
    CAPTURE    = 4,
    EP_CAPTURE = 5,
    PROMO_KNIGHT = 8,
    PROMO_BISHOP = 9,
    PROMO_ROOK   = 10,
    PROMO_QUEEN  = 11,
    PROMO_CAPTURE_KNIGHT = 12,
    PROMO_CAPTURE_BISHOP = 13,
    PROMO_CAPTURE_ROOK   = 14,
    PROMO_CAPTURE_QUEEN  = 15
};

constexpr bool is_promotion(MoveFlag f) { return f >= PROMO_KNIGHT; }
constexpr bool is_capture(MoveFlag f)   { return f == CAPTURE || f == EP_CAPTURE || f >= PROMO_CAPTURE_KNIGHT; }

constexpr PieceType promo_piece_type(MoveFlag f) {
    switch (f & 3) {
        case 0: return KNIGHT;
        case 1: return BISHOP;
        case 2: return ROOK;
        case 3: return QUEEN;
        default: return NO_PIECE_TYPE;
    }
}

// ── Score constants ─────────────────────────────────────────────────────
constexpr int VALUE_NONE     = 32002;
constexpr int VALUE_INFINITE = 32001;
constexpr int VALUE_MATE     = 32000;
constexpr int VALUE_DRAW     = 0;

constexpr int MATE_IN_MAX_PLY  =  VALUE_MATE - 256;
constexpr int MATED_IN_MAX_PLY = -VALUE_MATE + 256;

constexpr bool is_mate_score(int v) {
    return v >= MATE_IN_MAX_PLY || v <= MATED_IN_MAX_PLY;
}

} // namespace chess
