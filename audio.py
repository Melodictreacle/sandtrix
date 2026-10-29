"""Procedural sound effects engine for Sandtrix with QSoundEffect and graceful fallbacks."""

import os
import math
import wave
import struct
import random
from PyQt6.QtCore import QUrl
from PyQt6.QtMultimedia import QSoundEffect


def generate_wav(filepath: str, sample_rate: int = 44100, duration: float = 0.1, generator_fn=None):
    """Generates a mono 16-bit PCM WAV file using a sample generator callback."""
    total_samples = int(sample_rate * duration)
    with wave.open(filepath, 'w') as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(sample_rate)
        data = bytearray()
        for i in range(total_samples):
            t = i / sample_rate
            sample = generator_fn(t, duration, i)
            # Clamp to 16-bit signed range
            sample_val = max(-32767, min(32767, int(sample * 30000)))
            data.extend(struct.pack('<h', sample_val))
        wf.writeframes(data)


def create_sound_assets(sounds_dir: str):
    """Generates all game procedural sound effects if they don't already exist."""
    os.makedirs(sounds_dir, exist_ok=True)

    # 1. Move Sound: Quick high click/tick
    move_path = os.path.join(sounds_dir, "move.wav")
    if not os.path.exists(move_path):
        def gen_move(t, d, i):
            freq = 900 - (t / d) * 300
            env = math.exp(-35.0 * t / d)
            return math.sin(2 * math.pi * freq * t) * env * 0.4
        generate_wav(move_path, duration=0.035, generator_fn=gen_move)

    # 2. Rotate Sound: Snappy two-tone glide
    rot_path = os.path.join(sounds_dir, "rotate.wav")
    if not os.path.exists(rot_path):
        def gen_rot(t, d, i):
            freq = 600 + (t / d) * 450
            env = math.sin(math.pi * (t / d))
            return (0.7 * math.sin(2 * math.pi * freq * t) + 0.3 * math.sin(4 * math.pi * freq * t)) * env * 0.5
        generate_wav(rot_path, duration=0.06, generator_fn=gen_rot)

    # 3. Drop / Land Sound: Granular low thump with sand rustle
    drop_path = os.path.join(sounds_dir, "drop.wav")
    if not os.path.exists(drop_path):
        def gen_drop(t, d, i):
            env = math.exp(-18.0 * t / d)
            thump = math.sin(2 * math.pi * (140 - 70 * (t / d)) * t)
            noise = (random.random() * 2.0 - 1.0) * 0.35
            return (thump * 0.7 + noise * 0.3) * env * 0.65
        generate_wav(drop_path, duration=0.12, generator_fn=gen_drop)

    # 4. Lock Sound: Crisp tactile mechanical snap
    lock_path = os.path.join(sounds_dir, "lock.wav")
    if not os.path.exists(lock_path):
        def gen_lock(t, d, i):
            env = math.exp(-30.0 * t / d)
            return (math.sin(2 * math.pi * 520 * t) + 0.5 * math.sin(2 * math.pi * 1040 * t)) * env * 0.45
        generate_wav(lock_path, duration=0.045, generator_fn=gen_lock)

    # 5. Clear Sounds (Combo Levels 1, 2, 3, 4+): Euphoric ascending arpeggios
    root_freqs = [523.25, 659.25, 783.99, 1046.50]  # C5, E5, G5, C6
    for idx, root in enumerate(root_freqs, start=1):
        clear_path = os.path.join(sounds_dir, f"clear_{idx}.wav")
        if not os.path.exists(clear_path):
            def make_gen_clear(r_freq):
                def gen_clear(t, d, i):
                    # Play 3 arpeggio notes in rapid sequence with bell envelopes
                    seg = int(t / (d / 3.0))
                    freq_mult = [1.0, 1.25, 1.5][min(2, seg)]
                    freq = r_freq * freq_mult
                    local_t = t - (seg * (d / 3.0))
                    env = math.exp(-12.0 * local_t / (d / 3.0))
                    wave_val = (
                        0.7 * math.sin(2 * math.pi * freq * t) +
                        0.25 * math.sin(4 * math.pi * freq * t) +
                        0.1 * math.sin(6 * math.pi * freq * t)
                    )
                    return wave_val * env * 0.7
                return gen_clear
            generate_wav(clear_path, duration=0.28, generator_fn=make_gen_clear(root))

    # 6. Game Over Sound: Melancholy descending minor phrase
    go_path = os.path.join(sounds_dir, "game_over.wav")
    if not os.path.exists(go_path):
        def gen_go(t, d, i):
            seg = int(t / (d / 3.0))
            freqs = [440.0, 392.0, 349.23]  # A4, G4, F4
            freq = freqs[min(2, seg)]
            local_t = t - (seg * (d / 3.0))
            env = math.exp(-8.0 * local_t / (d / 3.0))
            return math.sin(2 * math.pi * freq * t) * env * 0.6
        generate_wav(go_path, duration=0.6, generator_fn=gen_go)

    # 7. Freeze Sound: Shimmering icy high-frequency crystal chime
    freeze_path = os.path.join(sounds_dir, "freeze.wav")
    if not os.path.exists(freeze_path):
        def gen_freeze(t, d, i):
            env = math.exp(-12.0 * t / d)
            c1 = math.sin(2 * math.pi * (1200 + 400 * (t / d)) * t)
            c2 = math.sin(2 * math.pi * 2400 * t) * 0.4
            noise = (random.random() * 2.0 - 1.0) * 0.15
            return (c1 * 0.6 + c2 + noise) * env * 0.55
        generate_wav(freeze_path, duration=0.18, generator_fn=gen_freeze)

    # 8. Steam / Melt Sound: Sizzling hiss when fire melts ice
    steam_path = os.path.join(sounds_dir, "steam.wav")
    if not os.path.exists(steam_path):
        def gen_steam(t, d, i):
            env = math.exp(-8.0 * t / d)
            noise = (random.random() * 2.0 - 1.0)
            tone = math.sin(2 * math.pi * 320 * t) * 0.2
            return (noise * 0.8 + tone) * env * 0.5
        generate_wav(steam_path, duration=0.22, generator_fn=gen_steam)

    # 9. Toxic Gas Sound: Bubbling noxious chemical hiss when fire meets acid
    toxic_path = os.path.join(sounds_dir, "toxic_gas.wav")
    if not os.path.exists(toxic_path):
        def gen_toxic(t, d, i):
            env = math.exp(-5.0 * t / d)
            bubble = math.sin(2 * math.pi * (180 + 90 * math.sin(40 * t)) * t) * 0.4
            hiss = (random.random() * 2.0 - 1.0) * 0.6
            return (bubble + hiss) * env * 0.5
        generate_wav(toxic_path, duration=0.28, generator_fn=gen_toxic)

    # 10. Scorch Sound: Crackling burn when fire lands on sand
    scorch_path = os.path.join(sounds_dir, "scorch.wav")
    if not os.path.exists(scorch_path):
        def gen_scorch(t, d, i):
            env = math.exp(-9.0 * t / d)
            crackle = (random.random() * 2.0 - 1.0) if random.random() < 0.35 else 0.0
            flame = math.sin(2 * math.pi * (240 - 80 * (t / d)) * t) * 0.3
            return (crackle * 0.7 + flame) * env * 0.5
        generate_wav(scorch_path, duration=0.20, generator_fn=gen_scorch)

    # 11. Acid Dilution Sound: Sizzling effervescent chemical reaction hiss when acid melts ice/water
    acid_dilute_path = os.path.join(sounds_dir, "acid_dilute.wav")
    if not os.path.exists(acid_dilute_path):
        def gen_acid_dilute(t, d, i):
            env = math.exp(-6.0 * t / d)
            noise = (random.random() * 2.0 - 1.0) * 0.45
            bubble = math.sin(2 * math.pi * (260 + 130 * math.sin(55 * t)) * t) * 0.35
            chime = math.sin(2 * math.pi * 1400 * t) * 0.15 * math.exp(-15.0 * t / d)
            return (noise + bubble + chime) * env * 0.6
        generate_wav(acid_dilute_path, duration=0.25, generator_fn=gen_acid_dilute)


class AudioManager:
    """Manages audio playback for Sandtrix with volume and mute control."""

    def __init__(self):
        self.muted = False
        self.volume = 0.65
        self.sounds = {}
        
        self.sounds_dir = os.path.join(os.path.dirname(__file__), "sounds")
        try:
            create_sound_assets(self.sounds_dir)
            self._load_effects()
        except Exception as e:
            print(f"[AudioManager] Audio initialization notice: {e}")

    def _load_effects(self):
        effect_names = [
            "move", "rotate", "drop", "lock",
            "clear_1", "clear_2", "clear_3", "clear_4",
            "game_over", "freeze", "steam", "toxic_gas", "scorch", "acid_dilute"
        ]
        for name in effect_names:
            path = os.path.join(self.sounds_dir, f"{name}.wav")
            if os.path.exists(path):
                effect = QSoundEffect()
                effect.setSource(QUrl.fromLocalFile(os.path.abspath(path)))
                effect.setVolume(self.volume)
                self.sounds[name] = effect

    def set_muted(self, muted: bool):
        self.muted = muted

    def toggle_mute(self) -> bool:
        self.muted = not self.muted
        return self.muted

    def set_volume(self, volume: float):
        self.volume = max(0.0, min(1.0, volume))
        for eff in self.sounds.values():
            eff.setVolume(self.volume)

    def play(self, name: str):
        if self.muted:
            return
        eff = self.sounds.get(name)
        if eff:
            try:
                # If currently playing, stop and rewind for instant response
                eff.play()
            except Exception:
                pass

    def play_move(self):
        self.play("move")

    def play_rotate(self):
        self.play("rotate")

    def play_drop(self):
        self.play("drop")

    def play_lock(self):
        self.play("lock")

    def play_clear(self, combo: int = 1):
        sound_key = f"clear_{min(4, max(1, combo))}"
        self.play(sound_key)

    def play_game_over(self):
        self.play("game_over")

    def play_freeze(self):
        self.play("freeze")

    def play_steam(self):
        self.play("steam")

    def play_toxic_gas(self):
        self.play("toxic_gas")

    def play_scorch(self):
        self.play("scorch")

    def play_acid_dilute(self):
        self.play("acid_dilute")
