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
    PHYSICS_SUBSTEPS, HIGHSCORE_FILE,
    MATERIAL_WATER, MATERIAL_ACID, MATERIAL_BOMB, MATERIAL_ICE, MATERIAL_FIRE,
    MATERIAL_STEAM, MATERIAL_TOXIC_GAS, MATERIAL_ASH
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
        
        # Elemental reaction tracking
        self.last_frozen_coords: List[Tuple[int, int]] = []
        self.last_melted_coords: List[Tuple[int, int]] = []
        self.last_toxic_coords: List[Tuple[int, int]] = []
        self.last_burned_coords: List[Tuple[int, int]] = []
        self.last_steam_coords: List[Tuple[int, int]] = []
        self.last_extinguished_coords: List[Tuple[int, int]] = []
        
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
        self.last_frozen_coords.clear()
        self.last_melted_coords.clear()
        self.last_toxic_coords.clear()
        self.last_burned_coords.clear()
        self.last_steam_coords.clear()
        self.last_extinguished_coords.clear()
        
        self.randomizer = BagRandomizer(num_colors=self.num_colors, enable_elements=True)
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
            if self.active_piece.try_move(dx, 0, self.grid):
                self.lock_timer = 0
                return True
            # Slide flush against obstacle if full step blocked
            direction = 1 if dx > 0 else -1
            for step in range(abs(dx) - 1, 0, -1):
                if self.active_piece.try_move(step * direction, 0, self.grid):
                    self.lock_timer = 0
                    return True
            # Step up over minor 1-2 grain bumps on sand floor
            for dy in (-1, -2):
                if self.active_piece.try_move(dx, dy, self.grid):
                    self.lock_timer = 0
                    return True
        return False

    def rotate_piece(self, direction: int) -> bool:
        """Rotates active piece CW (+1) or CCW (-1)."""
        if self.active_piece and not self.game_over and not self.paused:
            if self.active_piece.try_rotate(direction, self.grid):
                self.lock_timer = 0
                return True
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

        # When ice block moves downwards, existing water in contact converts into ice
        if self.active_piece.is_ice():
            for gy, gx, _ in self.active_piece.get_occupied_grains():
                for dy in (0, 1, 2):
                    for dx in (-1, 0, 1):
                        ny, nx = gy + dy, gx + dx
                        if 0 <= ny < self.height and 0 <= nx < self.width:
                            if self.grid[ny, nx] == MATERIAL_WATER:
                                self.grid[ny, nx] = MATERIAL_ICE
                                self.last_frozen_coords.append((ny, nx))

        impact_coords = []
        for gy, gx, _ in self.active_piece.get_occupied_grains():
            impact_coords.append((float(gx), float(gy)))

        self.lock_active_piece()
        return land_x, land_y, impact_coords

    def lock_active_piece(self):
        """Converts the active rigid piece into loose sand grains in the grid."""
        if not self.active_piece:
            return

        is_ice = self.active_piece.is_ice()
        if is_ice:
            for gy, gx, _ in self.active_piece.get_occupied_grains():
                for dy in (0, 1, 2):
                    for dx in (-1, 0, 1):
                        ny, nx = gy + dy, gx + dx
                        if 0 <= ny < self.height and 0 <= nx < self.width:
                            if self.grid[ny, nx] == MATERIAL_WATER:
                                self.grid[ny, nx] = MATERIAL_ICE
                                self.last_frozen_coords.append((ny, nx))

        is_fire = self.active_piece.is_fire()
        hit_water = False
        hit_ice = False
        hit_acid = False
        hit_sand = False
        contact_sand_coords = []
        contact_acid_coords = []

        if is_fire:
            # Check what materials the fire piece landed on or touches
            for gy, gx, _ in self.active_piece.get_occupied_grains():
                for dy, dx in [(1, 0), (0, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (1, -1)]:
                    ny, nx = gy + dy, gx + dx
                    if 0 <= ny < self.height and 0 <= nx < self.width:
                        val = self.grid[ny, nx]
                        if val == MATERIAL_WATER:
                            hit_water = True
                        elif val == MATERIAL_ICE:
                            hit_ice = True
                        elif val == MATERIAL_ACID:
                            hit_acid = True
                            contact_acid_coords.append((ny, nx))
                        elif 1 <= val <= 10 or val == MATERIAL_ASH:
                            hit_sand = True
                            contact_sand_coords.append((ny, nx))

        # 0. Fire on Water: When fire falls down upon water, fire is totally destroyed!
        if is_fire and hit_water:
            for gy, gx, _ in self.active_piece.get_occupied_grains():
                if 0 <= gy < self.height and 0 <= gx < self.width:
                    self.last_extinguished_coords.append((gy, gx))
            # Fire is completely erased/extinguished: none of its grains are placed on the grid

        # 1. Fire on Ice: Converts ice to water AND fire to steam (steam disappears after few seconds)
        elif is_fire and hit_ice:
            ice_mask = (self.grid == MATERIAL_ICE)
            if np.any(ice_mask):
                ice_indices = np.argwhere(ice_mask)
                self.grid[ice_mask] = MATERIAL_WATER
                self.last_melted_coords.extend([(int(y), int(x)) for y, x in ice_indices])
            
            # Fire that fell on ice converts to buoyant steam grains in the air
            for gy, gx, _ in self.active_piece.get_occupied_grains():
                if 0 <= gy < self.height and 0 <= gx < self.width:
                    self.grid[gy, gx] = MATERIAL_STEAM
                    self.last_steam_coords.append((gy, gx))

        # 2. Fire on Acid: Creates green toxic cloud (disappears after few seconds)
        elif is_fire and hit_acid:
            for ay, ax in set(contact_acid_coords):
                self.grid[ay, ax] = MATERIAL_TOXIC_GAS
                self.last_toxic_coords.append((ay, ax))
            for gy, gx, _ in self.active_piece.get_occupied_grains():
                if 0 <= gy < self.height and 0 <= gx < self.width:
                    self.grid[gy, gx] = MATERIAL_TOXIC_GAS
                    self.last_toxic_coords.append((gy, gx))

        # 3. Fire on Sand: A portion of sand burns to ash; fire piece burns entirely into ash (no red pixels remain!)
        elif is_fire and hit_sand:
            # Portion of touching sand grains burns into ash
            for sy, sx in set(contact_sand_coords):
                if random.random() < 0.60:
                    self.grid[sy, sx] = MATERIAL_ASH
                    self.last_burned_coords.append((sy, sx))
            # Fire piece settles entirely as ash residue (100% ash, 0% red fire)
            for gy, gx, _ in self.active_piece.get_occupied_grains():
                if 0 <= gy < self.height and 0 <= gx < self.width:
                    self.grid[gy, gx] = MATERIAL_ASH
                    self.last_burned_coords.append((gy, gx))
        else:
            for gy, gx, col in self.active_piece.get_occupied_grains():
                if 0 <= gy < self.height and 0 <= gx < self.width:
                    cur = self.grid[gy, gx]
                    if cur in (0, MATERIAL_STEAM, MATERIAL_TOXIC_GAS):
                        self.grid[gy, gx] = col

        self.active_piece = None
        self.can_hold = True
        self.lock_timer = 0
        self.is_soft_dropping = False
        self.is_sand_moving = True

        # Check if sand breached danger line at top (ignoring rising atmospheric gases)
        danger_zone = self.grid[:DANGER_ROW, :]
        solid_in_danger = (danger_zone > 0) & (danger_zone != MATERIAL_STEAM) & (danger_zone != MATERIAL_TOXIC_GAS)
        if np.any(solid_in_danger):
            self.game_over = True
            if self.score > self.high_score:
                self.high_score = self.score
                self._save_high_score()
            return

        self.spawn_piece()

    def update_physics(self) -> bool:
        """
        Executes one sub-step of cellular automata (sand, water fluid, ice, fire, steam, toxic gas, ash).
        Returns True if any grain moved or reacted.
        """
        moved = False
        H = self.height
        W = self.width
        grid = self.grid

        # ================= PASS 1: RISING GASES (STEAM & TOXIC CLOUD) =================
        # Process top-down so rising gas grains advance at most 1 grain per sub-step
        for y in range(0, H):
            row = grid[y]
            gas_xs = np.where((row == MATERIAL_STEAM) | (row == MATERIAL_TOXIC_GAS))[0]
            if len(gas_xs) == 0:
                continue

            np.random.shuffle(gas_xs)

            for x in gas_xs:
                gmat = grid[y, x]
                if gmat not in (MATERIAL_STEAM, MATERIAL_TOXIC_GAS):
                    continue

                # Spontaneous dissipation over a few seconds (~2-3s half-life)
                dissipate_rate = 0.010 if gmat == MATERIAL_STEAM else 0.012
                if random.random() < dissipate_rate:
                    grid[y, x] = 0
                    moved = True
                    continue

                # Dissipate into the open sky if reached ceiling
                if y == 0:
                    grid[y, x] = 0
                    moved = True
                    continue

                # Buoyant upward rise
                if grid[y - 1, x] == 0:
                    grid[y - 1, x] = gmat
                    grid[y, x] = 0
                    moved = True
                else:
                    # Up-diagonals
                    ul_empty = (x > 0 and grid[y - 1, x - 1] == 0)
                    ur_empty = (x < W - 1 and grid[y - 1, x + 1] == 0)
                    if ul_empty and ur_empty:
                        nx = x - 1 if random.random() < 0.5 else x + 1
                        grid[y - 1, nx] = gmat
                        grid[y, x] = 0
                        moved = True
                    elif ul_empty:
                        grid[y - 1, x - 1] = gmat
                        grid[y, x] = 0
                        moved = True
                    elif ur_empty:
                        grid[y - 1, x + 1] = gmat
                        grid[y, x] = 0
                        moved = True
                    else:
                        # Lateral drift if ceiling blocked
                        l_empty = (x > 0 and grid[y, x - 1] == 0)
                        r_empty = (x < W - 1 and grid[y, x + 1] == 0)
                        if (l_empty or r_empty) and random.random() < 0.25:
                            nx = (x - 1) if (l_empty and (not r_empty or random.random() < 0.5)) else (x + 1)
                            grid[y, nx] = gmat
                            grid[y, x] = 0
                            moved = True

        # ================= PASS 2: FALLING / SETTLING MATTER =================
        # Process bottom-up for gravity
        for y in range(H - 2, -1, -1):
            row = grid[y]
            sand_xs = np.where(row > 0)[0]
            if len(sand_xs) == 0:
                continue

            # Randomize order to remove directional bias
            np.random.shuffle(sand_xs)

            for x in sand_xs:
                mat = grid[y, x]
                if mat in (0, MATERIAL_STEAM, MATERIAL_TOXIC_GAS):
                    continue

                # ================= 1. WATER (FLUID DYNAMICS) =================
                if mat == MATERIAL_WATER:
                    # A. Straight Down
                    if grid[y + 1, x] == 0:
                        grid[y + 1, x] = MATERIAL_WATER
                        grid[y, x] = 0
                        moved = True
                    else:
                        # B. Down Diagonals
                        dl_empty = (x > 0 and grid[y + 1, x - 1] == 0)
                        dr_empty = (x < W - 1 and grid[y + 1, x + 1] == 0)
                        if dl_empty and dr_empty:
                            nx = x - 1 if random.random() < 0.5 else x + 1
                            grid[y + 1, nx] = MATERIAL_WATER
                            grid[y, x] = 0
                            moved = True
                        elif dl_empty:
                            grid[y + 1, x - 1] = MATERIAL_WATER
                            grid[y, x] = 0
                            moved = True
                        elif dr_empty:
                            grid[y + 1, x + 1] = MATERIAL_WATER
                            grid[y, x] = 0
                            moved = True
                        else:
                            # C. Lateral Leveling Flow
                            left_empty = (x > 0 and grid[y, x - 1] == 0)
                            right_empty = (x < W - 1 and grid[y, x + 1] == 0)
                            if left_empty and right_empty:
                                nx = x - 1 if random.random() < 0.5 else x + 1
                                grid[y, nx] = MATERIAL_WATER
                                grid[y, x] = 0
                                moved = True
                            elif left_empty:
                                grid[y, x - 1] = MATERIAL_WATER
                                grid[y, x] = 0
                                moved = True
                            elif right_empty:
                                grid[y, x + 1] = MATERIAL_WATER
                                grid[y, x] = 0
                                moved = True

                # ================= 2. ICE DYNAMICS =================
                elif mat == MATERIAL_ICE:
                    below = grid[y + 1, x]
                    # Ice touching water: freeze the water!
                    if below == MATERIAL_WATER:
                        grid[y + 1, x] = MATERIAL_ICE
                        self.last_frozen_coords.append((y + 1, x))
                        moved = True
                    elif below == 0:
                        grid[y + 1, x] = MATERIAL_ICE
                        grid[y, x] = 0
                        moved = True
                    else:
                        dl_free = (x > 0 and (grid[y + 1, x - 1] == 0 or grid[y + 1, x - 1] == MATERIAL_WATER))
                        dr_free = (x < W - 1 and (grid[y + 1, x + 1] == 0 or grid[y + 1, x + 1] == MATERIAL_WATER))
                        if (dl_free or dr_free) and random.random() < 0.12:
                            nx = (x - 1) if (dl_free and (not dr_free or random.random() < 0.5)) else (x + 1)
                            target_val = grid[y + 1, nx]
                            if target_val == MATERIAL_WATER:
                                grid[y + 1, nx] = MATERIAL_ICE
                                self.last_frozen_coords.append((y + 1, nx))
                            else:
                                grid[y + 1, nx] = MATERIAL_ICE
                                grid[y, x] = 0
                            moved = True

                # ================= 3. ACID DYNAMICS =================
                elif mat == MATERIAL_ACID:
                    below = grid[y + 1, x]
                    if below == 0:
                        grid[y + 1, x] = MATERIAL_ACID
                        grid[y, x] = 0
                        moved = True
                    elif below not in (MATERIAL_ACID, MATERIAL_WATER, MATERIAL_ICE, MATERIAL_FIRE, MATERIAL_TOXIC_GAS):
                        # Acid dissolves sand
                        grid[y + 1, x] = 0
                        grid[y, x] = 0
                        moved = True
                    else:
                        dl_empty = (x > 0 and grid[y + 1, x - 1] == 0)
                        dr_empty = (x < W - 1 and grid[y + 1, x + 1] == 0)
                        if (dl_empty or dr_empty) and random.random() < 0.25:
                            nx = (x - 1) if (dl_empty and (not dr_empty or random.random() < 0.5)) else (x + 1)
                            grid[y + 1, nx] = MATERIAL_ACID
                            grid[y, x] = 0
                            moved = True

                # ================= 4. FIRE DYNAMICS =================
                elif mat == MATERIAL_FIRE:
                    below = grid[y + 1, x]
                    # A. Fire touching Water: fire is totally destroyed!
                    if below == MATERIAL_WATER:
                        grid[y, x] = 0
                        self.last_extinguished_coords.append((y, x))
                        moved = True
                    # B. Fire touching Ice: melts ice into water AND fire turns to steam!
                    elif below == MATERIAL_ICE:
                        grid[y + 1, x] = MATERIAL_WATER
                        grid[y, x] = MATERIAL_STEAM
                        self.last_melted_coords.append((y + 1, x))
                        self.last_steam_coords.append((y, x))
                        moved = True
                    # C. Fire touching Acid: creates green toxic cloud!
                    elif below == MATERIAL_ACID:
                        grid[y + 1, x] = MATERIAL_TOXIC_GAS
                        grid[y, x] = MATERIAL_TOXIC_GAS
                        self.last_toxic_coords.extend([(y + 1, x), (y, x)])
                        moved = True
                    # D. Fire touching Sand or Ash: burns sand into ash, fire itself converts to ash! (No red pixels!)
                    elif (1 <= below <= 10) or below == MATERIAL_ASH:
                        if 1 <= below <= 10:
                            grid[y + 1, x] = MATERIAL_ASH
                            self.last_burned_coords.append((y + 1, x))
                        grid[y, x] = MATERIAL_ASH
                        self.last_burned_coords.append((y, x))
                        moved = True
                    # E. Fire falling into empty space
                    elif below == 0:
                        grid[y + 1, x] = MATERIAL_FIRE
                        grid[y, x] = 0
                        moved = True
                    else:
                        # Check diagonal contact reactions
                        dl_water = (x > 0 and grid[y + 1, x - 1] == MATERIAL_WATER)
                        l_water = (x > 0 and grid[y, x - 1] == MATERIAL_WATER)
                        r_water = (x < W - 1 and grid[y, x + 1] == MATERIAL_WATER)
                        dl_ice = (x > 0 and grid[y + 1, x - 1] == MATERIAL_ICE)
                        dr_ice = (x < W - 1 and grid[y + 1, x + 1] == MATERIAL_ICE)
                        dl_acid = (x > 0 and grid[y + 1, x - 1] == MATERIAL_ACID)
                        dr_acid = (x < W - 1 and grid[y + 1, x + 1] == MATERIAL_ACID)
                        dl_sand = (x > 0 and ((1 <= grid[y + 1, x - 1] <= 10) or grid[y + 1, x - 1] == MATERIAL_ASH))
                        dr_sand = (x < W - 1 and ((1 <= grid[y + 1, x + 1] <= 10) or grid[y + 1, x + 1] == MATERIAL_ASH))
                        
                        if dl_water or dr_water or l_water or r_water:
                            grid[y, x] = 0
                            self.last_extinguished_coords.append((y, x))
                            moved = True
                        elif dl_ice:
                            grid[y + 1, x - 1] = MATERIAL_WATER
                            grid[y, x] = MATERIAL_STEAM
                            self.last_melted_coords.append((y + 1, x - 1))
                            self.last_steam_coords.append((y, x))
                            moved = True
                        elif dr_ice:
                            grid[y + 1, x + 1] = MATERIAL_WATER
                            grid[y, x] = MATERIAL_STEAM
                            self.last_melted_coords.append((y + 1, x + 1))
                            self.last_steam_coords.append((y, x))
                            moved = True
                        elif dl_acid:
                            grid[y + 1, x - 1] = MATERIAL_TOXIC_GAS
                            grid[y, x] = MATERIAL_TOXIC_GAS
                            self.last_toxic_coords.extend([(y + 1, x - 1), (y, x)])
                            moved = True
                        elif dr_acid:
                            grid[y + 1, x + 1] = MATERIAL_TOXIC_GAS
                            grid[y, x] = MATERIAL_TOXIC_GAS
                            self.last_toxic_coords.extend([(y + 1, x + 1), (y, x)])
                            moved = True
                        elif dl_sand or dr_sand:
                            # Fire touching sand or ash diagonally converts to ash immediately
                            grid[y, x] = MATERIAL_ASH
                            self.last_burned_coords.append((y, x))
                            moved = True
                        else:
                            dl_empty = (x > 0 and grid[y + 1, x - 1] == 0)
                            dr_empty = (x < W - 1 and grid[y + 1, x + 1] == 0)
                            if (dl_empty or dr_empty) and random.random() < 0.12:
                                nx = (x - 1) if (dl_empty and (not dr_empty or random.random() < 0.5)) else (x + 1)
                                grid[y + 1, nx] = MATERIAL_FIRE
                                grid[y, x] = 0
                                moved = True

                # ================= 5. ASH DYNAMICS (BURNT SAND POWDER) =================
                elif mat == MATERIAL_ASH:
                    below = grid[y + 1, x]
                    if below == 0:
                        grid[y + 1, x] = MATERIAL_ASH
                        grid[y, x] = 0
                        moved = True
                    elif below == MATERIAL_WATER:
                        # Ash sinks in water
                        grid[y + 1, x] = MATERIAL_ASH
                        grid[y, x] = MATERIAL_WATER
                        moved = True
                    else:
                        dl_free = (x > 0 and (grid[y + 1, x - 1] == 0 or grid[y + 1, x - 1] == MATERIAL_WATER))
                        dr_free = (x < W - 1 and (grid[y + 1, x + 1] == 0 or grid[y + 1, x + 1] == MATERIAL_WATER))
                        if (dl_free or dr_free) and random.random() < 0.10:
                            nx = (x - 1) if (dl_free and (not dr_free or random.random() < 0.5)) else (x + 1)
                            target_val = grid[y + 1, nx]
                            grid[y + 1, nx] = MATERIAL_ASH
                            grid[y, x] = MATERIAL_WATER if target_val == MATERIAL_WATER else 0
                            moved = True

                # ================= 6. NORMAL SAND DYNAMICS (BUOYANCY) =================
                else:
                    below = grid[y + 1, x]
                    if below == 0:
                        grid[y + 1, x] = mat
                        grid[y, x] = 0
                        moved = True
                    elif below == MATERIAL_WATER:
                        # Sand sinks down, liquid water floats up!
                        grid[y + 1, x] = mat
                        grid[y, x] = MATERIAL_WATER
                        moved = True
                    else:
                        dl_ok = (x > 0 and (grid[y + 1, x - 1] == 0 or grid[y + 1, x - 1] == MATERIAL_WATER))
                        dr_ok = (x < W - 1 and (grid[y + 1, x + 1] == 0 or grid[y + 1, x + 1] == MATERIAL_WATER))
                        if (dl_ok or dr_ok) and random.random() < 0.08:
                            nx = (x - 1) if (dl_ok and (not dr_ok or random.random() < 0.5)) else (x + 1)
                            target_val = grid[y + 1, nx]
                            grid[y + 1, nx] = mat
                            grid[y, x] = MATERIAL_WATER if target_val == MATERIAL_WATER else 0
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
        neighbors = [(-1, 0), (0, -1), (0, 1), (1, 0)]

        total_cleared_coords = []
        bands_count = 0
        last_color = 0

        # Check each color currently on board
        unique_colors = np.unique(grid)
        for color in unique_colors:
            if color in (0, MATERIAL_STEAM, MATERIAL_TOXIC_GAS):
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
                'landed': bool,
                'water_frozen': bool,
                'frozen_coords': list,
                'ice_melted': bool,
                'melted_coords': list,
                'is_water_clear': bool,
                'is_ice_clear': bool,
                'is_fire_clear': bool
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
            'landed': False,
            'water_frozen': False,
            'frozen_coords': [],
            'ice_melted': False,
            'melted_coords': [],
            'steam_created': False,
            'steam_coords': [],
            'toxic_cloud_created': False,
            'toxic_coords': [],
            'sand_burned': False,
            'burned_coords': [],
            'fire_extinguished': False,
            'extinguished_coords': [],
            'is_water_clear': False,
            'is_ice_clear': False,
            'is_fire_clear': False,
            'is_ash_clear': False
        }

        if self.game_over or self.paused:
            return events

        # 1. Simulate sand/fluid physics sub-steps
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
            bonus_per_band = POINTS_PER_BAND * 2 if cleared_color in (MATERIAL_WATER, MATERIAL_ICE, MATERIAL_FIRE, MATERIAL_ASH) else POINTS_PER_BAND
            points = int((grains_count * POINTS_PER_GRAIN + bands_count * bonus_per_band) * combo_mult * self.level)
            self.score += points
            if self.score > self.high_score:
                self.high_score = self.score
                self._save_high_score()

            self.lines_cleared += bands_count
            self.level = 1 + (self.lines_cleared // 3)
            self.last_drop_had_clear = True

            events.update({
                'cleared': True,
                'grains': grains_count,
                'bands': bands_count,
                'coords': cleared_coords,
                'color': cleared_color,
                'combo': self.combo,
                'points': points,
                'is_water_clear': (cleared_color == MATERIAL_WATER),
                'is_ice_clear': (cleared_color == MATERIAL_ICE),
                'is_fire_clear': (cleared_color == MATERIAL_FIRE),
                'is_ash_clear': (cleared_color == MATERIAL_ASH)
            })
        elif not any_sand_moved and not self.is_sand_moving:
            # When all sand has completely settled, reset combo if no clear occurred
            if not self.last_drop_had_clear:
                self.combo = 0
            self.last_drop_had_clear = False

        # 3. Active Tetromino Gravity & Lock
        if self.active_piece:
            current_interval = SOFT_DROP_GRAVITY_MS if self.is_soft_dropping else GRAVITY_INTERVALS.get(min(10, self.level), 8)
            self.gravity_timer += dt_ms
            max_steps = 12
            steps = 0

            while self.gravity_timer >= current_interval and steps < max_steps:
                self.gravity_timer -= current_interval
                steps += 1
                if self.active_piece.try_move(0, 1, self.grid):
                    self.lock_timer = 0
                    # When ice block moves downwards, existing water in contact converts into ice!
                    if self.active_piece.is_ice():
                        for gy, gx, _ in self.active_piece.get_occupied_grains():
                            for dy in (0, 1, 2):
                                for dx in (-1, 0, 1):
                                    ny, nx = gy + dy, gx + dx
                                    if 0 <= ny < self.height and 0 <= nx < self.width:
                                        if self.grid[ny, nx] == MATERIAL_WATER:
                                            self.grid[ny, nx] = MATERIAL_ICE
                                            self.last_frozen_coords.append((ny, nx))
                else:
                    self.lock_timer += current_interval
                    # If fire piece hits water directly below, extinguish instantly
                    fire_hit_water = False
                    if self.active_piece.is_fire():
                        for gy, gx, _ in self.active_piece.get_occupied_grains():
                            ny, nx = gy + 1, gx
                            if 0 <= ny < self.height and 0 <= nx < self.width:
                                if self.grid[ny, nx] == MATERIAL_WATER:
                                    fire_hit_water = True
                                    break
                    if self.lock_timer >= LOCK_DELAY_MS or self.is_soft_dropping or fire_hit_water:
                        self.lock_active_piece()
                        events['landed'] = True
                    break

            if self.gravity_timer > current_interval * 3:
                self.gravity_timer = 0

        # Collect any frozen / melted / steam / toxic / burned events
        if self.last_frozen_coords:
            events['water_frozen'] = True
            events['frozen_coords'] = list(set(self.last_frozen_coords))
            self.last_frozen_coords.clear()

        if self.last_melted_coords:
            events['ice_melted'] = True
            events['melted_coords'] = list(set(self.last_melted_coords))
            self.last_melted_coords.clear()

        if self.last_steam_coords:
            events['steam_created'] = True
            events['steam_coords'] = list(set(self.last_steam_coords))
            self.last_steam_coords.clear()

        if self.last_toxic_coords:
            events['toxic_cloud_created'] = True
            events['toxic_coords'] = list(set(self.last_toxic_coords))
            self.last_toxic_coords.clear()

        if self.last_burned_coords:
            events['sand_burned'] = True
            events['burned_coords'] = list(set(self.last_burned_coords))
            self.last_burned_coords.clear()

        if self.last_extinguished_coords:
            events['fire_extinguished'] = True
            events['extinguished_coords'] = list(set(self.last_extinguished_coords))
            self.last_extinguished_coords.clear()

        return events
