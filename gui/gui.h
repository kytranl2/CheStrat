#pragma once

#include "renderer.h"
#include "../src/engine/engine.h"
#include <future>
#include <mutex>

namespace gui {

enum class GameState {
    HUMAN_TURN,
    AI_THINKING,
    PROMOTION_DIALOG,
    GAME_OVER
};

class GameController {
public:
    GameController();
    void run();

private:
    void handle_event(const sf::Event& event);
    void handle_click(int x, int y);
    void handle_promotion_click(int x, int y);
    void start_ai_turn();
    void render();
    void new_game(chess::Color human_color);
    std::string get_status_text() const;

    sf::RenderWindow window_;
    Renderer renderer_;
    chess::Engine engine_;

    GameState state_ = GameState::HUMAN_TURN;
    chess::Color human_color_ = chess::WHITE;
    chess::Square selected_ = chess::SQ_NONE;
    chess::Move last_move_ = chess::Move::none();
    chess::Move pending_promo_move_ = chess::Move::none();
    chess::Square promo_square_ = chess::SQ_NONE;

    // AI
    std::future<chess::Move> ai_future_;
    std::mutex info_mutex_;
    int ai_depth_ = 0;
    int ai_score_ = 0;
    uint64_t ai_nodes_ = 0;
};

} // namespace gui
