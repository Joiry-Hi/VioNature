#include "Game.h"
#include "GameMath.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
constexpr float kHorsemanHitPadding = 5.0f;
constexpr float kDeathRiderHitPadding = 5.4f;

bool SegmentIntersectsAabb(Vector3 start, Vector3 end, Vector3 minBounds, Vector3 maxBounds) {
    Vector3 delta = Vector3Subtract(end, start);
    float tMin = 0.0f;
    float tMax = 1.0f;

    auto testAxis = [&](float origin, float direction, float minValue, float maxValue) {
        if (std::abs(direction) < 0.00001f) {
            return origin >= minValue && origin <= maxValue;
        }
        float inv = 1.0f / direction;
        float t1 = (minValue - origin) * inv;
        float t2 = (maxValue - origin) * inv;
        if (t1 > t2) std::swap(t1, t2);
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        return tMin <= tMax;
    };

    return testAxis(start.x, delta.x, minBounds.x, maxBounds.x)
        && testAxis(start.y, delta.y, minBounds.y, maxBounds.y)
        && testAxis(start.z, delta.z, minBounds.z, maxBounds.z);
}
}

void Game::UpdateBeam(float dt) {
    for (size_t i = 0; i < beams_.size();) {
        beams_[i].life -= dt;

        // Sustained damage beam (rainbow beam / super mode)
        if (beams_[i].damagePerFrame > 0.0f && beams_[i].life > 0.0f) {
            if (beams_[i].followPlayerMuzzle) {
                Vector3 muzzle = WeaponMuzzlePosition();
                Vector3 forward = PlayerForward();
                float beamRange = config_.laserBeamRange * 1.3f;
                beams_[i].start = muzzle;
                beams_[i].end = Vector3Add(muzzle, Vector3Scale(forward, beamRange));
            }

            float frameDamage = beams_[i].damagePerFrame * dt;
            if (ScavengerUfoDamageable()) {
                float hitDist = beams_[i].width * 0.22f + 2.6f;
                if (DistancePointToSegment(scavengerUfo_.position, beams_[i].start, beams_[i].end) <= hitDist) {
                    DamageScavengerUfo(frameDamage, scavengerUfo_.position, beams_[i].color);
                }
            }
            if (throneAngel_.active) {
                float hitDist = beams_[i].width * 0.22f + 4.2f;
                if (DistancePointToSegment(throneAngel_.position, beams_[i].start, beams_[i].end) <= hitDist) {
                    DamageThroneAngel(frameDamage, throneAngel_.position, beams_[i].color);
                }
            }
            for (const SeraphBoss& seraph : seraphs_) {
                if (!seraph.active) continue;
                float hitDist = beams_[i].width * 0.22f + 3.6f;
                if (DistancePointToSegment(seraph.position, beams_[i].start, beams_[i].end) <= hitDist) {
                    DamageSeraph(frameDamage, seraph.position, beams_[i].color);
                }
            }
            if (warRider_.active) {
                float hitDist = beams_[i].width * 0.22f + kHorsemanHitPadding;
                if (DistancePointToSegment(warRider_.position, beams_[i].start, beams_[i].end) <= hitDist) {
                    DamageWarRider(frameDamage, warRider_.position, beams_[i].color);
                }
            }
            if (conquestRider_.active) {
                float hitDist = beams_[i].width * 0.22f + kHorsemanHitPadding;
                if (DistancePointToSegment(conquestRider_.position, beams_[i].start, beams_[i].end) <= hitDist) {
                    DamageConquestRider(frameDamage, conquestRider_.position, beams_[i].color);
                }
            }
            if (famineRider_.active) {
                float hitDist = beams_[i].width * 0.22f + kHorsemanHitPadding;
                if (DistancePointToSegment(famineRider_.position, beams_[i].start, beams_[i].end) <= hitDist) {
                    DamageFamineRider(frameDamage, famineRider_.position, beams_[i].color);
                }
            }
            if (deathRider_.active) {
                float hitDist = beams_[i].width * 0.22f + kDeathRiderHitPadding;
                if (DistancePointToSegment(deathRider_.position, beams_[i].start, beams_[i].end) <= hitDist) {
                    DamageDeathRider(frameDamage, deathRider_.position, beams_[i].color);
                }
            }
            for (size_t c = 0; c < cherubs_.size();) {
                float hitDist = beams_[i].width * 0.22f + 0.55f;
                if (DistancePointToSegment(cherubs_[c].position, beams_[i].start, beams_[i].end) <= hitDist) {
                    cherubs_[c].health -= frameDamage;
                    totalDamageDealt_ += frameDamage;
                    cherubs_[c].flashTimer = 0.16f;
                    if (cherubs_[c].health <= 0.0f) {
                        SpawnHitBurst(cherubs_[c].position, Color{245, 245, 235, 255}, 18);
                        cherubs_[c] = cherubs_.back();
                        cherubs_.pop_back();
                        continue;
                    }
                }
                ++c;
            }
            for (size_t e = 0; e < enemies_.size();) {
                Vector3 enemyPos = BodyPosition(enemies_[e].body);
                float hitDist = beams_[i].width * 0.22f + enemies_[e].radius * 0.85f;
                if (DistancePointToSegment(enemyPos, beams_[i].start, beams_[i].end) <= hitDist) {
                    enemies_[e].health -= frameDamage;
                    totalDamageDealt_ += frameDamage;
                    if (enemies_[e].type == EnemyType::Dummy || enemies_[e].type == EnemyType::DummyBoss) {
                        RecordDummyDamage(enemies_[e], frameDamage);
                    }
                    if (enemies_[e].health <= 0.0f) {
                        score_ += enemies_[e].scoreValue;
                        SpawnHitBurst(enemyPos, Color{255, 255, 255, 255}, 22);
                        DestroyEnemy(e);
                        continue;
                    }
                }
                ++e;
            }
        }

        if (beams_[i].life <= 0.0f) {
            beams_[i] = beams_.back();
            beams_.pop_back();
            continue;
        }
        ++i;
    }
}
void Game::UpdateShockwaves(float dt) {
    for (size_t i = 0; i < shockwaves_.size();) {
        shockwaves_[i].life -= dt;
        if (shockwaves_[i].life <= 0.0f) {
            shockwaves_[i] = shockwaves_.back();
            shockwaves_.pop_back();
            continue;
        }
        ++i;
    }
}
void Game::UpdateHeatwaves(float dt) {
    for (size_t i = 0; i < heatwaves_.size();) {
        heatwaves_[i].life -= dt;
        if (heatwaves_[i].life <= 0.0f) {
            heatwaves_[i] = heatwaves_.back();
            heatwaves_.pop_back();
            continue;
        }
        ++i;
    }
}

void Game::UpdateFirePatches(float dt) {
    for (size_t i = 0; i < firePatches_.size();) {
        firePatches_[i].life -= dt;

        if (firePatches_[i].life > 0.0f) {
            if (firePatches_[i].infectsEnemies) {
                for (Enemy& enemy : enemies_) {
                    if (enemy.world != firePatches_[i].world) continue;
                    Vector3 ep = BodyPosition(enemy.body);
                    float dist;
                    if (IsSphericalMap()) {
                        float altDiff = SphericalAltitudeAt(ep, firePatches_[i].world)
                            - SphericalAltitudeAt(firePatches_[i].position, firePatches_[i].world);
                        if (std::abs(altDiff) > 2.0f) continue;
                        Vector3 tangent = ProjectOnSphericalTangent(
                            Vector3Subtract(ep, firePatches_[i].position),
                            firePatches_[i].up);
                        dist = Vector3Length(tangent);
                    } else {
                        if (std::abs(ep.y - firePatches_[i].position.y) > 2.0f) continue;
                        dist = DistanceXZ(ep, firePatches_[i].position);
                    }
                    if (dist <= firePatches_[i].radius + enemy.radius) {
                        enemy.plagueTimer = std::max(enemy.plagueTimer, config_.conquestRiderPlagueInfectDuration);
                    }
                }
            }

            if (firePatches_[i].hurtsEnemies) {
                float frameDamage = firePatches_[i].damagePerSecond * dt;
                for (size_t e = 0; e < enemies_.size();) {
                    if (enemies_[e].world != firePatches_[i].world) { ++e; continue; }
                    if (!firePatches_[i].sourceBody.IsInvalid() && enemies_[e].body == firePatches_[i].sourceBody) { ++e; continue; }
                    Vector3 ep = BodyPosition(enemies_[e].body);
                    float dist;
                    if (IsSphericalMap()) {
                        float altDiff = SphericalAltitudeAt(ep, firePatches_[i].world)
                            - SphericalAltitudeAt(firePatches_[i].position, firePatches_[i].world);
                        if (std::abs(altDiff) > 2.0f) { ++e; continue; }
                        Vector3 tangent = ProjectOnSphericalTangent(
                            Vector3Subtract(ep, firePatches_[i].position),
                            firePatches_[i].up);
                        dist = Vector3Length(tangent);
                    } else {
                        if (std::abs(ep.y - firePatches_[i].position.y) > 2.0f) { ++e; continue; }
                        dist = DistanceXZ(ep, firePatches_[i].position);
                    }
                    if (dist <= firePatches_[i].radius + enemies_[e].radius) {
                        enemies_[e].health -= frameDamage;
                        totalDamageDealt_ += frameDamage;
                        if (enemies_[e].type == EnemyType::Dummy || enemies_[e].type == EnemyType::DummyBoss) {
                            RecordDummyDamage(enemies_[e], frameDamage);
                        }
                        if (enemies_[e].health <= 0.0f) {
                            score_ += enemies_[e].scoreValue;
                            SpawnHitBurst(ep, Color{255, 180, 40, 255}, 15);
                            DestroyEnemy(e);
                            continue;
                        }
                    }
                    ++e;
                }
            }

            if (firePatches_[i].hurtsPlayer && firePatches_[i].world == playerWorld_) {
                float dist;
                if (IsSphericalMap()) {
                    float playerAlt = SphericalAltitudeAt(camera_.position, playerWorld_);
                    float patchAlt = SphericalAltitudeAt(firePatches_[i].position, firePatches_[i].world);
                    if (std::abs(playerAlt - patchAlt) > 2.0f) goto skipPlayerDamage;
                    Vector3 tangent = ProjectOnSphericalTangent(
                        Vector3Subtract(camera_.position, firePatches_[i].position),
                        firePatches_[i].up);
                    dist = Vector3Length(tangent);
                } else {
                    if (std::abs(camera_.position.y - firePatches_[i].position.y) > 2.0f) goto skipPlayerDamage;
                    dist = DistanceXZ(camera_.position, firePatches_[i].position);
                }
                if (dist <= firePatches_[i].radius + playerRadius_) {
                    if (firePatches_[i].plague) {
                        ApplyPlayerPlague(camera_.position);
                    } else {
                        ApplyPlayerHit(camera_.position, firePatches_[i].innerColor, "BURNT");
                    }
                }
                skipPlayerDamage: ;
            }

            // Spawn ember particles rising from the fire patch
            if (RandomFloat(0.0f, 1.0f) < 0.5f) {
                Vector3 tangent;
                if (IsSphericalMap()) {
                    tangent = ProjectOnSphericalTangent(
                        Vector3{RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f)},
                        firePatches_[i].up);
                } else {
                    tangent = Vector3{RandomFloat(-1.0f, 1.0f), 0.0f, RandomFloat(-1.0f, 1.0f)};
                }
                if (Vector3Length(tangent) > 0.001f) {
                    tangent = Vector3Scale(Vector3Normalize(tangent), RandomFloat(0.0f, firePatches_[i].radius));
                }
                Vector3 particlePos = Vector3Add(firePatches_[i].position, tangent);
                particles_.push_back(Particle{
                    particlePos,
                    Vector3Scale(firePatches_[i].up, RandomFloat(1.5f, 5.0f)),
                    firePatches_[i].particleColor,
                    RandomFloat(0.15f, 0.4f), RandomFloat(0.15f, 0.4f),
                    RandomFloat(0.03f, 0.08f)
                });
            }
        }

        if (firePatches_[i].life <= 0.0f) {
            firePatches_[i] = firePatches_.back();
            firePatches_.pop_back();
            continue;
        }
        ++i;
    }
}

void Game::IgniteEnemy(Enemy& enemy) {
    if (enemy.ignited) return;
    enemy.ignited = true;
    enemy.igniteTimer = config_.napalmIgniteDuration;
    enemy.igniteDps = config_.napalmIgniteDps;
    enemy.igniteSpreadTimer = config_.napalmSpreadInterval * 0.5f; // first spread sooner
}

void Game::UpdateNapalmGrenades(float dt) {
    for (size_t i = 0; i < napalmGrenades_.size();) {
        NapalmGrenade& g = napalmGrenades_[i];

        // Apply gravity
        float grav = config_.gravity;
        if (IsSphericalMap()) {
            Vector3 up = SphericalUpAt(g.position, g.world);
            g.velocity = Vector3Subtract(g.velocity, Vector3Scale(up, grav * dt));
        } else {
            g.velocity.y -= grav * dt;
        }

        // Move
        Vector3 prevPos = g.position;
        g.position = Vector3Add(g.position, Vector3Scale(g.velocity, dt));

        // Nano platforms are solid enough to deflect napalm canisters before they reach the ground.
        bool hitNanoPlatform = false;
        for (const NanoPlatform& platform : nanoPlatforms_) {
            if (platform.world != g.world || platform.delay > 0.0f) {
                continue;
            }

            Vector3 normal = SafeNormalize(platform.normal, UpForWorldAt(platform.position, platform.world));
            Vector3 right = SafeNormalize(ProjectOnSphericalTangent(platform.right, normal), PlayerRight());
            Vector3 forward = SafeNormalize(ProjectOnSphericalTangent(platform.forward, normal), PlayerForward());
            Vector3 center = IsSphericalMap()
                ? platform.position
                : Vector3{platform.position.x, platform.position.y + platform.scale.y * 0.5f, platform.position.z};

            Vector3 previousOffset = Vector3Subtract(prevPos, center);
            Vector3 offset = Vector3Subtract(g.position, center);
            const Vector3 axes[3] = {right, normal, forward};
            const float halfExtents[3] = {
                platform.scale.x * 0.5f + 0.22f,
                platform.scale.y * 0.5f + 0.22f,
                platform.scale.z * 0.5f + 0.22f
            };
            float p0[3] = {
                Vector3DotProduct(previousOffset, axes[0]),
                Vector3DotProduct(previousOffset, axes[1]),
                Vector3DotProduct(previousOffset, axes[2])
            };
            float p1[3] = {
                Vector3DotProduct(offset, axes[0]),
                Vector3DotProduct(offset, axes[1]),
                Vector3DotProduct(offset, axes[2])
            };

            float enterT = 0.0f;
            float exitT = 1.0f;
            Vector3 hitNormal = Vector3Zero();
            bool intersects = true;
            for (int axis = 0; axis < 3; ++axis) {
                float delta = p1[axis] - p0[axis];
                float minV = -halfExtents[axis];
                float maxV = halfExtents[axis];
                if (std::abs(delta) <= 0.0001f) {
                    if (p0[axis] < minV || p0[axis] > maxV) {
                        intersects = false;
                        break;
                    }
                    continue;
                }

                float t1 = (minV - p0[axis]) / delta;
                float t2 = (maxV - p0[axis]) / delta;
                Vector3 n1 = Vector3Scale(axes[axis], -1.0f);
                Vector3 n2 = axes[axis];
                if (t1 > t2) {
                    std::swap(t1, t2);
                    std::swap(n1, n2);
                }
                if (t1 > enterT) {
                    enterT = t1;
                    hitNormal = n1;
                }
                exitT = std::min(exitT, t2);
                if (enterT > exitT) {
                    intersects = false;
                    break;
                }
            }

            if (!intersects || enterT < 0.0f || enterT > 1.0f) {
                continue;
            }
            if (Vector3Length(hitNormal) <= 0.001f) {
                float bestSpeed = 0.0f;
                for (int axis = 0; axis < 3; ++axis) {
                    float component = Vector3DotProduct(g.velocity, axes[axis]);
                    if (std::abs(component) > bestSpeed) {
                        bestSpeed = std::abs(component);
                        hitNormal = Vector3Scale(axes[axis], component > 0.0f ? -1.0f : 1.0f);
                    }
                }
            }

            float incoming = Vector3DotProduct(g.velocity, hitNormal);
            if (incoming < 0.0f) {
                Vector3 hitPoint = Vector3Lerp(prevPos, g.position, enterT);
                g.position = Vector3Add(hitPoint, Vector3Scale(hitNormal, 0.24f));
                g.velocity = Vector3Subtract(g.velocity, Vector3Scale(hitNormal, incoming * (1.0f + config_.napalmBounceRestitution)));

                Vector3 tangent = Vector3Subtract(g.velocity, Vector3Scale(hitNormal, Vector3DotProduct(g.velocity, hitNormal)));
                tangent = Vector3Scale(tangent, RandomFloat(0.55f, 0.95f));
                g.velocity = Vector3Add(Vector3Scale(g.velocity, 0.35f), tangent);
                g.bounces++;
                hitNanoPlatform = true;
                SpawnHitBurst(g.position, Color{255, 224, 120, 255}, 8);
                break;
            }
        }

        // Ground bounce
        bool hitGround = hitNanoPlatform;
        if (!hitNanoPlatform && IsSphericalMap()) {
            float alt = SphericalAltitudeAt(g.position, g.world);
            float surfAlt = 0.02f;  // match gravity well surface (physics collision at ~proj radius)
            if (alt < surfAlt) {
                g.position = SphericalSurfacePoint(g.position, surfAlt, g.world);
                Vector3 up = SphericalUpAt(g.position, g.world);
                float inward = Vector3DotProduct(g.velocity, up);
                if (inward < 0.0f) {
                    g.velocity = Vector3Subtract(g.velocity, Vector3Scale(up, inward * (1.0f + config_.napalmBounceRestitution)));
                    // Scatter tangentially
                    Vector3 tangent = ProjectOnSphericalTangent(g.velocity, up);
                    tangent = Vector3Scale(tangent, RandomFloat(0.4f, 0.85f));
                    g.velocity = Vector3Add(Vector3Scale(g.velocity, 0.3f), tangent);
                    g.bounces++;
                    hitGround = true;
                }
            }
        } else if (!hitNanoPlatform) {
            float groundY = FlatGroundYForWorld(g.world);
            if (g.position.y < groundY) {
                g.position.y = groundY;
                g.velocity.y = std::abs(g.velocity.y) * config_.napalmBounceRestitution;
                // Random horizontal scatter on bounce
                g.velocity.x += RandomFloat(-3.0f, 3.0f);
                g.velocity.z += RandomFloat(-3.0f, 3.0f);
                g.bounces++;
                hitGround = true;
            }
        }

        // Rolling friction when on ground
        if (hitGround && Vector3Length(g.velocity) < 3.0f) {
            g.velocity = Vector3Scale(g.velocity, 0.7f);
        }

        // Contact detonation on enemies
        bool contactDetonate = false;
        for (size_t e = 0; e < enemies_.size(); ++e) {
            if (enemies_[e].world != g.world) continue;
            // Don't detonate on the duelist that fired this grenade
            if (!g.shooterBody.IsInvalid() && enemies_[e].body == g.shooterBody) continue;
            Vector3 ep = BodyPosition(enemies_[e].body);
            if (Vector3Distance(g.position, ep) <= 0.5f + enemies_[e].radius) {
                contactDetonate = true;
                break;
            }
        }
        // Contact detonation on magic circle frames (check octahedron center)
        if (!contactDetonate) {
            for (const MagicCircle& circle : magicCircles_) {
                if (circle.world != g.world) continue;
                if (circle.activatedByNapalm) continue;  // don't self-destruct on spawning circle
                Vector3 frameUp = UpForWorldAt(circle.position, circle.world);
                Vector3 octaCenter = Vector3Add(circle.position, Vector3Scale(frameUp, 1.0f + circle.radius * 0.4f));
                if (Vector3Distance(g.position, octaCenter) <= circle.radius + 1.0f) {
                    contactDetonate = true;
                    break;
                }
            }
        }
        // Contact detonation on player (duelist-fired napalm only)
        if (!contactDetonate && !g.shooterBody.IsInvalid() && g.world == playerWorld_) {
            if (Vector3Distance(g.position, camera_.position) <= 2.0f + playerRadius_) {
                contactDetonate = true;
            }
        }
        if (contactDetonate) {
            DetonateNapalm(g.position, g.world, g.shooterBody);
            napalmGrenades_[i] = napalmGrenades_.back();
            napalmGrenades_.pop_back();
            continue;
        }

        // Activate nearby magic circles as napalm (proximity)
        for (MagicCircle& circle : magicCircles_) {
            if (circle.activatedByLaserBeam) continue;
            if (circle.world != g.world) continue;
            Vector3 frameUp = UpForWorldAt(circle.position, circle.world);
            Vector3 octaCenter = Vector3Add(circle.position, Vector3Scale(frameUp, 1.0f + circle.radius * 0.4f));
            float dist = Vector3Distance(octaCenter, g.position);
            if (dist <= circle.radius + 5.0f) {
                if (!circle.activated) {
                    circle.activatedKind = ProjectileKind::Flame;
                    circle.activated = true;
                    circle.activatedByNapalm = true;
                    circle.fireInterval = 0.85f;  // match player napalm cooldown
                    circle.fireRateMult = config_.magicCircleFireRateMult;
                    circle.homingTurnRate = config_.magicCircleHomingTurnRate;
                    circle.fireCooldown = 0.0f;
                    PlaySfxAt(sfxMagicCircleActivate_, octaCenter, 70.0f, 0.9f);
                    SpawnShockwave(octaCenter, circle.radius * 1.5f, Color{255, 160, 40, 255});
                }
            }
        }

        // Fuse timer
        g.fuse -= dt;

        // Out of bounds
        bool outOfBounds = false;
        if (IsSphericalMap()) {
            outOfBounds = SphericalOutOfBounds(g.position, 6.0f, g.world);
        } else if (IsEdenMap()) {
            outOfBounds = DistanceXZ(g.position, Vector3Zero()) > EdenCombatBoundaryRadius();
        } else {
            outOfBounds = IsSquareMap()
                ? (std::abs(g.position.x) > squareHalfExtent_ + 6.0f || std::abs(g.position.z) > squareHalfExtent_ + 6.0f)
                : DistanceXZ(g.position, Vector3Zero()) > arenaRadius_ + 6.0f;
        }

        if (g.fuse <= 0.0f || outOfBounds) {
            DetonateNapalm(g.position, g.world, g.shooterBody);
            napalmGrenades_[i] = napalmGrenades_.back();
            napalmGrenades_.pop_back();
            continue;
        }

        // Bounce spark particles
        if (hitGround) {
            SpawnHitBurst(g.position, Color{255, 180, 40, 255}, 6);
        }

        ++i;
    }
}

void Game::DetonateNapalm(Vector3 position, int world, JPH::BodyID shooterBody) {
    PlaySfxAt(sfxNapalmExplosion_, position, 70.0f, 3.0f);
    float radius = config_.napalmExplosionRadius;
    float damage = config_.napalmExplosionDamage;

    // AoE damage + ignite
    for (size_t i = 0; i < enemies_.size();) {
        if (enemies_[i].world != world) { ++i; continue; }
        // Skip the duelist that fired this napalm
        if (!shooterBody.IsInvalid() && enemies_[i].body == shooterBody) { ++i; continue; }
        Vector3 ep = BodyPosition(enemies_[i].body);
        float dist = Vector3Distance(ep, position);
        if (dist <= radius + enemies_[i].radius) {
            float falloff = 1.0f - std::clamp(dist / std::max(0.001f, radius), 0.0f, 1.0f);
            enemies_[i].health -= damage * (0.3f + falloff * 0.7f);
            totalDamageDealt_ += damage * (0.3f + falloff * 0.7f);
            PlayEnemyHitSfx(ep);
            IgniteEnemy(enemies_[i]);
            SpawnHitBurst(ep, Color{255, 180, 40, 255}, 10);
            if (enemies_[i].health <= 0.0f) {
                score_ += enemies_[i].scoreValue;
                SpawnHitBurst(ep, Color{255, 245, 200, 255}, 20);
                DestroyEnemy(i);
                continue;
            }
        }
        ++i;
    }

    // Enemy-created napalm: damage player in blast radius
    if (!shooterBody.IsInvalid() && world == playerWorld_) {
        float playerDist = Vector3Distance(camera_.position, position);
        if (playerDist <= radius + playerRadius_) {
            ApplyPlayerHit(camera_.position, Color{255, 140, 20, 255}, "NAPALM");
        }
    }

    // Activate nearby magic circles (napalm = fire-based, activates as Flame)
    for (MagicCircle& circle : magicCircles_) {
        if (circle.activatedByLaserBeam) continue;
        if (circle.world != world) continue;
        float dist = Vector3Distance(circle.position, position);
        if (dist <= radius + circle.radius) {
            if (!circle.activated) {
                circle.activatedKind = ProjectileKind::Flame;
                circle.activated = true;
                circle.activatedByNapalm = true;
                circle.fireInterval = 0.85f;  // match player napalm cooldown
                circle.fireRateMult = config_.magicCircleFireRateMult;
                circle.homingTurnRate = config_.magicCircleHomingTurnRate;
                circle.fireCooldown = 0.0f;
                PlaySfxAt(sfxMagicCircleActivate_, circle.position, 70.0f, 0.9f);
                SpawnShockwave(circle.position, circle.radius * 0.8f, Color{255, 180, 60, 255});
            }
        }
    }

    // Spawn fire patch
    Vector3 patchUp;
    if (IsSphericalMap()) {
        position = SphericalSurfacePoint(position, 0.02f, world);  // match gravity well altitude
        patchUp = SphericalUpAt(position, world);
    } else {
        position.y = FlatGroundYForWorld(world);
        patchUp = FlatUpForWorld(world);
    }
    firePatches_.push_back(FirePatch{
        position, patchUp,
        config_.heatwaveFirePatchLifetime, config_.heatwaveFirePatchLifetime,
        config_.heatwaveFirePatchRadius, config_.heatwaveFirePatchDamage,
        world, shooterBody,
        !shooterBody.IsInvalid(), shooterBody.IsInvalid(),
        Color{255, 140, 30, 255},
        Color{255, 200, 60, 255},
        Color{255, 120, 20, 200}
    });

    // Visual
    SpawnHitBurst(position, Color{255, 200, 50, 255}, 80);
    SpawnHitBurst(position, Color{255, 140, 20, 255}, 40);
    SpawnShockwave(position, radius, Color{255, 160, 40, 255});
    cameraShake_ = std::min(1.0f, cameraShake_ + 0.8f);
}

void Game::UpdateGravityWells(float dt) {
    for (size_t i = 0; i < gravityWells_.size();) {
        GravityWell& well = gravityWells_[i];
        well.life -= dt;
        if (well.life <= 0.0f) {
            gravityWells_[i] = gravityWells_.back();
            gravityWells_.pop_back();
            continue;
        }

        if (well.blackHole && state_ == State::Playing) {
            Vector3 player = camera_.position;
            Vector3 up = UpForWorldAt(player, playerWorld_);
            Vector3 capsuleBottom = IsSphericalMap()
                ? Vector3Subtract(player, Vector3Scale(up, SphericalPlayerAltitude() - playerRadius_))
                : Vector3Subtract(player, Vector3Scale(up, playerHeight_ - playerRadius_));
            Vector3 capsuleTop = IsSphericalMap()
                ? Vector3Subtract(player, Vector3Scale(up, playerRadius_ * 0.35f))
                : Vector3Subtract(player, Vector3Scale(up, playerRadius_ * 0.35f));
            if (DistancePointToSegment(well.position, capsuleBottom, capsuleTop) <= config_.blackHoleEventHorizonRadius + playerRadius_ * 0.25f) {
                ApplyPlayerHit(player, Color{95, 45, 155, 255}, "EVENT HORIZON");
            }
        }

        for (size_t enemyIndex = 0; enemyIndex < enemies_.size();) {
            Enemy& enemy = enemies_[enemyIndex];
            Vector3 enemyPosition = BodyPosition(enemy.body);
            Vector3 toWell = Vector3Subtract(well.position, enemyPosition);
            float distance = Vector3Length(toWell);
            if (well.blackHole && distance <= config_.blackHoleEventHorizonRadius + enemy.radius * 0.55f) {
                score_ += enemy.scoreValue;
                SpawnHitBurst(enemyPosition, Color{40, 10, 75, 255}, 26);
                DestroyEnemy(enemyIndex);
                continue;
            }
            if (distance > 0.05f && distance <= well.radius) {
                Vector3 direction = Vector3Scale(toWell, 1.0f / distance);
                float falloff = 1.0f - distance / well.radius;
                float strength = well.force * (0.35f + falloff * 0.65f);
                enemy.health -= well.damagePerSecond * dt * (0.25f + falloff * 0.75f);
                totalDamageDealt_ += well.damagePerSecond * dt * (0.25f + falloff * 0.75f);
                if (enemy.type == EnemyType::Dummy || enemy.type == EnemyType::DummyBoss) {
                    RecordDummyDamage(enemy, well.damagePerSecond * dt * (0.25f + falloff * 0.75f));
                }
                AddEnemyImpulse(enemy, Vector3{direction.x * strength * dt, direction.y * strength * 0.45f * dt, direction.z * strength * dt});
                if (enemy.health <= 0.0f) {
                    score_ += enemy.scoreValue;
                    SpawnHitBurst(enemyPosition, well.blackHole ? Color{165, 90, 255, 255} : Color{210, 225, 255, 255}, 18);
                    DestroyEnemy(enemyIndex);
                    continue;
                }
            }
            ++enemyIndex;
        }

        // Pull player toward enemy-origin gravity wells
        if (well.enemyOrigin && !timeStopped_) {
            Vector3 playerPos = camera_.position;
            Vector3 toWell = Vector3Subtract(well.position, playerPos);
            float dist = Vector3Length(toWell);
            if (dist > 0.05f && dist <= well.radius) {
                Vector3 dir = Vector3Scale(toWell, 1.0f / dist);
                float falloff = 1.0f - dist / well.radius;
                float strength = well.force * 7.0f * (0.2f + falloff * 0.8f);
                // Project onto tangent plane on spherical maps
                if (IsSphericalMap()) {
                    Vector3 up = SphericalUpAt(playerPos, playerWorld_);
                    dir = ProjectOnSphericalTangent(dir, up);
                } else {
                    dir.y = 0.0f;
                }
                if (Vector3Length(dir) > 0.001f) {
                    dir = Vector3Normalize(dir);
                    playerVelocity_ = Vector3Add(playerVelocity_, Vector3Scale(dir, strength * dt));
                }
            }
        }

        for (Projectile& projectile : projectiles_) {
            if (projectile.kind != ProjectileKind::EnemyShot) {
                continue;
            }
            Vector3 position = BodyPosition(projectile.body);
            Vector3 toWell = Vector3Subtract(well.position, position);
            float distance = Vector3Length(toWell);
            if (distance > 0.05f && distance <= well.radius) {
                Vector3 direction = Vector3Scale(toWell, 1.0f / distance);
                float falloff = 1.0f - distance / well.radius;
                float strength = well.force * 0.9f * (0.35f + falloff * 0.65f);
                AddProjectileImpulse(projectile, Vector3{direction.x * strength * dt, direction.y * strength * dt, direction.z * strength * dt});
            }
        }

        ++i;
    }
}
void Game::UpdateNanoBlades(float dt) {
    for (size_t bladeIndex = 0; bladeIndex < nanoBlades_.size();) {
        NanoBlade& blade = nanoBlades_[bladeIndex];
        if (blade.delay > 0.0f) {
            blade.delay -= dt;
            if (blade.delay <= 0.0f) {
                SpawnHitBurst(blade.center, Color{255, 235, 150, 255}, 18);
                PlaySfxAt(sfxNanoBlade_, blade.center, 58.0f, 0.9f);
            }
            ++bladeIndex;
            continue;
        }

        blade.life -= dt;
        if (blade.life <= 0.0f) {
            nanoBlades_[bladeIndex] = nanoBlades_.back();
            nanoBlades_.pop_back();
            continue;
        }
        blade.center = Vector3Add(blade.center, Vector3Scale(blade.velocity, dt));

        bool outsideBounds = false;
        if (IsSphericalMap()) {
            outsideBounds = SphericalOutOfBounds(blade.center, blade.radius, blade.world);
        } else if (IsEdenMap()) {
            outsideBounds = DistanceXZ(blade.center, Vector3Zero()) > EdenCombatBoundaryRadius();
            outsideBounds = outsideBounds || blade.center.y < -blade.radius || blade.center.y > EdenHeightAt(0.0f, 0.0f) + 180.0f;
        } else {
            outsideBounds = IsSquareMap()
                ? (std::abs(blade.center.x) > squareHalfExtent_ + blade.radius || std::abs(blade.center.z) > squareHalfExtent_ + blade.radius)
                : (Vector3Length(Vector3{blade.center.x, 0.0f, blade.center.z}) > arenaRadius_ + blade.radius);
            outsideBounds = outsideBounds || blade.center.y < -blade.radius || blade.center.y > 48.0f;
        }
        if (outsideBounds) {
            nanoBlades_[bladeIndex] = nanoBlades_.back();
            nanoBlades_.pop_back();
            continue;
        }

        auto insideBlade = [&](Vector3 point, float radiusPadding) {
            Vector3 offset = Vector3Subtract(point, blade.center);
            float planeDistance = std::abs(Vector3DotProduct(offset, blade.normal));
            float horizontal = Vector3DotProduct(offset, blade.right);
            float vertical = Vector3DotProduct(offset, blade.up);
            float radius = std::sqrt(horizontal * horizontal + vertical * vertical);
            float normalizedVertical = vertical / std::max(0.001f, blade.radius);
            float crescentCenter = std::sqrt(std::max(0.0f, 1.0f - horizontal * horizontal / std::max(0.001f, blade.radius * blade.radius))) * blade.radius * 0.34f;
            float arcDistance = std::abs(vertical - crescentCenter);
            return planeDistance <= blade.planeThickness + radiusPadding * 0.45f
                && radius <= blade.radius + radiusPadding
                && radius >= blade.radius - blade.thickness - radiusPadding
                && normalizedVertical > -0.55f
                && arcDistance <= blade.thickness * 0.72f + radiusPadding * 0.45f;
        };

        if (blade.owner == ProjectileOwner::Enemy && insideBlade(camera_.position, playerRadius_)) {
            ApplyPlayerHit(camera_.position, Color{255, 210, 120, 255});
        }

        if (blade.owner == ProjectileOwner::Player) {
            if (ScavengerUfoDamageable() && insideBlade(scavengerUfo_.position, 2.6f)) {
                DamageScavengerUfo(blade.damagePerSecond * dt, scavengerUfo_.position, Color{255, 235, 150, 255});
            }
            for (const SeraphBoss& seraph : seraphs_) {
                if (seraph.active && blade.world == seraph.world && insideBlade(seraph.position, 3.6f)) {
                    DamageSeraph(blade.damagePerSecond * dt, seraph.position, Color{255, 230, 130, 255});
                }
            }
            if (warRider_.active && blade.world == warRider_.world && insideBlade(warRider_.position, kHorsemanHitPadding)) {
                DamageWarRider(blade.damagePerSecond * dt, warRider_.position, Color{255, 120, 70, 255});
            }
            if (conquestRider_.active && blade.world == conquestRider_.world && insideBlade(conquestRider_.position, kHorsemanHitPadding)) {
                DamageConquestRider(blade.damagePerSecond * dt, conquestRider_.position, Color{180, 255, 100, 255});
            }
            if (famineRider_.active && blade.world == famineRider_.world && insideBlade(famineRider_.position, kHorsemanHitPadding)) {
                DamageFamineRider(blade.damagePerSecond * dt, famineRider_.position, Color{210, 178, 100, 255});
            }
            if (deathRider_.active && blade.world == deathRider_.world && insideBlade(deathRider_.position, kDeathRiderHitPadding)) {
                DamageDeathRider(blade.damagePerSecond * dt, deathRider_.position, Color{170, 174, 190, 255});
            }
            for (size_t enemyIndex = 0; enemyIndex < enemies_.size();) {
                Enemy& enemy = enemies_[enemyIndex];
                Vector3 enemyPosition = BodyPosition(enemy.body);
                if (insideBlade(enemyPosition, enemy.radius)) {
                    enemy.health -= blade.damagePerSecond * dt;
                    totalDamageDealt_ += blade.damagePerSecond * dt;
                    if (enemy.type == EnemyType::Dummy || enemy.type == EnemyType::DummyBoss) {
                        RecordDummyDamage(enemy, blade.damagePerSecond * dt);
                    }
                    Vector3 offset = Vector3Subtract(enemyPosition, blade.center);
                    float horizontal = Vector3DotProduct(offset, blade.right);
                    float vertical = Vector3DotProduct(offset, blade.up);
                    Vector3 push = Vector3Add(blade.normal, Vector3Scale(Vector3Normalize(Vector3Add(Vector3Scale(blade.right, horizontal), Vector3Scale(blade.up, vertical))), 0.35f));
                    if (Vector3Length(push) > 0.001f) {
                        push = Vector3Normalize(push);
                        AddEnemyImpulse(enemy, Vector3Scale(push, 7.0f * dt));
                    }
                    if (enemy.health <= 0.0f) {
                        score_ += enemy.scoreValue;
                        SpawnHitBurst(enemyPosition, Color{255, 230, 150, 255}, 20);
                        DestroyEnemy(enemyIndex);
                        continue;
                    }
                }
                ++enemyIndex;
            }
        }

        ++bladeIndex;
    }
}
void Game::UpdateJudgmentStigmas(float dt) {
    for (size_t stigmaIndex = 0; stigmaIndex < judgmentStigmas_.size();) {
        JudgmentStigma& stigma = judgmentStigmas_[stigmaIndex];
        stigma.life -= dt;
        if (stigma.life <= 0.0f) {
            judgmentStigmas_[stigmaIndex] = judgmentStigmas_.back();
            judgmentStigmas_.pop_back();
            continue;
        }

        float frameDamage = stigma.damagePerSecond * dt;
        float radius = stigma.radius;
        Color judgmentColor{255, 224, 94, 255};
        auto inside = [&](Vector3 point, float padding) {
            return DistancePointToSegment(point, stigma.start, stigma.end) <= radius + padding;
        };
        auto closest = [&](Vector3 point) {
            Vector3 segment = Vector3Subtract(stigma.end, stigma.start);
            float lenSq = Vector3DotProduct(segment, segment);
            if (lenSq <= 0.001f) return stigma.start;
            float t = std::clamp(Vector3DotProduct(Vector3Subtract(point, stigma.start), segment) / lenSq, 0.0f, 1.0f);
            return Vector3Add(stigma.start, Vector3Scale(segment, t));
        };

        if (ScavengerUfoDamageable() && stigma.world == scavengerUfo_.world && inside(scavengerUfo_.position, 2.8f)) {
            DamageScavengerUfo(frameDamage, scavengerUfo_.position, judgmentColor);
        }
        if (throneAngel_.active && stigma.world == throneAngel_.world && inside(throneAngel_.position, 4.2f)) {
            DamageThroneAngel(frameDamage, throneAngel_.position, judgmentColor);
        }
        for (const SeraphBoss& seraph : seraphs_) {
            if (seraph.active && stigma.world == seraph.world && inside(seraph.position, 3.8f)) {
                DamageSeraph(frameDamage, seraph.position, judgmentColor);
            }
        }
        if (warRider_.active && stigma.world == warRider_.world && inside(warRider_.position, kHorsemanHitPadding)) {
            DamageWarRider(frameDamage, warRider_.position, judgmentColor);
        }
        if (conquestRider_.active && stigma.world == conquestRider_.world && inside(conquestRider_.position, kHorsemanHitPadding)) {
            DamageConquestRider(frameDamage, conquestRider_.position, judgmentColor);
        }
        if (famineRider_.active && stigma.world == famineRider_.world && inside(famineRider_.position, kHorsemanHitPadding)) {
            DamageFamineRider(frameDamage, famineRider_.position, judgmentColor);
        }
        if (deathRider_.active && stigma.world == deathRider_.world && inside(deathRider_.position, kDeathRiderHitPadding)) {
            DamageDeathRider(frameDamage, deathRider_.position, judgmentColor);
        }
        for (size_t c = 0; c < cherubs_.size();) {
            if (stigma.world == cherubs_[c].world && inside(cherubs_[c].position, 0.65f)) {
                cherubs_[c].health -= frameDamage;
                cherubs_[c].flashTimer = 0.12f;
                if (cherubs_[c].health <= 0.0f) {
                    SpawnHitBurst(cherubs_[c].position, Color{255, 238, 150, 255}, 18);
                    cherubs_[c] = cherubs_.back();
                    cherubs_.pop_back();
                    continue;
                }
            }
            ++c;
        }
        for (size_t s = 0; s < deathSkulls_.size();) {
            if (stigma.world == deathSkulls_[s].world && inside(deathSkulls_[s].position, deathSkulls_[s].radius)) {
                deathSkulls_[s].health -= frameDamage;
                if (deathSkulls_[s].health <= 0.0f) {
                    SpawnHitBurst(deathSkulls_[s].position, Color{245, 238, 190, 255}, 16);
                    deathSkulls_[s] = deathSkulls_.back();
                    deathSkulls_.pop_back();
                    continue;
                }
            }
            ++s;
        }

        for (size_t enemyIndex = 0; enemyIndex < enemies_.size();) {
            Enemy& enemy = enemies_[enemyIndex];
            if (enemy.world != stigma.world) {
                ++enemyIndex;
                continue;
            }
            Vector3 enemyPosition = BodyPosition(enemy.body);
            if (inside(enemyPosition, enemy.radius)) {
                enemy.health -= frameDamage;
                totalDamageDealt_ += frameDamage;
                if (enemy.type == EnemyType::Dummy || enemy.type == EnemyType::DummyBoss) {
                    RecordDummyDamage(enemy, frameDamage);
                }
                Vector3 push = Vector3Subtract(enemyPosition, closest(enemyPosition));
                if (Vector3Length(push) > 0.001f) {
                    AddEnemyImpulse(enemy, Vector3Scale(Vector3Normalize(push), 5.0f * dt));
                }
                if (enemy.health <= 0.0f) {
                    score_ += enemy.scoreValue;
                    SpawnHitBurst(enemyPosition, Color{255, 230, 150, 255}, 20);
                    DestroyEnemy(enemyIndex);
                    continue;
                }
            }
            ++enemyIndex;
        }

        if (RandomFloat(0.0f, 1.0f) < 0.55f) {
            float t = RandomFloat(0.0f, 1.0f);
            Vector3 center = Vector3Add(stigma.start, Vector3Scale(Vector3Subtract(stigma.end, stigma.start), t));
            Vector3 offset = Vector3Add(Vector3Scale(stigma.right, RandomFloat(-radius, radius)),
                                        Vector3Scale(stigma.up, RandomFloat(-radius * 0.45f, radius * 0.45f)));
            particles_.push_back(Particle{
                Vector3Add(center, offset),
                Vector3Scale(stigma.up, RandomFloat(0.2f, 1.6f)),
                Color{255, 224, 96, 190},
                RandomFloat(0.16f, 0.34f), RandomFloat(0.16f, 0.34f),
                RandomFloat(0.04f, 0.095f)
            });
        }

        ++stigmaIndex;
    }
}
void Game::ResetEdenGuardians() {
    edenGuardians_.clear();
    edenFireSlashes_.clear();
    if (!IsEdenMap()) return;
    for (int i = 0; i < 4; ++i) {
        float angle = static_cast<float>(i) / 4.0f * 2.0f * PI + 0.38f;
        Vector3 radial{std::cos(angle), 0.0f, std::sin(angle)};
        float radius = config_.edenMapRadius + 260.0f;
        Vector3 position = Vector3Scale(radial, radius);
        position.y = EdenGroundYAt(Vector3Scale(radial, config_.edenMapRadius * 0.98f));
        edenGuardians_.push_back(EdenGuardian{
            position,
            radial,
            RandomFloat(0.8f, config_.edenGuardianAttackInterval),
            RandomFloat(0.0f, 6.28f)
        });
    }
}

void Game::FireEdenGuardianSlash(EdenGuardian& guardian) {
    Vector3 target = camera_.position;
    target.y = EdenGroundYAt(target) + playerHeight_ * 0.65f;
    Vector3 origin = guardian.position;
    origin.y += 56.0f;
    Vector3 forward = SafeNormalize(Vector3Subtract(target, origin), Vector3Scale(guardian.radial, -1.0f));
    Vector3 right = SafeNormalize(Vector3CrossProduct(Vector3{0.0f, 1.0f, 0.0f}, forward), Vector3{1.0f, 0.0f, 0.0f});
    Vector3 up = SafeNormalize(Vector3CrossProduct(forward, right), Vector3{0.0f, 1.0f, 0.0f});

    edenFireSlashes_.push_back(EdenFireSlash{
        Vector3Add(origin, Vector3Scale(forward, 12.0f)),
        right,
        forward,
        up,
        Vector3Scale(forward, config_.edenGuardianSlashSpeed),
        config_.edenGuardianSlashLifetime,
        config_.edenGuardianSlashLifetime,
        config_.edenGuardianSlashRadius,
        config_.edenGuardianSlashThickness,
        config_.edenGuardianSlashPlaneThickness
    });
    PlaySfxAt(sfxFlamethrowerFireball_, origin, 180.0f, 0.85f);
    SpawnHitBurst(origin, Color{255, 120, 42, 255}, 22);
}

void Game::UpdateEdenGuardians(float dt) {
    if (!IsEdenMap() || !edenForbiddenFruit_.claimed) {
        edenGuardians_.clear();
        return;
    }
    if (edenGuardians_.empty()) {
        ResetEdenGuardians();
    }
    for (EdenGuardian& guardian : edenGuardians_) {
        guardian.stepPhase += dt;
        float currentRadius = DistanceXZ(guardian.position, Vector3Zero());
        float targetRadius = config_.edenMapRadius + 36.0f;
        if (currentRadius > targetRadius) {
            currentRadius = std::max(targetRadius, currentRadius - config_.edenGuardianApproachSpeed * dt);
            guardian.position = Vector3Scale(guardian.radial, currentRadius);
        }
        guardian.position.y = EdenGroundYAt(Vector3Scale(guardian.radial, config_.edenMapRadius * 0.98f));

        guardian.attackTimer -= dt;
        if (guardian.attackTimer <= 0.0f) {
            FireEdenGuardianSlash(guardian);
            guardian.attackTimer = config_.edenGuardianAttackInterval * RandomFloat(0.82f, 1.18f);
        }
    }
}

void Game::UpdateEdenFireSlashes(float dt) {
    bool hasAudibleSlash = false;
    Vector3 nearestSlash = {};
    float nearestDistSq = std::numeric_limits<float>::max();
    for (size_t i = 0; i < edenFireSlashes_.size();) {
        EdenFireSlash& slash = edenFireSlashes_[i];
        slash.life -= dt;
        if (slash.life <= 0.0f) {
            edenFireSlashes_[i] = edenFireSlashes_.back();
            edenFireSlashes_.pop_back();
            continue;
        }
        slash.center = Vector3Add(slash.center, Vector3Scale(slash.velocity, dt));
        bool outOfBounds = false;
        if (IsEdenMap()) {
            outOfBounds = DistanceXZ(slash.center, Vector3Zero()) > config_.edenMapRadius + 320.0f
                || slash.center.y < -40.0f
                || slash.center.y > EdenHeightAt(0.0f, 0.0f) + 180.0f;
        } else if (IsSphericalMap()) {
            outOfBounds = SphericalOutOfBounds(slash.center, slash.radius + 12.0f, 0);
        } else if (IsSquareMap()) {
            outOfBounds = std::abs(slash.center.x) > squareHalfExtent_ + slash.radius + 24.0f
                || std::abs(slash.center.z) > squareHalfExtent_ + slash.radius + 24.0f;
        } else {
            outOfBounds = DistanceXZ(slash.center, Vector3Zero()) > arenaRadius_ + slash.radius + 24.0f;
        }
        if (outOfBounds) {
            edenFireSlashes_[i] = edenFireSlashes_.back();
            edenFireSlashes_.pop_back();
            continue;
        }
        float distSq = Vector3DistanceSqr(slash.center, camera_.position);
        if (distSq < nearestDistSq) {
            nearestDistSq = distSq;
            nearestSlash = slash.center;
            hasAudibleSlash = true;
        }

        Vector3 offset = Vector3Subtract(camera_.position, slash.center);
        float planeDistance = std::abs(Vector3DotProduct(offset, slash.normal));
        float horizontal = Vector3DotProduct(offset, slash.right);
        float vertical = Vector3DotProduct(offset, slash.up);
        float radius = std::sqrt(horizontal * horizontal + vertical * vertical);
        float normalizedVertical = vertical / std::max(0.001f, slash.radius);
        float crescentCenter = std::sqrt(std::max(0.0f, 1.0f - horizontal * horizontal / std::max(0.001f, slash.radius * slash.radius))) * slash.radius * 0.34f;
        float arcDistance = std::abs(vertical - crescentCenter);
        bool hitPlayer = planeDistance <= slash.planeThickness + playerRadius_
            && radius <= slash.radius + playerRadius_
            && radius >= slash.radius - slash.thickness - playerRadius_
            && normalizedVertical > -0.68f
            && arcDistance <= slash.thickness * 0.85f + playerRadius_;
        if (hitPlayer) {
            const bool warSlash = slash.radius <= config_.warRiderSlashRadius + 0.5f;
            ApplyPlayerHit(camera_.position, warSlash ? Color{255, 55, 36, 255} : Color{255, 108, 42, 255},
                warSlash ? "WAR BLADE" : "FLAMING SWORD");
        }

        if (RandomFloat(0.0f, 1.0f) < 0.85f) {
            particles_.push_back(Particle{
                slash.center,
                Vector3{RandomFloat(-1.4f, 1.4f), RandomFloat(0.1f, 1.9f), RandomFloat(-1.4f, 1.4f)},
                Color{255, static_cast<unsigned char>(RandomFloat(82, 180)), 38, 210},
                RandomFloat(0.16f, 0.32f), RandomFloat(0.16f, 0.32f),
                RandomFloat(0.05f, 0.10f)
            });
        }
        ++i;
    }
    if (hasAudibleSlash) {
        UpdateLoopingSfxAt(sfxFlamethrowerFireball_, nearestSlash, 220.0f, 0.68f);
    } else {
        StopSfx(sfxFlamethrowerFireball_);
    }
}
void Game::UpdateNanoPlatforms(float dt) {
    for (size_t i = 0; i < nanoPlatforms_.size();) {
        NanoPlatform& platform = nanoPlatforms_[i];
        if (platform.delay > 0.0f) {
            platform.delay -= dt;
            if (platform.delay <= 0.0f) {
                Vector3 burstPosition = IsSphericalMap()
                    ? platform.position
                    : Vector3{platform.position.x, platform.position.y + platform.scale.y, platform.position.z};
                SpawnHitBurst(burstPosition, Color{255, 232, 150, 255}, 16);
                PlaySfxAt(sfxNanoPlatform_, burstPosition, 58.0f, 0.9f);

                Vector3 center = IsSphericalMap()
                    ? platform.position
                    : Vector3{platform.position.x, platform.position.y + platform.scale.y * 0.5f, platform.position.z};
                JPH::Vec3 halfExt(platform.scale.x * 0.5f, platform.scale.y * 0.5f, platform.scale.z * 0.5f);
                JPH::BoxShape* boxShape = new JPH::BoxShape(halfExt);

                Matrix rotMatrix = MatrixIdentity();
                rotMatrix.m0 = platform.right.x;   rotMatrix.m4 = platform.normal.x;   rotMatrix.m8 = -platform.forward.x;
                rotMatrix.m1 = platform.right.y;   rotMatrix.m5 = platform.normal.y;   rotMatrix.m9 = -platform.forward.y;
                rotMatrix.m2 = platform.right.z;   rotMatrix.m6 = platform.normal.z;   rotMatrix.m10 = -platform.forward.z;
                Quaternion rq = QuaternionFromMatrix(rotMatrix);
                JPH::Quat rotation(rq.x, rq.y, rq.z, rq.w);

                PhysicsWorld::BodyConfig platformBodyConfig;
                platformBodyConfig.motionType = JPH::EMotionType::Static;
                platformBodyConfig.layer = Layers::NON_MOVING;
                platformBodyConfig.allowSleeping = false;
                platform.platformBody = physics_.CreateBody(
                    boxShape, ToJoltVector(center), rotation,
                    platformBodyConfig, JPH::EActivation::DontActivate);
            }
            ++i;
            continue;
        }

        platform.life -= dt;
        if (platform.life <= 0.0f) {
            Vector3 burstPosition = IsSphericalMap()
                ? platform.position
                : Vector3{platform.position.x, platform.position.y + platform.scale.y, platform.position.z};
            SpawnHitBurst(burstPosition, Color{255, 210, 110, 255}, 10);
            physics_.DestroyBody(platform.platformBody);
            nanoPlatforms_[i] = nanoPlatforms_.back();
            nanoPlatforms_.pop_back();
            continue;
        }
        ++i;
    }
}
void Game::UpdateProjectiles(float dt) {
    for (size_t i = 0; i < projectiles_.size();) {
        Projectile& projectile = projectiles_[i];
        projectile.life -= dt;
        Vector3 position = BodyPosition(projectile.body);
        int projectileWorld = projectile.world;

        Vector3 projectileVelocity = projectile.frozen
            ? projectile.storedVelocity
            : ToRayVector(physics_.Bodies().GetLinearVelocity(projectile.body));
        Vector3 previousPosition = Vector3Subtract(position, Vector3Scale(projectileVelocity, dt));

        if (projectile.kind == ProjectileKind::Lance
            && projectile.owner == ProjectileOwner::Player
            && CloseWormholeAlongSegment(previousPosition, position, projectile.radius)) {
            DestroyProjectile(i);
            continue;
        }

        if (projectile.owner == ProjectileOwner::Enemy && HasWormhole() && projectileWorld != playerWorld_) {
            const WormholePortal& portal = wormholes_.front();
            Vector3 gate = WormholeCenterForWorld(portal, projectileWorld);
            if (Vector3Distance(position, gate) <= config_.wormholeTriggerRadius + projectile.radius) {
                SpawnHitBurst(position, Color{170, 70, 255, 255}, 6);
                DestroyProjectile(i);
                continue;
            }
        }

        if ((projectile.kind == ProjectileKind::Rocket
             || projectile.kind == ProjectileKind::BlackHoleGrenade
             || projectile.kind == ProjectileKind::DroneCanister)
            && (IsSphericalMap() || projectileWorld == 1)) {
            Vector3 up = IsSphericalMap() ? SphericalUpAt(position, projectileWorld) : FlatUpForWorld(projectileWorld);
            float gravityFactor = projectile.kind == ProjectileKind::BlackHoleGrenade ? 0.45f : projectile.kind == ProjectileKind::Rocket ? 0.02f : 0.65f;
            Vector3 gravity = Vector3Scale(up, -CurrentGravity() * gravityFactor * dt);
            AddProjectileImpulse(projectile, gravity);
        }

        if (projectile.kind == ProjectileKind::Flame && projectile.maxLife > 0.0f) {
            float age = 1.0f - std::clamp(projectile.life / projectile.maxLife, 0.0f, 1.0f);
            projectile.radius = projectile.maxRadius * (0.25f + age * 0.75f);
        }

        if (projectile.kind == ProjectileKind::GlassShard && config_.glassShardDrag > 0.0f) {
            JPH::Vec3 vel = physics_.Bodies().GetLinearVelocity(projectile.body);
            float speed = vel.Length();
            if (speed > 0.01f) {
                float dragFactor = 1.0f - config_.glassShardDrag * dt;
                if (dragFactor < 0.0f) dragFactor = 0.0f;
                physics_.Bodies().SetLinearVelocity(projectile.body, vel * dragFactor);
            }
        }

        if ((projectile.kind == ProjectileKind::HomingShot
             || projectile.kind == ProjectileKind::CurseOrb
             || projectile.kind == ProjectileKind::UfoOrb
             || projectile.kind == ProjectileKind::SoulOrb
             || projectile.fromMagicCircle)
            && projectile.turnRate > 0.0f && !projectile.frozen) {
            JPH::Vec3 currentVel = physics_.Bodies().GetLinearVelocity(projectile.body);
            float speed = currentVel.Length();
            if (speed > 0.5f) {
                Vector3 currentDir = Vector3Scale(ToRayVector(currentVel), 1.0f / speed);
                Vector3 target;
                if (projectile.owner == ProjectileOwner::Enemy) {
                    target = camera_.position;
                    if (HasWormhole() && projectile.world != playerWorld_) {
                        target = WormholeCenterForWorld(wormholes_.front(), projectile.world);
                    }
                } else {
                    float nearestDist = INFINITY;
                    for (const Enemy& enemy : enemies_) {
                        Vector3 ep = BodyPosition(enemy.body);
                        float d = Vector3Distance(position, ep);
                        if (d < nearestDist) { nearestDist = d; target = ep; }
                    }
                    if (nearestDist == INFINITY) { ++i; continue; }
                }
                Vector3 toTarget = Vector3Subtract(target, position);
                float distToTarget = Vector3Length(toTarget);
                if (distToTarget > 0.01f) {
                    toTarget = Vector3Scale(toTarget, 1.0f / distToTarget);
                    float dot = std::clamp(Vector3DotProduct(currentDir, toTarget), -1.0f, 1.0f);
                    float angle = std::acos(dot);
                    float maxTurn = projectile.turnRate * dt;
                    if (angle > maxTurn) {
                        Vector3 axis = Vector3CrossProduct(currentDir, toTarget);
                        float axisLen = Vector3Length(axis);
                        if (axisLen > 0.001f) {
                            axis = Vector3Scale(axis, 1.0f / axisLen);
                            Quaternion rot = QuaternionFromAxisAngle(axis, maxTurn);
                            toTarget = Vector3RotateByQuaternion(currentDir, rot);
                        }
                    }
                    physics_.Bodies().SetLinearVelocity(projectile.body, ToJoltVelocity(Vector3Scale(toTarget, speed)));
                }
            }
        }

        if (projectile.owner == ProjectileOwner::Enemy && projectile.kind != ProjectileKind::EnemyShot && projectile.kind != ProjectileKind::HomingShot && projectile.kind != ProjectileKind::UfoOrb && EnemyTouchesPlayer(position, projectile.radius)) {
            if (projectile.kind == ProjectileKind::Rocket) {
                ExplodeRocket(position, projectile.owner, projectile.shooterBody);
            } else if (projectile.kind == ProjectileKind::BlackHoleGrenade) {
                SpawnGravityWell(position, true, true);  // enemy projectile
            } else {
                ApplyPlayerHit(camera_.position, projectile.color);
            }
            DestroyProjectile(i);
            continue;
        }

        // Prop collision on flat maps (AABB block check). Labyrinth walls are
        // intentionally a hard sink for projectiles so overlapping wall seams
        // never get a chance to destabilize physics contacts.
        if (!IsSphericalMap()) {
            bool hitProp = false;
            for (const Prop& prop : props_) {
                if (!prop.collidable || prop.shape != 0) continue;
                Vector3 minBounds{
                    prop.position.x - prop.scale.x * 0.5f - projectile.radius,
                    prop.position.y - projectile.radius,
                    prop.position.z - prop.scale.z * 0.5f - projectile.radius
                };
                Vector3 maxBounds{
                    prop.position.x + prop.scale.x * 0.5f + projectile.radius,
                    prop.position.y + prop.scale.y + projectile.radius,
                    prop.position.z + prop.scale.z * 0.5f + projectile.radius
                };
                if (SegmentIntersectsAabb(previousPosition, position, minBounds, maxBounds)) {
                    hitProp = true;
                    break;
                }
            }
            if (hitProp) {
                DestroyProjectile(i);
                continue;
            }
        }

        bool detonatesOnGround = projectile.kind == ProjectileKind::Rocket || projectile.kind == ProjectileKind::GravityNail || projectile.kind == ProjectileKind::BlackHoleGrenade || projectile.kind == ProjectileKind::Lance || projectile.kind == ProjectileKind::DroneCanister || projectile.kind == ProjectileKind::UfoOrb;
        bool touchesGround = false;
        if (IsSphericalMap()) {
            touchesGround = SphericalTouchesSurface(position, projectile.radius, projectileWorld);
        } else if (IsEdenMap()) {
            touchesGround = position.y <= EdenGroundYAt(position) + projectile.radius * 0.22f;
        } else {
            float groundY = FlatGroundYForWorld(projectileWorld) + FlatUpForWorld(projectileWorld).y * 0.22f;
            touchesGround = projectileWorld == 0 ? position.y <= groundY : position.y >= groundY;
        }
        bool hitGround = IsSphericalMap() ? touchesGround : detonatesOnGround && touchesGround;
        bool outOfBounds = false;
        if (IsSphericalMap()) {
            outOfBounds = SphericalOutOfBounds(position, 6.0f, projectileWorld);
        } else if (IsEdenMap()) {
            outOfBounds = DistanceXZ(position, Vector3Zero()) > EdenCombatBoundaryRadius();
        } else if (IsLabyrinthMap()) {
            float limitX = static_cast<float>(std::max(1, labyrinthWidth_ - 1)) * config_.labyrinthCellSize * 0.5f + 6.0f;
            float limitZ = static_cast<float>(std::max(1, labyrinthHeight_ - 1)) * config_.labyrinthCellSize * 0.5f + 6.0f;
            outOfBounds = std::abs(position.x) > limitX || std::abs(position.z) > limitZ;
        } else {
            outOfBounds = IsSquareMap()
                ? (std::abs(position.x) > squareHalfExtent_ + 6.0f || std::abs(position.z) > squareHalfExtent_ + 6.0f)
                : DistanceXZ(position, Vector3Zero()) > arenaRadius_ + 6.0f;
        }
        bool expired = projectile.life <= 0.0f || outOfBounds || hitGround;

        if (IsSphericalMap() && touchesGround && projectile.kind == ProjectileKind::LaserShot) {
            Vector3 normal = SphericalUpAt(position, projectileWorld);
            Vector3 velocity = projectile.frozen ? projectile.storedVelocity : ToRayVector(physics_.Bodies().GetLinearVelocity(projectile.body));
            float inwardSpeed = Vector3DotProduct(velocity, normal);
            if (inwardSpeed < 0.0f) {
                velocity = Vector3Subtract(velocity, Vector3Scale(normal, inwardSpeed * 2.0f));
                Vector3 corrected = SphericalSurfacePoint(position, projectile.radius + 0.04f, projectileWorld);
                physics_.Bodies().SetPosition(projectile.body, ToJoltVector(corrected), JPH::EActivation::Activate);
                if (projectile.frozen) {
                    projectile.storedVelocity = velocity;
                } else {
                    physics_.Bodies().SetLinearVelocity(projectile.body, ToJoltVelocity(velocity));
                }
                SpawnHitBurst(corrected, projectile.color, 3);
            }
            expired = projectile.life <= 0.0f || outOfBounds;
        }

        if (expired && projectile.kind == ProjectileKind::GravityNail) {
            SpawnGravityWell(position, false, projectile.owner == ProjectileOwner::Enemy);
            DestroyProjectile(i);
            continue;
        }

        if (expired && projectile.kind == ProjectileKind::BlackHoleGrenade) {
            SpawnGravityWell(position, true, projectile.owner == ProjectileOwner::Enemy);
            DestroyProjectile(i);
            continue;
        }

        if (projectile.kind == ProjectileKind::EnemyShot && projectile.owner == ProjectileOwner::Enemy && EnemyTouchesPlayer(position, projectile.radius)) {
            ApplyPlayerHit(camera_.position, Color{120, 245, 255, 255});
            DestroyProjectile(i);
            continue;
        }

        if (projectile.kind == ProjectileKind::HomingShot && projectile.owner == ProjectileOwner::Enemy && EnemyTouchesPlayer(position, projectile.radius)) {
            ApplyPlayerHit(camera_.position, projectile.color);
            if (projectile.color.r >= 235 && projectile.color.g >= 230 && projectile.color.b >= 200) {
                ApplyAntigravity(config_.throneAntigravityDuration);
                eventText_ = "GRAVITY SEVERED";
                eventTextTimer_ = 1.2f;
            }
            DestroyProjectile(i);
            continue;
        }

        if (projectile.kind == ProjectileKind::UfoOrb) {
            float altitude = IsSphericalMap()
                ? SphericalAltitudeAt(position, projectileWorld)
                : std::abs(Vector3DotProduct(Vector3Subtract(position, Vector3{0.0f, FlatGroundYForWorld(projectileWorld), 0.0f}), FlatUpForWorld(projectileWorld)));
            bool oldEnough = projectile.maxLife <= 0.0f || (projectile.maxLife - projectile.life) > 0.35f;
            bool touchesPlayer = projectile.owner == ProjectileOwner::Enemy && EnemyTouchesPlayer(position, projectile.radius);
            if (touchesPlayer || expired || (oldEnough && altitude <= config_.ufoOrbAirburstAltitude)) {
                ExplodeUfoOrb(position, projectileWorld, projectile.owner == ProjectileOwner::Player, projectile.damage);
                DestroyProjectile(i);
                continue;
            }
        }

        if (expired && projectile.kind == ProjectileKind::Rocket) {
            ExplodeRocket(position, projectile.owner, projectile.shooterBody);
            DestroyProjectile(i);
            continue;
        }

        if (expired && projectile.kind == ProjectileKind::Lance) {
            if (CloseWormhole(position)) {
                DestroyProjectile(i);
                continue;
            }
            DetonateSpear(position, projectile.owner);
            DestroyProjectile(i);
            continue;
        }

        if (expired && projectile.kind == ProjectileKind::DroneCanister) {
            drones_.push_back(Drone{position, Vector3Zero(), config_.droneDeployTime, 0.0f, config_.droneRocketInterval, RandomFloat(0.0f, 6.28f), config_.droneLifetime, DroneState::Deploying, projectileWorld});
            PlaySfxAt(sfxDroneDeploy_, position, 58.0f, 0.85f);
            SpawnHitBurst(position, Color{200, 210, 220, 255}, 22);
            SpawnShockwave(position, 1.8f, Color{140, 155, 170, 255});
            DestroyProjectile(i);
            continue;
        }

        if (projectile.kind == ProjectileKind::DroneBullet && (projectile.life <= 0.0f || outOfBounds || touchesGround)) {
            DestroyProjectile(i);
            continue;
        }

        if (expired) {
            DestroyProjectile(i);
            continue;
        }

        ++i;
    }

    // Glass shard dust cloud boids (only affect slow / lingering shards)
    if (config_.glassShardCenterForce > 0.0f || config_.glassShardSeparationRadius > 0.0f) {
        constexpr float kCloudSpeedThreshold = 8.0f;
        float formTime = config_.glassShardCloudFormTime;
        // Collect shards still in the active cloud-forming phase
        int cloudCount = 0;
        Vector3 cloudCenter = {};
        for (const Projectile& p : projectiles_) {
            if (p.kind != ProjectileKind::GlassShard || p.fromMagicCircle) continue;
            float age = p.maxLife - p.life;
            if (formTime > 0.0f && age >= formTime) continue;  // frozen
            JPH::Vec3 vel = physics_.Bodies().GetLinearVelocity(p.body);
            if (vel.Length() > kCloudSpeedThreshold) continue;
            cloudCenter = Vector3Add(cloudCenter, BodyPosition(p.body));
            cloudCount++;
        }
        if (cloudCount >= 2) {
            cloudCenter = Vector3Scale(cloudCenter, 1.0f / static_cast<float>(cloudCount));
            if (!IsSphericalMap()) {
                cloudCenter.y = config_.glassShardLingerHeight;
            } else {
                cloudCenter = SphericalSurfacePoint(cloudCenter, config_.glassShardLingerHeight);
            }
            float cloudR = config_.glassShardCloudRadius;
            float sepR = config_.glassShardSeparationRadius;
            float centerF = config_.glassShardCenterForce;
            for (Projectile& p : projectiles_) {
                if (p.kind != ProjectileKind::GlassShard || p.fromMagicCircle) continue;
                float age = p.maxLife - p.life;
                if (formTime > 0.0f && age >= formTime) continue;
                JPH::Vec3 vel = physics_.Bodies().GetLinearVelocity(p.body);
                if (vel.Length() > kCloudSpeedThreshold) continue;
                Vector3 pos = BodyPosition(p.body);
                Vector3 force = {};
                Vector3 toCenter = Vector3Subtract(cloudCenter, pos);
                float distToCenter = Vector3Length(toCenter);
                if (distToCenter > cloudR && distToCenter > 0.001f) {
                    force = Vector3Add(force, Vector3Scale(Vector3Normalize(toCenter), centerF * std::min((distToCenter - cloudR) / cloudR, 2.0f)));
                }
                if (sepR > 0.0f) {
                    for (const Projectile& other : projectiles_) {
                        if (&other == &p) continue;
                        if (other.kind != ProjectileKind::GlassShard || other.fromMagicCircle) continue;
                        float otherAge = other.maxLife - other.life;
                        if (formTime > 0.0f && otherAge >= formTime) continue;
                        JPH::Vec3 otherVel = physics_.Bodies().GetLinearVelocity(other.body);
                        if (otherVel.Length() > kCloudSpeedThreshold) continue;
                        Vector3 otherPos = BodyPosition(other.body);
                        Vector3 away = Vector3Subtract(pos, otherPos);
                        float dist = Vector3Length(away);
                        if (dist < sepR && dist > 0.001f) {
                            force = Vector3Add(force, Vector3Scale(Vector3Normalize(away), centerF * 0.8f * (1.0f - dist / sepR)));
                        }
                    }
                }
                if (Vector3Length(force) > 0.001f) {
                    JPH::Vec3 newVel = vel + ToJoltVelocity(Vector3Scale(force, dt));
                    float speed = newVel.Length();
                    if (speed > 4.0f) newVel = newVel / speed * 4.0f;
                    physics_.Bodies().SetLinearVelocity(p.body, newVel);
                } else if (age >= formTime * 0.5f) {
                    // Freeze shards that have been forming long enough and are stable
                    physics_.Bodies().SetLinearVelocity(p.body, JPH::Vec3::sZero());
                }
            }
        }
    }
}
void Game::UpdateSlimeSpawnPods(float dt) {
    for (size_t i = 0; i < slimeSpawnPods_.size();) {
        SlimeSpawnPod& pod = slimeSpawnPods_[i];
        // Apply gravity (world-space Y on flat, radial on spherical)
        if (IsSphericalMap()) {
            Vector3 podUp = SphericalUpAt(pod.position);
            float alt = SphericalAltitudeAt(pod.position);
            float targetAlt = SphericalEnemyAltitude(EnemyType::SlimeKing);
            float pull = (targetAlt - alt) * 10.0f;
            pod.velocity = Vector3Add(pod.velocity, Vector3Scale(podUp, pull * dt));
        } else {
            pod.velocity.y -= CurrentGravity() * 0.65f * dt;
        }
        // Move
        pod.position = Vector3Add(pod.position, Vector3Scale(pod.velocity, dt));
        // Ground landing
        bool landed = false;
        if (IsSphericalMap()) {
            float alt = SphericalAltitudeAt(pod.position);
            if (alt <= SphericalEnemyAltitude(EnemyType::SlimeKing) + 0.2f) {
                pod.position = SphericalSurfacePoint(pod.position, SphericalEnemyAltitude(EnemyType::SlimeKing));
                pod.velocity = Vector3Scale(pod.velocity, 0.3f);
                landed = true;
            }
        } else {
            if (pod.position.y <= 0.5f) {
                pod.position.y = 0.5f;
                pod.velocity = Vector3Scale(pod.velocity, 0.3f);
                landed = true;
            }
        }
        // Countdown (faster after landing)
        pod.timer -= dt * (landed ? 1.0f : 0.35f);
        if (pod.timer <= 0.0f) {
            // Spawn child slime
            Vector3 spawnPos = IsSphericalMap()
                ? SphericalSurfacePoint(pod.position, SphericalEnemyAltitude(EnemyType::SlimeKing))
                : Vector3{pod.position.x, 1.5f, pod.position.z};
            PhysicsWorld::BodyConfig childConfig;
            childConfig.motionType = JPH::EMotionType::Dynamic;
            childConfig.layer = Layers::MOVING;
            childConfig.gravityFactor = !IsSphericalMap() ? 0.75f : 0.0f;
            childConfig.linearDamping = 0.0f;
            childConfig.friction = 0.0f;
            childConfig.allowSleeping = false;
            JPH::BodyID childBody = physics_.CreateBody(enemyShape_, ToJoltVector(spawnPos), JPH::Quat::sIdentity(), childConfig);
            enemies_.push_back(Enemy{childBody, EnemyType::SlimeKing, pod.radius, config_.slimeKingSpeed,
                pod.health, pod.health, RandomFloat(0.0f, 6.28f), 0.0f, RandomFloat(0.8f, 1.4f),
                0.0f, 0, 0, 0.0f, 0.0f, 100, Color{100, 220, 140, 255},
                Vector3Zero(), Vector3Zero(), false, pod.generation});
            SpawnHitBurst(spawnPos, Color{80, 210, 120, 255}, 12);
            slimeSpawnPods_[i] = slimeSpawnPods_.back();
            slimeSpawnPods_.pop_back();
            continue;
        }
        ++i;
    }
}
void Game::UpdateMagicCircles(float dt) {
    for (size_t i = 0; i < magicCircles_.size();) {
        MagicCircle& circle = magicCircles_[i];
        if (!circle.isWormhole) {
            circle.life -= dt;
            if (circle.life <= 0.0f) {
                SpawnShockwave(circle.position, circle.radius, Color{180, 130, 255, 240});
                SpawnHitBurst(circle.position, Color{200, 160, 255, 255}, 20);
                magicCircles_[i] = magicCircles_.back();
                magicCircles_.pop_back();
                continue;
            }
        } else {
            circle.life = circle.maxLife;
        }
        circle.fireCooldown -= dt;
        if (!circle.isWormhole && circle.activated
            && circle.fireCooldown <= 0.0f && !enemies_.empty()) {
            float baseCooldown = MagicCircleBaseCooldown(circle);
            circle.fireCooldown = baseCooldown / std::max(0.1f, circle.fireRateMult);
            float nearestDist = INFINITY;
            Vector3 nearestPos = {};
            for (const Enemy& enemy : enemies_) {
                if (enemy.world != circle.world) continue;
                Vector3 ep = BodyPosition(enemy.body);
                float d = Vector3Distance(circle.position, ep);
                if (d < nearestDist) { nearestDist = d; nearestPos = ep; }
            }
            if (nearestDist < INFINITY) {
                Vector3 up = UpForWorldAt(circle.position, circle.world);
                Vector3 octaCenter = Vector3Add(circle.position, Vector3Scale(up, 1.0f + circle.radius * 0.4f));
                Vector3 dir = Vector3Subtract(nearestPos, octaCenter);
                if (Vector3Length(dir) > 0.01f) dir = Vector3Normalize(dir);
                if (circle.activatedByLaserBeam) {
                    FireMagicLaserBeam(octaCenter, dir, circle.world);
                    continue;
                }
                if (circle.activatedByNapalm) {
                    // Launch horizontally toward enemy, slight upward toss
                    Vector3 horizDir = dir;
                    if (IsSphericalMap()) {
                        Vector3 surfaceUp = SphericalUpAt(octaCenter, circle.world);
                        horizDir = ProjectOnSphericalTangent(dir, surfaceUp);
                    } else {
                        horizDir.y = 0.0f;
                    }
                    if (Vector3Length(horizDir) > 0.01f) horizDir = Vector3Normalize(horizDir);
                    Vector3 grenadeVel = Vector3Scale(horizDir, config_.napalmSpeed * 0.8f);
                    grenadeVel = Vector3Add(grenadeVel, Vector3Scale(up, 2.0f));
                    napalmGrenades_.push_back(NapalmGrenade{
                        octaCenter, grenadeVel,
                        config_.napalmFuse, config_.napalmFuse, 0, circle.world,
                        JPH::BodyID()  // magic-circle fired
                    });
                    continue;
                }
                Vector3 right = SafeNormalize(Vector3CrossProduct(dir, up), Vector3{1.0f, 0.0f, 0.0f});

                float speed = config_.plasmaSpeed;
                float damage = config_.plasmaDamage;
                float life = config_.plasmaLifetime;
                float radius = config_.plasmaRadius;
                float maxR = config_.plasmaRadius;
                switch (circle.activatedKind) {
                    case ProjectileKind::LaserShot:
                        speed = config_.plasmaSpeed;
                        damage = config_.plasmaDamage;
                        life = config_.plasmaLifetime;
                        radius = config_.plasmaRadius;
                        maxR = config_.plasmaRadius;
                        break;
                    case ProjectileKind::Flame:
                        speed = RandomFloat(19.0f, 23.0f);
                        damage = config_.flameDamage;
                        life = config_.flameLifetime;
                        radius = 0.12f;
                        maxR = config_.flameMaxRadius;
                        break;
                    case ProjectileKind::Rocket:
                        speed = 34.0f;
                        damage = config_.rocketImpactDamage;
                        life = 2.8f;
                        radius = 0.34f;
                        maxR = 0.34f;
                        break;
                    case ProjectileKind::Pellet:
                        speed = RandomFloat(48.0f, 58.0f);
                        damage = config_.shotgunPelletDamage;
                        life = 0.62f;
                        radius = 0.11f;
                        maxR = 0.11f;
                        break;
                    case ProjectileKind::GlassShard:
                        speed = config_.glassShardSpeed * 0.7f;
                        damage = config_.glassShardDamage;
                        life = 4.0f;
                        radius = 0.13f;
                        maxR = 0.13f;
                        break;
                    default: continue;
                }
                int shotCount = 1;
                if (circle.activatedKind == ProjectileKind::Pellet) shotCount = config_.shotgunPelletCount;
                else if (circle.activatedKind == ProjectileKind::GlassShard) shotCount = config_.shotgunShardCount;
                for (int shot = 0; shot < shotCount; ++shot) {
                    Vector3 shotDir = dir;
                    if (circle.activatedKind == ProjectileKind::Pellet) {
                        float side = RandomFloat(-0.18f, 0.18f);
                        float lift = RandomFloat(-0.12f, 0.12f);
                        shotDir = Vector3Normalize(Vector3Add(dir, Vector3Add(Vector3Scale(right, side), Vector3Scale(up, lift))));
                        speed = RandomFloat(48.0f, 58.0f);
                    } else if (circle.activatedKind == ProjectileKind::GlassShard) {
                        float side = RandomFloat(-0.15f, 0.15f);
                        float lift = RandomFloat(-0.10f, 0.10f);
                        shotDir = Vector3Normalize(Vector3Add(dir, Vector3Add(Vector3Scale(right, side), Vector3Scale(up, lift))));
                        speed = config_.glassShardSpeed * RandomFloat(0.85f, 1.15f);
                    } else if (circle.activatedKind == ProjectileKind::Flame) {
                        speed = RandomFloat(19.0f, 23.0f);
                    }
                    FireMagicProjectile(circle.activatedKind, octaCenter, shotDir, speed, damage, life, radius, maxR,
                        MagicCircleTint(circle), circle.homingTurnRate, circle.world);
                }
            }
        }
        ++i;
    }
}
void Game::UpdateParticles(float dt) {
    for (size_t i = 0; i < particles_.size();) {
        Particle& particle = particles_[i];
        particle.life -= dt;
        particle.velocity.y -= 6.0f * dt;
        particle.position = Vector3Add(particle.position, Vector3Scale(particle.velocity, dt));

        if (particle.life <= 0.0f) {
            particles_[i] = particles_.back();
            particles_.pop_back();
            continue;
        }

        ++i;
    }
}
void Game::UpdateDrones(float dt) {
    const int droneCount = static_cast<int>(drones_.size());

    // ── Collect active drone positions for flocking ──────────────────
    struct DroneData {
        Vector3 position;
        Vector3 velocity;
    };
    std::vector<DroneData> activeData(droneCount);
    std::vector<int> activeIndices;
    for (int i = 0; i < droneCount; ++i) {
        if (drones_[i].state == DroneState::Active) {
            activeData[i] = {drones_[i].position, drones_[i].velocity};
            activeIndices.push_back(i);
        }
    }
    const int activeCount = static_cast<int>(activeIndices.size());

    // ── Flocking forces (boids) ──────────────────────────────────────
    // Pre-compute per-drone flocking acceleration
    std::vector<Vector3> flockAccel(droneCount, Vector3Zero());

    for (int ai = 0; ai < activeCount; ++ai) {
        int i = activeIndices[ai];
        Vector3 sep = Vector3Zero();
        Vector3 coh = Vector3Zero();
        Vector3 ali = Vector3Zero();
        int sepCount = 0, cohCount = 0, aliCount = 0;

        for (int aj = 0; aj < activeCount; ++aj) {
            if (aj == ai) continue;
            int j = activeIndices[aj];
            if (drones_[j].world != drones_[i].world) continue;
            Vector3 toOther = Vector3Subtract(drones_[j].position, drones_[i].position);
            float dist = Vector3Length(toOther);

            // Separation: push away from nearby drones
            if (dist < config_.droneSeparationRadius && dist > 0.001f) {
                Vector3 pushDir = Vector3Scale(Vector3Normalize(toOther), -1.0f);
                float weight = 1.0f - (dist / config_.droneSeparationRadius);  // stronger at close range
                sep = Vector3Add(sep, Vector3Scale(pushDir, weight));
                sepCount++;
            }

            // Cohesion + Alignment: only if within flocking radius
            if (dist < config_.droneFlockingRadius) {
                coh = Vector3Add(coh, drones_[j].position);
                ali = Vector3Add(ali, drones_[j].velocity);
                cohCount++;
                aliCount++;
            }
        }

        Vector3 force = Vector3Zero();

        // Separation
        if (sepCount > 0) {
            force = Vector3Add(force, Vector3Scale(sep, config_.droneSeparationForce));
        }

        // Cohesion: steer toward rally point (if active) or average position of flockmates
        if ((rallyPhase_ == RallyPhase::Assembling || rallyPhase_ == RallyPhase::Holding)
            && drones_[i].world == playerWorld_) {
            Vector3 toRally = Vector3Subtract(rallyPoint_, drones_[i].position);
            float rallyDist = Vector3Length(toRally);
            if (rallyDist > 0.001f) {
                force = Vector3Add(force,
                    Vector3Scale(Vector3Normalize(toRally), config_.droneFlockingForce * 2.0f * std::min(rallyDist / config_.droneFlockingRadius, 1.0f)));
            }
        } else if (cohCount > 0) {
            Vector3 avgPos = Vector3Scale(coh, 1.0f / static_cast<float>(cohCount));
            Vector3 toCenter = Vector3Subtract(avgPos, drones_[i].position);
            float toCenterLen = Vector3Length(toCenter);
            if (toCenterLen > 0.001f) {
                force = Vector3Add(force,
                    Vector3Scale(Vector3Normalize(toCenter), config_.droneFlockingForce * std::min(toCenterLen / config_.droneFlockingRadius, 1.0f)));
            }
        }

        // Alignment: match velocity of flockmates
        if (aliCount > 0) {
            Vector3 avgVel = Vector3Scale(ali, 1.0f / static_cast<float>(aliCount));
            Vector3 velDiff = Vector3Subtract(avgVel, drones_[i].velocity);
            force = Vector3Add(force, Vector3Scale(velDiff, config_.droneFlockingForce * 0.5f));
        }

        // Project force to horizontal plane (or tangent on spherical maps)
        Vector3 up = UpForWorldAt(drones_[i].position, drones_[i].world);
        Vector3 horizForce = IsSphericalMap()
            ? ProjectOnSphericalTangent(force, up)
            : Vector3{force.x, 0.0f, force.z};
        flockAccel[i] = horizForce;
    }

    // ── Update each drone ────────────────────────────────────────────
    for (size_t i = 0; i < drones_.size();) {
        Drone& drone = drones_[i];
        drone.life -= dt;

        if (drone.life <= 0.0f) {
            SpawnHitBurst(drone.position, Color{160, 170, 185, 255}, 22);
            drones_[i] = drones_.back();
            drones_.pop_back();
            continue;
        }

        if (drone.state == DroneState::Deploying) {
            drone.deployTimer -= dt;
            if (drone.deployTimer <= 0.0f) {
                drone.state = DroneState::Active;
                SpawnHitBurst(drone.position, Color{200, 220, 240, 255}, 20);
                SpawnShockwave(drone.position, 2.0f, Color{170, 185, 200, 255});
            }
            ++i;
            continue;
        }

        // Active state
        drone.bobTimer += dt;
        Vector3 up = UpForWorldAt(drone.position, drone.world);

        // Find nearest enemy
        Vector3 targetPos = drone.position;
        float nearestDist = 1000.0f;
        bool hasTarget = false;
        for (const Enemy& enemy : enemies_) {
            if (enemy.world != drone.world) continue;
            Vector3 enemyPos = BodyPosition(enemy.body);
            float dist = Vector3Distance(drone.position, enemyPos);
            if (dist < nearestDist) {
                nearestDist = dist;
                targetPos = enemyPos;
                hasTarget = true;
            }
        }

        // Build desired horizontal velocity (rally-aware)
        Vector3 desiredVel = {};

        if (rallyPhase_ == RallyPhase::Assembling && drone.world == playerWorld_) {
            Vector3 toRally = Vector3Subtract(rallyPoint_, drone.position);
            Vector3 rallyDir = IsSphericalMap()
                ? ProjectOnSphericalTangent(toRally, up)
                : Vector3{toRally.x, 0.0f, toRally.z};
            float rallyDist = Vector3Length(rallyDir);
            if (rallyDist > 0.3f) {
                rallyDir = Vector3Normalize(rallyDir);
                desiredVel = Vector3Scale(rallyDir, config_.droneMoveSpeed);
            }
        } else if (rallyPhase_ == RallyPhase::Holding && drone.world == playerWorld_) {
            // Mild rally-point attraction for holding, flocking does the rest
            Vector3 toRally = Vector3Subtract(rallyPoint_, drone.position);
            Vector3 rallyDir = IsSphericalMap()
                ? ProjectOnSphericalTangent(toRally, up)
                : Vector3{toRally.x, 0.0f, toRally.z};
            float rallyDist = Vector3Length(rallyDir);
            if (rallyDist > 0.3f) {
                rallyDir = Vector3Normalize(rallyDir);
                desiredVel = Vector3Scale(rallyDir, config_.droneMoveSpeed * std::min(rallyDist / config_.droneFlockingRadius, 1.0f));
            }
        } else if (hasTarget) {
            Vector3 toTarget = Vector3Subtract(targetPos, drone.position);
            Vector3 pursuitDir = IsSphericalMap()
                ? ProjectOnSphericalTangent(toTarget, up)
                : Vector3{toTarget.x, 0.0f, toTarget.z};
            if (Vector3Length(pursuitDir) > 0.001f) {
                pursuitDir = Vector3Normalize(pursuitDir);
            }
            desiredVel = Vector3Scale(pursuitDir, config_.droneMoveSpeed);
        }

        // Blend desired velocity with flocking acceleration
        Vector3 horizVel = IsSphericalMap()
            ? ProjectOnSphericalTangent(drone.velocity, up)
            : Vector3{drone.velocity.x, 0.0f, drone.velocity.z};

        float blend = std::clamp(3.5f * dt, 0.0f, 1.0f);
        Vector3 steerTarget = Vector3Add(desiredVel, Vector3Scale(flockAccel[i], 0.6f));
        horizVel = Vector3Add(horizVel,
            Vector3Scale(Vector3Subtract(steerTarget, horizVel), blend));

        // Clamp horizontal speed
        float maxSpeed = config_.droneMoveSpeed * 1.5f;
        float hSpeed = Vector3Length(horizVel);
        if (hSpeed > maxSpeed) {
            horizVel = Vector3Scale(horizVel, maxSpeed / hSpeed);
        }

        // Hover altitude
        float targetAltitude = config_.droneHoverAltitude;
        float currentAlt = IsSphericalMap()
            ? SphericalAltitudeAt(drone.position, drone.world)
            : Vector3DotProduct(drone.position, FlatUpForWorld(drone.world));
        float altError = targetAltitude - currentAlt;
        float verticalSpeed = Vector3DotProduct(drone.velocity, up);
        float verticalAccel = altError * 12.0f - verticalSpeed * 5.0f;
        float newVerticalSpeed = verticalSpeed + verticalAccel * dt;

        drone.velocity = Vector3Add(horizVel, Vector3Scale(up, newVerticalSpeed));

        drone.position = Vector3Add(drone.position, Vector3Scale(drone.velocity, dt));

        // Keep in bounds
        if (IsSphericalMap()) {
            drone.position = SphericalSurfacePoint(drone.position, config_.droneHoverAltitude, drone.world);
            drone.velocity = ProjectOnSphericalTangent(drone.velocity, SphericalUpAt(drone.position, drone.world));
        } else if (IsSquareMap()) {
            float limit = squareHalfExtent_ - 1.5f;
            drone.position.x = std::clamp(drone.position.x, -limit, limit);
            drone.position.z = std::clamp(drone.position.z, -limit, limit);
            float altitude = std::clamp(std::abs(Vector3DotProduct(Vector3Subtract(drone.position, Vector3{0.0f, FlatGroundYForWorld(drone.world), 0.0f}), FlatUpForWorld(drone.world))), 1.2f, 16.0f);
            drone.position.y = FlatGroundYForWorld(drone.world) + FlatUpForWorld(drone.world).y * altitude;
        } else {
            Vector3 flat = Vector3{drone.position.x, 0.0f, drone.position.z};
            float limit = arenaRadius_ - 1.5f;
            if (Vector3Length(flat) > limit) {
                flat = Vector3Scale(Vector3Normalize(flat), limit);
                drone.position.x = flat.x;
                drone.position.z = flat.z;
            }
            float altitude = std::clamp(std::abs(Vector3DotProduct(Vector3Subtract(drone.position, Vector3{0.0f, FlatGroundYForWorld(drone.world), 0.0f}), FlatUpForWorld(drone.world))), 1.2f, 16.0f);
            drone.position.y = FlatGroundYForWorld(drone.world) + FlatUpForWorld(drone.world).y * altitude;
        }

        // Machine gun (disabled while assembling)
        drone.shootTimer -= dt;
        if (rallyPhase_ != RallyPhase::Assembling && hasTarget && nearestDist < config_.droneShootRange && drone.shootTimer <= 0.0f) {
            Vector3 toEnemy = Vector3Normalize(Vector3Subtract(targetPos, drone.position));
            Vector3 r = SafeNormalize(Vector3CrossProduct(toEnemy, up), Vector3{1.0f, 0.0f, 0.0f});
            Vector3 u = up;
            Vector3 aimDir = Vector3Normalize(Vector3Add(toEnemy,
                Vector3Add(Vector3Scale(r, RandomFloat(-0.07f, 0.07f)),
                           Vector3Scale(u, RandomFloat(-0.05f, 0.05f)))));

            Vector3 spawnPos = Vector3Add(drone.position, Vector3Scale(up, 0.2f));
            Vector3 intendedVel = Vector3Scale(aimDir, config_.droneBulletSpeed);

            PhysicsWorld::BodyConfig bulletConfig;
            bulletConfig.motionType = JPH::EMotionType::Dynamic;
            bulletConfig.layer = Layers::PROJECTILE;
            bulletConfig.linearVelocity = JPH::Vec3(intendedVel.x, intendedVel.y, intendedVel.z);
            bulletConfig.gravityFactor = 0.0f;
            bulletConfig.linearDamping = 0.0f;
            bulletConfig.motionQuality = JPH::EMotionQuality::LinearCast;
            bulletConfig.allowSleeping = false;

            JPH::BodyID bulletBody = physics_.CreateBody(projectileShape_, ToJoltVector(spawnPos), JPH::Quat::sIdentity(), bulletConfig);
            projectiles_.push_back(Projectile{bulletBody, ProjectileKind::DroneBullet, 1.2f, 1.2f, config_.droneBulletDamage, 0.06f, 0.06f, Color{255, 240, 140, 255}, 0, intendedVel, ProjectileOwner::Player, false, {}, 0.0f, false, drone.world});

            PlaySfxAt(sfxLaserPlasma_, spawnPos, 44.0f, 0.45f);
            drone.shootTimer = config_.droneShootInterval;
        }

        // Rockets (disabled while assembling)
        drone.rocketTimer -= dt;
        if (rallyPhase_ != RallyPhase::Assembling && hasTarget && nearestDist < config_.droneRocketRange && drone.rocketTimer <= 0.0f) {
            Vector3 toEnemy = Vector3Normalize(Vector3Subtract(targetPos, drone.position));
            Vector3 spawnPos = Vector3Add(drone.position, Vector3Scale(up, 0.35f));
            float rocketSpeed = 24.0f;
            Vector3 rocketVel = Vector3Scale(toEnemy, rocketSpeed);

            PhysicsWorld::BodyConfig rocketConfig;
            rocketConfig.motionType = JPH::EMotionType::Dynamic;
            rocketConfig.layer = Layers::PROJECTILE;
            rocketConfig.linearVelocity = JPH::Vec3(rocketVel.x, rocketVel.y, rocketVel.z);
            rocketConfig.gravityFactor = IsSphericalMap() ? 0.0f : 0.02f;
            rocketConfig.linearDamping = 0.0f;
            rocketConfig.motionQuality = JPH::EMotionQuality::LinearCast;
            rocketConfig.allowSleeping = false;

            JPH::BodyID rocketBody = physics_.CreateBody(projectileShape_, ToJoltVector(spawnPos), JPH::Quat::sIdentity(), rocketConfig);
            projectiles_.push_back(Projectile{rocketBody, ProjectileKind::Rocket, 2.8f, 2.8f, config_.rocketImpactDamage, 0.34f, 0.34f, Color{230, 235, 210, 255}, 0, rocketVel, ProjectileOwner::Player, false, {}, 0.0f, false, drone.world});

            PlaySfxAt(sfxRocketLauncher_, spawnPos, 58.0f, 0.6f);
            drone.rocketTimer = config_.droneRocketInterval + RandomFloat(-0.3f, 0.3f);
            SpawnHitBurst(spawnPos, Color{255, 180, 60, 255}, 8);
        }

        ++i;
    }

    // ── Rally phase transitions ──────────────────────────────────────
    if (rallyPhase_ == RallyPhase::Assembling) {
        bool allAtRally = true;
        int activeCount = 0;
        for (const Drone& d : drones_) {
            if (d.state != DroneState::Active) continue;
            if (d.world != playerWorld_) continue;
            activeCount++;
            float dist = Vector3Distance(d.position, rallyPoint_);
            if (dist > config_.droneFlockingRadius * 1.3f) {
                allAtRally = false;
                break;
            }
        }
        if (activeCount > 0 && allAtRally) {
            rallyPhase_ = RallyPhase::Holding;
            eventText_ = "RALLY: HOLDING";
            eventTextTimer_ = 1.4f;
        }
    }

    if (rallyPhase_ == RallyPhase::Holding) {
        rallyHoldTimer_ -= dt;
        if (rallyHoldTimer_ <= 0.0f) {
            rallyPhase_ = RallyPhase::Complete;
            eventText_ = "RALLY COMPLETE";
            eventTextTimer_ = 1.8f;
        }
    }

    if (rallyPhase_ == RallyPhase::Complete) {
        rallyPhase_ = RallyPhase::Inactive;
    }
}
void Game::UpdateCollisions() {
    for (size_t projectileIndex = 0; projectileIndex < projectiles_.size();) {
        bool projectileDestroyed = false;
        if (projectiles_[projectileIndex].owner == ProjectileOwner::Enemy || projectiles_[projectileIndex].kind == ProjectileKind::EnemyShot) {
            ++projectileIndex;
            continue;
        }

        Vector3 projectilePosition = BodyPosition(projectiles_[projectileIndex].body);
        Vector3 projectileVelocity = ToRayVector(physics_.Bodies().GetLinearVelocity(projectiles_[projectileIndex].body));
        Vector3 previousPosition = Vector3Subtract(projectilePosition, Vector3Scale(projectileVelocity, kFixedFrame));

        for (size_t enemyIndex = 0; enemyIndex < enemies_.size(); ++enemyIndex) {
            Enemy& enemy = enemies_[enemyIndex];
            Vector3 enemyPosition = BodyPosition(enemy.body);
            float hitDistance = enemy.radius + projectiles_[projectileIndex].radius;

            if (Vector3Distance(projectilePosition, enemyPosition) <= hitDistance
                || DistancePointToSegment(enemyPosition, previousPosition, projectilePosition) <= hitDistance) {
                if (projectiles_[projectileIndex].kind == ProjectileKind::Flame && projectiles_[projectileIndex].lastHitEnemy == enemy.body) {
                    continue;
                }
                if (projectiles_[projectileIndex].kind == ProjectileKind::UfoOrb) {
                    ExplodeUfoOrb(projectilePosition, projectiles_[projectileIndex].world, true, projectiles_[projectileIndex].damage);
                    DestroyProjectile(projectileIndex);
                    projectileDestroyed = true;
                    break;
                }
                if (projectiles_[projectileIndex].kind == ProjectileKind::Rocket) {
                    ExplodeRocket(projectilePosition, projectiles_[projectileIndex].owner, projectiles_[projectileIndex].shooterBody);
                    DestroyProjectile(projectileIndex);
                    projectileDestroyed = true;
                    break;
                } else if (projectiles_[projectileIndex].kind == ProjectileKind::GravityNail) {
                    enemy.health -= projectiles_[projectileIndex].damage;
                    totalDamageDealt_ += projectiles_[projectileIndex].damage;
                    PlayEnemyHitSfx(enemyPosition);
                    if (enemy.type == EnemyType::Dummy || enemy.type == EnemyType::DummyBoss) {
                        RecordDummyDamage(enemy, projectiles_[projectileIndex].damage);
                    }
                    SpawnGravityWell(enemyPosition);
                    DestroyProjectile(projectileIndex);
                    projectileDestroyed = true;
                    cameraShake_ = std::min(1.0f, cameraShake_ + 0.22f);
                    if (enemy.health <= 0.0f) {
                        SpawnHitBurst(enemyPosition, Color{210, 225, 255, 255}, 20);
                        score_ += enemy.scoreValue;
                        DestroyEnemy(enemyIndex);
                    }
                    break;
                } else if (projectiles_[projectileIndex].kind == ProjectileKind::BlackHoleGrenade) {
                    enemy.health -= projectiles_[projectileIndex].damage;
                    totalDamageDealt_ += projectiles_[projectileIndex].damage;
                    PlayEnemyHitSfx(enemyPosition);
                    if (enemy.type == EnemyType::Dummy || enemy.type == EnemyType::DummyBoss) {
                        RecordDummyDamage(enemy, projectiles_[projectileIndex].damage);
                    }
                    SpawnGravityWell(projectilePosition, true);
                    DestroyProjectile(projectileIndex);
                    projectileDestroyed = true;
                    cameraShake_ = std::min(1.0f, cameraShake_ + 0.4f);
                    if (enemy.health <= 0.0f) {
                        SpawnHitBurst(enemyPosition, Color{180, 120, 255, 255}, 24);
                        score_ += enemy.scoreValue;
                        DestroyEnemy(enemyIndex);
                    }
                    break;
                } else if (projectiles_[projectileIndex].kind == ProjectileKind::Lance) {
                    enemy.health -= projectiles_[projectileIndex].damage;
                    totalDamageDealt_ += projectiles_[projectileIndex].damage;
                    PlayEnemyHitSfx(enemyPosition);
                    if (enemy.type == EnemyType::Dummy || enemy.type == EnemyType::DummyBoss) {
                        RecordDummyDamage(enemy, projectiles_[projectileIndex].damage);
                    }
                    SpawnHitBurst(enemyPosition, Color{210, 245, 255, 255}, 10);
                    cameraShake_ = std::min(1.0f, cameraShake_ + 0.18f);
                    if (enemy.health <= 0.0f) {
                        SpawnHitBurst(enemyPosition, Color{245, 255, 255, 255}, 20);
                        score_ += enemy.scoreValue;
                        DestroyEnemy(enemyIndex);
                    }
                    continue;
                } else if (projectiles_[projectileIndex].kind == ProjectileKind::CurseOrb) {
                    enemy.health -= projectiles_[projectileIndex].damage;
                    totalDamageDealt_ += projectiles_[projectileIndex].damage;
                    PlayEnemyHitSfx(enemyPosition);
                    if (enemy.type == EnemyType::Dummy || enemy.type == EnemyType::DummyBoss) {
                        RecordDummyDamage(enemy, projectiles_[projectileIndex].damage);
                    }
                    if (!enemy.cursed) {
                        enemy.cursed = true;
                        enemy.curseDps = config_.curseOrbDps;
                    } else {
                        enemy.curseDps = std::min(enemy.curseDps * 1.15f, config_.curseOrbDps * config_.curseOrbMaxStackMult);
                    }
                    SpawnHitBurst(enemyPosition, Color{180, 100, 255, 255}, 14);
                    DestroyProjectile(projectileIndex);
                    projectileDestroyed = true;
                    cameraShake_ = std::min(1.0f, cameraShake_ + 0.2f);
                    if (enemy.health <= 0.0f) {
                        SpawnHitBurst(enemyPosition, Color{140, 60, 240, 255}, 20);
                        score_ += enemy.scoreValue;
                        DestroyEnemy(enemyIndex);
                    }
                    break;
                } else if (projectiles_[projectileIndex].kind == ProjectileKind::SoulOrb) {
                    enemy.health -= projectiles_[projectileIndex].damage;
                    totalDamageDealt_ += projectiles_[projectileIndex].damage;
                    PlayEnemyHitSfx(enemyPosition);
                    if (enemy.type == EnemyType::Dummy || enemy.type == EnemyType::DummyBoss) {
                        RecordDummyDamage(enemy, projectiles_[projectileIndex].damage);
                    }
                    SpawnHitBurst(enemyPosition, Color{60, 20, 100, 255}, 10);
                    DestroyProjectile(projectileIndex);
                    projectileDestroyed = true;
                    cameraShake_ = std::min(1.0f, cameraShake_ + 0.15f);
                    if (enemy.health <= 0.0f) {
                        enemy.killedBySoulOrb = true;
                        SpawnHitBurst(enemyPosition, Color{40, 10, 70, 255}, 20);
                        score_ += enemy.scoreValue;
                        DestroyEnemy(enemyIndex);
                    }
                    break;
                }

                enemy.health -= projectiles_[projectileIndex].damage;
                totalDamageDealt_ += projectiles_[projectileIndex].damage;
                PlayEnemyHitSfx(bethlehem_.position);
                if (enemy.type == EnemyType::Dummy || enemy.type == EnemyType::DummyBoss) {
                    RecordDummyDamage(enemy, projectiles_[projectileIndex].damage);
                }
                if (projectiles_[projectileIndex].kind == ProjectileKind::Flame) {
                    SpawnHitBurst(projectilePosition, Color{255, 135, 28, 255}, 4);
                    projectiles_[projectileIndex].lastHitEnemy = enemy.body;
                } else {
                    SpawnHitBurst(enemyPosition, enemy.color, 10);
                    DestroyProjectile(projectileIndex);
                    projectileDestroyed = true;
                    cameraShake_ = std::min(1.0f, cameraShake_ + 0.3f);
                }

                if (enemy.health <= 0.0f) {
                    SpawnHitBurst(enemyPosition, Color{255, 255, 255, 255}, 20);
                    score_ += enemy.scoreValue;
                    DestroyEnemy(enemyIndex);
                }

                if (projectiles_[projectileIndex].kind != ProjectileKind::Flame) {
                    break;
                }
            }
        }

        if (!projectileDestroyed) {
            for (MagicCircle& circle : magicCircles_) {
                if (projectiles_[projectileIndex].world != circle.world) continue;
                Vector3 up = UpForWorldAt(circle.position, circle.world);
                Vector3 octaCenter = Vector3Add(circle.position, Vector3Scale(up, 1.0f + circle.radius * 0.4f));
                float dist = Vector3Distance(projectilePosition, octaCenter);
                if (dist <= circle.radius + projectiles_[projectileIndex].radius
                    && !projectiles_[projectileIndex].fromMagicCircle) {
                    ProjectileKind kind = projectiles_[projectileIndex].kind;

                    // BlackHoleGrenade → activate wormhole
                    if (kind == ProjectileKind::BlackHoleGrenade) {
                        int circleIndex = static_cast<int>(&circle - magicCircles_.data());
                        ActivateWormhole(circle, circleIndex, octaCenter);
                        DestroyProjectile(projectileIndex);
                        projectileDestroyed = true;
                        break;
                    }

                    // Lance → deactivate circle
                    if (kind == ProjectileKind::Lance && projectiles_[projectileIndex].owner == ProjectileOwner::Player) {
                        if (circle.isWormhole && CloseWormhole(projectilePosition)) {
                            DestroyProjectile(projectileIndex);
                            projectileDestroyed = true;
                            break;
                        }
                        if (!circle.isWormhole) {
                            circle.activated = false;
                            circle.activatedByLaserBeam = false;
                            circle.fireCooldown = 0.0f;
                            PlaySfxAt(sfxMagicCircleClear_, octaCenter, 70.0f, 0.9f);
                            SpawnShockwave(octaCenter, circle.radius * 1.2f, Color{255, 200, 60, 255});
                            SpawnHitBurst(projectilePosition, Color{255, 220, 80, 255}, 20);
                            eventText_ = "CIRCLE CLEARED";
                            eventTextTimer_ = 1.1f;
                            DestroyProjectile(projectileIndex);
                            projectileDestroyed = true;
                            break;
                        }
                    }

                    // Wormhole absorbs everything except Lance (handled above)
                    if (circle.isWormhole) {
                        SpawnHitBurst(projectilePosition, Color{170, 70, 255, 255}, 5);
                        DestroyProjectile(projectileIndex);
                        projectileDestroyed = true;
                        break;
                    }

                    if (MagicCircleCanActivate(kind)) {
                        if (!circle.activated) {
                            circle.activatedKind = kind;
                            circle.activated = true;
                            circle.activatedByLaserBeam = false;
                            circle.fireRateMult = config_.magicCircleFireRateMult;
                            circle.homingTurnRate = config_.magicCircleHomingTurnRate;
                            circle.fireCooldown = 0.0f;
                            PlaySfxAt(sfxMagicCircleActivate_, octaCenter, 70.0f, 0.9f);
                            SpawnShockwave(octaCenter, circle.radius * 1.8f, MagicCircleTint(circle));
                            SpawnHitBurst(projectilePosition, MagicCircleTint(circle), 30);
                            eventText_ = MagicCircleKindName(circle);
                            eventTextTimer_ = 1.2f;
                        } else if (circle.activatedKind != kind) {
                            SpawnShockwave(octaCenter, 0.5f, Color{120, 100, 140, 255});
                            DestroyProjectile(projectileIndex);
                            projectileDestroyed = true;
                            break;
                        }
                        SpawnHitBurst(projectilePosition, Color{220, 180, 255, 255}, 6);
                        DestroyProjectile(projectileIndex);
                        projectileDestroyed = true;
                        break;
                    }
                    // Non-activator projectile → ignored (passes through)
                }
            }
        }
        if (!projectileDestroyed) {
            ++projectileIndex;
        }
    }

    // Player projectiles vs Bethlehem boss (no physics body)
    if (bethlehem_.active) {
        for (size_t pi = 0; pi < projectiles_.size();) {
            if (projectiles_[pi].owner != ProjectileOwner::Player) { ++pi; continue; }
            Vector3 pp = BodyPosition(projectiles_[pi].body);
            float hitDist = projectiles_[pi].radius + 3.0f;
            if (Vector3Distance(pp, bethlehem_.position) <= hitDist) {
                bethlehem_.health -= projectiles_[pi].damage;
                totalDamageDealt_ += projectiles_[pi].damage;
                PlayEnemyHitSfx(bethlehem_.position);
                SpawnHitBurst(pp, Color{255, 210, 100, 255}, 8);
                DestroyProjectile(pi);
                if (bethlehem_.health <= 0.0f) {
                    DestroyBethlehem();
                }
            } else {
                ++pi;
            }
        }
    }

    // Player projectiles vs Scavenger UFO boss (no physics body)
    if (ScavengerUfoDamageable()) {
        for (size_t pi = 0; pi < projectiles_.size();) {
            if (projectiles_[pi].owner != ProjectileOwner::Player) { ++pi; continue; }
            Vector3 pp = BodyPosition(projectiles_[pi].body);
            Vector3 pv = ToRayVector(physics_.Bodies().GetLinearVelocity(projectiles_[pi].body));
            Vector3 prev = Vector3Subtract(pp, Vector3Scale(pv, kFixedFrame));
            float hitDist = projectiles_[pi].radius + 2.7f;
            if (Vector3Distance(pp, scavengerUfo_.position) <= hitDist
                || DistancePointToSegment(scavengerUfo_.position, prev, pp) <= hitDist) {
                ProjectileKind kind = projectiles_[pi].kind;
                float damage = projectiles_[pi].damage;
                if (kind == ProjectileKind::Rocket) {
                    ExplodeRocket(pp, projectiles_[pi].owner, projectiles_[pi].shooterBody);
                    DestroyProjectile(pi);
                    continue;
                }
                if (kind == ProjectileKind::BlackHoleGrenade) {
                    SpawnGravityWell(pp, true);
                    DamageScavengerUfo(damage, pp, Color{170, 90, 255, 255});
                    DestroyProjectile(pi);
                    continue;
                }
                DamageScavengerUfo(damage, pp, projectiles_[pi].color);
                if (kind != ProjectileKind::Flame && kind != ProjectileKind::Lance) {
                    DestroyProjectile(pi);
                    continue;
                }
            }
            ++pi;
        }
    }

    // Player projectiles vs Throne Angel boss and cherubs (no physics bodies)
    if (throneAngel_.active || !cherubs_.empty()) {
        for (size_t pi = 0; pi < projectiles_.size();) {
            if (projectiles_[pi].owner != ProjectileOwner::Player) { ++pi; continue; }
            Vector3 pp = BodyPosition(projectiles_[pi].body);
            Vector3 pv = ToRayVector(physics_.Bodies().GetLinearVelocity(projectiles_[pi].body));
            Vector3 prev = Vector3Subtract(pp, Vector3Scale(pv, kFixedFrame));
            ProjectileKind kind = projectiles_[pi].kind;
            bool consumed = false;

            if (throneAngel_.active && projectiles_[pi].world == throneAngel_.world) {
                float hitDist = projectiles_[pi].radius + 4.2f;
                if (Vector3Distance(pp, throneAngel_.position) <= hitDist
                    || DistancePointToSegment(throneAngel_.position, prev, pp) <= hitDist) {
                    float damage = projectiles_[pi].damage;
                    if (kind == ProjectileKind::Rocket) {
                        ExplodeRocket(pp, projectiles_[pi].owner, projectiles_[pi].shooterBody);
                        DestroyProjectile(pi);
                        continue;
                    }
                    if (kind == ProjectileKind::BlackHoleGrenade) {
                        SpawnGravityWell(pp, true);
                        DamageThroneAngel(damage, pp, Color{210, 210, 255, 255});
                        DestroyProjectile(pi);
                        continue;
                    }
                    DamageThroneAngel(damage, pp, projectiles_[pi].color);
                    if (kind != ProjectileKind::Flame && kind != ProjectileKind::Lance) {
                        DestroyProjectile(pi);
                        continue;
                    }
                    ++pi;
                    continue;
                }
            }

            if (!consumed) {
                for (size_t ci = 0; ci < cherubs_.size(); ++ci) {
                    if (projectiles_[pi].world != cherubs_[ci].world) continue;
                    float hitDist = projectiles_[pi].radius + 0.58f;
                    if (Vector3Distance(pp, cherubs_[ci].position) > hitDist
                        && DistancePointToSegment(cherubs_[ci].position, prev, pp) > hitDist) {
                        continue;
                    }
                    cherubs_[ci].health -= projectiles_[pi].damage;
                    totalDamageDealt_ += projectiles_[pi].damage;
                    cherubs_[ci].flashTimer = 0.18f;
                    SpawnHitBurst(cherubs_[ci].position, projectiles_[pi].color, 8);
                    if (cherubs_[ci].health <= 0.0f) {
                        SpawnHitBurst(cherubs_[ci].position, Color{245, 245, 235, 255}, 18);
                        cherubs_[ci] = cherubs_.back();
                        cherubs_.pop_back();
                    }
                    if (kind != ProjectileKind::Flame && kind != ProjectileKind::Lance) {
                        DestroyProjectile(pi);
                        consumed = true;
                    }
                    break;
                }
            }
            if (!consumed) ++pi;
        }
    }

    // Player projectiles vs Seraph boss (no physics body)
    if (!seraphs_.empty()) {
        for (size_t pi = 0; pi < projectiles_.size();) {
            if (projectiles_[pi].owner != ProjectileOwner::Player) { ++pi; continue; }
            Vector3 pp = BodyPosition(projectiles_[pi].body);
            Vector3 pv = ToRayVector(physics_.Bodies().GetLinearVelocity(projectiles_[pi].body));
            Vector3 prev = Vector3Subtract(pp, Vector3Scale(pv, kFixedFrame));
            const SeraphBoss* hitSeraph = nullptr;
            for (const SeraphBoss& seraph : seraphs_) {
                if (!seraph.active || projectiles_[pi].world != seraph.world) continue;
                float hitDist = projectiles_[pi].radius + 3.6f;
                if (Vector3Distance(pp, seraph.position) <= hitDist
                    || DistancePointToSegment(seraph.position, prev, pp) <= hitDist) {
                    hitSeraph = &seraph;
                    break;
                }
            }
            if (hitSeraph) {
                ProjectileKind kind = projectiles_[pi].kind;
                float damage = projectiles_[pi].damage;
                if (kind == ProjectileKind::Rocket) {
                    ExplodeRocket(pp, projectiles_[pi].owner, projectiles_[pi].shooterBody);
                    DestroyProjectile(pi);
                    continue;
                }
                if (kind == ProjectileKind::BlackHoleGrenade) {
                    SpawnGravityWell(pp, true);
                    DamageSeraph(damage, hitSeraph->position, Color{255, 220, 150, 255});
                    DestroyProjectile(pi);
                    continue;
                }
                DamageSeraph(damage, hitSeraph->position, projectiles_[pi].color);
                if (kind != ProjectileKind::Flame && kind != ProjectileKind::Lance) {
                    DestroyProjectile(pi);
                    continue;
                }
            }
            ++pi;
        }
    }

    if (warRider_.active) {
        for (size_t pi = 0; pi < projectiles_.size();) {
            if (projectiles_[pi].owner != ProjectileOwner::Player || projectiles_[pi].world != warRider_.world) {
                ++pi;
                continue;
            }
            Vector3 pp = BodyPosition(projectiles_[pi].body);
            Vector3 pv = ToRayVector(physics_.Bodies().GetLinearVelocity(projectiles_[pi].body));
            Vector3 prev = Vector3Subtract(pp, Vector3Scale(pv, kFixedFrame));
            float hitDist = projectiles_[pi].radius + kHorsemanHitPadding;
            if (Vector3Distance(pp, warRider_.position) <= hitDist
                || DistancePointToSegment(warRider_.position, prev, pp) <= hitDist) {
                ProjectileKind kind = projectiles_[pi].kind;
                float damage = projectiles_[pi].damage;
                if (kind == ProjectileKind::Rocket) {
                    ExplodeRocket(pp, projectiles_[pi].owner, projectiles_[pi].shooterBody);
                    DestroyProjectile(pi);
                    continue;
                }
                if (kind == ProjectileKind::BlackHoleGrenade) {
                    SpawnGravityWell(pp, true);
                    DamageWarRider(damage, pp, Color{180, 70, 255, 255});
                    DestroyProjectile(pi);
                    continue;
                }
                DamageWarRider(damage, pp, projectiles_[pi].color);
                if (kind != ProjectileKind::Flame && kind != ProjectileKind::Lance) {
                    DestroyProjectile(pi);
                    continue;
                }
            }
            ++pi;
        }
    }
    if (conquestRider_.active) {
        for (size_t pi = 0; pi < projectiles_.size();) {
            if (projectiles_[pi].owner != ProjectileOwner::Player || projectiles_[pi].world != conquestRider_.world) {
                ++pi;
                continue;
            }
            Vector3 pp = BodyPosition(projectiles_[pi].body);
            Vector3 pv = ToRayVector(physics_.Bodies().GetLinearVelocity(projectiles_[pi].body));
            Vector3 prev = Vector3Subtract(pp, Vector3Scale(pv, kFixedFrame));
            float hitDist = projectiles_[pi].radius + kHorsemanHitPadding;
            if (Vector3Distance(pp, conquestRider_.position) <= hitDist
                || DistancePointToSegment(conquestRider_.position, prev, pp) <= hitDist) {
                ProjectileKind kind = projectiles_[pi].kind;
                float damage = projectiles_[pi].damage;
                if (kind == ProjectileKind::Rocket) {
                    ExplodeRocket(pp, projectiles_[pi].owner, projectiles_[pi].shooterBody);
                    DestroyProjectile(pi);
                    continue;
                }
                if (kind == ProjectileKind::BlackHoleGrenade) {
                    SpawnGravityWell(pp, true);
                    DamageConquestRider(damage, pp, Color{180, 70, 255, 255});
                    DestroyProjectile(pi);
                    continue;
                }
                DamageConquestRider(damage, pp, projectiles_[pi].color);
                if (kind != ProjectileKind::Flame && kind != ProjectileKind::Lance) {
                    DestroyProjectile(pi);
                    continue;
                }
            }
            ++pi;
        }
    }
    if (famineRider_.active) {
        for (size_t pi = 0; pi < projectiles_.size();) {
            if (projectiles_[pi].owner != ProjectileOwner::Player || projectiles_[pi].world != famineRider_.world) {
                ++pi;
                continue;
            }
            Vector3 pp = BodyPosition(projectiles_[pi].body);
            Vector3 pv = ToRayVector(physics_.Bodies().GetLinearVelocity(projectiles_[pi].body));
            Vector3 prev = Vector3Subtract(pp, Vector3Scale(pv, kFixedFrame));
            float hitDist = projectiles_[pi].radius + kHorsemanHitPadding;
            if (Vector3Distance(pp, famineRider_.position) <= hitDist
                || DistancePointToSegment(famineRider_.position, prev, pp) <= hitDist) {
                ProjectileKind kind = projectiles_[pi].kind;
                float damage = projectiles_[pi].damage;
                if (kind == ProjectileKind::Rocket) {
                    ExplodeRocket(pp, projectiles_[pi].owner, projectiles_[pi].shooterBody);
                    DestroyProjectile(pi);
                    continue;
                }
                if (kind == ProjectileKind::BlackHoleGrenade) {
                    SpawnGravityWell(pp, true);
                    DamageFamineRider(damage, pp, Color{180, 70, 255, 255});
                    DestroyProjectile(pi);
                    continue;
                }
                DamageFamineRider(damage, pp, projectiles_[pi].color);
                if (kind != ProjectileKind::Flame && kind != ProjectileKind::Lance) {
                    DestroyProjectile(pi);
                    continue;
                }
            }
            ++pi;
        }
    }
    if (deathRider_.active) {
        for (size_t pi = 0; pi < projectiles_.size();) {
            if (projectiles_[pi].owner != ProjectileOwner::Player || projectiles_[pi].world != deathRider_.world) {
                ++pi;
                continue;
            }
            Vector3 pp = BodyPosition(projectiles_[pi].body);
            Vector3 pv = ToRayVector(physics_.Bodies().GetLinearVelocity(projectiles_[pi].body));
            Vector3 prev = Vector3Subtract(pp, Vector3Scale(pv, kFixedFrame));
            float hitDist = projectiles_[pi].radius + kDeathRiderHitPadding;
            if (Vector3Distance(pp, deathRider_.position) <= hitDist
                || DistancePointToSegment(deathRider_.position, prev, pp) <= hitDist) {
                ProjectileKind kind = projectiles_[pi].kind;
                float damage = projectiles_[pi].damage;
                if (kind == ProjectileKind::Rocket) {
                    ExplodeRocket(pp, projectiles_[pi].owner, projectiles_[pi].shooterBody);
                    DestroyProjectile(pi);
                    continue;
                }
                if (kind == ProjectileKind::BlackHoleGrenade) {
                    SpawnGravityWell(pp, true);
                    DamageDeathRider(damage, pp, Color{180, 70, 255, 255});
                    DestroyProjectile(pi);
                    continue;
                }
                DamageDeathRider(damage, pp, projectiles_[pi].color);
                if (kind != ProjectileKind::Flame && kind != ProjectileKind::Lance) {
                    DestroyProjectile(pi);
                    continue;
                }
            }
            ++pi;
        }
    }
    for (size_t si = 0; si < deathSkulls_.size();) {
        bool skullDestroyed = false;
        for (size_t pi = 0; pi < projectiles_.size();) {
            if (projectiles_[pi].owner != ProjectileOwner::Player || projectiles_[pi].world != deathSkulls_[si].world) {
                ++pi;
                continue;
            }
            Vector3 pp = BodyPosition(projectiles_[pi].body);
            Vector3 pv = ToRayVector(physics_.Bodies().GetLinearVelocity(projectiles_[pi].body));
            Vector3 prev = Vector3Subtract(pp, Vector3Scale(pv, kFixedFrame));
            float hitDist = projectiles_[pi].radius + deathSkulls_[si].radius;
            if (Vector3Distance(pp, deathSkulls_[si].position) <= hitDist
                || DistancePointToSegment(deathSkulls_[si].position, prev, pp) <= hitDist) {
                ProjectileKind kind = projectiles_[pi].kind;
                float damage = projectiles_[pi].damage;
                if (kind == ProjectileKind::Rocket) {
                    ExplodeRocket(pp, projectiles_[pi].owner, projectiles_[pi].shooterBody);
                    DestroyProjectile(pi);
                    skullDestroyed = true;
                    break;
                }
                if (kind == ProjectileKind::BlackHoleGrenade) {
                    SpawnGravityWell(pp, true);
                    deathSkulls_[si].health -= damage;
                    DestroyProjectile(pi);
                } else {
                    deathSkulls_[si].health -= damage;
                    if (kind != ProjectileKind::Flame && kind != ProjectileKind::Lance) {
                        DestroyProjectile(pi);
                    } else {
                        ++pi;
                    }
                }
                SpawnHitBurst(deathSkulls_[si].position, Color{240, 240, 230, 255}, 5);
                if (deathSkulls_[si].health <= 0.0f) {
                    SpawnHitBurst(deathSkulls_[si].position, Color{245, 245, 235, 255}, 16);
                    deathSkulls_[si] = deathSkulls_.back();
                    deathSkulls_.pop_back();
                    skullDestroyed = true;
                    break;
                }
                continue;
            }
            ++pi;
        }
        if (!skullDestroyed) {
            ++si;
        }
    }
}
void Game::FireEnemyProjectile(ProjectileKind kind, Vector3 position, Vector3 direction, float speed, float damage, float life, float radius, float maxRadius, Color color, int world, JPH::BodyID shooterBody) {
    Vector3 intendedVelocity = Vector3Scale(direction, speed);

    PhysicsWorld::BodyConfig projectileConfig;
    projectileConfig.motionType = JPH::EMotionType::Dynamic;
    projectileConfig.layer = Layers::PROJECTILE;
    projectileConfig.linearVelocity = timeStopped_ ? JPH::Vec3::sZero() : JPH::Vec3(intendedVelocity.x, intendedVelocity.y, intendedVelocity.z);
    projectileConfig.gravityFactor = (IsSphericalMap() || world == 1)
        ? 0.0f
        : kind == ProjectileKind::Rocket ? 0.02f : kind == ProjectileKind::BlackHoleGrenade ? 0.45f : kind == ProjectileKind::DroneCanister ? 0.65f : 0.0f;
    projectileConfig.linearDamping = 0.0f;
    projectileConfig.motionQuality = JPH::EMotionQuality::LinearCast;
    projectileConfig.allowSleeping = false;

    JPH::BodyID body = physics_.CreateBody(projectileShape_, ToJoltVector(position), JPH::Quat::sIdentity(), projectileConfig);
    projectiles_.push_back(Projectile{body, kind, life, life, damage, radius, maxRadius, color, 0, intendedVelocity, ProjectileOwner::Enemy, timeStopped_, {}, 0.0f, false, world, shooterBody});
    if (kind == ProjectileKind::Rocket) {
        PlaySfxAt(sfxRocketLauncher_, position, 64.0f, 0.62f);
    } else if (kind == ProjectileKind::BlackHoleGrenade) {
        PlaySfxAt(sfxGravityBlackHole_, position, 64.0f, 0.62f);
    } else if (kind == ProjectileKind::GravityNail) {
        PlaySfxAt(sfxGravityNailer_, position, 54.0f, 0.52f);
    } else if (kind == ProjectileKind::Flame) {
        PlaySfxAt(sfxFlamethrowerFireball_, position, 46.0f, 0.36f);
    } else if (kind == ProjectileKind::Lance) {
        PlaySfxAt(sfxSpearThrow_, position, 60.0f, 0.58f);
    } else if (kind == ProjectileKind::GlassShard) {
        PlaySfxAt(sfxShotgunGlass_, position, 48.0f, 0.42f);
    } else if (kind == ProjectileKind::Pellet) {
        PlaySfxAt(sfxShotgunPellet_, position, 42.0f, 0.36f);
    } else {
        PlaySfxAt(sfxLaserPlasma_, position, 48.0f, 0.38f);
    }
}
void Game::FireMagicProjectile(ProjectileKind kind, Vector3 position, Vector3 direction, float speed, float damage,
    float life, float radius, float maxRadius, Color color, float turnRate, int world) {
    Vector3 intendedVelocity = Vector3Scale(direction, speed);
    PhysicsWorld::BodyConfig projConfig;
    projConfig.motionType = JPH::EMotionType::Dynamic;
    projConfig.layer = Layers::PROJECTILE;
    projConfig.linearVelocity = timeStopped_ ? JPH::Vec3::sZero()
        : JPH::Vec3(intendedVelocity.x, intendedVelocity.y, intendedVelocity.z);
    projConfig.gravityFactor = (IsSphericalMap())
        ? 0.0f
        : kind == ProjectileKind::Rocket ? 0.02f : 0.0f;
    projConfig.linearDamping = 0.0f;
    projConfig.motionQuality = JPH::EMotionQuality::LinearCast;
    projConfig.allowSleeping = false;

    JPH::BodyID body = physics_.CreateBody(projectileShape_, ToJoltVector(position),
        JPH::Quat::sIdentity(), projConfig);
    Projectile proj{body, kind, life, life, damage, radius, maxRadius,
        color, 0, intendedVelocity, ProjectileOwner::Player, timeStopped_, {}, 0.0f, false, world};
    proj.turnRate = turnRate;
    proj.fromMagicCircle = true;
    projectiles_.push_back(proj);
}
void Game::FireMagicLaserBeam(Vector3 position, Vector3 direction, int world) {
    Vector3 forward = SafeNormalize(direction, Vector3{0.0f, 0.0f, -1.0f});
    Vector3 end = Vector3Add(position, Vector3Scale(forward, config_.laserBeamRange));
    float damage = config_.laserBaseDamage + config_.laserChargeDamage;
    float beamRadius = config_.laserBeamRadius + config_.laserBeamRadiusChargeBonus;
    float beamLife = config_.laserBeamLifetime + config_.laserBeamLifetimeChargeBonus;
    beams_.push_back(Beam{
        position,
        end,
        beamLife,
        beamLife,
        beamRadius,
        1.0f,
        Color{190, 70, 255, 255}
    });

    for (size_t i = 0; i < enemies_.size();) {
        if (enemies_[i].world != world) {
            ++i;
            continue;
        }
        Vector3 enemyPosition = BodyPosition(enemies_[i].body);
        float hitDistance = beamRadius + enemies_[i].radius * 0.85f;
        if (DistancePointToSegment(enemyPosition, position, end) <= hitDistance) {
            enemies_[i].health -= damage;
            totalDamageDealt_ += damage;
            SpawnHitBurst(enemyPosition, Color{210, 130, 255, 255}, 20);
            if (enemies_[i].health <= 0.0f) {
                score_ += enemies_[i].scoreValue;
                SpawnHitBurst(enemyPosition, Color{255, 245, 255, 255}, 22);
                DestroyEnemy(i);
                continue;
            }
        }
        ++i;
    }
    cameraShake_ = std::min(1.0f, cameraShake_ + 0.22f);
}
float Game::MagicCircleBaseCooldown(const MagicCircle& circle) const {
    if (circle.activatedByLaserBeam) {
        return config_.laserBeamCooldown;
    }
    if (circle.activatedByNapalm) {
        return 0.85f;  // match player napalm cooldown
    }
    switch (circle.activatedKind) {
        case ProjectileKind::LaserShot:
            return config_.plasmaCooldown;
        case ProjectileKind::Flame:
            return 0.045f;
        case ProjectileKind::Rocket:
            return 0.82f;
        case ProjectileKind::Pellet:
            return 0.58f;
        default:
            return config_.magicCircleFireInterval;
    }
}
bool Game::MagicCircleCanActivate(ProjectileKind kind) const {
    return kind == ProjectileKind::LaserShot
        || kind == ProjectileKind::Flame
        || kind == ProjectileKind::Rocket
        || kind == ProjectileKind::Pellet
        || kind == ProjectileKind::GlassShard;
}

Color Game::MagicCircleTint(ProjectileKind kind) const {
    switch (kind) {
        case ProjectileKind::LaserShot:
            return Color{210, 120, 255, 255};
        case ProjectileKind::Flame:
            return Color{185, 70, 245, 235};
        case ProjectileKind::Rocket:
            return Color{170, 90, 255, 255};
        case ProjectileKind::Pellet:
            return Color{225, 155, 255, 255};
        case ProjectileKind::GlassShard:
            return Color{160, 140, 255, 255};
        default:
            return Color{190, 90, 255, 255};
    }
}

Color Game::MagicCircleTint(const MagicCircle& circle) const {
    if (circle.activatedByLaserBeam) {
        return Color{190, 70, 255, 255};
    }
    if (circle.activatedByNapalm) {
        return Color{255, 160, 40, 255};
    }
    return MagicCircleTint(circle.activatedKind);
}
const char* Game::MagicCircleKindName(const MagicCircle& circle) const {
    if (circle.activatedByLaserBeam) {
        return "MAGIC LASER";
    }
    if (circle.activatedByNapalm) {
        return "MAGIC NAPALM";
    }
    return MagicCircleKindName(circle.activatedKind);
}

const char* Game::MagicCircleKindName(ProjectileKind kind) const {
    switch (kind) {
        case ProjectileKind::LaserShot:
            return "MAGIC PLASMA";
        case ProjectileKind::Flame:
            return "MAGIC FLAME";
        case ProjectileKind::Rocket:
            return "MAGIC ROCKET";
        case ProjectileKind::Pellet:
            return "MAGIC SHOT";
        default:
            return "MAGIC CIRCLE";
    }
}

void Game::FireEnemyBeam(Vector3 origin, Vector3 direction, float charge) {
    Vector3 forward = Vector3Normalize(direction);
    float normalizedCharge = std::clamp(charge, 0.25f, 1.0f);
    Vector3 end = Vector3Add(origin, Vector3Scale(forward, 58.0f));
    float beamRadius = 0.28f + normalizedCharge * 0.36f;
    float beamLife = 0.14f + normalizedCharge * 0.08f;
    beams_.push_back(Beam{
        origin,
        end,
        beamLife,
        beamLife,
        beamRadius,
        normalizedCharge,
        Color{255, 205, 105, 255}
    });

    if (DistancePointToSegment(camera_.position, origin, end) <= beamRadius + playerRadius_) {
        ApplyPlayerHit(camera_.position, Color{255, 220, 125, 255});
    }
    SpawnHitBurst(origin, Color{255, 230, 145, 255}, 8);
}
void Game::SpawnEnemyNanoPlatform(Vector3 origin, Vector3 direction, int world) {
    Vector3 forward = Vector3Normalize(direction);
    Vector3 target = Vector3Add(origin, Vector3Scale(forward, config_.nanoPlatformRange * 0.72f));
    float halfLength = config_.nanoPlatformLength * 0.36f;
    float halfWidth = config_.nanoPlatformWidth * 0.36f;
    Vector3 scale = Vector3{config_.nanoPlatformLength * 0.72f, config_.nanoPlatformHeight, config_.nanoPlatformWidth * 0.72f};
    if (IsSphericalMap()) {
        Vector3 normal = SphericalUpAt(target, world);
        float targetAltitude = std::max(SphericalPlayerAltitude(), SphericalAltitudeAt(target, world));
        targetAltitude += config_.nanoPlatformRange * 0.72f * 0.18f;
        Vector3 center = SphericalSurfacePoint(target, targetAltitude, world);
        Vector3 platformRight = SafeNormalize(Vector3CrossProduct(forward, normal), PlayerRight());
        Vector3 platformForward = SafeNormalize(Vector3CrossProduct(normal, platformRight), PlayerForward());
        nanoPlatforms_.push_back(NanoPlatform{center, scale, normal, platformRight, platformForward, config_.nanoPlatformDelay, config_.nanoPlatformLifetime * 0.45f, config_.nanoPlatformLifetime * 0.45f, world});
        SpawnHitBurst(center, Color{255, 220, 115, 255}, 8);
        return;
    }

    if (IsSquareMap()) {
        float limitX = squareHalfExtent_ - halfLength - 0.25f;
        float limitZ = squareHalfExtent_ - halfWidth - 0.25f;
        target.x = std::clamp(target.x, -limitX, limitX);
        target.z = std::clamp(target.z, -limitZ, limitZ);
    } else {
        Vector3 flat = Vector3{target.x, 0.0f, target.z};
        float maxHalf = std::max(halfLength, halfWidth);
        float limit = std::max(0.1f, arenaRadius_ - maxHalf - 0.25f);
        if (Vector3Length(flat) > limit) {
            flat = Vector3Scale(Vector3Normalize(flat), limit);
            target.x = flat.x;
            target.z = flat.z;
        }
    }
    float centerY = std::clamp(target.y, 1.2f, 16.0f);
    Vector3 position = Vector3{target.x, centerY - scale.y * 0.5f, target.z};
    Vector3 normal = Vector3{0.0f, 1.0f, 0.0f};
    Vector3 platformRight = Vector3CrossProduct(forward, normal);
    if (Vector3Length(platformRight) <= 0.001f) {
        platformRight = PlayerRight();
    } else {
        platformRight = Vector3Normalize(platformRight);
    }
    Vector3 platformForward = Vector3Normalize(Vector3CrossProduct(normal, platformRight));
    nanoPlatforms_.push_back(NanoPlatform{position, scale, normal, platformRight, platformForward, config_.nanoPlatformDelay, config_.nanoPlatformLifetime * 0.45f, config_.nanoPlatformLifetime * 0.45f, world});
    SpawnHitBurst(Vector3{position.x, position.y + scale.y, position.z}, Color{255, 220, 115, 255}, 8);
}
void Game::FireBossRing(Vector3 position, int count, float speedScale, int world) {
    PlaySfxAt(sfxBossBarrage_, position, 100.0f, std::clamp(0.45f + count * 0.045f, 0.55f, 1.0f));
    float spin = static_cast<float>(GetTime()) * 0.65f;
    Vector3 up = UpForWorldAt(position, world);
    Vector3 basisA = SafeNormalize(ProjectOnSphericalTangent(camera_.position, up), Vector3{1.0f, 0.0f, 0.0f});
    if (!IsSphericalMap()) {
        basisA = Vector3{1.0f, 0.0f, 0.0f};
    }
    Vector3 basisB = SafeNormalize(Vector3CrossProduct(up, basisA), Vector3{0.0f, 0.0f, 1.0f});
    for (int i = 0; i < count; ++i) {
        float angle = spin + (static_cast<float>(i) / static_cast<float>(count)) * 6.2831853f;
        Vector3 direction = Vector3Normalize(Vector3Add(Vector3Add(Vector3Scale(basisA, std::cos(angle)), Vector3Scale(basisB, std::sin(angle))), Vector3Scale(up, 0.08f)));
        Vector3 intendedVelocity = Vector3Scale(direction, config_.enemyShotSpeed * speedScale);

        PhysicsWorld::BodyConfig projectileConfig;
        projectileConfig.motionType = JPH::EMotionType::Dynamic;
        projectileConfig.layer = Layers::PROJECTILE;
        projectileConfig.linearVelocity = timeStopped_ ? JPH::Vec3::sZero() : JPH::Vec3(intendedVelocity.x, intendedVelocity.y, intendedVelocity.z);
        projectileConfig.gravityFactor = 0.0f;
        projectileConfig.linearDamping = 0.0f;
        projectileConfig.motionQuality = JPH::EMotionQuality::LinearCast;
        projectileConfig.allowSleeping = false;

        JPH::BodyID body = physics_.CreateBody(projectileShape_, ToJoltVector(position), JPH::Quat::sIdentity(), projectileConfig);
        projectiles_.push_back(Projectile{body, ProjectileKind::EnemyShot, 4.2f, 4.2f, config_.enemyShotDamage, 0.24f, 0.24f, Color{180, 125, 255, 255}, 0, intendedVelocity, ProjectileOwner::Enemy, timeStopped_, {}, 0.0f, false, world});
    }
    SpawnShockwave(position, 4.8f, Color{170, 115, 255, 255});
}
void Game::FireDuelistHeatwave(Vector3 origin, Vector3 direction) {
    Vector3 forward = Vector3Normalize(direction);
    float range = config_.heatwaveRange * 0.75f;
    for (Projectile& projectile : projectiles_) {
        if (projectile.owner != ProjectileOwner::Player) {
            continue;
        }
        Vector3 position = BodyPosition(projectile.body);
        Vector3 offset = Vector3Subtract(position, origin);
        float distance = Vector3Length(offset);
        if (distance <= 0.05f || distance > range) {
            continue;
        }
        Vector3 toProjectile = Vector3Scale(offset, 1.0f / distance);
        if (Vector3DotProduct(forward, toProjectile) < 0.45f) {
            continue;
        }
        AddProjectileImpulse(projectile, Vector3Scale(toProjectile, config_.heatwaveForce * 0.75f));
    }
    heatwaves_.push_back(HeatwavePulse{origin, forward, 0.18f, 0.18f, range, 0.8f, Color{255, 150, 70, 255}});
}
void Game::DetonateSpear(Vector3 position, ProjectileOwner owner) {
    PlaySfxAt(sfxSpearImpact_, position, 70.0f, 0.95f);
    float radius = config_.longinusSpearShockwaveRadius;
    if (owner == ProjectileOwner::Player) {
        DamageThroneAngelInRadius(position, radius, config_.longinusSpearShockwaveDamage, Color{255, 220, 150, 255});
        DamageSeraphInRadius(position, radius, config_.longinusSpearShockwaveDamage, Color{255, 230, 150, 255});
        DamageWarRiderInRadius(position, radius, config_.longinusSpearShockwaveDamage, Color{255, 110, 70, 255});
        DamageConquestRiderInRadius(position, radius, config_.longinusSpearShockwaveDamage, Color{185, 255, 100, 255});
        DamageFamineRiderInRadius(position, radius, config_.longinusSpearShockwaveDamage, Color{210, 178, 100, 255});
        DamageDeathRiderInRadius(position, radius, config_.longinusSpearShockwaveDamage, Color{170, 174, 190, 255});
        DamageDeathSkullsInRadius(position, radius, config_.longinusSpearShockwaveDamage, Color{230, 230, 220, 255});
        for (size_t c = 0; c < cherubs_.size();) {
            float distance = Vector3Distance(cherubs_[c].position, position);
            if (distance <= radius + 0.58f) {
                float falloff = 1.0f - std::clamp(distance / std::max(0.001f, radius), 0.0f, 1.0f);
                float dealt = config_.longinusSpearShockwaveDamage * (0.25f + falloff * 0.75f);
                cherubs_[c].health -= dealt;
                totalDamageDealt_ += dealt;
                cherubs_[c].flashTimer = 0.18f;
                if (cherubs_[c].health <= 0.0f) {
                    SpawnHitBurst(cherubs_[c].position, Color{245, 245, 235, 255}, 18);
                    cherubs_[c] = cherubs_.back();
                    cherubs_.pop_back();
                    continue;
                }
            }
            ++c;
        }
        for (size_t i = 0; i < enemies_.size();) {
            Enemy& enemy = enemies_[i];
            Vector3 enemyPosition = BodyPosition(enemy.body);
            float distance = Vector3Distance(enemyPosition, position);
            if (distance <= radius + enemy.radius) {
                float falloff = 1.0f - std::clamp(distance / std::max(0.001f, radius), 0.0f, 1.0f);
                enemy.health -= config_.longinusSpearShockwaveDamage * (0.25f + falloff * 0.75f);
                totalDamageDealt_ += config_.longinusSpearShockwaveDamage * (0.25f + falloff * 0.75f);
                PlayEnemyHitSfx(enemyPosition);
                if (enemy.type == EnemyType::Dummy || enemy.type == EnemyType::DummyBoss) {
                    RecordDummyDamage(enemy, config_.longinusSpearShockwaveDamage * (0.25f + falloff * 0.75f));
                }
                Vector3 direction = Vector3Subtract(enemyPosition, position);
                if (Vector3Length(direction) <= 0.001f) {
                    direction = UpForWorldAt(position, enemy.world);
                } else {
                    direction = Vector3Normalize(direction);
                }
                AddEnemyImpulse(enemy, Vector3Scale(direction, config_.longinusSpearShockwaveForce * (0.35f + falloff * 0.85f)));
                SpawnHitBurst(enemyPosition, Color{255, 150, 40, 255}, 7);
                if (enemy.health <= 0.0f) {
                    score_ += enemy.scoreValue;
                    SpawnHitBurst(enemyPosition, Color{255, 200, 80, 255}, 18);
                    DestroyEnemy(i);
                    continue;
                }
            }
            ++i;
        }
    } else if (Vector3Distance(camera_.position, position) <= radius + playerRadius_) {
        ApplyPlayerHit(camera_.position, Color{255, 150, 40, 255});
    }

    SpawnShockwave(position, radius, Color{255, 150, 40, 255});
    SpawnHitBurst(position, Color{255, 200, 80, 255}, 28);
    beams_.push_back(Beam{
        position,
        Vector3Add(position, Vector3Scale(UpForWorldAt(position, owner == ProjectileOwner::Player ? playerWorld_ : 0), 0.01f)),
        0.16f,
        0.16f,
        radius * 1.4f,
        1.0f,
        Color{255, 150, 40, 255}
    });
}
void Game::SpawnEnemyNanoBlade(Vector3 origin, Vector3 direction, int world) {
    Vector3 forward = Vector3Normalize(direction);
    Vector3 localUp = UpForWorldAt(origin, world);
    Vector3 planeNormal = Vector3Normalize(Vector3CrossProduct(forward, localUp));
    if (Vector3Length(planeNormal) <= 0.001f) {
        planeNormal = Vector3{1.0f, 0.0f, 0.0f};
    }
    Vector3 up = Vector3Normalize(Vector3CrossProduct(planeNormal, forward));
    Vector3 center = Vector3Add(origin, Vector3Scale(forward, config_.nanoBladeWaveSpawnDistance));
    nanoBlades_.push_back(NanoBlade{center, planeNormal, forward, up, Vector3Scale(forward, config_.nanoBladeWaveSpeed * 0.82f), config_.nanoBladeDelay, config_.nanoBladeLifetime, config_.nanoBladeLifetime, config_.nanoBladeRadius, config_.nanoBladeThickness, config_.nanoBladePlaneThickness, config_.nanoBladeDamage / config_.nanoBladeLifetime, ProjectileOwner::Enemy, world});
    SpawnHitBurst(origin, Color{255, 210, 120, 255}, 6);
}
void Game::SpawnGravityWell(Vector3 position, bool blackHole, bool enemyOrigin) {
    PlaySfxAt(blackHole ? sfxBlackHoleOpen_ : sfxGravityWellOpen_, position, blackHole ? 92.0f : 58.0f, blackHole ? 1.0f : 0.82f);
    float lifetime = blackHole ? config_.blackHoleLifetime : config_.gravityWellLifetime;
    float radius = blackHole ? config_.blackHoleRadius : config_.gravityWellRadius;
    float force = blackHole ? config_.blackHoleForce : config_.gravityWellForce;
    float damage = blackHole ? config_.blackHoleGrenadeDamage : 0.0f;
    gravityWells_.push_back(GravityWell{position, lifetime, lifetime, radius, force, damage, blackHole, enemyOrigin});
    SpawnHitBurst(position, blackHole ? Color{130, 70, 220, 255} : Color{150, 185, 255, 255}, blackHole ? 34 : 22);
    beams_.push_back(Beam{
        position,
        Vector3Add(position, Vector3{0.0f, 0.01f, 0.0f}),
        blackHole ? 0.32f : 0.2f,
        blackHole ? 0.32f : 0.2f,
        radius * (blackHole ? 1.85f : 1.45f),
        blackHole ? 1.0f : 0.8f,
        blackHole ? Color{95, 45, 180, 255} : Color{135, 105, 255, 255}
    });
    if (blackHole) {
        SpawnShockwave(position, radius, Color{120, 65, 220, 255});
        cameraShake_ = std::min(1.0f, cameraShake_ + 0.5f);
    }
}
void Game::SpawnShockwave(Vector3 position, float radius, Color color) {
    shockwaves_.push_back(Shockwave{position, 0.34f, 0.34f, radius, color});
}
void Game::ExplodeRocket(Vector3 position, ProjectileOwner owner, JPH::BodyID shooterBody) {
    if (owner == ProjectileOwner::Player) {
        PlaySfxAt(sfxRocketExplosion_, position, 70.0f, 0.9f);
    } else {
        PlaySfxAt(sfxRocketExplosion_, position, 78.0f, 1.0f);
    }
    float radius = config_.rocketExplosionRadius;
    if (owner == ProjectileOwner::Player) {
        DamageScavengerUfoInRadius(position, radius, config_.rocketExplosionDamage, Color{255, 190, 70, 255});
        DamageThroneAngelInRadius(position, radius, config_.rocketExplosionDamage, Color{255, 220, 160, 255});
        DamageSeraphInRadius(position, radius, config_.rocketExplosionDamage, Color{255, 225, 130, 255});
        DamageWarRiderInRadius(position, radius, config_.rocketExplosionDamage, Color{255, 105, 60, 255});
        DamageConquestRiderInRadius(position, radius, config_.rocketExplosionDamage, Color{175, 255, 95, 255});
        DamageFamineRiderInRadius(position, radius, config_.rocketExplosionDamage, Color{210, 178, 100, 255});
        DamageDeathRiderInRadius(position, radius, config_.rocketExplosionDamage, Color{166, 170, 184, 255});
        DamageDeathSkullsInRadius(position, radius, config_.rocketExplosionDamage, Color{235, 235, 225, 255});
        for (size_t c = 0; c < cherubs_.size();) {
            float distance = Vector3Distance(cherubs_[c].position, position);
            if (distance <= radius + 0.58f) {
                float falloff = 1.0f - std::clamp(distance / std::max(0.001f, radius), 0.0f, 1.0f);
                float dealt = config_.rocketExplosionDamage * (0.35f + falloff * 0.65f);
                cherubs_[c].health -= dealt;
                totalDamageDealt_ += dealt;
                cherubs_[c].flashTimer = 0.18f;
                if (cherubs_[c].health <= 0.0f) {
                    SpawnHitBurst(cherubs_[c].position, Color{245, 245, 235, 255}, 18);
                    cherubs_[c] = cherubs_.back();
                    cherubs_.pop_back();
                    continue;
                }
            }
            ++c;
        }
        for (size_t i = 0; i < enemies_.size();) {
            Vector3 enemyPosition = BodyPosition(enemies_[i].body);
            float distance = Vector3Distance(enemyPosition, position);
            if (distance <= radius + enemies_[i].radius) {
                float falloff = 1.0f - std::clamp(distance / std::max(0.001f, radius), 0.0f, 1.0f);
                enemies_[i].health -= config_.rocketExplosionDamage * (0.35f + falloff * 0.65f);
                totalDamageDealt_ += config_.rocketExplosionDamage * (0.35f + falloff * 0.65f);
                if (enemies_[i].type == EnemyType::Dummy || enemies_[i].type == EnemyType::DummyBoss) {
                    RecordDummyDamage(enemies_[i], config_.rocketExplosionDamage * (0.35f + falloff * 0.65f));
                }
                PlayEnemyHitSfx(enemyPosition);
                SpawnHitBurst(enemyPosition, Color{255, 150, 45, 255}, 14);
                if (enemies_[i].health <= 0.0f) {
                    score_ += enemies_[i].scoreValue;
                    SpawnHitBurst(enemyPosition, Color{255, 245, 210, 255}, 20);
                    DestroyEnemy(i);
                    continue;
                }
            }
            ++i;
        }
    } else if (Vector3Distance(camera_.position, position) <= radius + playerRadius_) {
        ApplyPlayerHit(camera_.position, Color{255, 160, 70, 255});
    }

    if (owner == ProjectileOwner::Enemy) {
        for (size_t i = 0; i < enemies_.size();) {
            if (enemies_[i].type != EnemyType::Duelist) {
                ++i;
                continue;
            }
            // Skip the duelist that fired this rocket
            if (!shooterBody.IsInvalid() && enemies_[i].body == shooterBody) {
                ++i;
                continue;
            }
            Vector3 enemyPosition = BodyPosition(enemies_[i].body);
            float distance = Vector3Distance(enemyPosition, position);
            if (distance <= radius + enemies_[i].radius) {
                float falloff = 1.0f - std::clamp(distance / std::max(0.001f, radius), 0.0f, 1.0f);
                enemies_[i].health -= config_.rocketExplosionDamage * 0.5f * (0.35f + falloff * 0.65f);
                PlayEnemyHitSfx(enemyPosition);
                if (enemies_[i].health <= 0.0f) {
                    score_ += enemies_[i].scoreValue;
                    SpawnHitBurst(enemyPosition, Color{255, 245, 210, 255}, 20);
                    DestroyEnemy(i);
                    continue;
                }
            }
            ++i;
        }
    }

    SpawnHitBurst(position, Color{255, 190, 70, 255}, 68);
    SpawnHitBurst(position, Color{255, 245, 190, 255}, 22);
    SpawnHitBurst(position, Color{80, 70, 64, 255}, 34);
    beams_.push_back(Beam{
        position,
        Vector3Add(position, Vector3{0.0f, 0.01f, 0.0f}),
        0.18f,
        0.18f,
        radius * 2.2f,
        1.0f,
        Color{255, 155, 45, 255}
    });
    ApplyExplosionImpulse(position, config_.rocketJumpRadius, config_.rocketJumpImpulse);
    cameraShake_ = std::min(1.0f, cameraShake_ + 1.0f);
}

void Game::ExplodeUfoOrb(Vector3 position, int world, bool playerOwned, float damage) {
    PlaySfxAt(sfxBallLightningExplosion_, position, 78.0f, 0.28f);
    float radius = config_.ufoOrbExplosionRadius;
    float damageRadius = radius * config_.ufoOrbDamageRadiusMult;
    float explosionDamage = playerOwned ? config_.ufoPilotOrbExplosionDamage : config_.ufoOrbExplosionDamage;
    if (!playerOwned && playerWorld_ == world && Vector3Distance(camera_.position, position) <= damageRadius + playerRadius_) {
        ApplyPlayerHit(camera_.position, Color{120, 235, 255, 255}, "ORB BURST");
    }
    if (playerOwned && world == 0) {
        DamageThroneAngelInRadius(position, damageRadius, explosionDamage, Color{130, 240, 255, 255});
        DamageSeraphInRadius(position, damageRadius, explosionDamage, Color{170, 245, 255, 255});
        DamageWarRiderInRadius(position, damageRadius, explosionDamage, Color{120, 240, 255, 255});
        DamageConquestRiderInRadius(position, damageRadius, explosionDamage, Color{165, 255, 120, 255});
        DamageFamineRiderInRadius(position, damageRadius, explosionDamage, Color{150, 220, 255, 255});
        DamageDeathRiderInRadius(position, damageRadius, explosionDamage, Color{150, 220, 255, 255});
        DamageDeathSkullsInRadius(position, damageRadius, explosionDamage, Color{210, 245, 255, 255});
        for (size_t c = 0; c < cherubs_.size();) {
            float distance = Vector3Distance(cherubs_[c].position, position);
            if (distance <= damageRadius + 0.58f) {
                float falloff = 1.0f - std::clamp(distance / std::max(0.001f, damageRadius), 0.0f, 1.0f);
                float dealt = explosionDamage * (0.35f + falloff * 0.65f);
                cherubs_[c].health -= dealt;
                totalDamageDealt_ += dealt;
                cherubs_[c].flashTimer = 0.18f;
                if (cherubs_[c].health <= 0.0f) {
                    SpawnHitBurst(cherubs_[c].position, Color{245, 245, 235, 255}, 18);
                    cherubs_[c] = cherubs_.back();
                    cherubs_.pop_back();
                    continue;
                }
            }
            ++c;
        }
    }
    for (size_t i = 0; i < enemies_.size();) {
        if (enemies_[i].world != world) { ++i; continue; }
        Vector3 enemyPosition = BodyPosition(enemies_[i].body);
        float distance = Vector3Distance(enemyPosition, position);
        if (distance <= damageRadius + enemies_[i].radius) {
            float falloff = 1.0f - std::clamp(distance / std::max(0.001f, damageRadius), 0.0f, 1.0f);
            float dealt = explosionDamage * (0.35f + falloff * 0.65f);
            enemies_[i].health -= dealt;
            if (playerOwned) {
                totalDamageDealt_ += dealt;
            }
            PlayEnemyHitSfx(enemyPosition);
            SpawnHitBurst(enemyPosition, Color{110, 235, 255, 255}, 9);
            if (enemies_[i].health <= 0.0f) {
                score_ += enemies_[i].scoreValue;
                SpawnHitBurst(enemyPosition, Color{180, 245, 255, 255}, 18);
                DestroyEnemy(i);
                continue;
            }
        }
        ++i;
    }
    SpawnHitBurst(position, Color{110, 235, 255, 255}, 35);
    SpawnShockwave(position, radius, Color{110, 235, 255, 255});
    cameraShake_ = std::min(1.0f, cameraShake_ + 0.4f);
}

void Game::FireBallLightning() {
    Vector3 forward = PlayerForward();
    Vector3 spawn = WeaponMuzzlePosition();
    Vector3 velocity = Vector3Scale(forward, config_.superBallSpeed);

    ballLightnings_.push_back(BallLightning{
        spawn,
        velocity,
        config_.superBallLifetime,
        config_.superBallLifetime,
        config_.superBallRadius,
        0.0f,   // fire immediately on first frame
        0.0f,   // hue starts at 0
        playerWorld_
    });

    cameraShake_ = std::min(1.0f, cameraShake_ + 0.7f);
    SpawnHitBurst(spawn, Color{255, 215, 60, 255}, 60);
    SpawnShockwave(spawn, config_.superBallRadius * 1.5f, Color{255, 200, 60, 255});
    eventText_ = "BALL LIGHTNING";
    eventTextTimer_ = 2.0f;
}

void Game::UpdateBallLightnings(float dt) {
    bool humActive = false;
    Vector3 humPosition = {};
    float humDistance = INFINITY;

    for (size_t i = 0; i < ballLightnings_.size();) {
        BallLightning& ball = ballLightnings_[i];

        // Straight-line flight (no gravity, no surface tracking)
        ball.position = Vector3Add(ball.position, Vector3Scale(ball.velocity, dt));

        // Out of bounds check
        bool outOfBounds = false;
        if (IsSphericalMap()) {
            outOfBounds = SphericalOutOfBounds(ball.position, 6.0f, ball.world);
        } else if (IsEdenMap()) {
            outOfBounds = DistanceXZ(ball.position, Vector3Zero()) > EdenCombatBoundaryRadius();
        } else {
            outOfBounds = IsSquareMap()
                ? (std::abs(ball.position.x) > squareHalfExtent_ + 6.0f || std::abs(ball.position.z) > squareHalfExtent_ + 6.0f)
                : DistanceXZ(ball.position, Vector3Zero()) > arenaRadius_ + 6.0f;
        }

        ball.life -= dt;
        if (ball.life <= 0.0f || outOfBounds) {
            ExplodeBallLightning(ball);
            ballLightnings_[i] = ballLightnings_.back();
            ballLightnings_.pop_back();
            continue;
        }

        // Hue cycling (rainbow effect)
        ball.hue = std::fmod(ball.hue + dt * 45.0f, 360.0f);

        float distanceToPlayer = Vector3Distance(ball.position, camera_.position);
        if (distanceToPlayer < humDistance) {
            humDistance = distanceToPlayer;
            humPosition = ball.position;
            humActive = true;
        }

        // Fire laser beams at nearest enemy periodically
        ball.fireTimer -= dt;
        if (ball.fireTimer <= 0.0f) {
            ball.fireTimer = config_.superBallFireInterval;

            // Find nearest enemy within range, same world
            float bestDist = config_.superBallBeamRange;
            Vector3 bestTarget = {};
            bool found = false;
            for (const Enemy& enemy : enemies_) {
                if (enemy.world != ball.world) continue;
                Vector3 ep = BodyPosition(enemy.body);
                float dist = Vector3Distance(ball.position, ep);
                if (dist < bestDist) {
                    bestDist = dist;
                    bestTarget = ep;
                    found = true;
                }
            }
            if (throneAngel_.active && ball.world == 0) {
                float dist = Vector3Distance(ball.position, throneAngel_.position);
                if (dist < bestDist) {
                    bestDist = dist;
                    bestTarget = throneAngel_.position;
                    found = true;
                }
            }
            if (ball.world == 0) {
                for (const SeraphBoss& seraph : seraphs_) {
                    if (!seraph.active) continue;
                    float dist = Vector3Distance(ball.position, seraph.position);
                    if (dist < bestDist) {
                        bestDist = dist;
                        bestTarget = seraph.position;
                        found = true;
                    }
                }
            }
            for (const CherubMinion& cherub : cherubs_) {
                if (cherub.world != ball.world) continue;
                float dist = Vector3Distance(ball.position, cherub.position);
                if (dist < bestDist) {
                    bestDist = dist;
                    bestTarget = cherub.position;
                    found = true;
                }
            }

            if (found) {
                Vector3 dir = Vector3Normalize(Vector3Subtract(bestTarget, ball.position));
                Vector3 beamEnd = Vector3Add(ball.position, Vector3Scale(dir, bestDist + 3.0f));
                Color beamColor = ColorFromHSV(ball.hue, 0.9f, 1.0f);
                float beamWidth = config_.superBallBeamWidth;
                beams_.push_back(Beam{
                    ball.position,
                    beamEnd,
                    0.25f,
                    0.25f,
                    beamWidth,
                    0.5f,
                    beamColor,
                    0.0f,        // no sustained damage (instant hit)
                    0.0f,        // no rainbow (color already computed)
                    ball.world
                });

                // Instant damage to the targeted enemy
                if (throneAngel_.active && ball.world == 0
                    && DistancePointToSegment(throneAngel_.position, ball.position, beamEnd) <= beamWidth + 4.2f) {
                    DamageThroneAngel(config_.superBallBeamDamage, throneAngel_.position, beamColor);
                }
                if (warRider_.active && ball.world == warRider_.world
                    && DistancePointToSegment(warRider_.position, ball.position, beamEnd) <= beamWidth + kHorsemanHitPadding) {
                    DamageWarRider(config_.superBallBeamDamage, warRider_.position, beamColor);
                }
                if (conquestRider_.active && ball.world == conquestRider_.world
                    && DistancePointToSegment(conquestRider_.position, ball.position, beamEnd) <= beamWidth + kHorsemanHitPadding) {
                    DamageConquestRider(config_.superBallBeamDamage, conquestRider_.position, beamColor);
                }
                if (famineRider_.active && ball.world == famineRider_.world
                    && DistancePointToSegment(famineRider_.position, ball.position, beamEnd) <= beamWidth + kHorsemanHitPadding) {
                    DamageFamineRider(config_.superBallBeamDamage, famineRider_.position, beamColor);
                }
                if (deathRider_.active && ball.world == deathRider_.world
                    && DistancePointToSegment(deathRider_.position, ball.position, beamEnd) <= beamWidth + kDeathRiderHitPadding) {
                    DamageDeathRider(config_.superBallBeamDamage, deathRider_.position, beamColor);
                }
                if (ball.world == 0) {
                    for (const SeraphBoss& seraph : seraphs_) {
                        if (!seraph.active) continue;
                        if (DistancePointToSegment(seraph.position, ball.position, beamEnd) <= beamWidth + 3.6f) {
                            DamageSeraph(config_.superBallBeamDamage, seraph.position, beamColor);
                        }
                    }
                }
                for (size_t c = 0; c < cherubs_.size();) {
                    if (cherubs_[c].world != ball.world) { ++c; continue; }
                    if (DistancePointToSegment(cherubs_[c].position, ball.position, beamEnd) <= beamWidth + 0.55f) {
                        cherubs_[c].health -= config_.superBallBeamDamage;
                        totalDamageDealt_ += config_.superBallBeamDamage;
                        cherubs_[c].flashTimer = 0.18f;
                        if (cherubs_[c].health <= 0.0f) {
                            SpawnHitBurst(cherubs_[c].position, Color{245, 245, 235, 255}, 18);
                            cherubs_[c] = cherubs_.back();
                            cherubs_.pop_back();
                            continue;
                        }
                    }
                    ++c;
                }
                for (size_t e = 0; e < enemies_.size(); ++e) {
                    Vector3 ep = BodyPosition(enemies_[e].body);
                    if (DistancePointToSegment(ep, ball.position, beamEnd) <= beamWidth + enemies_[e].radius * 0.85f) {
                        enemies_[e].health -= config_.superBallBeamDamage;
                        totalDamageDealt_ += config_.superBallBeamDamage;
                        SpawnHitBurst(ep, beamColor, 8);
                        if (enemies_[e].health <= 0.0f) {
                            score_ += enemies_[e].scoreValue;
                            SpawnHitBurst(ep, Color{255, 255, 255, 255}, 22);
                            DestroyEnemy(e);
                            break;  // index may be invalidated
                        }
                    }
                }
                PlaySfxAt(sfxLaserPlasma_, ball.position, 100.0f, 1.0f);
            }
        }

        // Spawn trailing particles
        if (RandomFloat(0.0f, 1.0f) < 0.6f) {
            Color trailColor = ColorFromHSV(ball.hue, 0.85f, 1.0f);
            trailColor.a = 180;
            particles_.push_back(Particle{
                ball.position,
                Vector3{RandomFloat(-1.5f, 1.5f), RandomFloat(-1.5f, 1.5f), RandomFloat(-1.5f, 1.5f)},
                trailColor,
                RandomFloat(0.15f, 0.35f), RandomFloat(0.15f, 0.35f),
                RandomFloat(0.04f, 0.1f)
            });
        }

        ++i;
    }

    if (humActive) {
        UpdateLoopingSfxAt(sfxBallLightningHum_, humPosition, 110.0f, 0.85f);
    } else {
        StopSfx(sfxBallLightningHum_);
    }
}

void Game::ExplodeBallLightning(BallLightning& ball) {
    PlaySfxAt(sfxBallLightningExplosion_, ball.position, 90.0f, 10.0f);
    TriggerScavengerUfo(ball.position);
    float radius = config_.superBallExplosionRadius;
    float damage = config_.superBallExplosionDamage;
    DamageScavengerUfoInRadius(ball.position, radius, damage, ColorFromHSV(ball.hue, 0.9f, 1.0f));
    DamageThroneAngelInRadius(ball.position, radius, damage, ColorFromHSV(ball.hue, 0.45f, 1.0f));
    DamageSeraphInRadius(ball.position, radius, damage, ColorFromHSV(ball.hue, 0.55f, 1.0f));
    DamageWarRiderInRadius(ball.position, radius, damage, ColorFromHSV(ball.hue, 0.75f, 1.0f));
    DamageConquestRiderInRadius(ball.position, radius, damage, ColorFromHSV(ball.hue, 0.55f, 1.0f));
    DamageFamineRiderInRadius(ball.position, radius, damage, ColorFromHSV(ball.hue, 0.35f, 1.0f));
    DamageDeathRiderInRadius(ball.position, radius, damage, ColorFromHSV(ball.hue, 0.15f, 1.0f));
    DamageDeathSkullsInRadius(ball.position, radius, damage, ColorFromHSV(ball.hue, 0.2f, 1.0f));
    for (size_t c = 0; c < cherubs_.size();) {
        float distance = Vector3Distance(cherubs_[c].position, ball.position);
        if (distance <= radius + 0.58f) {
            float falloff = 1.0f - std::clamp(distance / std::max(0.001f, radius), 0.0f, 1.0f);
            float dealt = damage * (0.35f + falloff * 0.65f);
            cherubs_[c].health -= dealt;
            totalDamageDealt_ += dealt;
            cherubs_[c].flashTimer = 0.18f;
            if (cherubs_[c].health <= 0.0f) {
                SpawnHitBurst(cherubs_[c].position, Color{245, 245, 235, 255}, 18);
                cherubs_[c] = cherubs_.back();
                cherubs_.pop_back();
                continue;
            }
        }
        ++c;
    }

    // Damage enemies in radius
    for (size_t i = 0; i < enemies_.size();) {
        Vector3 enemyPosition = BodyPosition(enemies_[i].body);
        float distance = Vector3Distance(enemyPosition, ball.position);
        if (distance <= radius + enemies_[i].radius) {
            float falloff = 1.0f - std::clamp(distance / std::max(0.001f, radius), 0.0f, 1.0f);
            enemies_[i].health -= damage * (0.35f + falloff * 0.65f);
            totalDamageDealt_ += damage * (0.35f + falloff * 0.65f);
            if (enemies_[i].type == EnemyType::Dummy || enemies_[i].type == EnemyType::DummyBoss) {
                RecordDummyDamage(enemies_[i], damage * (0.35f + falloff * 0.65f));
            }
            SpawnHitBurst(enemyPosition, ColorFromHSV(ball.hue, 0.9f, 1.0f), 14);
            if (enemies_[i].health <= 0.0f) {
                score_ += enemies_[i].scoreValue;
                SpawnHitBurst(enemyPosition, Color{255, 255, 255, 255}, 20);
                DestroyEnemy(i);
                continue;
            }
        }
        ++i;
    }

    // Big visual explosion
    Color expColor = ColorFromHSV(ball.hue, 0.9f, 1.0f);
    SpawnHitBurst(ball.position, Color{255, 215, 60, 255}, 80);   // gold
    SpawnHitBurst(ball.position, expColor, 50);                     // rainbow
    SpawnHitBurst(ball.position, Color{255, 255, 255, 255}, 30);   // white

    // Ring flash
    beams_.push_back(Beam{
        ball.position,
        Vector3Add(ball.position, Vector3{0.0f, 0.01f, 0.0f}),
        0.25f, 0.25f,
        radius * 2.2f,
        1.0f,
        expColor
    });

    SpawnShockwave(ball.position, radius, expColor);
    cameraShake_ = std::min(1.0f, cameraShake_ + 1.0f);
}

void Game::FireSuperRainbowBeam(float chargeRatio) {
    Vector3 forward = PlayerForward();
    Vector3 start = WeaponMuzzlePosition();
    Vector3 end = Vector3Add(start, Vector3Scale(forward, config_.laserBeamRange * 1.3f));
    float beamLife = config_.superRainbowBeamBaseLife + static_cast<float>(superEssenceConsumed_) * config_.superRainbowBeamLifePerEssence;
    float damage = config_.laserBaseDamage + chargeRatio * config_.laserChargeDamage * 5.0f;
    float beamWidth = config_.superRainbowBeamWidthBase + static_cast<float>(superEssenceConsumed_) * config_.superRainbowBeamWidthPerEssence;

    // DPS — total damage scales with beamLife (longer beam = more total damage)
    float damagePerFrame = damage;

    beams_.push_back(Beam{
        start,
        end,
        beamLife,
        beamLife,
        beamWidth,
        chargeRatio,
        ColorFromHSV(0.0f, 0.9f, 1.0f),  // initial color (hue cycles in render)
        damagePerFrame,
        1.0f,  // hue marker for rainbow rendering
        playerWorld_,
        true
    });

    // Stretch laser beam sound to match rainbow beam duration (via alias)
    if (sfxLaserBeamAlias_[0].frameCount > 0) {
        Sound& alias = sfxLaserBeamAlias_[sfxLaserBeamAliasIdx_];
        sfxLaserBeamAliasIdx_ = (sfxLaserBeamAliasIdx_ + 1) % 4;
        float pitch = 0.5f / std::max(0.05f, beamLife);
        SetSoundPitch(alias, pitch);
        PlaySound(alias);
    }

    cameraShake_ = std::min(1.0f, cameraShake_ + 0.45f + chargeRatio * 0.35f);
    eventText_ = "RAINBOW BEAM";
    eventTextTimer_ = 1.0f;
}

void Game::SpawnWaterDroplet(Vector3 position, int world) {
    WaterDroplet droplet;
    droplet.position = position;
    droplet.velocity = Vector3Zero();
    droplet.hasTarget = false;
    droplet.life = config_.waterDropletLifetime;
    droplet.maxLife = config_.waterDropletLifetime;
    droplet.radius = config_.waterDropletRadius;
    droplet.damage = config_.waterDropletDamage;
    droplet.reacquireTimer = 0.0f;
    droplet.world = world;
    waterDroplets_.push_back(droplet);
}

void Game::UpdateWaterDroplets(float dt) {
    for (size_t i = 0; i < waterDroplets_.size();) {
        WaterDroplet& droplet = waterDroplets_[i];

        // Re-acquire target
        droplet.reacquireTimer -= dt;
        if (!droplet.hasTarget || droplet.reacquireTimer <= 0.0f) {
            droplet.reacquireTimer = 0.3f;
            droplet.hasTarget = false;
            float bestDist = INFINITY;
            for (const Enemy& enemy : enemies_) {
                if (enemy.world != droplet.world) continue;
                Vector3 ep = BodyPosition(enemy.body);
                float d = Vector3Distance(droplet.position, ep);
                if (d < bestDist) {
                    bestDist = d;
                    droplet.targetPos = ep;
                    droplet.hasTarget = true;
                }
            }
        }

        // Steer toward target
        if (droplet.hasTarget) {
            Vector3 toTarget = Vector3Subtract(droplet.targetPos, droplet.position);
            float dist = Vector3Length(toTarget);
            if (dist > 0.001f) {
                Vector3 desired = Vector3Scale(Vector3Normalize(toTarget), config_.waterDropletSpeed);
                // Smooth steering
                float blend = std::min(1.0f, dt * config_.waterDropletTurnRate);
                droplet.velocity = Vector3Add(
                    Vector3Scale(droplet.velocity, 1.0f - blend),
                    Vector3Scale(desired, blend));
            }
        }

        // Move (straight-line flight on all map types)
        droplet.position = Vector3Add(droplet.position, Vector3Scale(droplet.velocity, dt));

        // Minimum altitude clamp — just above surface so droplet can reach all enemies
        if (IsSphericalMap()) {
            float alt = SphericalAltitudeAt(droplet.position, droplet.world);
            float minAlt = droplet.radius + 0.2f;
            if (alt < minAlt) {
                droplet.position = SphericalSurfacePoint(droplet.position, minAlt, droplet.world);
            }
        } else {
            float groundY = FlatGroundYForWorld(droplet.world);
            if (droplet.position.y < groundY + droplet.radius) {
                droplet.position.y = groundY + droplet.radius;
            }
        }

        // Trail history
        droplet.trailTimer += dt;
        if (droplet.trailTimer >= 0.03f) {
            droplet.trailTimer = 0.0f;
            droplet.trailHistory[droplet.trailHead] = droplet.position;
            droplet.trailHead = (droplet.trailHead + 1) % 8;
            if (droplet.trailCount < 8) droplet.trailCount++;
        }

        // Contact damage against enemies
        if (ScavengerUfoDamageable() && droplet.world == 0
            && Vector3Distance(droplet.position, scavengerUfo_.position) <= droplet.radius + 2.6f) {
            DamageScavengerUfo(droplet.damage * dt * 8.0f, droplet.position, Color{255, 220, 80, 255});
        }
        if (throneAngel_.active && droplet.world == 0
            && Vector3Distance(droplet.position, throneAngel_.position) <= droplet.radius + 4.2f) {
            DamageThroneAngel(droplet.damage * dt * 8.0f, droplet.position, Color{255, 245, 160, 255});
        }
        if (warRider_.active && droplet.world == warRider_.world
            && Vector3Distance(droplet.position, warRider_.position) <= droplet.radius + kHorsemanHitPadding) {
            DamageWarRider(droplet.damage * dt * 8.0f, droplet.position, Color{255, 120, 70, 255});
        }
        if (conquestRider_.active && droplet.world == conquestRider_.world
            && Vector3Distance(droplet.position, conquestRider_.position) <= droplet.radius + kHorsemanHitPadding) {
            DamageConquestRider(droplet.damage * dt * 8.0f, droplet.position, Color{180, 255, 100, 255});
        }
        if (famineRider_.active && droplet.world == famineRider_.world
            && Vector3Distance(droplet.position, famineRider_.position) <= droplet.radius + kHorsemanHitPadding) {
            DamageFamineRider(droplet.damage * dt * 8.0f, droplet.position, Color{210, 178, 100, 255});
        }
        if (deathRider_.active && droplet.world == deathRider_.world
            && Vector3Distance(droplet.position, deathRider_.position) <= droplet.radius + kDeathRiderHitPadding) {
            DamageDeathRider(droplet.damage * dt * 8.0f, droplet.position, Color{170, 174, 190, 255});
        }
        if (droplet.world == 0) {
            for (const SeraphBoss& seraph : seraphs_) {
                if (!seraph.active) continue;
                if (Vector3Distance(droplet.position, seraph.position) <= droplet.radius + 3.6f) {
                    DamageSeraph(droplet.damage * dt * 8.0f, seraph.position, Color{255, 230, 130, 255});
                }
            }
        }
        for (size_t c = 0; c < cherubs_.size();) {
            if (cherubs_[c].world != droplet.world) { ++c; continue; }
            if (Vector3Distance(droplet.position, cherubs_[c].position) <= droplet.radius + 0.55f) {
                cherubs_[c].health -= droplet.damage;
                totalDamageDealt_ += droplet.damage;
                SpawnHitBurst(cherubs_[c].position, Color{255, 245, 160, 255}, 10);
                if (cherubs_[c].health <= 0.0f) {
                    SpawnHitBurst(cherubs_[c].position, Color{245, 245, 235, 255}, 18);
                    cherubs_[c] = cherubs_.back();
                    cherubs_.pop_back();
                    continue;
                }
            }
            ++c;
        }
        for (size_t e = 0; e < enemies_.size();) {
            if (enemies_[e].world != droplet.world) { ++e; continue; }
            Vector3 ep = BodyPosition(enemies_[e].body);
            float hitDist = droplet.radius + enemies_[e].radius;
            if (Vector3Distance(droplet.position, ep) <= hitDist) {
                enemies_[e].health -= droplet.damage;
                totalDamageDealt_ += droplet.damage;
                if (enemies_[e].type == EnemyType::Dummy || enemies_[e].type == EnemyType::DummyBoss) {
                    RecordDummyDamage(enemies_[e], droplet.damage);
                }
                SpawnHitBurst(ep, Color{255, 220, 80, 255}, 10);
                if (enemies_[e].health <= 0.0f) {
                    score_ += enemies_[e].scoreValue;
                    SpawnHitBurst(ep, Color{255, 245, 210, 255}, 20);
                    DestroyEnemy(e);
                    continue;
                }
            }
            ++e;
        }

        // Spawn trail particles
        if (RandomFloat(0.0f, 1.0f) < 0.7f) {
            particles_.push_back(Particle{
                droplet.position,
                Vector3{RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f)},
                Color{255, 220, 80, 200},
                RandomFloat(0.1f, 0.25f), RandomFloat(0.1f, 0.25f),
                RandomFloat(0.03f, 0.07f)
            });
        }

        // Expire on lifetime only (no out-of-bounds — droplet can roam freely)
        droplet.life -= dt;
        if (droplet.life <= 0.0f) {
            PlaySfxAt(sfxWaterDropletBurst_, droplet.position, 62.0f, 0.9f);
            SpawnHitBurst(droplet.position, Color{255, 220, 80, 255}, 30);
            SpawnShockwave(droplet.position, droplet.radius * 3.0f, Color{255, 200, 60, 255});
            waterDroplets_[i] = waterDroplets_.back();
            waterDroplets_.pop_back();
            continue;
        }

        ++i;
    }

    // Pairwise repulsion between droplets for even distribution
    for (size_t a = 0; a < waterDroplets_.size(); ++a) {
        for (size_t b = a + 1; b < waterDroplets_.size(); ++b) {
            if (waterDroplets_[a].world != waterDroplets_[b].world) continue;
            float sepDist = (waterDroplets_[a].radius + waterDroplets_[b].radius) * config_.waterDropletSeparationMult;
            float dist = Vector3Distance(waterDroplets_[a].position, waterDroplets_[b].position);
            if (dist < sepDist && dist > 0.001f) {
                Vector3 pushDir = Vector3Normalize(Vector3Subtract(waterDroplets_[a].position, waterDroplets_[b].position));
                float strength = (1.0f - dist / sepDist) * config_.waterDropletSpeed * config_.waterDropletRepulsionStrength;
                // Project to tangent plane on spherical maps
                if (IsSphericalMap()) {
                    Vector3 upA = SphericalUpAt(waterDroplets_[a].position, waterDroplets_[a].world);
                    Vector3 upB = SphericalUpAt(waterDroplets_[b].position, waterDroplets_[b].world);
                    pushDir = Vector3Normalize(Vector3Add(
                        ProjectOnSphericalTangent(pushDir, upA),
                        ProjectOnSphericalTangent(Vector3Scale(pushDir, -1.0f), upB)
                    ));
                }
                waterDroplets_[a].velocity = Vector3Add(waterDroplets_[a].velocity, Vector3Scale(pushDir, strength));
                waterDroplets_[b].velocity = Vector3Add(waterDroplets_[b].velocity, Vector3Scale(pushDir, -strength));
            }
        }
    }
}

void Game::AddEnemyImpulse(Enemy& enemy, Vector3 impulse) {
    if (enemy.frozen) {
        enemy.storedVelocity = Vector3Add(enemy.storedVelocity, impulse);
        return;
    }

    enemy.externalVelocity = Vector3Add(enemy.externalVelocity, impulse);
    float speed = Vector3Length(enemy.externalVelocity);
    if (speed > 34.0f) {
        enemy.externalVelocity = Vector3Scale(Vector3Normalize(enemy.externalVelocity), 34.0f);
    }

    JPH::Vec3 velocity = physics_.Bodies().GetLinearVelocity(enemy.body);
    velocity += JPH::Vec3(impulse.x, impulse.y, impulse.z);
    physics_.Bodies().SetLinearVelocity(enemy.body, velocity);
}
void Game::AddProjectileImpulse(Projectile& projectile, Vector3 impulse) {
    if (projectile.frozen) {
        projectile.storedVelocity = Vector3Add(projectile.storedVelocity, impulse);
        return;
    }

    JPH::Vec3 velocity = physics_.Bodies().GetLinearVelocity(projectile.body);
    velocity += JPH::Vec3(impulse.x, impulse.y, impulse.z);
    physics_.Bodies().SetLinearVelocity(projectile.body, velocity);
}
void Game::SpawnHitBurst(Vector3 position, Color color, int count) {
    for (int i = 0; i < count; ++i) {
        Vector3 velocity = Vector3{
            RandomFloat(-1.0f, 1.0f),
            RandomFloat(0.1f, 1.2f),
            RandomFloat(-1.0f, 1.0f)
        };

        if (Vector3Length(velocity) > 0.001f) {
            velocity = Vector3Scale(Vector3Normalize(velocity), RandomFloat(4.0f, 12.0f));
        }

        float life = RandomFloat(0.18f, 0.55f);
        particles_.push_back(Particle{position, velocity, color, life, life, RandomFloat(0.04f, 0.12f)});
    }
}
void Game::DestroyProjectile(size_t index) {
    physics_.DestroyBody(projectiles_[index].body);
    projectiles_[index] = projectiles_.back();
    projectiles_.pop_back();
}
void Game::PlayEnemyHitSfx(Vector3 position) {
    if (sfxEnemyHitCooldown_ > 0.0f) {
        return;
    }
    PlaySfxAt(sfxEnemyHit_, position, 48.0f, 0.75f);
    sfxEnemyHitCooldown_ = 0.045f;
}
void Game::DestroyEnemy(size_t index) {
    bool wasBoss = enemies_[index].type == EnemyType::Boss;
    bool wasDuelist = enemies_[index].type == EnemyType::Duelist;
    bool wasSlimeKing = enemies_[index].type == EnemyType::SlimeKing;
    bool wasNormalEnemy = enemies_[index].type != EnemyType::Boss
        && enemies_[index].type != EnemyType::Duelist
        && enemies_[index].type != EnemyType::Dummy
        && enemies_[index].type != EnemyType::DummyBoss
        && enemies_[index].type != EnemyType::SlimeKing;
    bool wasPlagued = (enemies_[index].plagueTimer > 0.0f || enemies_[index].plagueBurstOnDeath)
        && enemies_[index].type != EnemyType::Boss
        && enemies_[index].type != EnemyType::Duelist
        && enemies_[index].type != EnemyType::Dummy
        && enemies_[index].type != EnemyType::DummyBoss
        && enemies_[index].type != EnemyType::SlimeKing;
    int enemyWorld = enemies_[index].world;
    Vector3 position = BodyPosition(enemies_[index].body);

    // Slime King split: launch spawn pods that fly outward and land before spawning
    if (wasSlimeKing && enemies_[index].slimeGeneration < config_.slimeKingMaxGenerations) {
        PlaySfxAt(sfxBossPhase_, position, 100.0f, 1.0f);
        int generation = enemies_[index].slimeGeneration;
        float parentHealth = enemies_[index].health;
        float parentRadius = enemies_[index].radius;
        float childHealth = std::max(parentHealth * 0.5f / static_cast<float>(config_.slimeKingSplitCount), config_.slimeKingMinHealth);
        float childRadius = parentRadius * config_.slimeKingChildScale;
        int childCount = config_.slimeKingSplitCount;

        for (int i = 0; i < childCount; ++i) {
            float angle = static_cast<float>(i) / static_cast<float>(childCount) * 6.2831853f + RandomFloat(-0.3f, 0.3f);
            Vector3 outDir = Vector3{std::cos(angle), 0.5f, std::sin(angle)};
            outDir = Vector3Normalize(outDir);
            float ejectSpeed = RandomFloat(10.0f, 16.0f) * (1.0f + generation * 0.25f);
            slimeSpawnPods_.push_back(SlimeSpawnPod{
                position, Vector3Scale(outDir, ejectSpeed), 1.0f, 1.0f,
                childHealth, childRadius, generation + 1});
        }
        SpawnShockwave(position, parentRadius * 2.0f, Color{120, 240, 160, 255});
        SpawnHitBurst(position, Color{100, 230, 150, 255}, 35);
        cameraShake_ = std::min(1.0f, cameraShake_ + 0.4f);
    }

    // Soul orb chain reaction: cursed or soul-orb-killed enemies spawn soul orbs
    bool spawnSoulOrbs = enemies_[index].killedBySoulOrb || enemies_[index].cursed;
    if (spawnSoulOrbs) {
        float baseDamage = enemies_[index].maxHealth * config_.soulOrbDamageScale;
        int count = static_cast<int>(config_.soulOrbCount);
        for (int i = 0; i < count; ++i) {
            float angle = static_cast<float>(i) / static_cast<float>(count) * 6.2831853f + RandomFloat(-0.3f, 0.3f);
            Vector3 dir = Vector3{std::cos(angle), RandomFloat(0.2f, 0.6f), std::sin(angle)};
            dir = Vector3Normalize(dir);
            FireSoulOrb(position, baseDamage, dir);
        }
        SpawnShockwave(position, 2.5f, Color{90, 30, 140, 255});
        SpawnHitBurst(position, Color{130, 50, 220, 255}, 18);
    }

    if (wasPlagued) {
        SpawnPlagueCircle(position, enemyWorld, config_.conquestRiderPlagueSmallRadius, config_.conquestRiderPlagueSmallDuration);
    }
    if (deathRider_.active
        && enemyWorld == deathRider_.world
        && enemies_[index].type != EnemyType::Dummy
        && enemies_[index].type != EnemyType::DummyBoss) {
        SpawnDeathSoul(position, enemyWorld);
    }
    if (wasNormalEnemy && config_.normalEnemyEssenceDropChance > 0.0f) {
        float dropChance = config_.normalEnemyEssenceDropChance;
        if (FaminePressureActive()) {
            dropChance *= config_.famineRiderEssenceDropChanceMult;
        }
        if (RandomFloat(0.0f, 1.0f) < dropChance) {
            Pickup essenceDrop;
            essenceDrop.type = PickupType::Essence;
            essenceDrop.position = position;
            essenceDrop.radius = 0.85f;
            essenceDrop.bobTimer = RandomFloat(0.0f, 6.28f);
            essenceDrop.maxLife = config_.droppedEssenceLifetime;
            Vector3 up = UpForWorldAt(position, enemyWorld);
            Vector3 scatter = Vector3{RandomFloat(-2.0f, 2.0f), RandomFloat(-0.4f, 0.4f), RandomFloat(-2.0f, 2.0f)};
            if (IsSphericalMap()) {
                scatter = ProjectOnSphericalTangent(scatter, up);
            } else {
                scatter.y = 0.0f;
            }
            essenceDrop.velocity = Vector3Add(Vector3Scale(up, 3.8f), scatter);
            essenceDrop.horizontalDrag = 1.4f;
            essenceDrop.gravityScale = 0.45f;
            pickups_.push_back(essenceDrop);
        }
    }

    physics_.DestroyBody(enemies_[index].body);
    enemies_[index] = enemies_.back();
    enemies_.pop_back();
    if (wasBoss) {
        PlaySfxAt(sfxBossDeath_, position, 120.0f, 1.0f);
        SpawnShockwave(position, 12.0f, Color{210, 160, 255, 255});
        SpawnHitBurst(position, Color{230, 210, 255, 255}, 90);
        eventText_ = "BOSS SHATTERED";
        eventTextTimer_ = 4.0f;
        cameraShake_ = 1.0f;
    } else if (wasDuelist) {
        PlaySfxAt(sfxBossDeath_, position, 120.0f, 1.0f);
        duelWon_ = true;
        Pickup essenceDrop;
        essenceDrop.type = PickupType::Essence;
        essenceDrop.position = position;
        essenceDrop.radius = 0.85f;
        essenceDrop.bobTimer = RandomFloat(0.0f, 6.28f);
        Vector3 up = UpForWorldAt(position, 0);
        Vector3 scatter = Vector3{RandomFloat(-2.2f, 2.2f), RandomFloat(-0.5f, 0.5f), RandomFloat(-2.2f, 2.2f)};
        if (IsSphericalMap()) {
            scatter = ProjectOnSphericalTangent(scatter, up);
        } else {
            scatter.y = 0.0f;
        }
        essenceDrop.velocity = Vector3Add(Vector3Scale(up, 4.5f), scatter);
        essenceDrop.horizontalDrag = 1.4f;
        essenceDrop.gravityScale = 0.45f;
        pickups_.push_back(essenceDrop);
        SpawnShockwave(position, 9.0f, Color{255, 225, 130, 255});
        SpawnHitBurst(position, Color{255, 235, 170, 255}, 76);
        eventText_ = "DUEL WON";
        eventTextTimer_ = 5.0f;
        cameraShake_ = 1.0f;
    } else {
        PlaySfxAt(sfxEnemyKill_, position, 58.0f, 0.85f);
    }
}
