#include "board.h"
#include <sstream>
#include <cstring>

namespace chess {

// ── Zobrist ─────────────────────────────────────────────────────────────
namespace zobrist {
    uint64_t PieceSquare[PIECE_NB][SQUARE_NB];
    uint64_t Castling[16];
    uint64_t EnPassant[8];
    uint64_t Side;

    void init() {
        std::mt19937_64 rng(0xBEEF1234CAFE5678ULL);
        for (int p = 0; p < PIECE_NB; ++p)
            for (int s = 0; s < SQUARE_NB; ++s)
                PieceSquare[p][s] = rng();
        for (int i = 0; i < 16; ++i)
            Castling[i] = rng();
        for (int i = 0; i < 8; ++i)
            EnPassant[i] = rng();
        Side = rng();
    }
}

// ── Board ───────────────────────────────────────────────────────────────
Board::Board() {
    static bool zobrist_initialized = false;
    if (!zobrist_initialized) {
        zobrist::init();
        zobrist_initialized = true;
    }
}

void Board::put_piece(Piece p, Square s) {
    mailbox_[s] = p;
    Bitboard sq = bb::square_bb(s);
    by_type_[piece_type(p)] |= sq;
    by_color_[piece_color(p)] |= sq;
}

void Board::remove_piece(Square s) {
    Piece p = mailbox_[s];
    Bitboard sq = bb::square_bb(s);
    by_type_[piece_type(p)] ^= sq;
    by_color_[piece_color(p)] ^= sq;
    mailbox_[s] = NO_PIECE;
}

void Board::move_piece(Square from, Square to) {
    Piece p = mailbox_[from];
    Bitboard fromto = bb::square_bb(from) | bb::square_bb(to);
    by_type_[piece_type(p)] ^= fromto;
    by_color_[piece_color(p)] ^= fromto;
    mailbox_[from] = NO_PIECE;
    mailbox_[to] = p;
}

void Board::compute_hash() {
    uint64_t h = 0;
    for (int s = 0; s < 64; ++s)
        if (mailbox_[s] != NO_PIECE)
            h ^= zobrist::PieceSquare[mailbox_[s]][s];
    h ^= zobrist::Castling[state_->castling];
    if (state_->ep_square != SQ_NONE)
        h ^= zobrist::EnPassant[file_of(state_->ep_square)];
    if (side_ == BLACK)
        h ^= zobrist::Side;
    state_->hash = h;
}

void Board::set_startpos() {
    set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

void Board::set_fen(const std::string& fen) {
    std::memset(by_type_, 0, sizeof(by_type_));
    std::memset(by_color_, 0, sizeof(by_color_));
    std::memset(mailbox_, 0, sizeof(mailbox_));

    std::istringstream ss(fen);
    std::string board_str, side_str, castling_str, ep_str;
    int halfmove = 0, fullmove = 1;
    ss >> board_str >> side_str >> castling_str >> ep_str >> halfmove >> fullmove;

    // Parse board
    int rank = 7, file = 0;
    for (char c : board_str) {
        if (c == '/') { --rank; file = 0; continue; }
        if (c >= '1' && c <= '8') { file += c - '0'; continue; }
        Color color = (c >= 'a') ? BLACK : WHITE;
        PieceType pt = NO_PIECE_TYPE;
        char lower = c | 32;
        switch (lower) {
            case 'p': pt = PAWN; break;
            case 'n': pt = KNIGHT; break;
            case 'b': pt = BISHOP; break;
            case 'r': pt = ROOK; break;
            case 'q': pt = QUEEN; break;
            case 'k': pt = KING; break;
        }
        put_piece(make_piece(color, pt), make_square(file, rank));
        ++file;
    }

    side_ = (side_str == "b") ? BLACK : WHITE;
    fullmove_ = fullmove;

    // Must have a StateInfo set before calling compute_hash
    // Caller should provide one, but we initialize fields here
    if (state_) {
        state_->castling = NO_CASTLING;
        for (char c : castling_str) {
            switch (c) {
                case 'K': state_->castling |= WHITE_OO; break;
                case 'Q': state_->castling |= WHITE_OOO; break;
                case 'k': state_->castling |= BLACK_OO; break;
                case 'q': state_->castling |= BLACK_OOO; break;
            }
        }
        state_->ep_square = (ep_str != "-") ? string_to_square(ep_str) : SQ_NONE;
        state_->halfmove_clock = halfmove;
        state_->captured = NO_PIECE;
        state_->plies_from_null = 0;
        compute_hash();
    }

    game_ply_ = 2 * (fullmove - 1) + (side_ == BLACK ? 1 : 0);
}

std::string Board::to_fen() const {
    std::string fen;
    for (int rank = 7; rank >= 0; --rank) {
        int empty = 0;
        for (int file = 0; file < 8; ++file) {
            Piece p = mailbox_[make_square(file, rank)];
            if (p == NO_PIECE) { ++empty; continue; }
            if (empty) { fen += char('0' + empty); empty = 0; }
            const char chars[] = " PNBRQK  pnbrqk";
            fen += chars[p];
        }
        if (empty) fen += char('0' + empty);
        if (rank > 0) fen += '/';
    }
    fen += (side_ == WHITE) ? " w " : " b ";

    std::string castling;
    if (state_->castling & WHITE_OO)  castling += 'K';
    if (state_->castling & WHITE_OOO) castling += 'Q';
    if (state_->castling & BLACK_OO)  castling += 'k';
    if (state_->castling & BLACK_OOO) castling += 'q';
    fen += castling.empty() ? "-" : castling;

    fen += ' ';
    fen += (state_->ep_square != SQ_NONE) ? square_to_string(state_->ep_square) : "-";
    fen += ' ' + std::to_string(state_->halfmove_clock);
    fen += ' ' + std::to_string(fullmove_);
    return fen;
}

bool Board::is_square_attacked(Square s, Color by) const {
    if (bb::PawnAttacks[~by][s] & pieces(by, PAWN)) return true;
    if (bb::KnightAttacks[s] & pieces(by, KNIGHT)) return true;
    if (bb::KingAttacks[s] & pieces(by, KING)) return true;
    Bitboard occ = pieces();
    if (bb::bishop_attacks(s, occ) & pieces(by, BISHOP, QUEEN)) return true;
    if (bb::rook_attacks(s, occ) & pieces(by, ROOK, QUEEN)) return true;
    return false;
}

Bitboard Board::attackers_to(Square s, Bitboard occupied) const {
    return (bb::PawnAttacks[BLACK][s] & pieces(WHITE, PAWN))
         | (bb::PawnAttacks[WHITE][s] & pieces(BLACK, PAWN))
         | (bb::KnightAttacks[s]     & pieces(KNIGHT))
         | (bb::bishop_attacks(s, occupied) & pieces(BISHOP, QUEEN))
         | (bb::rook_attacks(s, occupied)   & pieces(ROOK, QUEEN))
         | (bb::KingAttacks[s]       & pieces(KING));
}

Bitboard Board::checkers() const {
    return attackers_to(king_square(side_), pieces()) & pieces(~side_);
}

void Board::make_move(Move m, StateInfo& new_si) {
    // Copy state
    new_si.castling = state_->castling;
    new_si.halfmove_clock = state_->halfmove_clock + 1;
    new_si.ep_square = SQ_NONE;
    new_si.captured = NO_PIECE;
    new_si.hash = state_->hash;
    new_si.plies_from_null = state_->plies_from_null + 1;

    StateInfo* prev = state_;
    state_ = &new_si;

    Square from = m.from();
    Square to   = m.to();
    MoveFlag flag = m.flags();
    Piece moving = mailbox_[from];
    PieceType pt = piece_type(moving);
    Color us = side_;

    // Hash out old state
    state_->hash ^= zobrist::Castling[prev->castling];
    if (prev->ep_square != SQ_NONE)
        state_->hash ^= zobrist::EnPassant[file_of(prev->ep_square)];

    // Handle captures
    if (is_capture(flag)) {
        Square cap_sq = to;
        if (flag == EP_CAPTURE) {
            cap_sq = (us == WHITE) ? to - NORTH : to - SOUTH;
        }
        state_->captured = mailbox_[cap_sq];
        state_->hash ^= zobrist::PieceSquare[state_->captured][cap_sq];
        remove_piece(cap_sq);
        state_->halfmove_clock = 0;
    }

    // Move the piece
    state_->hash ^= zobrist::PieceSquare[moving][from];

    if (chess::is_promotion(flag)) {
        // Remove pawn, add promotion piece
        remove_piece(from);
        Piece promo = make_piece(us, promo_piece_type(flag));
        put_piece(promo, to);
        state_->hash ^= zobrist::PieceSquare[promo][to];
        state_->halfmove_clock = 0;
    } else {
        move_piece(from, to);
        state_->hash ^= zobrist::PieceSquare[moving][to];
    }

    // Castling: move the rook
    if (flag == KING_CASTLE) {
        Square rook_from = (us == WHITE) ? SQ_H1 : SQ_H8;
        Square rook_to   = (us == WHITE) ? SQ_F1 : SQ_F8;
        Piece rook = make_piece(us, ROOK);
        state_->hash ^= zobrist::PieceSquare[rook][rook_from] ^ zobrist::PieceSquare[rook][rook_to];
        move_piece(rook_from, rook_to);
    } else if (flag == QUEEN_CASTLE) {
        Square rook_from = (us == WHITE) ? SQ_A1 : SQ_A8;
        Square rook_to   = (us == WHITE) ? SQ_D1 : SQ_D8;
        Piece rook = make_piece(us, ROOK);
        state_->hash ^= zobrist::PieceSquare[rook][rook_from] ^ zobrist::PieceSquare[rook][rook_to];
        move_piece(rook_from, rook_to);
    }

    // Pawn double push -> set ep square
    if (flag == DOUBLE_PUSH) {
        state_->ep_square = (us == WHITE) ? from + NORTH : from + SOUTH;
        state_->halfmove_clock = 0;
    }

    if (pt == PAWN) state_->halfmove_clock = 0;

    // Update castling rights
    state_->castling &= ~(CastlingMask[from] | CastlingMask[to]);
    state_->hash ^= zobrist::Castling[state_->castling];
    if (state_->ep_square != SQ_NONE)
        state_->hash ^= zobrist::EnPassant[file_of(state_->ep_square)];

    // Flip side
    side_ = ~side_;
    state_->hash ^= zobrist::Side;

    if (side_ == WHITE) ++fullmove_;
    ++game_ply_;
}

void Board::undo_move(Move m) {
    side_ = ~side_;
    --game_ply_;
    if (side_ == BLACK) --fullmove_;

    Square from = m.from();
    Square to   = m.to();
    MoveFlag flag = m.flags();
    Color us = side_;

    if (chess::is_promotion(flag)) {
        remove_piece(to);
        put_piece(make_piece(us, PAWN), from);
    } else {
        move_piece(to, from);
    }

    // Restore capture
    if (is_capture(flag)) {
        Square cap_sq = to;
        if (flag == EP_CAPTURE) {
            cap_sq = (us == WHITE) ? to - NORTH : to - SOUTH;
        }
        put_piece(state_->captured, cap_sq);
    }

    // Undo castling rook move
    if (flag == KING_CASTLE) {
        Square rook_from = (us == WHITE) ? SQ_H1 : SQ_H8;
        Square rook_to   = (us == WHITE) ? SQ_F1 : SQ_F8;
        move_piece(rook_to, rook_from);
    } else if (flag == QUEEN_CASTLE) {
        Square rook_from = (us == WHITE) ? SQ_A1 : SQ_A8;
        Square rook_to   = (us == WHITE) ? SQ_D1 : SQ_D8;
        move_piece(rook_to, rook_from);
    }

    // Restore previous state (caller manages StateInfo lifetime)
    // We need to go back to the previous StateInfo - the caller should manage this
    // For now, we don't have a prev pointer, so the Engine/caller must manage state_ reset
}

// Move::from_uci needs Board
Move Move::from_uci(const std::string& str, const Board& board) {
    Square from = string_to_square(str.substr(0, 2));
    Square to   = string_to_square(str.substr(2, 2));

    MoveFlag flag = NORMAL;
    PieceType pt = piece_type(board.piece_on(from));
    bool is_cap = board.piece_on(to) != NO_PIECE;

    // Promotion
    if (str.size() == 5) {
        PieceType promo = NO_PIECE_TYPE;
        switch (str[4]) {
            case 'n': promo = KNIGHT; break;
            case 'b': promo = BISHOP; break;
            case 'r': promo = ROOK; break;
            case 'q': promo = QUEEN; break;
        }
        int base = is_cap ? PROMO_CAPTURE_KNIGHT : PROMO_KNIGHT;
        flag = MoveFlag(base + int(promo) - int(KNIGHT));
        return Move(from, to, flag);
    }

    if (pt == KING) {
        if (from == SQ_E1 && to == SQ_G1) return Move(from, to, KING_CASTLE);
        if (from == SQ_E1 && to == SQ_C1) return Move(from, to, QUEEN_CASTLE);
        if (from == SQ_E8 && to == SQ_G8) return Move(from, to, KING_CASTLE);
        if (from == SQ_E8 && to == SQ_C8) return Move(from, to, QUEEN_CASTLE);
    }

    if (pt == PAWN) {
        int diff = int(to) - int(from);
        if (diff == 16 || diff == -16) flag = DOUBLE_PUSH;
        else if (to == board.ep_square()) flag = EP_CAPTURE;
        else if (is_cap) flag = CAPTURE;
        return Move(from, to, flag);
    }

    if (is_cap) flag = CAPTURE;
    return Move(from, to, flag);
}

} // namespace chess
