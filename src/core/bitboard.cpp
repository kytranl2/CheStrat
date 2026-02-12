#include "bitboard.h"

namespace chess {
namespace bb {

Bitboard PawnAttacks[COLOR_NB][SQUARE_NB];
Bitboard KnightAttacks[SQUARE_NB];
Bitboard KingAttacks[SQUARE_NB];
Bitboard RayTable[SQUARE_NB][8];
Bitboard BetweenBB[SQUARE_NB][SQUARE_NB];
Bitboard LineBB[SQUARE_NB][SQUARE_NB];

// Direction vectors for ray generation
// Order: N, NE, E, SE, S, SW, W, NW
static constexpr int DirFile[8] = { 0, 1, 1, 1, 0,-1,-1,-1};
static constexpr int DirRank[8] = { 1, 1, 0,-1,-1,-1, 0, 1};

static Bitboard compute_ray(Square s, int dir) {
    Bitboard ray = 0;
    int f = file_of(s) + DirFile[dir];
    int r = rank_of(s) + DirRank[dir];
    while (f >= 0 && f <= 7 && r >= 0 && r <= 7) {
        ray |= square_bb(make_square(f, r));
        f += DirFile[dir];
        r += DirRank[dir];
    }
    return ray;
}

static Bitboard slide_attack(Square s, Bitboard occupied, int dir) {
    Bitboard attacks = 0;
    int f = file_of(s) + DirFile[dir];
    int r = rank_of(s) + DirRank[dir];
    while (f >= 0 && f <= 7 && r >= 0 && r <= 7) {
        Bitboard sq = square_bb(make_square(f, r));
        attacks |= sq;
        if (sq & occupied) break;
        f += DirFile[dir];
        r += DirRank[dir];
    }
    return attacks;
}

Bitboard bishop_attacks(Square s, Bitboard occupied) {
    return slide_attack(s, occupied, 1) | slide_attack(s, occupied, 3) |
           slide_attack(s, occupied, 5) | slide_attack(s, occupied, 7);
}

Bitboard rook_attacks(Square s, Bitboard occupied) {
    return slide_attack(s, occupied, 0) | slide_attack(s, occupied, 2) |
           slide_attack(s, occupied, 4) | slide_attack(s, occupied, 6);
}

void init() {
    // Knight offsets
    static constexpr int KnightDf[] = {-2,-1, 1, 2, 2, 1,-1,-2};
    static constexpr int KnightDr[] = { 1, 2, 2, 1,-1,-2,-2,-1};

    // King offsets
    static constexpr int KingDf[] = {-1,-1,-1, 0, 0, 1, 1, 1};
    static constexpr int KingDr[] = {-1, 0, 1,-1, 1,-1, 0, 1};

    for (int sq = 0; sq < 64; ++sq) {
        Square s = Square(sq);
        int f = file_of(s), r = rank_of(s);

        // Pawn attacks
        if (r < 7) {
            if (f > 0) PawnAttacks[WHITE][sq] |= square_bb(make_square(f-1, r+1));
            if (f < 7) PawnAttacks[WHITE][sq] |= square_bb(make_square(f+1, r+1));
        }
        if (r > 0) {
            if (f > 0) PawnAttacks[BLACK][sq] |= square_bb(make_square(f-1, r-1));
            if (f < 7) PawnAttacks[BLACK][sq] |= square_bb(make_square(f+1, r-1));
        }

        // Knight attacks
        for (int i = 0; i < 8; ++i) {
            int nf = f + KnightDf[i], nr = r + KnightDr[i];
            if (nf >= 0 && nf <= 7 && nr >= 0 && nr <= 7)
                KnightAttacks[sq] |= square_bb(make_square(nf, nr));
        }

        // King attacks
        for (int i = 0; i < 8; ++i) {
            int kf = f + KingDf[i], kr = r + KingDr[i];
            if (kf >= 0 && kf <= 7 && kr >= 0 && kr <= 7)
                KingAttacks[sq] |= square_bb(make_square(kf, kr));
        }

        // Rays
        for (int d = 0; d < 8; ++d)
            RayTable[sq][d] = compute_ray(s, d);
    }

    // Between and Line bitboards
    for (int s1 = 0; s1 < 64; ++s1) {
        for (int s2 = 0; s2 < 64; ++s2) {
            BetweenBB[s1][s2] = 0;
            LineBB[s1][s2] = 0;
            if (s1 == s2) continue;

            for (int d = 0; d < 8; ++d) {
                if (RayTable[s1][d] & square_bb(Square(s2))) {
                    // s2 is on ray d from s1
                    int opp = (d + 4) & 7;
                    LineBB[s1][s2] = RayTable[s1][d] | RayTable[s1][opp] | square_bb(Square(s1));
                    BetweenBB[s1][s2] = RayTable[s1][d] & RayTable[s2][opp];
                    break;
                }
            }
        }
    }
}

} // namespace bb
} // namespace chess
