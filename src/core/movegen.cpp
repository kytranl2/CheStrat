#include "movegen.h"

namespace chess {

// ── Pseudo-legal generation helpers ─────────────────────────────────────

static void generate_pawn_moves(const Board& board, MoveList& list, bool caps_only) {
    Color us = board.side_to_move();
    Color them = ~us;
    Bitboard pawns = board.pieces(us, PAWN);
    Bitboard occ = board.pieces();
    Bitboard enemies = board.pieces(them);
    Square ep = board.ep_square();

    Direction up    = (us == WHITE) ? NORTH : SOUTH;
    Bitboard rank7  = (us == WHITE) ? bb::Rank7_BB : bb::Rank2_BB;
    Bitboard rank3  = (us == WHITE) ? bb::Rank3_BB : bb::Rank6_BB;

    Bitboard promo_pawns = pawns & rank7;
    Bitboard non_promo   = pawns & ~rank7;

    // Single push (non-promoting)
    if (!caps_only) {
        Bitboard single = (us == WHITE) ? bb::shift_north(non_promo) : bb::shift_south(non_promo);
        single &= ~occ;
        Bitboard dbl = (us == WHITE) ? bb::shift_north(single & rank3) : bb::shift_south(single & rank3);
        dbl &= ~occ;

        while (single) {
            Square to = bb::pop_lsb(single);
            list.push(Move(to - up, to, NORMAL));
        }
        while (dbl) {
            Square to = bb::pop_lsb(dbl);
            list.push(Move(to - up - up, to, DOUBLE_PUSH));
        }
    }

    // Captures (non-promoting)
    {
        Bitboard left  = (us == WHITE) ? bb::shift_nw(non_promo) : bb::shift_sw(non_promo);
        Bitboard right = (us == WHITE) ? bb::shift_ne(non_promo) : bb::shift_se(non_promo);
        left &= enemies;
        right &= enemies;

        Direction left_dir  = (us == WHITE) ? NORTH_WEST : SOUTH_WEST;
        Direction right_dir = (us == WHITE) ? NORTH_EAST : SOUTH_EAST;

        while (left) {
            Square to = bb::pop_lsb(left);
            list.push(Move(to - left_dir, to, CAPTURE));
        }
        while (right) {
            Square to = bb::pop_lsb(right);
            list.push(Move(to - right_dir, to, CAPTURE));
        }
    }

    // Promotions (push + capture)
    if (promo_pawns) {
        Bitboard push_promo = (us == WHITE) ? bb::shift_north(promo_pawns) : bb::shift_south(promo_pawns);
        push_promo &= ~occ;

        Bitboard left_promo  = (us == WHITE) ? bb::shift_nw(promo_pawns) : bb::shift_sw(promo_pawns);
        Bitboard right_promo = (us == WHITE) ? bb::shift_ne(promo_pawns) : bb::shift_se(promo_pawns);
        left_promo &= enemies;
        right_promo &= enemies;

        Direction left_dir  = (us == WHITE) ? NORTH_WEST : SOUTH_WEST;
        Direction right_dir = (us == WHITE) ? NORTH_EAST : SOUTH_EAST;

        while (push_promo) {
            Square to = bb::pop_lsb(push_promo);
            if (!caps_only) {
                list.push(Move(to - up, to, PROMO_KNIGHT));
                list.push(Move(to - up, to, PROMO_BISHOP));
                list.push(Move(to - up, to, PROMO_ROOK));
            }
            list.push(Move(to - up, to, PROMO_QUEEN));
        }
        while (left_promo) {
            Square to = bb::pop_lsb(left_promo);
            list.push(Move(to - left_dir, to, PROMO_CAPTURE_KNIGHT));
            list.push(Move(to - left_dir, to, PROMO_CAPTURE_BISHOP));
            list.push(Move(to - left_dir, to, PROMO_CAPTURE_ROOK));
            list.push(Move(to - left_dir, to, PROMO_CAPTURE_QUEEN));
        }
        while (right_promo) {
            Square to = bb::pop_lsb(right_promo);
            list.push(Move(to - right_dir, to, PROMO_CAPTURE_KNIGHT));
            list.push(Move(to - right_dir, to, PROMO_CAPTURE_BISHOP));
            list.push(Move(to - right_dir, to, PROMO_CAPTURE_ROOK));
            list.push(Move(to - right_dir, to, PROMO_CAPTURE_QUEEN));
        }
    }

    // En passant
    if (ep != SQ_NONE) {
        Bitboard ep_candidates = bb::PawnAttacks[them][ep] & pawns;
        while (ep_candidates) {
            Square from = bb::pop_lsb(ep_candidates);
            list.push(Move(from, ep, EP_CAPTURE));
        }
    }
}

static void generate_piece_moves(const Board& board, MoveList& list, PieceType pt, bool caps_only) {
    Color us = board.side_to_move();
    Bitboard occ = board.pieces();
    Bitboard targets = caps_only ? board.pieces(~us) : ~board.pieces(us);
    Bitboard pieces = board.pieces(us, pt);

    while (pieces) {
        Square from = bb::pop_lsb(pieces);
        Bitboard attacks;
        switch (pt) {
            case KNIGHT: attacks = bb::KnightAttacks[from]; break;
            case BISHOP: attacks = bb::bishop_attacks(from, occ); break;
            case ROOK:   attacks = bb::rook_attacks(from, occ); break;
            case QUEEN:  attacks = bb::queen_attacks(from, occ); break;
            case KING:   attacks = bb::KingAttacks[from]; break;
            default: attacks = 0;
        }
        attacks &= targets;
        while (attacks) {
            Square to = bb::pop_lsb(attacks);
            MoveFlag flag = (board.piece_on(to) != NO_PIECE) ? CAPTURE : NORMAL;
            list.push(Move(from, to, flag));
        }
    }
}

static void generate_castling(const Board& board, MoveList& list) {
    Color us = board.side_to_move();
    Bitboard occ = board.pieces();

    if (us == WHITE) {
        if ((board.castling_rights() & WHITE_OO) &&
            !(occ & (bb::square_bb(SQ_F1) | bb::square_bb(SQ_G1))) &&
            !board.is_square_attacked(SQ_E1, BLACK) &&
            !board.is_square_attacked(SQ_F1, BLACK) &&
            !board.is_square_attacked(SQ_G1, BLACK))
        {
            list.push(Move(SQ_E1, SQ_G1, KING_CASTLE));
        }
        if ((board.castling_rights() & WHITE_OOO) &&
            !(occ & (bb::square_bb(SQ_D1) | bb::square_bb(SQ_C1) | bb::square_bb(SQ_B1))) &&
            !board.is_square_attacked(SQ_E1, BLACK) &&
            !board.is_square_attacked(SQ_D1, BLACK) &&
            !board.is_square_attacked(SQ_C1, BLACK))
        {
            list.push(Move(SQ_E1, SQ_C1, QUEEN_CASTLE));
        }
    } else {
        if ((board.castling_rights() & BLACK_OO) &&
            !(occ & (bb::square_bb(SQ_F8) | bb::square_bb(SQ_G8))) &&
            !board.is_square_attacked(SQ_E8, WHITE) &&
            !board.is_square_attacked(SQ_F8, WHITE) &&
            !board.is_square_attacked(SQ_G8, WHITE))
        {
            list.push(Move(SQ_E8, SQ_G8, KING_CASTLE));
        }
        if ((board.castling_rights() & BLACK_OOO) &&
            !(occ & (bb::square_bb(SQ_D8) | bb::square_bb(SQ_C8) | bb::square_bb(SQ_B8))) &&
            !board.is_square_attacked(SQ_E8, WHITE) &&
            !board.is_square_attacked(SQ_D8, WHITE) &&
            !board.is_square_attacked(SQ_C8, WHITE))
        {
            list.push(Move(SQ_E8, SQ_C8, QUEEN_CASTLE));
        }
    }
}

template<GenType GT>
void generate_moves(const Board& board, MoveList& list) {
    bool caps_only = (GT == CAPTURES_ONLY);

    generate_pawn_moves(board, list, caps_only);
    generate_piece_moves(board, list, KNIGHT, caps_only);
    generate_piece_moves(board, list, BISHOP, caps_only);
    generate_piece_moves(board, list, ROOK, caps_only);
    generate_piece_moves(board, list, QUEEN, caps_only);
    generate_piece_moves(board, list, KING, caps_only);

    if (!caps_only) {
        generate_castling(board, list);
    }
}

// Explicit instantiations
template void generate_moves<ALL_MOVES>(const Board& board, MoveList& list);
template void generate_moves<CAPTURES_ONLY>(const Board& board, MoveList& list);

// ── Legal move filtering ────────────────────────────────────────────────

static bool is_legal(const Board& board, Move m) {
    // Make move on a copy, check if king is in check
    Board copy = board;
    StateInfo si;
    copy.set_state(&si);
    // We need state set up - copy the current state
    si.castling = board.castling_rights();
    si.ep_square = board.ep_square();
    si.halfmove_clock = board.halfmove_clock();
    si.hash = board.hash();
    si.captured = NO_PIECE;
    si.plies_from_null = 0;

    StateInfo new_si;
    copy.make_move(m, new_si);
    // After make_move, side has flipped; check if the moving side's king is attacked
    Color moved = ~copy.side_to_move();
    return !copy.is_square_attacked(copy.king_square(moved), copy.side_to_move());
}

void generate_legal_moves(const Board& board, MoveList& list) {
    MoveList pseudo;
    generate_moves<ALL_MOVES>(board, pseudo);
    for (int i = 0; i < pseudo.count; ++i) {
        if (is_legal(board, pseudo.moves[i]))
            list.push(pseudo.moves[i]);
    }
}

void generate_legal_captures(const Board& board, MoveList& list) {
    MoveList pseudo;
    generate_moves<CAPTURES_ONLY>(board, pseudo);
    for (int i = 0; i < pseudo.count; ++i) {
        if (is_legal(board, pseudo.moves[i]))
            list.push(pseudo.moves[i]);
    }
}

// ── Perft ───────────────────────────────────────────────────────────────

uint64_t perft(Board& board, int depth, std::vector<StateInfo>& states) {
    if (depth == 0) return 1;

    MoveList moves;
    generate_legal_moves(board, moves);

    if (depth == 1) return moves.count;

    uint64_t nodes = 0;
    StateInfo* prev_state = board.state();
    size_t state_idx = states.size();
    states.emplace_back();

    for (int i = 0; i < moves.count; ++i) {
        board.make_move(moves.moves[i], states[state_idx]);
        nodes += perft(board, depth - 1, states);
        board.undo_move(moves.moves[i]);
        board.set_state(prev_state);
    }

    states.pop_back();
    return nodes;
}

} // namespace chess
