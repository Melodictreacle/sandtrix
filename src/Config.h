#pragma once

#include <QString>
#include <QColor>
#include <vector>

// Sand Grid Dimensions
constexpr int BOARD_WIDTH = 80;          // Sand grid columns (grains)
constexpr int BOARD_HEIGHT = 140;        // Sand grid rows (grains)
constexpr int MINO_SIZE = 8;             // Each Tetromino block consists of 8x8 sand grains
constexpr int CELL_DISPLAY_SIZE = 5;     // Screen scale per grain (80 * 5 = 400px, 140 * 5 = 700px)

constexpr int CANVAS_WIDTH = BOARD_WIDTH * CELL_DISPLAY_SIZE;    // 400 pixels
constexpr int CANVAS_HEIGHT = BOARD_HEIGHT * CELL_DISPLAY_SIZE;  // 700 pixels

// Danger ceiling row
constexpr int DANGER_ROW = 16;           // Top 16 grains (approx 2 minos from ceiling)

// Physics & Timing
constexpr int FRAME_RATE = 60;
constexpr int FRAME_TIME_MS = 1000 / FRAME_RATE; // ~16 ms
constexpr int SOFT_DROP_GRAVITY_MS = 4;
constexpr int LOCK_DELAY_MS = 450;
constexpr int DAS_DELAY_MS = 140;        // Delayed Auto Shift initial wait
constexpr int ARR_REPEAT_MS = 35;        // Auto Repeat Rate interval
constexpr int PHYSICS_SUBSTEPS = 2;      // Sub-steps per frame

// Scoring Constants
constexpr int POINTS_PER_GRAIN = 5;
constexpr int POINTS_PER_BAND = 400;
constexpr double COMBO_MULTIPLIER_BASE = 1.5;

// Special Elemental Material IDs
constexpr uint8_t MATERIAL_WATER = 50;   // Liquid: flows horizontally, displaced upward by sinking sand
constexpr uint8_t MATERIAL_ACID  = 51;   // Acid: dissolves sand directly below it on contact

// Distinct, High-Visibility Material Colors (Completely distinct from sand blocks)
inline QColor getWaterColor() { return QColor(0, 210, 215); }   // Distinct Turquoise Aqua (#00D2D7)
inline QColor getAcidColor()  { return QColor(205, 235, 45); }  // Distinct Chartreuse Lime (#CDEB2D)

// Color Palette struct
struct ColorPalette {
    QString name;
    QString description;
    std::vector<QColor> colors; // Index 0: Background; Index 1..N: Sand colors
};

// Single canonical, high-contrast & distinguishable color palette
inline const ColorPalette& getGamePalette() {
    static const ColorPalette palette = {
        "Sandtrix Vibrant",
        "Clear, high-contrast, and unmistakably distinct block colors",
        {
            QColor(18, 21, 28),     // 0: Deep slate background (#12151C)
            QColor(235, 75, 75),    // 1: Crimson Ruby (#EB4B4B)   - Vivid Distinct Red
            QColor(245, 185, 35),   // 2: Amber Gold (#F5B923)     - Vivid Distinct Yellow
            QColor(40, 185, 100),   // 3: Emerald Jade (#28B964)   - Vivid Distinct Green
            QColor(50, 125, 235)    // 4: Sapphire Blue (#327DEB)  - Vivid Distinct Blue
        }
    };
    return palette;
}

inline const std::vector<ColorPalette>& getAvailablePalettes() {
    static const std::vector<ColorPalette> palettes = { getGamePalette() };
    return palettes;
}

// Game Mode enumeration
enum class GameMode {
    Sandtrix = 0,       // ⏳ Sandtrix: Falling sand cellular automata, edge-to-edge flood-fill clears
    ClassicTetris = 1   // 🧱 Classic Tetris: Solid rigid minos, traditional horizontal row clears
};

// Game Settings chosen before game start
struct GameSettings {
    int paletteIndex = 0; // Single fixed palette
    int startingLevel = 1;
    GameMode gameMode = GameMode::Sandtrix; // Default is Sandtrix!
    bool enableElements = false;            // Elemental pieces (Water & Acid) in Sandtrix mode
    bool soundEnabled = true;
    float volume = 0.65f;
    bool fullscreen = false;                // Default to windowed mode (user can maximize/minimize)
};
