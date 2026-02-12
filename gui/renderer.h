#pragma once

#include <SFML/Graphics.hpp>
#include "../src/engine/engine.h"
#include <map>
#include <string>

namespace gui {

class Renderer {
public:
    static constexpr int SQUARE_SIZE = 80;
    static constexpr int BOARD_SIZE = SQUARE_SIZE * 8;  // 640
    static constexpr int INFO_BAR_HEIGHT = 80;
    static constexpr int WINDOW_WIDTH = BOARD_SIZE;
    static constexpr int WINDOW_HEIGHT = BOARD_SIZE + INFO_BAR_HEIGHT;

    Renderer(sf::RenderWindow& window);

    bool load_textures(const std::string& asset_path);
    void draw_board();
    void draw_pieces(const chess::Board& board, bool flipped = false);
    void draw_highlights(chess::Square selected, const chess::MoveList& legal_moves, bool flipped = false);
    void draw_last_move(chess::Move last_move, bool flipped = false);
    void draw_info_bar(const std::string& text);
    void draw_promotion_dialog(chess::Color color, chess::Square sq, bool flipped = false);
    void draw_check_highlight(chess::Square king_sq, bool flipped = false);

    sf::Vector2i square_from_pixel(int x, int y, bool flipped = false) const;

private:
    sf::RenderWindow& window_;
    std::map<chess::Piece, sf::Texture> piece_textures_;
    sf::Font font_;
    bool font_loaded_ = false;
    bool textures_loaded_ = false;

    sf::Color light_color_{240, 217, 181};
    sf::Color dark_color_{181, 136, 99};
    sf::Color highlight_color_{255, 255, 0, 100};
    sf::Color legal_move_color_{0, 200, 0, 100};
    sf::Color last_move_color_{0, 100, 200, 80};
    sf::Color check_color_{255, 0, 0, 120};

    void draw_square_at(int file, int rank, const sf::Color& color, bool flipped);
};

} // namespace gui
