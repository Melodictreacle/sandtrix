"""Configuration and constants for Sandtrix."""

import os

# Base Dimensions
BOARD_WIDTH = 80          # Sand grid columns (grains)
BOARD_HEIGHT = 140        # Sand grid rows (grains)
MINO_SIZE = 8             # Each Tetromino block consists of 8x8 sand grains
CELL_DISPLAY_SIZE = 5     # Pixel scale on screen (80 * 5 = 400px wide, 140 * 5 = 700px tall)

CANVAS_WIDTH = BOARD_WIDTH * CELL_DISPLAY_SIZE    # 400 pixels
CANVAS_HEIGHT = BOARD_HEIGHT * CELL_DISPLAY_SIZE  # 700 pixels

# Danger Ceiling (Row index above which sand triggers warning/game over)
DANGER_ROW = 16           # Top 16 grains (approx 2 minos from top)

# Timings (milliseconds)
FRAME_RATE = 60
FRAME_TIME_MS = int(1000 / FRAME_RATE)  # ~16ms

# Base gravity drop intervals per level (ms per 1-grain step)
GRAVITY_INTERVALS = {
    1: 45,
    2: 40,
    3: 35,
    4: 30,
    5: 25,
    6: 20,
    7: 16,
    8: 13,
    9: 10,
    10: 8,
}
SOFT_DROP_GRAVITY_MS = 4   # Fast falling speed when holding Down/S

# Lock delay: time piece can rest on ground/sand before locking into sand
LOCK_DELAY_MS = 140

# DAS (Delayed Auto Shift) and ARR (Auto Repeat Rate)
DAS_DELAY_MS = 140
ARR_REPEAT_MS = 35

# Physics Sub-steps per tick for fluid sand cascading
PHYSICS_SUBSTEPS = 2

# Scoring Values
POINTS_PER_GRAIN = 5
POINTS_PER_BAND = 400
COMBO_MULTIPLIER_BASE = 1.5

# Special Elemental Material IDs
MATERIAL_WATER     = 50   # Liquid: flows horizontally, buoyant
MATERIAL_ACID      = 51   # Acid: dissolves sand directly below it
MATERIAL_BOMB      = 52   # TNT Bomb: explodes crater
MATERIAL_ICE       = 53   # Ice: frozen water, freezes water on downward movement
MATERIAL_FIRE      = 54   # Fire: blazing heat, melts ice, ignites acid, burns sand
MATERIAL_STEAM     = 55   # Steam: light vapor gas, rises upward, dissipates after few seconds
MATERIAL_TOXIC_GAS = 56   # Toxic Cloud: green vapor from fire + acid, rises & dissipates
MATERIAL_ASH       = 57   # Ash: burnt sand residue, charcoal ash gray powder

# Distinct Elemental Colors (RGBA)
ELEMENT_COLORS = {
    MATERIAL_WATER:     (0, 210, 215, 255),    # Turquoise Aqua (#00D2D7)
    MATERIAL_ACID:      (205, 235, 45, 255),   # Chartreuse Lime (#CDEB2D)
    MATERIAL_BOMB:      (255, 75, 75, 255),    # TNT Red (#FF4B4B)
    MATERIAL_ICE:       (153, 230, 255, 255),  # Glacial Frost Blue (#99E6FF)
    MATERIAL_FIRE:      (255, 69, 0, 255),     # Blazing Fiery Orange (#FF4500)
    MATERIAL_STEAM:     (225, 235, 245, 220),  # Steam Mist White/Blue (#E1EBF5)
    MATERIAL_TOXIC_GAS: (57, 255, 20, 240),    # Radioactive Toxic Green (#39FF14)
    MATERIAL_ASH:       (138, 144, 150, 255),  # Charcoal Ash Gray (#8A9096)
}

# High Scores file path
HIGHSCORE_FILE = os.path.join(os.path.dirname(__file__), "highscores.json")

# Color Palettes
# Each palette contains:
# index 0: Background dark color (RGBA)
# index 1..N: Vibrant sand colors (RGBA)
PALETTES = {
    "Cyber Neon": {
        "description": "Vibrant arcade neon with 4 balanced colors",
        "colors": [
            (15, 17, 26, 255),       # 0: Dark Void background
            (255, 75, 130, 255),     # 1: Neon Pink / Coral
            (0, 230, 255, 255),      # 2: Electric Cyan
            (255, 215, 0, 255),      # 3: Radiant Amber Gold
            (46, 213, 115, 255),     # 4: Mint Emerald Green
        ]
    },
    "Synthwave": {
        "description": "80s retro synthwave aesthetic",
        "colors": [
            (20, 14, 34, 255),       # 0: Deep Synthwave Purple
            (255, 46, 147, 255),     # 1: Hot Magenta
            (162, 89, 255, 255),     # 2: Electric Violet
            (0, 240, 255, 255),      # 3: Cyber Cyan
            (255, 230, 0, 255),      # 4: Laser Yellow
        ]
    },
    "Pastel Sand": {
        "description": "Soft soothing desert sand tones",
        "colors": [
            (26, 28, 35, 255),       # 0: Charcoal Dark
            (242, 132, 130, 255),    # 1: Soft Coral
            (246, 189, 96, 255),     # 2: Sandy Gold
            (132, 165, 157, 255),    # 3: Sage Mint
            (142, 202, 230, 255),    # 4: Sky Blue
        ]
    },
    "Classic 7": {
        "description": "Expert mode with all 7 traditional Tetris colors",
        "colors": [
            (14, 16, 24, 255),       # 0: Dark Navy
            (0, 235, 235, 255),      # 1: Cyan (I)
            (245, 220, 0, 255),      # 2: Yellow (O)
            (170, 0, 255, 255),      # 3: Purple (T)
            (0, 230, 80, 255),       # 4: Green (S)
            (255, 40, 50, 255),      # 5: Red (Z)
            (30, 100, 255, 255),     # 6: Blue (J)
            (255, 140, 0, 255),      # 7: Orange (L)
        ]
    }
}

DEFAULT_PALETTE = "Cyber Neon"
