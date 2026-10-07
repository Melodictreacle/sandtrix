"""Particle effects, screen shake, and floating score popups for Sandtrix."""

import random
from typing import List, Tuple
from PyQt6.QtGui import QPainter, QColor, QFont
from PyQt6.QtCore import Qt, QPointF


class SparkParticle:
    """A glowing sand spark emitted when a line clears or sand impacts."""

    def __init__(self, x: float, y: float, color: Tuple[int, int, int], is_bright: bool = False):
        self.x = x
        self.y = y
        # Velocity in pixels per frame
        angle = random.uniform(0, 6.28318)
        speed = random.uniform(1.2, 4.5) if not is_bright else random.uniform(2.0, 6.0)
        self.vx = speed * random.uniform(-1.0, 1.0)
        self.vy = -abs(speed) * random.uniform(0.6, 1.4)  # upward burst
        
        self.gravity = 0.15
        self.friction = 0.96
        self.color = color
        self.alpha = 255.0
        self.max_life = random.randint(25, 45)
        self.life = self.max_life
        self.size = random.uniform(1.8, 3.5)

    def update(self) -> bool:
        """Updates physics. Returns False if particle died."""
        self.x += self.vx
        self.y += self.vy
        self.vy += self.gravity
        self.vx *= self.friction
        self.life -= 1
        self.alpha = max(0.0, 255.0 * (self.life / self.max_life))
        return self.life > 0

    def draw(self, painter: QPainter):
        c = QColor(self.color[0], self.color[1], self.color[2], int(self.alpha))
        painter.setPen(Qt.PenStyle.NoPen)
        painter.setBrush(c)
        painter.drawRect(int(self.x), int(self.y), int(self.size), int(self.size))


class FloatingText:
    """Floating score or combo notification text that rises and fades."""

    def __init__(self, x: float, y: float, text: str, color: QColor, font_size: int = 14, bold: bool = True):
        self.x = x
        self.y = y
        self.vy = -1.2
        self.text = text
        self.color = color
        self.font_size = font_size
        self.bold = bold
        self.max_life = 40
        self.life = self.max_life

    def update(self) -> bool:
        self.y += self.vy
        self.vy *= 0.95
        self.life -= 1
        return self.life > 0

    def draw(self, painter: QPainter):
        alpha = int(255 * (self.life / self.max_life))
        col = QColor(self.color)
        col.setAlpha(alpha)

        font = QFont("Segoe UI", self.font_size)
        font.setBold(self.bold)
        painter.setFont(font)
        
        # Soft shadow
        shadow_col = QColor(0, 0, 0, int(alpha * 0.7))
        painter.setPen(shadow_col)
        painter.drawText(int(self.x + 2), int(self.y + 2), self.text)
        
        # Foreground text
        painter.setPen(col)
        painter.drawText(int(self.x), int(self.y), self.text)


class ParticleManager:
    """Manages all active particles, text popups, and screen shake."""

    def __init__(self):
        self.particles: List[SparkParticle] = []
        self.floating_texts: List[FloatingText] = []
        self.shake_intensity = 0.0

    def add_cleared_sand_sparks(self, pixel_coords: List[Tuple[float, float]], color: Tuple[int, int, int]):
        """Spawns sparkling burst particles for cleared sand grains."""
        # Sample a subset to keep performance silky smooth even for huge clears
        sample_step = max(1, len(pixel_coords) // 120)
        for i in range(0, len(pixel_coords), sample_step):
            px, py = pixel_coords[i]
            # Add 1-2 particles per sampled grain
            self.particles.append(SparkParticle(px + random.uniform(-2, 2), py + random.uniform(-2, 2), color))
            if random.random() < 0.3:
                # Occasional white-hot spark
                self.particles.append(SparkParticle(px, py, (255, 255, 255), is_bright=True))

    def add_landing_dust(self, pixel_x: float, pixel_y: float, color: Tuple[int, int, int], count: int = 15):
        """Spawns soft dust puffs when a piece impacts."""
        for _ in range(count):
            p = SparkParticle(pixel_x + random.uniform(-10, 10), pixel_y, color)
            p.vy = random.uniform(-1.0, -0.2)
            p.max_life = 20
            p.life = 20
            p.size = 2.0
            self.particles.append(p)

    def add_floating_text(self, x: float, y: float, text: str, color: QColor, font_size: int = 14):
        self.floating_texts.append(FloatingText(x, y, text, color, font_size=font_size))

    def trigger_shake(self, intensity: float = 6.0):
        self.shake_intensity = max(self.shake_intensity, intensity)

    def update(self) -> Tuple[float, float]:
        """
        Updates all particles and shake. Returns (shake_offset_x, shake_offset_y).
        """
        self.particles = [p for p in self.particles if p.update()]
        self.floating_texts = [t for t in self.floating_texts if t.update()]

        # Compute shake offset
        offset_x = 0.0
        offset_y = 0.0
        if self.shake_intensity > 0.1:
            offset_x = random.uniform(-self.shake_intensity, self.shake_intensity)
            offset_y = random.uniform(-self.shake_intensity, self.shake_intensity)
            self.shake_intensity *= 0.85
        else:
            self.shake_intensity = 0.0

        return offset_x, offset_y

    def draw(self, painter: QPainter):
        for p in self.particles:
            p.draw(painter)
        for t in self.floating_texts:
            t.draw(painter)

    def clear(self):
        self.particles.clear()
        self.floating_texts.clear()
        self.shake_intensity = 0.0
