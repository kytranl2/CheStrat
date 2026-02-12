#pragma once

#include "types.h"
#include <string>

namespace chess {

// 16-bit packed move: [flags:4][to:6][from:6]
class Move {
public:
    Move() : data_(0) {}
    Move(Square from, Square to, MoveFlag flags = NORMAL)
        : data_(uint16_t(from) | (uint16_t(to) << 6) | (uint16_t(flags) << 12)) {}

    Square from()    const { return Square(data_ & 0x3F); }
    Square to()      const { return Square((data_ >> 6) & 0x3F); }
    MoveFlag flags() const { return MoveFlag(data_ >> 12); }
    uint16_t raw()   const { return data_; }

    bool is_promotion() const { return chess::is_promotion(flags()); }
    bool is_capture()   const { return chess::is_capture(flags()); }
    PieceType promo_type() const { return promo_piece_type(flags()); }

    bool operator==(Move o) const { return data_ == o.data_; }
    bool operator!=(Move o) const { return data_ != o.data_; }
    explicit operator bool() const { return data_ != 0; }

    std::string to_uci() const {
        std::string s = square_to_string(from()) + square_to_string(to());
        if (is_promotion()) {
            const char promo[] = "  nbrq";
            s += promo[int(promo_type())];
        }
        return s;
    }

    static Move from_uci(const std::string& str, const class Board& board);

    static Move none() { return Move(); }

private:
    uint16_t data_;
};

} // namespace chess
