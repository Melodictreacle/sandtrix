#pragma once

#include <QString>
#include <QColor>
#include <vector>
#include <map>

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

// Color Palette struct
struct ColorPalette {
    QString name;
    QString description;
    std::vector<QColor> colors; // Index 0: Background; Index 1..N: Sand colors
};

inline const std::vector<ColorPalette>& getAvailablePalettes() {
    static const std::vector<ColorPalette> palettes = {
        {
            "Cyber Neon",
            "Vibrant arcade neon with 4 balanced colors",
            {
                QColor(15, 17, 26),     // 0: Dark Void background
                QColor(255, 75, 130),   // 1: Neon Pink / Coral
                QColor(0, 230, 255),    // 2: Electric Cyan
                QColor(255, 215, 0),    // 3: Radiant Amber Gold
                QColor(46, 213, 115)    // 4: Mint Emerald Green
            }
        },
        {
            "Synthwave",
            "80s retro synthwave aesthetic",
            {
                QColor(20, 14, 34),     // 0: Deep Purple background
                QColor(255, 46, 147),   // 1: Hot Magenta
                QColor(162, 89, 255),   // 2: Electric Violet
                QColor(0, 240, 255),    // 3: Cyber Cyan
                QColor(255, 230, 0)     // 4: Laser Yellow
            }
        },
        {
            "Pastel Sand",
            "Soft soothing desert sand tones",
            {
                QColor(26, 28, 35),     // 0: Charcoal Dark background
                QColor(242, 132, 130),  // 1: Soft Coral
                QColor(246, 189, 96),   // 2: Sandy Gold
                QColor(132, 165, 157),  // 3: Sage Mint
                QColor(142, 202, 230)   // 4: Sky Blue
            }
        },
        {
            "Classic 7",
            "Expert mode with all 7 traditional Tetris colors",
            {
                QColor(14, 16, 24),     // 0: Dark Navy background
                QColor(0, 235, 235),    // 1: Cyan (I)
                QColor(245, 220, 0),    // 2: Yellow (O)
                QColor(170, 0, 255),    // 3: Purple (T)
                QColor(0, 230, 80),     // 4: Green (S)
                QColor(255, 40, 50),    // 5: Red (Z)
                QColor(30, 100, 255),   // 6: Blue (J)
                QColor(255, 140, 0)     // 7: Orange (L)
            }
        }
    };
    return palettes;
}
