# ⏳ SANDTRIX - Sand Tetris for Qt (C++)

A high-performance, polished C++ implementation of **Sandtrix** (Sand Tetris) designed for **Qt Creator** using **Qt 6** and modern **C++17**.

Falling tetromino blocks dissolve into loose sand grains upon landing, and lines are cleared when matching sand colors form an unbroken path from the left wall to the right wall!

---

## 🛠️ How to Open and Run in Qt Creator

### Method 1: Using CMake (Recommended)
1. Launch **Qt Creator** (located at `C:\Qt\Tools\QtCreator\bin\qtcreator.exe`).
2. Go to **File → Open File or Project...** (`Ctrl + O`).
3. Navigate to `C:\Users\omega\Desktop\sandtrix` and select [`CMakeLists.txt`](file:///C:/Users/omega/Desktop/sandtrix/CMakeLists.txt).
4. On the **Configure Project** screen, choose the **Desktop Qt 6.12.0 MinGW 64-bit** kit.
5. Click **Configure Project**.
6. Click the green **Run** button at the bottom-left (or press `Ctrl + R`).

### Method 2: Using qmake
1. In Qt Creator, select **File → Open File or Project...**
2. Choose [`sandtrix.pro`](file:///C:/Users/omega/Desktop/sandtrix/sandtrix.pro).
3. Select the **Desktop Qt 6.12.0 MinGW 64-bit** kit and click **Configure Project**.
4. Press `Ctrl + R` to build and run!

### Method 3: One-Click Quick Run (Command Line / Explorer)
Simply double-click [`build.bat`](file:///C:/Users/omega/Desktop/sandtrix/build.bat) inside the `sandtrix` folder to compile and launch automatically.

---

## 🌟 Game Mechanics

1. **Rigid Falling Tetrominoes**: Blocks fall in classic shapes (`I`, `O`, `T`, `S`, `Z`, `J`, `L`) that you can steer, rotate, and hold.
2. **Falling Sand Cellular Automata**: Upon landing on the floor or resting sand, the tetromino dissolves into loose sand grains that cascade and tumble naturally with a 45° angle of repose.
3. **Edge-to-Edge Line Clears**: A clear triggers whenever grains of the **same color** connect the **left wall** (`x = 0`) to the **right wall** (`x = Width - 1`) through an unbroken 8-way connected path.
4. **Avalanche Combos**: Cleared sand collapses, allowing sand on top to avalanche down. If the cascade bridges the walls again, a **Combo Clear** triggers with exponential score multipliers!

---

## 🎮 Controls

| Action | Primary Key | Secondary Key |
| :--- | :--- | :--- |
| **Move Left / Right** | `←` / `→` | `A` / `D` *(DAS & ARR enabled)* |
| **Rotate Clockwise** | `↑` | `W` or `X` |
| **Rotate Counter-Clockwise** | `Z` | `Ctrl` |
| **Soft Drop** | `↓` | `S` |
| **Hard Drop** | `Space` | — |
| **Hold Piece** | `C` | `Shift` |
| **Pause / Resume** | `P` | `Escape` |
| **Restart Game** | `R` | — |
| **Mute / Unmute** | `M` | — |

---

## 📂 Project Architecture

```
sandtrix/
├── CMakeLists.txt        # Primary Qt Creator project configuration (CMake)
├── sandtrix.pro          # Alternative Qt Creator project configuration (qmake)
├── build.bat             # One-click build & run script
├── README.md             # Documentation and instructions
├── src/
│   ├── main.cpp          # Application entry point, dark Fusion palette
│   ├── Config.h          # Grid dimensions (80x140), timings, scoring, color palettes
│   ├── Tetromino.h       # Mino matrices, SRS wall kicks, 7-bag randomizer
│   ├── Tetromino.cpp
│   ├── SandEngine.h      # Cellular automata sand physics, 8-way BFS edge-to-edge clears
│   ├── SandEngine.cpp
│   ├── ParticleSystem.h  # Sand spark bursts, floating popups, screen shake
│   ├── ParticleSystem.cpp
│   ├── AudioManager.h    # Procedural sound synthesizer with QSoundEffect
│   ├── AudioManager.cpp
│   ├── UIPanels.h        # Stat cards, combo badges, next/hold previews, controls guide
│   ├── UIPanels.cpp
│   ├── GameCanvas.h      # 60 FPS interactive canvas, direct QImage pixel blit, ghost piece
│   ├── GameCanvas.cpp
│   ├── MainWindow.h      # Window layout, side panels, HUD sync
│   └── MainWindow.cpp
└── sounds/               # Procedurally generated WAV sound cache
```
