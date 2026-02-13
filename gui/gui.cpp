#include "gui.h"
#include "../src/core/movegen.h"
#include <iostream>

namespace gui {

GameController::GameController()
    : window_(sf::VideoMode(Renderer::WINDOW_WIDTH, Renderer::WINDOW_HEIGHT),
              "CheStrat Chess Engine",
              sf::Style::Titlebar | sf::Style::Close),
      renderer_(window_)
{
    window_.setFramerateLimit(60);
    renderer_.load_textures("assets/pieces");
}

void GameController::new_game(chess::Color human_color) {
    engine_.new_game();
    human_color_ = human_color;
    selected_ = chess::SQ_NONE;
    last_move_ = chess::Move::none();
    pending_promo_move_ = chess::Move::none();
    ai_depth_ = 0;
    ai_score_ = 0;
    ai_nodes_ = 0;

    if (engine_.board().side_to_move() != human_color_) {
        state_ = GameState::AI_THINKING;
        start_ai_turn();
    } else {
        state_ = GameState::HUMAN_TURN;
    }
}

void GameController::start_ai_turn() {
    state_ = GameState::AI_THINKING;
    render_board_ = engine_.board();
    ai_future_ = std::async(std::launch::async, [this]() {
        chess::SearchLimits limits;
        limits.max_depth = 20;
        limits.time_ms = 3000;
        return engine_.think(limits, [this](const chess::SearchInfo& info) {
            std::lock_guard<std::mutex> lock(info_mutex_);
            ai_depth_ = info.depth;
            ai_score_ = info.score;
            ai_nodes_ = info.nodes;
        });
    });
}

std::string GameController::get_status_text() const {
    switch (state_) {
        case GameState::HUMAN_TURN:
            if (engine_.board().in_check())
                return "Your turn - CHECK!";
            return "Your turn";
        case GameState::AI_THINKING: {
            std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(info_mutex_));
            return "Thinking... depth " + std::to_string(ai_depth_) +
                   " score " + std::to_string(ai_score_) + "cp" +
                   " nodes " + std::to_string(ai_nodes_);
        }
        case GameState::PROMOTION_DIALOG:
            return "Choose promotion piece";
        case GameState::GAME_OVER:
            if (engine_.is_checkmate()) {
                chess::Color winner = ~engine_.board().side_to_move();
                return (winner == human_color_) ? "Checkmate - You win!" : "Checkmate - AI wins!";
            }
            if (engine_.is_stalemate()) return "Stalemate - Draw!";
            return "Draw (50-move rule)";
    }
    return "";
}

void GameController::handle_click(int x, int y) {
    bool flipped = (human_color_ == chess::BLACK);
    sf::Vector2i sq_pos = renderer_.square_from_pixel(x, y, flipped);
    if (sq_pos.x < 0) return;

    chess::Square clicked = chess::make_square(sq_pos.x, sq_pos.y);
    chess::MoveList legal = engine_.legal_moves();

    if (selected_ == chess::SQ_NONE) {
        // Select a piece
        chess::Piece p = engine_.board().piece_on(clicked);
        if (p != chess::NO_PIECE && chess::piece_color(p) == human_color_) {
            // Check if this piece has any legal moves
            bool has_moves = false;
            for (int i = 0; i < legal.count; ++i) {
                if (legal.moves[i].from() == clicked) { has_moves = true; break; }
            }
            if (has_moves) selected_ = clicked;
        }
    } else {
        // Try to make a move
        bool moved = false;
        for (int i = 0; i < legal.count; ++i) {
            chess::Move m = legal.moves[i];
            if (m.from() == selected_ && m.to() == clicked) {
                // Check for promotion
                if (m.is_promotion()) {
                    promo_square_ = clicked;
                    pending_promo_move_ = m; // Store one; we'll build the actual one from dialog
                    state_ = GameState::PROMOTION_DIALOG;
                    selected_ = chess::SQ_NONE;
                    return;
                }
                engine_.apply_move(m);
                last_move_ = m;
                selected_ = chess::SQ_NONE;
                moved = true;

                if (engine_.is_game_over()) {
                    state_ = GameState::GAME_OVER;
                } else {
                    start_ai_turn();
                }
                return;
            }
        }
        // If clicked on own piece, reselect
        chess::Piece p = engine_.board().piece_on(clicked);
        if (p != chess::NO_PIECE && chess::piece_color(p) == human_color_) {
            selected_ = clicked;
        } else {
            selected_ = chess::SQ_NONE;
        }
    }
}

void GameController::handle_promotion_click(int x, int y) {
    bool flipped = (human_color_ == chess::BLACK);
    int file = chess::file_of(promo_square_);
    int draw_file = flipped ? (7 - file) : file;

    int px_file = x / Renderer::SQUARE_SIZE;
    int px_rank = y / Renderer::SQUARE_SIZE;

    if (px_file != draw_file) { state_ = GameState::HUMAN_TURN; return; }

    bool white = (human_color_ == chess::WHITE);
    int start_rank = white ? 0 : 4;
    int idx = px_rank - start_rank;
    if (idx < 0 || idx >= 4) { state_ = GameState::HUMAN_TURN; return; }

    chess::PieceType promos[] = {chess::QUEEN, chess::ROOK, chess::BISHOP, chess::KNIGHT};
    chess::PieceType promo = promos[idx];

    // Build the correct promotion move
    chess::Square from = pending_promo_move_.from();
    bool is_cap = engine_.board().piece_on(promo_square_) != chess::NO_PIECE;
    // Check if it's a capture promotion
    int base = is_cap ? chess::PROMO_CAPTURE_KNIGHT : chess::PROMO_KNIGHT;
    chess::MoveFlag flag = chess::MoveFlag(base + int(promo) - int(chess::KNIGHT));

    chess::Move m(from, promo_square_, flag);
    engine_.apply_move(m);
    last_move_ = m;
    pending_promo_move_ = chess::Move::none();

    if (engine_.is_game_over()) {
        state_ = GameState::GAME_OVER;
    } else {
        start_ai_turn();
    }
}

void GameController::handle_event(const sf::Event& event) {
    if (event.type == sf::Event::Closed) {
        if (state_ == GameState::AI_THINKING) engine_.stop_thinking();
        window_.close();
        return;
    }

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::N) {
            if (state_ == GameState::AI_THINKING) engine_.stop_thinking();
            new_game(human_color_);
            return;
        }
        if (event.key.code == sf::Keyboard::F) {
            if (state_ == GameState::AI_THINKING) engine_.stop_thinking();
            new_game(~human_color_);
            return;
        }
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (state_ == GameState::HUMAN_TURN) {
            handle_click(event.mouseButton.x, event.mouseButton.y);
        } else if (state_ == GameState::PROMOTION_DIALOG) {
            handle_promotion_click(event.mouseButton.x, event.mouseButton.y);
        }
    }
}

void GameController::render() {
    bool flipped = (human_color_ == chess::BLACK);

    // Use snapshot during AI search to avoid race condition
    const chess::Board& board = (state_ == GameState::AI_THINKING) ? render_board_ : engine_.board();

    window_.clear();
    renderer_.draw_board();
    renderer_.draw_last_move(last_move_, flipped);

    if (board.in_check()) {
        renderer_.draw_check_highlight(board.king_square(board.side_to_move()), flipped);
    }

    if (state_ == GameState::HUMAN_TURN) {
        chess::MoveList legal = engine_.legal_moves();
        renderer_.draw_highlights(selected_, legal, flipped);
    }

    renderer_.draw_pieces(board, flipped);

    if (state_ == GameState::PROMOTION_DIALOG) {
        renderer_.draw_promotion_dialog(human_color_, promo_square_, flipped);
    }

    renderer_.draw_info_bar(get_status_text());
    window_.display();
}

void GameController::run() {
    new_game(chess::WHITE);

    while (window_.isOpen()) {
        sf::Event event;
        while (window_.pollEvent(event)) {
            handle_event(event);
        }

        // Check if AI is done
        if (state_ == GameState::AI_THINKING && ai_future_.valid()) {
            if (ai_future_.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                chess::Move ai_move = ai_future_.get();
                if (ai_move) {
                    engine_.apply_move(ai_move);
                    last_move_ = ai_move;
                }
                if (engine_.is_game_over()) {
                    state_ = GameState::GAME_OVER;
                } else {
                    state_ = GameState::HUMAN_TURN;
                    selected_ = chess::SQ_NONE;
                }
            }
        }

        render();
    }
}

} // namespace gui
