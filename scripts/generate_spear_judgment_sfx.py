#!/usr/bin/env python3
"""Generate Longinus Judgment Stigma SFX.

Compact synthetic WAV: a bright golden spear-chime, a low impact, and a
short rushing trail so the charged release reads larger than the regular spear.
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


def env_ad(t, duration, attack=0.018, decay_power=1.75):
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


def judgment_release(t, duration):
    e = env_ad(t, duration, 0.02, 1.55)
    u = t / duration
    impact = math.exp(-t * 18.0) * (
        0.55 * math.sin(TAU * 58.0 * t)
        + 0.30 * math.sin(TAU * 96.0 * t + 0.4)
    )
    shimmer_env = math.exp(-t * 2.6)
    shimmer = (
        0.28 * math.sin(TAU * (880.0 + 180.0 * u) * t)
        + 0.16 * math.sin(TAU * 1320.0 * t + 0.8)
        + 0.11 * math.sin(TAU * 1760.0 * t + 1.6)
    ) * shimmer_env
    trail_gate = (1.0 - min(1.0, u)) ** 0.85
    trail = (random.random() * 2.0 - 1.0) * (0.18 + 0.20 * math.sin(TAU * 18.0 * t) ** 2) * trail_gate
    flare = math.exp(-((t - 0.11) / 0.055) ** 2) * math.sin(TAU * 620.0 * t + 3.0 * t * t)
    return (impact * 0.72 + shimmer * 0.72 + trail * 0.45 + flare * 0.24) * e


def main():
    random.seed(7777)
    OUT.mkdir(parents=True, exist_ok=True)
    write_wav("spear_judgment.wav", 1.15, judgment_release)
    print("generated spear_judgment.wav")


if __name__ == "__main__":
    main()
