# World Layer Guide

This project now has an experimental "front/back world" system built around
wormholes.  Read this before changing movement, projectiles, enemies, map
collision, magic circles, drones, or any weapon that creates persistent objects.

## Core Idea

- `playerWorld_ == 0` means the player is in the front world.
- `playerWorld_ == 1` means the player is in the back world.
- `Enemy::world`, `Projectile::world`, `Drone::world`, `NanoBlade::world`, and
  `NanoPlatform::world` describe which physical layer that object follows.
- World layers are not separate scene copies.  Most objects still share one
  object list and one renderer.  Damage/range effects may intentionally cross
  layers unless a system explicitly checks `world`.
- Direct body contact should generally only affect the player when enemy and
  player are in the same world.

## Map Duality

Spherical maps use dual physics:

- `map_type = asteroid`
  - World 0: outside of asteroid.
  - World 1: hollow-world physics on the same sphere.
- `map_type = hollow_world`
  - World 0: inside shell.
  - World 1: asteroid physics on the same sphere.

Flat maps use mirrored flat physics:

- World 0 ground is `y = 0`.
- World 1 ground is below the front world, currently controlled by
  `kFlatBackWorldDepth` in `src/Game.cpp`.
- Use `FlatGroundYForWorld(world)` and `FlatUpForWorld(world)` instead of
  hardcoding `0`, `+Y`, `-Y`, `playerHeight_`, or `-playerHeight_`.

The flat back-world offset exists because the Jolt static floor for front-world
flat maps still occupies physical space near `y = 0`.  Moving the whole back
flat layer down avoids visual and projectile interaction with the front floor.

## Key Helpers

Use these helpers rather than open-coding world logic:

- `IsSphericalMap()`
- `IsHollowPhysicsForWorld(world)`
- `SphericalUpAt(position, world)`
- `SphericalSurfacePoint(position, altitude, world)`
- `SphericalAltitudeAt(position, world)`
- `SphericalSignedRadius(altitude, world)`
- `SphericalTouchesSurface(position, radius, world)`
- `SphericalOutOfBounds(position, padding, world)`
- `FlatGroundYForWorld(world)`
- `FlatUpForWorld(world)`
- `UpForWorldAt(position, world)`
- `TeleportThroughWormhole(position, targetWorld, altitude)`
- `ReflectVelocityThroughWormhole(velocity, targetPosition, targetWorld)`
- `WormholeCenterForWorld(portal, world)`

Default arguments on spherical helpers often fall back to `playerWorld_`.  That
is convenient for player-only code, but dangerous in enemy/projectile code.  If
you are updating an enemy, projectile, drone, nano blade, or platform, pass the
object's own `world` explicitly.

## Wormhole Lifecycle

Wormholes are stored in `wormholes_` as `WormholePortal`.

Activation:

- A black-hole grenade hitting a magic circle octahedron calls
  `ActivateWormhole(...)`.
- The original magic circle becomes permanent and purple.
- First implementation supports only one wormhole pair.
- Front and back portal centers are stored in the portal.

Traversal:

- `UpdateWormholes(dt)` handles player and enemy crossing.
- The player can cross when touching the portal in the current world.
- Enemies only cross when they are in a different world from the player; this
  prevents immediate back-and-forth jitter.
- Enemies in a different world from the player target their own world's portal.
- Enemy projectiles touching their own world's portal are destroyed.

Closing:

- Longinus Spear closes the wormhole.
- Thrown spear uses `CloseWormholeAlongSegment(...)`, not just point distance,
  because fast spears can skip through the portal in one frame.
- Spear thrust also checks the portal cone/range.
- `CloseWormhole(...)` clears the wormhole, returns the player to front world if
  needed, and annihilates back-world enemies/projectiles/nano blades/nano
  platforms/drones.

## Object Ownership And World Rules

When spawning a new object, set its `world` at creation time:

- Player projectile: `playerWorld_`
- Enemy projectile: `enemy.world`
- Drone canister result: canister projectile world
- Drone bullets/rockets: `drone.world`
- Player nano blade/platform: `playerWorld_`
- Enemy nano platform: pass `enemy.world`

Common constructors already do this in many places.  If you add a new
`projectiles_.push_back(...)`, `drones_.push_back(...)`, or `nanoPlatforms_`
entry, check its world field.

## Movement And Camera Rules

Player:

- Use `UpForWorldAt(camera_.position, playerWorld_)` for local up.
- On spherical maps, movement, jump, gravity, and camera up use the spherical
  helpers with `playerWorld_`.
- On flat maps, standing height is:
  `FlatGroundYForWorld(playerWorld_) + FlatUpForWorld(playerWorld_).y * playerHeight_`.

Enemies:

- Use `enemy.world` for all local up / surface projection.
- If enemy and player worlds differ and a wormhole exists, target
  `WormholeCenterForWorld(portal, enemy.world)` instead of the player.
- If enemy and player worlds match, target the player normally.

Flight/drone-style hover:

- Flat hover altitude should be measured relative to
  `FlatGroundYForWorld(world)`, not absolute world Y.
- Spherical hover altitude should use `SphericalAltitudeAt(position, world)`.

## Projectile Rules

Do not assume `y <= 0` means "hit ground".

For spherical maps:

- Use `SphericalTouchesSurface(position, radius, projectile.world)`.
- Surface normal is `SphericalUpAt(position, projectile.world)`.

For flat maps:

- Ground height is `FlatGroundYForWorld(projectile.world)`.
- Direction is `FlatUpForWorld(projectile.world)`.
- For back world, "falling into ground" means moving toward negative local up,
  not necessarily decreasing world Y.

Special note: flat back world uses manual gravity for gravity-affected
projectiles.  If Jolt gravity is left enabled, it always pulls in global `-Y`,
which is wrong for the mirrored back layer.

## Rendering Rules

Draw code should usually show the player's current layer clearly.

- Circle and square maps draw the ground at `FlatGroundYForWorld(playerWorld_)`.
- Grid/wire offsets should be along `FlatUpForWorld(playerWorld_)` to avoid
  z-fighting.
- Spherical map rendering is still one shared sphere; color/readability can be
  adjusted based on `map_type`, not necessarily `playerWorld_`.
- Wormholes draw both ends.  Current-world portal can be brighter; the opposite
  portal can be dimmer.

Persistent world-specific visuals such as nano platforms should use their own
stored orientation/world fields, not the player's current world, whenever
possible.

## Damage And Collision Policy

Current design intentionally keeps layers partially porous:

- Projectile/range damage may cross layers unless the feature explicitly filters
  by world.
- Direct enemy-player contact should require same world.
- Blink clear now filters to `enemy.world == playerWorld_`.
- Enemy projectiles in another world may target the wormhole instead of the
  player, depending on weapon type.

Before adding a damaging effect, decide:

- Is it a physical body contact? Usually same-world only.
- Is it an area/energy/exotic effect? Cross-layer can be acceptable.
- Does it create confusing off-screen damage? Add a world check.

## Current Known Limitations

- Only one wormhole pair is supported.
- `GravityWell` does not currently store `world`; black holes are intentionally
  allowed to be strange cross-layer effects.  If future behavior needs strict
  layer-local gravity wells, add `GravityWell::world` and update spawn/draw/damage
  logic.
- `SlimeSpawnPod` does not currently store `world`.  Slime King itself uses
  `Enemy::world`, but split pods remain a possible follow-up if Slime King is
  expected to travel through wormholes cleanly.
- There is still one Jolt world, not two independent physics scenes.  Flat back
  world is offset downward to avoid the front floor, but shared physics can still
  surprise new dynamic systems if they rely on global gravity or static geometry.
- Some rendering helpers still use player context for fallback orientation.
  Persistent object rendering should avoid `PlayerRight()` / `PlayerForward()`
  as a fallback when exact orientation matters.

## Checklist For New Features

When adding a new weapon, projectile, enemy, pickup, platform, field, or boss
attack:

1. Add a `world` field if the object persists or moves independently.
2. Set that field at spawn time from the creator:
   player uses `playerWorld_`, enemy uses `enemy.world`, drone uses `drone.world`.
3. Use world-aware up/ground helpers.
4. Avoid hardcoded `0.0f`, `1.2f`, `playerHeight_`, `-playerHeight_`, or global
   `Vector3{0, 1, 0}` unless the object is explicitly front-world only.
5. Decide whether damage/collision should cross layers.
6. If the object is in the back flat world and should fall, avoid Jolt global
   gravity or add manual gravity along `-FlatUpForWorld(world)`.
7. If the object can hit a wormhole, decide whether it is destroyed, teleported,
   activates the portal, or closes the portal.
8. If it should be annihilated when the wormhole closes, update
   `CloseWormhole(...)`.
9. Smoke test after changes:
   `cmake --build build-sandbox -j2`
   `build-sandbox/MyShooter --smoke-test`

## Good Search Targets

Useful searches before modifying related systems:

```sh
rg -n "playerWorld_|\\.world|Wormhole|FlatGroundYForWorld|SphericalUpAt|SphericalSurfacePoint" src/Game.cpp src/Game.h
rg -n "position.y|target.y|playerHeight_|Vector3\\{0.0f, 1.0f, 0.0f\\}" src/Game.cpp
rg -n "projectiles_\\.push_back|drones_\\.push_back|nanoPlatforms_\\.push_back|enemies_\\.push_back" src/Game.cpp
```

The second search is intentionally noisy.  It catches code that may still be
assuming the old single-world flat-map model.
