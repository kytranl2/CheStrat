#include "evaluation.h"
#include "pst.h"
#include "../core/movegen.h"

namespace chess {

static bool is_endgame(const Board& board) {
    // Endgame if no queens or only queens + pawns
    Bitboard queens = board.pieces(QUEEN);
    if (!queens) return true;
    Bitboard non_pawn_non_king = board.pieces(KNIGHT) | board.pieces(BISHOP) |
                                  board.pieces(ROOK) | board.pieces(QUEEN);
    // Simple heuristic: few pieces left
    return bb::popcount(non_pawn_non_king) <= 4;
}

static int eval_material_and_pst(const Board& board, Color c, bool endgame) {
    int score = 0;
    for (int pt = PAWN; pt <= QUEEN; ++pt) {
        Bitboard pieces = board.pieces(c, PieceType(pt));
        while (pieces) {
            Square s = bb::pop_lsb(pieces);
            score += pst::PieceValue[pt];
            score += pst::value(c, PieceType(pt), s, endgame);
        }
    }
    // King PST only
    Square ks = board.king_square(c);
    score += pst::value(c, KING, ks, endgame);
    return score;
}

static int eval_pawn_structure(const Board& board, Color c) {
    int score = 0;
    Bitboard pawns = board.pieces(c, PAWN);
    Bitboard enemy_pawns = board.pieces(~c, PAWN);
    Bitboard our_pawns = pawns;

    while (pawns) {
        Square s = bb::pop_lsb(pawns);
        int f = file_of(s);
        int r = rank_of(s);

        // Doubled pawns
        Bitboard file_pawns = our_pawns & bb::file_bb(f);
        if (bb::more_than_one(file_pawns)) {
            score -= 15; // Penalty shared among doubled pawns; counted per pawn
        }

        // Isolated pawns
        Bitboard adj_files = 0;
        if (f > 0) adj_files |= bb::file_bb(f - 1);
        if (f < 7) adj_files |= bb::file_bb(f + 1);
        if (!(our_pawns & adj_files)) {
            score -= 20;
        }

        // Passed pawns
        Bitboard front_span;
        if (c == WHITE) {
            // Squares ahead on same file and adjacent files
            front_span = 0;
            for (int rr = r + 1; rr <= 7; ++rr) {
                if (f > 0) front_span |= bb::square_bb(make_square(f - 1, rr));
                front_span |= bb::square_bb(make_square(f, rr));
                if (f < 7) front_span |= bb::square_bb(make_square(f + 1, rr));
            }
        } else {
            front_span = 0;
            for (int rr = r - 1; rr >= 0; --rr) {
                if (f > 0) front_span |= bb::square_bb(make_square(f - 1, rr));
                front_span |= bb::square_bb(make_square(f, rr));
                if (f < 7) front_span |= bb::square_bb(make_square(f + 1, rr));
            }
        }
        if (!(enemy_pawns & front_span)) {
            int rel_rank = relative_rank(c, s);
            score += 20 + rel_rank * 10; // Passed pawn bonus increases with rank
        }
    }
    return score;
}

static int eval_bishop_pair(const Board& board, Color c) {
    if (bb::popcount(board.pieces(c, BISHOP)) >= 2) return 30;
    return 0;
}

static int eval_rook_files(const Board& board, Color c) {
    int score = 0;
    Bitboard rooks = board.pieces(c, ROOK);
    Bitboard our_pawns = board.pieces(c, PAWN);
    Bitboard enemy_pawns = board.pieces(~c, PAWN);

    while (rooks) {
        Square s = bb::pop_lsb(rooks);
        int f = file_of(s);
        Bitboard file = bb::file_bb(f);
        if (!(file & our_pawns)) {
            if (!(file & enemy_pawns))
                score += 20; // Open file
            else
                score += 10; // Semi-open file
        }
    }
    return score;
}

static int eval_king_safety(const Board& board, Color c, bool endgame) {
    if (endgame) return 0;
    int score = 0;
    Square ks = board.king_square(c);
    int kf = file_of(ks);
    Bitboard our_pawns = board.pieces(c, PAWN);

    // Pawn shield
    for (int df = -1; df <= 1; ++df) {
        int f = kf + df;
        if (f < 0 || f > 7) continue;
        int shield_rank = (c == WHITE) ? rank_of(ks) + 1 : rank_of(ks) - 1;
        if (shield_rank >= 0 && shield_rank <= 7) {
            if (our_pawns & bb::square_bb(make_square(f, shield_rank)))
                score += 5;
        }
    }
    return score;
}

static int eval_mobility(const Board& board, Color c) {
    int mobility = 0;
    Bitboard occ = board.pieces();

    // Knight mobility
    Bitboard knights = board.pieces(c, KNIGHT);
    while (knights) {
        Square s = bb::pop_lsb(knights);
        mobility += bb::popcount(bb::KnightAttacks[s] & ~board.pieces(c));
    }

    // Bishop mobility
    Bitboard bishops = board.pieces(c, BISHOP);
    while (bishops) {
        Square s = bb::pop_lsb(bishops);
        mobility += bb::popcount(bb::bishop_attacks(s, occ) & ~board.pieces(c));
    }

    // Rook mobility
    Bitboard rooks = board.pieces(c, ROOK);
    while (rooks) {
        Square s = bb::pop_lsb(rooks);
        mobility += bb::popcount(bb::rook_attacks(s, occ) & ~board.pieces(c));
    }

    // Queen mobility
    Bitboard queens = board.pieces(c, QUEEN);
    while (queens) {
        Square s = bb::pop_lsb(queens);
        mobility += bb::popcount(bb::queen_attacks(s, occ) & ~board.pieces(c));
    }

    return mobility * 2;
}

int evaluate(const Board& board) {
    bool endgame = is_endgame(board);

    int score = 0;
    // Material + PST
    score += eval_material_and_pst(board, WHITE, endgame);
    score -= eval_material_and_pst(board, BLACK, endgame);

    // Pawn structure
    score += eval_pawn_structure(board, WHITE);
    score -= eval_pawn_structure(board, BLACK);

    // Bishop pair
    score += eval_bishop_pair(board, WHITE);
    score -= eval_bishop_pair(board, BLACK);

    // Rook on open files
    score += eval_rook_files(board, WHITE);
    score -= eval_rook_files(board, BLACK);

    // King safety
    score += eval_king_safety(board, WHITE, endgame);
    score -= eval_king_safety(board, BLACK, endgame);

    // Mobility
    score += eval_mobility(board, WHITE);
    score -= eval_mobility(board, BLACK);

    // Return from side-to-move perspective
    return (board.side_to_move() == WHITE) ? score : -score;
}

} // namespace chess
