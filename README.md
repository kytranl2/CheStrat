# CheStrat

CheStrat is a chess engine and GUI project written in C++. It features a modular architecture with separate components for the chess engine, evaluation, search, and a graphical user interface.

## Features
- Bitboard-based chess engine
- Move generation and search algorithms
- Evaluation functions and piece-square tables
- Transposition table support
- GUI for visualizing and interacting with the engine

## Project Structure
```
assets/           # Piece images and other assets
  pieces/

src/              # Source code
  core/           # Core chess logic (bitboards, board, movegen, etc.)
  engine/         # Engine entry point and logic
  eval/           # Evaluation functions and piece-square tables
  search/         # Search algorithms and transposition table

gui/              # Graphical user interface (C++ source and headers)
  gui.cpp
  gui.h
  main.cpp
  renderer.cpp
  renderer.h

tests/            # Unit and integration tests
```

## Build Instructions
1. **Clone the repository:**
   ```sh
   git clone https://github.com/<your-username>/CheStrat.git
   cd CheStrat
   ```
2. **Build the project:**
   - Use your preferred C++ build system (e.g., CMake, Makefile, or your IDE).
   - Make sure to link any required libraries for GUI (e.g., SDL2, SFML, or Qt if used).

3. **Run the application:**
   - Execute the compiled binary from the build output directory.

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## License
[MIT](LICENSE)
