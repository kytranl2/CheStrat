#include "renderer.h"
#include <iostream>

namespace gui {

Renderer::Renderer(sf::RenderWindow& window) : window_(window) {
    // Try to load system font
    if (font_.loadFromFile("/System/Library/Fonts/Helvetica.ttc") ||
        font_.loadFromFile("/System/Library/Fonts/SFNSMono.ttf") ||
        font_.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
        font_loaded_ = true;
    }
}

bool Renderer::load_textures(const std::string& asset_path) {
    struct PieceFile {
        chess::Piece piece;
        const char* filename;
    };

    PieceFile files[] = {
        {chess::W_KING,   "wK.png"}, {chess::W_QUEEN,  "wQ.png"},
        {chess::W_ROOK,   "wR.png"}, {chess::W_BISHOP, "wB.png"},
        {chess::W_KNIGHT, "wN.png"}, {chess::W_PAWN,   "wP.png"},
        {chess::B_KING,   "bK.png"}, {chess::B_QUEEN,  "bQ.png"},
        {chess::B_ROOK,   "bR.png"}, {chess::B_BISHOP, "bB.png"},
        {chess::B_KNIGHT, "bN.png"}, {chess::B_PAWN,   "bP.png"},
    };

    textures_loaded_ = true;
    for (auto& pf : files) {
        sf::Texture tex;
        if (!tex.loadFromFile(asset_path + "/" + pf.filename)) {
            std::cerr << "Warning: Could not load " << pf.filename
                      << " - using text fallback\n";
            textures_loaded_ = false;
        } else {
            tex.setSmooth(true);
            piece_textures_[pf.piece] = tex;
        }
    }
    return textures_loaded_;
}

void Renderer::draw_board() {
    for (int rank = 0; rank < 8; ++rank) {
        for (int file = 0; file < 8; ++file) {
            sf::RectangleShape sq(sf::Vector2f(SQUARE_SIZE, SQUARE_SIZE));
            sq.setPosition(file * SQUARE_SIZE, (7 - rank) * SQUARE_SIZE);
            sq.setFillColor(((file + rank) % 2 == 0) ? dark_color_ : light_color_);
            window_.draw(sq);
        }
    }
}

void Renderer::draw_square_at(int file, int rank, const sf::Color& color, bool flipped) {
    int draw_file = flipped ? (7 - file) : file;
    int draw_rank = flipped ? rank : (7 - rank);
    sf::RectangleShape sq(sf::Vector2f(SQUARE_SIZE, SQUARE_SIZE));
    sq.setPosition(draw_file * SQUARE_SIZE, draw_rank * SQUARE_SIZE);
    sq.setFillColor(color);
    window_.draw(sq);
}

void Renderer::draw_pieces(const chess::Board& board, bool flipped) {
    const char* piece_chars = " PNBRQK  pnbrqk";

    for (int sq = 0; sq < 64; ++sq) {
        chess::Piece p = board.piece_on(chess::Square(sq));
        if (p == chess::NO_PIECE) continue;

        int file = chess::file_of(chess::Square(sq));
        int rank = chess::rank_of(chess::Square(sq));
        int draw_file = flipped ? (7 - file) : file;
        int draw_rank = flipped ? rank : (7 - rank);

        float x = draw_file * SQUARE_SIZE;
        float y = draw_rank * SQUARE_SIZE;

        auto it = piece_textures_.find(p);
        if (it != piece_textures_.end()) {
            sf::Sprite sprite(it->second);
            float scale = float(SQUARE_SIZE) / it->second.getSize().x;
            sprite.setScale(scale, scale);
            sprite.setPosition(x, y);
            window_.draw(sprite);
        } else if (font_loaded_) {
            // Text fallback
            sf::Text text;
            text.setFont(font_);
            text.setString(std::string(1, piece_chars[p]));
            text.setCharacterSize(48);
            text.setFillColor(chess::piece_color(p) == chess::WHITE ?
                              sf::Color::White : sf::Color::Black);
            if (chess::piece_color(p) == chess::WHITE) {
                text.setOutlineColor(sf::Color::Black);
                text.setOutlineThickness(2);
            }
            sf::FloatRect bounds = text.getLocalBounds();
            text.setPosition(x + (SQUARE_SIZE - bounds.width) / 2 - bounds.left,
                           y + (SQUARE_SIZE - bounds.height) / 2 - bounds.top);
            window_.draw(text);
        }
    }
}

void Renderer::draw_highlights(chess::Square selected, const chess::MoveList& legal_moves, bool flipped) {
    if (selected == chess::SQ_NONE) return;

    // Highlight selected square
    draw_square_at(chess::file_of(selected), chess::rank_of(selected), highlight_color_, flipped);

    // Highlight legal destinations
    for (int i = 0; i < legal_moves.count; ++i) {
        if (legal_moves.moves[i].from() == selected) {
            chess::Square to = legal_moves.moves[i].to();
            draw_square_at(chess::file_of(to), chess::rank_of(to), legal_move_color_, flipped);
        }
    }
}

void Renderer::draw_last_move(chess::Move last_move, bool flipped) {
    if (!last_move) return;
    draw_square_at(chess::file_of(last_move.from()), chess::rank_of(last_move.from()), last_move_color_, flipped);
    draw_square_at(chess::file_of(last_move.to()), chess::rank_of(last_move.to()), last_move_color_, flipped);
}

void Renderer::draw_check_highlight(chess::Square king_sq, bool flipped) {
    draw_square_at(chess::file_of(king_sq), chess::rank_of(king_sq), check_color_, flipped);
}

void Renderer::draw_info_bar(const std::string& text) {
    // Background
    sf::RectangleShape bar(sf::Vector2f(WINDOW_WIDTH, INFO_BAR_HEIGHT));
    bar.setPosition(0, BOARD_SIZE);
    bar.setFillColor(sf::Color(40, 40, 40));
    window_.draw(bar);

    if (!font_loaded_) return;

    sf::Text info;
    info.setFont(font_);
    info.setString(text);
    info.setCharacterSize(20);
    info.setFillColor(sf::Color::White);
    sf::FloatRect bounds = info.getLocalBounds();
    info.setPosition(10, BOARD_SIZE + (INFO_BAR_HEIGHT - bounds.height) / 2 - bounds.top);
    window_.draw(info);
}

void Renderer::draw_promotion_dialog(chess::Color color, chess::Square sq, bool flipped) {
    int file = chess::file_of(sq);
    int draw_file = flipped ? (7 - file) : file;

    chess::PieceType promos[] = {chess::QUEEN, chess::ROOK, chess::BISHOP, chess::KNIGHT};
    bool white = (color == chess::WHITE);
    int start_rank = white ? 0 : 4;

    // Dim overlay
    sf::RectangleShape overlay(sf::Vector2f(WINDOW_WIDTH, BOARD_SIZE));
    overlay.setFillColor(sf::Color(0, 0, 0, 128));
    window_.draw(overlay);

    for (int i = 0; i < 4; ++i) {
        float x = draw_file * SQUARE_SIZE;
        float y = (start_rank + i) * SQUARE_SIZE;

        sf::RectangleShape bg(sf::Vector2f(SQUARE_SIZE, SQUARE_SIZE));
        bg.setPosition(x, y);
        bg.setFillColor(sf::Color(220, 220, 220));
        bg.setOutlineColor(sf::Color::Black);
        bg.setOutlineThickness(2);
        window_.draw(bg);

        chess::Piece p = chess::make_piece(color, promos[i]);
        auto it = piece_textures_.find(p);
        if (it != piece_textures_.end()) {
            sf::Sprite sprite(it->second);
            float scale = float(SQUARE_SIZE) / it->second.getSize().x;
            sprite.setScale(scale, scale);
            sprite.setPosition(x, y);
            window_.draw(sprite);
        } else if (font_loaded_) {
            const char* piece_chars = " PNBRQK  pnbrqk";
            sf::Text text;
            text.setFont(font_);
            text.setString(std::string(1, piece_chars[p]));
            text.setCharacterSize(48);
            text.setFillColor(white ? sf::Color::White : sf::Color::Black);
            sf::FloatRect bounds = text.getLocalBounds();
            text.setPosition(x + (SQUARE_SIZE - bounds.width) / 2 - bounds.left,
                           y + (SQUARE_SIZE - bounds.height) / 2 - bounds.top);
            window_.draw(text);
        }
    }
}

sf::Vector2i Renderer::square_from_pixel(int x, int y, bool flipped) const {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE)
        return {-1, -1};
    int file = x / SQUARE_SIZE;
    int rank = 7 - y / SQUARE_SIZE;
    if (flipped) {
        file = 7 - file;
        rank = 7 - rank;
    }
    return {file, rank};
}

} // namespace gui
