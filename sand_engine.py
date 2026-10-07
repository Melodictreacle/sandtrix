"""Core Sandtrix physics, cellular automata simulation, line-clearing BFS, and game state."""

import json
import random
from collections import deque
from typing import List, Tuple, Set, Dict, Optional
import numpy as np

from config import (
    BOARD_WIDTH, BOARD_HEIGHT, DANGER_ROW,
    POINTS_PER_GRAIN, POINTS_PER_BAND, COMBO_MULTIPLIER_BASE,
    GRAVITY_INTERVALS, SOFT_DROP_GRAVITY_MS, LOCK_DELAY_MS,
    PHYSICS_SUBSTEPS, HIGHSCORE_FILE
)
from tetromino import Tetromino, BagRandomizer


class SandEngine:
    """Simulates falling sand dynamics, active tetromino, and edge-to-edge line clearing."""

    def __init__(self, num_colors: int = 4):
        self.num_colors = num_colors
        self.width = BOARD_WIDTH
        self.height = BOARD_HEIGHT
        
        # Grid: 0 = empty air, 1..N = sand color
        self.grid = np.zeros((self.height, self.width), dtype=np.uint8)
        
        # Game State
        self.score = 0
        self.high_score = self._load_high_score()
        self.level = 1
        self.lines_cleared = 0
        self.combo = 0
        self.game_over = False
        self.paused = False
        
        # Pieces
        self.randomizer = BagRandomizer(num_colors=self.num_colors)
        self.active_piece: Optional[Tetromino] = None
        self.hold_piece: Optional[Tetromino] = None
        self.can_hold = True
        self.next_queue: List[Tetromino] = []
        
        # Timers (ms)
        self.gravity_timer = 0
        self.lock_timer = 0
        self.is_soft_dropping = False
        
        # Settling state
        self.is_sand_moving = False
        self.last_drop_had_clear = False
        
        # Cleared grains flash / dissolve animation buffer: { (y, x): (color, frames_left) }
        self.dissolving_grains: Dict[Tuple[int, int], Tuple[int, int]] = {}
        
        # Initialize queue and spawn first piece
        self._refill_next_queue()
        self.spawn_piece()

    def set_palette_colors_count(self, num_colors: int):
        """Updates color count when changing palettes and restarts game."""
        self.num_colors = num_colors
        self.reset()

    def _load_high_score(self) -> int:
        try:
            with open(HIGHSCORE_FILE, "r") as f:
                data = json.load(f)
                return int(data.get("high_score", 0))
        except Exception:
            return 0

    def _save_high_score(self):
        try:
            with open(HIGHSCORE_FILE, "w") as f:
                json.dump({"high_score": self.high_score}, f)
        except Exception:
            pass

    def reset(self):
        """Resets the game state for a new round."""
        self.grid.fill(0)
        self.score = 0
        self.level = 1
        self.lines_cleared = 0
        self.combo = 0
        self.game_over = False
        self.paused = False
        self.hold_piece = None
        self.can_hold = True
        self.gravity_timer = 0
        self.lock_timer = 0
        self.is_soft_dropping = False
        self.is_sand_moving = False
        self.last_drop_had_clear = False
        self.dissolving_grains.clear()
        
        self.randomizer = BagRandomizer(num_colors=self.num_colors)
        self.next_queue.clear()
        self._refill_next_queue()
        self.spawn_piece()

    def _refill_next_queue(self):
        while len(self.next_queue) < 4:
            self.next_queue.append(self.randomizer.next_piece())

    def spawn_piece(self) -> bool:
        """Spawns the next piece from the queue. Returns False if blocked (Game Over)."""
        self._refill_next_queue()
        self.active_piece = self.next_queue.pop(0)
        self.lock_timer = 0
        self.gravity_timer = 0
        
        # Check if spawned piece collides immediately with sand
        if self.active_piece.collides(self.grid):
            self.game_over = True
            if self.score > self.high_score:
                self.high_score = self.score
                self._save_high_score()
            return False
        return True

    def hold_current_piece(self) -> bool:
        """Swaps active piece with hold piece. Returns True if swap succeeded."""
        if not self.can_hold or self.active_piece is None or self.game_over or self.paused:
            return False

        current_shape = self.active_piece.shape_name
        current_color = self.active_piece.color_idx

        if self.hold_piece is None:
            self.hold_piece = Tetromino(current_shape, current_color)
            self.spawn_piece()
        else:
            held_shape = self.hold_piece.shape_name
            held_color = self.hold_piece.color_idx
            self.hold_piece = Tetromino(current_shape, current_color)
            self.active_piece = Tetromino(held_shape, held_color)

        self.can_hold = False
        self.lock_timer = 0
        self.gravity_timer = 0
        return True

    def move_piece(self, dx: int) -> bool:
        """Moves active piece horizontally by dx grains (e.g. +/- MINO_SIZE or 1 grain)."""
        if self.active_piece and not self.game_over and not self.paused:
            return self.active_piece.try_move(dx, 0, self.grid)
        return False

    def rotate_piece(self, direction: int) -> bool:
        """Rotates active piece CW (+1) or CCW (-1)."""
        if self.active_piece and not self.game_over and not self.paused:
            return self.active_piece.try_rotate(direction, self.grid)
        return False

    def hard_drop(self) -> Tuple[int, int, List[Tuple[float, float]]]:
        """
        Instantly drops piece to lowest position and locks it into sand.
        Returns (land_x, land_y, list of grain pixel coordinates for impact particles).
        """
        if not self.active_piece or self.game_over or self.paused:
            return 0, 0, []

        ghost_y = self.active_piece.get_ghost_y(self.grid)
        self.active_piece.y = ghost_y
        land_x = self.active_piece.x
        land_y = ghost_y

        impact_coords = []
        for gy, gx, _ in self.active_piece.get_occupied_grains():
            impact_coords.append((float(gx), float(gy)))

        self.lock_active_piece()
        return land_x, land_y, impact_coords

    def lock_active_piece(self):
        """Converts the active rigid piece into loose sand grains in the grid."""
        if not self.active_piece:
            return

        for gy, gx, col in self.active_piece.get_occupied_grains():
            if 0 <= gy < self.height and 0 <= gx < self.width:
                self.grid[gy, gx] = col

        self.active_piece = None
        self.can_hold = True
        self.lock_timer = 0
        self.is_sand_moving = True

        # Check if sand breached danger line at top
        if np.any(self.grid[:DANGER_ROW, :] > 0):
            self.game_over = True
            if self.score > self.high_score:
                self.high_score = self.score
                self._save_high_score()
            return

        self.spawn_piece()

    def update_physics(self) -> bool:
        """
        Executes one sub-step of falling sand cellular automata.
        Returns True if any sand grain moved.
        """
        moved = False
        H = self.height
        W = self.width
        grid = self.grid

        # Process from bottom-up
        for y in range(H - 2, -1, -1):
            row = grid[y]
            sand_xs = np.where(row > 0)[0]
            if len(sand_xs) == 0:
                continue

            # Randomize order to remove directional bias
            np.random.shuffle(sand_xs)

            for x in sand_xs:
                color = grid[y, x]
                # 1. Check straight down
                if grid[y + 1, x] == 0:
                    grid[y + 1, x] = color
                    grid[y, x] = 0
                    moved = True
                else:
                    # 2. Check diagonals
                    left_ok = (x > 0 and grid[y + 1, x - 1] == 0)
                    right_ok = (x < W - 1 and grid[y + 1, x + 1] == 0)

                    if left_ok and right_ok:
                        nx = x - 1 if random.random() < 0.5 else x + 1
                        grid[y + 1, nx] = color
                        grid[y, x] = 0
                        moved = True
                    elif left_ok:
                        grid[y + 1, x - 1] = color
                        grid[y, x] = 0
                        moved = True
                    elif right_ok:
                        grid[y + 1, x + 1] = color
                        grid[y, x] = 0
                        moved = True

        return moved

    def check_line_clears(self) -> Tuple[int, int, List[Tuple[int, int]], int]:
        """
        Detects continuous edge-to-edge paths of the same color from x=0 to x=W-1.
        Returns:
            (grains_count, bands_count, list_of_cleared_coords, cleared_color)
        """
        H = self.height
        W = self.width
        grid = self.grid
        neighbors = [(-1, -1), (-1, 0), (-1, 1), (0, -1), (0, 1), (1, -1), (1, 0), (1, 1)]

        total_cleared_coords = []
        bands_count = 0
        last_color = 0

        # Check each color currently on board
        unique_colors = np.unique(grid)
        for color in unique_colors:
            if color == 0:
                continue

            # Find all grains of this color touching the left wall
            start_rows = np.where(grid[:, 0] == color)[0]
            if len(start_rows) == 0:
                continue

            visited = np.zeros((H, W), dtype=bool)

            for sy in start_rows:
                if visited[sy, 0]:
                    continue

                # 8-connectivity BFS
                queue = deque([(sy, 0)])
                visited[sy, 0] = True
                component = [(sy, 0)]
                touches_right_wall = False

                while queue:
                    cy, cx = queue.popleft()
                    if cx == W - 1:
                        touches_right_wall = True

                    for dy, dx in neighbors:
                        ny, nx = cy + dy, cx + dx
                        if 0 <= ny < H and 0 <= nx < W:
                            if not visited[ny, nx] and grid[ny, nx] == color:
                                visited[ny, nx] = True
                                queue.append((ny, nx))
                                component.append((ny, nx))

                if touches_right_wall:
                    bands_count += 1
                    last_color = color
                    for cy, cx in component:
                        total_cleared_coords.append((cy, cx))

        if total_cleared_coords:
            # Remove duplicates if any
            total_cleared_coords = list(set(total_cleared_coords))
            # Clear grains from grid
            for cy, cx in total_cleared_coords:
                grid[cy, cx] = 0

            return len(total_cleared_coords), bands_count, total_cleared_coords, last_color

        return 0, 0, [], 0

    def tick(self, dt_ms: int) -> Dict:
        """
        Main game update tick called every frame.
        Returns a dict of events triggered during this frame:
            {
                'cleared': bool,
                'grains': int,
                'bands': int,
                'coords': list,
                'color': int,
                'combo': int,
                'points': int,
                'landed': bool
            }
        """
        events = {
            'cleared': False,
            'grains': 0,
            'bands': 0,
            'coords': [],
            'color': 0,
            'combo': 0,
            'points': 0,
            'landed': False
        }

        if self.game_over or self.paused:
            return events

        # 1. Simulate sand physics sub-steps
        any_sand_moved = False
        for _ in range(PHYSICS_SUBSTEPS):
            if self.update_physics():
                any_sand_moved = True

        self.is_sand_moving = any_sand_moved

        # 2. Check for line clears
        grains_count, bands_count, cleared_coords, cleared_color = self.check_line_clears()
        if bands_count > 0:
            # Award points with combo multiplier
            self.combo += 1
            combo_mult = COMBO_MULTIPLIER_BASE ** (self.combo - 1)
            points = int((grains_count * POINTS_PER_GRAIN + bands_count * POINTS_PER_BAND) * combo_mult * self.level)
            self.score += points
            if self.score > self.high_score:
                self.high_score = self.score
                self._save_high_score()

            self.lines_cleared += bands_count
            self.level = 1 + (self.lines_cleared // 5)
            self.last_drop_had_clear = True

            events.update({
                'cleared': True,
                'grains': grains_count,
                'bands': bands_count,
                'coords': cleared_coords,
                'color': cleared_color,
                'combo': self.combo,
                'points': points
            })
        elif not any_sand_moved and not self.is_sand_moving:
            # When all sand has completely settled, reset combo if no clear occurred
            if not self.last_drop_had_clear:
                self.combo = 0
            self.last_drop_had_clear = False

        # 3. Active Tetromino Gravity & Lock
        if self.active_piece:
            # Determine gravity speed
            current_interval = SOFT_DROP_GRAVITY_MS if self.is_soft_dropping else GRAVITY_INTERVALS.get(min(10, self.level), 8)
            self.gravity_timer += dt_ms

            if self.gravity_timer >= current_interval:
                self.gravity_timer = 0
                # Try moving down by 1 grain
                if self.active_piece.try_move(0, 1, self.grid):
                    self.lock_timer = 0
                else:
                    # Piece is resting on sand or floor
                    self.lock_timer += current_interval
                    if self.lock_timer >= LOCK_DELAY_MS or self.is_soft_dropping:
                        self.lock_active_piece()
                        events['landed'] = True

        return events
