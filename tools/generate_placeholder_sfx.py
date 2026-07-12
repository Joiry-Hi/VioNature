#!/usr/bin/env python3
import math
import random
import struct
import wave
from pathlib import Path

SAMPLE_RATE = 44100
OUT_DIR = Path("assets/SFX")


def clamp(v, lo=-1.0, hi=1.0):
    return max(lo, min(hi, v))


def env(t, duration, attack=0.006, release=0.08):
    if t < attack:
        return t / max(attack, 1e-6)
    if t > duration - release:
        return max(0.0, (duration - t) / max(release, 1e-6))
    return 1.0


def osc(freq, t, kind="sine"):
    phase = (freq * t) % 1.0
    if kind == "square":
        return 1.0 if phase < 0.5 else -1.0
    if kind == "saw":
        return phase * 2.0 - 1.0
    if kind == "tri":
        return 1.0 - abs(phase * 4.0 - 2.0)
    return math.sin(2.0 * math.pi * freq * t)


def write_wav(name, samples):
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    path = OUT_DIR / name
    if path.exists():
        return False
    peak = max(0.001, max(abs(s) for s in samples))
    scale = 0.86 / peak
    with wave.open(str(path), "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(SAMPLE_RATE)
        frames = bytearray()
        for s in samples:
            frames.extend(struct.pack("<h", int(clamp(s * scale) * 32767)))
        wav.writeframes(frames)
    return True


def write_reversed_wav(source, name):
    path = OUT_DIR / name
    if path.exists():
        return False
    source_path = OUT_DIR / source
    if not source_path.exists():
        return False
    with wave.open(str(source_path), "rb") as wav:
        params = wav.getparams()
        frames = wav.readframes(wav.getnframes())
        frame_size = wav.getsampwidth() * wav.getnchannels()
    chunks = [frames[i:i + frame_size] for i in range(0, len(frames), frame_size)]
    with wave.open(str(path), "wb") as wav:
        wav.setparams(params)
        wav.writeframes(b"".join(reversed(chunks)))
    return True


def render(duration, fn):
    count = int(duration * SAMPLE_RATE)
    return [fn(i / SAMPLE_RATE, i) for i in range(count)]


def chirp(duration, start, end, kind="sine", amp=0.5, attack=0.005, release=0.07):
    def fn(t, _):
        p = t / duration
        freq = start * ((end / start) ** p) if start > 0 and end > 0 else start + (end - start) * p
        return osc(freq, t, kind) * amp * env(t, duration, attack, release)
    return render(duration, fn)


def noise_burst(duration, amp=0.5, tone=0.0, attack=0.002, release=0.08):
    last = 0.0
    def fn(t, _):
        nonlocal last
        n = random.uniform(-1.0, 1.0)
        last = last * tone + n * (1.0 - tone)
        return last * amp * env(t, duration, attack, release)
    return render(duration, fn)


def mix(*tracks):
    length = max(len(t) for t in tracks)
    out = [0.0] * length
    for track in tracks:
        for i, v in enumerate(track):
            out[i] += v
    return out


def delay(track, seconds):
    return [0.0] * int(seconds * SAMPLE_RATE) + track


def tone_sequence(notes, amp=0.45, kind="sine"):
    out = []
    for freq, duration in notes:
        out.extend(chirp(duration, freq, freq, kind, amp, 0.004, 0.035))
    return out


def explosion(duration=0.55):
    base = noise_burst(duration, 0.9, 0.84, 0.001, 0.25)
    boom = chirp(duration, 120.0, 34.0, "sine", 0.9, 0.001, 0.35)
    crack = delay(noise_burst(0.08, 0.8, 0.1, 0.001, 0.04), 0.015)
    return mix(base, boom, crack)


def portal(duration=0.72, reverse=False):
    a = chirp(duration, 160.0 if not reverse else 520.0, 620.0 if not reverse else 90.0, "saw", 0.36, 0.02, 0.18)
    b = chirp(duration, 330.0 if not reverse else 880.0, 920.0 if not reverse else 170.0, "tri", 0.28, 0.02, 0.2)
    n = noise_burst(duration, 0.18, 0.92, 0.02, 0.22)
    return mix(a, b, n)


def main():
    random.seed(47)
    sounds = {
        "weapon_switch.wav": tone_sequence([(360, 0.035), (540, 0.055)], 0.34, "tri"),
        "fire_control_mode.wav": tone_sequence([(420, 0.04), (300, 0.04), (620, 0.06)], 0.32, "square"),
        "rocket_explosion.wav": explosion(0.58),
        "napalm_explosion.wav": mix(
            explosion(0.48),
            delay(chirp(0.42, 180, 58, "saw", 0.34, 0.001, 0.22), 0.04),
            delay(noise_burst(0.32, 0.24, 0.78, 0.002, 0.18), 0.08),
        ),
        "gravity_well_open.wav": mix(
            chirp(0.46, 720, 120, "tri", 0.4, 0.002, 0.2),
            chirp(0.46, 180, 55, "sine", 0.36, 0.002, 0.24),
        ),
        "black_hole_open.wav": mix(
            chirp(0.82, 820, 42, "saw", 0.52, 0.004, 0.36),
            delay(noise_burst(0.5, 0.24, 0.9, 0.02, 0.3), 0.08),
        ),
        "drone_deploy.wav": mix(
            tone_sequence([(240, 0.05), (420, 0.05), (760, 0.09)], 0.34, "square"),
            delay(chirp(0.28, 1200, 2200, "tri", 0.18, 0.004, 0.08), 0.04),
        ),
        "spear_impact.wav": mix(
            chirp(0.36, 1280, 90, "tri", 0.56, 0.001, 0.18),
            delay(noise_burst(0.16, 0.32, 0.35, 0.001, 0.08), 0.02),
        ),
        "ball_lightning_explosion.wav": mix(
            chirp(0.72, 260, 1560, "sine", 0.32, 0.005, 0.26),
            chirp(0.72, 1400, 180, "tri", 0.36, 0.002, 0.3),
            delay(noise_burst(0.28, 0.2, 0.6, 0.001, 0.16), 0.05),
        ),
        "water_droplet_burst.wav": mix(
            tone_sequence([(1320, 0.05), (1760, 0.06), (960, 0.08)], 0.32, "sine"),
            delay(chirp(0.22, 2400, 720, "tri", 0.18, 0.002, 0.1), 0.025),
        ),
        "nano_water_droplet.wav": mix(
            tone_sequence([(880, 0.08), (1320, 0.09), (1760, 0.16)], 0.35, "sine"),
            delay(chirp(0.32, 2200, 680, "tri", 0.18, 0.004, 0.18), 0.04),
        ),
        "nano_command.wav": mix(
            tone_sequence([(1180, 0.035), (1560, 0.04), (920, 0.055)], 0.34, "square"),
            delay(chirp(0.16, 2200, 1380, "tri", 0.16, 0.002, 0.05), 0.018),
        ),
        "mystic_circle.wav": mix(
            chirp(0.82, 180, 520, "sine", 0.42, 0.05, 0.24),
            delay(chirp(0.55, 360, 1080, "tri", 0.22, 0.02, 0.2), 0.18),
        ),
        "essence_pickup_alt.wav": mix(
            tone_sequence([(720, 0.055), (1080, 0.07), (1620, 0.12)], 0.34, "sine"),
            delay(chirp(0.34, 520, 1880, "tri", 0.2, 0.006, 0.12), 0.035),
            delay(noise_burst(0.18, 0.08, 0.88, 0.01, 0.12), 0.02),
        ),
        "essence_consume.wav": mix(chirp(0.36, 960, 220, "tri", 0.55, 0.002, 0.14), noise_burst(0.22, 0.16, 0.7)),
        "enemy_hit.wav": mix(chirp(0.12, 220, 90, "square", 0.42, 0.001, 0.05), noise_burst(0.09, 0.28, 0.35)),
        "enemy_kill.wav": mix(chirp(0.34, 420, 85, "saw", 0.46, 0.002, 0.16), delay(noise_burst(0.18, 0.26, 0.55), 0.03)),
        "player_hit.wav": mix(chirp(0.62, 260, 36, "saw", 0.66, 0.001, 0.32), noise_burst(0.36, 0.35, 0.7)),
        "armor_hit.wav": mix(chirp(0.24, 760, 390, "square", 0.45, 0.001, 0.09), delay(chirp(0.18, 1180, 520, "tri", 0.28), 0.035)),
        "magic_circle_activate.wav": mix(chirp(0.5, 260, 960, "tri", 0.34, 0.01, 0.18), delay(chirp(0.24, 720, 1440, "sine", 0.24), 0.13)),
        "magic_circle_clear.wav": mix(chirp(0.42, 880, 180, "tri", 0.38, 0.002, 0.18), delay(noise_burst(0.12, 0.22, 0.45), 0.05)),
        "wormhole_open.wav": portal(0.86, False),
        "wormhole_travel.wav": mix(chirp(0.44, 1200, 190, "saw", 0.32, 0.002, 0.16), delay(chirp(0.26, 210, 820, "sine", 0.26), 0.08)),
        "wormhole_close.wav": mix(portal(0.74, True), delay(explosion(0.35), 0.18)),
        "boss_spawn.wav": mix(chirp(1.05, 70, 220, "saw", 0.48, 0.08, 0.28), delay(chirp(0.55, 440, 110, "square", 0.24), 0.18)),
        "boss_phase.wav": mix(tone_sequence([(180, 0.09), (240, 0.09), (320, 0.14)], 0.42, "saw"), delay(noise_burst(0.22, 0.2, 0.68), 0.08)),
        "boss_death.wav": mix(explosion(0.85), delay(chirp(0.72, 620, 60, "tri", 0.38, 0.001, 0.36), 0.08)),
        "boss_barrage.wav": mix(
            tone_sequence([(320, 0.035), (420, 0.035), (560, 0.05)], 0.36, "square"),
            delay(chirp(0.22, 900, 260, "saw", 0.22, 0.002, 0.08), 0.02),
        ),
        "slime_slam.wav": mix(
            chirp(0.46, 180, 42, "saw", 0.55, 0.001, 0.24),
            noise_burst(0.34, 0.34, 0.82, 0.001, 0.2),
            delay(chirp(0.2, 90, 38, "sine", 0.42, 0.001, 0.14), 0.04),
        ),
        "bethlehem_laser_warn.wav": mix(chirp(0.68, 440, 1320, "sine", 0.34, 0.04, 0.06), delay(chirp(0.44, 660, 1980, "tri", 0.2, 0.03, 0.05), 0.16)),
        "bethlehem_laser_fire.wav": mix(chirp(0.5, 1800, 520, "square", 0.42, 0.001, 0.14), noise_burst(0.32, 0.18, 0.88)),
    }
    created = []
    skipped = []
    for name, samples in sounds.items():
        if write_wav(name, samples):
            created.append(name)
        else:
            skipped.append(name)
    if write_reversed_wav("gauntlet_timestop.wav", "gauntlet_timestop_release.wav"):
        created.append("gauntlet_timestop_release.wav")
    else:
        skipped.append("gauntlet_timestop_release.wav")
    print("created:")
    for name in created:
        print(f"  {name}")
    if skipped:
        print("skipped existing:")
        for name in skipped:
            print(f"  {name}")


if __name__ == "__main__":
    main()
