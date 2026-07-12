#include "Game.h"
#include "GameMath.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace {
constexpr float kWarRiderHitPadding = 4.8f;
constexpr float kHorsemanHitPadding = 5.0f;
constexpr float kDeathRiderHitPadding = 5.4f;

Vector3 ClosestPointOnSegmentLocal(Vector3 point, Vector3 start, Vector3 end) {
    Vector3 segment = Vector3Subtract(end, start);
    float lenSq = Vector3DotProduct(segment, segment);
    if (lenSq <= 0.0001f) {
        return start;
    }
    float t = std::clamp(Vector3DotProduct(Vector3Subtract(point, start), segment) / lenSq, 0.0f, 1.0f);
    return Vector3Add(start, Vector3Scale(segment, t));
}

std::string PickDifferentString(const std::vector<std::string>& values, const std::string& current) {
    if (values.empty()) return current;
    std::string picked = values[GetRandomValue(0, static_cast<int>(values.size()) - 1)];
    if (values.size() <= 1) return picked;
    for (int attempts = 0; attempts < 8 && picked == current; ++attempts) {
        picked = values[GetRandomValue(0, static_cast<int>(values.size()) - 1)];
    }
    return picked;
}

Vector3 RandomUnitVector() {
    float z = RandomFloat(-1.0f, 1.0f);
    float a = RandomFloat(0.0f, 2.0f * PI);
    float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
    return Vector3{r * std::cos(a), z, r * std::sin(a)};
}

int ScaledSpawnCount(int baseCount, float scale) {
    if (baseCount <= 0 || scale <= 0.0f) return 0;
    float scaled = static_cast<float>(baseCount) * scale;
    int count = static_cast<int>(std::floor(scaled));
    float fractional = scaled - static_cast<float>(count);
    if (RandomFloat(0.0f, 1.0f) < fractional) {
        ++count;
    }
    return std::max(1, count);
}

void MakeBasis(Vector3 up, Vector3& right, Vector3& forward) {
    Vector3 seed = std::abs(up.y) < 0.92f ? Vector3{0.0f, 1.0f, 0.0f} : Vector3{1.0f, 0.0f, 0.0f};
    right = Vector3Normalize(Vector3CrossProduct(seed, up));
    forward = Vector3Normalize(Vector3CrossProduct(up, right));
}
}

void Game::UpdateEnemies(float dt) {
    Vector3 player = camera_.position;

    for (Enemy& enemy : enemies_) {
        float healthBefore = enemy.health;
        Vector3 position = BodyPosition(enemy.body);
        Vector3 target = player;
        bool sameWorldAsPlayer = enemy.world == playerWorld_;
        if (HasWormhole() && !sameWorldAsPlayer) {
            target = WormholeCenterForWorld(wormholes_.front(), enemy.world);
        }
        Vector3 direction = Vector3Subtract(target, position);
        Vector3 enemyUp = UpForWorldAt(position, enemy.world);
        if (IsSphericalMap()) {
            direction = ProjectOnSphericalTangent(direction, enemyUp);
        } else {
            direction = ProjectOnSphericalTangent(direction, enemyUp);
        }

        if (Vector3Length(direction) > 0.001f) {
            direction = Vector3Normalize(direction);
        }

        enemy.bobTimer += dt;
        enemy.actionTimer += dt;
        enemy.cooldownTimer = std::max(0.0f, enemy.cooldownTimer - dt);
        enemy.plagueTimer = std::max(0.0f, enemy.plagueTimer - dt);
        enemy.warEnrageTimer = std::max(0.0f, enemy.warEnrageTimer - dt);
        if (enemy.warCommandTimer > 0.0f) {
            enemy.warCommandTimer = std::max(0.0f, enemy.warCommandTimer - dt);
        }
        enemy.burstTimer = std::max(0.0f, enemy.burstTimer - dt);
        float speed = enemy.speed + std::min(4.0f, survivalTime_ * 0.04f);
        float warCommandSpeedMult = 1.0f;
        if (enemy.warEnrageTimer > 0.0f) {
            warCommandSpeedMult *= config_.warRiderGlobalEnrageSpeedMult;
        }
        bool skipVelocity = false;
        if (enemy.type == EnemyType::Wisp) {
            Vector3 tangent = SafeNormalize(Vector3CrossProduct(enemyUp, direction), PlayerRight());
            direction = Vector3Normalize(Vector3Add(Vector3Scale(direction, 0.72f), Vector3Scale(tangent, std::sin(enemy.bobTimer * 2.8f) * 0.5f)));
        } else if (enemy.type == EnemyType::Spitter) {
            Vector3 tangent = SafeNormalize(Vector3CrossProduct(enemyUp, direction), PlayerRight());
            float distance = IsSphericalMap() ? Vector3Length(ProjectOnSphericalTangent(Vector3Subtract(target, position), enemyUp)) : DistanceXZ(position, target);
            float rangeBias = distance < 10.0f ? -0.65f : 0.35f;
            direction = Vector3Normalize(Vector3Add(Vector3Scale(direction, rangeBias), Vector3Scale(tangent, 0.9f)));
            speed = enemy.speed;
            if (sameWorldAsPlayer && enemy.cooldownTimer <= 0.0f) {
                Vector3 shotOrigin = Vector3Add(position, Vector3Scale(enemyUp, enemy.radius * 0.35f));
                Vector3 shotDirection = Vector3Normalize(Vector3Subtract(target, shotOrigin));
                FireEnemyShot(shotOrigin, shotDirection, enemy.world);
                enemy.cooldownTimer = config_.spitterFireInterval;
            }
        } else if (enemy.type == EnemyType::Pouncer) {
            speed = enemy.speed;
            if (enemy.cooldownTimer <= 0.0f) {
                Vector3 leap = Vector3Add(Vector3Scale(direction, config_.pouncerLeapSpeed * warCommandSpeedMult), Vector3Scale(enemyUp, 4.8f));
                physics_.Bodies().SetLinearVelocity(enemy.body, ToJoltVelocity(leap));
                enemy.cooldownTimer = config_.pouncerLeapInterval;
                enemy.actionTimer = 0.0f;
                continue;
            }
            if (enemy.actionTimer < 0.42f) {
                continue;
            }
        } else if (enemy.type == EnemyType::Harrier) {
            Vector3 tangent = SafeNormalize(Vector3CrossProduct(enemyUp, direction), PlayerRight());
            float distance = IsSphericalMap() ? Vector3Length(ProjectOnSphericalTangent(Vector3Subtract(target, position), enemyUp)) : DistanceXZ(position, target);
            float rangeBias = distance < 13.0f ? -0.45f : 0.35f;
            float sway = std::sin(enemy.bobTimer * 2.1f) * 0.35f;
            direction = Vector3Normalize(Vector3Add(Vector3Scale(direction, rangeBias), Vector3Scale(tangent, 1.0f + sway)));
            speed = enemy.speed;
            if (sameWorldAsPlayer && enemy.cooldownTimer <= 0.0f) {
                Vector3 shotOrigin = Vector3Subtract(position, Vector3Scale(enemyUp, enemy.radius * 0.15f));
                Vector3 shotDirection = Vector3Normalize(Vector3Subtract(target, shotOrigin));
                FireEnemyProjectile(ProjectileKind::EnemyShot, shotOrigin, shotDirection, config_.enemyShotSpeed * 0.82f, config_.enemyShotDamage, 3.4f, 0.22f, 0.22f, Color{150, 245, 255, 255}, enemy.world);
                enemy.cooldownTimer = config_.harrierFireInterval;
            }
        } else if (enemy.type == EnemyType::Blinker && sameWorldAsPlayer) {
            speed = enemy.speed;
            if (enemy.telegraphTimer > 0.0f) {
                enemy.telegraphTimer = std::max(0.0f, enemy.telegraphTimer - dt);
                direction = Vector3Scale(direction, 0.05f);
                if (enemy.telegraphTimer <= 0.0f) {
                    Vector3 playerForward = PlayerForward();
                    if (IsSphericalMap()) {
                        playerForward = ProjectOnSphericalTangent(playerForward, SphericalUpAt(camera_.position, playerWorld_));
                    } else {
                        playerForward.y = 0.0f;
                    }
                    if (Vector3Length(playerForward) <= 0.001f) {
                        playerForward = direction;
                    } else {
                        playerForward = Vector3Normalize(playerForward);
                    }
                    Vector3 playerUp = UpForWorldAt(camera_.position, playerWorld_);
                    Vector3 playerRight = SafeNormalize(Vector3CrossProduct(playerForward, playerUp), PlayerRight());
                    float side = GetRandomValue(0, 1) == 0 ? -1.0f : 1.0f;
                    Vector3 target = Vector3Add(camera_.position, Vector3Add(Vector3Scale(playerRight, side * RandomFloat(3.2f, 5.6f)), Vector3Scale(playerForward, RandomFloat(-4.2f, 2.0f))));
                    if (IsSphericalMap()) {
                        target = SphericalSurfacePoint(target, SphericalEnemyAltitude(enemy.type), enemy.world);
                    } else if (IsSquareMap()) {
                        target.y = FlatGroundYForWorld(enemy.world) + FlatUpForWorld(enemy.world).y * 1.0f;
                        float limit = squareHalfExtent_ - enemy.radius - 0.8f;
                        target.x = std::clamp(target.x, -limit, limit);
                        target.z = std::clamp(target.z, -limit, limit);
                    } else {
                        target.y = FlatGroundYForWorld(enemy.world) + FlatUpForWorld(enemy.world).y * 1.0f;
                        Vector3 flat = Vector3{target.x, 0.0f, target.z};
                        float limit = arenaRadius_ - enemy.radius - 0.8f;
                        if (Vector3Length(flat) > limit) {
                            flat = Vector3Scale(Vector3Normalize(flat), limit);
                            target.x = flat.x;
                            target.z = flat.z;
                        }
                    }
                    physics_.Bodies().SetPosition(enemy.body, ToJoltVector(target), JPH::EActivation::Activate);
                    position = target;
                    Vector3 dash = Vector3Subtract(camera_.position, target);
                    if (IsSphericalMap()) {
                        Vector3 targetUp = SphericalUpAt(target, enemy.world);
                        dash = Vector3Add(ProjectOnSphericalTangent(dash, targetUp), Vector3Scale(targetUp, 0.1f));
                    } else {
                        dash = Vector3Add(ProjectOnSphericalTangent(dash, FlatUpForWorld(enemy.world)), Vector3Scale(FlatUpForWorld(enemy.world), 0.1f));
                    }
                    if (Vector3Length(dash) > 0.001f) {
                        dash = Vector3Normalize(dash);
                    }
                    Vector3 dashVelocity = IsSphericalMap()
                        ? Vector3Add(Vector3Scale(dash, config_.blinkerDashSpeed), Vector3Scale(SphericalUpAt(target, enemy.world), 1.6f))
                        : Vector3Add(Vector3Scale(dash, config_.blinkerDashSpeed), Vector3Scale(FlatUpForWorld(enemy.world), 1.6f));
                    physics_.Bodies().SetLinearVelocity(enemy.body, ToJoltVelocity(dashVelocity));
                    enemy.externalVelocity = Vector3Scale(dash, config_.blinkerDashSpeed * 0.2f);
                    enemy.cooldownTimer = config_.blinkerCooldown;
                    enemy.actionTimer = 0.0f;
                    SpawnShockwave(target, 2.2f, Color{255, 95, 210, 255});
                    SpawnHitBurst(target, Color{255, 95, 210, 255}, 16);
                    continue;
                }
            } else if (enemy.cooldownTimer <= 0.0f) {
                enemy.telegraphTimer = config_.blinkerWindup;
                enemy.cooldownTimer = config_.blinkerCooldown + config_.blinkerWindup;
                SpawnHitBurst(position, Color{255, 90, 220, 255}, 8);
                continue;
            }
        } else if (enemy.type == EnemyType::Minotaur && sameWorldAsPlayer && IsLabyrinthMap()) {
            speed = config_.labyrinthMinotaurSpeed;
            LabyrinthCell enemyCell = LabyrinthCellForPosition(position);
            LabyrinthCell playerCell = LabyrinthCellForPosition(camera_.position);
            if (enemy.burstCount == 1 && enemy.burstTimer > 0.0f) {
                Vector3 chargeDir = SafeNormalize(enemy.storedVelocity, direction);
                LabyrinthCell nextCell = LabyrinthCellForPosition(Vector3Add(position, Vector3Scale(chargeDir, config_.labyrinthCellSize * 0.58f)));
                if (!LabyrinthCellOpen(nextCell.x, nextCell.y)) {
                    enemy.burstCount = 0;
                    enemy.burstTimer = 0.0f;
                    enemy.cooldownTimer = config_.labyrinthMinotaurChargeInterval;
                    speed = config_.labyrinthMinotaurSpeed * 0.45f;
                    direction = Vector3Scale(chargeDir, -0.25f);
                    cameraShake_ = std::min(1.0f, cameraShake_ + 0.18f);
                    SpawnHitBurst(position, Color{190, 118, 62, 255}, 12);
                } else {
                    direction = chargeDir;
                    speed = config_.labyrinthMinotaurChargeSpeed;
                    if (EnemyTouchesPlayer(position, enemy.radius + 0.35f)) {
                        ApplyPlayerHit(position, Color{255, 70, 35, 255}, "MINOTAUR CHARGE");
                    }
                }
            } else {
                enemy.burstCount = 0;
                bool lineCharge = enemy.cooldownTimer <= 0.0f
                    && LabyrinthHasLineOfSight(enemyCell, playerCell)
                    && (std::abs(enemyCell.x - playerCell.x) + std::abs(enemyCell.y - playerCell.y)) >= 2;
                if (lineCharge) {
                    Vector3 from = LabyrinthCellCenter(enemyCell.x, enemyCell.y);
                    Vector3 to = LabyrinthCellCenter(playerCell.x, playerCell.y);
                    Vector3 chargeDir = SafeNormalize(Vector3{to.x - from.x, 0.0f, to.z - from.z}, direction);
                    enemy.storedVelocity = chargeDir;
                    enemy.burstCount = 1;
                    enemy.burstTimer = std::max(0.65f,
                        config_.labyrinthCellSize * static_cast<float>(std::max(labyrinthWidth_, labyrinthHeight_))
                            / std::max(1.0f, config_.labyrinthMinotaurChargeSpeed));
                    enemy.cooldownTimer = config_.labyrinthMinotaurChargeInterval;
                    direction = chargeDir;
                    speed = config_.labyrinthMinotaurChargeSpeed;
                    SpawnHitBurst(position, Color{255, 96, 42, 255}, 18);
                    cameraShake_ = std::min(1.0f, cameraShake_ + 0.12f);
                } else {
                    LabyrinthCell next = LabyrinthNextStepToward(enemyCell, playerCell);
                    Vector3 nextCenter = LabyrinthCellCenter(next.x, next.y);
                    Vector3 toNext{nextCenter.x - position.x, 0.0f, nextCenter.z - position.z};
                    if (Vector3Length(toNext) > 0.2f) {
                        direction = Vector3Normalize(toNext);
                    }
                    speed = config_.labyrinthMinotaurSpeed;
                }
            }
        } else if (enemy.type == EnemyType::Boss) {
            Vector3 tangent = SafeNormalize(Vector3CrossProduct(enemyUp, direction), PlayerRight());
            float distance = IsSphericalMap() ? Vector3Length(ProjectOnSphericalTangent(Vector3Subtract(target, position), enemyUp)) : DistanceXZ(position, target);
            float rangeBias = distance < 16.0f ? -0.45f : 0.35f;
            direction = Vector3Normalize(Vector3Add(Vector3Scale(direction, rangeBias), Vector3Scale(tangent, 0.85f)));
            speed = enemy.speed;

            bool enraged = enemy.health < enemy.maxHealth * 0.45f;
            float cooldownAfter = enraged ? 1.4f : 2.0f;
            float burstInterval = config_.bossHomingBurstInterval * (enraged ? 0.7f : 1.0f);

            if (enemy.cooldownTimer <= 0.0f && enemy.burstCount == 0) {
                FireBossRing(Vector3Add(position, Vector3Scale(enemyUp, enemy.radius * 0.35f)), enraged ? 12 : 8, 0.85f, enemy.world);
                Vector3 shotOrigin = Vector3Add(position, Vector3Scale(enemyUp, enemy.radius * 0.55f));
                Vector3 shotDir = Vector3Normalize(Vector3Subtract(target, shotOrigin));
                float speed = config_.enemyShotSpeed * config_.bossHomingSpeedScale;
                FireHomingShot(shotOrigin, shotDir, speed, config_.bossHomingTurnRate, config_.bossHomingLife, config_.enemyShotDamage, Color{180, 125, 255, 255}, ProjectileOwner::Enemy, enemy.world);
                enemy.burstCount = 1;
                if (enemy.burstCount >= config_.bossHomingBurstCount) {
                    enemy.cooldownTimer = cooldownAfter;
                    enemy.burstCount = 0;
                } else {
                    enemy.burstTimer = burstInterval;
                }
                cameraShake_ = std::min(1.0f, cameraShake_ + 0.08f);
            } else if (enemy.burstCount > 0 && enemy.burstTimer <= 0.0f) {
                Vector3 shotOrigin = Vector3Add(position, Vector3Scale(enemyUp, enemy.radius * 0.55f));
                Vector3 shotDir = Vector3Normalize(Vector3Subtract(target, shotOrigin));
                float speed = config_.enemyShotSpeed * config_.bossHomingSpeedScale;
                FireHomingShot(shotOrigin, shotDir, speed, config_.bossHomingTurnRate, config_.bossHomingLife, config_.enemyShotDamage, Color{180, 125, 255, 255}, ProjectileOwner::Enemy, enemy.world);
                enemy.burstCount++;
                if (enemy.burstCount >= config_.bossHomingBurstCount) {
                    enemy.cooldownTimer = cooldownAfter;
                    enemy.burstCount = 0;
                } else {
                    enemy.burstTimer = burstInterval;
                }
                cameraShake_ = std::min(1.0f, cameraShake_ + 0.08f);
            }
        } else if (enemy.type == EnemyType::SlimeKing && sameWorldAsPlayer) {
            speed = enemy.speed;
            Vector3 enemyUp = UpForWorldAt(position, enemy.world);
            Vector3 playerGround = camera_.position;
            if (IsSphericalMap()) {
                playerGround = SphericalSurfacePoint(camera_.position, SphericalPlayerAltitude(), playerWorld_);
            } else {
                playerGround = Vector3Subtract(camera_.position, Vector3Scale(FlatUpForWorld(playerWorld_), playerHeight_));
            }
            float dist = Vector3Length(Vector3Subtract(position, camera_.position));
            float targetAlt = IsSphericalMap() ? SphericalEnemyAltitude(EnemyType::SlimeKing) : 0.0f;
            float currentAlt = IsSphericalMap() ? SphericalAltitudeAt(position, enemy.world) : 0.0f;
            float flatEnemySurfaceY = FlatGroundYForWorld(enemy.world) + FlatUpForWorld(enemy.world).y * (0.6f + enemy.radius * 0.5f);
            float flatEnemySlamY = FlatGroundYForWorld(enemy.world) + FlatUpForWorld(enemy.world).y * (0.6f + enemy.radius * 0.3f);
            bool nearSurface = IsSphericalMap()
                ? currentAlt <= targetAlt + enemy.radius * 0.55f
                : (enemy.world == 0 ? position.y <= flatEnemySurfaceY : position.y >= flatEnemySurfaceY);

            // State machine using burstCount: 0=ground, 1=long_jump, 2=high_jump_rise, 3=high_jump_fall, 4=shooting
            if (enemy.burstCount == 0) {
                enemy.cooldownTimer -= dt;
                if (enemy.cooldownTimer <= 0.0f) {
                    if (dist > config_.slimeKingSlamRange) {
                        // Long jump toward player
                        Vector3 jumpDir = Vector3Normalize(Vector3Subtract(camera_.position, position));
                        if (IsSphericalMap()) {
                            jumpDir = ProjectOnSphericalTangent(jumpDir, enemyUp);
                            jumpDir = Vector3Normalize(jumpDir);
                        }
                        Vector3 jumpVel = Vector3Add(
                            Vector3Scale(jumpDir, config_.slimeKingLongJumpSpeed),
                            Vector3Scale(enemyUp, config_.slimeKingLongJumpSpeed * 1.0f));
                        physics_.Bodies().SetLinearVelocity(enemy.body, ToJoltVelocity(jumpVel));
                        enemy.burstCount = 1;
                        enemy.actionTimer = 1.2f;
                        skipVelocity = true;
                    } else if (RandomFloat(0.0f, 1.0f) < 0.25f) {
                        enemy.burstCount = 4;
                        enemy.burstTimer = 0.0f;
                        enemy.actionTimer = 0;
                        skipVelocity = true;
                    } else {
                        Vector3 jumpVel = Vector3Scale(enemyUp, config_.slimeKingHighJumpSpeed);
                        physics_.Bodies().SetLinearVelocity(enemy.body, ToJoltVelocity(jumpVel));
                        enemy.burstCount = 2;
                        enemy.actionTimer = 1.2f;
                        skipVelocity = true;
                    }
                }
            } else if (enemy.burstCount == 1) {
                // Long jump: apply radial gravity on spherical maps, detect landing
                enemy.actionTimer -= dt;
                if (IsSphericalMap()) {
                    // Pull toward surface
                    float alt = SphericalAltitudeAt(position, enemy.world);
                    float pull = (targetAlt - alt) * 7.0f;
                    JPH::Vec3 vel = physics_.Bodies().GetLinearVelocity(enemy.body);
                    JPH::Vec3 radialVel = JPH::Vec3(enemyUp.x * pull, enemyUp.y * pull, enemyUp.z * pull);
                    physics_.Bodies().SetLinearVelocity(enemy.body, vel + radialVel * dt);
                }
                if (nearSurface || enemy.actionTimer <= 0.0f) {
                    enemy.burstCount = 0;
                    enemy.cooldownTimer = config_.slimeKingCooldown * 0.6f;
                }
                skipVelocity = true;
            } else if (enemy.burstCount == 2) {
                // High jump rising: detect apex via radial velocity reversal
                enemy.actionTimer -= dt;
                if (IsSphericalMap()) {
                    float alt = SphericalAltitudeAt(position, enemy.world);
                    float pull = (targetAlt - alt) * 12.0f;
                    JPH::Vec3 vel = physics_.Bodies().GetLinearVelocity(enemy.body);
                    JPH::Vec3 radialVel = JPH::Vec3(enemyUp.x * pull, enemyUp.y * pull, enemyUp.z * pull);
                    physics_.Bodies().SetLinearVelocity(enemy.body, vel + radialVel * dt);
                }
                JPH::Vec3 vel = physics_.Bodies().GetLinearVelocity(enemy.body);
                bool falling = IsSphericalMap()
                    ? Vector3DotProduct(ToRayVector(vel), enemyUp) <= 0.5f
                    : vel.GetY() <= 0.0f;
                if (enemy.actionTimer <= 0.0f || falling) {
                    Vector3 slamTarget = IsSphericalMap()
                        ? SphericalSurfacePoint(camera_.position, SphericalPlayerAltitude(), playerWorld_)
                        : Vector3{camera_.position.x, FlatGroundYForWorld(enemy.world) + FlatUpForWorld(enemy.world).y * 0.6f, camera_.position.z};
                    Vector3 slamDir = Vector3Normalize(Vector3Subtract(slamTarget, position));
                    Vector3 slamVel = Vector3Scale(slamDir, config_.slimeKingSlamSpeed);
                    physics_.Bodies().SetLinearVelocity(enemy.body, ToJoltVelocity(slamVel));
                    enemy.burstCount = 3;
                    enemy.actionTimer = 0.8f;
                }
                skipVelocity = true;
            } else if (enemy.burstCount == 3) {
                // Slamming down — contact damage during descent + ground impact
                enemy.actionTimer -= dt;
                // Mid-air contact: hitting the player during the slam descent
                if (sameWorldAsPlayer && EnemyTouchesPlayer(position, enemy.radius)) {
                    ApplyPlayerHit(position, Color{100, 220, 140, 255}, "SLIME SLAM");
                }
                bool hitGround = IsSphericalMap()
                    ? SphericalAltitudeAt(position, enemy.world) <= targetAlt + enemy.radius * 0.35f
                    : (enemy.world == 0 ? position.y <= flatEnemySlamY : position.y >= flatEnemySlamY);
                if (hitGround || enemy.actionTimer <= 0.0f) {
                    Vector3 impactPos = IsSphericalMap()
                        ? SphericalSurfacePoint(position, targetAlt, enemy.world)
                        : Vector3{position.x, FlatGroundYForWorld(enemy.world) + FlatUpForWorld(enemy.world).y * 0.6f, position.z};
                    float slamVolume = std::clamp(0.45f + std::sqrt(std::max(0.0f, enemy.maxHealth) / std::max(1.0f, config_.slimeKingHealth)) * 0.65f, 0.35f, 1.25f);
                    PlaySfxAt(sfxSlimeSlam_, impactPos, 80.0f + enemy.radius * 10.0f, slamVolume);
                    SpawnShockwave(impactPos, config_.slimeKingSlamRadius, Color{120, 240, 160, 255});
                    SpawnHitBurst(impactPos, Color{100, 220, 140, 255}, 24);
                    cameraShake_ = std::min(1.0f, cameraShake_ + 0.35f);
                    if (sameWorldAsPlayer && EnemyTouchesPlayer(impactPos, config_.slimeKingSlamRadius)) {
                        ApplyPlayerHit(impactPos, Color{100, 220, 140, 255}, "SLIME SLAM");
                    }
                    enemy.burstCount = 0;
                    enemy.cooldownTimer = config_.slimeKingCooldown;
                }
                skipVelocity = true;
            } else if (enemy.burstCount == 4) {
                // Shooting
                enemy.burstTimer -= dt;
                if (enemy.burstTimer <= 0.0f && enemy.actionTimer < config_.slimeKingShootCount) {
                    Vector3 shootDir = Vector3Normalize(Vector3Subtract(camera_.position, position));
                    float spread = (enemy.actionTimer - config_.slimeKingShootCount * 0.5f) * 0.12f;
                    Vector3 spreadDir = Vector3Add(shootDir, Vector3{RandomFloat(-spread, spread), RandomFloat(-spread, spread), RandomFloat(-spread, spread)});
                    spreadDir = Vector3Normalize(spreadDir);
                    FireEnemyProjectile(ProjectileKind::EnemyShot, position, spreadDir,
                        config_.slimeKingShootSpeed, config_.slimeKingSlamDamage,
                        4.0f, 0.28f, 0.28f, Color{100, 230, 150, 255}, enemy.world);
                    enemy.burstTimer = config_.slimeKingShootInterval;
                    enemy.actionTimer += 1.0f;
                }
                if (enemy.actionTimer >= config_.slimeKingShootCount) {
                    enemy.burstCount = 0;
                    enemy.cooldownTimer = config_.slimeKingCooldown;
                }
                skipVelocity = true;
            }

            // SlimeKing spherical gravity: constant surface pull + damping (no teleport)
            if (IsSphericalMap()) {
                JPH::Vec3 vel = physics_.Bodies().GetLinearVelocity(enemy.body);
                Vector3 worldVel = ToRayVector(vel);
                float radialSpeed = Vector3DotProduct(worldVel, enemyUp);
                float alt = SphericalAltitudeAt(position, enemy.world);

                // Constant gravity toward surface (-enemyUp direction)
                Vector3 tangentVel = Vector3Subtract(worldVel, Vector3Scale(enemyUp, radialSpeed));
                radialSpeed -= config_.slimeKingSphericalGravity * dt;

                // Damping near surface: reduce radial oscillation
                float altError = alt - targetAlt;
                if (std::abs(altError) < 1.5f && radialSpeed * altError > 0.0f) {
                    radialSpeed *= config_.slimeKingSurfaceDamping;
                }

                // Hard floor: if below surface, bounce back with damped velocity
                if (alt < targetAlt - 0.2f && radialSpeed < 0.0f) {
                    radialSpeed *= -0.4f;
                }

                Vector3 correctedVel = Vector3Add(tangentVel, Vector3Scale(enemyUp, radialSpeed));
                physics_.Bodies().SetLinearVelocity(enemy.body, ToJoltVelocity(correctedVel));
            }
        } else if (enemy.type == EnemyType::Duelist && sameWorldAsPlayer) {
            UpdateDuelist(enemy, position, direction, dt, speed, skipVelocity);
        } else if (enemy.type == EnemyType::Dummy) {
            // Reuses generic approach movement (falls through below)
        } else if (enemy.type == EnemyType::DummyBoss) {
            // Reuse Boss strafe movement, no attacks
            Vector3 tangent = SafeNormalize(Vector3CrossProduct(enemyUp, direction), PlayerRight());
            float dist = IsSphericalMap()
                ? Vector3Length(ProjectOnSphericalTangent(Vector3Subtract(target, position), enemyUp))
                : DistanceXZ(position, target);
            float rangeBias = dist < 16.0f ? -0.45f : 0.35f;
            direction = Vector3Normalize(Vector3Add(
                Vector3Scale(direction, rangeBias),
                Vector3Scale(tangent, 0.85f)));
            speed = enemy.speed;
        }

        if (skipVelocity) {
            continue;
        }

        Vector3 velocity = {};
        if (IsSphericalMap()) {
            float targetAltitude = enemy.type == EnemyType::Harrier ? config_.harrierTargetHeight + std::sin(enemy.bobTimer * 2.6f) * 1.2f : SphericalEnemyAltitude(enemy.type);
            float targetDistance = SphericalSignedRadius(targetAltitude, enemy.world);
            float radialCorrection = std::clamp((targetDistance - Vector3Length(position)) * 7.0f, -12.0f, 8.0f);
            if (IsHollowPhysicsForWorld(enemy.world)) {
                radialCorrection = -radialCorrection;
            }
            if (enemy.type == EnemyType::Pouncer || enemy.type == EnemyType::SlimeKing) {
                JPH::Vec3 current = physics_.Bodies().GetLinearVelocity(enemy.body);
                radialCorrection = Vector3DotProduct(ToRayVector(current), enemyUp);
            }
            velocity = Vector3Add(Vector3Add(Vector3Scale(direction, speed * warCommandSpeedMult), Vector3Scale(enemyUp, radialCorrection)), enemy.externalVelocity);
        } else {
            float verticalVelocity = 0.0f;
            if (enemy.type == EnemyType::Pouncer || enemy.type == EnemyType::SlimeKing) {
                verticalVelocity = physics_.Bodies().GetLinearVelocity(enemy.body).GetY();
            } else {
                float targetHeight = enemy.type == EnemyType::Harrier ? config_.harrierTargetHeight + std::sin(enemy.bobTimer * 2.6f) * 1.2f : enemy.type == EnemyType::Blinker ? 1.0f : enemy.type == EnemyType::Wisp || enemy.type == EnemyType::Spitter ? 1.35f : enemy.type == EnemyType::Boss ? 2.2f : enemy.type == EnemyType::Minotaur ? 1.55f : enemy.type == EnemyType::Duelist ? 1.2f : 0.8f;
                float targetY = FlatGroundYForWorld(enemy.world) + FlatUpForWorld(enemy.world).y * targetHeight;
                verticalVelocity = std::clamp((targetY - position.y) * 7.0f, -12.0f, 8.0f);
            }
            velocity = Vector3{
                direction.x * speed * warCommandSpeedMult + enemy.externalVelocity.x,
                verticalVelocity + enemy.externalVelocity.y,
                direction.z * speed * warCommandSpeedMult + enemy.externalVelocity.z
            };
        }
        enemy.externalVelocity = Vector3Scale(enemy.externalVelocity, std::pow(0.12f, dt));
        physics_.Bodies().SetLinearVelocity(enemy.body, ToJoltVelocity(velocity));

        // Prop collision (AABB pushback, same as player)
        for (const Prop& prop : props_) {
            if (!prop.collidable || prop.shape != 0) continue;
            float minX = prop.position.x - prop.scale.x * 0.5f - enemy.radius;
            float maxX = prop.position.x + prop.scale.x * 0.5f + enemy.radius;
            float minZ = prop.position.z - prop.scale.z * 0.5f - enemy.radius;
            float maxZ = prop.position.z + prop.scale.z * 0.5f + enemy.radius;
            float topY = prop.position.y + prop.scale.y;
            float bottomY = prop.position.y;
            float feetY = position.y - enemy.radius;
            bool overlapsXZ = position.x >= minX && position.x <= maxX && position.z >= minZ && position.z <= maxZ;
            if (!overlapsXZ) continue;
            if (feetY >= topY || position.y <= bottomY) continue;
            float pushLeft = std::abs(position.x - minX), pushRight = std::abs(maxX - position.x);
            float pushBack = std::abs(position.z - minZ), pushFront = std::abs(maxZ - position.z);
            float best = std::min(std::min(pushLeft, pushRight), std::min(pushBack, pushFront));
            if (best == pushLeft) position.x = minX;
            else if (best == pushRight) position.x = maxX;
            else if (best == pushBack) position.z = minZ;
            else position.z = maxZ;
            physics_.Bodies().SetPosition(enemy.body, ToJoltVector(position), JPH::EActivation::Activate);
        }

        // Curse damage over time
        if (enemy.cursed && !timeStopped_) {
            enemy.health -= enemy.curseDps * dt;
            totalDamageDealt_ += enemy.curseDps * dt;
            if (enemy.type == EnemyType::Dummy || enemy.type == EnemyType::DummyBoss) {
                RecordDummyDamage(enemy, enemy.curseDps * dt);
            }
            if (static_cast<int>(GetTime() * 10.0f) % 3 == 0) {
                SpawnHitBurst(position, Color{160, 80, 240, 255}, 2);
            }
        }

        // Ignite damage over time + fire particles
        if (enemy.ignited && !timeStopped_) {
            enemy.igniteTimer -= dt;
            enemy.health -= enemy.igniteDps * dt;
            totalDamageDealt_ += enemy.igniteDps * dt;
            if (enemy.type == EnemyType::Dummy || enemy.type == EnemyType::DummyBoss) {
                RecordDummyDamage(enemy, enemy.igniteDps * dt);
            }
            // Fire particles on burning enemy
            if (RandomFloat(0.0f, 1.0f) < 0.5f) {
                particles_.push_back(Particle{
                    Vector3Add(position, Vector3Scale(enemyUp, enemy.radius * 0.5f)),
                    Vector3{RandomFloat(-1.0f, 1.0f), RandomFloat(2.0f, 5.0f), RandomFloat(-1.0f, 1.0f)},
                    Color{255, (unsigned char)RandomFloat(80, 160), 20, 200},
                    RandomFloat(0.15f, 0.35f), RandomFloat(0.15f, 0.35f),
                    RandomFloat(0.04f, 0.09f)
                });
            }
            // Chain spread to nearby enemies
            enemy.igniteSpreadTimer -= dt;
            if (enemy.igniteSpreadTimer <= 0.0f) {
                enemy.igniteSpreadTimer = config_.napalmSpreadInterval;
                for (Enemy& other : enemies_) {
                    if (&other == &enemy) continue;
                    if (other.ignited) continue;
                    if (other.world != enemy.world) continue;
                    Vector3 op = BodyPosition(other.body);
                    if (Vector3Distance(position, op) <= config_.napalmSpreadRadius + other.radius) {
                        IgniteEnemy(other);
                        break; // spread to one enemy per interval
                    }
                }
            }
            if (enemy.igniteTimer <= 0.0f) {
                enemy.ignited = false;
            }
        }

        if (sameWorldAsPlayer
            && EnemyTouchesPlayer(position, enemy.radius)
            && enemy.type != EnemyType::Dummy
            && enemy.type != EnemyType::DummyBoss) {
            if (enemy.type == EnemyType::Minotaur) {
                ApplyPlayerHit(player, Color{255, 70, 35, 255}, "MINOTAUR");
            } else {
                ApplyPlayerHit(player, Color{255, 35, 25, 255});
            }
        }

        // Track health changes for health bar display priority
        if (enemy.health < healthBefore) {
            enemy.lastDamageTime = survivalTime_;
        }
    }

    // Check for enemies killed by curse DoT
    for (size_t i = enemies_.size(); i > 0; --i) {
        size_t idx = i - 1;
        if (enemies_[idx].health <= 0.0f) {
            Vector3 deadPos = BodyPosition(enemies_[idx].body);
            SpawnHitBurst(deadPos, Color{140, 60, 240, 255}, 20);
            score_ += enemies_[idx].scoreValue;
            DestroyEnemy(idx);
        }
    }
}
void Game::UpdateWaveDirector(float dt) {
    int newWave = survivalTime_ < 20.0f ? 1 : survivalTime_ < 45.0f ? 2 : survivalTime_ < 75.0f ? 3 : 4;
    if (newWave != waveIndex_) {
        waveIndex_ = newWave;
        eventText_ = WaveLabel();
        eventTextTimer_ = 3.0f;
        spawnTimer_ = std::min(spawnTimer_, 0.35f);
    }

    float baseInterval = waveIndex_ == 1 ? 1.85f : waveIndex_ == 2 ? 1.45f : waveIndex_ == 3 ? 1.05f : 0.72f;
    float spawnScale = config_.heavenFalls ? config_.heavenFallsSpawnIntervalScale : config_.survivalSpawnIntervalScale;
    float eventScale = config_.heavenFalls ? config_.heavenFallsEventSpawnScale : config_.survivalEventSpawnScale;
    spawnInterval_ = baseInterval * spawnScale;
    spawnTimer_ -= dt;
    if (!config_.bossRushMode && spawnTimer_ <= 0.0f) {
        SpawnEnemy();
        spawnTimer_ = spawnInterval_;
    }

    if (IsLabyrinthMap() && config_.labyrinthMinotaurEnabled
        && !labyrinthMinotaurSpawned_
        && survivalTime_ >= config_.labyrinthMinotaurSpawnDelay) {
        SpawnEnemyOfType(EnemyType::Minotaur);
        eventText_ = "MINOTAUR";
        eventTextTimer_ = 4.0f;
        labyrinthMinotaurSpawned_ = true;
    }

    if (!config_.bossRushMode && !wispSurgeDone_ && survivalTime_ >= 25.0f) {
        for (int i = 0, count = ScaledSpawnCount(5, eventScale); i < count; ++i) {
            SpawnEnemyOfType(EnemyType::Wisp);
        }
        eventText_ = "WISP SURGE";
        eventTextTimer_ = 3.0f;
        wispSurgeDone_ = true;
    }

    if (!config_.bossRushMode && !spitterAmbushDone_ && survivalTime_ >= 52.0f) {
        for (int i = 0, count = ScaledSpawnCount(3, eventScale); i < count; ++i) {
            SpawnEnemyOfType(EnemyType::Spitter);
        }
        eventText_ = "SPITTERS";
        eventTextTimer_ = 3.0f;
        spitterAmbushDone_ = true;
    }

    if (!config_.bossRushMode && !pouncerRushDone_ && survivalTime_ >= 82.0f) {
        for (int i = 0, count = ScaledSpawnCount(4, eventScale); i < count; ++i) {
            SpawnEnemyOfType(EnemyType::Pouncer);
        }
        eventText_ = "POUNCER RUSH";
        eventTextTimer_ = 3.0f;
        pouncerRushDone_ = true;
    }

    if (!config_.heavenFalls && !bossSpawned_ && survivalTime_ >= config_.bossSpawnTime) {
        SpawnEnemyOfType(EnemyType::Boss);
        if (!config_.bossRushMode) {
            for (int i = 0, count = ScaledSpawnCount(3, eventScale); i < count; ++i) {
                SpawnEnemyOfType(i % 2 == 0 ? EnemyType::Wisp : EnemyType::Spitter);
            }
        }
        eventText_ = "GEOMETRY LORD";
        eventTextTimer_ = 4.0f;
        bossSpawned_ = true;
    }

    if (!config_.heavenFalls && !slimeKingSpawned_ && survivalTime_ >= config_.slimeKingSpawnTime) {
        SpawnEnemyOfType(EnemyType::SlimeKing);
        eventText_ = "SLIME KING";
        eventTextTimer_ = 4.0f;
        slimeKingSpawned_ = true;
    }

    if (!config_.heavenFalls && !bethlehemSpawned_ && survivalTime_ >= config_.bethlehemSpawnTime) {
        SpawnBethlehem();
        eventText_ = "STAR OF BETHLEHEM";
        eventTextTimer_ = 4.0f;
        bethlehemSpawned_ = true;
    }

    if (!config_.heavenFalls && config_.throneEnabled && !throneAngelSpawned_ && survivalTime_ >= config_.throneSpawnTime) {
        SpawnThroneAngel();
        eventText_ = "THRONE ANGEL";
        eventTextTimer_ = 4.0f;
        throneAngelSpawned_ = true;
    }

    if (!config_.heavenFalls && config_.seraphEnabled && !seraphSpawned_ && survivalTime_ >= config_.seraphSpawnTime) {
        SpawnSeraph();
        eventText_ = "SERAPH";
        eventTextTimer_ = 4.0f;
        seraphSpawned_ = true;
    }

    if (config_.heavenFalls && config_.warRiderEnabled && !warRiderSpawned_ && survivalTime_ >= config_.warRiderSpawnTime) {
        SpawnWarRider();
        eventText_ = "WAR RIDER";
        eventTextTimer_ = 4.0f;
        warRiderSpawned_ = true;
    }

    if (config_.heavenFalls && config_.conquestRiderEnabled && !conquestRiderSpawned_ && survivalTime_ >= config_.conquestRiderSpawnTime) {
        SpawnConquestRider();
        eventText_ = "CONQUEST RIDER";
        eventTextTimer_ = 4.0f;
        conquestRiderSpawned_ = true;
    }

    if (config_.heavenFalls && config_.famineRiderEnabled && !famineRiderSpawned_ && survivalTime_ >= config_.famineRiderSpawnTime) {
        SpawnFamineRider();
        eventText_ = "FAMINE RIDER";
        eventTextTimer_ = 4.0f;
        famineRiderSpawned_ = true;
    }

    if (config_.heavenFalls && config_.deathRiderEnabled && !deathRiderSpawned_ && survivalTime_ >= config_.deathRiderSpawnTime) {
        SpawnDeathRider();
        eventText_ = "DEATH RIDER";
        eventTextTimer_ = 4.0f;
        deathRiderSpawned_ = true;
    }

    if (!config_.bossRushMode && waveIndex_ >= 4 && survivalTime_ >= nextMixedEventTime_) {
        int eventRoll = GetRandomValue(0, 99);
        if (eventRoll < 34) {
            for (int i = 0, count = ScaledSpawnCount(3, eventScale); i < count; ++i) {
                SpawnEnemyOfType(EnemyType::Harrier);
            }
            eventText_ = "HARRIER SWARM";
        } else if (eventRoll < 67) {
            for (int i = 0, count = ScaledSpawnCount(2, eventScale); i < count; ++i) {
                SpawnEnemyOfType(EnemyType::Blinker);
            }
            eventText_ = "BLINK STRIKE";
        } else {
            int count = ScaledSpawnCount(3, eventScale);
            for (int i = 0; i < count; ++i) {
                EnemyType type = i % 3 == 0 ? EnemyType::Spitter : i % 3 == 1 ? EnemyType::Pouncer : EnemyType::Harrier;
                SpawnEnemyOfType(type);
            }
            eventText_ = "MIXED EVENT";
        }
        eventTextTimer_ = 2.0f;
        nextMixedEventTime_ += 28.0f;
    }
}
void Game::SpawnEnemy() {
    EnemyType type = EnemyType::Skitter;
    int roll = GetRandomValue(0, 100);
    if (waveIndex_ == 2) {
        type = roll > 72 ? EnemyType::Wisp : EnemyType::Skitter;
    } else if (waveIndex_ == 3) {
        type = roll > 88 ? EnemyType::Harrier : roll > 78 ? EnemyType::Spitter : roll > 55 ? EnemyType::Brute : roll > 34 ? EnemyType::Wisp : EnemyType::Skitter;
    } else if (waveIndex_ >= 4) {
        type = roll > 91 ? EnemyType::Blinker : roll > 80 ? EnemyType::Harrier : roll > 66 ? EnemyType::Pouncer : roll > 52 ? EnemyType::Spitter : roll > 36 ? EnemyType::Brute : roll > 18 ? EnemyType::Wisp : EnemyType::Skitter;
    }
    SpawnEnemyOfType(type);
}
void Game::SpawnEnemyOfType(EnemyType type) {
    Vector3 position = {};
    if (IsLabyrinthMap()) {
        LabyrinthCell playerCell = LabyrinthCellForPosition(camera_.position);
        LabyrinthCell spawnCell = type == EnemyType::Minotaur
            ? LabyrinthFarthestCellFrom(playerCell.x, playerCell.y)
            : LabyrinthCell{labyrinthWidth_ > 2 ? 1 : 0, labyrinthHeight_ > 2 ? 1 : 0};
        if (type != EnemyType::Minotaur) {
            float bestScore = -1.0f;
            for (int attempt = 0; attempt < 48; ++attempt) {
                int x = GetRandomValue(1, std::max(1, labyrinthWidth_ - 2));
                int y = GetRandomValue(1, std::max(1, labyrinthHeight_ - 2));
                if (!LabyrinthCellOpen(x, y)) continue;
                Vector3 center = LabyrinthCellCenter(x, y);
                float dist = DistanceXZ(center, camera_.position);
                float wallBias = ((x & 1) && (y & 1)) ? 4.0f : 0.0f;
                float score = dist + wallBias;
                if (dist > config_.labyrinthCellSize * 2.4f && score > bestScore) {
                    bestScore = score;
                    spawnCell = LabyrinthCell{x, y};
                }
            }
            if (!LabyrinthCellOpen(spawnCell.x, spawnCell.y)) {
                spawnCell = LabyrinthFarthestCellFrom(playerCell.x, playerCell.y);
            }
        }
        position = LabyrinthCellCenter(spawnCell.x, spawnCell.y);
        position.y = 0.8f;
    } else if (IsSphericalMap()) {
        Vector3 playerUp = SphericalUpAt(camera_.position);
        Vector3 tangentA = SafeNormalize(ProjectOnSphericalTangent(PlayerRight(), playerUp), Vector3{1.0f, 0.0f, 0.0f});
        Vector3 tangentB = SafeNormalize(Vector3CrossProduct(playerUp, tangentA), Vector3{0.0f, 0.0f, 1.0f});
        float angle = RandomFloat(0.0f, 6.2831853f);
        float arc = RandomFloat(0.45f, 1.85f);
        Vector3 direction = Vector3Add(Vector3Scale(playerUp, std::cos(arc)), Vector3Scale(Vector3Add(Vector3Scale(tangentA, std::cos(angle)), Vector3Scale(tangentB, std::sin(angle))), std::sin(arc)));
        position = SphericalSurfacePoint(direction, SphericalEnemyAltitude(type));
    } else if (IsSquareMap()) {
        int side = GetRandomValue(0, 3);
        float edge = squareHalfExtent_ - 1.2f;
        float lane = RandomFloat(-squareHalfExtent_ + 2.0f, squareHalfExtent_ - 2.0f);
        if (side == 0) {
            position = Vector3{lane, 0.8f, -edge};
        } else if (side == 1) {
            position = Vector3{edge, 0.8f, lane};
        } else if (side == 2) {
            position = Vector3{lane, 0.8f, edge};
        } else {
            position = Vector3{-edge, 0.8f, lane};
        }
    } else {
        float angle = RandomFloat(0.0f, 6.2831853f);
        float radius = arenaRadius_ - 1.2f;
        position = Vector3{std::cos(angle) * radius, 0.8f, std::sin(angle) * radius};
    }

    if (IsSphericalMap()) {
        position = SphericalSurfacePoint(position, SphericalEnemyAltitude(type));
    } else if (type == EnemyType::Wisp || type == EnemyType::Spitter) {
        position.y = 1.35f;
    } else if (type == EnemyType::Pouncer) {
        position.y = 0.9f;
    } else if (type == EnemyType::Harrier) {
        position.y = config_.harrierTargetHeight;
    } else if (type == EnemyType::Blinker) {
        position.y = 1.0f;
    } else if (type == EnemyType::Boss) {
        position.y = 2.2f;
    } else if (type == EnemyType::SlimeKing) {
        position.y = 1.5f;
    } else if (type == EnemyType::Minotaur) {
        position.y = 1.55f;
    } else if (type == EnemyType::Duelist) {
        position.y = 1.2f;
    } else if (type == EnemyType::Dummy) {
        position.y = 0.8f;
    } else if (type == EnemyType::DummyBoss) {
        position.y = 2.2f;
    }

    float enemyRadius = 0.65f;
    float health = 1.0f;
    float speed = RandomFloat(2.5f, 4.0f);
    int scoreValue = 10;
    Color color = Color{205, 30, 35, 255};

    if (type == EnemyType::Brute) {
        enemyRadius = 1.0f;
        health = 4.0f;
        speed = RandomFloat(1.45f, 2.15f);
        scoreValue = 35;
        color = Color{255, 95, 25, 255};
    } else if (type == EnemyType::Wisp) {
        enemyRadius = 0.72f;
        health = 1.5f;
        speed = RandomFloat(4.8f, 6.4f);
        scoreValue = 20;
        color = Color{120, 210, 255, 255};
    } else if (type == EnemyType::Spitter) {
        enemyRadius = 0.78f;
        health = 2.2f;
        speed = RandomFloat(2.0f, 2.8f);
        scoreValue = 30;
        color = Color{105, 255, 185, 255};
    } else if (type == EnemyType::Pouncer) {
        enemyRadius = 0.58f;
        health = 1.8f;
        speed = RandomFloat(3.4f, 4.4f);
        scoreValue = 28;
        color = Color{235, 80, 255, 255};
    } else if (type == EnemyType::Harrier) {
        enemyRadius = 0.56f;
        health = 1.4f;
        speed = config_.harrierSpeed;
        scoreValue = 34;
        color = Color{135, 240, 255, 255};
    } else if (type == EnemyType::Blinker) {
        enemyRadius = 0.62f;
        health = 2.1f;
        speed = RandomFloat(3.3f, 4.2f);
        scoreValue = 42;
        color = Color{255, 70, 205, 255};
    } else if (type == EnemyType::Boss) {
        enemyRadius = 2.4f;
        health = config_.bossHealth;
        speed = 2.3f;
        scoreValue = 650;
        color = Color{120, 95, 255, 255};
    } else if (type == EnemyType::SlimeKing) {
        enemyRadius = config_.slimeKingRadius;
        health = config_.slimeKingHealth;
        speed = config_.slimeKingSpeed;
        scoreValue = 800;
        color = Color{100, 220, 140, 255};
    } else if (type == EnemyType::Minotaur) {
        enemyRadius = 1.35f;
        health = config_.labyrinthMinotaurHealth;
        speed = config_.labyrinthMinotaurSpeed;
        scoreValue = 1000;
        color = Color{92, 62, 42, 255};
    } else if (type == EnemyType::Duelist) {
        enemyRadius = 0.82f;
        health = config_.duelistHealth;
        speed = 4.9f;
        scoreValue = 900;
        color = Color{255, 225, 135, 255};
    } else if (type == EnemyType::Dummy) {
        enemyRadius = 0.65f;
        health = config_.dummyHealth;
        speed = RandomFloat(1.0f, 2.0f);
        scoreValue = 0;
        color = Color{120, 120, 130, 255};
    } else if (type == EnemyType::DummyBoss) {
        enemyRadius = 2.4f;
        health = config_.bossHealth;
        speed = 2.3f;
        scoreValue = 0;
        color = Color{110, 110, 125, 255};
    }

    PhysicsWorld::BodyConfig enemyConfig;
    enemyConfig.motionType = JPH::EMotionType::Dynamic;
    enemyConfig.layer = Layers::MOVING;
    enemyConfig.gravityFactor = !IsSphericalMap() && (type == EnemyType::Pouncer || type == EnemyType::SlimeKing) ? 0.75f : 0.0f;
    enemyConfig.linearDamping = 0.0f;
    enemyConfig.angularDamping = 1.0f;
    enemyConfig.friction = 0.0f;
    enemyConfig.allowSleeping = false;

    JPH::BodyID body = physics_.CreateBody(
        enemyShape_,
        ToJoltVector(position),
        JPH::Quat::sIdentity(),
        enemyConfig);

    enemies_.push_back(Enemy{body, type, enemyRadius, speed, health, health, RandomFloat(0.0f, 6.28f), RandomFloat(0.0f, 1.0f), RandomFloat(0.4f, 1.6f), RandomFloat(0.0f, 0.5f), 0, 0, RandomFloat(config_.duelistWeaponSwitchMin, config_.duelistWeaponSwitchMax), 0.0f, scoreValue, color, Vector3Zero(), Vector3Zero(), false, 0});
    if (type == EnemyType::Boss || type == EnemyType::Duelist || type == EnemyType::SlimeKing || type == EnemyType::Minotaur) {
        PlaySfxAt(sfxBossSpawn_, position, 110.0f, 1.0f);
    }
    if (timeStopped_) {
        Enemy& enemy = enemies_.back();
        enemy.frozen = true;
        enemy.storedVelocity = Vector3Zero();
        physics_.Bodies().SetLinearVelocity(enemy.body, JPH::Vec3::sZero());
    }
}
bool Game::BossAlive() const {
    for (const Enemy& enemy : enemies_) {
        if (enemy.type == EnemyType::Boss) {
            return true;
        }
    }
    return false;
}
bool Game::DuelMode() const {
    return config_.gameMode == "duel";
}
bool Game::TutorialMode() const {
    return config_.gameMode == "tutorial";
}
void Game::RecordDummyDamage(const Enemy& enemy, float damage) {
    for (DamageNumber& dn : damageNumbers_) {
        if (dn.life > 0.0f && dn.enemyBody == enemy.body) {
            dn.value += damage;
            dn.life = dn.maxLife;
            dn.screenYOffset = 0.0f;
            return;
        }
    }
    Vector3 pos = BodyPosition(enemy.body);
    Vector3 up = IsSphericalMap() ? SphericalUpAt(pos) : Vector3{0.0f, 1.0f, 0.0f};
    float hOff = enemy.radius + 0.8f;
    Vector3 spawnPos = Vector3Add(pos, Vector3Scale(up, hOff));
    damageNumbers_.push_back(DamageNumber{enemy.body, spawnPos, damage, 1.2f, 1.2f, 0.0f, hOff});
}
void Game::ShowTutorialTip(const char* text) {
    strncpy(tutorialTip_, text, sizeof(tutorialTip_) - 1);
    tutorialTip_[sizeof(tutorialTip_) - 1] = '\0';
    tutorialTipTimer_ = 5.0f;
    tutorialTipDuration_ = 5.0f;
}
bool Game::DuelWon() const {
    return DuelMode() && duelWon_;
}
bool Game::FaminePressureActive() const {
    return famineRider_.active && !IsEdenMap();
}
void Game::SwitchDuelistWeapon(Enemy& enemy, float distance) {
    int roll = GetRandomValue(0, 99);
    if (enemy.aiTier >= 1) {
        // --- Strategic AI: counter-pick based on player's weapon ---
        int playerSlot = static_cast<int>(activeWeapon_);
        if (playerSlot == 8) playerSlot = 8; // MysticStaff

        // Counter matrix: {playerWeapon → preferred counter slots}
        // Laser(0)→Spear(6)/Gauntlet(5) | Flame(1)→Rocket(2)/Spear(6)
        // Rocket(2)→Gauntlet(5)/Shield(Staff 8) | Shotgun(3)→Flame(1)/Staff(8)
        // Nail(4)→Gauntlet(5)/Nano(7) | Gauntlet(5)→Laser(0)/Flame(1)
        // Spear(6)→Shotgun(3)/Staff(8) | Nano(7)→Spear(6)/Rocket(2)
        // Staff(8)→Rocket(2)/Laser(0)
        static const int counters[9][3] = {
            {6,5,2},  // vs Laser: Spear, Gauntlet, Rocket
            {2,6,5},  // vs Flame: Rocket, Spear, Gauntlet
            {5,8,0},  // vs Rocket: Gauntlet, Staff, Laser
            {1,8,6},  // vs Shotgun: Flame, Staff, Spear
            {5,7,2},  // vs Nail: Gauntlet, Nano, Rocket
            {0,1,3},  // vs Gauntlet: Laser, Flame, Shotgun
            {3,8,1},  // vs Spear: Shotgun, Staff, Flame
            {6,2,0},  // vs Nano: Spear, Rocket, Laser
            {2,0,6},  // vs Staff: Rocket, Laser, Spear
        };
        int picks[3] = {counters[playerSlot][0], counters[playerSlot][1], counters[playerSlot][2]};
        // 60% pick best counter, 25% second, 15% third (with distance adjustment)
        int idx = roll < 60 ? 0 : roll < 85 ? 1 : 2;
        // Blend with distance logic: close range → favor close-range weapons
        if (distance < 7.0f && roll > 70) {
            int closeOpts[] = {3, 1, 5, 6}; // Shotgun, Flame, Gauntlet, Spear
            picks[idx] = closeOpts[roll % 4];
        }
        enemy.weaponSlot = picks[idx];
    } else {
        // --- Random Barrage AI (original logic) ---
        if (distance > 50.0f) {
            enemy.weaponSlot = roll < 24 ? 1 : roll < 44 ? 6 : roll < 65 ? 2 : roll < 84 ? 7 : 4;
        } else if (distance > 18.0f) {
            enemy.weaponSlot = roll < 18 ? 0 : roll < 34 ? 3 : roll < 51 ? 4 : roll < 67 ? 2 : roll < 83 ? 7 : 5;
        } else {
            enemy.weaponSlot = roll < 28 ? 3 : roll < 50 ? 1 : roll < 72 ? 5 : roll < 88 ? 4 : 6;
        }
    }
    enemy.weaponSwitchTimer = RandomFloat(config_.duelistWeaponSwitchMin, config_.duelistWeaponSwitchMax);
    enemy.telegraphTimer = enemy.weaponSlot == 2 || enemy.weaponSlot == 4 || enemy.weaponSlot == 6 || enemy.weaponSlot == 7 ? 0.35f : 0.16f;
}
void Game::UpdateDuelist(Enemy& enemy, Vector3 position, Vector3 direction, float dt, float& speed, bool& skipVelocity) {
    Vector3 toPlayer = Vector3Subtract(camera_.position, position);
    float distance = Vector3Length(toPlayer);
    Vector3 localUp = UpForWorldAt(position, enemy.world);
    Vector3 tangent = IsSphericalMap()
        ? SafeNormalize(Vector3CrossProduct(localUp, direction), PlayerRight())
        : SafeNormalize(Vector3CrossProduct(localUp, direction), Vector3{-direction.z, 0.0f, direction.x});

    // Strategic AI: adaptive movement
    if (enemy.aiTier >= 1) {
        float playerSpeed = Vector3Length(playerVelocity_);
        bool playerRushing = distance < 8.0f && playerSpeed > 12.0f;
        bool playerFar = distance > 22.0f;

        if (playerRushing) {
            // Evasive: backstep hard, wide strafe
            float rangeBias = -0.9f;
            direction = Vector3Normalize(Vector3Add(Vector3Scale(direction, rangeBias), Vector3Scale(tangent, std::sin(enemy.bobTimer * 2.5f) * 1.0f)));
        } else if (playerFar) {
            // Rush: close distance aggressively + spear thrust later if weapon=6
            float rangeBias = 0.8f;
            direction = Vector3Normalize(Vector3Add(Vector3Scale(direction, rangeBias), Vector3Scale(tangent, std::sin(enemy.bobTimer * 1.2f) * 0.3f)));
            if (enemy.weaponSlot == 6 && enemy.cooldownTimer <= 0.0f && distance < 25.0f) {
                // Spear thrust boost toward player
                AddEnemyImpulse(enemy, Vector3Scale(Vector3Normalize(toPlayer), config_.longinusSpearThrustImpulse * 0.5f));
                enemy.cooldownTimer = 0.5f;
            }
        } else {
            // Mid-range: circle strafe
            float rangeBias = 0.05f;
            direction = Vector3Normalize(Vector3Add(Vector3Scale(direction, rangeBias), Vector3Scale(tangent, std::sin(enemy.bobTimer * 1.9f) * 0.7f)));
        }

        // Equipment toggling (every 15s)
        enemy.equipmentTimer -= dt;
        if (enemy.equipmentTimer <= 0.0f) {
            enemy.equipmentTimer = RandomFloat(12.0f, 20.0f);
            int gearRoll = GetRandomValue(0, 2);
            if (gearRoll == 0 && !enemy.usingSpaceSuit) {
                enemy.usingSpaceSuit = true;
                enemy.usingFlightRig = enemy.usingSkates = false;
            } else if (gearRoll == 1 && !enemy.usingFlightRig) {
                enemy.usingFlightRig = true;
                enemy.usingSpaceSuit = enemy.usingSkates = false;
            } else {
                enemy.usingSkates = true;
                enemy.usingSpaceSuit = enemy.usingFlightRig = false;
            }
        }

        // Equipment effects on speed
        if (enemy.usingSpaceSuit) speed = enemy.speed * 1.35f;  // faster aerial movement
        else if (enemy.usingSkates) speed = enemy.speed * 1.5f;   // faster ground speed
        else if (enemy.usingFlightRig) speed = enemy.speed * 1.2f; // moderate hover speed
        else speed = enemy.speed;

        // Shield cooldown tick (for staff shield)
        enemy.shieldCooldown = std::max(0.0f, enemy.shieldCooldown - dt);
    } else {
        // Original movement: simple range bias + sine strafe
        float rangeBias = distance < 9.0f ? -0.75f : distance > 18.0f ? 0.55f : 0.1f;
        direction = Vector3Normalize(Vector3Add(Vector3Scale(direction, rangeBias), Vector3Scale(tangent, std::sin(enemy.bobTimer * 1.7f) * 0.8f)));
        speed = enemy.speed;
    }

    enemy.weaponSwitchTimer -= dt;
    enemy.telegraphTimer = std::max(0.0f, enemy.telegraphTimer - dt);
    if (enemy.weaponSwitchTimer <= 0.0f) {
        SwitchDuelistWeapon(enemy, distance);
    }

    // Blink defense (both tiers, strategic triggers more aggressively)
    bool shouldBlink = enemy.weaponSlot == 5 && distance < 8.0f && enemy.cooldownTimer <= 0.0f;
    if (enemy.aiTier >= 1 && !shouldBlink && distance < 5.0f && enemy.cooldownTimer <= 0.0f && GetRandomValue(0, 99) < 30) {
        // Strategic AI: emergency blink at very close range regardless of weapon
        shouldBlink = true;
    }
    if (shouldBlink) {
        BlinkDuelist(enemy, Vector3Normalize(Vector3Subtract(position, camera_.position)));
        enemy.cooldownTimer = 2.6f / config_.duelistFireRateScale;
        skipVelocity = true;
        return;
    }

    if (enemy.cooldownTimer <= 0.0f && enemy.telegraphTimer <= 0.0f) {
        FireDuelistWeapon(enemy, position, Vector3Normalize(toPlayer));
    }

    enemy.externalVelocity = Vector3Scale(enemy.externalVelocity, std::pow(0.12f, dt));
    Vector3 velocity = {};
    if (IsSphericalMap()) {
        float targetAlt = SphericalEnemyAltitude(EnemyType::Duelist);
        if (enemy.usingFlightRig) targetAlt += 4.0f;
        float radialCorrection = std::clamp((SphericalSignedRadius(targetAlt, enemy.world) - Vector3Length(position)) * 7.0f, -12.0f, 8.0f);
        if (IsHollowPhysicsForWorld(enemy.world)) radialCorrection = -radialCorrection;
        velocity = Vector3Add(Vector3Add(Vector3Scale(direction, speed), Vector3Scale(localUp, radialCorrection)), enemy.externalVelocity);
    } else {
        float targetAltitude = enemy.usingFlightRig ? 5.0f : 1.2f;
        float targetY = FlatGroundYForWorld(enemy.world) + FlatUpForWorld(enemy.world).y * targetAltitude;
        float gravScale = enemy.usingSpaceSuit ? 0.24f : 1.0f;
        velocity = Vector3{
            direction.x * speed + enemy.externalVelocity.x,
            std::clamp((targetY - position.y) * 7.0f * gravScale, -12.0f, 8.0f) + enemy.externalVelocity.y,
            direction.z * speed + enemy.externalVelocity.z
        };
    }
    physics_.Bodies().SetLinearVelocity(enemy.body, ToJoltVelocity(velocity));
    skipVelocity = true;
}
void Game::FireDuelistWeapon(Enemy& enemy, Vector3 position, Vector3 toPlayer) {
    Vector3 localUp = UpForWorldAt(position, enemy.world);
    Vector3 origin = Vector3Add(position, Vector3Scale(localUp, enemy.radius * 0.35f));
    Vector3 target = camera_.position;
    float distanceToPlayer = Vector3Distance(origin, target);
    float aimSpread = 0.22f + distanceToPlayer * 0.018f;
    if (enemy.weaponSlot == 2 || enemy.weaponSlot == 4 || enemy.weaponSlot == 7) {
        aimSpread *= 1.45f;
    } else if (enemy.weaponSlot == 0 || enemy.weaponSlot == 6) {
        aimSpread *= 1.1f;
    }

    Vector3 baseAim = Vector3Normalize(Vector3Subtract(target, origin));
    Vector3 aimRight = SafeNormalize(Vector3CrossProduct(baseAim, localUp), Vector3{1.0f, 0.0f, 0.0f});
    if (Vector3Length(aimRight) <= 0.001f) {
        aimRight = Vector3{1.0f, 0.0f, 0.0f};
    }
    Vector3 aimUp = Vector3Normalize(Vector3CrossProduct(aimRight, baseAim));
    target = Vector3Add(target, Vector3Scale(aimRight, RandomFloat(-aimSpread, aimSpread)));
    target = Vector3Add(target, Vector3Scale(aimUp, RandomFloat(-aimSpread * 0.55f, aimSpread * 0.28f)));
    Vector3 aimDirection = Vector3Normalize(Vector3Subtract(target, origin));
    Vector3 right = SafeNormalize(Vector3CrossProduct(aimDirection, localUp), Vector3{1.0f, 0.0f, 0.0f});
    if (Vector3Length(right) <= 0.001f) {
        right = Vector3{1.0f, 0.0f, 0.0f};
    }
    Vector3 up = localUp;
    float rate = config_.duelistFireRateScale;

    if (enemy.weaponSlot == 0) {
        if (GetRandomValue(0, 99) < 24) {
            FireEnemyBeam(origin, aimDirection, RandomFloat(0.55f, 0.9f));
            enemy.cooldownTimer = 1.05f / rate;
            enemy.telegraphTimer = 0.34f;
        } else {
            FireEnemyProjectile(ProjectileKind::LaserShot, origin, aimDirection, 58.0f, config_.plasmaDamage, 1.6f, 0.17f, 0.17f, Color{255, 235, 145, 255}, enemy.world, enemy.body);
            enemy.cooldownTimer = 0.28f / rate;
        }
    } else if (enemy.weaponSlot == 1) {
        if (distanceToPlayer > 9.0f) {
            // Fire napalm grenade (self-damage immune via shooterBody)
            Vector3 napalmVel = Vector3Scale(aimDirection, config_.napalmSpeed);
            napalmVel = Vector3Add(napalmVel, Vector3Scale(localUp, 4.0f));
            napalmGrenades_.push_back(NapalmGrenade{
                origin, napalmVel,
                config_.napalmFuse, config_.napalmFuse, 0, enemy.world,
                enemy.body
            });
            enemy.cooldownTimer = 1.1f / rate;
            enemy.telegraphTimer = 0.3f;
        } else {
            Vector3 flameDirection = Vector3Normalize(Vector3Add(aimDirection, Vector3Add(Vector3Scale(right, RandomFloat(-0.12f, 0.12f)), Vector3Scale(up, RandomFloat(-0.04f, 0.08f)))));
            FireEnemyProjectile(ProjectileKind::Flame, origin, flameDirection, RandomFloat(17.0f, 22.0f), config_.flameDamage, config_.flameLifetime, 0.14f, config_.flameMaxRadius, Color{255, 120, 34, 235}, enemy.world, enemy.body);
            enemy.cooldownTimer = 0.12f / rate;
        }
    } else if (enemy.weaponSlot == 2) {
        if (GetRandomValue(0, 99) < 22) {
            for (int i = 0; i < 3; ++i) {
                Vector3 direction = Vector3Normalize(Vector3Add(aimDirection, Vector3Add(Vector3Scale(right, RandomFloat(-0.18f, 0.18f)), Vector3Scale(up, RandomFloat(-0.04f, 0.1f)))));
                FireEnemyProjectile(ProjectileKind::Rocket, origin, direction, 22.0f, config_.rocketImpactDamage, 2.6f, 0.28f, 0.28f, Color{245, 190, 130, 255}, enemy.world, enemy.body);
            }
            enemy.cooldownTimer = 1.65f / rate;
        } else {
            FireEnemyProjectile(ProjectileKind::Rocket, origin, aimDirection, 27.0f, config_.rocketImpactDamage, 2.8f, 0.34f, 0.34f, Color{245, 190, 130, 255}, enemy.world, enemy.body);
            enemy.cooldownTimer = 1.15f / rate;
        }
        enemy.telegraphTimer = 0.28f;
    } else if (enemy.weaponSlot == 3) {
        bool glass = GetRandomValue(0, 99) < 32;
        int pelletCount = glass ? 5 : 7;
        for (int i = 0; i < pelletCount; ++i) {
            float side = RandomFloat(glass ? -0.08f : -0.16f, glass ? 0.08f : 0.16f);
            float lift = RandomFloat(glass ? -0.045f : -0.08f, glass ? 0.055f : 0.08f);
            Vector3 direction = Vector3Normalize(Vector3Add(aimDirection, Vector3Add(Vector3Scale(right, side), Vector3Scale(up, lift))));
            FireEnemyProjectile(glass ? ProjectileKind::GlassShard : ProjectileKind::Pellet, origin, direction, glass ? config_.glassShardSpeed * 0.78f : RandomFloat(42.0f, 50.0f), glass ? config_.glassShardDamage : config_.shotgunPelletDamage, glass ? config_.glassShardLingerTime : 0.58f, glass ? 0.13f : 0.1f, glass ? 0.13f : 0.1f, glass ? Color{190, 245, 255, 255} : Color{255, 205, 130, 255}, enemy.world, enemy.body);
        }
        AddEnemyImpulse(enemy, Vector3Scale(aimDirection, -5.0f));
        enemy.cooldownTimer = (glass ? 1.05f : 0.82f) / rate;
    } else if (enemy.weaponSlot == 4) {
        // Strategic AI avoids self-damaging black hole; random AI only uses it at long range
        bool blackHole = false;
        if (enemy.aiTier >= 1) {
            blackHole = false;  // never self-damage
        } else {
            blackHole = distanceToPlayer > 22.0f && GetRandomValue(0, 99) < 1;
        }
        FireEnemyProjectile(blackHole ? ProjectileKind::BlackHoleGrenade : ProjectileKind::GravityNail, origin, aimDirection, blackHole ? 22.0f : 58.0f, blackHole ? config_.blackHoleGrenadeDamage : config_.gravityNailDamage, blackHole ? 1.55f : 1.0f, blackHole ? 0.28f : 0.15f, blackHole ? 0.28f : 0.15f, blackHole ? Color{120, 70, 190, 255} : Color{170, 200, 255, 255}, enemy.world, enemy.body);
        enemy.cooldownTimer = (blackHole ? 1.55f : 0.9f) / rate;
        enemy.telegraphTimer = 0.3f;
    } else if (enemy.weaponSlot == 6) {
        int burst = GetRandomValue(0, 99) < 26 ? 2 : 1;
        for (int i = 0; i < burst; ++i) {
            Vector3 direction = Vector3Normalize(Vector3Add(aimDirection, Vector3Add(Vector3Scale(right, RandomFloat(-0.06f, 0.06f)), Vector3Scale(up, RandomFloat(-0.035f, 0.045f)))));
            FireEnemyProjectile(ProjectileKind::Lance, origin, direction, config_.longinusSpearSpeed * 0.82f, config_.longinusSpearDamage, 1.15f, 0.28f, 0.28f, Color{255, 175, 70, 255}, enemy.world, enemy.body);
        }
        AddEnemyImpulse(enemy, Vector3Scale(aimDirection, -config_.longinusSpearImpulse * (burst > 1 ? 0.45f : 0.32f)));
        enemy.cooldownTimer = (burst > 1 ? 1.35f : 1.05f) / rate;
    } else if (enemy.weaponSlot == 7) {
        if (GetRandomValue(0, 99) < 28) {
            SpawnEnemyNanoPlatform(origin, aimDirection, enemy.world);
            enemy.cooldownTimer = 1.4f / rate;
            enemy.telegraphTimer = 0.28f;
        } else {
            SpawnEnemyNanoBlade(origin, aimDirection, enemy.world);
            enemy.cooldownTimer = 1.25f / rate;
            enemy.telegraphTimer = 0.35f;
        }
    } else if (enemy.weaponSlot == 5) {
        BlinkDuelist(enemy, Vector3Normalize(Vector3Subtract(position, camera_.position)));
        enemy.cooldownTimer = 2.35f / rate;
        enemy.telegraphTimer = 0.35f;
    }
}
void Game::SpawnBethlehem() {
    bethlehem_.active = true;
    bethlehem_.health = config_.bethlehemHealth;
    bethlehem_.maxHealth = config_.bethlehemHealth;
    bethlehem_.attackTimer = 1.5f;
    bethlehem_.phaseTimer = 0.0f;
    bethlehem_.essenceTimer = RandomFloat(config_.bethlehemEssenceIntervalMin, config_.bethlehemEssenceIntervalMax);
    bethlehem_.laserPhase = BethlehemLaserPhase::Inactive;
    bethlehem_.orbitAngle = 0.0f;

    if (IsSphericalMap()) {
        if (IsHollowWorldMap()) {
            bethlehem_.position = Vector3{0.0f, 0.0f, 0.0f};
        } else {
            float r = config_.bethlehemOrbitRadius;
            bethlehem_.position = SphericalSurfacePoint(Vector3{r, 0.0f, 0.0f}, config_.bethlehemOrbitAltitude);
        }
    } else {
        bethlehem_.position = Vector3{0.0f, config_.bethlehemOrbitAltitude, 0.0f};
    }
    bethlehem_.laserDirection = Vector3{0.0f, -1.0f, 0.0f};
    PlaySfxAt(sfxBossSpawn_, bethlehem_.position, 140.0f, 1.0f);
}
void Game::DestroyBethlehem() {
    if (!bethlehem_.active) return;
    bethlehem_.active = false;
    bethlehem_.laserPhase = BethlehemLaserPhase::Inactive;
    StopSfx(sfxBethlehemLaserFire_);
    PlaySfxAt(sfxBossDeath_, bethlehem_.position, 140.0f, 1.0f);
    SpawnShockwave(bethlehem_.position, 14.0f, Color{255, 180, 60, 255});
    SpawnHitBurst(bethlehem_.position, Color{255, 220, 140, 255}, 110);
    // Spawn falling essence pickups in random directions
    Vector3 launchUp = IsSphericalMap() ? SphericalUpAt(bethlehem_.position) : Vector3{0.0f, 1.0f, 0.0f};
    Vector3 launchA = {}, launchB = {};
    bool hollowCenter = IsHollowWorldMap();
    if (!hollowCenter) {
        if (IsSphericalMap()) {
            launchA = {1.0f, 0.0f, 0.0f};
            launchA = SafeNormalize(ProjectOnSphericalTangent(launchA, launchUp), Vector3{1.0f, 0.0f, 0.0f});
            launchB = SafeNormalize(Vector3CrossProduct(launchUp, launchA), Vector3{0.0f, 0.0f, 1.0f});
            float spin = RandomFloat(0.0f, 6.2831853f);
            float c = std::cos(spin), s = std::sin(spin);
            Vector3 ta = Vector3Add(Vector3Scale(launchA, c), Vector3Scale(launchB, s));
            Vector3 tb = Vector3Add(Vector3Scale(launchA, -s), Vector3Scale(launchB, c));
            launchA = ta; launchB = tb;
        } else { launchA = {1.0f, 0.0f, 0.0f}; launchB = {0.0f, 0.0f, 1.0f}; }
    }
    for (int ei = 0; ei < config_.bethlehemEssenceDeathCount; ++ei) {
        float speed = config_.bethlehemEssenceDeathSpeed * RandomFloat(0.6f, 1.4f);
        float lift = RandomFloat(config_.bethlehemEssenceDeathLift * 0.5f, config_.bethlehemEssenceDeathLift * 1.5f);
        Vector3 vel;
        if (hollowCenter) {
            float phi = std::acos(RandomFloat(-1.0f, 1.0f));
            float theta = RandomFloat(0.0f, 6.2831853f);
            Vector3 dir = {std::sin(phi) * std::cos(theta), std::cos(phi), std::sin(phi) * std::sin(theta)};
            vel = Vector3Scale(dir, speed);
        } else {
            float angle = RandomFloat(0.0f, 6.2831853f);
            vel = Vector3Add(Vector3Add(Vector3Scale(launchA, std::cos(angle) * speed), Vector3Scale(launchUp, lift)), Vector3Scale(launchB, std::sin(angle) * speed));
        }
        Pickup p;
        p.type = PickupType::Essence;
        p.position = bethlehem_.position;
        p.velocity = vel;
        p.horizontalDrag = config_.bethlehemEssenceDeathDrag;
        p.gravityScale = config_.bethlehemEssenceDeathGravity;
        p.bobTimer = RandomFloat(0.0f, 6.28f);
        pickups_.push_back(p);
    }
    eventText_ = "STAR FALLEN";
    eventTextTimer_ = 4.0f;
    cameraShake_ = 1.0f;
    score_ += 1000;
}
void Game::UpdateBethlehem(float dt) {
    if (!bethlehem_.active) return;

    // Position
    Vector3 playerPos = camera_.position;
    if (IsSphericalMap()) {
        if (IsHollowWorldMap()) {
            bethlehem_.position = Vector3{0.0f, 0.0f, 0.0f};
        } else {
            float angularSpeed = (2.0f * PI) / config_.bethlehemOrbitPeriod;
            bethlehem_.orbitAngle += dt * angularSpeed;
            Vector3 orbitPos = {config_.bethlehemOrbitRadius * std::cos(bethlehem_.orbitAngle), 0.0f, config_.bethlehemOrbitRadius * std::sin(bethlehem_.orbitAngle)};
            bethlehem_.position = SphericalSurfacePoint(orbitPos, config_.bethlehemOrbitAltitude);
        }
    } else {
        bethlehem_.position = Vector3{0.0f, config_.bethlehemOrbitAltitude, 0.0f};
    }

    // Periodically launch essence pickups while alive
    bethlehem_.essenceTimer -= dt;
    if (bethlehem_.essenceTimer <= 0.0f) {
        bethlehem_.essenceTimer = RandomFloat(config_.bethlehemEssenceIntervalMin, config_.bethlehemEssenceIntervalMax);
        float angle = RandomFloat(0.0f, 6.2831853f);
        float speed = config_.bethlehemEssenceLaunchSpeed * RandomFloat(0.7f, 1.3f);
        float lift = RandomFloat(config_.bethlehemEssenceLaunchLift * 0.5f, config_.bethlehemEssenceLaunchLift * 1.5f);
        Vector3 up = IsSphericalMap() ? SphericalUpAt(bethlehem_.position) : Vector3{0.0f, 1.0f, 0.0f};
        Vector3 vel;
        if (IsHollowWorldMap()) {
            // At center: launch in fully random 3D direction
            float phi = std::acos(RandomFloat(-1.0f, 1.0f));
            float theta = RandomFloat(0.0f, 6.2831853f);
            Vector3 dir = {std::sin(phi) * std::cos(theta), std::cos(phi), std::sin(phi) * std::sin(theta)};
            vel = Vector3Scale(dir, speed);
        } else {
            Vector3 tangentA = {1.0f, 0.0f, 0.0f}, tangentB = {0.0f, 0.0f, 1.0f};
            if (IsSphericalMap()) {
                tangentA = SafeNormalize(ProjectOnSphericalTangent(tangentA, up), Vector3{1.0f, 0.0f, 0.0f});
                tangentB = SafeNormalize(Vector3CrossProduct(up, tangentA), Vector3{0.0f, 0.0f, 1.0f});
                float spin = RandomFloat(0.0f, 6.2831853f);
                float c = std::cos(spin), s = std::sin(spin);
                Vector3 ta = Vector3Add(Vector3Scale(tangentA, c), Vector3Scale(tangentB, s));
                Vector3 tb = Vector3Add(Vector3Scale(tangentA, -s), Vector3Scale(tangentB, c));
                tangentA = ta; tangentB = tb;
            }
            vel = Vector3Add(Vector3Add(Vector3Scale(tangentA, std::cos(angle) * speed), Vector3Scale(up, lift)), Vector3Scale(tangentB, std::sin(angle) * speed));
        }
        Pickup p;
        p.type = PickupType::Essence;
        p.position = bethlehem_.position;
        p.velocity = vel;
        p.horizontalDrag = config_.bethlehemEssenceFallDrag;
        p.gravityScale = config_.bethlehemEssenceFallGravity;
        p.bobTimer = RandomFloat(0.0f, 6.28f);
        pickups_.push_back(p);
    }

    // In tutorial mode, keep orbital movement but skip laser attacks
    if (TutorialMode()) return;

    // Laser state machine
    bethlehem_.attackTimer -= dt;
    bethlehem_.phaseTimer += dt;

    if (bethlehem_.laserPhase == BethlehemLaserPhase::Inactive) {
        if (bethlehem_.attackTimer <= 0.0f) {
            bethlehem_.laserPhase = BethlehemLaserPhase::Warning;
            bethlehem_.phaseTimer = 0.0f;
            Vector3 toPlayer = Vector3Subtract(playerPos, bethlehem_.position);
            bethlehem_.laserDirection = Vector3Length(toPlayer) > 0.001f ? Vector3Normalize(toPlayer) : Vector3{0.0f, -1.0f, 0.0f};
            Vector3 beamStart = bethlehem_.position;
            Vector3 beamEnd = Vector3Add(beamStart, Vector3Scale(bethlehem_.laserDirection, config_.bethlehemLaserRange));
            Vector3 closest = ClosestPointOnSegmentLocal(camera_.position, beamStart, beamEnd);
            PlaySfxAt(sfxBethlehemLaserWarn_, closest, 90.0f, 1.0f);
        }
    } else if (bethlehem_.laserPhase == BethlehemLaserPhase::Warning) {
        Vector3 toPlayer = Vector3Normalize(Vector3Subtract(playerPos, bethlehem_.position));
        float dot = Vector3DotProduct(bethlehem_.laserDirection, toPlayer);
        float angleBetween = std::acos(std::clamp(dot, -1.0f, 1.0f));
        float maxRotate = config_.bethlehemLaserRotateSpeed * dt;
        float rotateAmount = std::min(angleBetween, maxRotate);
        if (rotateAmount > 0.0001f) {
            Vector3 axis = Vector3CrossProduct(bethlehem_.laserDirection, toPlayer);
            if (Vector3Length(axis) > 0.0001f) {
                axis = Vector3Normalize(axis);
                bethlehem_.laserDirection = Vector3Normalize(RotateAroundAxis(bethlehem_.laserDirection, axis, rotateAmount));
            }
        }
        if (bethlehem_.phaseTimer >= config_.bethlehemLaserWarningDuration) {
            bethlehem_.laserPhase = BethlehemLaserPhase::Damaging;
            bethlehem_.phaseTimer = 0.0f;
        }
    } else if (bethlehem_.laserPhase == BethlehemLaserPhase::Damaging) {
        // Continue tracking player during damaging phase
        Vector3 toPlayer = Vector3Normalize(Vector3Subtract(playerPos, bethlehem_.position));
        float dot = Vector3DotProduct(bethlehem_.laserDirection, toPlayer);
        float angleBetween = std::acos(std::clamp(dot, -1.0f, 1.0f));
        float maxRotate = config_.bethlehemLaserRotateSpeed * dt;
        float rotateAmount = std::min(angleBetween, maxRotate);
        if (rotateAmount > 0.0001f) {
            Vector3 axis = Vector3CrossProduct(bethlehem_.laserDirection, toPlayer);
            if (Vector3Length(axis) > 0.0001f) {
                axis = Vector3Normalize(axis);
                bethlehem_.laserDirection = Vector3Normalize(RotateAroundAxis(bethlehem_.laserDirection, axis, rotateAmount));
            }
        }
        Vector3 beamStart = bethlehem_.position;
        Vector3 beamEnd = Vector3Add(beamStart, Vector3Scale(bethlehem_.laserDirection, config_.bethlehemLaserRange));
        Vector3 closest = ClosestPointOnSegmentLocal(playerPos, beamStart, beamEnd);
        UpdateLoopingSfxAt(sfxBethlehemLaserFire_, closest, 90.0f, 1.0f);
        float dist = DistancePointToSegment(playerPos, beamStart, beamEnd);
        if (dist <= config_.bethlehemLaserRadius + playerRadius_) {
            ApplyPlayerHit(playerPos, Color{255, 160, 40, 255}, "STAR BURNT");
        }
        if (bethlehem_.phaseTimer >= config_.bethlehemLaserDuration) {
            bethlehem_.laserPhase = BethlehemLaserPhase::Inactive;
            bethlehem_.attackTimer = config_.bethlehemLaserCooldown;
            bethlehem_.phaseTimer = 0.0f;
            StopSfx(sfxBethlehemLaserFire_);
        }
    }
}

void Game::SpawnThroneAngel() {
    throneAngel_ = {};
    throneAngel_.active = true;
    throneAngel_.health = config_.throneHealth;
    throneAngel_.maxHealth = config_.throneHealth;
    throneAngel_.summonTimer = 2.5f;
    throneAngel_.pulseTimer = config_.thronePulseInterval * 0.65f;
    throneAngel_.wanderTimer = 0.0f;
    throneAngel_.world = 0;
    for (int i = 0; i < 3; ++i) {
        throneAngel_.ringAxes[i] = RandomUnitVector();
        throneAngel_.ringAngles[i] = RandomFloat(0.0f, 2.0f * PI);
        throneAngel_.ringSpeeds[i] = RandomFloat(0.55f, 1.45f) * (i % 2 == 0 ? 1.0f : -1.0f);
    }

    if (IsSphericalMap()) {
        Vector3 dir = SafeNormalize(camera_.position, Vector3{0.0f, 1.0f, 0.0f});
        if (IsHollowWorldMap()) dir = Vector3Scale(dir, -1.0f);
        throneAngel_.position = SphericalSurfacePoint(dir, config_.throneHoverAltitude, 0);
    } else {
        Vector3 up = FlatUpForWorld(0);
        throneAngel_.position = Vector3Add(Vector3{0.0f, FlatGroundYForWorld(0), 0.0f}, Vector3Scale(up, config_.throneHoverAltitude));
    }
    throneAngel_.wanderTarget = throneAngel_.position;
    PlaySfxAt(sfxBossSpawn_, throneAngel_.position, 150.0f, 1.0f);
    SpawnShockwave(throneAngel_.position, 10.0f, Color{235, 245, 255, 255});
}

void Game::SpawnCherubs(int count) {
    if (!throneAngel_.active || count <= 0) return;
    Vector3 up = UpForWorldAt(throneAngel_.position, 0);
    Vector3 right = {}, forward = {};
    MakeBasis(up, right, forward);
    for (int i = 0; i < count && static_cast<int>(cherubs_.size()) < config_.throneMaxCherubs; ++i) {
        float angle = RandomFloat(0.0f, 2.0f * PI);
        float radius = RandomFloat(1.8f, 5.5f);
        Vector3 offset = Vector3Add(Vector3Scale(right, std::cos(angle) * radius), Vector3Scale(forward, std::sin(angle) * radius));
        offset = Vector3Add(offset, Vector3Scale(up, RandomFloat(-1.2f, 1.2f)));
        CherubMinion cherub;
        cherub.position = Vector3Add(throneAngel_.position, offset);
        cherub.velocity = Vector3Add(Vector3Scale(offset, 0.7f), Vector3Scale(up, RandomFloat(-1.5f, 1.5f)));
        cherub.health = config_.cherubHealth;
        cherub.shootTimer = RandomFloat(0.4f, config_.cherubShotInterval);
        cherub.reacquireTimer = RandomFloat(0.0f, 0.25f);
        cherub.wingTimer = RandomFloat(0.0f, 6.28f);
        cherub.world = 0;
        cherubs_.push_back(cherub);
    }
    if (count > 0) {
        PlaySfxAt(sfxBossBarrage_, throneAngel_.position, 130.0f, 0.72f);
    }
}

void Game::TriggerThronePulse() {
    if (!throneAngel_.active && !throneAngel_.defeated) return;
    Vector3 origin = throneAngel_.position;
    float radius = config_.thronePulseRadius;
    float force = config_.thronePulseForce;
    Color pulseColor = throneAngel_.defeated ? Color{255, 174, 0, 255} : Color{235, 245, 255, 255};
    SpawnShockwave(origin, radius, pulseColor);
    SpawnHitBurst(origin, throneAngel_.defeated ? Color{255, 194, 0, 255} : Color{245, 250, 255, 255}, 46);
    PlaySfxAt(sfxThronePulse_, origin, 150.0f, 0.95f);
    cameraShake_ = std::min(0.85f, cameraShake_ + 0.35f);

    if (playerWorld_ == 0) {
        float dist = Vector3Distance(camera_.position, origin);
        if (dist <= radius + playerRadius_ && !PlayerInsideThroneLadderBeam()) {
            Vector3 dir = SafeNormalize(Vector3Subtract(camera_.position, origin), UpForWorldAt(camera_.position, 0));
            playerVelocity_ = Vector3Add(playerVelocity_, Vector3Scale(dir, force));
            ApplyAntigravity(config_.throneAntigravityDuration);
            eventText_ = "GRAVITY SEVERED";
            eventTextTimer_ = 1.4f;
        }
    }

    for (Enemy& enemy : enemies_) {
        if (enemy.world != 0) continue;
        Vector3 ep = BodyPosition(enemy.body);
        float dist = Vector3Distance(ep, origin);
        if (dist > radius + enemy.radius || dist <= 0.001f) continue;
        float falloff = 1.0f - std::clamp(dist / radius, 0.0f, 1.0f);
        AddEnemyImpulse(enemy, Vector3Scale(Vector3Normalize(Vector3Subtract(ep, origin)), force * (0.35f + falloff * 0.65f)));
        if (throneAngel_.defeated && config_.throneDefeatedPulseDamage > 0.0f) {
            float damage = config_.throneDefeatedPulseDamage * (0.35f + falloff * 0.65f);
            enemy.health -= damage;
            enemy.lastDamageTime = survivalTime_;
            SpawnHitBurst(ep, Color{255, 196, 20, 255}, 6);
        }
    }

    for (CherubMinion& cherub : cherubs_) {
        float dist = Vector3Distance(cherub.position, origin);
        if (dist > radius + 0.5f || dist <= 0.001f) continue;
        float falloff = 1.0f - std::clamp(dist / radius, 0.0f, 1.0f);
        cherub.velocity = Vector3Add(cherub.velocity, Vector3Scale(Vector3Normalize(Vector3Subtract(cherub.position, origin)), force * (0.45f + falloff * 0.75f)));
        cherub.antigravityTimer = std::max(cherub.antigravityTimer, config_.throneAntigravityDuration);
        if (throneAngel_.defeated && config_.throneDefeatedPulseDamage > 0.0f) {
            cherub.health -= config_.throneDefeatedPulseDamage * (0.45f + falloff * 0.75f);
            cherub.flashTimer = 0.18f;
        }
    }

    for (Projectile& projectile : projectiles_) {
        if (projectile.world != 0) continue;
        Vector3 pp = BodyPosition(projectile.body);
        float dist = Vector3Distance(pp, origin);
        if (dist > radius + projectile.radius || dist <= 0.001f) continue;
        float falloff = 1.0f - std::clamp(dist / radius, 0.0f, 1.0f);
        AddProjectileImpulse(projectile, Vector3Scale(Vector3Normalize(Vector3Subtract(pp, origin)), force * (0.45f + falloff * 0.75f)));
    }

    for (Pickup& pickup : pickups_) {
        float dist = Vector3Distance(pickup.position, origin);
        if (dist > radius + pickup.radius || dist <= 0.001f) continue;
        float falloff = 1.0f - std::clamp(dist / radius, 0.0f, 1.0f);
        pickup.velocity = Vector3Add(pickup.velocity, Vector3Scale(Vector3Normalize(Vector3Subtract(pickup.position, origin)), force * (0.45f + falloff * 0.75f)));
        pickup.gravityScale = std::min(pickup.gravityScale, 0.05f);
    }
}

void Game::UpdateThroneAngel(float dt) {
    if (!throneAngel_.active && !throneAngel_.defeated) return;

    throneAngel_.hitFlash = std::max(0.0f, throneAngel_.hitFlash - dt * 4.5f);
    for (int i = 0; i < 3; ++i) {
        throneAngel_.ringAngles[i] += throneAngel_.ringSpeeds[i] * dt * (throneAngel_.defeated ? 0.55f : 1.0f);
    }

    Vector3 up = UpForWorldAt(throneAngel_.position, 0);
    if (throneAngel_.defeated) {
        throneAngel_.jacobLadderTimer = std::min(config_.throneJacobLadderOpenTime,
            throneAngel_.jacobLadderTimer + dt);
        throneAngel_.velocity = {};
        throneAngel_.summonTimer = config_.throneSummonInterval;
        UpdateThroneJacobLadder(dt);
        throneAngel_.pulseTimer -= dt;
        if (throneAngel_.pulseTimer <= 0.0f) {
            throneAngel_.pulseTimer = config_.thronePulseInterval * config_.throneDefeatedPulseIntervalScale;
            TriggerThronePulse();
        }
        return;
    }

    Vector3 right = {}, forward = {};
    MakeBasis(up, right, forward);
    throneAngel_.wanderTimer -= dt;
    if (throneAngel_.wanderTimer <= 0.0f || Vector3Distance(throneAngel_.position, throneAngel_.wanderTarget) < 2.5f) {
        throneAngel_.wanderTimer = RandomFloat(2.4f, 4.8f);
        if (IsSphericalMap()) {
            Vector3 baseDir = SafeNormalize(throneAngel_.position, Vector3{0.0f, 1.0f, 0.0f});
            float a = RandomFloat(0.0f, 2.0f * PI);
            float arc = RandomFloat(0.08f, 0.32f);
            Vector3 tangent = Vector3Add(Vector3Scale(right, std::cos(a)), Vector3Scale(forward, std::sin(a)));
            Vector3 dir = SafeNormalize(Vector3Add(Vector3Scale(baseDir, std::cos(arc)), Vector3Scale(tangent, std::sin(arc))), baseDir);
            throneAngel_.wanderTarget = SphericalSurfacePoint(dir, config_.throneHoverAltitude, 0);
        } else {
            float a = RandomFloat(0.0f, 2.0f * PI);
            float r = RandomFloat(0.0f, config_.throneWanderRadius);
            Vector3 flat = Vector3Add(Vector3Scale(right, std::cos(a) * r), Vector3Scale(forward, std::sin(a) * r));
            throneAngel_.wanderTarget = Vector3Add(Vector3{0.0f, FlatGroundYForWorld(0), 0.0f}, Vector3Add(flat, Vector3Scale(up, config_.throneHoverAltitude)));
        }
    }

    Vector3 toTarget = Vector3Subtract(throneAngel_.wanderTarget, throneAngel_.position);
    Vector3 desired = Vector3Length(toTarget) > 0.001f ? Vector3Scale(Vector3Normalize(toTarget), config_.throneMoveSpeed) : Vector3Zero();
    float blend = std::min(1.0f, dt * 1.8f);
    throneAngel_.velocity = Vector3Add(Vector3Scale(throneAngel_.velocity, 1.0f - blend), Vector3Scale(desired, blend));
    throneAngel_.position = Vector3Add(throneAngel_.position, Vector3Scale(throneAngel_.velocity, dt));
    if (IsSphericalMap()) {
        throneAngel_.position = SphericalSurfacePoint(throneAngel_.position, config_.throneHoverAltitude, 0);
    } else {
        throneAngel_.position.y = FlatGroundYForWorld(0) + config_.throneHoverAltitude;
    }

    throneAngel_.summonTimer -= dt;
    if (throneAngel_.summonTimer <= 0.0f) {
        throneAngel_.summonTimer = config_.throneSummonInterval;
        SpawnCherubs(config_.throneSummonCount);
    }

    throneAngel_.pulseTimer -= dt;
    if (throneAngel_.pulseTimer <= 0.0f) {
        throneAngel_.pulseTimer = config_.thronePulseInterval;
        TriggerThronePulse();
    }
}

bool Game::ThroneJacobLadderActive() const {
    return config_.throneJacobLadderEnabled && throneAngel_.defeated;
}

bool Game::PlayerInsideThroneLadderBeam(float* beamT, Vector3* axisPoint, float* radialRatio) const {
    if (!ThroneJacobLadderActive() || playerWorld_ != 0) return false;
    Vector3 up = UpForWorldAt(throneAngel_.position, 0);
    Vector3 down = Vector3Scale(up, -1.0f);
    Vector3 top = throneAngel_.position;
    Vector3 toPlayer = Vector3Subtract(camera_.position, top);
    float t = Vector3DotProduct(toPlayer, down);
    float length = config_.throneJacobLadderBeamLength;
    if (t < -playerRadius_ || t > length + playerRadius_) return false;
    float clampedT = std::clamp(t, 0.0f, length);
    Vector3 onAxis = Vector3Add(top, Vector3Scale(down, clampedT));
    float radiusT = length > 0.001f ? clampedT / length : 0.0f;
    float openProgress = std::clamp(throneAngel_.jacobLadderTimer / std::max(0.001f, config_.throneJacobLadderOpenTime), 0.0f, 1.0f);
    openProgress = openProgress * openProgress * (3.0f - 2.0f * openProgress);
    float beamRadius = config_.throneJacobLadderTopRadius
        + (config_.throneJacobLadderBottomRadius - config_.throneJacobLadderTopRadius) * radiusT;
    beamRadius *= (0.18f + 0.82f * openProgress);
    if (beamT) *beamT = clampedT;
    if (axisPoint) *axisPoint = onAxis;
    float radialDistance = Vector3Distance(camera_.position, onAxis);
    if (radialRatio) {
        *radialRatio = beamRadius > 0.001f ? std::clamp(radialDistance / beamRadius, 0.0f, 1.35f) : 0.0f;
    }
    return radialDistance <= beamRadius + playerRadius_;
}

void Game::UpdateThroneJacobLadder(float dt) {
    if (!ThroneJacobLadderActive() || playerWorld_ != 0) return;

    float beamT = 0.0f;
    Vector3 axisPoint = {};
    float radialRatio = 0.0f;
    if (!PlayerInsideThroneLadderBeam(&beamT, &axisPoint, &radialRatio)) return;

    Vector3 up = UpForWorldAt(throneAngel_.position, 0);
    Vector3 towardAxis = Vector3Subtract(axisPoint, camera_.position);
    Vector3 lift = Vector3Scale(up, config_.throneJacobLadderLiftSpeed);
    float outerPull = 0.35f + std::pow(std::clamp(radialRatio, 0.0f, 1.0f), 1.8f) * 1.65f;
    Vector3 pull = Vector3Length(towardAxis) > 0.001f
        ? Vector3Scale(Vector3Normalize(towardAxis), config_.throneJacobLadderAxisPull * outerPull)
        : Vector3Zero();
    float blend = std::clamp(dt * 4.5f, 0.0f, 1.0f);
    Vector3 desired = Vector3Add(lift, pull);
    playerVelocity_ = Vector3Add(Vector3Scale(playerVelocity_, 1.0f - blend), Vector3Scale(desired, blend));
    grounded_ = false;
    coyoteTimer_ = 0.0f;
    jumpBufferTimer_ = 0.0f;
    playerAntigravityTimer_ = std::max(playerAntigravityTimer_, 0.08f);

    if (!throneAngel_.jacobLadderEntered) {
        eventText_ = "JACOB'S LADDER";
        eventTextTimer_ = 1.8f;
    }

    if (Vector3Distance(camera_.position, throneAngel_.position) <= config_.throneJacobLadderContactRadius + playerRadius_) {
        TriggerEdenGatePlaceholder();
    }
}

void Game::TriggerEdenGatePlaceholder() {
    if (throneAngel_.jacobLadderEntered) return;
    throneAngel_.jacobLadderEntered = true;
    SpawnShockwave(throneAngel_.position, config_.thronePulseRadius * 0.65f, Color{255, 174, 0, 255});
    SpawnHitBurst(throneAngel_.position, Color{255, 196, 0, 255}, 90);
    PlaySfxAt(sfxBossPhase_, throneAngel_.position, 150.0f, 1.0f);
    cameraShake_ = std::min(1.0f, cameraShake_ + 0.55f);
    EnterEdenFromGate();
}

void Game::UpdateCherubs(float dt) {
    for (size_t i = 0; i < cherubs_.size();) {
        CherubMinion& cherub = cherubs_[i];
        cherub.reacquireTimer -= dt;
        cherub.shootTimer -= dt;
        cherub.antigravityTimer = std::max(0.0f, cherub.antigravityTimer - dt);
        cherub.flashTimer = std::max(0.0f, cherub.flashTimer - dt);
        cherub.wingTimer += dt * 9.0f;

        bool canSeePlayer = playerWorld_ == cherub.world;
        if (cherub.reacquireTimer <= 0.0f) {
            cherub.reacquireTimer = 0.25f;
            if (canSeePlayer) {
                cherub.target = camera_.position;
            } else if (HasWormhole()) {
                cherub.target = WormholeCenterForWorld(wormholes_.front(), cherub.world);
            } else if (throneAngel_.active) {
                cherub.target = throneAngel_.position;
            }
        }

        Vector3 steer = {};
        Vector3 toTarget = Vector3Subtract(cherub.target, cherub.position);
        if (Vector3Length(toTarget) > 0.001f) {
            steer = Vector3Add(steer, Vector3Scale(Vector3Normalize(toTarget), config_.cherubSpeed));
        }
        Vector3 center = {};
        int neighbors = 0;
        for (const CherubMinion& other : cherubs_) {
            float dist = Vector3Distance(cherub.position, other.position);
            if (dist <= 0.001f || dist > config_.cherubSeparation * 4.0f) continue;
            if (dist < config_.cherubSeparation) {
                steer = Vector3Add(steer, Vector3Scale(Vector3Normalize(Vector3Subtract(cherub.position, other.position)), config_.cherubSpeed * 0.9f));
            }
            center = Vector3Add(center, other.position);
            ++neighbors;
        }
        if (neighbors > 0) {
            center = Vector3Scale(center, 1.0f / static_cast<float>(neighbors));
            Vector3 toCenter = Vector3Subtract(center, cherub.position);
            if (Vector3Length(toCenter) > 0.001f) {
                steer = Vector3Add(steer, Vector3Scale(Vector3Normalize(toCenter), config_.cherubSpeed * 0.18f));
            }
        }

        Vector3 up = UpForWorldAt(cherub.position, cherub.world);
        float alt = IsSphericalMap()
            ? SphericalAltitudeAt(cherub.position, cherub.world)
            : Vector3DotProduct(Vector3Subtract(cherub.position, Vector3{0.0f, FlatGroundYForWorld(cherub.world), 0.0f}), FlatUpForWorld(cherub.world));
        float minAlt = 2.8f;
        float maxAlt = std::max(minAlt + 1.0f, config_.throneHoverAltitude * 0.82f);
        if (alt < minAlt) steer = Vector3Add(steer, Vector3Scale(up, config_.cherubSpeed));
        if (alt > maxAlt) steer = Vector3Subtract(steer, Vector3Scale(up, config_.cherubSpeed * 0.8f));
        if (cherub.antigravityTimer <= 0.0f) {
            steer = Vector3Subtract(steer, Vector3Scale(up, CurrentGravity() * 0.12f));
        }

        float blend = std::min(1.0f, dt * 3.5f);
        cherub.velocity = Vector3Add(Vector3Scale(cherub.velocity, 1.0f - blend), Vector3Scale(steer, blend));
        float speed = Vector3Length(cherub.velocity);
        float maxSpeed = config_.cherubSpeed * 1.6f;
        if (speed > maxSpeed) cherub.velocity = Vector3Scale(Vector3Normalize(cherub.velocity), maxSpeed);
        cherub.position = Vector3Add(cherub.position, Vector3Scale(cherub.velocity, dt));
        if (IsSphericalMap()) {
            float clampedAlt = std::clamp(SphericalAltitudeAt(cherub.position, cherub.world), minAlt, maxAlt);
            cherub.position = SphericalSurfacePoint(cherub.position, clampedAlt, cherub.world);
        } else {
            float groundY = FlatGroundYForWorld(cherub.world);
            cherub.position.y = std::clamp(cherub.position.y, groundY + minAlt, groundY + maxAlt);
        }

        if (canSeePlayer && cherub.shootTimer <= 0.0f && Vector3Distance(cherub.position, camera_.position) <= config_.cherubAttackRange) {
            Vector3 dir = SafeNormalize(Vector3Subtract(camera_.position, cherub.position), Vector3Scale(up, -1.0f));
            FireHomingShot(cherub.position, dir, config_.cherubShotSpeed, config_.cherubShotTurnRate, 5.0f,
                config_.cherubShotDamage, Color{245, 245, 225, 255}, ProjectileOwner::Enemy, cherub.world);
            cherub.shootTimer = config_.cherubShotInterval;
            cherub.flashTimer = 0.18f;
            PlaySfxAt(sfxBossBarrage_, cherub.position, 70.0f, 0.42f);
        }

        if (cherub.health <= 0.0f) {
            SpawnHitBurst(cherub.position, Color{245, 245, 235, 255}, 18);
            cherubs_[i] = cherubs_.back();
            cherubs_.pop_back();
            continue;
        }
        ++i;
    }
}

void Game::ApplyAntigravity(float duration) {
    playerAntigravityTimer_ = std::max(playerAntigravityTimer_, duration);
}

void Game::DamageThroneAngel(float damage, Vector3 hitPosition, Color color) {
    if (!throneAngel_.active || damage <= 0.0f) return;
    throneAngel_.health -= damage;
    totalDamageDealt_ += damage;
    throneAngel_.hitFlash = 1.0f;
    PlayEnemyHitSfx(throneAngel_.position);
    SpawnHitBurst(hitPosition, color, 12);
    if (throneAngel_.health <= 0.0f) {
        throneAngel_.active = false;
        throneAngel_.defeated = true;
        throneAngel_.health = 0.0f;
        throneAngel_.velocity = {};
        throneAngel_.pulseTimer = std::min(config_.thronePulseInterval * config_.throneDefeatedPulseIntervalScale, 1.2f);
        throneAngel_.jacobLadderTimer = 0.0f;
        throneAngel_.jacobLadderEntered = false;
        cherubs_.clear();
        SpawnShockwave(throneAngel_.position, config_.thronePulseRadius * 1.25f, Color{245, 250, 255, 255});
        SpawnHitBurst(throneAngel_.position, Color{245, 250, 255, 255}, 120);
        PlaySfxAt(sfxBossDeath_, throneAngel_.position, 150.0f, 1.0f);
        eventText_ = config_.throneJacobLadderEnabled ? "LADDER DESCENDS" : "THRONE SILENCED";
        eventTextTimer_ = 4.0f;
        cameraShake_ = 1.0f;
        score_ += 1200;
    }
}

void Game::DamageThroneAngelInRadius(Vector3 position, float radius, float damage, Color color) {
    if (!throneAngel_.active || damage <= 0.0f) return;
    float distance = Vector3Distance(position, throneAngel_.position);
    float hitRadius = radius + 4.2f;
    if (distance > hitRadius) return;
    float falloff = 1.0f - std::clamp(distance / std::max(0.001f, hitRadius), 0.0f, 1.0f);
    DamageThroneAngel(damage * (0.35f + falloff * 0.65f), throneAngel_.position, color);
}

void Game::SpawnSeraph() {
    seraphs_.clear();
    int count = std::clamp(config_.seraphSpawnCount, 1, 8);
    for (int i = 0; i < count; ++i) {
        SeraphBoss seraph;
        seraph.active = true;
        seraph.health = config_.seraphHealth;
        seraph.maxHealth = config_.seraphHealth;
        seraph.attackTimer = 2.0f + static_cast<float>(i) * config_.seraphAttackStagger;
        seraph.wanderTimer = 0.0f;
        seraph.wingTimer = RandomFloat(0.0f, 6.28f);
        seraph.world = 0;

        float angle = count > 1 ? (static_cast<float>(i) / static_cast<float>(count)) * 2.0f * PI : 0.0f;
        if (IsSphericalMap()) {
            Vector3 dir = SafeNormalize(camera_.position, Vector3{0.0f, 1.0f, 0.0f});
            if (IsHollowWorldMap()) dir = Vector3Scale(dir, -1.0f);
            Vector3 up = dir;
            Vector3 right = {}, forward = {};
            MakeBasis(up, right, forward);
            float arc = count > 1 ? 0.13f : 0.0f;
            Vector3 tangent = Vector3Add(Vector3Scale(right, std::cos(angle)), Vector3Scale(forward, std::sin(angle)));
            dir = SafeNormalize(Vector3Add(Vector3Scale(dir, std::cos(arc)), Vector3Scale(tangent, std::sin(arc))), dir);
            seraph.position = SphericalSurfacePoint(dir, config_.seraphHoverAltitude, 0);
        } else {
            Vector3 up = FlatUpForWorld(0);
            Vector3 right = {}, forward = {};
            MakeBasis(up, right, forward);
            float radius = count > 1 ? std::min(config_.seraphWanderRadius * 0.45f, config_.seraphSeparationRadius * 0.8f) : 0.0f;
            Vector3 offset = Vector3Add(Vector3Scale(right, std::cos(angle) * radius), Vector3Scale(forward, std::sin(angle) * radius));
            seraph.position = Vector3Add(Vector3{0.0f, FlatGroundYForWorld(0), 0.0f}, Vector3Add(offset, Vector3Scale(up, config_.seraphHoverAltitude)));
        }
        seraph.wanderTarget = seraph.position;
        seraphs_.push_back(seraph);
    }
    if (!seraphs_.empty()) {
        PlaySfxAt(sfxBossSpawn_, seraphs_.front().position, 150.0f, 1.0f);
        SpawnShockwave(seraphs_.front().position, 8.0f, Color{255, 230, 150, 255});
        for (const SeraphBoss& seraph : seraphs_) {
            SpawnHitBurst(seraph.position, Color{255, 240, 170, 255}, 48);
        }
    }
}

void Game::SpawnEdenApocalypseSeraphs() {
    seraphs_.erase(std::remove_if(seraphs_.begin(), seraphs_.end(),
        [](const SeraphBoss& seraph) { return seraph.edenApocalypse; }),
        seraphs_.end());

    int count = std::clamp(config_.edenApocalypseSeraphCount, 0, 8);
    if (count <= 0) return;

    Vector3 up{0.0f, 1.0f, 0.0f};
    Vector3 right{}, forward{};
    MakeBasis(up, right, forward);
    float baseAngle = RandomFloat(0.0f, 2.0f * PI);
    for (int i = 0; i < count; ++i) {
        float angle = baseAngle + (static_cast<float>(i) / static_cast<float>(count)) * 2.0f * PI;
        float radius = RandomFloat(28.0f, 52.0f);
        Vector3 offset = Vector3Add(Vector3Scale(right, std::cos(angle) * radius), Vector3Scale(forward, std::sin(angle) * radius));
        Vector3 position = Vector3Add(camera_.position, offset);
        float limit = std::max(8.0f, config_.edenPlayRadius * 0.86f);
        float mapR = DistanceXZ(position, Vector3Zero());
        if (mapR > limit && mapR > 0.001f) {
            float scale = limit / mapR;
            position.x *= scale;
            position.z *= scale;
        }
        position.y = EdenGroundYAt(position) + config_.seraphHoverAltitude + RandomFloat(8.0f, 18.0f);

        SeraphBoss seraph;
        seraph.active = true;
        seraph.edenApocalypse = true;
        seraph.health = 100.0f;
        seraph.maxHealth = 100.0f;
        seraph.attackTimer = 1.0f + static_cast<float>(i) * 0.45f;
        seraph.wanderTimer = RandomFloat(0.4f, 1.3f);
        seraph.wingTimer = RandomFloat(0.0f, 6.28f);
        seraph.world = 0;
        seraph.position = position;
        seraph.wanderTarget = position;
        seraphs_.push_back(seraph);
    }

    if (!seraphs_.empty()) {
        PlaySfxAt(sfxBossSpawn_, camera_.position, 150.0f, 0.9f);
        SpawnShockwave(camera_.position, 12.0f, Color{255, 115, 70, 255});
        eventText_ = "EDEN JUDGMENT";
        eventTextTimer_ = 2.8f;
    }
}

void Game::UpdateSeraph(float dt) {
    for (size_t i = 0; i < seraphs_.size();) {
        SeraphBoss& seraph = seraphs_[i];
        if (!seraph.active) {
            seraphs_[i] = seraphs_.back();
            seraphs_.pop_back();
            continue;
        }

        seraph.hitFlash = std::max(0.0f, seraph.hitFlash - dt * 4.5f);
        seraph.attackFlash = std::max(0.0f, seraph.attackFlash - dt * 2.4f);
        seraph.wingTimer += dt * 3.4f;

        Vector3 up = UpForWorldAt(seraph.position, seraph.world);
        Vector3 right = {}, forward = {};
        MakeBasis(up, right, forward);
        seraph.wanderTimer -= dt;
        if (seraph.wanderTimer <= 0.0f || Vector3Distance(seraph.position, seraph.wanderTarget) < 2.5f) {
            seraph.wanderTimer = RandomFloat(2.2f, 4.6f);
            if (IsSphericalMap()) {
                Vector3 baseDir = SafeNormalize(seraph.position, Vector3{0.0f, 1.0f, 0.0f});
                float a = RandomFloat(0.0f, 2.0f * PI);
                float arc = RandomFloat(0.08f, 0.32f);
                Vector3 tangent = Vector3Add(Vector3Scale(right, std::cos(a)), Vector3Scale(forward, std::sin(a)));
                Vector3 dir = SafeNormalize(Vector3Add(Vector3Scale(baseDir, std::cos(arc)), Vector3Scale(tangent, std::sin(arc))), baseDir);
                seraph.wanderTarget = SphericalSurfacePoint(dir, config_.seraphHoverAltitude, seraph.world);
            } else {
                float a = RandomFloat(0.0f, 2.0f * PI);
                float r = RandomFloat(0.0f, config_.seraphWanderRadius);
                Vector3 flat = Vector3Add(Vector3Scale(right, std::cos(a) * r), Vector3Scale(forward, std::sin(a) * r));
                if (IsEdenMap()) {
                    Vector3 center = camera_.position;
                    center.y = 0.0f;
                    seraph.wanderTarget = Vector3Add(Vector3Add(center, flat), Vector3Scale(up, config_.seraphHoverAltitude));
                    float limit = std::max(8.0f, config_.edenPlayRadius * 0.9f);
                    float mapR = DistanceXZ(seraph.wanderTarget, Vector3Zero());
                    if (mapR > limit && mapR > 0.001f) {
                        float scale = limit / mapR;
                        seraph.wanderTarget.x *= scale;
                        seraph.wanderTarget.z *= scale;
                    }
                    seraph.wanderTarget.y = EdenGroundYAt(seraph.wanderTarget) + config_.seraphHoverAltitude + 10.0f;
                } else {
                    seraph.wanderTarget = Vector3Add(Vector3{0.0f, FlatGroundYForWorld(seraph.world), 0.0f}, Vector3Add(flat, Vector3Scale(up, config_.seraphHoverAltitude)));
                }
            }
        }

        Vector3 toTarget = Vector3Subtract(seraph.wanderTarget, seraph.position);
        Vector3 desired = Vector3Length(toTarget) > 0.001f ? Vector3Scale(Vector3Normalize(toTarget), config_.seraphMoveSpeed) : Vector3Zero();
        if (config_.seraphSeparationRadius > 0.001f && config_.seraphSeparationForce > 0.001f) {
            Vector3 separate = {};
            for (size_t j = 0; j < seraphs_.size(); ++j) {
                if (i == j || !seraphs_[j].active) continue;
                Vector3 away = Vector3Subtract(seraph.position, seraphs_[j].position);
                float dist = Vector3Length(away);
                if (dist <= 0.001f || dist >= config_.seraphSeparationRadius) continue;
                float strength = 1.0f - dist / config_.seraphSeparationRadius;
                separate = Vector3Add(separate, Vector3Scale(away, strength / dist));
            }
            if (Vector3Length(separate) > 0.001f) {
                desired = Vector3Add(desired, Vector3Scale(Vector3Normalize(separate), config_.seraphSeparationForce));
            }
        }
        float blend = std::min(1.0f, dt * 1.8f);
        seraph.velocity = Vector3Add(Vector3Scale(seraph.velocity, 1.0f - blend), Vector3Scale(desired, blend));
        seraph.position = Vector3Add(seraph.position, Vector3Scale(seraph.velocity, dt));
        if (IsSphericalMap()) {
            seraph.position = SphericalSurfacePoint(seraph.position, config_.seraphHoverAltitude, seraph.world);
        } else if (IsEdenMap()) {
            seraph.position.y = EdenGroundYAt(seraph.position) + config_.seraphHoverAltitude + 10.0f;
        } else {
            seraph.position.y = FlatGroundYForWorld(seraph.world) + config_.seraphHoverAltitude;
        }

        seraph.attackTimer -= dt;
        if (seraph.attackTimer <= 0.0f) {
            FireSeraphBurst(seraph);
            seraph.attackTimer = config_.seraphAttackInterval + RandomFloat(0.0f, std::max(0.0f, config_.seraphAttackStagger));
        }
        ++i;
    }
}

void Game::FireSeraphBurst(SeraphBoss& seraph) {
    if (!seraph.active) return;
    Vector3 up = UpForWorldAt(seraph.position, seraph.world);
    Vector3 target = playerWorld_ == seraph.world ? camera_.position : Vector3Subtract(seraph.position, Vector3Scale(up, 12.0f));
    Vector3 aim = SafeNormalize(Vector3Subtract(target, seraph.position), Vector3Scale(up, -1.0f));
    Vector3 right = {}, aimUp = {};
    MakeBasis(aim, right, aimUp);
    Vector3 origin = Vector3Add(seraph.position, Vector3Scale(aim, 2.2f));

    int count = std::clamp(config_.seraphFireballCount, 1, 80);
    float spread = config_.seraphFireballSpread;
    for (int i = 0; i < count; ++i) {
        float angle = RandomFloat(0.0f, 2.0f * PI);
        float cone = std::sqrt(RandomFloat(0.0f, 1.0f)) * spread;
        Vector3 lateral = Vector3Add(Vector3Scale(right, std::cos(angle) * cone), Vector3Scale(aimUp, std::sin(angle) * cone));
        Vector3 dir = SafeNormalize(Vector3Add(aim, lateral), aim);
        float speed = config_.seraphFireballSpeed * RandomFloat(0.86f, 1.12f);
        SeraphFireball fireball;
        fireball.position = origin;
        fireball.prevPosition = origin;
        fireball.velocity = Vector3Scale(dir, speed);
        fireball.flightDirection = dir;
        fireball.tipDirection = dir;
        fireball.visualSide = SafeNormalize(Vector3Add(Vector3Scale(right, std::cos(angle)), Vector3Scale(aimUp, std::sin(angle))), right);
        fireball.life = config_.seraphFireballLifetime * RandomFloat(0.85f, 1.1f);
        fireball.maxLife = fireball.life;
        fireball.radius = config_.seraphFireballRadius;
        fireball.damage = config_.seraphFireballDamage;
        fireball.world = seraph.world;
        seraphFireballs_.push_back(fireball);
    }
    seraph.attackFlash = 1.0f;
    PlaySfxAt(sfxSeraphFireBurst_, seraph.position, 130.0f, 0.92f);
    SpawnHitBurst(origin, Color{255, 235, 140, 255}, 18);
}

void Game::UpdateSeraphFireballs(float dt) {
    for (size_t i = 0; i < seraphFireballs_.size();) {
        SeraphFireball& fireball = seraphFireballs_[i];
        fireball.life -= dt;
        fireball.prevPosition = fireball.position;

        Vector3 up = UpForWorldAt(fireball.position, fireball.world);
        fireball.velocity = Vector3Subtract(fireball.velocity, Vector3Scale(up, config_.gravity * 0.18f * dt));
        fireball.position = Vector3Add(fireball.position, Vector3Scale(fireball.velocity, dt));

        bool explode = fireball.life <= 0.0f;
        bool makeFireLayer = false;
        bool hitPlayer = false;
        if (!explode && fireball.world == playerWorld_ && EnemyTouchesPlayer(fireball.position, fireball.radius)) {
            ApplyPlayerHit(camera_.position, Color{255, 220, 130, 255}, "HOLY FIRE");
            hitPlayer = true;
            explode = true;
        }

        if (!explode) {
            if (IsSphericalMap()) {
                float alt = SphericalAltitudeAt(fireball.position, fireball.world);
                if (alt <= fireball.radius * 0.45f) {
                    explode = true;
                    makeFireLayer = true;
                }
            } else if (IsEdenMap()) {
                float ground = EdenGroundYAt(fireball.position);
                if (fireball.position.y <= ground + fireball.radius * 0.45f) {
                    explode = true;
                    makeFireLayer = true;
                }
            } else {
                float ground = FlatGroundYForWorld(fireball.world);
                if (Vector3DotProduct(Vector3Subtract(fireball.position, Vector3{0.0f, ground, 0.0f}), FlatUpForWorld(fireball.world)) <= fireball.radius * 0.45f) {
                    explode = true;
                    makeFireLayer = true;
                }
            }
        }

        if (!explode) {
            if (IsSquareMap()) {
                float limit = squareHalfExtent_ - 0.25f;
                if (std::abs(fireball.position.x) > limit || std::abs(fireball.position.z) > limit) explode = true;
            } else if (IsEdenMap()) {
                if (DistanceXZ(fireball.position, Vector3Zero()) > config_.edenMapRadius + 8.0f) explode = true;
            } else if (!IsSphericalMap()) {
                Vector3 flat = {fireball.position.x, 0.0f, fireball.position.z};
                if (Vector3Length(flat) > arenaRadius_ + 0.25f) explode = true;
            }
        }

        if (explode) {
            bool warFire = fireball.warFire;
            ExplodeSeraphFireball(fireball.position, fireball.world, hitPlayer, makeFireLayer);
            if (warFire && makeFireLayer && !firePatches_.empty()) {
                FirePatch& patch = firePatches_.back();
                patch.radius = config_.warRiderChargeFirePatchRadius;
                patch.life = config_.warRiderChargeFirePatchDuration;
                patch.maxLife = patch.life;
                patch.damagePerSecond = std::max(config_.seraphFireLayerDps, config_.heatwaveFirePatchDamage * 0.45f);
                patch.outerColor = Color{255, 70, 24, 255};
                patch.innerColor = Color{255, 190, 72, 255};
                patch.particleColor = Color{255, 110, 30, 220};
            }
            seraphFireballs_[i] = seraphFireballs_.back();
            seraphFireballs_.pop_back();
            continue;
        }

        if (RandomFloat(0.0f, 1.0f) < 0.7f) {
            particles_.push_back(Particle{
                fireball.position,
                Vector3Scale(up, RandomFloat(0.6f, 1.8f)),
                Color{255, 220, 115, 190},
                RandomFloat(0.12f, 0.26f), RandomFloat(0.12f, 0.26f),
                RandomFloat(0.04f, 0.09f)
            });
        }
        ++i;
    }
}

void Game::ExplodeSeraphFireball(Vector3 position, int world, bool hitPlayer, bool makeFireLayer) {
    Vector3 up = UpForWorldAt(position, world);
    if (makeFireLayer) {
        if (IsSphericalMap()) {
            position = SphericalSurfacePoint(position, 0.02f, world);
            up = SphericalUpAt(position, world);
        } else if (IsEdenMap()) {
            position.y = EdenGroundYAt(position);
            up = Vector3{0.0f, 1.0f, 0.0f};
        } else {
            position.y = FlatGroundYForWorld(world);
            up = FlatUpForWorld(world);
        }
    }
    if (!hitPlayer && world == playerWorld_ && Vector3Distance(camera_.position, position) <= config_.seraphFireballRadius + playerRadius_ + 0.45f) {
        ApplyPlayerHit(camera_.position, Color{255, 220, 130, 255}, "HOLY FIRE");
    }
    if (makeFireLayer) {
        firePatches_.push_back(FirePatch{
            position, up,
            config_.seraphFireLayerDuration, config_.seraphFireLayerDuration,
            config_.seraphFireLayerRadius, config_.seraphFireLayerDps,
            world, JPH::BodyID(),
            true, true,
            Color{255, 185, 70, 255},
            Color{255, 245, 185, 255},
            Color{255, 220, 130, 210}
        });
    }
    PlaySfxAt(sfxNapalmExplosion_, position, 76.0f, 0.62f);
    SpawnShockwave(position, (makeFireLayer ? config_.seraphFireLayerRadius * 0.9f : 1.5f), Color{255, 220, 120, 255});
    SpawnHitBurst(position, Color{255, 235, 150, 255}, 28);
}

void Game::DamageSeraph(float damage, Vector3 hitPosition, Color color) {
    if (seraphs_.empty() || damage <= 0.0f) return;
    SeraphBoss* target = nullptr;
    float bestDist = std::numeric_limits<float>::max();
    for (SeraphBoss& seraph : seraphs_) {
        if (!seraph.active) continue;
        float dist = Vector3Distance(hitPosition, seraph.position);
        if (dist < bestDist) {
            bestDist = dist;
            target = &seraph;
        }
    }
    if (!target) return;
    target->health -= damage;
    totalDamageDealt_ += damage;
    target->hitFlash = 1.0f;
    PlayEnemyHitSfx(target->position);
    SpawnHitBurst(hitPosition, color, 12);
    if (target->health <= 0.0f) {
        target->active = false;
        target->defeated = true;
        if (target->edenApocalypse) {
            SpawnShockwave(target->position, 8.0f, Color{255, 150, 80, 255});
            SpawnHitBurst(target->position, Color{255, 205, 120, 255}, 48);
            PlaySfxAt(sfxBossDeath_, target->position, 110.0f, 0.55f);
            score_ += 180;
        } else {
            SpawnShockwave(target->position, 18.0f, Color{255, 230, 135, 255});
            SpawnHitBurst(target->position, Color{255, 245, 180, 255}, 120);
            PlaySfxAt(sfxBossDeath_, target->position, 150.0f, 1.0f);
            eventText_ = "SERAPH EXTINGUISHED";
            eventTextTimer_ = 4.0f;
            cameraShake_ = 1.0f;
            score_ += 1200;
        }
    }
}

void Game::DamageSeraphInRadius(Vector3 position, float radius, float damage, Color color) {
    if (seraphs_.empty() || damage <= 0.0f) return;
    for (SeraphBoss& seraph : seraphs_) {
        if (!seraph.active) continue;
        float distance = Vector3Distance(position, seraph.position);
        float hitRadius = radius + 3.6f;
        if (distance > hitRadius) continue;
        float falloff = 1.0f - std::clamp(distance / std::max(0.001f, hitRadius), 0.0f, 1.0f);
        DamageSeraph(damage * (0.35f + falloff * 0.65f), seraph.position, color);
    }
}

void Game::SpawnWarRider() {
    warRider_ = {};
    warRider_.active = true;
    warRider_.health = config_.warRiderHealth;
    warRider_.maxHealth = config_.warRiderHealth;
    warRider_.chargeTimer = config_.warRiderChargeInterval * 0.55f;
    warRider_.slashTimer = config_.warRiderSlashInterval * 0.35f;
    warRider_.commandTimer = config_.warRiderCommandInterval * 0.45f;
    warRider_.world = 0;

    if (IsSphericalMap()) {
        Vector3 up = SphericalUpAt(camera_.position, 0);
        Vector3 right = {}, forward = {};
        MakeBasis(up, right, forward);
        Vector3 dir = SafeNormalize(Vector3Add(up, Vector3Scale(forward, 0.38f)), up);
        if (IsHollowWorldMap()) dir = Vector3Scale(dir, -1.0f);
        warRider_.position = SphericalSurfacePoint(dir, config_.warRiderHoverAltitude, 0);
        Vector3 toPlayer = ProjectOnSphericalTangent(Vector3Subtract(camera_.position, warRider_.position), SphericalUpAt(warRider_.position, 0));
        warRider_.forward = SafeNormalize(toPlayer, forward);
    } else if (IsSquareMap()) {
        float angle = RandomFloat(0.0f, 2.0f * PI);
        warRider_.orbitAngle = angle;
        float radius = squareHalfExtent_ * config_.warRiderOrbitRadiusFactor;
        warRider_.position = Vector3{std::cos(angle) * radius, FlatGroundYForWorld(0) + config_.warRiderHoverAltitude, std::sin(angle) * radius};
        warRider_.forward = SafeNormalize(Vector3Subtract(camera_.position, warRider_.position), Vector3{0.0f, 0.0f, 1.0f});
        warRider_.forward.y = 0.0f;
        warRider_.forward = SafeNormalize(warRider_.forward, Vector3{0.0f, 0.0f, 1.0f});
    } else {
        float angle = RandomFloat(0.0f, 2.0f * PI);
        warRider_.orbitAngle = angle;
        float radius = arenaRadius_ * config_.warRiderOrbitRadiusFactor;
        warRider_.position = Vector3{std::cos(angle) * radius, FlatGroundYForWorld(0) + config_.warRiderHoverAltitude, std::sin(angle) * radius};
        warRider_.forward = SafeNormalize(Vector3Subtract(camera_.position, warRider_.position), Vector3{0.0f, 0.0f, 1.0f});
        warRider_.forward.y = 0.0f;
        warRider_.forward = SafeNormalize(warRider_.forward, Vector3{0.0f, 0.0f, 1.0f});
    }

    PlaySfxAt(sfxWarRiderSpawn_, warRider_.position, 170.0f, 1.0f);
    SpawnShockwave(warRider_.position, 9.0f, Color{255, 70, 45, 255});
    SpawnHitBurst(warRider_.position, Color{255, 88, 48, 255}, 90);
}

void Game::CommandWarRiderMinions() {
    if (!warRider_.active) return;
    int affected = 0;
    Vector3 origin = warRider_.position;
    Vector3 up = UpForWorldAt(origin, warRider_.world);
    for (Enemy& enemy : enemies_) {
        if (enemy.world != warRider_.world) continue;
        if (enemy.type == EnemyType::Duelist || enemy.type == EnemyType::Dummy || enemy.type == EnemyType::DummyBoss) continue;
        Vector3 position = BodyPosition(enemy.body);
        enemy.warEnrageTimer = std::max(enemy.warEnrageTimer, config_.warRiderGlobalEnrageDuration);
        float distance = Vector3Distance(position, origin);
        if (distance > config_.warRiderCommandRadius) {
            continue;
        }
        EmpowerWarMinion(enemy, position);
        Vector3 push = IsSphericalMap()
            ? ProjectOnSphericalTangent(Vector3Subtract(camera_.position, position), UpForWorldAt(position, enemy.world))
            : Vector3{camera_.position.x - position.x, 0.0f, camera_.position.z - position.z};
        push = SafeNormalize(push, warRider_.forward);
        enemy.externalVelocity = Vector3Add(enemy.externalVelocity, Vector3Scale(push, 2.4f));
        ++affected;
    }
    SpawnShockwave(Vector3Add(origin, Vector3Scale(up, -0.8f)), config_.warRiderCommandRadius, Color{255, 55, 30, 255});
    SpawnHitBurst(origin, Color{255, 76, 34, 255}, 28 + affected * 4);
    cameraShake_ = std::min(1.0f, cameraShake_ + 0.16f);
    PlaySfxAt(sfxWarRiderCommand_, origin, 190.0f, 1.0f);
}

void Game::EmpowerWarMinion(Enemy& enemy, Vector3 position) {
    enemy.warCommandTimer = std::max(enemy.warCommandTimer, config_.warRiderCommandDuration);
    if (enemy.warGrowthStacks >= config_.warRiderCommandMaxGrowthStacks) {
        return;
    }

    float healthMult = config_.warRiderCommandGrowthMult;
    float sizeMult = config_.warRiderCommandSizeGrowthMult;
    enemy.warGrowthStacks += 1;
    enemy.radius *= sizeMult;
    enemy.maxHealth *= healthMult;
    enemy.health = std::max(enemy.health * healthMult, enemy.maxHealth * 0.5f);
    enemy.health = std::min(enemy.health, enemy.maxHealth);
    enemy.color = Color{
        static_cast<unsigned char>(std::min(255, static_cast<int>(enemy.color.r) + 32)),
        static_cast<unsigned char>(std::max(20, static_cast<int>(enemy.color.g) - 28)),
        static_cast<unsigned char>(std::max(20, static_cast<int>(enemy.color.b) - 28)),
        enemy.color.a
    };
    SpawnHitBurst(position, Color{255, 65, 34, 255}, 18);
}

void Game::FireWarRiderSlash() {
    if (!warRider_.active) return;
    Vector3 up = UpForWorldAt(warRider_.position, warRider_.world);
    Vector3 forward = warRider_.forward;
    if (IsSphericalMap()) {
        forward = SafeNormalize(ProjectOnSphericalTangent(forward, up), PlayerForward());
    } else {
        forward.y = 0.0f;
        forward = SafeNormalize(forward, Vector3{0.0f, 0.0f, 1.0f});
    }
    Vector3 origin = Vector3Add(warRider_.position, Vector3Add(Vector3Scale(up, 1.8f), Vector3Scale(forward, 2.2f)));
    Vector3 toPlayer = Vector3Subtract(camera_.position, origin);
    forward = toPlayer;
    forward = SafeNormalize(forward, warRider_.forward);
    Vector3 facingForward = IsSphericalMap()
        ? ProjectOnSphericalTangent(forward, up)
        : Vector3{forward.x, 0.0f, forward.z};
    warRider_.forward = SafeNormalize(facingForward, warRider_.forward);
    origin = Vector3Add(warRider_.position, Vector3Add(Vector3Scale(up, 1.8f), Vector3Scale(warRider_.forward, 2.2f)));
    Vector3 right = SafeNormalize(Vector3CrossProduct(up, forward), Vector3{1.0f, 0.0f, 0.0f});
    Vector3 bladeUp = SafeNormalize(Vector3CrossProduct(forward, right), up);
    edenFireSlashes_.push_back(EdenFireSlash{
        origin,
        right,
        forward,
        bladeUp,
        Vector3Scale(forward, config_.warRiderSlashSpeed),
        3.2f,
        3.2f,
        config_.warRiderSlashRadius,
        std::max(0.7f, config_.warRiderSlashRadius * 0.22f),
        1.25f
    });
    PlaySfxAt(sfxWarRiderSlash_, origin, 190.0f, 0.98f);
    SpawnHitBurst(origin, Color{255, 60, 38, 255}, 18);
}

void Game::UpdateWarRider(float dt) {
    if (!warRider_.active) return;
    if (IsEdenMap() || warRider_.world != playerWorld_) return;

    warRider_.gallopTimer += dt;
    warRider_.hitFlash = std::max(0.0f, warRider_.hitFlash - dt * 4.2f);
    warRider_.contactCooldown = std::max(0.0f, warRider_.contactCooldown - dt);
    warRider_.chargeTimer -= dt;
    warRider_.slashTimer -= dt;
    warRider_.commandTimer -= dt;
    warRider_.chargeFireballTimer = std::max(0.0f, warRider_.chargeFireballTimer - dt);

    for (Enemy& enemy : enemies_) {
        if (enemy.world != warRider_.world) continue;
        if (enemy.type == EnemyType::Duelist || enemy.type == EnemyType::Dummy || enemy.type == EnemyType::DummyBoss) continue;
        enemy.warEnrageTimer = std::max(enemy.warEnrageTimer, config_.warRiderGlobalEnrageDuration);
    }

    Vector3 up = UpForWorldAt(warRider_.position, warRider_.world);
    Vector3 toPlayer = Vector3Subtract(camera_.position, warRider_.position);
    Vector3 desired = IsSphericalMap()
        ? ProjectOnSphericalTangent(toPlayer, up)
        : Vector3{toPlayer.x, 0.0f, toPlayer.z};
    desired = SafeNormalize(desired, warRider_.forward);

    if (warRider_.chargeTimeLeft <= 0.0f) {
        float turn = std::clamp(dt * 1.8f, 0.0f, 1.0f);
        warRider_.forward = SafeNormalize(Vector3Add(Vector3Scale(warRider_.forward, 1.0f - turn), Vector3Scale(desired, turn)), desired);
    }

    if (warRider_.chargeTimeLeft <= 0.0f && warRider_.chargeTimer <= 0.0f) {
        Vector3 playerUp = UpForWorldAt(camera_.position, warRider_.world);
        warRider_.chargeTarget = Vector3Add(camera_.position, Vector3Scale(playerUp, config_.warRiderOverheadChargeAltitude));
        Vector3 chargeDir = IsSphericalMap()
            ? ProjectOnSphericalTangent(Vector3Subtract(warRider_.chargeTarget, warRider_.position), up)
            : Vector3{camera_.position.x - warRider_.position.x, 0.0f, camera_.position.z - warRider_.position.z};
        warRider_.chargeDirection = SafeNormalize(chargeDir, warRider_.forward);
        warRider_.forward = warRider_.chargeDirection;
        float mapRadius = IsSquareMap() ? squareHalfExtent_ : IsSphericalMap() ? SphericalRadius() : arenaRadius_;
        float orbitRadius = IsSphericalMap()
            ? mapRadius
            : std::max(2.0f, mapRadius * config_.warRiderOrbitRadiusFactor);
        warRider_.chargeDistanceLeft = std::max(config_.warRiderChargeSpeed * config_.warRiderChargeDuration, orbitRadius * 2.25f);
        warRider_.chargeTimeLeft = std::max(config_.warRiderChargeDuration, warRider_.chargeDistanceLeft / std::max(0.001f, config_.warRiderChargeSpeed));
        warRider_.chargeTimer = config_.warRiderChargeInterval * RandomFloat(0.9f, 1.2f);
        warRider_.chargeFireballTimer = 0.0f;
        SpawnHitBurst(warRider_.position, Color{255, 70, 34, 255}, 20);
    }

    if (warRider_.chargeTimeLeft > 0.0f) {
        warRider_.chargeTimeLeft = std::max(0.0f, warRider_.chargeTimeLeft - dt);
        float stepDistance = std::min(warRider_.chargeDistanceLeft, config_.warRiderChargeSpeed * dt);
        if (warRider_.chargeDistanceLeft <= 0.0f || stepDistance <= 0.0f) {
            warRider_.chargeTimeLeft = 0.0f;
            warRider_.chargeDistanceLeft = 0.0f;
        }
        Vector3 chargeDir = SafeNormalize(warRider_.chargeDirection, warRider_.forward);
        warRider_.forward = chargeDir;
        warRider_.position = Vector3Add(warRider_.position, Vector3Scale(chargeDir, stepDistance));
        warRider_.chargeDistanceLeft = std::max(0.0f, warRider_.chargeDistanceLeft - stepDistance);
        if (IsSphericalMap()) {
            float targetAlt = config_.warRiderHoverAltitude + config_.warRiderOverheadChargeAltitude;
            warRider_.position = SphericalSurfacePoint(warRider_.position, targetAlt, warRider_.world);
        } else {
            float minY = FlatGroundYForWorld(warRider_.world) + config_.warRiderHoverAltitude;
            float maxY = FlatGroundYForWorld(warRider_.world) + config_.warRiderHoverAltitude + config_.warRiderOverheadChargeAltitude + 4.0f;
            warRider_.position.y = std::clamp(warRider_.position.y, minY, maxY);
            float orbitRadius = std::max(2.0f, (IsSquareMap() ? squareHalfExtent_ : arenaRadius_) * config_.warRiderOrbitRadiusFactor);
            Vector3 flat{warRider_.position.x, 0.0f, warRider_.position.z};
            float dist = Vector3Length(flat);
            if (dist > orbitRadius + 0.15f) {
                Vector3 radial = SafeNormalize(flat, Vector3{1.0f, 0.0f, 0.0f});
                warRider_.position.x = radial.x * orbitRadius;
                warRider_.position.z = radial.z * orbitRadius;
                warRider_.position.y = FlatGroundYForWorld(warRider_.world) + config_.warRiderHoverAltitude;
                warRider_.orbitAngle = std::atan2(radial.z, radial.x);
                warRider_.chargeTimeLeft = 0.0f;
                warRider_.chargeDistanceLeft = 0.0f;
                Vector3 tangent{-std::sin(warRider_.orbitAngle), 0.0f, std::cos(warRider_.orbitAngle)};
                warRider_.forward = SafeNormalize(tangent, desired);
            }
        }
        if (warRider_.chargeFireballTimer <= 0.0f) {
            Vector3 dropUp = UpForWorldAt(warRider_.position, warRider_.world);
            Vector3 side = SafeNormalize(Vector3CrossProduct(dropUp, warRider_.forward), Vector3{1.0f, 0.0f, 0.0f});
            SeraphFireball fireball;
            fireball.position = Vector3Add(warRider_.position, Vector3Add(Vector3Scale(dropUp, -1.2f), Vector3Scale(side, RandomFloat(-1.2f, 1.2f))));
            fireball.prevPosition = fireball.position;
            fireball.flightDirection = Vector3Scale(dropUp, -1.0f);
            fireball.tipDirection = fireball.flightDirection;
            fireball.visualSide = side;
            fireball.velocity = Vector3Scale(fireball.flightDirection, config_.warRiderChargeFireballSpeed);
            fireball.life = 4.0f;
            fireball.maxLife = 4.0f;
            fireball.radius = 0.36f;
            fireball.damage = config_.warRiderSlashDamage;
            fireball.world = warRider_.world;
            fireball.warFire = true;
            fireball.sodomFire = true;
            seraphFireballs_.push_back(fireball);
            warRider_.chargeFireballTimer = config_.warRiderChargeFireballInterval;
            PlaySfxAt(sfxFlamethrowerFireball_, fireball.position, 120.0f, 0.76f);
        }
    } else if (IsSphericalMap()) {
        Vector3 tangent = SafeNormalize(ProjectOnSphericalTangent(warRider_.forward, up), PlayerForward());
        warRider_.position = Vector3Add(warRider_.position, Vector3Scale(tangent, config_.warRiderMoveSpeed * dt));
        warRider_.position = SphericalSurfacePoint(warRider_.position, config_.warRiderHoverAltitude, warRider_.world);
        up = SphericalUpAt(warRider_.position, warRider_.world);
        warRider_.forward = SafeNormalize(ProjectOnSphericalTangent(tangent, up), desired);
    } else {
        float baseRadius = IsSquareMap() ? squareHalfExtent_ : arenaRadius_;
        float orbitRadius = std::max(2.0f, baseRadius * config_.warRiderOrbitRadiusFactor);
        warRider_.orbitAngle += dt * config_.warRiderMoveSpeed / orbitRadius;
        warRider_.position = Vector3{
            std::cos(warRider_.orbitAngle) * orbitRadius,
            FlatGroundYForWorld(warRider_.world) + config_.warRiderHoverAltitude,
            std::sin(warRider_.orbitAngle) * orbitRadius
        };
        warRider_.position.y = FlatGroundYForWorld(warRider_.world) + config_.warRiderHoverAltitude;
        Vector3 face = Vector3{camera_.position.x - warRider_.position.x, 0.0f, camera_.position.z - warRider_.position.z};
        warRider_.forward = SafeNormalize(face, Vector3{-std::sin(warRider_.orbitAngle), 0.0f, std::cos(warRider_.orbitAngle)});
    }
    warRider_.velocity = Vector3Scale(warRider_.forward, warRider_.chargeTimeLeft > 0.0f ? config_.warRiderChargeSpeed : config_.warRiderMoveSpeed);

    if (warRider_.slashTimer <= 0.0f) {
        FireWarRiderSlash();
        warRider_.slashTimer = config_.warRiderSlashInterval * RandomFloat(0.85f, 1.18f);
    }

    if (warRider_.commandTimer <= 0.0f) {
        CommandWarRiderMinions();
        warRider_.commandTimer = config_.warRiderCommandInterval * RandomFloat(0.86f, 1.16f);
    }

    if (RandomFloat(0.0f, 1.0f) < 0.55f) {
        particles_.push_back(Particle{
            Vector3Subtract(warRider_.position, Vector3Scale(warRider_.forward, 1.4f)),
            Vector3{RandomFloat(-1.0f, 1.0f), RandomFloat(0.2f, 1.4f), RandomFloat(-1.0f, 1.0f)},
            Color{255, static_cast<unsigned char>(RandomFloat(50, 140)), 35, 200},
            RandomFloat(0.16f, 0.32f), RandomFloat(0.16f, 0.32f), RandomFloat(0.05f, 0.11f)
        });
    }
}

void Game::DamageWarRider(float damage, Vector3 hitPosition, Color color) {
    if (!warRider_.active || damage <= 0.0f) return;
    warRider_.health -= damage;
    totalDamageDealt_ += damage;
    warRider_.hitFlash = 1.0f;
    PlayEnemyHitSfx(hitPosition);
    SpawnHitBurst(hitPosition, color, 12);
    if (warRider_.health <= 0.0f) {
        warRider_.active = false;
        warRider_.defeated = true;
        score_ += 1400;
        PlaySfxAt(sfxBossDeath_, warRider_.position, 150.0f, 1.0f);
        SpawnShockwave(warRider_.position, 16.0f, Color{255, 62, 38, 255});
        SpawnHitBurst(warRider_.position, Color{255, 120, 70, 255}, 130);
        eventText_ = "WAR FALLS";
        eventTextTimer_ = 4.0f;
        cameraShake_ = 1.0f;
    }
}

void Game::DamageWarRiderInRadius(Vector3 position, float radius, float damage, Color color) {
    if (!warRider_.active || damage <= 0.0f) return;
    float distance = Vector3Distance(position, warRider_.position);
    float hitRadius = radius + kWarRiderHitPadding;
    if (distance > hitRadius) return;
    float falloff = 1.0f - std::clamp(distance / std::max(0.001f, hitRadius), 0.0f, 1.0f);
    DamageWarRider(damage * (0.35f + falloff * 0.65f), warRider_.position, color);
}

void Game::SpawnConquestRider() {
    conquestRider_ = {};
    conquestRider_.active = true;
    conquestRider_.health = config_.conquestRiderHealth;
    conquestRider_.maxHealth = config_.conquestRiderHealth;
    conquestRider_.arrowTimer = config_.conquestRiderArrowInterval * 0.45f;
    conquestRider_.summonTimer = config_.conquestRiderSummonInterval * 0.65f;
    conquestRider_.world = 0;

    if (IsSphericalMap()) {
        Vector3 up = SphericalUpAt(camera_.position, 0);
        Vector3 right = {}, forward = {};
        MakeBasis(up, right, forward);
        Vector3 dir = SafeNormalize(Vector3Add(up, Vector3Scale(right, 0.42f)), up);
        if (IsHollowWorldMap()) dir = Vector3Scale(dir, -1.0f);
        conquestRider_.position = SphericalSurfacePoint(dir, config_.conquestRiderHoverAltitude, 0);
        Vector3 toPlayer = ProjectOnSphericalTangent(Vector3Subtract(camera_.position, conquestRider_.position), SphericalUpAt(conquestRider_.position, 0));
        conquestRider_.forward = SafeNormalize(toPlayer, forward);
    } else {
        float angle = RandomFloat(0.0f, 2.0f * PI);
        conquestRider_.orbitAngle = angle;
        float baseRadius = IsSquareMap() ? squareHalfExtent_ : arenaRadius_;
        float radius = baseRadius * config_.conquestRiderOrbitRadiusFactor;
        conquestRider_.position = Vector3{std::cos(angle) * radius, FlatGroundYForWorld(0) + config_.conquestRiderHoverAltitude, std::sin(angle) * radius};
        conquestRider_.forward = SafeNormalize(Vector3Subtract(camera_.position, conquestRider_.position), Vector3{0.0f, 0.0f, 1.0f});
        conquestRider_.forward.y = 0.0f;
        conquestRider_.forward = SafeNormalize(conquestRider_.forward, Vector3{0.0f, 0.0f, 1.0f});
    }

    PlaySfxAt(sfxBossSpawn_, conquestRider_.position, 150.0f, 0.95f);
    SpawnShockwave(conquestRider_.position, 8.0f, Color{230, 255, 185, 255});
    SpawnHitBurst(conquestRider_.position, Color{180, 255, 115, 255}, 70);
}

void Game::SummonConquestRiderMinions() {
    if (!conquestRider_.active || config_.conquestRiderSummonCount <= 0) return;
    Vector3 up = UpForWorldAt(conquestRider_.position, conquestRider_.world);
    Vector3 side = SafeNormalize(Vector3CrossProduct(up, conquestRider_.forward), Vector3{1.0f, 0.0f, 0.0f});
    for (int i = 0; i < config_.conquestRiderSummonCount; ++i) {
        EnemyType type = i % 4 == 0 ? EnemyType::Spitter
            : i % 4 == 1 ? EnemyType::Pouncer
            : i % 4 == 2 ? EnemyType::Harrier
            : EnemyType::Skitter;
        SpawnEnemyOfType(type);
        if (!enemies_.empty()) {
            Enemy& enemy = enemies_.back();
            float offset = (static_cast<float>(i) - static_cast<float>(config_.conquestRiderSummonCount - 1) * 0.5f) * 2.2f;
            Vector3 spawn = Vector3Add(conquestRider_.position,
                Vector3Add(Vector3Scale(side, offset), Vector3Scale(conquestRider_.forward, RandomFloat(-4.0f, 3.0f))));
            if (IsSphericalMap()) {
                spawn = SphericalSurfacePoint(spawn, config_.asteroidEnemyAltitude, conquestRider_.world);
            } else {
                spawn.y = FlatGroundYForWorld(conquestRider_.world) + enemy.radius;
            }
            physics_.Bodies().SetPosition(enemy.body, ToJoltVector(spawn), JPH::EActivation::Activate);
            enemy.world = conquestRider_.world;
            enemy.color = Color{170, 235, 105, 255};
            enemy.plagueTimer = std::max(enemy.plagueTimer, config_.conquestRiderPlagueInfectDuration * 0.35f);
            enemy.plagueBurstOnDeath = true;
        }
    }
    SpawnShockwave(conquestRider_.position, 10.0f, Color{165, 235, 92, 255});
    SpawnHitBurst(conquestRider_.position, Color{180, 255, 110, 255}, 36);
    PlaySfxAt(sfxBossBarrage_, conquestRider_.position, 130.0f, 0.7f);
}

void Game::FireConquestRiderArrow() {
    if (!conquestRider_.active) return;
    Vector3 up = UpForWorldAt(conquestRider_.position, conquestRider_.world);
    Vector3 toPlayer = Vector3Subtract(camera_.position, conquestRider_.position);
    Vector3 forward = IsSphericalMap()
        ? ProjectOnSphericalTangent(toPlayer, up)
        : Vector3{toPlayer.x, 0.0f, toPlayer.z};
    forward = SafeNormalize(forward, conquestRider_.forward);
    conquestRider_.forward = forward;

    Vector3 side = SafeNormalize(Vector3CrossProduct(up, forward), Vector3{1.0f, 0.0f, 0.0f});
    Vector3 origin = Vector3Add(conquestRider_.position, Vector3Add(Vector3Scale(up, 2.15f), Vector3Scale(forward, 2.35f)));
    Vector3 aim = SafeNormalize(Vector3Subtract(camera_.position, origin), forward);
    plagueArrows_.push_back(PlagueArrow{
        origin,
        origin,
        Vector3Scale(aim, config_.conquestRiderArrowSpeed),
        aim,
        side,
        config_.conquestRiderArrowLifetime,
        config_.conquestRiderArrowLifetime,
        config_.conquestRiderArrowRadius,
        conquestRider_.world
    });
    PlaySfxAt(sfxSpearThrow_, origin, 130.0f, 0.86f);
    SpawnHitBurst(origin, Color{190, 255, 110, 255}, 12);
}

void Game::UpdateConquestRider(float dt) {
    if (!conquestRider_.active) return;
    if (IsEdenMap() || conquestRider_.world != playerWorld_) return;

    conquestRider_.gallopTimer += dt;
    conquestRider_.hitFlash = std::max(0.0f, conquestRider_.hitFlash - dt * 4.2f);
    Vector3 up = UpForWorldAt(conquestRider_.position, conquestRider_.world);
    Vector3 toPlayer = Vector3Subtract(camera_.position, conquestRider_.position);
    Vector3 tangentToPlayer = IsSphericalMap() ? ProjectOnSphericalTangent(toPlayer, up) : Vector3{toPlayer.x, 0.0f, toPlayer.z};
    Vector3 desiredForward = SafeNormalize(tangentToPlayer, conquestRider_.forward);
    conquestRider_.forward = SafeNormalize(Vector3Lerp(conquestRider_.forward, desiredForward, std::clamp(dt * 2.4f, 0.0f, 1.0f)), desiredForward);

    conquestRider_.orbitAngle += dt * config_.conquestRiderMoveSpeed * 0.035f;
    if (IsSphericalMap()) {
        Vector3 right = SafeNormalize(Vector3CrossProduct(up, conquestRider_.forward), Vector3{1.0f, 0.0f, 0.0f});
        Vector3 drift = Vector3Add(Vector3Scale(conquestRider_.forward, std::sin(conquestRider_.orbitAngle) * 0.45f), Vector3Scale(right, std::cos(conquestRider_.orbitAngle * 0.7f) * 0.65f));
        Vector3 moved = Vector3Add(conquestRider_.position, Vector3Scale(SafeNormalize(drift, conquestRider_.forward), config_.conquestRiderMoveSpeed * dt));
        conquestRider_.position = SphericalSurfacePoint(moved, config_.conquestRiderHoverAltitude, conquestRider_.world);
    } else {
        float baseRadius = IsSquareMap() ? squareHalfExtent_ : arenaRadius_;
        float patrolRadius = baseRadius * config_.conquestRiderOrbitRadiusFactor;
        float x = std::cos(conquestRider_.orbitAngle) * patrolRadius;
        float z = std::sin(conquestRider_.orbitAngle) * patrolRadius;
        Vector3 target = Vector3{x, FlatGroundYForWorld(conquestRider_.world) + config_.conquestRiderHoverAltitude, z};
        conquestRider_.position = Vector3Lerp(conquestRider_.position, target, std::clamp(dt * 0.8f, 0.0f, 1.0f));
    }

    conquestRider_.arrowTimer -= dt;
    if (conquestRider_.arrowTimer <= 0.0f) {
        FireConquestRiderArrow();
        conquestRider_.arrowTimer = config_.conquestRiderArrowInterval;
    }

    conquestRider_.summonTimer -= dt;
    if (conquestRider_.summonTimer <= 0.0f) {
        SummonConquestRiderMinions();
        conquestRider_.summonTimer = config_.conquestRiderSummonInterval;
    }
}

void Game::SpawnPlagueCircle(Vector3 position, int world, float radius, float duration) {
    Vector3 up = UpForWorldAt(position, world);
    if (IsSphericalMap()) {
        position = SphericalSurfacePoint(position, 0.025f, world);
        up = SphericalUpAt(position, world);
    } else {
        position.y = FlatGroundYForWorld(world);
        up = FlatUpForWorld(world);
    }
    firePatches_.push_back(FirePatch{
        position, up,
        duration, duration,
        radius, config_.conquestRiderPlagueDps,
        world, JPH::BodyID(),
        true, false,
        Color{88, 150, 45, 230},
        Color{178, 235, 83, 245},
        Color{165, 255, 100, 205},
        true,
        true
    });
    SpawnShockwave(position, radius * 0.78f, Color{150, 230, 70, 255});
    SpawnHitBurst(position, Color{165, 255, 95, 255}, 28);
}

void Game::ExplodePlagueArrow(Vector3 position, int world, bool makePlagueCircle) {
    if (world == playerWorld_ && Vector3Distance(camera_.position, position) <= config_.conquestRiderArrowRadius + playerRadius_ + 0.6f) {
        ApplyPlayerPlague(camera_.position);
    }
    if (makePlagueCircle) {
        SpawnPlagueCircle(position, world, config_.conquestRiderPlagueRadius, config_.conquestRiderPlagueDuration);
    } else {
        SpawnShockwave(position, 2.0f, Color{145, 220, 80, 255});
        SpawnHitBurst(position, Color{165, 255, 100, 255}, 18);
    }
    PlaySfxAt(sfxMysticCurseOrb_, position, 95.0f, 0.82f);
}

void Game::ApplyPlayerPlague(Vector3 position) {
    bool newlyPlagued = playerPlagueTimer_ <= 0.0f;
    playerPlagueTimer_ = std::max(playerPlagueTimer_, config_.conquestRiderPlagueDotDuration);
    if (newlyPlagued) {
        playerPlagueTickTimer_ = 0.0f;
    }
    ApplyPlayerHit(position, Color{160, 235, 80, 255}, "PLAGUE");
}

void Game::UpdatePlayerPlague(float dt) {
    if (playerPlagueTimer_ <= 0.0f || config_.conquestRiderPlagueDps <= 0.0f) {
        playerPlagueTimer_ = 0.0f;
        playerPlagueTickTimer_ = 0.0f;
        return;
    }
    playerPlagueTimer_ = std::max(0.0f, playerPlagueTimer_ - dt);
    playerPlagueTickTimer_ -= dt;
    if (playerPlagueTickTimer_ <= 0.0f) {
        ApplyPlayerHit(camera_.position, Color{140, 220, 70, 255}, "PLAGUE");
        playerPlagueTickTimer_ = config_.conquestRiderPlagueDotInterval;
    }
}

void Game::UpdatePlagueArrows(float dt) {
    for (size_t i = 0; i < plagueArrows_.size();) {
        PlagueArrow& arrow = plagueArrows_[i];
        arrow.life -= dt;
        arrow.prevPosition = arrow.position;
        arrow.position = Vector3Add(arrow.position, Vector3Scale(arrow.velocity, dt));
        bool explode = arrow.life <= 0.0f;
        bool makePlagueCircle = false;

        if (!explode && arrow.world == playerWorld_ && Vector3Distance(camera_.position, arrow.position) <= arrow.radius + playerRadius_) {
            explode = true;
        }

        if (!explode) {
            if (IsSphericalMap()) {
                float alt = SphericalAltitudeAt(arrow.position, arrow.world);
                if (alt <= arrow.radius * 0.45f) {
                    explode = true;
                    makePlagueCircle = true;
                }
            } else {
                float ground = FlatGroundYForWorld(arrow.world);
                if (arrow.position.y <= ground + arrow.radius * 0.45f) {
                    explode = true;
                    makePlagueCircle = true;
                }
            }
        }

        if (!explode) {
            if (IsSquareMap()) {
                float limit = squareHalfExtent_ - 0.25f;
                if (std::abs(arrow.position.x) > limit || std::abs(arrow.position.z) > limit) explode = true;
            } else if (!IsSphericalMap()) {
                Vector3 flat = {arrow.position.x, 0.0f, arrow.position.z};
                if (Vector3Length(flat) > arenaRadius_ + 0.25f) explode = true;
            }
        }

        if (explode) {
            ExplodePlagueArrow(arrow.position, arrow.world, makePlagueCircle);
            plagueArrows_[i] = plagueArrows_.back();
            plagueArrows_.pop_back();
            continue;
        }

        if (RandomFloat(0.0f, 1.0f) < 0.55f) {
            particles_.push_back(Particle{
                arrow.position,
                Vector3Scale(UpForWorldAt(arrow.position, arrow.world), RandomFloat(0.3f, 1.1f)),
                Color{165, 255, 95, 175},
                RandomFloat(0.1f, 0.22f), RandomFloat(0.1f, 0.22f),
                RandomFloat(0.035f, 0.08f)
            });
        }
        ++i;
    }
}

void Game::DamageConquestRider(float damage, Vector3 hitPosition, Color color) {
    if (!conquestRider_.active || damage <= 0.0f) return;
    if (conquestRider_.world != playerWorld_) return;
    conquestRider_.health -= damage;
    totalDamageDealt_ += damage;
    conquestRider_.hitFlash = 1.0f;
    PlayEnemyHitSfx(conquestRider_.position);
    SpawnHitBurst(hitPosition, color, 12);
    if (conquestRider_.health <= 0.0f) {
        conquestRider_.active = false;
        conquestRider_.defeated = true;
        SpawnShockwave(conquestRider_.position, 16.0f, Color{180, 255, 105, 255});
        SpawnHitBurst(conquestRider_.position, Color{200, 255, 125, 255}, 100);
        PlaySfxAt(sfxBossDeath_, conquestRider_.position, 150.0f, 0.95f);
        eventText_ = "CONQUEST BROKEN";
        eventTextTimer_ = 4.0f;
        cameraShake_ = 0.85f;
        score_ += 1100;
    }
}

void Game::DamageConquestRiderInRadius(Vector3 position, float radius, float damage, Color color) {
    if (!conquestRider_.active || damage <= 0.0f) return;
    if (conquestRider_.world != playerWorld_) return;
    float distance = Vector3Distance(position, conquestRider_.position);
    float hitRadius = radius + kHorsemanHitPadding;
    if (distance > hitRadius) return;
    float falloff = 1.0f - std::clamp(distance / std::max(0.001f, hitRadius), 0.0f, 1.0f);
    DamageConquestRider(damage * (0.35f + falloff * 0.65f), conquestRider_.position, color);
}

void Game::SpawnFamineRider() {
    famineRider_ = {};
    famineRider_.active = true;
    famineRider_.health = config_.famineRiderHealth;
    famineRider_.maxHealth = config_.famineRiderHealth;
    famineRider_.witherTimer = config_.famineRiderWitherInterval * 0.55f;
    famineRider_.world = 0;

    if (IsSphericalMap()) {
        Vector3 up = SphericalUpAt(camera_.position, 0);
        Vector3 right = {}, forward = {};
        MakeBasis(up, right, forward);
        Vector3 dir = SafeNormalize(Vector3Add(up, Vector3Scale(Vector3Subtract(right, forward), 0.32f)), up);
        if (IsHollowWorldMap()) dir = Vector3Scale(dir, -1.0f);
        famineRider_.position = SphericalSurfacePoint(dir, config_.famineRiderHoverAltitude, 0);
        Vector3 toPlayer = ProjectOnSphericalTangent(Vector3Subtract(camera_.position, famineRider_.position), SphericalUpAt(famineRider_.position, 0));
        famineRider_.forward = SafeNormalize(toPlayer, forward);
    } else {
        float angle = RandomFloat(0.0f, 2.0f * PI);
        famineRider_.orbitAngle = angle;
        float baseRadius = IsSquareMap() ? squareHalfExtent_ : arenaRadius_;
        float radius = baseRadius * config_.famineRiderOrbitRadiusFactor;
        famineRider_.position = Vector3{std::cos(angle) * radius, FlatGroundYForWorld(0) + config_.famineRiderHoverAltitude, std::sin(angle) * radius};
        famineRider_.forward = SafeNormalize(Vector3Subtract(camera_.position, famineRider_.position), Vector3{0.0f, 0.0f, 1.0f});
        famineRider_.forward.y = 0.0f;
        famineRider_.forward = SafeNormalize(famineRider_.forward, Vector3{0.0f, 0.0f, 1.0f});
    }

    PlaySfxAt(sfxBossPhase_, famineRider_.position, 160.0f, 0.92f);
    SpawnShockwave(famineRider_.position, 8.0f, Color{42, 36, 24, 255});
    SpawnHitBurst(famineRider_.position, Color{115, 98, 58, 255}, 80);
}

void Game::TriggerFamineWither() {
    if (!famineRider_.active) return;
    int affected = 0;
    int consumed = 0;
    float effectiveRadius = FamineWitherRadius();
    for (size_t i = 0; i < pickups_.size();) {
        Pickup& pickup = pickups_[i];
        if (pickup.type != PickupType::Essence || pickup.maxLife <= 0.0f) {
            ++i;
            continue;
        }
        if (Vector3Distance(pickup.position, famineRider_.position) > effectiveRadius) {
            ++i;
            continue;
        }
        pickup.age += config_.famineRiderWitherAgeBoost;
        ++affected;
        if (pickup.age >= pickup.maxLife) {
            SpawnHitBurst(pickup.position, Color{48, 42, 32, 255}, 8);
            ++consumed;
            pickups_[i] = pickups_.back();
            pickups_.pop_back();
            continue;
        }
        SpawnHitBurst(pickup.position, Color{92, 82, 55, 255}, 4);
        ++i;
    }
    if (consumed > 0) {
        FeedFamineWitheredEssence(consumed);
    }

    Vector3 up = UpForWorldAt(famineRider_.position, famineRider_.world);
    Vector3 origin = Vector3Add(famineRider_.position, Vector3Scale(up, -0.5f));
    float pulseRadius = FamineWitherRadius();
    SpawnShockwave(origin, pulseRadius, Color{68, 58, 34, 255});
    SpawnHitBurst(famineRider_.position, Color{128, 108, 62, 255}, 24 + affected * 2);
    PlaySfxAt(sfxMysticCurseOrb_, famineRider_.position, 170.0f, 0.82f);
    famineRider_.scaleTipTimer = 1.2f;
    if (famineRider_.world == playerWorld_ && Vector3Distance(camera_.position, origin) <= pulseRadius + playerRadius_) {
        famineFireRateDebuffTimer_ = std::max(famineFireRateDebuffTimer_, config_.famineRiderFireRateDebuffDuration);
        damageFlash_ = std::max(damageFlash_, 0.18f);
        SpawnHitBurst(camera_.position, Color{128, 108, 62, 255}, 10);
    }
    cameraShake_ = std::min(1.0f, cameraShake_ + 0.12f);
}

float Game::FamineWitherRadius() const {
    return config_.famineRiderWitherRadius
        + (famineRider_.active ? std::clamp(famineRider_.witherRadiusBonus, 0.0f, config_.famineRiderMaxRadiusBonus) : 0.0f);
}

void Game::FeedFamineWitheredEssence(int count) {
    if (!famineRider_.active || count <= 0) return;
    float gain = config_.famineRiderRadiusGainPerEssence * static_cast<float>(count);
    famineRider_.witherRadiusBonus = std::min(config_.famineRiderMaxRadiusBonus, famineRider_.witherRadiusBonus + gain);
    famineRider_.scaleTipTimer = std::max(famineRider_.scaleTipTimer, 0.65f);
}

void Game::UpdateFamineRider(float dt) {
    if (!famineRider_.active) return;
    if (IsEdenMap() || famineRider_.world != playerWorld_) return;

    famineRider_.gallopTimer += dt;
    famineRider_.hitFlash = std::max(0.0f, famineRider_.hitFlash - dt * 4.0f);
    famineRider_.scaleTipTimer = std::max(0.0f, famineRider_.scaleTipTimer - dt);
    famineRider_.witherTimer -= dt;
    famineRider_.wanderTimer = std::max(0.0f, famineRider_.wanderTimer - dt);

    Vector3 up = UpForWorldAt(famineRider_.position, famineRider_.world);
    Vector3 target = {};
    bool hasEssenceTarget = false;
    float bestScore = std::numeric_limits<float>::max();
    for (const Pickup& pickup : pickups_) {
        if (pickup.type != PickupType::Essence) continue;
        float distance = Vector3Distance(famineRider_.position, pickup.position);
        if (distance > config_.famineRiderEssenceSeekRange) continue;
        float ageBias = pickup.maxLife > 0.0f ? std::clamp(pickup.age / pickup.maxLife, 0.0f, 1.0f) : 0.0f;
        float score = distance * (1.0f - ageBias * 0.35f);
        if (score < bestScore) {
            bestScore = score;
            Vector3 pickupUp = UpForWorldAt(pickup.position, famineRider_.world);
            target = Vector3Add(pickup.position, Vector3Scale(pickupUp, config_.famineRiderHoverAltitude));
            hasEssenceTarget = true;
        }
    }

    if (!hasEssenceTarget) {
        if (famineRider_.wanderTimer <= 0.0f || Vector3Length(Vector3Subtract(famineRider_.wanderTarget, famineRider_.position)) < 4.0f) {
            if (IsSphericalMap()) {
                Vector3 jitter = Vector3{RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f)};
                Vector3 dir = SafeNormalize(Vector3Add(SphericalUpAt(camera_.position, famineRider_.world), Vector3Scale(jitter, 0.55f)), SphericalUpAt(camera_.position, famineRider_.world));
                if (IsHollowWorldMap()) dir = Vector3Scale(dir, -1.0f);
                famineRider_.wanderTarget = SphericalSurfacePoint(dir, config_.famineRiderHoverAltitude, famineRider_.world);
            } else {
                float baseRadius = IsSquareMap() ? squareHalfExtent_ : arenaRadius_;
                float radius = baseRadius * RandomFloat(0.28f, config_.famineRiderOrbitRadiusFactor);
                float angle = RandomFloat(0.0f, 2.0f * PI);
                famineRider_.wanderTarget = Vector3{std::cos(angle) * radius, FlatGroundYForWorld(famineRider_.world) + config_.famineRiderHoverAltitude, std::sin(angle) * radius};
            }
            famineRider_.wanderTimer = RandomFloat(2.2f, 4.8f);
        }
        target = famineRider_.wanderTarget;
    } else if (config_.famineRiderWanderJitter > 0.0f) {
        Vector3 jitter = IsSphericalMap()
            ? ProjectOnSphericalTangent(Vector3{RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f)}, up)
            : Vector3{RandomFloat(-1.0f, 1.0f), 0.0f, RandomFloat(-1.0f, 1.0f)};
        target = Vector3Add(target, Vector3Scale(SafeNormalize(jitter, famineRider_.forward), config_.famineRiderHoverAltitude * config_.famineRiderWanderJitter));
    }

    Vector3 toTarget = Vector3Subtract(target, famineRider_.position);
    Vector3 tangentToTarget = IsSphericalMap() ? ProjectOnSphericalTangent(toTarget, up) : toTarget;
    Vector3 desiredForward = SafeNormalize(tangentToTarget, famineRider_.forward);
    famineRider_.forward = SafeNormalize(Vector3Lerp(famineRider_.forward, desiredForward, std::clamp(dt * 1.8f, 0.0f, 1.0f)), desiredForward);

    if (IsSphericalMap()) {
        Vector3 moved = Vector3Add(famineRider_.position, Vector3Scale(famineRider_.forward, config_.famineRiderMoveSpeed * dt));
        famineRider_.position = SphericalSurfacePoint(moved, config_.famineRiderHoverAltitude, famineRider_.world);
    } else {
        famineRider_.position = Vector3Add(famineRider_.position, Vector3Scale(famineRider_.forward, config_.famineRiderMoveSpeed * dt));
        famineRider_.position.y = FlatGroundYForWorld(famineRider_.world) + config_.famineRiderHoverAltitude;
        if (IsSquareMap()) {
            float limit = squareHalfExtent_ * 0.92f;
            famineRider_.position.x = std::clamp(famineRider_.position.x, -limit, limit);
            famineRider_.position.z = std::clamp(famineRider_.position.z, -limit, limit);
        } else {
            Vector3 flat{famineRider_.position.x, 0.0f, famineRider_.position.z};
            float limit = arenaRadius_ * 0.92f;
            float len = Vector3Length(flat);
            if (len > limit) {
                flat = Vector3Scale(Vector3Normalize(flat), limit);
                famineRider_.position.x = flat.x;
                famineRider_.position.z = flat.z;
            }
        }
    }
    famineRider_.velocity = Vector3Scale(famineRider_.forward, config_.famineRiderMoveSpeed);

    if (famineRider_.witherTimer <= 0.0f) {
        TriggerFamineWither();
        famineRider_.witherTimer = config_.famineRiderWitherInterval * RandomFloat(0.88f, 1.16f);
    }

    if (RandomFloat(0.0f, 1.0f) < 0.38f) {
        particles_.push_back(Particle{
            Vector3Subtract(famineRider_.position, Vector3Scale(famineRider_.forward, 1.2f)),
            Vector3Scale(up, RandomFloat(-0.6f, 0.3f)),
            Color{70, 62, 42, 190},
            RandomFloat(0.22f, 0.42f), RandomFloat(0.22f, 0.42f), RandomFloat(0.05f, 0.12f)
        });
    }
}

void Game::DamageFamineRider(float damage, Vector3 hitPosition, Color color) {
    if (!famineRider_.active || damage <= 0.0f) return;
    if (famineRider_.world != playerWorld_) return;
    famineRider_.health -= damage;
    totalDamageDealt_ += damage;
    famineRider_.hitFlash = 1.0f;
    PlayEnemyHitSfx(hitPosition);
    SpawnHitBurst(hitPosition, color, 12);
    if (famineRider_.health <= 0.0f) {
        famineRider_.active = false;
        famineRider_.defeated = true;
        SpawnShockwave(famineRider_.position, 16.0f, Color{96, 82, 48, 255});
        SpawnHitBurst(famineRider_.position, Color{150, 126, 72, 255}, 110);
        PlaySfxAt(sfxBossDeath_, famineRider_.position, 150.0f, 0.92f);
        eventText_ = "FAMINE ENDS";
        eventTextTimer_ = 4.0f;
        cameraShake_ = 0.82f;
        score_ += 1200;
    }
}

void Game::DamageFamineRiderInRadius(Vector3 position, float radius, float damage, Color color) {
    if (!famineRider_.active || damage <= 0.0f) return;
    if (famineRider_.world != playerWorld_) return;
    float distance = Vector3Distance(position, famineRider_.position);
    float hitRadius = radius + kHorsemanHitPadding;
    if (distance > hitRadius) return;
    float falloff = 1.0f - std::clamp(distance / std::max(0.001f, hitRadius), 0.0f, 1.0f);
    DamageFamineRider(damage * (0.35f + falloff * 0.65f), famineRider_.position, color);
}

void Game::SpawnDeathRider() {
    deathRider_ = {};
    deathRider_.active = true;
    deathRider_.health = config_.deathRiderHealth;
    deathRider_.maxHealth = config_.deathRiderHealth;
    deathRider_.skullTimer = config_.deathRiderSkullInterval * 0.55f;
    deathRider_.world = 0;

    if (IsSphericalMap()) {
        Vector3 up = SphericalUpAt(camera_.position, 0);
        Vector3 right = {}, forward = {};
        MakeBasis(up, right, forward);
        Vector3 dir = SafeNormalize(Vector3Add(up, Vector3Scale(Vector3Add(right, forward), -0.35f)), up);
        if (IsHollowWorldMap()) dir = Vector3Scale(dir, -1.0f);
        deathRider_.position = SphericalSurfacePoint(dir, config_.deathRiderHoverAltitude, 0);
        Vector3 toPlayer = ProjectOnSphericalTangent(Vector3Subtract(camera_.position, deathRider_.position), SphericalUpAt(deathRider_.position, 0));
        deathRider_.forward = SafeNormalize(toPlayer, forward);
    } else {
        float angle = RandomFloat(0.0f, 2.0f * PI);
        deathRider_.orbitAngle = angle;
        float baseRadius = IsSquareMap() ? squareHalfExtent_ : arenaRadius_;
        float radius = baseRadius * config_.deathRiderOrbitRadiusFactor;
        deathRider_.position = Vector3{std::cos(angle) * radius, FlatGroundYForWorld(0) + config_.deathRiderHoverAltitude, std::sin(angle) * radius};
        deathRider_.forward = SafeNormalize(Vector3Subtract(camera_.position, deathRider_.position), Vector3{0.0f, 0.0f, 1.0f});
        deathRider_.forward.y = 0.0f;
        deathRider_.forward = SafeNormalize(deathRider_.forward, Vector3{0.0f, 0.0f, 1.0f});
    }

    PlaySfxAt(sfxBossSpawn_, deathRider_.position, 160.0f, 0.72f);
    SpawnShockwave(deathRider_.position, 9.0f, Color{96, 96, 104, 255});
    SpawnHitBurst(deathRider_.position, Color{70, 70, 78, 255}, 90);
}

void Game::SpawnDeathSoul(Vector3 position, int world) {
    if (!deathRider_.active || world != deathRider_.world) return;
    DeathSoul soul;
    soul.position = position;
    soul.velocity = Vector3Scale(SafeNormalize(Vector3Subtract(deathRider_.position, position), RandomUnitVector()), RandomFloat(4.0f, 9.0f));
    soul.life = 8.0f;
    soul.radius = RandomFloat(0.24f, 0.42f);
    soul.world = world;
    deathSouls_.push_back(soul);
}

void Game::UpdateDeathSouls(float dt) {
    for (size_t i = 0; i < deathSouls_.size();) {
        DeathSoul& soul = deathSouls_[i];
        soul.life -= dt;
        if (!deathRider_.active || soul.life <= 0.0f || soul.world != deathRider_.world) {
            deathSouls_[i] = deathSouls_.back();
            deathSouls_.pop_back();
            continue;
        }
        Vector3 toDeath = Vector3Subtract(deathRider_.position, soul.position);
        float distance = Vector3Length(toDeath);
        if (distance <= soul.radius + 1.15f) {
            deathRider_.souls += 1;
            SpawnHitBurst(deathRider_.position, Color{34, 34, 42, 255}, 8);
            if (deathRider_.souls >= config_.deathRiderSoulThreshold) {
                TriggerDeathSkullSwarm();
            }
            deathSouls_[i] = deathSouls_.back();
            deathSouls_.pop_back();
            continue;
        }
        Vector3 dir = SafeNormalize(toDeath, deathRider_.forward);
        soul.velocity = Vector3Lerp(soul.velocity, Vector3Scale(dir, config_.deathRiderSoulSpeed), std::clamp(dt * 2.8f, 0.0f, 1.0f));
        soul.position = Vector3Add(soul.position, Vector3Scale(soul.velocity, dt));
        if (RandomFloat(0.0f, 1.0f) < 0.45f) {
            particles_.push_back(Particle{
                soul.position,
                Vector3Scale(dir, -RandomFloat(0.4f, 1.6f)),
                Color{18, 18, 24, 185},
                RandomFloat(0.12f, 0.28f), RandomFloat(0.12f, 0.28f), RandomFloat(0.035f, 0.075f)
            });
        }
        ++i;
    }
}

void Game::FireDeathSkull(Vector3 origin, float waitTimer, Vector3 initialDirection) {
    if (!deathRider_.active) return;
    Vector3 dir = Vector3Length(initialDirection) > 0.01f
        ? SafeNormalize(initialDirection, deathRider_.forward)
        : SafeNormalize(Vector3Subtract(camera_.position, origin), deathRider_.forward);
    DeathSkull skull;
    skull.position = origin;
    skull.forward = dir;
    skull.velocity = waitTimer > 0.0f ? Vector3Zero() : Vector3Scale(dir, config_.deathRiderSkullSpeed);
    skull.health = config_.deathRiderSkullHealth;
    skull.life = config_.deathRiderSkullLife + waitTimer;
    skull.waitTimer = waitTimer;
    skull.radius = 0.55f;
    skull.world = deathRider_.world;
    deathSkulls_.push_back(skull);
}

void Game::TriggerDeathSkullSwarm() {
    if (!deathRider_.active || deathRider_.souls < config_.deathRiderSoulThreshold) return;
    deathRider_.souls = std::max(0, deathRider_.souls - config_.deathRiderSoulThreshold);
    Vector3 up = UpForWorldAt(camera_.position, deathRider_.world);
    Vector3 right = SafeNormalize(Vector3CrossProduct(up, PlayerForward()), PlayerRight());
    Vector3 forward = SafeNormalize(ProjectOnSphericalTangent(PlayerForward(), up), PlayerForward());
    for (int i = 0; i < config_.deathRiderSkullSwarmCount; ++i) {
        float angle = (static_cast<float>(i) / static_cast<float>(std::max(1, config_.deathRiderSkullSwarmCount))) * 2.0f * PI
            + RandomFloat(-0.18f, 0.18f);
        float radius = config_.deathRiderSkullSwarmRadius * RandomFloat(0.45f, 1.0f);
        Vector3 offset = Vector3Add(Vector3Scale(right, std::cos(angle) * radius), Vector3Scale(forward, std::sin(angle) * radius));
        Vector3 spawn = Vector3Add(camera_.position, Vector3Add(offset, Vector3Scale(up, RandomFloat(3.5f, 8.0f))));
        if (IsSphericalMap()) {
            float alt = SphericalAltitudeAt(spawn, deathRider_.world);
            spawn = SphericalSurfacePoint(spawn, std::max(alt, 4.0f), deathRider_.world);
        }
        FireDeathSkull(spawn, config_.deathRiderSkullSwarmDelay + RandomFloat(0.0f, 0.45f), Vector3Subtract(camera_.position, spawn));
    }
    SpawnShockwave(camera_.position, config_.deathRiderSkullSwarmRadius, Color{210, 210, 220, 255});
    PlaySfxAt(sfxBossBarrage_, deathRider_.position, 170.0f, 0.62f);
}

void Game::UpdateDeathSkulls(float dt) {
    for (size_t i = 0; i < deathSkulls_.size();) {
        DeathSkull& skull = deathSkulls_[i];
        skull.life -= dt;
        if (skull.life <= 0.0f || skull.health <= 0.0f) {
            SpawnHitBurst(skull.position, Color{230, 230, 220, 255}, skull.health <= 0.0f ? 14 : 6);
            deathSkulls_[i] = deathSkulls_.back();
            deathSkulls_.pop_back();
            continue;
        }
        if (skull.waitTimer > 0.0f) {
            skull.waitTimer = std::max(0.0f, skull.waitTimer - dt);
            skull.forward = SafeNormalize(Vector3Subtract(camera_.position, skull.position), skull.forward);
            if (skull.waitTimer <= 0.0f) {
                skull.velocity = Vector3Scale(skull.forward, config_.deathRiderSkullSpeed * 1.15f);
            }
        } else {
            Vector3 toPlayer = Vector3Subtract(camera_.position, skull.position);
            Vector3 desired = SafeNormalize(toPlayer, skull.forward);
            float speed = std::max(config_.deathRiderSkullSpeed, Vector3Length(skull.velocity));
            if (config_.deathRiderSkullTurnRate > 0.0f) {
                skull.forward = SafeNormalize(Vector3Lerp(skull.forward, desired, std::clamp(dt * config_.deathRiderSkullTurnRate, 0.0f, 1.0f)), desired);
            }
            skull.velocity = Vector3Scale(skull.forward, speed);
            skull.position = Vector3Add(skull.position, Vector3Scale(skull.velocity, dt));
            bool touchesGround = false;
            if (IsSphericalMap()) {
                touchesGround = SphericalTouchesSurface(skull.position, skull.radius, skull.world);
            } else if (IsEdenMap()) {
                touchesGround = skull.position.y <= EdenGroundYAt(skull.position) + skull.radius * 0.35f;
            } else {
                float groundY = FlatGroundYForWorld(skull.world) + FlatUpForWorld(skull.world).y * 0.22f;
                touchesGround = skull.world == 0
                    ? skull.position.y <= groundY + skull.radius * 0.35f
                    : skull.position.y >= groundY - skull.radius * 0.35f;
            }
            if (touchesGround) {
                SpawnHitBurst(skull.position, Color{230, 230, 220, 255}, 10);
                deathSkulls_[i] = deathSkulls_.back();
                deathSkulls_.pop_back();
                continue;
            }
            if (skull.world == playerWorld_ && Vector3Distance(camera_.position, skull.position) <= skull.radius + playerRadius_) {
                ApplyPlayerHit(camera_.position, Color{230, 230, 220, 255}, "SKULL");
                SpawnHitBurst(skull.position, Color{245, 245, 235, 255}, 18);
                deathSkulls_[i] = deathSkulls_.back();
                deathSkulls_.pop_back();
                continue;
            }
        }
        if (RandomFloat(0.0f, 1.0f) < 0.35f) {
            particles_.push_back(Particle{
                skull.position,
                Vector3Scale(skull.forward, -RandomFloat(0.5f, 2.0f)),
                Color{225, 225, 215, 160},
                RandomFloat(0.08f, 0.18f), RandomFloat(0.08f, 0.18f), RandomFloat(0.025f, 0.055f)
            });
        }
        ++i;
    }
}

void Game::DamageDeathSkullsInRadius(Vector3 position, float radius, float damage, Color color) {
    if (damage <= 0.0f) return;
    for (size_t i = 0; i < deathSkulls_.size();) {
        float distance = Vector3Distance(position, deathSkulls_[i].position);
        if (distance <= radius + deathSkulls_[i].radius) {
            float falloff = 1.0f - std::clamp(distance / std::max(0.001f, radius + deathSkulls_[i].radius), 0.0f, 1.0f);
            deathSkulls_[i].health -= damage * (0.35f + falloff * 0.65f);
            SpawnHitBurst(deathSkulls_[i].position, color, 5);
            if (deathSkulls_[i].health <= 0.0f) {
                SpawnHitBurst(deathSkulls_[i].position, Color{245, 245, 235, 255}, 12);
                deathSkulls_[i] = deathSkulls_.back();
                deathSkulls_.pop_back();
                continue;
            }
        }
        ++i;
    }
}

void Game::UpdateDeathRider(float dt) {
    if (!deathRider_.active) return;
    if (IsEdenMap() || deathRider_.world != playerWorld_) return;

    deathRider_.gallopTimer += dt;
    deathRider_.hitFlash = std::max(0.0f, deathRider_.hitFlash - dt * 4.0f);
    deathRider_.skullTimer -= dt;

    Vector3 up = UpForWorldAt(deathRider_.position, deathRider_.world);
    Vector3 toPlayer = Vector3Subtract(camera_.position, deathRider_.position);
    Vector3 desired = IsSphericalMap() ? ProjectOnSphericalTangent(toPlayer, up) : Vector3{toPlayer.x, 0.0f, toPlayer.z};
    desired = SafeNormalize(desired, deathRider_.forward);
    deathRider_.forward = SafeNormalize(Vector3Lerp(deathRider_.forward, desired, std::clamp(dt * 1.7f, 0.0f, 1.0f)), desired);

    if (IsSphericalMap()) {
        Vector3 tangent = SafeNormalize(ProjectOnSphericalTangent(deathRider_.forward, up), PlayerForward());
        deathRider_.position = Vector3Add(deathRider_.position, Vector3Scale(tangent, config_.deathRiderMoveSpeed * dt));
        deathRider_.position = SphericalSurfacePoint(deathRider_.position, config_.deathRiderHoverAltitude, deathRider_.world);
    } else {
        float baseRadius = IsSquareMap() ? squareHalfExtent_ : arenaRadius_;
        float orbitRadius = std::max(2.0f, baseRadius * config_.deathRiderOrbitRadiusFactor);
        deathRider_.orbitAngle += dt * config_.deathRiderMoveSpeed / orbitRadius;
        Vector3 orbit{
            std::cos(deathRider_.orbitAngle) * orbitRadius,
            FlatGroundYForWorld(deathRider_.world) + config_.deathRiderHoverAltitude,
            std::sin(deathRider_.orbitAngle) * orbitRadius
        };
        deathRider_.position = orbit;
    }
    deathRider_.velocity = Vector3Scale(deathRider_.forward, config_.deathRiderMoveSpeed);

    if (deathRider_.skullTimer <= 0.0f) {
        Vector3 origin = Vector3Add(deathRider_.position, Vector3Add(Vector3Scale(up, 1.25f), Vector3Scale(deathRider_.forward, 2.2f)));
        FireDeathSkull(origin);
        deathRider_.skullTimer = config_.deathRiderSkullInterval * RandomFloat(0.85f, 1.15f);
        PlaySfxAt(sfxBossBarrage_, origin, 135.0f, 0.48f);
    }

    if (RandomFloat(0.0f, 1.0f) < 0.32f) {
        particles_.push_back(Particle{
            Vector3Subtract(deathRider_.position, Vector3Scale(deathRider_.forward, 1.1f)),
            Vector3Scale(up, RandomFloat(-0.3f, 0.7f)),
            Color{36, 36, 44, 190},
            RandomFloat(0.18f, 0.36f), RandomFloat(0.18f, 0.36f), RandomFloat(0.04f, 0.09f)
        });
    }
}

void Game::DamageDeathRider(float damage, Vector3 hitPosition, Color color) {
    if (!deathRider_.active || damage <= 0.0f) return;
    if (deathRider_.world != playerWorld_) return;
    deathRider_.health -= damage;
    totalDamageDealt_ += damage;
    deathRider_.hitFlash = 1.0f;
    PlayEnemyHitSfx(hitPosition);
    SpawnHitBurst(hitPosition, color, 12);
    if (deathRider_.health <= 0.0f) {
        deathRider_.active = false;
        deathRider_.defeated = true;
        deathSouls_.clear();
        SpawnShockwave(deathRider_.position, 18.0f, Color{72, 72, 84, 255});
        SpawnHitBurst(deathRider_.position, Color{96, 96, 108, 255}, 120);
        PlaySfxAt(sfxBossDeath_, deathRider_.position, 155.0f, 0.78f);
        eventText_ = "DEATH WITHERS";
        eventTextTimer_ = 4.0f;
        cameraShake_ = 0.85f;
        score_ += 1400;
    }
}

void Game::DamageDeathRiderInRadius(Vector3 position, float radius, float damage, Color color) {
    if (!deathRider_.active || damage <= 0.0f) return;
    if (deathRider_.world != playerWorld_) return;
    float distance = Vector3Distance(position, deathRider_.position);
    float hitRadius = radius + kDeathRiderHitPadding;
    if (distance > hitRadius) return;
    float falloff = 1.0f - std::clamp(distance / std::max(0.001f, hitRadius), 0.0f, 1.0f);
    DamageDeathRider(damage * (0.35f + falloff * 0.65f), deathRider_.position, color);
}

bool Game::ScavengerUfoEncounterActive() const {
    return scavengerUfo_.state == ScavengerUfoState::Pending
        || scavengerUfo_.state == ScavengerUfoState::Active
        || scavengerUfo_.state == ScavengerUfoState::Escaping
        || scavengerUfo_.state == ScavengerUfoState::DefeatedFalling;
}

bool Game::ScavengerUfoDamageable() const {
    return scavengerUfo_.state == ScavengerUfoState::Active
        || scavengerUfo_.state == ScavengerUfoState::Escaping;
}

bool Game::UfoPilotActive() const {
    return scavengerUfo_.state == ScavengerUfoState::PlayerPiloted;
}

bool Game::UfoHyperspaceActive() const {
    return ufoTravelState_ != UfoTravelState::Inactive;
}

bool Game::UfoEnterAvailable() const {
    if (scavengerUfo_.state != ScavengerUfoState::DefeatedLanded
        && scavengerUfo_.state != ScavengerUfoState::ParkedHover) {
        return false;
    }
    if (playerWorld_ != 0) return false;
    return Vector3Distance(camera_.position, scavengerUfo_.position) <= config_.ufoEnterRange;
}

void Game::EnterScavengerUfo() {
    if (!UfoEnterAvailable()) return;
    playerWorld_ = 0;
    scavengerUfo_.state = ScavengerUfoState::PlayerPiloted;
    scavengerUfo_.velocity = {};
    scavengerUfo_.altitudeTarget = IsSphericalMap()
        ? SphericalAltitudeAt(scavengerUfo_.position, 0)
        : std::abs(Vector3DotProduct(Vector3Subtract(scavengerUfo_.position, Vector3{0.0f, FlatGroundYForWorld(0), 0.0f}), FlatUpForWorld(0)));
    scavengerUfo_.pilotOrbTimer = 0.0f;
    scavengerUfo_.pilotJumpCooldown = 0.0f;
    ufoPilotWeapon_ = UfoPilotWeapon::Orb;
    ufoOrbMode_ = UfoOrbMode::Projectile;
    fireControlActive_ = false;
    eventText_ = "UFO ONLINE";
    eventTextTimer_ = 1.5f;
    PlaySfxAt(sfxNanoCommand_, scavengerUfo_.position, 80.0f, 0.85f);
}

void Game::ExitScavengerUfo() {
    if (!UfoPilotActive()) return;
    StopSfx(sfxUfoTractor_);
    StopSfx(sfxLaserBeam_);
    Vector3 up = UpForWorldAt(scavengerUfo_.position, 0);
    Vector3 right = PlayerRight();
    camera_.position = Vector3Add(scavengerUfo_.position, Vector3Add(Vector3Scale(right, 3.0f), Vector3Scale(up, -0.8f)));
    camera_.up = up;
    playerWorld_ = 0;
    playerVelocity_ = Vector3Scale(up, 0.5f);
    scavengerUfo_.state = ScavengerUfoState::ParkedHover;
    scavengerUfo_.velocity = {};
    scavengerUfo_.altitudeTarget = IsSphericalMap()
        ? SphericalAltitudeAt(scavengerUfo_.position, 0)
        : std::abs(Vector3DotProduct(Vector3Subtract(scavengerUfo_.position, Vector3{0.0f, FlatGroundYForWorld(0), 0.0f}), FlatUpForWorld(0)));
    eventText_ = "UFO PARKED";
    eventTextTimer_ = 1.2f;
}

void Game::TeleportPilotedUfo() {
    if (!UfoPilotActive()) return;
    if (scavengerUfo_.pilotJumpCooldown > 0.0f || scavengerUfo_.pilotEssence < config_.ufoPilotJumpCost) return;
    scavengerUfo_.pilotEssence -= config_.ufoPilotJumpCost;
    scavengerUfo_.pilotJumpCooldown = config_.ufoPilotJumpCooldown;

    Vector3 target = {};
    if (IsSphericalMap()) {
        Vector3 dir = SafeNormalize(Vector3{
            RandomFloat(-1.0f, 1.0f),
            RandomFloat(-1.0f, 1.0f),
            RandomFloat(-1.0f, 1.0f)
        }, Vector3{1.0f, 0.0f, 0.0f});
        target = SphericalSurfacePoint(dir, config_.ufoPilotHoverAltitude, 0);
    } else if (IsSquareMap()) {
        float limit = squareHalfExtent_ * 0.72f;
        Vector3 up = FlatUpForWorld(0);
        target = Vector3{RandomFloat(-limit, limit), FlatGroundYForWorld(0), RandomFloat(-limit, limit)};
        target = Vector3Add(target, Vector3Scale(up, config_.ufoPilotHoverAltitude));
    } else {
        float angle = RandomFloat(0.0f, 2.0f * PI);
        float radius = arenaRadius_ * std::sqrt(RandomFloat(0.0f, 0.72f));
        Vector3 up = FlatUpForWorld(0);
        target = Vector3{std::cos(angle) * radius, FlatGroundYForWorld(0), std::sin(angle) * radius};
        target = Vector3Add(target, Vector3Scale(up, config_.ufoPilotHoverAltitude));
    }

    SpawnShockwave(scavengerUfo_.position, 5.5f, Color{90, 235, 255, 255});
    scavengerUfo_.position = target;
    scavengerUfo_.velocity = {};
    camera_.position = scavengerUfo_.position;
    camera_.up = UpForWorldAt(camera_.position, 0);
    SpawnShockwave(scavengerUfo_.position, 6.5f, Color{150, 80, 255, 255});
    eventText_ = "HYPERSPACE";
    eventTextTimer_ = 1.4f;
    PlaySfxAt(sfxBossPhase_, scavengerUfo_.position, 120.0f, 0.9f);
}

void Game::BeginUfoHyperspaceCharge() {
    if (!UfoPilotActive()) return;
    StopSfx(sfxUfoTractor_);
    StopSfx(sfxLaserBeam_);
    if (scavengerUfo_.pilotJumpCooldown > 0.0f || scavengerUfo_.pilotEssence < config_.ufoPilotJumpCost) {
        eventText_ = "HYPERSPACE NEEDS ESSENCE";
        eventTextTimer_ = 0.8f;
        return;
    }
    ufoTravelState_ = UfoTravelState::Charging;
    ufoHyperspaceHoldTimer_ = 0.0f;
    ufoHyperspaceFlash_ = 0.0f;
}

void Game::BeginUfoHyperspace() {
    if (!UfoPilotActive()) return;
    if (scavengerUfo_.pilotEssence < config_.ufoPilotJumpCost) {
        ufoTravelState_ = UfoTravelState::Inactive;
        ufoHyperspaceHoldTimer_ = 0.0f;
        StopSfx(sfxUfoHyperspaceCharge_);
        return;
    }
    StopSfx(sfxUfoHyperspaceCharge_);
    scavengerUfo_.pilotEssence = std::max(0, scavengerUfo_.pilotEssence - config_.ufoPilotJumpCost);
    scavengerUfo_.pilotJumpCooldown = config_.ufoPilotJumpCooldown;
    ufoTravelState_ = UfoTravelState::Hyperspace;
    ufoHyperspaceTimer_ = 0.0f;
    ufoHyperspaceObstacleTimer_ = 0.2f;
    ufoHyperspaceObstacles_.clear();
    ufoHyperspaceAngle_ = 0.0f;
    ufoHyperspaceAltitude_ = std::clamp(config_.ufoHyperspaceMinAltitude + 0.8f,
        config_.ufoHyperspaceMinAltitude,
        config_.ufoHyperspaceMaxAltitude);
    ufoHyperspaceFlash_ = 1.0f;
    cameraShake_ = std::min(1.0f, cameraShake_ + 0.7f);
    eventText_ = "HYPERSPACE";
    eventTextTimer_ = 1.2f;
    PlaySfxAt(sfxBossPhase_, scavengerUfo_.position, 120.0f, 1.0f);
}

void Game::UpdateUfoHyperspace(float dt) {
    if (ufoTravelState_ == UfoTravelState::Inactive) return;

    ufoHyperspaceFlash_ = std::max(0.0f, ufoHyperspaceFlash_ - dt * 1.8f);

    if (ufoTravelState_ == UfoTravelState::Charging) {
        if (!UfoPilotActive() || !IsKeyDown(KEY_H) || consoleOpen_) {
            ufoTravelState_ = UfoTravelState::Inactive;
            ufoHyperspaceHoldTimer_ = 0.0f;
            StopSfx(sfxUfoHyperspaceCharge_);
            return;
        }
        if (scavengerUfo_.pilotJumpCooldown > 0.0f || scavengerUfo_.pilotEssence < config_.ufoPilotJumpCost) {
            ufoTravelState_ = UfoTravelState::Inactive;
            ufoHyperspaceHoldTimer_ = 0.0f;
            StopSfx(sfxUfoHyperspaceCharge_);
            eventText_ = "HYPERSPACE NEEDS ESSENCE";
            eventTextTimer_ = 0.8f;
            return;
        }
        UpdateLoopingSfxAt(sfxUfoHyperspaceCharge_, camera_.position, 64.0f, 0.72f);
        ufoHyperspaceHoldTimer_ += dt;
        cameraShake_ = std::min(0.35f, cameraShake_ + dt * 0.22f);
        if (ufoHyperspaceHoldTimer_ >= config_.ufoHyperspaceHoldTime) {
            BeginUfoHyperspace();
        }
        return;
    }

    if (ufoTravelState_ == UfoTravelState::Arriving) {
        ufoHyperspaceFlash_ = std::min(1.0f, ufoHyperspaceFlash_ + dt * 3.0f);
        if (ufoHyperspaceFlash_ >= 0.95f) {
            CompleteUfoHyperspaceArrival();
        }
        return;
    }

    ufoHyperspaceTimer_ += dt;
    if (!consoleOpen_) {
        if (IsKeyDown(KEY_A)) {
            ufoHyperspaceAngle_ += config_.ufoHyperspaceAngularSpeed * dt;
        }
        if (IsKeyDown(KEY_D)) {
            ufoHyperspaceAngle_ -= config_.ufoHyperspaceAngularSpeed * dt;
        }
        if (IsKeyDown(KEY_SPACE)) {
            ufoHyperspaceAltitude_ += config_.ufoHyperspaceVerticalSpeed * dt;
        }
        if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
            ufoHyperspaceAltitude_ -= config_.ufoHyperspaceVerticalSpeed * dt;
        }
    }
    ufoHyperspaceAltitude_ = std::clamp(ufoHyperspaceAltitude_,
        config_.ufoHyperspaceMinAltitude,
        config_.ufoHyperspaceMaxAltitude);

    ufoHyperspaceObstacleTimer_ -= dt;
    while (ufoHyperspaceObstacleTimer_ <= 0.0f) {
        UfoHyperspaceObstacle obstacle;
        obstacle.angle = RandomFloat(0.0f, 2.0f * PI);
        obstacle.altitude = RandomFloat(config_.ufoHyperspaceMinAltitude, config_.ufoHyperspaceMaxAltitude);
        obstacle.distance = config_.ufoHyperspaceObstacleSpawnDistance;
        obstacle.radius = config_.ufoHyperspaceObstacleRadius * RandomFloat(0.75f, 1.35f);
        ufoHyperspaceObstacles_.push_back(obstacle);
        ufoHyperspaceObstacleTimer_ += config_.ufoHyperspaceObstacleInterval;
    }

    for (size_t i = 0; i < ufoHyperspaceObstacles_.size();) {
        UfoHyperspaceObstacle& obstacle = ufoHyperspaceObstacles_[i];
        obstacle.distance -= config_.ufoHyperspaceObstacleSpeed * dt;
        float angleDelta = std::atan2(std::sin(obstacle.angle - ufoHyperspaceAngle_), std::cos(obstacle.angle - ufoHyperspaceAngle_));
        float ringDistance = std::abs(angleDelta) * std::max(1.0f, config_.ufoHyperspaceTunnelRadius - ufoHyperspaceAltitude_);
        float altitudeDistance = std::abs(obstacle.altitude - ufoHyperspaceAltitude_);
        bool closeLongitudinal = std::abs(obstacle.distance) <= obstacle.radius + 1.0f;
        bool closeCrossSection = std::sqrt(ringDistance * ringDistance + altitudeDistance * altitudeDistance) <= obstacle.radius + 0.9f;
        if (!obstacle.hit && closeLongitudinal && closeCrossSection) {
            obstacle.hit = true;
            scavengerUfo_.pilotEssence = std::max(0, scavengerUfo_.pilotEssence - static_cast<int>(std::ceil(config_.ufoHyperspaceObstacleDamage)));
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.55f);
            ufoHyperspaceFlash_ = std::max(ufoHyperspaceFlash_, 0.45f);
            PlaySfxAt(sfxArmorHit_, camera_.position, 80.0f, 0.8f);
        }
        if (obstacle.distance < -std::max(8.0f, obstacle.radius * 6.0f)) {
            ufoHyperspaceObstacles_[i] = ufoHyperspaceObstacles_.back();
            ufoHyperspaceObstacles_.pop_back();
            continue;
        }
        ++i;
    }

    if (ufoHyperspaceTimer_ >= config_.ufoHyperspaceDuration) {
        ufoTravelState_ = UfoTravelState::Arriving;
        ufoHyperspaceFlash_ = 0.1f;
    }
}

void Game::CompleteUfoHyperspaceArrival() {
    UfoPreservedState preserved;
    preserved.ufoEssence = scavengerUfo_.pilotEssence;
    preserved.playerEssence = essence_;
    preserved.totalCollected = scavengerUfo_.pilotTotalCollected;
    preserved.weapon = ufoPilotWeapon_;
    preserved.orbMode = ufoOrbMode_;
    if (!ufoArrivalBaseConfigCaptured_) {
        ufoArrivalBaseConfig_ = config_;
        ufoArrivalBaseConfigCaptured_ = true;
    }

    static const std::vector<std::string> kModes = {"survival", "duel", "tutorial"};
    static const std::vector<std::string> kMaps = {"circle", "square_obstacle", "asteroid", "hollow_world"};
    std::string nextMode = PickDifferentString(kModes, config_.gameMode);
    std::string nextMap = PickDifferentString(kMaps, config_.mapType);
    for (int attempts = 0; attempts < 8 && nextMode == config_.gameMode && nextMap == config_.mapType; ++attempts) {
        nextMode = kModes[GetRandomValue(0, static_cast<int>(kModes.size()) - 1)];
        nextMap = kMaps[GetRandomValue(0, static_cast<int>(kMaps.size()) - 1)];
    }

    ResetWorldForUfoArrival(nextMode, nextMap, preserved);
}

void Game::ResetWorldForUfoArrival(const std::string& mode, const std::string& map, const UfoPreservedState& preserved) {
    GameplayConfig baseConfig = ufoArrivalBaseConfigCaptured_ ? ufoArrivalBaseConfig_ : config_;
    config_ = baseConfig;
    config_.gameMode = mode;
    config_.mapType = map;
    ApplyUfoArrivalVariant(baseConfig);
    arenaRadius_ = config_.circleRadius;
    bool startWithVehicle = config_.ufoStartWithVehicle;
    config_.ufoStartWithVehicle = false;
    Reset();
    config_.ufoStartWithVehicle = startWithVehicle;

    scavengerUfo_ = {};
    scavengerUfo_.state = ScavengerUfoState::PlayerPiloted;
    scavengerUfo_.world = 0;
    scavengerUfo_.health = config_.ufoHealth;
    scavengerUfo_.maxHealth = config_.ufoHealth;
    scavengerUfo_.pilotEssence = std::clamp(preserved.ufoEssence, 0, config_.ufoPilotEssenceMax);
    scavengerUfo_.pilotTotalCollected = preserved.totalCollected;
    scavengerUfo_.altitudeTarget = config_.ufoArrivalAltitude;
    scavengerUfo_.triggeredThisRun = true;
    essence_ = std::max(0, preserved.playerEssence);
    ufoPilotWeapon_ = preserved.weapon;
    ufoOrbMode_ = preserved.orbMode;
    ufoTravelState_ = UfoTravelState::Inactive;
    ufoHyperspaceObstacles_.clear();
    ufoHyperspaceHoldTimer_ = 0.0f;
    ufoHyperspaceTimer_ = 0.0f;
    ufoHyperspaceObstacleTimer_ = 0.0f;
    ufoHyperspaceFlash_ = 0.42f;

    if (IsSphericalMap()) {
        Vector3 dir = SafeNormalize(Vector3{RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f)},
            Vector3{0.0f, 1.0f, 0.0f});
        scavengerUfo_.position = SphericalSurfacePoint(dir, config_.ufoArrivalAltitude, 0);
    } else if (IsSquareMap()) {
        float limit = squareHalfExtent_ * 0.35f;
        Vector3 up = FlatUpForWorld(0);
        scavengerUfo_.position = Vector3Add(Vector3{RandomFloat(-limit, limit), FlatGroundYForWorld(0), RandomFloat(-limit, limit)},
            Vector3Scale(up, config_.ufoArrivalAltitude));
    } else {
        float angle = RandomFloat(0.0f, 2.0f * PI);
        float radius = arenaRadius_ * std::sqrt(RandomFloat(0.0f, 0.35f));
        Vector3 up = FlatUpForWorld(0);
        scavengerUfo_.position = Vector3Add(Vector3{std::cos(angle) * radius, FlatGroundYForWorld(0), std::sin(angle) * radius},
            Vector3Scale(up, config_.ufoArrivalAltitude));
    }

    camera_.position = scavengerUfo_.position;
    playerWorld_ = 0;
    camera_.up = UpForWorldAt(camera_.position, 0);
    yaw_ = -90.0f;
    pitch_ = -6.0f;
    camera_.target = Vector3Add(camera_.position, PlayerForward());
    playerVelocity_ = {};
    eventText_ = TextFormat("ARRIVED %s | %s", config_.gameMode.c_str(), config_.mapType.c_str());
    eventTextTimer_ = 3.0f;
    PlaySfxAt(sfxBossSpawn_, scavengerUfo_.position, 150.0f, 0.9f);
}

void Game::ApplyUfoArrivalVariant(const GameplayConfig& baseConfig) {
    if (!baseConfig.ufoArrivalVariantEnabled) {
        return;
    }

    float worldVariance = std::clamp(baseConfig.ufoArrivalWorldVariance, 0.0f, 0.5f);
    float enemyVariance = std::clamp(baseConfig.ufoArrivalEnemyVariance, 0.0f, 0.5f);
    auto vary = [](float base, float variance) {
        return base * RandomFloat(1.0f - variance, 1.0f + variance);
    };
    auto clampInt = [](int value, int lo, int hi) {
        return std::max(lo, std::min(hi, value));
    };

    float gravityScale = vary(1.0f, worldVariance * 0.8f);
    config_.gravity = std::max(4.0f, baseConfig.gravity * gravityScale);
    config_.spaceSuitGravityScale = std::clamp(baseConfig.spaceSuitGravityScale * RandomFloat(0.9f, 1.1f), 0.05f, 1.0f);

    config_.circleRadius = std::clamp(vary(baseConfig.circleRadius, worldVariance), 14.0f, std::max(15.0f, squareHalfExtent_ - 1.0f));
    config_.asteroidRadius = std::max(14.0f, vary(baseConfig.asteroidRadius, worldVariance));
    config_.hollowWorldRadius = std::max(24.0f, vary(baseConfig.hollowWorldRadius, worldVariance));

    float altitudeScale = vary(1.0f, worldVariance * 0.7f);
    config_.flightMaxAltitude = std::max(config_.flightMinAltitude + 2.0f, baseConfig.flightMaxAltitude * altitudeScale);
    config_.ufoPilotHoverAltitude = std::max(3.0f, baseConfig.ufoPilotHoverAltitude * altitudeScale);
    config_.ufoPilotMoveSpeed = std::max(6.0f, vary(baseConfig.ufoPilotMoveSpeed, worldVariance * 0.8f));
    config_.ufoPilotVerticalSpeed = std::max(3.0f, vary(baseConfig.ufoPilotVerticalSpeed, worldVariance * 0.8f));

    float tempoScale = vary(1.0f, enemyVariance);
    config_.bossSpawnTime = std::max(30.0f, baseConfig.bossSpawnTime / tempoScale);
    config_.slimeKingSpawnTime = std::max(config_.bossSpawnTime + 25.0f, baseConfig.slimeKingSpawnTime / tempoScale);
    config_.spitterFireInterval = std::max(0.5f, baseConfig.spitterFireInterval / tempoScale);
    config_.pouncerLeapInterval = std::max(0.6f, baseConfig.pouncerLeapInterval / tempoScale);
    config_.harrierSpeed = std::max(2.0f, vary(baseConfig.harrierSpeed, enemyVariance));
    config_.blinkerCooldown = std::max(0.8f, baseConfig.blinkerCooldown / tempoScale);
    config_.dummySpawnInterval = std::max(0.6f, baseConfig.dummySpawnInterval / tempoScale);
    config_.duelistCount = clampInt(baseConfig.duelistCount + GetRandomValue(-1, 1), 1, 3);

    config_.essenceRespawnTime = std::max(12.0f, vary(baseConfig.essenceRespawnTime, worldVariance));
    config_.essenceMaxOnMap = clampInt(baseConfig.essenceMaxOnMap + GetRandomValue(-1, 1), 1, 4);

    float fadeScale = vary(1.0f, worldVariance);
    config_.bgmAltitudeFadeStart = std::max(0.0f, baseConfig.bgmAltitudeFadeStart * fadeScale);
    config_.bgmAltitudeFadeEnd = std::max(config_.bgmAltitudeFadeStart + 4.0f, baseConfig.bgmAltitudeFadeEnd * fadeScale);
}

void Game::TriggerScavengerUfo(Vector3 origin) {
    (void)origin;
    if (!config_.ufoEnabled) return;
    if (scavengerUfo_.triggeredThisRun) return;
    scavengerUfo_.triggeredThisRun = true;
    scavengerUfo_.state = ScavengerUfoState::Pending;
    scavengerUfo_.spawnTimer = config_.ufoSpawnDelay;
    scavengerUfo_.world = 0;
    eventText_ = "SIGNAL LOST";
    eventTextTimer_ = 3.0f;
    SpawnShockwave(camera_.position, 4.0f, Color{90, 220, 255, 255});
}

void Game::SpawnScavengerUfo() {
    scavengerUfo_.state = ScavengerUfoState::Active;
    scavengerUfo_.health = config_.ufoHealth;
    scavengerUfo_.maxHealth = config_.ufoHealth;
    scavengerUfo_.velocity = {};
    scavengerUfo_.attackTimer = 1.0f;
    scavengerUfo_.altitudeTarget = config_.ufoHoverAltitudeMax;
    scavengerUfo_.altitudeTimer = 2.0f;
    scavengerUfo_.escapeTimer = 0.0f;
    scavengerUfo_.fallSpeed = 0.0f;
    scavengerUfo_.targetPickupIndex = -1;
    scavengerUfo_.collected = 0;
    scavengerUfo_.tractoring = false;
    scavengerUfo_.world = 0;

    if (IsSphericalMap()) {
        Vector3 dir = SafeNormalize(camera_.position, Vector3{1.0f, 0.0f, 0.0f});
        scavengerUfo_.position = SphericalSurfacePoint(dir, config_.ufoHoverAltitudeMax, 0);
    } else {
        Vector3 flat = {camera_.position.x, 0.0f, camera_.position.z};
        float limit = IsSquareMap() ? squareHalfExtent_ * 0.45f : arenaRadius_ * 0.45f;
        if (Vector3Length(flat) > limit && Vector3Length(flat) > 0.001f) {
            flat = Vector3Scale(Vector3Normalize(flat), limit);
        }
        Vector3 up = FlatUpForWorld(0);
        scavengerUfo_.position = Vector3Add(Vector3{flat.x, FlatGroundYForWorld(0), flat.z}, Vector3Scale(up, config_.ufoHoverAltitudeMax));
    }

    PlaySfxAt(sfxBossSpawn_, scavengerUfo_.position, 150.0f, 1.0f);
    SpawnShockwave(scavengerUfo_.position, 7.0f, Color{100, 230, 255, 255});
    SpawnHitBurst(scavengerUfo_.position, Color{140, 100, 255, 255}, 80);
    eventText_ = "SCAVENGER UFO";
    eventTextTimer_ = 4.0f;
}

void Game::FireUfoOrb(Vector3 target) {
    Vector3 up = UpForWorldAt(scavengerUfo_.position, 0);
    Vector3 origin = Vector3Subtract(scavengerUfo_.position, Vector3Scale(up, 0.5f));
    Vector3 dir = SafeNormalize(Vector3Subtract(target, origin), Vector3Scale(up, -1.0f));
    FireHomingShot(origin, dir, config_.ufoOrbSpeed, config_.ufoOrbTurnRate, config_.ufoOrbLifetime,
        config_.ufoOrbDamage, Color{120, 235, 255, 255}, ProjectileOwner::Enemy, 0);
    if (!projectiles_.empty()) {
        Projectile& orb = projectiles_.back();
        orb.kind = ProjectileKind::UfoOrb;
        orb.radius = 0.32f;
        orb.maxRadius = 0.32f;
        orb.damage = config_.ufoOrbDamage;
    }
    PlaySfxAt(sfxLaserPlasma_, origin, 90.0f, 0.8f);
}

void Game::UpdateUfoPilot(float dt) {
    if (!UfoPilotActive()) return;
    if (IsKeyPressed(KEY_F)) {
        ExitScavengerUfo();
        return;
    }
    if (ufoTravelState_ == UfoTravelState::Inactive && IsKeyDown(KEY_H)) {
        BeginUfoHyperspaceCharge();
        return;
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        ufoPilotWeapon_ = ufoPilotWeapon_ == UfoPilotWeapon::Orb ? UfoPilotWeapon::Tractor : UfoPilotWeapon::Orb;
        eventText_ = ufoPilotWeapon_ == UfoPilotWeapon::Orb ? "UFO ORB" : "TRACTOR";
        eventTextTimer_ = 0.8f;
        PlaySfx(sfxWeaponModeSwitch_);
        if (ufoPilotWeapon_ == UfoPilotWeapon::Orb) {
            StopSfx(sfxUfoTractor_);
        } else {
            StopSfx(sfxLaserBeam_);
        }
    }
    if (ufoPilotWeapon_ == UfoPilotWeapon::Orb && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        ufoOrbMode_ = ufoOrbMode_ == UfoOrbMode::Projectile ? UfoOrbMode::Laser : UfoOrbMode::Projectile;
        eventText_ = ufoOrbMode_ == UfoOrbMode::Laser ? "ORB LASER" : "ORB SHOT";
        eventTextTimer_ = 0.8f;
        PlaySfx(sfxWeaponModeSwitch_);
        StopSfx(sfxLaserBeam_);
    }
    if (config_.ufoDebugLocalJumpEnabled && IsKeyPressed(KEY_J)) {
        TeleportPilotedUfo();
    }

    scavengerUfo_.pilotOrbTimer = std::max(0.0f, scavengerUfo_.pilotOrbTimer - dt);
    scavengerUfo_.pilotJumpCooldown = std::max(0.0f, scavengerUfo_.pilotJumpCooldown - dt);
    ufoEssenceTransferTimer_ = std::max(0.0f, ufoEssenceTransferTimer_ - dt);

    auto transferReady = [&](int key) {
        if (IsKeyPressed(key)) {
            ufoEssenceTransferTimer_ = 0.0f;
            return true;
        }
        return IsKeyDown(key) && ufoEssenceTransferTimer_ <= 0.0f;
    };
    auto markTransfer = [&]() {
        ufoEssenceTransferTimer_ = 0.12f;
    };

    if (transferReady(KEY_Q)) {
        if (essence_ > 0 && scavengerUfo_.pilotEssence < config_.ufoPilotEssenceMax) {
            --essence_;
            ++scavengerUfo_.pilotEssence;
            ++scavengerUfo_.pilotTotalCollected;
            eventText_ = "ESSENCE STORED";
            eventTextTimer_ = 0.55f;
            PlaySfx(sfxEssenceConsume_);
            markTransfer();
        } else if (IsKeyPressed(KEY_Q)) {
            eventText_ = scavengerUfo_.pilotEssence >= config_.ufoPilotEssenceMax ? "UFO ESSENCE FULL" : "NO PLAYER ESSENCE";
            eventTextTimer_ = 0.55f;
        }
    } else if (transferReady(KEY_E)) {
        if (scavengerUfo_.pilotEssence > 0) {
            --scavengerUfo_.pilotEssence;
            ++essence_;
            eventText_ = "ESSENCE EXTRACTED";
            eventTextTimer_ = 0.55f;
            PlaySfx(sfxEssence_);
            markTransfer();
        } else if (IsKeyPressed(KEY_E)) {
            eventText_ = "UFO ESSENCE EMPTY";
            eventTextTimer_ = 0.55f;
        }
    } else if (!IsKeyDown(KEY_Q) && !IsKeyDown(KEY_E)) {
        ufoEssenceTransferTimer_ = 0.0f;
    }

    Vector3 up = UpForWorldAt(scavengerUfo_.position, 0);
    Vector3 forward = PlayerForward();
    Vector3 tangentForward = IsSphericalMap()
        ? ProjectOnSphericalTangent(forward, up)
        : Vector3{forward.x, 0.0f, forward.z};
    tangentForward = SafeNormalize(tangentForward, SafeNormalize(Vector3CrossProduct(Vector3{1.0f, 0.0f, 0.0f}, up), Vector3{0.0f, 0.0f, 1.0f}));
    Vector3 right = SafeNormalize(Vector3CrossProduct(tangentForward, up), PlayerRight());

    Vector3 wish = Vector3Zero();
    if (IsKeyDown(KEY_W)) wish = Vector3Add(wish, tangentForward);
    if (IsKeyDown(KEY_S)) wish = Vector3Subtract(wish, tangentForward);
    if (IsKeyDown(KEY_D)) wish = Vector3Add(wish, right);
    if (IsKeyDown(KEY_A)) wish = Vector3Subtract(wish, right);
    if (Vector3Length(wish) > 0.001f) wish = Vector3Normalize(wish);

    float moveSpeed = config_.ufoPilotMoveSpeed;
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) moveSpeed *= config_.ufoPilotSprintMult;
    Vector3 desiredHoriz = Vector3Scale(wish, moveSpeed);
    Vector3 horizVel = IsSphericalMap()
        ? ProjectOnSphericalTangent(scavengerUfo_.velocity, up)
        : Vector3{scavengerUfo_.velocity.x, 0.0f, scavengerUfo_.velocity.z};
    horizVel = Vector3Add(horizVel, Vector3Scale(Vector3Subtract(desiredHoriz, horizVel), std::clamp(dt * 4.0f, 0.0f, 1.0f)));

    float verticalInput = 0.0f;
    if (IsKeyDown(KEY_SPACE)) verticalInput += 1.0f;
    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) verticalInput -= 1.0f;
    scavengerUfo_.altitudeTarget += verticalInput * config_.ufoPilotVerticalSpeed * dt;
    scavengerUfo_.altitudeTarget = std::clamp(scavengerUfo_.altitudeTarget, 2.0f, config_.ufoHoverAltitudeMax + 30.0f);

    auto altitudeAt = [&](Vector3 p) {
        if (IsSphericalMap()) return SphericalAltitudeAt(p, 0);
        return std::abs(Vector3DotProduct(Vector3Subtract(p, Vector3{0.0f, FlatGroundYForWorld(0), 0.0f}), FlatUpForWorld(0)));
    };
    float altitude = altitudeAt(scavengerUfo_.position);
    float verticalSpeed = Vector3DotProduct(scavengerUfo_.velocity, up);
    float vertical = verticalSpeed + ((scavengerUfo_.altitudeTarget - altitude) * config_.ufoPilotHoverStrength
        - verticalSpeed * config_.ufoPilotHoverDamping) * dt;

    scavengerUfo_.velocity = Vector3Add(horizVel, Vector3Scale(up, vertical));
    scavengerUfo_.position = Vector3Add(scavengerUfo_.position, Vector3Scale(scavengerUfo_.velocity, dt));

    if (IsSphericalMap()) {
        float clampedAlt = std::clamp(altitudeAt(scavengerUfo_.position), 1.0f, config_.ufoHoverAltitudeMax + 34.0f);
        scavengerUfo_.position = SphericalSurfacePoint(scavengerUfo_.position, clampedAlt, 0);
        scavengerUfo_.velocity = ProjectOnSphericalTangent(scavengerUfo_.velocity, SphericalUpAt(scavengerUfo_.position, 0));
        scavengerUfo_.velocity = Vector3Add(scavengerUfo_.velocity, Vector3Scale(SphericalUpAt(scavengerUfo_.position, 0), vertical));
    } else if (IsSquareMap()) {
        float limit = squareHalfExtent_ - 3.0f;
        scavengerUfo_.position.x = std::clamp(scavengerUfo_.position.x, -limit, limit);
        scavengerUfo_.position.z = std::clamp(scavengerUfo_.position.z, -limit, limit);
    } else {
        Vector3 flat = {scavengerUfo_.position.x, 0.0f, scavengerUfo_.position.z};
        float limit = arenaRadius_ - 3.0f;
        if (Vector3Length(flat) > limit && Vector3Length(flat) > 0.001f) {
            flat = Vector3Scale(Vector3Normalize(flat), limit);
            scavengerUfo_.position.x = flat.x;
            scavengerUfo_.position.z = flat.z;
        }
    }

    camera_.position = scavengerUfo_.position;
    camera_.up = UpForWorldAt(camera_.position, 0);
    camera_.target = Vector3Add(camera_.position, PlayerForward());
    playerWorld_ = 0;

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (ufoPilotWeapon_ == UfoPilotWeapon::Orb) {
            StopSfx(sfxUfoTractor_);
            if (ufoOrbMode_ == UfoOrbMode::Laser) {
                UpdatePilotedUfoOrbLaser(dt);
            } else {
                StopSfx(sfxLaserBeam_);
                FirePilotedUfoOrb();
            }
        } else {
            StopSfx(sfxLaserBeam_);
            UpdatePilotedUfoTractor(dt);
        }
    } else {
        StopSfx(sfxUfoTractor_);
        StopSfx(sfxLaserBeam_);
    }
}

void Game::FirePilotedUfoOrb() {
    if (!UfoPilotActive() || scavengerUfo_.pilotOrbTimer > 0.0f) return;
    Vector3 up = UpForWorldAt(scavengerUfo_.position, 0);
    Vector3 origin = Vector3Add(scavengerUfo_.position, Vector3Scale(up, -0.45f));
    Vector3 dir = SafeNormalize(PlayerForward(), Vector3Scale(up, -1.0f));
    float orbDamage = config_.ufoPilotOrbDamage;
    FireHomingShot(origin, dir, config_.ufoOrbSpeed, config_.ufoOrbTurnRate, config_.ufoOrbLifetime,
        orbDamage, Color{120, 235, 255, 255}, ProjectileOwner::Player, 0);
    if (!projectiles_.empty()) {
        Projectile& orb = projectiles_.back();
        orb.kind = ProjectileKind::UfoOrb;
        orb.radius = 0.32f;
        orb.maxRadius = 0.32f;
        orb.damage = orbDamage;
    }
    scavengerUfo_.pilotOrbTimer = config_.ufoPilotOrbInterval;
    PlaySfxAt(sfxLaserPlasma_, origin, 90.0f, 0.72f);
}

void Game::UpdatePilotedUfoOrbLaser(float dt) {
    if (!UfoPilotActive()) return;
    Vector3 up = UpForWorldAt(scavengerUfo_.position, 0);
    Vector3 origin = Vector3Add(scavengerUfo_.position, Vector3Scale(up, -0.42f));
    Vector3 dir = SafeNormalize(PlayerForward(), Vector3Scale(up, -1.0f));
    Vector3 end = Vector3Add(origin, Vector3Scale(dir, config_.ufoPilotOrbLaserRange));

    beams_.push_back(Beam{
        origin,
        end,
        0.08f,
        0.08f,
        config_.ufoPilotOrbLaserWidth,
        0.85f,
        Color{120, 235, 255, 255},
        config_.ufoPilotOrbLaserDamage,
        0.0f,
        0,
        false
    });
    UpdateLoopingSfxAt(sfxLaserBeam_, origin, 96.0f, 0.72f);
    cameraShake_ = std::min(0.32f, cameraShake_ + dt * 0.16f);
}

void Game::UpdatePilotedUfoTractor(float dt) {
    if (!UfoPilotActive()) return;
    Vector3 origin = Vector3Add(scavengerUfo_.position, Vector3Scale(UpForWorldAt(scavengerUfo_.position, 0), -0.6f));
    Vector3 dir = SafeNormalize(PlayerForward(), Vector3{0.0f, 0.0f, 1.0f});
    Vector3 end = Vector3Add(origin, Vector3Scale(dir, config_.ufoPilotTractorRange));
    UpdateLoopingSfxAt(sfxUfoTractor_, origin, 92.0f, 0.78f);

    auto closestPointOnBeam = [&](Vector3 p, float& tOut) {
        Vector3 toPoint = Vector3Subtract(p, origin);
        float t = std::clamp(Vector3DotProduct(toPoint, dir), 0.0f, config_.ufoPilotTractorRange);
        tOut = t;
        return Vector3Add(origin, Vector3Scale(dir, t));
    };

    for (size_t i = 0; i < pickups_.size();) {
        if (pickups_[i].type != PickupType::Essence) {
            ++i;
            continue;
        }
        float t = 0.0f;
        Vector3 closest = closestPointOnBeam(pickups_[i].position, t);
        float dist = Vector3Distance(pickups_[i].position, closest);
        if (dist <= 3.2f) {
            Vector3 toUfo = Vector3Subtract(scavengerUfo_.position, pickups_[i].position);
            Vector3 pull = SafeNormalize(Vector3Add(Vector3Scale(Vector3Subtract(closest, pickups_[i].position), 0.65f), toUfo), dir);
            pickups_[i].velocity = Vector3Add(Vector3Scale(pickups_[i].velocity, std::max(0.0f, 1.0f - dt * 8.0f)),
                Vector3Scale(pull, config_.ufoPilotTractorStrength * dt));
            pickups_[i].position = Vector3Add(pickups_[i].position, Vector3Scale(pickups_[i].velocity, dt));
            pickups_[i].gravityScale = 0.0f;
            if (Vector3Distance(pickups_[i].position, scavengerUfo_.position) <= config_.ufoCollectRange + 0.8f) {
                pickups_[i] = pickups_.back();
                pickups_.pop_back();
                scavengerUfo_.pilotEssence = std::min(config_.ufoPilotEssenceMax, scavengerUfo_.pilotEssence + 1);
                scavengerUfo_.pilotTotalCollected++;
                PlaySfxAt(sfxEssence_, scavengerUfo_.position, 80.0f, 0.75f);
                SpawnHitBurst(scavengerUfo_.position, Color{150, 245, 255, 255}, 14);
                continue;
            }
        }
        ++i;
    }

    for (Enemy& enemy : enemies_) {
        if (enemy.world != 0) continue;
        Vector3 ep = BodyPosition(enemy.body);
        float t = 0.0f;
        Vector3 closest = closestPointOnBeam(ep, t);
        float dist = Vector3Distance(ep, closest);
        if (dist > enemy.radius + 3.0f) continue;
        Vector3 pullDir = SafeNormalize(Vector3Subtract(closest, ep), dir);
        enemy.externalVelocity = Vector3Add(enemy.externalVelocity, Vector3Scale(pullDir, config_.ufoPilotTractorStrength * dt * 3.0f));
    }

    beams_.push_back(Beam{origin, end, 0.08f, 0.08f, 0.18f, 0.0f, Color{120, 235, 255, 255}, 0.0f, 0.0f, 0});
}

void Game::UpdateScavengerUfo(float dt) {
    if (scavengerUfo_.state == ScavengerUfoState::Inactive
        || scavengerUfo_.state == ScavengerUfoState::Gone) {
        StopSfx(sfxUfoTractor_);
        return;
    }

    if (scavengerUfo_.state == ScavengerUfoState::DefeatedLanded) {
        StopSfx(sfxUfoTractor_);
        scavengerUfo_.velocity = {};
        scavengerUfo_.fallSpeed = 0.0f;
        scavengerUfo_.tractoring = false;
        scavengerUfo_.targetPickupIndex = -1;
        return;
    }

    if (scavengerUfo_.state == ScavengerUfoState::PlayerPiloted) {
        return;
    }

    if (scavengerUfo_.state == ScavengerUfoState::Pending) {
        StopSfx(sfxUfoTractor_);
        scavengerUfo_.spawnTimer -= dt;
        if (scavengerUfo_.spawnTimer <= 0.0f) {
            SpawnScavengerUfo();
        }
        return;
    }

    scavengerUfo_.bobTimer += dt;
    Vector3 up = UpForWorldAt(scavengerUfo_.position, 0);

    auto altitudeAt = [&](Vector3 p) {
        if (IsSphericalMap()) return SphericalAltitudeAt(p, 0);
        return std::abs(Vector3DotProduct(Vector3Subtract(p, Vector3{0.0f, FlatGroundYForWorld(0), 0.0f}), FlatUpForWorld(0)));
    };
    auto surfacePoint = [&](Vector3 p, float altitude) {
        if (IsSphericalMap()) return SphericalSurfacePoint(p, altitude, 0);
        Vector3 flatUp = FlatUpForWorld(0);
        return Vector3{p.x, FlatGroundYForWorld(0) + flatUp.y * altitude, p.z};
    };

    if (scavengerUfo_.state == ScavengerUfoState::ParkedHover) {
        StopSfx(sfxUfoTractor_);
        float alt = altitudeAt(scavengerUfo_.position);
        float verticalSpeed = Vector3DotProduct(scavengerUfo_.velocity, up);
        float vertical = verticalSpeed + ((scavengerUfo_.altitudeTarget - alt) * config_.ufoPilotHoverStrength
            - verticalSpeed * config_.ufoPilotHoverDamping) * dt;
        scavengerUfo_.velocity = Vector3Scale(up, vertical);
        scavengerUfo_.position = Vector3Add(scavengerUfo_.position, Vector3Scale(scavengerUfo_.velocity, dt));
        if (IsSphericalMap()) {
            float maxParkAltitude = std::max(config_.ufoHoverAltitudeMax + 34.0f, scavengerUfo_.altitudeTarget + 2.0f);
            float clampedAlt = std::clamp(altitudeAt(scavengerUfo_.position), 1.0f, maxParkAltitude);
            scavengerUfo_.position = SphericalSurfacePoint(scavengerUfo_.position, clampedAlt, 0);
        }
        return;
    }

    if (scavengerUfo_.state == ScavengerUfoState::DefeatedFalling) {
        StopSfx(sfxUfoTractor_);
        scavengerUfo_.fallSpeed += 2.8f * dt;
        scavengerUfo_.position = Vector3Subtract(scavengerUfo_.position, Vector3Scale(up, scavengerUfo_.fallSpeed * dt));
        if (RandomFloat(0.0f, 1.0f) < 0.45f) {
            SpawnHitBurst(scavengerUfo_.position, Color{120, 180, 190, 255}, 3);
        }
        if (altitudeAt(scavengerUfo_.position) <= 1.2f) {
            scavengerUfo_.position = surfacePoint(scavengerUfo_.position, 1.0f);
            scavengerUfo_.fallSpeed = 0.0f;
            scavengerUfo_.velocity = {};
            eventText_ = "SCAVENGER DOWN";
            eventTextTimer_ = 3.0f;
            score_ += 1200;
            scavengerUfo_.state = ScavengerUfoState::DefeatedLanded;
        }
        return;
    }

    if (scavengerUfo_.state == ScavengerUfoState::Escaping) {
        StopSfx(sfxUfoTractor_);
        scavengerUfo_.escapeTimer += dt;
        scavengerUfo_.altitudeTarget = config_.ufoHoverAltitudeMax + 22.0f;
        if (scavengerUfo_.escapeTimer > 3.2f || altitudeAt(scavengerUfo_.position) >= scavengerUfo_.altitudeTarget - 1.0f) {
            PlaySfxAt(sfxBossPhase_, scavengerUfo_.position, 150.0f, 1.0f);
            SpawnShockwave(scavengerUfo_.position, 9.0f, Color{100, 235, 255, 255});
            eventText_ = "SCAVENGER ESCAPED";
            eventTextTimer_ = 3.0f;
            scavengerUfo_.state = ScavengerUfoState::Gone;
            return;
        }
    }

    int bestPickup = -1;
    float bestDist = INFINITY;
    for (int i = 0; i < static_cast<int>(pickups_.size()); ++i) {
        if (pickups_[i].type != PickupType::Essence) continue;
        float d = Vector3Distance(scavengerUfo_.position, pickups_[i].position);
        if (d < bestDist) {
            bestDist = d;
            bestPickup = i;
        }
    }
    scavengerUfo_.targetPickupIndex = bestPickup;
    scavengerUfo_.tractoring = false;

    Vector3 target = scavengerUfo_.position;
    bool hasEssence = bestPickup >= 0;
    if (hasEssence) {
        target = pickups_[bestPickup].position;
        if (bestDist <= config_.ufoTractorRange) {
            scavengerUfo_.tractoring = true;
            UpdateLoopingSfxAt(sfxUfoTractor_, scavengerUfo_.position, 110.0f, 0.82f);
            scavengerUfo_.altitudeTarget = config_.ufoTractorAltitude;
            Vector3 toUfo = Vector3Subtract(scavengerUfo_.position, pickups_[bestPickup].position);
            float pullDist = Vector3Length(toUfo);
            if (pullDist > 0.001f) {
                Vector3 pullDir = Vector3Scale(toUfo, 1.0f / pullDist);
                pickups_[bestPickup].velocity = Vector3Add(
                    Vector3Scale(pickups_[bestPickup].velocity, std::max(0.0f, 1.0f - dt * 6.0f)),
                    Vector3Scale(pullDir, config_.ufoTractorStrength * dt));
                pickups_[bestPickup].position = Vector3Add(pickups_[bestPickup].position, Vector3Scale(pickups_[bestPickup].velocity, dt));
                pickups_[bestPickup].gravityScale = 0.0f;
            }
            if (pullDist <= config_.ufoCollectRange) {
                pickups_[bestPickup] = pickups_.back();
                pickups_.pop_back();
                scavengerUfo_.collected++;
                SpawnHitBurst(scavengerUfo_.position, Color{160, 245, 255, 255}, 18);
                eventText_ = TextFormat("ESSENCE STOLEN %d/%d", scavengerUfo_.collected, config_.ufoCollectRequired);
                eventTextTimer_ = 1.5f;
                if (scavengerUfo_.collected >= config_.ufoCollectRequired) {
                    scavengerUfo_.state = ScavengerUfoState::Escaping;
                    scavengerUfo_.escapeTimer = 0.0f;
                }
            }
        }
    } else if (playerWorld_ == 0) {
        target = camera_.position;
    } else if (IsSphericalMap()) {
        target = SphericalSurfacePoint(scavengerUfo_.position, config_.ufoHoverAltitudeMax, 0);
    } else {
        target = Vector3{0.0f, FlatGroundYForWorld(0), 0.0f};
    }
    if (!scavengerUfo_.tractoring) {
        StopSfx(sfxUfoTractor_);
    }

    scavengerUfo_.altitudeTimer -= dt;
    if (scavengerUfo_.altitudeTimer <= 0.0f && !scavengerUfo_.tractoring && scavengerUfo_.state == ScavengerUfoState::Active) {
        scavengerUfo_.altitudeTarget = RandomFloat(config_.ufoHoverAltitudeMin, config_.ufoHoverAltitudeMax);
        scavengerUfo_.altitudeTimer = RandomFloat(2.0f, 4.5f);
    }

    up = UpForWorldAt(scavengerUfo_.position, 0);
    Vector3 toTarget = Vector3Subtract(target, scavengerUfo_.position);
    Vector3 tangent = IsSphericalMap() ? ProjectOnSphericalTangent(toTarget, up) : Vector3{toTarget.x, 0.0f, toTarget.z};
    Vector3 desiredHoriz = Vector3Zero();
    float tangentLen = Vector3Length(tangent);
    if (tangentLen > 0.4f) {
        desiredHoriz = Vector3Scale(Vector3Normalize(tangent), config_.ufoMoveSpeed * std::min(1.0f, tangentLen / 8.0f));
    }
    Vector3 horizVel = IsSphericalMap() ? ProjectOnSphericalTangent(scavengerUfo_.velocity, up) : Vector3{scavengerUfo_.velocity.x, 0.0f, scavengerUfo_.velocity.z};
    horizVel = Vector3Add(horizVel, Vector3Scale(Vector3Subtract(desiredHoriz, horizVel), std::clamp(dt * 2.8f, 0.0f, 1.0f)));

    float alt = altitudeAt(scavengerUfo_.position);
    float verticalSpeed = Vector3DotProduct(scavengerUfo_.velocity, up);
    float altError = scavengerUfo_.altitudeTarget - alt;
    float newVertical = verticalSpeed + (altError * 10.0f - verticalSpeed * 4.5f) * dt;
    scavengerUfo_.velocity = Vector3Add(horizVel, Vector3Scale(up, newVertical));
    scavengerUfo_.position = Vector3Add(scavengerUfo_.position, Vector3Scale(scavengerUfo_.velocity, dt));

    if (IsSphericalMap()) {
        float clampedAlt = std::clamp(altitudeAt(scavengerUfo_.position), 1.0f, config_.ufoHoverAltitudeMax + 26.0f);
        scavengerUfo_.position = SphericalSurfacePoint(scavengerUfo_.position, clampedAlt, 0);
    } else if (IsSquareMap()) {
        float limit = squareHalfExtent_ - 3.0f;
        scavengerUfo_.position.x = std::clamp(scavengerUfo_.position.x, -limit, limit);
        scavengerUfo_.position.z = std::clamp(scavengerUfo_.position.z, -limit, limit);
    } else {
        Vector3 flat = {scavengerUfo_.position.x, 0.0f, scavengerUfo_.position.z};
        float limit = arenaRadius_ - 3.0f;
        if (Vector3Length(flat) > limit && Vector3Length(flat) > 0.001f) {
            flat = Vector3Scale(Vector3Normalize(flat), limit);
            scavengerUfo_.position.x = flat.x;
            scavengerUfo_.position.z = flat.z;
        }
    }

    bool threatNearby = false;
    if (playerWorld_ == 0 && Vector3Distance(camera_.position, scavengerUfo_.position) <= config_.ufoAttackRange) {
        threatNearby = true;
        target = camera_.position;
    }
    if (!threatNearby) {
        for (const Enemy& enemy : enemies_) {
            if (enemy.world != 0) continue;
            Vector3 ep = BodyPosition(enemy.body);
            if (Vector3Distance(ep, scavengerUfo_.position) <= config_.ufoAttackRange * 0.65f) {
                target = ep;
                threatNearby = true;
                break;
            }
        }
    }
    bool shouldAttack = threatNearby || !hasEssence;
    scavengerUfo_.attackTimer -= dt;
    if (scavengerUfo_.state == ScavengerUfoState::Active && shouldAttack && scavengerUfo_.attackTimer <= 0.0f) {
        FireUfoOrb(target);
        scavengerUfo_.attackTimer = config_.ufoAttackInterval;
    }
}

void Game::DamageScavengerUfo(float damage, Vector3 hitPosition, Color color) {
    if (!ScavengerUfoDamageable() || damage <= 0.0f) return;
    scavengerUfo_.health -= damage;
    totalDamageDealt_ += damage;
    PlayEnemyHitSfx(scavengerUfo_.position);
    SpawnHitBurst(hitPosition, color, 10);
    if (scavengerUfo_.health <= 0.0f) {
        StopSfx(sfxUfoTractor_);
        scavengerUfo_.health = 0.0f;
        int recoveredEssence = config_.ufoBaseEssence + scavengerUfo_.collected;
        scavengerUfo_.pilotEssence = std::clamp(recoveredEssence, 0, config_.ufoPilotEssenceMax);
        scavengerUfo_.pilotTotalCollected = scavengerUfo_.pilotEssence;
        scavengerUfo_.state = ScavengerUfoState::DefeatedFalling;
        scavengerUfo_.fallSpeed = 0.0f;
        scavengerUfo_.velocity = {};
        PlaySfxAt(sfxBossPhase_, scavengerUfo_.position, 150.0f, 1.0f);
        SpawnShockwave(scavengerUfo_.position, 6.0f, Color{120, 230, 255, 255});
        eventText_ = "UFO HIT";
        eventTextTimer_ = 2.0f;
    }
}

void Game::DamageScavengerUfoInRadius(Vector3 position, float radius, float damage, Color color) {
    if (!ScavengerUfoDamageable()) return;
    float dist = Vector3Distance(scavengerUfo_.position, position);
    if (dist > radius + 3.0f) return;
    float falloff = 1.0f - std::clamp(dist / std::max(0.001f, radius), 0.0f, 1.0f);
    DamageScavengerUfo(damage * (0.35f + falloff * 0.65f), scavengerUfo_.position, color);
}
