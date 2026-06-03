# VioNature Gameplay Guide

> ## ⚠ Important: Everything Is Tunable

> Nearly every parameter (200+) can be adjusted via **`config/gameplay.cfg`**, including:
> - Weapon damage, fire rate, projectile speed, blast radius, cooldowns
> - Enemy health, spawn intervals, boss properties
> - Player movement speed, gravity, jump height
> - Map type and dimensions
> - Game mode (Survival / Tutorial / Duel / Boss Rush)
>
> **How to modify:** Open `config/gameplay.cfg` with any text editor, tweak the numbers, save, and restart. Every key comes with an English comment explaining its purpose.
>
> > **Common issue:** If you installed from a zip archive, the file may be read-only.
> > **Fix:** Right-click the file → Properties → uncheck "Read-only" → OK.
>
> See the Configuration Reference at the end for details.

### In-Game Developer Console (Quick Tuning)

VioNature has a built-in developer console — no need to restart to tweak parameters:

1. Press **`~`** (the key below Escape, above Tab) to open the console
2. Type commands in `key = value` format and press Enter, e.g. `gravity = 15`
3. **Tab** auto-completes parameter names (from 219 keys); **↑↓** navigates completions / command history
4. When you type a full key name, the current value appears as a grey hint on the right
5. Changes take effect immediately in your current session; to make them permanent, edit `config/gameplay.cfg`

This turns the game into a real-time tuning laboratory — experiment live, then lock in your changes.

---

## Basic Controls

| Key | Action |
|-----|--------|
| **W A S D** | Move forward / left / back / right |
| **Mouse** | Aim (look around) |
| **Left Click** | Fire (hold for continuous fire) |
| **Right Click** | Alt-mode toggle / charge / hold for drone command |
| **Scroll Wheel** | Switch weapons / adjust nano-platform range / adjust blink distance |
| **Space** | Jump / fly up (with flight rig) |
| **Ctrl** | Fly down (with flight rig) |
| **Shift** | Run (accelerated movement) |
| **1–8** | Select weapons 1–8 |
| **0** | Select Mystic Staff (weapon 9) |
| **~** | Open/close developer console (real-time tuning) |
| **P** | Hide/show all HUD and weapon model |
| **F11** | Toggle fullscreen |
| **R** | Reset game |
| **Z** | Toggle Space Suit (low-gravity mode) |
| **X** | Toggle Flight Rig (hover flight) |
| **C** | Toggle Skates (ultra-low friction) |

## Weapons

VioNature features **9 weapons**, each switchable between primary and alternate modes by **tapping Right Click** (< 0.22s).

### 1. Laser Rifle (LASER)
- **Primary: Plasma Bolts** — Rapid-fire, low-damage projectiles at extreme fire rate (0.075s/shot). Sustained pressure tool.
- **Alt: Charged Beam** — Hold RMB + LMB to charge, release to fire a piercing laser. Damage scales with charge bar.
- HUD mode: blank (always primary)

### 2. Flamethrower (FLAME)
- **Primary: Fireball (F)** — Medium-range projectile whose radius expands over its lifetime. Lifetime and max size are configurable (`flame_lifetime` / `flame_max_radius`).
- **Alt: Heatwave (H)** — Close-range cone damage + heavy knockback. Can deflect enemy projectiles.
- HUD mode: F / H

### 3. Rocket Launcher (ROCKET)
- **Primary: Rocket** — Classic explosive projectile. Rocket-jump by firing at your feet.
- **Alt: Drone Canister (D)** — Launches a canister that deploys an autonomous combat drone (quadcopter). Drones strafe enemies with machine guns and fire rockets every 2s. Max drone count, lifetime, and flocking behavior are all configurable.
- **Drone Command Interface** — Hold RMB (> 0.22s) to open a tactical overlay with X-ray octahedron markers on all enemies. Left-click to set a rally point where drones assemble and hold position.
- HUD mode: D (blank for rockets)

### 4. Shotgun (SHOTGUN)
- **Primary: Pellets (P)** — Spread shot with recoil impulse for movement chaining.
- **Alt: Glass Shard Cloud (G)** — Shards decelerate via air drag and coalesce into a floating boid cloud that deals persistent damage. All cloud parameters are configurable.
- HUD mode: P / G

### 5. Gravity Nailer (NAIL)
- **Primary: Gravity Nail (N)** — On impact, creates a gravity well that continuously pulls in nearby enemies.
- **Alt: Black Hole Grenade (BH)** — Larger pull radius + stronger force + instant-kill event horizon at the center.
- HUD mode: N / BH

### 6. Infinity Gauntlet (GAUNT)
- **Right-click toggles:** TimeStop (TS) | Blink (B)
- **TS mode** — Left-click freezes all enemies and projectiles.
- **B mode** — Left-click teleports forward; scroll wheel adjusts distance logarithmically.
- HUD mode: TS / B (T when time-stop is active)

### 7. Longinus Spear (SPEAR)
- **Primary: Throw (S)** — High-speed twin-prong spear that pierces enemies and applies recoil to the player.
- **Alt: AT Thrust (T)** — Cone AoE melee dash with orange AT-field shockwave + **brief invincibility frames** (duration configurable).
- HUD mode: S / T

### 8. Nano Constructor (NANO)
- **Primary: Nano Blade Wave (B)** — Slow-moving, extremely high-damage crescent blade (1000 DPS).
- **Alt: Nano Platform (P)** — Deploys a standable temporary platform. Scroll wheel adjusts placement distance.
- HUD mode: B / P

### 9. Mystic Staff (STAFF) — Press 0 to select
- **Primary: Curse Orb (C)** — Purple homing projectile that inflicts stacking DoT. Cursed enemies release soul orbs on death in a chain reaction.
- **Alt: Arcane Shield (S)** — Deploy a spherical barrier that blocks enemies. Shatters on enemy projectile hit, releasing a purple shockwave. **Hold LMB while shield is active** to ping all Essence pickups on the map with golden pulse waves.
- **Magic Circle** — Hold both LMB + RMB simultaneously while grounded and stationary to channel for 3 seconds. Summons a pentagram circle on the ground with a floating octahedron frame. The circle absorbs specific projectile types (plasma, fireball, rockets, shotgun pellets) and auto-fires homing purple-tinted versions at nearby enemies. Deactivated by Longinus Spear hits.
- HUD mode: C / S / SA (shield active) / SC (shield cooldown)

## Enemies

| Enemy | Appearance | HP | Behavior |
|-------|-----------|-----|----------|
| **Skitter** | Red sphere | 1 | Basic melee rusher — charges the player directly |
| **Brute** | Orange large sphere | 4 | Tanky, slow movement, high HP |
| **Wisp** | Blue sphere | 1.5 | Fast, erratic wave-pattern evasion, hard to track |
| **Spitter** | Green sphere | 2.2 | Ranged attacker, keeps distance, fires aimed projectiles |
| **Pouncer** | Purple small sphere | 1.8 | Charges up then launches toward the player in a leap |
| **Harrier** | Cyan small sphere | 1.4 | Airborne hoverer with sinusoidal strafe + ranged shots |
| **Blinker** | Pink sphere | 2.1 | Telegraph → teleport to flank → high-speed dash at player |
| **Boss — Geometry Lord** | Purple large cube + spikes | 1000 | Rotating homing projectile barrages, enrages below 45% HP |
| **Boss — Slime King** | Green giant sphere | 200 | Ground movement → long/high jumps → slam shockwave. Splits into 4 offspring on death (up to 2 generations) |
| **Boss — Star of Bethlehem** | Golden cube + hexagram | 200 (configurable) | Orbits, hovers, or sits at world center depending on map. Fires a giant tracking laser (warning beam → damaging beam). **Immunities:** not counted as an enemy, immune to AoE (explosions/shockwaves/gravity wells/heatwave/nano blades), not targeted by homing projectiles — only direct projectile impacts can damage it |
| **Duelist** | Golden sphere | 100 | 1v1 mirror-match AI in Duel mode. Uses all 9 weapons, time-stop, blink, shield, and adapts tactics by range. Two AI tiers: random barrage and strategic counter |
| **Dummy** | Grey sphere | 10 | Tutorial mode only — stationary, no attacks, for damage testing |

## Pickups

| Pickup | Effect | Toggle |
|--------|--------|--------|
| **Space Suit** (blue) | Gravity reduced to 24% (0.24×) | Z |
| **Flight Rig** (cyan) | Hover flight — Space to ascend, Ctrl to descend | X |
| **Skates** (green) | Ultra-low ground friction, conserve momentum | C |
| **Essence** (rainbow star octahedron) | Extra life +1. Timed respawn on map | Auto-collect |

Suits, flight rigs, and skates spawn at game start distributed across the map. Essence pickups spawn periodically (default 45s interval).

## Essence Life System

Rainbow stella octangula pickups that appear on the map periodically, granting extra lives:

- **Display:** Solid hexagram stars below the TIME indicator (≤6 stars, >6 shows +N). Stars continuously hue-cycle over time.
- **On hit:** Consumes 1 life + brief invincibility (default 1.5s) + golden shockwave that knocks back nearby enemies and projectiles. You **do not die**.
- **Collection:** Touching an Essence pickup → +1 life, "ESSENCE +1" event text.
- **Staff detection:** In Shield mode, hold LMB to emit golden pulse waves toward all Essence locations, revealing them.
- Starting lives, invincibility duration, respawn interval, and max on map are all configurable.

## Map Types (switch via `map_type` in gameplay.cfg)

| Map | Description |
|-----|-------------|
| **circle** | Classic flat circular arena. Configurable radius. |
| **square** / **square_obstacle** | Flat square arena with 8 collision blocks + 7 low platforms + 6 high platforms. Multi-tier vertical combat. |
| **asteroid** | Spherical asteroid surface. Gravity pulls toward the core; players walk on the outer surface. Tangent velocity is conserved — great for orbital-speed strafing. |
| **hollow_world** | Hollow sphere interior. Gravity pulls outward toward the shell. Players fight on the inner surface looking inward. |

## Game Modes (switch via `game_mode` in gameplay.cfg)

### Survival
- Escalating waves with increasing enemy spawn rate.
- Scripted enemy surge events at 25s, 52s, 82s.
- Geometry Lord appears at 50s, Slime King at 100s.
- Beating bosses counts as victory, but enemies keep spawning.

### Tutorial
- No enemies by default. Training dummies available for damage testing.
- Weapon tips shown on first switch to each weapon.
- Pickup tips shown on first collection.
- Ideal for learning weapons and testing config changes.

### Duel
- 1v1 (or 1vn) against AI Duelists.
- Player has configurable armor charges + Essence extra lives.
- AI uses all 9 weapons + time-stop + blink + shield.
- `duelist_count` controls number of opponents (1–10).
- `duelist_smart_ai = true` enables strategic counter-picking AI.

### Boss Rush
- `boss_rush_mode = true` — only bosses spawn at their scheduled times. No regular enemies or events.

## Drone Command System

Hold RMB (>0.22s) with the rocket launcher to open the tactical overlay:

- All enemies marked with **3D octahedron wireframes visible through terrain** (X-ray).
- HUD shows active drone count, enemy count, range to aim point, and current mode.
- **Left-click** sets a rally point. Drones enter Assembling → Holding → Complete phases.
- Drones use **boids flocking** (separation, cohesion, alignment) to avoid clustering.

## HUD

```
TIME 12.3  SCORE 340  E 8  W SPEAR:S  G 0.24x  SUIT  FPS 60
WAVE 2                                    SLIME KING [=========     ]
DUEL: ARMOR 2
```

- **TIME** — Survival time (seconds)
- **SCORE** — Cumulative score
- **E** — Active enemy count
- **W** — Current weapon:mode
- **G** — Current gravity multiplier
- **State word** — GROUND / AIR / SUIT / FLIGHT / SKATE / STOP / GOD
- **Hexagrams** — Below TIME: filled gold stars (hue-cycling) showing current Essence count. Turn orange during invincibility frames. ≤6 stars displayed, >6 shows +N
- **WAVE / Event text** — Current wave or recent event
- **Boss/Duelist health bars** — Up to 3 displayed simultaneously, prioritized by most recently damaged

## Advanced Techniques

- **Rocket Jump** — Fire a rocket at your feet to gain extra height from the blast impulse.
- **Aerial Movement Chain** — Rocket jump → shotgun recoil → spear recoil for extreme three-stage air displacement.
- **Blade Bait** — Fire a nano blade wave, then blink to reposition, luring pursuing enemies into the blade.
- **Time-Stop Combo** — Freeze time → unload all rockets point-blank → drop a black hole → resume for instant burst.
- **Drone Crossfire** — Deploy drones at multiple positions to create crossfire. Use rally points to hold key locations.
- **Orbital Momentum** — On spherical maps (asteroid / hollow_world), tangent velocity is conserved — use it to achieve extreme orbiting speeds.
- **AT Thrust Invincibility** — Longinus Spear AT thrust grants invincibility frames. Use to crash through bullet curtains or tank boss hits.
- **Magic Circle Fortress** — Summon a magic circle in a fixed position, then feed it projectiles. The circle auto-fires homing demonized versions at enemies.
- **Shield Essence Ping** — In staff shield mode, hold LMB to reveal all Essence locations with golden pulse waves.
- **Essence Tanking** — With extra lives, deliberately eat a hit to trigger invincibility frames + shockwave, creating a window for aggression.

---

## Configuration Reference

The config file is located at `config/gameplay.cfg`. Below are the major parameter categories. For the complete list with defaults, see the file itself (all entries have English comments).

### Game Mode & Map

| Param | Description | Options |
|-------|-------------|---------|
| `game_mode` | Game mode | survival / tutorial / duel |
| `boss_rush_mode` | Boss-only mode | true / false |
| `map_type` | Map geometry | circle / square_obstacle / asteroid / hollow_world |

### Map Geometry

| Param | Description |
|-------|-------------|
| `circle_radius` | Circular arena radius |
| `asteroid_radius` | Asteroid sphere radius |
| `asteroid_player_altitude` / `_enemy_altitude` | Player/enemy height above asteroid surface |
| `hollow_world_radius` | Hollow sphere inner radius |
| `hollow_world_player_altitude` / `_enemy_altitude` | Player/enemy distance from shell |

### Player Movement

| Param | Description |
|-------|-------------|
| `gravity` | Gravity acceleration |
| `walk_speed` / `run_speed` | Walk/run speed |
| `ground_acceleration` / `air_acceleration` | Ground/air acceleration |
| `jump_speed` | Jump initial velocity |

### Weapon Parameters (key sections)

**Laser Rifle (5a):** `plasma_damage`, `plasma_speed`, `plasma_cooldown`, `laser_charge_damage`, `laser_beam_range`, etc.

**Flamethrower (5b):** `flame_damage`, `flame_lifetime`, `flame_max_radius`, `heatwave_damage`, `heatwave_force`, `heatwave_range`

**Rocket Launcher (5c):** `rocket_impact_damage`, `rocket_explosion_damage`, `rocket_explosion_radius`, `rocket_jump_impulse`

**Shotgun (5d):** `shotgun_pellet_damage`, `shotgun_pellet_count`, `glass_shard_damage`, `glass_shard_linger_time`, `glass_shard_cloud_radius`, etc.

**Gravity Nailer (5e):** `gravity_nail_damage`, `gravity_well_radius`, `gravity_well_force`, `black_hole_radius`, `black_hole_event_horizon_radius`, etc.

**Infinity Gauntlet (5f):** `blink_distance`, `blink_distance_min` / `_max`, `blink_clear_radius`, `time_stop_enabled`, `blink_enabled`

**Longinus Spear (5g):** `longinus_spear_damage`, `longinus_spear_speed`, `longinus_spear_impulse`, `longinus_spear_thrust_damage`, `longinus_spear_thrust_force`, `longinus_spear_thrust_range`, `longinus_spear_thrust_impulse`, `longinus_spear_shockwave_damage`, `longinus_spear_shockwave_force`, `longinus_spear_shockwave_radius`, `longinus_spear_thrust_invuln`

**Nano Constructor (5h):** `nano_blade_damage`, `nano_blade_lifetime`, `nano_blade_radius`, `nano_platform_range`, `nano_platform_lifetime`, etc.

**Mystic Staff (5i):** `curse_orb_direct_damage`, `curse_orb_dps`, `curse_orb_max_stack_mult`, `curse_orb_speed`, `curse_orb_turn_rate`, `soul_orb_count`, `soul_orb_damage_scale`, `mystic_staff_shield_radius`, `mystic_staff_shield_cooldown`, `mystic_staff_shockwave_radius`, `magic_circle_lifetime`, `magic_circle_radius`, `magic_circle_fire_interval`, `magic_circle_fire_rate_mult`, `magic_circle_homing_turn_rate`, etc.

### Boss Parameters

| Param | Description |
|-------|-------------|
| `boss_spawn_time` | Geometry Lord appearance time (seconds) |
| `boss_health` | Geometry Lord HP |
| `slime_king_spawn_time` | Slime King appearance time |
| `slime_king_health` / `_radius` / `_speed` | Slime King properties |
| `slime_king_long_jump_speed` / `_high_jump_speed` / `_slam_speed` | Slime King jump/slam speeds |
| `slime_king_spherical_gravity` / `_surface_damping` | Spherical map gravity & damping |
| `slime_king_split_count` / `_max_generations` | Death split count/generations |
| `bethlehem_spawn_time` / `_health` | Star of Bethlehem timing/HP |
| `bethlehem_laser_duration` / `_cooldown` / `_damage` | Star of Bethlehem laser properties |

### Drones (section 9)

| Param | Description |
|-------|-------------|
| `drone_max_count` | Max simultaneous drones |
| `drone_lifetime` | Per-drone lifetime |
| `drone_bullet_damage` / `_speed` / `_shoot_interval` | Drone machine gun properties |
| `drone_rocket_interval` / `_range` | Drone rocket timing/range |
| `drone_separation_radius` / `_force` | Boids separation |
| `drone_flocking_radius` / `_force` | Boids cohesion |
| `drone_rally_hold_time` | Rally point hold duration |

### Essence (section 10)

| Param | Description |
|-------|-------------|
| `starting_essence` | Starting extra lives (0 = none) |
| `essence_hit_invuln` | Invincibility time after losing an essence (seconds) |
| `essence_respawn_time` | Essence pickup respawn interval (seconds) |
| `essence_max_on_map` | Max essences simultaneously on map |

### Debug & Other

| Param | Description |
|-------|-------------|
| `invincible` | God mode (true = cannot die) |
| `dummy_max_count` / `_health` | Tutorial dummy count/HP |

---

> **Tip:** A Chinese annotated version of the config file is available at `config/gameplay_annotated.cfg`. Copy and rename it to `gameplay.cfg` if you prefer Chinese documentation.
