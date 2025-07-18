# Chess Database Viewer

A Qt-based chess game and database viewer. This application allows users to browse, manage, and analyze chess games stored in `.pgn` files, offering features such as search, filtering, variation tracking, and engine analysis.

![Figure 2.1b](screenshots/Figure_2.1b.png)
![Figure 2.1c](screenshots/Figure_2.1c.png)

## Features

- Browse and manage multiple chess databases
- Visual game preview and board interaction
- Add, delete, and navigate moves & variations
- UCI engine support for real-time position analysis
- Multi-tab navigation for parallel games and databases

## User Guide

For detailed instructions on how to use the software, see the [User Guide](./user-guide.md).

## Requirements

- Qt 6.5 or higher
- C++17

## Building

To build the project:

```bash
mkdir build
cd build
cmake ..
make
```

## Chess Piece Attribution

This project uses chess piece images from [Lichess.org](https://lichess.org), available under various open licenses. The original repository can be found at [https://github.com/lichess-org/lila](https://github.com/lichess-org/lila), and the files are located under [`public/piece/`](https://github.com/lichess-org/lila/tree/master/public/piece).
