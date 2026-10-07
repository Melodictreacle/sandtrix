"""Tetromino shapes, rotations, collision detection, and 7-bag randomizer."""

import random
import numpy as np
from config import BOARD_WIDTH, BOARD_HEIGHT, MINO_SIZE

# Mino matrices for standard 4 rotations (0: spawn, 1: 90 CW, 2: 180, 3: 270 CCW)
TETROMINO_SHAPES = {
    'I': [
        np.array([
            [0, 0, 0, 0],
            [1, 1, 1, 1],
            [0, 0, 0, 0],
            [0, 0, 0, 0]
        ], dtype=np.uint8),
        np.array([
            [0, 0, 1, 0],
            [0, 0, 1, 0],
            [0, 0, 1, 0],
            [0, 0, 1, 0]
        ], dtype=np.uint8),
        np.array([
            [0, 0, 0, 0],
            [0, 0, 0, 0],
            [1, 1, 1, 1],
            [0, 0, 0, 0]
        ], dtype=np.uint8),
        np.array([
            [0, 1, 0, 0],
            [0, 1, 0, 0],
            [0, 1, 0, 0],
            [0, 1, 0, 0]
        ], dtype=np.uint8),
    ],
    'O': [
        np.array([
            [1, 1],
            [1, 1]
        ], dtype=np.uint8)
    ] * 4,
    'T': [
        np.array([
            [0, 1, 0],
            [1, 1, 1],
            [0, 0, 0]
        ], dtype=np.uint8),
        np.array([
            [0, 1, 0],
            [0, 1, 1],
            [0, 1, 0]
        ], dtype=np.uint8),
        np.array([
            [0, 0, 0],
            [1, 1, 1],
            [0, 1, 0]
        ], dtype=np.uint8),
        np.array([
            [0, 1, 0],
            [1, 1, 0],
            [0, 1, 0]
        ], dtype=np.uint8),
    ],
    'S': [
        np.array([
            [0, 1, 1],
            [1, 1, 0],
            [0, 0, 0]
        ], dtype=np.uint8),
        np.array([
            [0, 1, 0],
            [0, 1, 1],
            [0, 0, 1]
        ], dtype=np.uint8),
        np.array([
            [0, 0, 0],
            [0, 1, 1],
            [1, 1, 0]
        ], dtype=np.uint8),
        np.array([
            [1, 0, 0],
            [1, 1, 0],
            [0, 1, 0]
        ], dtype=np.uint8),
    ],
    'Z': [
        np.array([
            [1, 1, 0],
            [0, 1, 1],
            [0, 0, 0]
        ], dtype=np.uint8),
        np.array([
            [0, 0, 1],
            [0, 1, 1],
            [0, 1, 0]
        ], dtype=np.uint8),
        np.array([
            [0, 0, 0],
            [1, 1, 0],
            [0, 1, 1]
        ], dtype=np.uint8),
        np.array([
            [0, 1, 0],
            [1, 1, 0],
            [1, 0, 0]
        ], dtype=np.uint8),
    ],
    'J': [
        np.array([
            [1, 0, 0],
            [1, 1, 1],
            [0, 0, 0]
        ], dtype=np.uint8),
        np.array([
            [0, 1, 1],
            [0, 1, 0],
            [0, 1, 0]
        ], dtype=np.uint8),
        np.array([
            [0, 0, 0],
            [1, 1, 1],
            [0, 0, 1]
        ], dtype=np.uint8),
        np.array([
            [0, 1, 0],
            [0, 1, 0],
            [1, 1, 0]
        ], dtype=np.uint8),
    ],
    'L': [
        np.array([
            [0, 0, 1],
            [1, 1, 1],
            [0, 0, 0]
        ], dtype=np.uint8),
        np.array([
            [0, 1, 0],
            [0, 1, 0],
            [0, 1, 1]
        ], dtype=np.uint8),
        np.array([
            [0, 0, 0],
            [1, 1, 1],
            [1, 0, 0]
        ], dtype=np.uint8),
        np.array([
            [1, 1, 0],
            [0, 1, 0],
            [0, 1, 0]
        ], dtype=np.uint8),
    ],
}

# Wall kick offsets in grains to test during rotation
WALL_KICK_TESTS = [
    (0, 0),
    (MINO_SIZE, 0),
    (-MINO_SIZE, 0),
    (2 * MINO_SIZE, 0),
    (-2 * MINO_SIZE, 0),
    (0, -MINO_SIZE),
    (MINO_SIZE, -MINO_SIZE),
    (-MINO_SIZE, -MINO_SIZE),
]


class Tetromino:
    """Represents an active falling Tetromino piece composed of sand minos."""

    def __init__(self, shape_name: str, color_idx: int):
        self.shape_name = shape_name
        self.color_idx = color_idx
        self.rotation = 0
        self.matrix = TETROMINO_SHAPES[shape_name][0]
        
        # Center horizontally at spawn
        matrix_w = self.matrix.shape[1] * MINO_SIZE
        self.x = (BOARD_WIDTH - matrix_w) // 2
        # Spawn near the top
        self.y = 0
        
        # Precomputed grain cache for current rotation
        self._grain_mask = None
        self._update_grain_mask()

    def _update_grain_mask(self):
        """Builds the 2D boolean mask of sand grains for this piece."""
        self.matrix = TETROMINO_SHAPES[self.shape_name][self.rotation]
        rows, cols = self.matrix.shape
        grain_h = rows * MINO_SIZE
        grain_w = cols * MINO_SIZE
        
        # Expand each mino cell into MINO_SIZE x MINO_SIZE grains
        self._grain_mask = np.kron(self.matrix, np.ones((MINO_SIZE, MINO_SIZE), dtype=bool))

    @property
    def grain_mask(self) -> np.ndarray:
        if self._grain_mask is None:
            self._update_grain_mask()
        return self._grain_mask

    def collides(self, sand_grid: np.ndarray, offset_x: int = 0, offset_y: int = 0, test_rotation: int = None) -> bool:
        """Checks if placing piece at (self.x + offset_x, self.y + offset_y) collides with boundaries or sand."""
        if test_rotation is not None:
            rot_matrix = TETROMINO_SHAPES[self.shape_name][test_rotation]
            mask = np.kron(rot_matrix, np.ones((MINO_SIZE, MINO_SIZE), dtype=bool))
        else:
            mask = self.grain_mask

        H, W = sand_grid.shape
        m_h, m_w = mask.shape
        tx = self.x + offset_x
        ty = self.y + offset_y

        # Quick boundary checks
        if tx < 0 or tx + m_w > W:
            # Check if any actual grain is outside the horizontal boundary
            grain_coords_y, grain_coords_x = np.where(mask)
            abs_x = tx + grain_coords_x
            if np.any(abs_x < 0) or np.any(abs_x >= W):
                return True

        if ty + m_h > H:
            grain_coords_y, _ = np.where(mask)
            abs_y = ty + grain_coords_y
            if np.any(abs_y >= H):
                return True

        # Check collision with existing sand grains
        grain_coords_y, grain_coords_x = np.where(mask)
        abs_y = ty + grain_coords_y
        abs_x = tx + grain_coords_x

        # Ignore grains that are above the top row (y < 0)
        valid = (abs_y >= 0) & (abs_y < H) & (abs_x >= 0) & (abs_x < W)
        if np.any(sand_grid[abs_y[valid], abs_x[valid]] > 0):
            return True

        return False

    def try_move(self, dx: int, dy: int, sand_grid: np.ndarray) -> bool:
        """Attempts to move by (dx, dy). Returns True on success."""
        if not self.collides(sand_grid, offset_x=dx, offset_y=dy):
            self.x += dx
            self.y += dy
            return True
        return False

    def try_rotate(self, direction: int, sand_grid: np.ndarray) -> bool:
        """
        Attempts to rotate CW (+1) or CCW (-1) using wall-kick tests.
        Returns True if rotation succeeded.
        """
        new_rot = (self.rotation + direction) % 4
        for kick_x, kick_y in WALL_KICK_TESTS:
            if not self.collides(sand_grid, offset_x=kick_x, offset_y=kick_y, test_rotation=new_rot):
                self.rotation = new_rot
                self.x += kick_x
                self.y += kick_y
                self._update_grain_mask()
                return True
        return False

    def get_ghost_y(self, sand_grid: np.ndarray) -> int:
        """Finds the lowest valid Y position where the piece can land."""
        ghost_y = self.y
        step = 1
        while not self.collides(sand_grid, offset_x=0, offset_y=(ghost_y - self.y + step)):
            ghost_y += step
        return ghost_y

    def get_occupied_grains(self) -> list:
        """Returns list of (y, x, color_idx) for all active grains."""
        gy, gx = np.where(self.grain_mask)
        return [(self.y + y, self.x + x, self.color_idx) for y, x in zip(gy, gx)]


class BagRandomizer:
    """Standard 7-Bag fair randomizer for Tetrominoes and balanced sand colors."""

    def __init__(self, num_colors: int = 4):
        self.num_colors = num_colors
        self.shapes_pool = list(TETROMINO_SHAPES.keys())
        self.current_bag = []
        self._refill_bag()

    def _refill_bag(self):
        self.current_bag = self.shapes_pool.copy()
        random.shuffle(self.current_bag)

    def next_piece(self) -> Tetromino:
        if not self.current_bag:
            self._refill_bag()
        shape = self.current_bag.pop()
        color_idx = random.randint(1, self.num_colors)
        return Tetromino(shape, color_idx)

    def peek_next(self, count: int = 3) -> list:
        """Looks ahead at upcoming pieces without consuming them."""
        preview = []
        temp_bag = self.current_bag.copy()
        while len(preview) < count:
            if not temp_bag:
                temp_bag = self.shapes_pool.copy()
                random.shuffle(temp_bag)
            shape = temp_bag.pop()
            # Assign deterministic preview color based on hash/random seed
            color_idx = (hash(shape + str(len(preview))) % self.num_colors) + 1
            preview.append(Tetromino(shape, color_idx))
        return preview
