#!/usr/bin/env python3
"""Generate dedicated War Rider SFX.

The sounds are intentionally synthetic and compact: no external dependencies,
just layered oscillators/noise written as mono 16-bit WAV files.
"""

import math
import random
import struct
import wave
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "assets" / "SFX"
SR = 44100
TAU = math.tau


def clamp(v, lo=-1.0, hi=1.0):
    return max(lo, min(hi, v))


def env_ad(t, duration, attack=0.04, decay_power=1.8):
    if t < attack:
        return t / max(attack, 1e-6)
    u = (t - attack) / max(duration - attack, 1e-6)
    return max(0.0, 1.0 - u) ** decay_power


def write_wav(name, duration, fn):
    frames = []
    n = int(SR * duration)
    for i in range(n):
        t = i / SR
        v = clamp(fn(t, duration))
        frames.append(struct.pack("<h", int(v * 32767)))
    with wave.open(str(OUT / name), "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(SR)
        wav.writeframes(b"".join(frames))


def war_horn(t, duration):
    e = env_ad(t, duration, 0.08, 1.35)
    bend = 1.0 - 0.16 * min(1.0, t / duration)
    vib = 1.0 + 0.018 * math.sin(TAU * 5.2 * t)
    f0 = 55.0 * bend * vib
    horn = (
        0.70 * math.sin(TAU * f0 * t)
        + 0.34 * math.sin(TAU * f0 * 1.5 * t + 0.6)
        + 0.22 * math.sin(TAU * f0 * 2.0 * t + 1.4)
        + 0.12 * math.sin(TAU * 176.0 * t + 0.2)
    )
    rumble = 0.24 * math.sin(TAU * 28.0 * t) * (1.0 - min(1.0, t / duration))
    noise = (random.random() * 2.0 - 1.0) * 0.055 * e
    return (horn * 0.48 + rumble + noise) * e


def command_pulse(t, duration):
    e = env_ad(t, duration, 0.035, 1.6)
    gate = 0.72 + 0.28 * math.sin(TAU * 7.0 * t) ** 2
    horn = war_horn(t, duration) * 0.86
    impact = math.exp(-t * 11.0) * (
        0.42 * math.sin(TAU * 42.0 * t)
        + 0.18 * math.sin(TAU * 91.0 * t)
    )
    metal = 0.13 * math.sin(TAU * (360.0 - 95.0 * t) * t) * math.exp(-t * 2.4)
    air = (random.random() * 2.0 - 1.0) * 0.11 * e
    return (horn * gate + impact + metal + air) * 0.82


def slash(t, duration):
    e = env_ad(t, duration, 0.015, 2.05)
    sweep = 820.0 - 610.0 * min(1.0, t / duration)
    low = 88.0 + 34.0 * min(1.0, t / duration)
    flame = (random.random() * 2.0 - 1.0) * (0.40 + 0.35 * math.sin(TAU * 37.0 * t) ** 2)
    blade = math.sin(TAU * sweep * t + 8.0 * t * t)
    body = math.sin(TAU * low * t) + 0.5 * math.sin(TAU * low * 0.5 * t)
    crack = math.exp(-t * 28.0) * math.sin(TAU * 1900.0 * t)
    return (0.34 * flame + 0.26 * blade + 0.25 * body + 0.36 * crack) * e * 0.95


def spawn(t, duration):
    e = env_ad(t, duration, 0.10, 1.15)
    horn = war_horn(t, duration) * 0.95
    rise = min(1.0, t / 0.8)
    shimmer = (
        0.13 * math.sin(TAU * (240.0 + 160.0 * rise) * t)
        + 0.08 * math.sin(TAU * (520.0 + 220.0 * rise) * t)
    ) * e
    hoof = 0.0
    for beat in (0.18, 0.38, 0.68, 0.96):
        d = max(0.0, t - beat)
        hoof += math.exp(-d * 42.0) * math.sin(TAU * 74.0 * d) * (1.0 if t >= beat else 0.0)
    return horn * 0.78 + shimmer + hoof * 0.20


def main():
    random.seed(4096)
    OUT.mkdir(parents=True, exist_ok=True)
    write_wav("war_rider_spawn.wav", 2.15, spawn)
    write_wav("war_rider_command.wav", 1.65, command_pulse)
    write_wav("war_rider_slash.wav", 0.92, slash)
    print("generated war_rider_spawn.wav, war_rider_command.wav, war_rider_slash.wav")


if __name__ == "__main__":
    main()
