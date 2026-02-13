# CheStrat

CheStrat is a chess engine and interactive GUI written in modern C++20. Play against an AI opponent powered by alpha-beta search with iterative deepening, transposition tables, and positional evaluation.

## Features

- **Bitboard-based engine** — efficient 64-bit board representation for fast move generation
- **Alpha-beta search** with quiescence search, iterative deepening, and move ordering (MVV-LVA)
- **Transposition table** (64 MB) for caching evaluated positions
- **Positional evaluation** — piece-square tables, pawn structure (doubled/isolated/passed), bishop pair, rook on open files, king safety, and mobility
- **Complete chess rules** — castling, en passant, promotion, 50-move draw, stalemate detection
- **SFML desktop GUI** — board rendering, piece sprites, move highlighting, async AI thinking, and promotion dialog

## Project Structure

```
assets/pieces/    # Chess piece images (PNG)
src/
  core/           # Board, bitboards, move generation, types
  engine/         # Engine API
  eval/           # Evaluation and piece-square tables
  search/         # Alpha-beta search and transposition table
gui/              # SFML-based graphical interface
tests/            # Unit and integration tests
```

## Prerequisites

- C++20 compiler (Clang 14+, GCC 12+, MSVC 2022+)
- CMake 3.16+
- SFML 2.5+ (`brew install sfml@2` on macOS)

## Build & Run

```sh
git clone https://github.com/<your-username>/CheStrat.git
cd CheStrat

# Configure and build
cmake -B build -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/sfml@2
cmake --build build

# Run
./build/CheStrat
```

## Controls

| Key / Action | Effect |
|---|---|
| Click piece | Select and show legal moves |
| Click highlighted square | Make move |
| **N** | New game (play as white) |
| **F** | Flip sides (new game as black) |

The info bar at the bottom displays the current game state and AI search progress (depth, score, nodes).

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## License

[MIT](LICENSE)
