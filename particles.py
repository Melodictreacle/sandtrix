import math
import random
from typing import List, Tuple
from PyQt6.QtGui import QPainter, QColor, QFont
from PyQt6.QtCore import Qt, QPointF


class CloudParticle:
    """A soft, billowing vapor/smoke cloud particle that drifts and dissipates after a few seconds."""

    def __init__(self, x: float, y: float, color: Tuple[int, int, int], max_life: int = 150, initial_size: float = 8.0, growth: float = 0.06):
        self.x = x
        self.y = y
        self.vx = random.uniform(-0.55, 0.55)
        self.vy = random.uniform(-0.85, -0.25)  # rises buoyantly
        self.color = color
        self.max_life = max_life  # ~120 to 180 frames = 2 to 3 seconds at 60 FPS
        self.life = max_life
        self.size = initial_size
        self.growth = growth
        self.friction = 0.99

    def update(self) -> bool:
        self.x += self.vx
        self.y += self.vy
        self.vx *= self.friction
        self.size += self.growth
        self.life -= 1
        return self.life > 0

    def draw(self, painter: QPainter):
        progress = self.life / self.max_life
        # Smooth bell fade: rises into visible cloud, then fades away after a few seconds
        alpha = int(170.0 * math.sin(progress * math.pi * 0.5))
        c = QColor(self.color[0], self.color[1], self.color[2], max(0, min(255, alpha)))
        painter.setPen(Qt.PenStyle.NoPen)
        painter.setBrush(c)
        r = int(self.size)
        painter.drawEllipse(int(self.x - r * 0.5), int(self.y - r * 0.5), r, r)


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
        self.clouds: List[CloudParticle] = []
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

    def add_freeze_crystals(self, pixel_coords: List[Tuple[float, float]], count: int = 40):
        """Spawns glittering crystalline frost particles when water freezes into ice."""
        if not pixel_coords:
            return
        sample_step = max(1, len(pixel_coords) // count)
        for i in range(0, len(pixel_coords), sample_step):
            px, py = pixel_coords[i]
            p = SparkParticle(px + random.uniform(-3, 3), py + random.uniform(-3, 3), (180, 240, 255), is_bright=True)
            p.vx = random.uniform(-1.5, 1.5)
            p.vy = random.uniform(-1.8, 0.2)
            p.gravity = 0.04
            p.max_life = 35
            p.life = 35
            p.size = 2.4
            self.particles.append(p)

    def add_steam_fizz(self, pixel_coords: List[Tuple[float, float]], count: int = 40):
        """Spawns rising steam puffs and vapor when fire melts ice."""
        if not pixel_coords:
            return
        sample_step = max(1, len(pixel_coords) // count)
        for i in range(0, len(pixel_coords), sample_step):
            px, py = pixel_coords[i]
            # Soft whitish-gray vapor particle
            col = random.choice([(230, 240, 255), (255, 200, 160), (220, 220, 230)])
            p = SparkParticle(px + random.uniform(-4, 4), py + random.uniform(-4, 4), col)
            p.vx = random.uniform(-0.8, 0.8)
            p.vy = random.uniform(-2.2, -0.6)  # rising vapor
            p.gravity = -0.03  # buoyant upward drift
            p.friction = 0.98
            p.max_life = 35
            p.life = 35
            p.size = 3.0
            self.particles.append(p)

    def add_steam_cloud(self, pixel_coords: List[Tuple[float, float]], count: int = 35):
        """Spawns billowing white steam cloud puffs that linger, rise, and disappear after a few seconds."""
        if not pixel_coords:
            return
        sample_step = max(1, len(pixel_coords) // count)
        for i in range(0, len(pixel_coords), sample_step):
            px, py = pixel_coords[i]
            col = random.choice([(230, 240, 255), (240, 245, 255), (210, 225, 240)])
            life = random.randint(120, 160)  # ~2.0 to 2.6 seconds
            self.clouds.append(CloudParticle(
                px + random.uniform(-8, 8), py + random.uniform(-6, 6),
                col, max_life=life, initial_size=random.uniform(7.0, 13.0), growth=random.uniform(0.04, 0.08)
            ))

    def add_toxic_cloud(self, pixel_coords: List[Tuple[float, float]], count: int = 40):
        """Spawns glowing green toxic cloud puffs when fire meets acid, disappearing after a few seconds."""
        if not pixel_coords:
            return
        sample_step = max(1, len(pixel_coords) // count)
        for i in range(0, len(pixel_coords), sample_step):
            px, py = pixel_coords[i]
            col = random.choice([(57, 255, 20), (80, 240, 45), (120, 255, 70), (45, 215, 35)])
            life = random.randint(130, 180)  # ~2.2 to 3.0 seconds
            self.clouds.append(CloudParticle(
                px + random.uniform(-8, 8), py + random.uniform(-6, 6),
                col, max_life=life, initial_size=random.uniform(8.0, 15.0), growth=random.uniform(0.05, 0.09)
            ))
            # Also add a few fiery-acidic sparks
            if random.random() < 0.3:
                spk = SparkParticle(px, py, (200, 255, 50))
                spk.vy = random.uniform(-2.5, -0.8)
                self.particles.append(spk)

    def add_ash_embers(self, pixel_coords: List[Tuple[float, float]], count: int = 35):
        """Spawns scorched ash flakes when sand burns to ash (pure ash gray tones, no red embers)."""
        if not pixel_coords:
            return
        sample_step = max(1, len(pixel_coords) // count)
        for i in range(0, len(pixel_coords), sample_step):
            px, py = pixel_coords[i]
            # Charcoal gray & soot ash flakes (pure gray tones, no red/orange pixels)
            ash_col = random.choice([(110, 115, 120), (140, 145, 150), (85, 90, 95), (160, 165, 170)])
            ash = SparkParticle(px + random.uniform(-4, 4), py + random.uniform(-4, 4), ash_col)
            ash.vx = random.uniform(-0.6, 0.6)
            ash.vy = random.uniform(-1.2, 0.2)
            ash.gravity = 0.02
            ash.max_life = 40
            ash.life = 40
            ash.size = random.uniform(2.0, 3.0)
            self.particles.append(ash)

    def add_diluted_acid_fizz(self, pixel_coords: List[Tuple[float, float]], count: int = 35):
        """Spawns effervescent seafoam-lime bubbles and soft frosty melt vapor when acid melts ice into diluted acid."""
        if not pixel_coords:
            return
        sample_step = max(1, len(pixel_coords) // count)
        for i in range(0, len(pixel_coords), sample_step):
            px, py = pixel_coords[i]
            col = random.choice([
                (24, 228, 204),   # Vibrant aquamarine water
                (140, 245, 240),  # Pale aqua shimmer
                (180, 245, 255),  # Frost melt water
                (40, 215, 225)    # Turquoise aqua highlight
            ])
            p = SparkParticle(px + random.uniform(-4, 4), py + random.uniform(-4, 4), col)
            p.vx = random.uniform(-1.0, 1.0)
            p.vy = random.uniform(-2.2, -0.6)  # Buoyant effervescent rise
            p.gravity = -0.02
            p.friction = 0.96
            p.max_life = 35
            p.life = 35
            p.size = random.uniform(2.5, 4.0)
            self.particles.append(p)
            if random.random() < 0.35:
                self.clouds.append(CloudParticle(
                    px + random.uniform(-6, 6), py + random.uniform(-4, 4),
                    (140, 240, 175), max_life=75, initial_size=6.0, growth=0.04
                ))

    def trigger_shake(self, intensity: float = 6.0):
        self.shake_intensity = max(self.shake_intensity, intensity)

    def update(self) -> Tuple[float, float]:
        """
        Updates all particles and shake. Returns (shake_offset_x, shake_offset_y).
        """
        self.particles = [p for p in self.particles if p.update()]
        self.clouds = [c for c in self.clouds if c.update()]
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
        for c in self.clouds:
            c.draw(painter)
        for p in self.particles:
            p.draw(painter)
        for t in self.floating_texts:
            t.draw(painter)

    def clear(self):
        self.particles.clear()
        self.clouds.clear()
        self.floating_texts.clear()
        self.shake_intensity = 0.0
