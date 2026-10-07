# ⏳ SANDTRIX - Sand Tetris for Qt (C++)

A high-performance, polished C++ implementation of **Sandtrix** (Sand Tetris) designed for **Qt Creator** using **Qt 6** and modern **C++17**, featuring an **Elemental Materials System** (Liquid / Water & Acid blocks)!

Falling tetromino blocks dissolve into loose sand grains upon landing, and lines are cleared when matching sand colors form an unbroken path from the left wall to the right wall!

---

## 🌊 Elemental Materials System

In addition to standard colored sand, special elemental pieces drop periodically:

1. **🌊 Water / Liquid Blocks**:
   - **Fluid Dynamics**: Unlike sand which rests at a 45° angle, water flows **horizontally** to level out, seeking low crevices and filling deep holes flat.
   - **Buoyancy / Density Displacement**: Sand is denser than water! When sand lands on water, the sand **sinks to the bottom** and displaces the water upward, forming realistic sandbars and rising pools.
   - **🌊 Tidal Wave Clear**: When water bridges the left wall to the right wall, it triggers a **Tidal Wave** with aquatic splash particles and double line clear bonus points (+800)!

2. **☣ Acid / Dissolver Blocks**:
   - **Corrosive Reaction**: Rare radioactive green pieces that eat through and dissolve sand directly below them on contact, helping dig through tall piles before neutralizing with a bubbling sizzle effect!

---

## 🛠️ How to Open and Run in Qt Creator

### Method 1: Using CMake (Recommended)
1. Launch **Qt Creator** (located at `C:\Qt\Tools\QtCreator\bin\qtcreator.exe`).
2. Go to **File → Open File or Project...** (`Ctrl + O`).
3. Navigate to `C:\Users\omega\Desktop\sandtrix` and select [`CMakeLists.txt`](file:///C:/Users/omega/Desktop/sandtrix/CMakeLists.txt).
4. On the **Configure Project** screen, choose the **Desktop Qt 6.12.0 MinGW 64-bit** kit.
5. Click **Configure Project**.
6. Click the green **Run** button at the bottom-left of Qt Creator (or press `Ctrl + R`).

### Method 2: Using qmake
1. In Qt Creator, select **File → Open File or Project...**
2. Choose [`sandtrix.pro`](file:///C:/Users/omega/Desktop/sandtrix/sandtrix.pro).
3. Select the **Desktop Qt 6.12.0 MinGW 64-bit** kit and click **Configure Project**.
4. Press `Ctrl + R` to build and run!

### Method 3: One-Click Quick Run (Command Line / Explorer)
Simply double-click [`build.bat`](file:///C:/Users/omega/Desktop/sandtrix/build.bat) inside the `sandtrix` folder to compile and launch automatically.

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
├── .github/workflows/    # Automated GitHub Actions CI workflow
├── src/
│   ├── main.cpp          # Application entry point, dark Fusion palette
│   ├── Config.h          # Grid dimensions, timings, scoring, elemental materials & palettes
│   ├── Tetromino.h       # Mino matrices, SRS wall kicks, elemental piece identification
│   ├── Tetromino.cpp
│   ├── SandEngine.h      # Cellular automata, fluid lateral flow, density buoyancy, acid corrosion
│   ├── SandEngine.cpp
│   ├── ParticleSystem.h  # Sand sparks, water splashes, acid bubbles, floating text, screen shake
│   ├── ParticleSystem.cpp
│   ├── AudioManager.h    # Procedural sound synthesizer (clicks, drops, chimes, splashes, sizzles)
│   ├── AudioManager.cpp
│   ├── UIPanels.h        # Stat cards, combo badges, next/hold previews with elemental badges
│   ├── UIPanels.cpp
│   ├── GameCanvas.h      # 60 FPS interactive canvas, direct QImage pixel blit, ghost piece
│   ├── GameCanvas.cpp
│   ├── MainWindow.h      # Window layout, side panels, HUD sync
│   └── MainWindow.cpp
└── sounds/               # Procedurally generated WAV sound cache
```
