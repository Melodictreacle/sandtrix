"""Main interactive game canvas, rendering loop, animations, and input handling for Sandtrix."""

import math
from typing import Dict, Optional, Tuple
import numpy as np

from PyQt6.QtWidgets import QWidget
from PyQt6.QtCore import Qt, QTimer, QRect, QPointF
from PyQt6.QtGui import (
    QPainter, QColor, QFont, QPen, QBrush, QImage, QKeyEvent
)

from config import (
    BOARD_WIDTH, BOARD_HEIGHT, CELL_DISPLAY_SIZE, CANVAS_WIDTH, CANVAS_HEIGHT,
    DANGER_ROW, FRAME_TIME_MS, PALETTES, DEFAULT_PALETTE,
    DAS_DELAY_MS, ARR_REPEAT_MS, MINO_SIZE
)
from sand_engine import SandEngine
from particles import ParticleManager
from audio import AudioManager


class SandtrixCanvas(QWidget):
    """The central game screen rendering the falling sand, active piece, and effects."""

    def __init__(self, audio_manager: AudioManager, palette_name: str = DEFAULT_PALETTE):
        super().__init__()
        self.audio = audio_manager
        self.palette_name = palette_name
        self.palette_info = PALETTES[palette_name]
        
        # Build NumPy color lookup table
        self._rebuild_palette_lut()

        # Engine & Particles
        num_colors = len(self.palette_info["colors"]) - 1
        self.engine = SandEngine(num_colors=num_colors)
        self.particles = ParticleManager()

        # UI scale
        self.setFixedSize(CANVAS_WIDTH, CANVAS_HEIGHT)
        self.setFocusPolicy(Qt.FocusPolicy.StrongFocus)

        # Keys tracking for DAS/ARR
        self.key_left_held = False
        self.key_right_held = False
        self.das_timer = 0
        self.arr_timer = 0
        self.active_dir = 0  # -1 for left, +1 for right

        # Animation state
        self.pulse_time = 0.0

        # Main 60 FPS Game Loop Timer
        self.timer = QTimer(self)
        self.timer.timeout.connect(self._game_loop)
        self.timer.start(FRAME_TIME_MS)

    def _rebuild_palette_lut(self):
        """Constructs an RGBA lookup array for NumPy array-to-QImage conversion."""
        raw_colors = self.palette_info["colors"]
        lut = []
        for c in raw_colors:
            # Format RGBA
            lut.append([c[0], c[1], c[2], c[3]])
        self.palette_lut = np.array(lut, dtype=np.uint8)

    def set_palette(self, palette_name: str):
        if palette_name in PALETTES:
            self.palette_name = palette_name
            self.palette_info = PALETTES[palette_name]
            self._rebuild_palette_lut()
            num_colors = len(self.palette_info["colors"]) - 1
            self.engine.set_palette_colors_count(num_colors)
            self.particles.clear()
            self.update()

    def _game_loop(self):
        """Main frame update running at 60 FPS."""
        self.pulse_time += 0.05
        dt = FRAME_TIME_MS

        # Handle DAS (Delayed Auto Shift) for horizontal movement
        if self.key_left_held or self.key_right_held:
            self.das_timer += dt
            if self.das_timer >= DAS_DELAY_MS:
                self.arr_timer += dt
                if self.arr_timer >= ARR_REPEAT_MS:
                    self.arr_timer = 0
                    if self.engine.move_piece(self.active_dir * MINO_SIZE):
                        self.audio.play_move()

        # Tick engine physics
        events = self.engine.tick(dt)

        # Handle game events
        if events['landed']:
            self.audio.play_lock()

        if events['cleared']:
            # Line clear triggered!
            coords = events['coords']
            col_tuple = self.palette_info["colors"][events['color']][:3]
            
            # Convert grain coordinates to canvas pixels
            pixel_coords = [(cx * CELL_DISPLAY_SIZE, cy * CELL_DISPLAY_SIZE) for cy, cx in coords]
            self.particles.add_cleared_sand_sparks(pixel_coords, col_tuple)
            
            # Floating text at midpoint of cleared band
            mid_y = int(np.mean([cy for cy, _ in coords])) * CELL_DISPLAY_SIZE if coords else 200
            combo = events['combo']
            pts = events['points']
            
            if combo > 1:
                self.particles.add_floating_text(
                    CANVAS_WIDTH // 2 - 50, mid_y,
                    f"COMBO x{combo}! +{pts}",
                    QColor(*col_tuple), font_size=15
                )
            else:
                self.particles.add_floating_text(
                    CANVAS_WIDTH // 2 - 35, mid_y,
                    f"+{pts}",
                    QColor(*col_tuple), font_size=14
                )

            self.particles.trigger_shake(3.0 + min(10.0, combo * 2.0))
            self.audio.play_clear(combo)

        # Trigger Game Over sound once
        if self.engine.game_over and not hasattr(self, "_game_over_sound_played"):
            self.audio.play_game_over()
            self._game_over_sound_played = True
        elif not self.engine.game_over:
            self._game_over_sound_played = False

        # Redraw screen
        self.update()

    def keyPressEvent(self, event: QKeyEvent):
        key = event.key()

        # Restart
        if key == Qt.Key.Key_R:
            self.engine.reset()
            self.particles.clear()
            self.update()
            return

        # Pause
        if key in (Qt.Key.Key_P, Qt.Key.Key_Escape):
            self.engine.paused = not self.engine.paused
            self.update()
            return

        # Mute audio
        if key == Qt.Key.Key_M:
            self.audio.toggle_mute()
            return

        if self.engine.game_over or self.engine.paused:
            return

        # Left
        if key in (Qt.Key.Key_Left, Qt.Key.Key_A):
            if not self.key_left_held:
                self.key_left_held = True
                self.active_dir = -1
                self.das_timer = 0
                self.arr_timer = 0
                if self.engine.move_piece(-MINO_SIZE):
                    self.audio.play_move()

        # Right
        elif key in (Qt.Key.Key_Right, Qt.Key.Key_D):
            if not self.key_right_held:
                self.key_right_held = True
                self.active_dir = 1
                self.das_timer = 0
                self.arr_timer = 0
                if self.engine.move_piece(MINO_SIZE):
                    self.audio.play_move()

        # Rotate Clockwise
        elif key in (Qt.Key.Key_Up, Qt.Key.Key_W, Qt.Key.Key_X):
            if self.engine.rotate_piece(1):
                self.audio.play_rotate()

        # Rotate Counter-Clockwise
        elif key in (Qt.Key.Key_Z, Qt.Key.Key_Control):
            if self.engine.rotate_piece(-1):
                self.audio.play_rotate()

        # Soft Drop
        elif key in (Qt.Key.Key_Down, Qt.Key.Key_S):
            self.engine.is_soft_dropping = True

        # Hard Drop
        elif key == Qt.Key.Key_Space:
            lx, ly, impact_coords = self.engine.hard_drop()
            self.audio.play_drop()
            self.particles.trigger_shake(4.0)
            if impact_coords:
                # Add landing dust particles
                sample_col = self.palette_info["colors"][1][:3]
                self.particles.add_landing_dust(
                    lx * CELL_DISPLAY_SIZE + 20, ly * CELL_DISPLAY_SIZE, sample_col
                )

        # Hold
        elif key in (Qt.Key.Key_C, Qt.Key.Key_Shift):
            if self.engine.hold_current_piece():
                self.audio.play_move()

    def keyReleaseEvent(self, event: QKeyEvent):
        key = event.key()

        if key in (Qt.Key.Key_Left, Qt.Key.Key_A):
            self.key_left_held = False
            if self.key_right_held:
                self.active_dir = 1
                self.das_timer = 0

        elif key in (Qt.Key.Key_Right, Qt.Key.Key_D):
            self.key_right_held = False
            if self.key_left_held:
                self.active_dir = -1
                self.das_timer = 0

        elif key in (Qt.Key.Key_Down, Qt.Key.Key_S):
            self.engine.is_soft_dropping = False

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)

        # Update and apply screen shake
        shake_x, shake_y = self.particles.update()
        painter.translate(shake_x, shake_y)

        # 1. Render Sand Grid from NumPy to QImage
        # Map grid integers to RGBA palette
        grid_data = self.engine.grid
        rgba_buffer = self.palette_lut[grid_data]  # shape: (H, W, 4)
        
        # Create QImage from memory buffer
        h, w = grid_data.shape
        qimg = QImage(rgba_buffer.data, w, h, w * 4, QImage.Format.Format_RGBA8888)
        
        # Draw scaled sand grid (nearest neighbor for crisp granular sand grains)
        painter.drawImage(QRect(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT), qimg)

        # 2. Draw Subtle Border & Grid Frame
        painter.setPen(QPen(QColor(42, 46, 63, 160), 1))
        painter.setBrush(Qt.BrushStyle.NoBrush)
        painter.drawRect(0, 0, CANVAS_WIDTH - 1, CANVAS_HEIGHT - 1)

        # 3. Draw Danger Ceiling Line
        danger_y = DANGER_ROW * CELL_DISPLAY_SIZE
        pulse_alpha = int(120 + 80 * math.sin(self.pulse_time * 3))
        painter.setPen(QPen(QColor(255, 60, 60, pulse_alpha), 1, Qt.PenStyle.DashLine))
        painter.drawLine(0, danger_y, CANVAS_WIDTH, danger_y)

        # 4. Render Ghost Piece (Dotted / Translucent Silhouette)
        if self.engine.active_piece and not self.engine.game_over:
            piece = self.engine.active_piece
            ghost_y = piece.get_ghost_y(self.engine.grid)
            
            # Don't draw ghost if piece is already at landing spot
            if ghost_y != piece.y:
                ghost_color_tuple = self.palette_info["colors"][piece.color_idx][:3]
                ghost_color = QColor(*ghost_color_tuple)
                ghost_color.setAlpha(65)
                painter.setPen(Qt.PenStyle.NoPen)
                painter.setBrush(ghost_color)

                gy_idx, gx_idx = np.where(piece.grain_mask)
                for gy, gx in zip(gy_idx, gx_idx):
                    px = (piece.x + gx) * CELL_DISPLAY_SIZE
                    py = (ghost_y + gy) * CELL_DISPLAY_SIZE
                    painter.drawRect(px, py, CELL_DISPLAY_SIZE, CELL_DISPLAY_SIZE)

        # 5. Render Active Falling Tetromino
        if self.engine.active_piece and not self.engine.game_over:
            piece = self.engine.active_piece
            piece_color_tuple = self.palette_info["colors"][piece.color_idx][:3]
            piece_color = QColor(*piece_color_tuple)
            
            painter.setPen(Qt.PenStyle.NoPen)
            painter.setBrush(piece_color)

            gy_idx, gx_idx = np.where(piece.grain_mask)
            for gy, gx in zip(gy_idx, gx_idx):
                px = (piece.x + gx) * CELL_DISPLAY_SIZE
                py = (piece.y + gy) * CELL_DISPLAY_SIZE
                painter.drawRect(px, py, CELL_DISPLAY_SIZE, CELL_DISPLAY_SIZE)

        # 6. Render Particles & Floating Text
        self.particles.draw(painter)

        # 7. Render Overlays (Pause / Game Over)
        if self.engine.paused:
            self._draw_pause_overlay(painter)
        elif self.engine.game_over:
            self._draw_game_over_overlay(painter)

    def _draw_pause_overlay(self, painter: QPainter):
        # Dark translucent overlay
        painter.fillRect(self.rect(), QColor(10, 12, 18, 190))

        painter.setFont(QFont("Segoe UI", 26, QFont.Weight.Bold))
        painter.setPen(QColor("#00E6FF"))
        painter.drawText(self.rect(), Qt.AlignmentFlag.AlignCenter, "PAUSED")

        painter.setFont(QFont("Segoe UI", 12))
        painter.setPen(QColor("#a0a8be"))
        rect_sub = self.rect().adjusted(0, 60, 0, 0)
        painter.drawText(rect_sub, Qt.AlignmentFlag.AlignCenter, "Press P or Esc to Resume")

    def _draw_game_over_overlay(self, painter: QPainter):
        # Dark backdrop with crimson tint
        painter.fillRect(self.rect(), QColor(15, 8, 12, 210))

        center_y = CANVAS_HEIGHT // 2 - 50

        # "GAME OVER" Text
        painter.setFont(QFont("Segoe UI", 28, QFont.Weight.Bold))
        painter.setPen(QColor("#FF4B82"))
        painter.drawText(QRect(0, center_y, CANVAS_WIDTH, 40), Qt.AlignmentFlag.AlignCenter, "GAME OVER")

        # Score Summary
        painter.setFont(QFont("Segoe UI", 14))
        painter.setPen(QColor("#FFFFFF"))
        painter.drawText(
            QRect(0, center_y + 55, CANVAS_WIDTH, 25),
            Qt.AlignmentFlag.AlignCenter,
            f"Score: {self.engine.score:,}"
        )

        painter.setFont(QFont("Segoe UI", 12))
        painter.setPen(QColor("#FFD700"))
        painter.drawText(
            QRect(0, center_y + 85, CANVAS_WIDTH, 25),
            Qt.AlignmentFlag.AlignCenter,
            f"High Score: {self.engine.high_score:,}"
        )

        # Pulsing Restart Prompt
        restart_alpha = int(170 + 75 * math.sin(self.pulse_time * 4))
        painter.setFont(QFont("Segoe UI", 12, QFont.Weight.Bold))
        painter.setPen(QColor(0, 230, 255, restart_alpha))
        painter.drawText(
            QRect(0, center_y + 130, CANVAS_WIDTH, 30),
            Qt.AlignmentFlag.AlignCenter,
            "Press [R] to Play Again"
        )
