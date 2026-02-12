#pragma once

#include "types.h"
#include <bit>

namespace chess {
namespace bb {

// ── Pre-computed tables ─────────────────────────────────────────────────
extern Bitboard PawnAttacks[COLOR_NB][SQUARE_NB];
extern Bitboard KnightAttacks[SQUARE_NB];
extern Bitboard KingAttacks[SQUARE_NB];
extern Bitboard RayTable[SQUARE_NB][8]; // 8 directions: N, NE, E, SE, S, SW, W, NW
extern Bitboard BetweenBB[SQUARE_NB][SQUARE_NB];
extern Bitboard LineBB[SQUARE_NB][SQUARE_NB];

void init();

// ── Bitboard helpers ────────────────────────────────────────────────────
constexpr Bitboard square_bb(Square s) { return 1ULL << s; }

constexpr Bitboard FileA_BB = 0x0101010101010101ULL;
constexpr Bitboard FileB_BB = FileA_BB << 1;
constexpr Bitboard FileG_BB = FileA_BB << 6;
constexpr Bitboard FileH_BB = FileA_BB << 7;
constexpr Bitboard Rank1_BB = 0xFFULL;
constexpr Bitboard Rank2_BB = Rank1_BB << 8;
constexpr Bitboard Rank3_BB = Rank1_BB << 16;
constexpr Bitboard Rank4_BB = Rank1_BB << 24;
constexpr Bitboard Rank5_BB = Rank1_BB << 32;
constexpr Bitboard Rank6_BB = Rank1_BB << 40;
constexpr Bitboard Rank7_BB = Rank1_BB << 48;
constexpr Bitboard Rank8_BB = Rank1_BB << 56;

constexpr Bitboard file_bb(int f) { return FileA_BB << f; }
constexpr Bitboard rank_bb(int r) { return Rank1_BB << (r * 8); }

inline int popcount(Bitboard b) { return std::popcount(b); }

inline Square lsb(Bitboard b) {
    return Square(std::countr_zero(b));
}

inline Square pop_lsb(Bitboard& b) {
    Square s = lsb(b);
    b &= b - 1;
    return s;
}

inline bool more_than_one(Bitboard b) { return b & (b - 1); }

// ── Slider attacks (classical ray approach) ─────────────────────────────
Bitboard bishop_attacks(Square s, Bitboard occupied);
Bitboard rook_attacks(Square s, Bitboard occupied);

inline Bitboard queen_attacks(Square s, Bitboard occupied) {
    return bishop_attacks(s, occupied) | rook_attacks(s, occupied);
}

// ── Pawn helpers ────────────────────────────────────────────────────────
constexpr Bitboard shift_north(Bitboard b) { return b << 8; }
constexpr Bitboard shift_south(Bitboard b) { return b >> 8; }
constexpr Bitboard shift_ne(Bitboard b)    { return (b & ~FileH_BB) << 9; }
constexpr Bitboard shift_nw(Bitboard b)    { return (b & ~FileA_BB) << 7; }
constexpr Bitboard shift_se(Bitboard b)    { return (b & ~FileH_BB) >> 7; }
constexpr Bitboard shift_sw(Bitboard b)    { return (b & ~FileA_BB) >> 9; }

} // namespace bb
} // namespace chess
