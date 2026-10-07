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

// Soft Eye-Friendly Material Colors (Muted, soothing saturation)
inline QColor getWaterColor() { return QColor(82, 183, 181); }   // Soft calming aquamarine
inline QColor getAcidColor()  { return QColor(136, 212, 152); }  // Soft soothing mint jade

// Color Palette struct
struct ColorPalette {
    QString name;
    QString description;
    std::vector<QColor> colors; // Index 0: Background; Index 1..N: Sand colors
};

inline const std::vector<ColorPalette>& getAvailablePalettes() {
    static const std::vector<ColorPalette> palettes = {
        {
            "Soft Zen (Eye-Friendly)",
            "Calming, muted earth and pastel tones designed for low eye fatigue",
            {
                QColor(22, 25, 33),     // 0: Deep warm slate background
                QColor(228, 128, 138),  // 1: Soft Dusty Coral
                QColor(98, 186, 145),   // 2: Calming Sage Jade
                QColor(238, 192, 102),  // 3: Warm Honey Amber
                QColor(112, 168, 222)   // 4: Soft Slate Sky Blue
            }
        },
        {
            "Warm Desert",
            "Natural mineral sands: terracotta, sandstone, dune gold, and river clay",
            {
                QColor(26, 24, 28),     // 0: Warm charcoal background
                QColor(218, 130, 98),   // 1: Terracotta Clay
                QColor(230, 190, 118),  // 2: Golden Sandstone
                QColor(140, 175, 142),  // 3: Desert Sage
                QColor(128, 160, 186)   // 4: Oasis Teal
            }
        },
        {
            "Nordic Dusk",
            "Cool, soothing Scandinavian twilight palette with gentle contrasts",
            {
                QColor(20, 23, 31),     // 0: Nordic Slate background
                QColor(214, 134, 168),  // 1: Muted Heather Rose
                QColor(118, 184, 180),  // 2: Nordic Fjord Mint
                QColor(224, 186, 126),  // 3: Birch Amber
                QColor(124, 154, 216)   // 4: Arctic Dusk Blue
            }
        },
        {
            "Classic 7 (Soft)",
            "The 7 traditional shapes with softened, eye-friendly luminance",
            {
                QColor(20, 22, 30),     // 0: Dark Navy background
                QColor(92, 196, 212),   // 1: Soft Cyan (I)
                QColor(228, 198, 96),   // 2: Soft Sun Yellow (O)
                QColor(168, 132, 216),  // 3: Soft Lavender (T)
                QColor(108, 194, 134),  // 4: Soft Fern Green (S)
                QColor(224, 118, 118),  // 5: Soft Crimson (Z)
                QColor(116, 154, 224),  // 6: Soft Cornflower Blue (J)
                QColor(228, 154, 96)    // 7: Soft Peach Orange (L)
            }
        },
        {
            "Pastel Meadow",
            "Gentle herbal pastel hues designed for prolonged comfortable play",
            {
                QColor(21, 25, 32),     // 0: Deep night slate
                QColor(142, 202, 176),  // 1: Soft Meadow Mint
                QColor(242, 195, 138),  // 2: Soft Apricot
                QColor(186, 170, 222),  // 3: Soft Heather Violet
                QColor(138, 188, 228)   // 4: Soft Powder Blue
            }
        },
        {
            "Cyber Neon",
            "High contrast vibrant arcade neon (Legacy)",
            {
                QColor(15, 17, 26),     // 0: Dark Void background
                QColor(255, 75, 130),   // 1: Neon Pink / Coral
                QColor(0, 230, 255),    // 2: Electric Cyan
                QColor(255, 215, 0),    // 3: Radiant Amber Gold
                QColor(46, 213, 115)    // 4: Mint Emerald Green
            }
        }
    };
    return palettes;
}

// Game Settings chosen before game start
struct GameSettings {
    int paletteIndex = 0; // Default to Soft Zen
    int startingLevel = 1;
    bool enableElements = true;
    bool soundEnabled = true;
    float volume = 0.65f;
};
