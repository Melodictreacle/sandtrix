"""Modern cyberpunk arcade UI panels, previews, scoreboards, and badges for Sandtrix."""

from typing import Optional, List, Tuple
from PyQt6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QLabel, QFrame,
    QComboBox, QPushButton
)
from PyQt6.QtCore import Qt, QSize
from PyQt6.QtGui import QPainter, QColor, QFont, QPen, QBrush

from config import PALETTES, MINO_SIZE
from tetromino import Tetromino


class StatCard(QFrame):
    """Sleek dark card displaying a metric with title and vibrant neon value."""

    def __init__(self, title: str, initial_value: str = "0", accent_color: str = "#00E6FF"):
        super().__init__()
        self.accent_color = accent_color
        self.setStyleSheet(f"""
            QFrame {{
                background-color: #161822;
                border: 1px solid #2a2e3f;
                border-radius: 8px;
                padding: 6px;
            }}
        """)
        
        layout = QVBoxLayout(self)
        layout.setContentsMargins(10, 8, 10, 8)
        layout.setSpacing(2)

        self.lbl_title = QLabel(title.upper())
        self.lbl_title.setStyleSheet("color: #7d8597; font-size: 11px; font-weight: bold; letter-spacing: 1px;")
        
        self.lbl_val = QLabel(initial_value)
        self.lbl_val.setStyleSheet(f"color: {accent_color}; font-size: 20px; font-weight: bold; font-family: 'Segoe UI', sans-serif;")

        layout.addWidget(self.lbl_title)
        layout.addWidget(self.lbl_val)

    def set_value(self, value: str):
        self.lbl_val.setText(str(value))


class ComboBadge(QFrame):
    """Dynamic badge highlighting active combo streaks."""

    def __init__(self):
        super().__init__()
        self.setStyleSheet("""
            QFrame {
                background-color: #1a1528;
                border: 1px solid #ff4b82;
                border-radius: 8px;
                padding: 4px;
            }
        """)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(10, 6, 10, 6)
        
        self.lbl_combo = QLabel("COMBO: --")
        self.lbl_combo.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.lbl_combo.setStyleSheet("""
            color: #ff4b82;
            font-size: 14px;
            font-weight: bold;
            letter-spacing: 1px;
        """)
        layout.addWidget(self.lbl_combo)
        self.setVisible(False)

    def update_combo(self, combo: int):
        if combo > 1:
            self.lbl_combo.setText(f"🔥 COMBO x{combo}!")
            self.setVisible(True)
        else:
            self.setVisible(False)


class PiecePreviewWidget(QWidget):
    """Custom canvas rendering a preview of a Tetromino with individual sand grains."""

    def __init__(self, title: str, palette_name: str = "Cyber Neon", box_size: int = 80):
        super().__init__()
        self.title = title
        self.palette_name = palette_name
        self.box_size = box_size
        self.piece: Optional[Tetromino] = None
        self.setFixedSize(box_size, box_size + 24)

    def set_palette(self, palette_name: str):
        self.palette_name = palette_name
        self.update()

    def set_piece(self, piece: Optional[Tetromino]):
        self.piece = piece
        self.update()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)

        # Draw Title
        painter.setFont(QFont("Segoe UI", 9, QFont.Weight.Bold))
        painter.setPen(QColor("#7d8597"))
        painter.drawText(0, 14, self.title.upper())

        # Draw Background Box
        box_y = 20
        box_rect_h = self.box_size
        box_rect_w = self.box_size
        painter.setPen(QPen(QColor("#2a2e3f"), 1))
        painter.setBrush(QBrush(QColor("#161822")))
        painter.drawRoundedRect(0, box_y, box_rect_w, box_rect_h, 6, 6)

        if not self.piece:
            return

        # Render Tetromino grains
        palette_colors = PALETTES.get(self.palette_name, PALETTES["Cyber Neon"])["colors"]
        color_tuple = palette_colors[self.piece.color_idx % len(palette_colors)]
        grain_color = QColor(*color_tuple)

        mask = self.piece.grain_mask
        rows, cols = mask.shape
        
        # Determine scale so the piece fits neatly inside the box
        scale = min((self.box_size - 16) / cols, (self.box_size - 16) / rows)
        offset_x = (box_rect_w - cols * scale) / 2.0
        offset_y = box_y + (box_rect_h - rows * scale) / 2.0

        painter.setPen(Qt.PenStyle.NoPen)
        painter.setBrush(QBrush(grain_color))

        gy_indices, gx_indices = mask.nonzero()
        for gy, gx in zip(gy_indices, gx_indices):
            px = offset_x + gx * scale
            py = offset_y + gy * scale
            painter.drawRect(int(px), int(py), max(1, int(scale)), max(1, int(scale)))


class ControlsGuideWidget(QFrame):
    """Compact stylish cheat-sheet for game controls."""

    def __init__(self):
        super().__init__()
        self.setStyleSheet("""
            QFrame {
                background-color: #12141c;
                border: 1px solid #1f2333;
                border-radius: 8px;
            }
        """)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(10, 8, 10, 8)
        layout.setSpacing(4)

        header = QLabel("CONTROLS")
        header.setStyleSheet("color: #7d8597; font-size: 11px; font-weight: bold; letter-spacing: 1px;")
        layout.addWidget(header)

        shortcuts = [
            ("← / → / A / D", "Move"),
            ("↑ / W / X", "Rotate CW"),
            ("Z", "Rotate CCW"),
            ("↓ / S", "Soft Drop"),
            ("SPACE", "Hard Drop"),
            ("C / Shift", "Hold Piece"),
            ("P / Esc", "Pause"),
            ("R", "Restart"),
            ("M", "Mute Audio"),
        ]

        for keys, desc in shortcuts:
            row = QHBoxLayout()
            row.setSpacing(4)
            lbl_key = QLabel(keys)
            lbl_key.setStyleSheet("""
                background-color: #212534;
                color: #00E6FF;
                font-size: 10px;
                font-family: Consolas, monospace;
                padding: 1px 4px;
                border-radius: 3px;
            """)
            lbl_desc = QLabel(desc)
            lbl_desc.setStyleSheet("color: #b0b8cb; font-size: 10px;")
            
            row.addWidget(lbl_key)
            row.addStretch()
            row.addWidget(lbl_desc)
            layout.addLayout(row)
