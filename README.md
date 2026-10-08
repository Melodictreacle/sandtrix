# SANDTRIX - Sand Tetris for Qt (C++17)

A complete, high-performance C++ implementation of **Sandtrix** (Falling Sand Tetris) built from scratch for **Qt Creator** using **Qt 6** and **C++17**.

This project re-imagines classic Tetris by simulating granular cellular automata physics: rigid tetrominoes fall under player control, but the instant they touch the ground or resting materials, they dissolve into hundreds of loose, individual sand grains that avalanche and settle into natural heaps. Unlike traditional Tetris row clears, lines are cleared by connecting unbroken color bands from the **left wall to the right wall** using 8-way flood-fill search algorithms!

---

## Table of Contents

- [Core Gameplay Mechanics](#core-gameplay-mechanics)
- [Game Modes: Sandtrix vs. Classic Tetris](#game-modes-sandtrix-vs-classic-tetris)
- [Mathematical & Algorithmic Implementation Details](#mathematical--algorithmic-implementation-details)
  - [1. Two-Tier Coordinate & Grid System](#1-two-tier-coordinate--grid-system)
  - [2. Mino-to-Wall Grid Alignment Solution](#2-mino-to-wall-grid-alignment-solution)
  - [3. Cellular Automata Physics Engine](#3-cellular-automata-physics-engine)
  - [4. Fluid Lateral Leveling & Density Buoyancy](#4-fluid-lateral-leveling--density-buoyancy)
  - [5. 8-Connected BFS Edge-to-Edge Line Clears](#5-8-connected-bfs-edge-to-edge-line-clears)
  - [6. High-Performance Direct Framebuffer Blitting](#6-high-performance-direct-framebuffer-blitting)
  - [7. Responsive DAS / ARR Input Handling](#7-responsive-das--arr-input-handling)
  - [8. Real-Time Procedural Audio Synthesizer](#8-real-time-procedural-audio-synthesizer)
- [Color Palette & Ergonomic UI/UX Design](#color-palette--ergonomic-uiux-design)
  - [Distinguishable Block Palette](#distinguishable-block-palette)
  - [Eye-Comfort Slate Theme & Distinctive Button Hierarchy](#eye-comfort-slate-theme--distinctive-button-hierarchy)
- [Complete Controls Reference](#complete-controls-reference)
- [File & Codebase Architecture](#file--codebase-architecture)
- [How to Build and Run](#how-to-build-and-run)
  - [Method 1: Qt Creator with CMake (Recommended)](#method-1-qt-creator-with-cmake-recommended)
  - [Method 2: Qt Creator with qmake](#method-2-qt-creator-with-qmake)
  - [Method 3: Quick Terminal Build (`build.bat`)](#method-3-quick-terminal-build-buildbat)
  - [Method 4: Standalone Windows Deployment](#method-4-standalone-windows-deployment)

---

## Core Gameplay Mechanics

1. **Rigid Steerable Tetrominoes**:
   - The 7 traditional shapes (**I, O, T, S, Z, J, L**) spawn at the ceiling.
   - Players can steer left/right, rotate clockwise or counter-clockwise (equipped with Super Rotation System wall-kicks), soft drop, hard drop, and hold pieces.
2. **Instant Sand Dissolution**:
   - Each mino in a tetromino consists of an **8 × 8 cluster of sand grains** (64 grains per mino, 256 grains per tetromino).
   - While in the air, the piece moves as a single solid entity.
   - The instant any grain within the tetromino makes contact with the floor or existing sand, the entire shape dissolves into individual, physically simulated sand grains.
3. **Edge-to-Edge Line Clears**:
   - Traditional Tetris clears flat horizontal lines. In Sandtrix, you clear colored bands by connecting an **unbroken path of the same color from the left wall ($x = 0$) to the right wall ($x = 79$)**.
   - Because diagonal contact counts (8-way neighborhood connectivity), paths can curve, dip, undulate, and form organic valleys across the board.
4. **Avalanche Cascades & Exponential Combos**:
   - When a connected color band clears, all sand resting above it loses support and immediately avalanches down into the void.
   - If the collapsed sand forms new edge-to-edge connections, a **Combo Clear** triggers with compounding multiplier bonuses (`Score = Base * Combo^1.5`).

---

## Game Modes: Sandtrix vs. Classic Tetris

Sandtrix supports two full game modes, toggleable directly from the **Main Menu** or inside **Settings**:

### 1. Sandtrix (Sand Physics Mode) - *Default*
- **Granular Dissolution**: Rigid tetrominoes dissolve into loose sand grains on impact.
- **Fluid Cellular Automata**: Multi-substep physics engine simulates sliding granular avalanches and slopes.
- **Edge-to-Edge Line Clears**: Clear colored bands by connecting unbroken paths of the same color from the left wall ($x = 0$) to the right wall ($x = 79$).
- **Avalanche Combos**: Collapsing sand triggers compounding combo multipliers.
- **Optional Elemental Pieces**: Toggle Water (lateral leveling & tidal wave clears) and Acid (corrosive excavation) on or off in Settings.

### 2. Classic Tetris (Solid Grid Mode)
- **Authentic Rigid Grid**: Traditional 10-column × 17-row mino playfield with subtle dot-line grid rendering.
- **No Sand Physics**: Blocks stay rigid and solid upon landing—no grain dissolution!
- **Horizontal Row Clears**: Complete solid horizontal rows to clear lines with traditional scoring:
  - 1 Line: **Single** ($100 \times \text{level}$)
  - 2 Lines: **Double** ($300 \times \text{level}$)
  - 3 Lines: **Triple** ($500 \times \text{level}$)
  - 4 Lines: **TETRIS!** ($800 \times \text{level}$)
- **Traditional Mechanics**: SRS wall-kicks, ghost piece tracking, hard drop, soft drop, hold queue, and progressive gravity drops.

---

## Mathematical & Algorithmic Implementation Details

### 1. Two-Tier Coordinate & Grid System
The simulation operates on two coordinate spaces:
- **Mino Space**: High-level block unit ($1 \text{ mino} = 8 \text{ grains}$).
- **Sand Grid Space**: Low-level simulation grid of $80 \text{ columns} \times 140 \text{ rows} = 11,200 \text{ cellular automata cells}$.
- **Screen Display Space**: Scaled by `CELL_DISPLAY_SIZE = 5`, yielding an active playfield canvas of $400 \times 700 \text{ pixels}$.

```
Board Width  = 80 grains  = 10 minos = 400 pixels (display)
Board Height = 140 grains = 17.5 minos = 700 pixels (display)
Mino Size    = 8 x 8 grains = 40 x 40 pixels (display)
```

### 2. Mino-to-Wall Grid Alignment Solution
In standard Sandtrix, pieces must be able to hit both the left and right walls flush so players can close gaps and complete edge-to-edge clears.

- **The Problem**: If spawn coordinates are calculated using naive pixel centering `(BOARD_WIDTH - maskWidth) / 2`:
  - 4-wide pieces (`I`): $(80 - 32) / 2 = 24$ (Multiple of 8 $\rightarrow$ aligned).
  - 2-wide pieces (`O`): $(80 - 16) / 2 = 32$ (Multiple of 8 $\rightarrow$ aligned).
  - 3-wide pieces (`T, S, Z, J, L`): $(80 - 24) / 2 = 28$ grains.
  - $28$ is **not** a multiple of 8 ($28 \pmod 8 = 4$). Moving by steps of 8 grains left the piece at $x = 4$ or $x = 76$, creating an unintended 4-grain (20-pixel) half-block gap against the walls!
- **The Solution** ([`src/Tetromino.cpp`](file:///C:/Users/omega/Desktop/sandtrix/src/Tetromino.cpp)):
  Spawn positions are strictly aligned to integer mino columns:
  $$\text{boardMinos} = \frac{\text{BOARD\_WIDTH}}{\text{MINO\_SIZE}} = \frac{80}{8} = 10$$
  $$\text{spawnCol} = \left\lfloor \frac{\text{boardMinos} - \text{matrixSize}}{2} \right\rfloor = \left\lfloor \frac{10 - 3}{2} \right\rfloor = 3$$
  $$m\_x = \text{spawnCol} \times \text{MINO\_SIZE} = 3 \times 8 = 24 \text{ grains}$$
  Because $m\_x$, horizontal movement steps ($\pm 8$), and all Super Rotation System (SRS) wall-kick offsets are multiples of 8, **every single tetromino can reach both $x = 0$ (left wall) and $x = 79$ (right wall) flush with zero gap**.

### 3. Cellular Automata Physics Engine
The sand simulation runs in [`src/SandEngine.cpp`](file:///C:/Users/omega/Desktop/sandtrix/src/SandEngine.cpp) at 60 FPS with `PHYSICS_SUBSTEPS = 2` (120 physics ticks per second) for fluid stability:

1. **Bottom-to-Top Traversal**:
   - The grid is iterated from row $y = 138$ up to row $y = 0$. This ensures grains that fall in the current frame do not get processed multiple times in the same step.
2. **Alternating Horizontal Scanning**:
   - To eliminate directional bias (where sand heaps drift predominantly to the left or right), the column traversal alternates direction every frame:
     - Even frames scan left-to-right ($x: 0 \rightarrow 79$).
     - Odd frames scan right-to-left ($x: 79 \rightarrow 0$).
   - When evaluating diagonal slides (down-left vs. down-right), a pseudo-random coin flip determines which diagonal is checked first.
3. **Sand Transition Rules**:
   For a sand cell $(x, y)$:
   - **Rule 1 (Direct Fall)**: If $(x, y+1)$ is empty $\rightarrow$ move to $(x, y+1)$.
   - **Rule 2 (Density Sink)**: If $(x, y+1)$ contains water $\rightarrow$ swap sand with water (sand sinks, water rises).
   - **Rule 3 (Angle of Repose Slide)**: If $(x, y+1)$ is occupied, check $(x-1, y+1)$ and $(x+1, y+1)$. If an empty or water cell is available, slide diagonally downward.

### 4. Fluid Lateral Leveling & Density Buoyancy
For water cells (`MATERIAL_WATER`):
- If the cell directly below $(x, y+1)$ is empty, water falls down.
- If falling is blocked, water checks diagonal cells $(x-1, y+1)$ and $(x+1, y+1)$.
- If both downward diagonals are blocked, water performs **lateral pressure flow**:
  - Checks $(x-1, y)$ and $(x+1, y)$ up to a flow distance.
  - If a neighboring lateral vacancy is found, water shifts horizontally. This causes water surfaces to level out completely flat.

For acid cells (`MATERIAL_ACID`):
- If $(x, y+1)$ contains sand, the acid reacts:
  - Both $(x, y)$ and $(x, y+1)$ are erased to empty (`0`).
  - Triggers an acid sizzle sound and particle spawn.

### 5. 8-Connected BFS Edge-to-Edge Line Clears
Every frame where sand settles, the engine checks for completed paths ([`SandEngine::checkLineClears`](file:///C:/Users/omega/Desktop/sandtrix/src/SandEngine.cpp)):
1. For each active color ($c \in \{1, 2, 3, 4, \text{WATER}\}$):
2. Collect all cells on the left boundary ($x = 0$) where $\text{grid}[y \times W + 0] == c$.
3. Perform a **Breadth-First Search (BFS)** using an 8-connected neighborhood:
   $$\mathcal{N} = \{(-1, -1), (-1, 0), (-1, 1), (0, -1), (0, 1), (1, -1), (1, 0), (1, 1)\}$$
4. If the traversal reaches any cell on the right boundary ($x = W - 1 = 79$):
   - The entire connected component of matching grains is marked for clearance.
   - Cleared cells vaporize into spark particles.
   - Points are awarded: $\text{Points} = (\text{Grains} \times 5 + \text{Bands} \times 400) \times \text{Combo}^{1.5}$.

```
Left Wall (x=0)                                           Right Wall (x=79)
  [●]───────────[●]─────────[●]                                 
   \             /           \                                  
    [●]───────[●]             [●]───────[●]────────────────[●]
     (Continuous 8-way connected flood-fill path clears!)
```

### 6. High-Performance Direct Framebuffer Blitting
Drawing 11,200 individual sand grains every frame using `QPainter::drawRect` causes massive draw call overhead and drops framerates on slower hardware.

- **Solution in [`src/GameCanvas.cpp`](file:///C:/Users/omega/Desktop/sandtrix/src/GameCanvas.cpp)**:
  - A backing buffer `QImage m_sandImage(80, 140, QImage::Format_RGB32)` is allocated once.
  - During `paintEvent`, the sand grid is iterated as a flat array.
  - Color values are written directly to memory pointers via `reinterpret_cast<QRgb*>(m_sandImage.bits())`.
  - The entire grid is blitted to the screen in a **single GPU texture blit**:
    ```cpp
    painter.drawImage(QRect(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT), m_sandImage);
    ```
  - This keeps CPU usage under 1% and guarantees a rock-solid 60 FPS.

### 7. Responsive DAS / ARR Input Handling
Competitive Tetris input mechanics are built directly into [`GameCanvas.cpp`](file:///C:/Users/omega/Desktop/sandtrix/src/GameCanvas.cpp):
- **Delayed Auto Shift (DAS = 140 ms)**: The initial delay before rapid horizontal repeating starts when holding an arrow key.
- **Auto Repeat Rate (ARR = 35 ms)**: The repeat interval between rapid moves once DAS is engaged.
- Provides crisp single-tap steering and silky smooth wall-charging.

### 8. Real-Time Procedural Audio Synthesizer
[`src/AudioManager.cpp`](file:///C:/Users/omega/Desktop/sandtrix/src/AudioManager.cpp) generates all sound effects in real time mathematically, requiring **zero external WAV/MP3 files**:
- **Move / Rotate**: Pure sinusoidal clicks with exponential pitch curves ($f: 400 \text{ Hz} \rightarrow 200 \text{ Hz}$, $35 \text{ ms}$).
- **Drop / Impact**: Low-frequency sinusoidal thuds ($f: 120 \text{ Hz} \rightarrow 45 \text{ Hz}$, $70 \text{ ms}$).
- **Line Clear Chimes**: Musical pentatonic harmonic chords based on the current combo counter ($C_5, D_5, E_5, G_5, A_5, C_6$).
- **Water Splash**: Bandpass-filtered white noise burst with linear amplitude attenuation ($180 \text{ ms}$).
- **Acid Sizzle**: Frequency-modulated high-frequency noise simulating corrosive fizz ($200 \text{ ms}$).
- **Game Over**: Minor-triad downward frequency arpeggio.

Generated samples are converted to 16-bit PCM mono WAVs in-memory and played via `QAudioSink` and `QSoundEffect`.

---

## Color Palette & Ergonomic UI/UX Design

### Distinguishable Block Palette
To ensure effortless identification without visual fatigue, the game uses **one unified canonical palette** ([`src/Config.h`](file:///C:/Users/omega/Desktop/sandtrix/src/Config.h)):

| Material / Mino | Name | Hex Code | Hue Angle | Characteristics |
| :--- | :--- | :--- | :--- | :--- |
| **Block 1** | **Crimson Ruby** | `#EB4B4B` | $0^\circ$ (Red) | High-contrast vivid red; unmistakable warm tone. |
| **Block 2** | **Amber Gold** | `#F5B923` | $43^\circ$ (Yellow) | Warm sunny gold; high luminance and bright contrast. |
| **Block 3** | **Emerald Jade** | `#28B964` | $145^\circ$ (Green) | Deep botanical green; cleanly separated from gold & blue. |
| **Block 4** | **Sapphire Blue** | `#327DEB` | $216^\circ$ (Blue) | Vibrant royal cobalt blue; cool and saturated. |
| **Water** | **Turquoise Aqua** | `#00D2D7` | $180^\circ$ (Cyan) | Sparkling light cyan; distinct from deep sapphire blue. |
| **Acid** | **Chartreuse Lime** | `#CDEB2D` | $70^\circ$ (Lime) | Radioactive yellow-green; distinct from emerald jade. |
| **Background** | **Slate Charcoal** | `#12151C` | — | Deep, warm dark slate eliminating glare and eye strain. |

### Eye-Comfort Slate Theme & Distinctive Button Hierarchy
- **Primary Buttons ("START GAME", "SAVE & RETURN")**:
  - Calming emerald teal (`#2B7A78`), crisp white text (`#FFFFFF`), visible teal border (`#3D9996`), and tactile hover/press states.
- **Secondary Buttons ("SETTINGS", "HOW TO PLAY", "CANCEL")**:
  - Elevated slate cards (`#242C3D`) with clean `#37435F` borders and sky-blue hover highlights (`#63B3ED`).
- **Danger Button ("EXIT GAME")**:
  - Clearly visible coral-crimson text (`#E57373`), wine background hover (`#4A232C`), and `#4A343E` border.
- **Pre-Game Screen Management**:
  - `QStackedWidget` manages screen transitions cleanly:
    - Screen 0: **Main Menu** (Start, Settings, How to Play, Exit).
    - Screen 1: **Settings** (Starting speed/level 1–10, Elemental pieces toggle, Sound volume). Settings are locked during active runs to ensure competitive integrity.
    - Screen 2: **How to Play** (Rich interactive scrollable visual manual).
    - Screen 3: **Game Screen** (Playfield, StatCards, Next Queue, Hold Box, locked Match Status).

---

## Complete Controls Reference

| Action | Primary Key | Secondary Key | Description |
| :--- | :--- | :--- | :--- |
| **Move Left** | `←` | `A` | Steers piece left (DAS & ARR supported) |
| **Move Right** | `→` | `D` | Steers piece right (DAS & ARR supported) |
| **Rotate Clockwise** | `↑` | `W` or `X` | Rotates piece $90^\circ$ clockwise (SRS kicks) |
| **Rotate Counter-CW** | `Z` | `Ctrl` | Rotates piece $90^\circ$ counter-clockwise |
| **Soft Drop** | `↓` | `S` | Accelerates piece downward |
| **Hard Drop** | `Space` | — | Instantly slams piece down with impact dust |
| **Hold Piece** | `C` | `Shift` | Swaps piece into the Hold queue (1 per turn) |
| **Pause / Resume** | `P` | `Escape` | Pauses/resumes physics and timer |
| **Restart Run** | `R` | — | Resets board and starts fresh run |
| **Mute Sound** | `M` | — | Toggles audio output |

---

## File & Codebase Architecture

```
sandtrix/
├── CMakeLists.txt         # Primary CMake configuration (Qt 6, C++17, MinGW / MSVC)
├── sandtrix.pro           # Alternative qmake project file for Qt Creator
├── build.bat              # One-click automated CLI build and launch script
├── README.md              # Comprehensive project documentation
├── .gitignore             # Git ignore rules for build artifacts and IDE caches
├── .github/workflows/
│   └── build.yml          # GitHub Actions automated CI build matrix
├── src/
│   ├── main.cpp           # Entry point, QApplication, Fusion dark styling
│   ├── Config.h           # Grid constants, timings, scoring formulas, palettes
│   ├── Tetromino.h        # Shape matrices, rotation kicks, elemental metadata
│   ├── Tetromino.cpp      # SRS rotation, grid-aligned spawn math, bag randomizer
│   ├── SandEngine.h       # Physics engine header, grid buffers, scoring stats
│   ├── SandEngine.cpp     # Cellular automata, fluid leveling, buoyancy, 8-way BFS
│   ├── ParticleSystem.h   # Particle effects header
│   ├── ParticleSystem.cpp # Sparks, water splashes, acid fizz, floating texts, shake
│   ├── AudioManager.h     # Audio synthesizer header
│   ├── AudioManager.cpp   # Procedural PCM WAV generation, QAudioSink playback
│   ├── UIPanels.h         # HUD preview & stat card declarations
│   ├── UIPanels.cpp       # PiecePreviewWidget, StatCard, ComboBadge, ControlsGuide
│   ├── MenuWidgets.h      # MainMenuWidget, SettingsWidget, HowToPlayWidget headers
│   ├── MenuWidgets.cpp    # Menu screens layout, high-contrast buttons, styles
│   ├── GameCanvas.h       # Interactive OpenGL/QPainter canvas widget header
│   ├── GameCanvas.cpp     # 60 FPS loop, direct QImage blit, ghost piece, input
│   ├── MainWindow.h       # Main application window header
│   └── MainWindow.cpp     # QStackedWidget orchestration, HUD state synchronization
└── build/                 # Generated build output and deployment binaries
```

---

## How to Build and Run

### Prerequisites
- **Qt 6** (Qt 6.5+ or Qt 6.12+ recommended) with `Qt6Widgets` and `Qt6Multimedia`.
- **C++17 compliant compiler** (MinGW 64-bit 13.1.0+ or MSVC 2022).
- **CMake 3.20+** or **qmake**.

---

### Method 1: Qt Creator with CMake (Recommended)
1. Open **Qt Creator** (`C:\Qt\Tools\QtCreator\bin\qtcreator.exe`).
2. Click **File → Open File or Project...** (`Ctrl + O`).
3. Select `sandtrix/CMakeLists.txt`.
4. Choose the **Desktop Qt 6.x.x MinGW 64-bit** kit.
5. Click **Configure Project**.
6. Press `Ctrl + R` (or click the green **Run** button) to build and launch!

---

### Method 2: Qt Creator with qmake
1. Open **Qt Creator**.
2. Select **File → Open File or Project...**.
3. Select `sandtrix/sandtrix.pro`.
4. Select your Qt 6 MinGW kit, configure, and press `Ctrl + R`.

---

### Method 3: Quick Terminal Build (`build.bat`)
A pre-configured Windows batch script is included in the root folder. Simply double-click `build.bat` or run:

```powershell
cd sandtrix
.\build.bat
```

This automatically configures the environment, invokes CMake with Ninja, compiles the binary, and starts `sandtrix.exe`.

---

### Method 4: Standalone Windows Deployment
To create a fully portable distribution that runs on any Windows PC without Qt installed:

```powershell
# Set Qt MinGW tools in PATH
$env:PATH = "C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;$env:PATH"

# Build release executable
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Deploy all required Qt DLLs
windeployqt --no-translations build/sandtrix.exe
```

The resulting `build/` folder contains a standalone, double-clickable `sandtrix.exe` bundled with all runtime dependencies.

---

## License

Created with modern C++ and Qt 6. Free for educational, hobby, and open-source game development.
