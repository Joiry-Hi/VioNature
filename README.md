# VioNature

A retro-styled fast-paced arena FPS with heavy Quake influences — high mobility, deep weapon mechanics, spherical maps, and boss fights. Built in C++17 on raylib + Jolt Physics.

![Genre](https://img.shields.io/badge/genre-arena%20FPS-blue)
![Style](https://img.shields.io/badge/style-retro%20pixel--art-purple)
![Platform](https://img.shields.io/badge/platform-Windows%20(x64)-lightgrey)

## Highlights

**8 weapons × 2 modes each** — every weapon has a primary fire and an alternate mode, giving 16 distinct combat tools. Weapon-switching is instant via number keys or scroll wheel.

**Mobility is your best defense.** Rocket jumping, shotgun recoil dashes, lance-throw momentum, blink teleports, flight rigs, and low-grav suits stack into air-strafe chains. The game has no health bar — every hit is lethal. Survive by out-moving everything.

**Four distinct maps** — flat arenas (circle / square with vertical platforms), a spherical asteroid surface where gravity pulls inward, and a hollow-world shell where you fight on the inside of a sphere.

**Two bosses with unique mechanics** — the Geometry Lord fires rotating ring barrages and enrages below 45% HP. The Star of Bethlehem orbits the arena like a satellite and sweeps a giant tracking laser, telegraphed by a transparent warning beam.

**Drone swarm with command interface** — the rocket launcher's alt-fire deploys combat drones that use boids flocking behavior. Long-press right-click opens a tactical overlay with X-ray enemy markers; left-click sets rally points where drones assemble and hold position.

## Weapons

| # | Weapon | Primary | Alternate |
|---|--------|---------|-----------|
| 1 | **Laser Rifle** | Rapid-fire projectiles | Charged piercing beam |
| 2 | **Flamethrower** | Expanding fireball | Close-range heatwave cone (repels projectiles) |
| 3 | **Rocket Launcher** | Explosive rockets (rocket-jump capable) | Drone canister + command interface |
| 4 | **Shotgun** | 9-pellet spread with recoil dash | Bouncing glass shards (one rebound) |
| 5 | **Gravity Nailer** | Pinned gravity well (pulls enemies) | Black hole grenade (event horizon kills) |
| 6 | **Infinity Gauntlet** | Time stop (freezes all enemies/projectiles) | Blink teleport (clears landing zone) |
| 7 | **Recoil Lance** | High-speed thrown lance (self-knockback) | Sonic thrust cone (directional boost) |
| 8 | **Rift Cutter** | Slow crescent blade wave (50 DPS) | Placeable nano-platform (temporary floor) |

Right-click toggles alt-mode; hold right-click (>0.22s) on the rocket launcher for the drone command overlay.

## Enemies

| Enemy | Behavior |
|-------|----------|
| **Skitter** | Basic melee rusher |
| **Brute** | Tanky, slow, high HP |
| **Wisp** | Erratic wave-pattern evasion, hard to track |
| **Spitter** | Keeps distance, fires aimed projectiles |
| **Pouncer** | Charges a leap that launches toward the player |
| **Harrier** | Airborne, sinusoidal strafe, ranged shots |
| **Blinker** | Telegraph → teleport to flank → high-speed dash |
| **Boss — Geometry Lord** | Purple cube with orbiting spikes. Rotating ring projectiles (12 / 18 enraged). Support spawns on arrival. |
| **Boss — Star of Bethlehem** | Golden cube-spike satellite. Orbits, hovers, or sits at world center depending on map. Fires a giant tracking laser (warning beam → damaging beam). |
| **Duelist** | Mirror-match AI (duel mode only). Uses all 8 weapons, time-stop, blink, and adapts tactics by range. |

## Maps

| Map | Description |
|-----|-------------|
| `circle` | Classic flat round arena. Configurable radius. |
| `square` / `square_obstacle` | Flat square with obstacles + multi-tier floating platforms for vertical combat. |
| `asteroid` | Spherical planetoid. Gravity points toward the core; players walk on the outer surface. Tangent velocity is conserved — great for orbital-speed strafing. |
| `hollow_world` | Hollow sphere interior. Gravity pulls outward toward the shell. Players fight on the inner surface looking inward. The Star of Bethlehem sits at the exact center. |

## Game Modes

- **Survival** — 4 escalating waves, scripted enemy surges at set times, two bosses spawn at their configured times.
- **Duel** — 1v1 against an AI Duelist. Player has configurable armor charges that absorb lethal hits.
- **Boss Rush** (`boss_rush_mode = true`) — bosses only, no regular enemies or events. Great for practice or boss-focused runs.

## Drone Command System

Holding the rocket launcher (any mode) and long-pressing right-click opens the tactical command interface:

- All enemies are marked with **3D octahedron wireframes visible through terrain** (X-ray).
- HUD shows active drone count, enemy count, range to aim point, and current mode.
- **Left-click** sets a rally point. Drones enter Assembling → Holding → Complete phases, holding position for a configurable duration before resuming normal pursuit.
- Drones use **boids flocking** (separation, cohesion, alignment) to avoid clustering.

## Tech Stack

| Layer | Library |
|-------|---------|
| Rendering & input | [raylib](https://www.raylib.com/) 5.6-dev (pixel-art pipeline: 426×240 → upscale with `TEXTURE_FILTER_POINT`) |
| Physics | [Jolt Physics](https://github.com/jrouwe/JoltPhysics) (custom C++17 wrapper) |
| Window / context | GLFW 3.4 |
| Build | CMake 3.24+, MinGW-w64 cross-compilation |
| Language | C++17 |

## Build (Windows cross-compile from Linux)

```bash
# Requires: cmake, mingw-w64
cd build-windows
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-mingw64.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DJPH_USE_DX12=OFF
cmake --build . -j $(nproc)
```

Or use the release script:

```bash
bash scripts/package-release.sh
```

This builds the project, collects `MyShooter.exe` + MinGW DLLs + assets + annotated config + gameplay guide, and packs everything into `build-windows/VioNature_Release.zip`.

## Configuration

All gameplay parameters live in `config/gameplay.cfg` (English) or `config/gameplay_annotated.cfg` (Chinese annotated). Over 80 tunable values covering:

- Map geometry, gravity, and player movement
- All weapon damage, speed, radius, and recoil values
- Enemy timing and behavior per type
- Boss spawn times, health, orbit parameters, and laser properties
- Drone counts, flocking parameters, and rally hold duration
- Duel mode armor and invulnerability frames

The release script packages the annotated Chinese config as the default `gameplay.cfg`.

Full controls and mechanics are documented in [GAMEPLAY_GUIDE.md](GAMEPLAY_GUIDE.md).

## Controls

| Input | Action |
|-------|--------|
| WASD | Move |
| Mouse | Aim |
| Left click | Fire |
| Right click (tap) | Toggle weapon alt-mode |
| Right click (hold, rocket launcher) | Drone command interface |
| Scroll wheel | Switch weapons / adjust nano-platform range |
| Space | Jump / fly up |
| Ctrl | Fly down |
| Shift | Run |
| 1–8 | Select weapon |
| Z / X / C | Toggle suit / flight / skates |
| P | Hide HUD + weapon model (screenshot mode) |
| R | Reset game |
