#include "gui.h"
#include "../src/core/bitboard.h"

int main() {
    chess::bb::init();
    gui::GameController game;
    game.run();
    return 0;
}
