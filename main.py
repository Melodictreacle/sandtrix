"""SANDTRIX - Sand Tetris for Qt.
Main entry point and window layout.
"""

import sys
import os

from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QHBoxLayout, QVBoxLayout,
    QLabel, QComboBox, QPushButton, QFrame, QSpinBox
)
from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtGui import QIcon, QFont, QPalette, QColor

from config import PALETTES, DEFAULT_PALETTE
from audio import AudioManager
from game_view import SandtrixCanvas
from ui_panels import StatCard, ComboBadge, PiecePreviewWidget, ControlsGuideWidget


class SandtrixWindow(QMainWindow):
    """Main Application Window for Sandtrix."""

    def __init__(self):
        super().__init__()
        self.setWindowTitle("SANDTRIX - Sand Tetris")
        self.setWindowFlags(self.windowFlags())

        # Audio Manager
        self.audio = AudioManager()

        # Canvas
        self.canvas = SandtrixCanvas(audio_manager=self.audio, palette_name=DEFAULT_PALETTE)

        # Build UI Layout
        self._init_ui()

        # UI update timer (sync UI labels with engine state every frame)
        self.ui_timer = QTimer(self)
        self.ui_timer.timeout.connect(self._sync_ui)
        self.ui_timer.start(30)

    def _init_ui(self):
        # Overall dark modern stylesheet
        self.setStyleSheet("""
            QMainWindow {
                background-color: #0b0d13;
            }
            QWidget {
                color: #e2e8f0;
                font-family: 'Segoe UI', system-ui, sans-serif;
            }
            QComboBox {
                background-color: #161822;
                border: 1px solid #2a2e3f;
                border-radius: 6px;
                padding: 6px 12px;
                color: #00E6FF;
                font-weight: bold;
                font-size: 11px;
            }
            QComboBox::drop-down {
                border: none;
                width: 20px;
            }
            QComboBox QAbstractItemView {
                background-color: #161822;
                border: 1px solid #2a2e3f;
                selection-background-color: #212534;
                color: #e2e8f0;
            }
            QPushButton {
                background-color: #1a1e2d;
                border: 1px solid #2a2e3f;
                border-radius: 6px;
                padding: 7px 12px;
                color: #e2e8f0;
                font-weight: bold;
                font-size: 11px;
            }
            QPushButton:hover {
                background-color: #252b3f;
                border-color: #00E6FF;
                color: #00E6FF;
            }
            QPushButton:pressed {
                background-color: #161822;
            }
        """)

        central_widget = QWidget()
        main_layout = QHBoxLayout(central_widget)
        main_layout.setContentsMargins(18, 18, 18, 18)
        main_layout.setSpacing(16)

        # ----------------- LEFT PANEL -----------------
        left_panel = QVBoxLayout()
        left_panel.setSpacing(10)
        left_panel.setAlignment(Qt.AlignmentFlag.AlignTop)

        # Hold piece preview
        self.hold_preview = PiecePreviewWidget(title="HOLD", palette_name=DEFAULT_PALETTE, box_size=84)
        left_panel.addWidget(self.hold_preview)

        # Stat cards
        self.card_score = StatCard("Score", "0", accent_color="#00E6FF")
        self.card_high_score = StatCard("High Score", str(self.canvas.engine.high_score), accent_color="#FFD700")
        self.card_level = StatCard("Level", "1", accent_color="#2ED573")
        self.card_bands = StatCard("Lines Cleared", "0", accent_color="#FF4B82")

        left_panel.addWidget(self.card_score)
        left_panel.addWidget(self.card_high_score)
        left_panel.addWidget(self.card_level)
        left_panel.addWidget(self.card_bands)

        # Combo badge
        self.combo_badge = ComboBadge()
        left_panel.addWidget(self.combo_badge)

        left_panel.addStretch()

        # ----------------- CENTER PANEL -----------------
        center_panel = QVBoxLayout()
        center_panel.setAlignment(Qt.AlignmentFlag.AlignCenter)
        center_panel.addWidget(self.canvas)

        # ----------------- RIGHT PANEL -----------------
        right_panel = QVBoxLayout()
        right_panel.setSpacing(10)
        right_panel.setAlignment(Qt.AlignmentFlag.AlignTop)

        # Next pieces preview (stack of 3 upcoming pieces)
        lbl_next = QLabel("NEXT PIECES")
        lbl_next.setStyleSheet("color: #7d8597; font-size: 11px; font-weight: bold; letter-spacing: 1px;")
        right_panel.addWidget(lbl_next)

        self.next_previews = []
        for i in range(3):
            preview = PiecePreviewWidget(title=f"#{i + 1}", palette_name=DEFAULT_PALETTE, box_size=72)
            self.next_previews.append(preview)
            right_panel.addWidget(preview)

        # Options Box
        options_frame = QFrame()
        options_frame.setStyleSheet("""
            QFrame {
                background-color: #12141c;
                border: 1px solid #1f2333;
                border-radius: 8px;
                padding: 6px;
            }
        """)
        options_layout = QVBoxLayout(options_frame)
        options_layout.setContentsMargins(8, 8, 8, 8)
        options_layout.setSpacing(8)

        lbl_pal = QLabel("COLOR PALETTE")
        lbl_pal.setStyleSheet("color: #7d8597; font-size: 10px; font-weight: bold; letter-spacing: 1px;")
        options_layout.addWidget(lbl_pal)

        self.combo_palette = QComboBox()
        for p_name in PALETTES.keys():
            self.combo_palette.addItem(p_name)
        self.combo_palette.currentTextChanged.connect(self._on_palette_changed)
        options_layout.addWidget(self.combo_palette)

        # Level / Difficulty Stepper
        lbl_lvl = QLabel("DIFFICULTY / STARTING LEVEL")
        lbl_lvl.setStyleSheet("color: #7d8597; font-size: 10px; font-weight: bold; letter-spacing: 1px;")
        options_layout.addWidget(lbl_lvl)

        lvl_row = QHBoxLayout()
        lvl_row.setSpacing(6)

        btn_lvl_down = QPushButton("◀ −")
        btn_lvl_down.setFixedWidth(46)
        btn_lvl_down.clicked.connect(self._on_level_down)

        self.spin_level = QSpinBox()
        self.spin_level.setRange(1, 10)
        self.spin_level.setValue(1)
        self.spin_level.setPrefix("Level  ")
        self.spin_level.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.spin_level.setStyleSheet("""
            QSpinBox {
                background-color: #161822;
                border: 1px solid #2a2e3f;
                border-radius: 6px;
                padding: 4px 8px;
                color: #2ED573;
                font-weight: bold;
                font-size: 12px;
            }
        """)
        self.spin_level.valueChanged.connect(self._on_level_changed)

        btn_lvl_up = QPushButton("+ ▶")
        btn_lvl_up.setFixedWidth(46)
        btn_lvl_up.clicked.connect(self._on_level_up)

        lvl_row.addWidget(btn_lvl_down)
        lvl_row.addWidget(self.spin_level)
        lvl_row.addWidget(btn_lvl_up)
        options_layout.addLayout(lvl_row)

        # Mission / Line Goal Stepper
        lbl_goal = QLabel("MISSION / LINE GOAL")
        lbl_goal.setStyleSheet("color: #7d8597; font-size: 10px; font-weight: bold; letter-spacing: 1px;")
        options_layout.addWidget(lbl_goal)

        goal_row = QHBoxLayout()
        goal_row.setSpacing(6)

        btn_goal_down = QPushButton("◀ −")
        btn_goal_down.setFixedWidth(46)
        btn_goal_down.clicked.connect(self._on_goal_prev)

        self.btn_goal_val = QPushButton("ENDLESS")
        self.btn_goal_val.setStyleSheet("""
            QPushButton {
                background-color: #161822;
                border: 1.5px solid #285e43;
                border-radius: 6px;
                padding: 4px 8px;
                color: #68c48a;
                font-weight: bold;
                font-size: 11px;
            }
        """)
        self.btn_goal_val.clicked.connect(self._on_goal_cycle)

        btn_goal_up = QPushButton("+ ▶")
        btn_goal_up.setFixedWidth(46)
        btn_goal_up.clicked.connect(self._on_goal_next)

        goal_row.addWidget(btn_goal_down)
        goal_row.addWidget(self.btn_goal_val)
        goal_row.addWidget(btn_goal_up)
        options_layout.addLayout(goal_row)

        self.goal_index = 0
        self.goal_options = [
            (0, "ENDLESS", "#68c48a", "#285e43"),
            (20, "20 LINES", "#38b2ac", "#234e52"),
            (40, "40 LINES", "#7cb7ea", "#2b4c7e"),
            (100, "100 LINES", "#f6ad55", "#7b341e")
        ]

        # Sound & Restart buttons
        btn_row = QHBoxLayout()
        btn_row.setSpacing(6)
        
        self.btn_sound = QPushButton("🔊 Sound")
        self.btn_sound.clicked.connect(self._toggle_sound)
        btn_row.addWidget(self.btn_sound)

        self.btn_restart = QPushButton("🔄 Restart")
        self.btn_restart.clicked.connect(self._restart_game)
        btn_row.addWidget(self.btn_restart)

        options_layout.addLayout(btn_row)
        right_panel.addWidget(options_frame)

        # Controls Guide
        self.controls_guide = ControlsGuideWidget()
        right_panel.addWidget(self.controls_guide)

        right_panel.addStretch()

        # Add all to main horizontal layout
        main_layout.addLayout(left_panel)
        main_layout.addLayout(center_panel)
        main_layout.addLayout(right_panel)

        self.setCentralWidget(central_widget)
        self.setFixedSize(self.sizeHint())

    def _sync_ui(self):
        """Synchronizes UI cards with the current game engine state."""
        engine = self.canvas.engine
        
        # Update metrics
        self.card_score.set_value(f"{engine.score:,}")
        self.card_high_score.set_value(f"{engine.high_score:,}")
        self.card_level.set_value(str(engine.level))
        if getattr(engine, 'line_goal', 0) > 0:
            self.card_bands.set_title("Goal Progress")
            self.card_bands.set_value(f"{engine.lines_cleared} / {engine.line_goal}")
        else:
            self.card_bands.set_title("Lines Cleared")
            self.card_bands.set_value(str(engine.lines_cleared))
        self.combo_badge.update_combo(engine.combo)

        # Update Hold piece
        self.hold_preview.set_piece(engine.hold_piece)

        # Update Next queue
        for i, preview in enumerate(self.next_previews):
            if i < len(engine.next_queue):
                preview.set_piece(engine.next_queue[i])
            else:
                preview.set_piece(None)

    def _on_palette_changed(self, palette_name: str):
        self.canvas.set_palette(palette_name)
        self.hold_preview.set_palette(palette_name)
        for prev in self.next_previews:
            prev.set_palette(palette_name)
        self.canvas.setFocus()

    def _toggle_sound(self):
        muted = self.audio.toggle_mute()
        self.btn_sound.setText("🔇 Muted" if muted else "🔊 Sound")
        self.canvas.setFocus()

    def _on_level_down(self):
        val = self.spin_level.value()
        if val > 1:
            self.spin_level.setValue(val - 1)

    def _on_level_up(self):
        val = self.spin_level.value()
        if val < 10:
            self.spin_level.setValue(val + 1)

    def _on_level_changed(self, val: int):
        self.canvas.engine.start_level = val
        self.canvas.engine.level = val
        self.card_level.set_value(str(val))
        self.canvas.setFocus()

    def _on_goal_prev(self):
        if self.goal_index > 0:
            self.goal_index -= 1
            self._update_goal_display()

    def _on_goal_next(self):
        if self.goal_index < len(self.goal_options) - 1:
            self.goal_index += 1
            self._update_goal_display()

    def _on_goal_cycle(self):
        self.goal_index = (self.goal_index + 1) % len(self.goal_options)
        self._update_goal_display()

    def _update_goal_display(self):
        val, label, col, border = self.goal_options[self.goal_index]
        self.canvas.engine.line_goal = val
        self.btn_goal_val.setText(label)
        self.btn_goal_val.setStyleSheet(f"""
            QPushButton {{
                background-color: #161822;
                border: 1.5px solid {border};
                border-radius: 6px;
                padding: 4px 8px;
                color: {col};
                font-weight: bold;
                font-size: 11px;
            }}
        """)
        self.canvas.setFocus()

    def _restart_game(self):
        self.canvas.engine.reset()
        self.card_level.set_value(str(self.canvas.engine.level))
        self.canvas.particles.clear()
        self.canvas.update()
        self.canvas.setFocus()


def main():
    app = QApplication(sys.argv)
    app.setStyle("Fusion")
    
    # Palette dark styling
    palette = QPalette()
    palette.setColor(QPalette.ColorRole.Window, QColor("#0b0d13"))
    palette.setColor(QPalette.ColorRole.WindowText, QColor("#e2e8f0"))
    palette.setColor(QPalette.ColorRole.Base, QColor("#161822"))
    palette.setColor(QPalette.ColorRole.AlternateBase, QColor("#1a1e2d"))
    palette.setColor(QPalette.ColorRole.ToolTipBase, QColor("#e2e8f0"))
    palette.setColor(QPalette.ColorRole.ToolTipText, QColor("#0b0d13"))
    palette.setColor(QPalette.ColorRole.Text, QColor("#e2e8f0"))
    palette.setColor(QPalette.ColorRole.Button, QColor("#1a1e2d"))
    palette.setColor(QPalette.ColorRole.ButtonText, QColor("#e2e8f0"))
    palette.setColor(QPalette.ColorRole.Highlight, QColor("#00E6FF"))
    palette.setColor(QPalette.ColorRole.HighlightedText, QColor("#0b0d13"))
    app.setPalette(palette)

    window = SandtrixWindow()
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
