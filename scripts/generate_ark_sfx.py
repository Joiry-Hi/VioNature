#!/usr/bin/env python3
"""Generate Noah's Ark movement SFX."""

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


def flood_current(seed=610, intensity=1.0):
    random.seed(seed)
    low_partials = [(random.randint(2, 12), random.uniform(0.0, TAU), random.uniform(0.06, 0.18)) for _ in range(16)]
    mid_partials = [(random.randint(24, 96), random.uniform(0.0, TAU), random.uniform(0.012, 0.038)) for _ in range(28)]
    hiss_partials = [(random.randint(160, 760), random.uniform(0.0, TAU), random.uniform(0.003, 0.011) * intensity) for _ in range(70)]
    splashes = [(random.random(), random.uniform(0.014, 0.052), random.uniform(0.025, 0.075) * intensity) for _ in range(11 + int(5 * intensity))]

    def sample(t, duration):
        u = t / duration
        surge = 0.58 + 0.42 * math.sin(TAU * (2.0 + 0.55 * intensity) * u + 0.35) ** 2
        low = sum(amp * math.sin(TAU * cycles * u + phase) for cycles, phase, amp in low_partials)
        mid = sum(amp * math.sin(TAU * cycles * u + phase) for cycles, phase, amp in mid_partials)
        hiss = sum(amp * math.sin(TAU * cycles * u + phase) for cycles, phase, amp in hiss_partials)
        hull = (0.16 + 0.05 * intensity) * math.sin(TAU * 74.0 * u + 0.4) + 0.10 * math.sin(TAU * 116.0 * u + 1.1)
        splash = 0.0
        for center, width, amp in splashes:
            d = abs(u - center)
            d = min(d, 1.0 - d)
            burst = math.exp(-(d / width) ** 2)
            splash += amp * burst * math.sin(TAU * (18.0 + center * 10.0) * u + center * TAU)
        whitewater = 0.0
        if intensity > 1.0:
            whitewater = 0.15 * math.sin(TAU * 420.0 * u + 0.2) + 0.08 * math.sin(TAU * 930.0 * u + 1.7)
        return (low * 0.95 * surge + mid * 0.85 + hiss * (0.75 + 0.45 * surge) + hull + splash + whitewater) * 0.86

    return sample


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    write_wav("ark_flood_current.wav", 2.0, flood_current(610, 1.0))
    write_wav("ark_flood_surge.wav", 2.0, flood_current(611, 1.85))
    print("generated ark_flood_current.wav, ark_flood_surge.wav")


if __name__ == "__main__":
    main()
