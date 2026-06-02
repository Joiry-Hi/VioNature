#include "Game.h"
#include "GameMath.h"

#include <algorithm>
#include <cmath>

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
        enemy.burstTimer = std::max(0.0f, enemy.burstTimer - dt);
        float speed = enemy.speed + std::min(4.0f, survivalTime_ * 0.04f);
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
                Vector3 leap = Vector3Add(Vector3Scale(direction, config_.pouncerLeapSpeed), Vector3Scale(enemyUp, 4.8f));
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
                // Slamming down
                enemy.actionTimer -= dt;
                bool hitGround = IsSphericalMap()
                    ? SphericalAltitudeAt(position, enemy.world) <= targetAlt + enemy.radius * 0.35f
                    : (enemy.world == 0 ? position.y <= flatEnemySlamY : position.y >= flatEnemySlamY);
                if (hitGround || enemy.actionTimer <= 0.0f) {
                    Vector3 impactPos = IsSphericalMap()
                        ? SphericalSurfacePoint(position, targetAlt, enemy.world)
                        : Vector3{position.x, FlatGroundYForWorld(enemy.world) + FlatUpForWorld(enemy.world).y * 0.6f, position.z};
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
            velocity = Vector3Add(Vector3Add(Vector3Scale(direction, speed), Vector3Scale(enemyUp, radialCorrection)), enemy.externalVelocity);
        } else {
            float verticalVelocity = 0.0f;
            if (enemy.type == EnemyType::Pouncer || enemy.type == EnemyType::SlimeKing) {
                verticalVelocity = physics_.Bodies().GetLinearVelocity(enemy.body).GetY();
            } else {
                float targetHeight = enemy.type == EnemyType::Harrier ? config_.harrierTargetHeight + std::sin(enemy.bobTimer * 2.6f) * 1.2f : enemy.type == EnemyType::Blinker ? 1.0f : enemy.type == EnemyType::Wisp || enemy.type == EnemyType::Spitter ? 1.35f : enemy.type == EnemyType::Boss ? 2.2f : enemy.type == EnemyType::Duelist ? 1.2f : 0.8f;
                float targetY = FlatGroundYForWorld(enemy.world) + FlatUpForWorld(enemy.world).y * targetHeight;
                verticalVelocity = std::clamp((targetY - position.y) * 7.0f, -12.0f, 8.0f);
            }
            velocity = Vector3{
                direction.x * speed + enemy.externalVelocity.x,
                verticalVelocity + enemy.externalVelocity.y,
                direction.z * speed + enemy.externalVelocity.z
            };
        }
        enemy.externalVelocity = Vector3Scale(enemy.externalVelocity, std::pow(0.12f, dt));
        physics_.Bodies().SetLinearVelocity(enemy.body, ToJoltVelocity(velocity));

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

        if (sameWorldAsPlayer
            && EnemyTouchesPlayer(position, enemy.radius)
            && enemy.type != EnemyType::Dummy
            && enemy.type != EnemyType::DummyBoss) {
            ApplyPlayerHit(player, Color{255, 35, 25, 255});
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

    spawnInterval_ = waveIndex_ == 1 ? 1.85f : waveIndex_ == 2 ? 1.45f : waveIndex_ == 3 ? 1.05f : 0.72f;
    spawnTimer_ -= dt;
    if (!config_.bossRushMode && spawnTimer_ <= 0.0f) {
        SpawnEnemy();
        spawnTimer_ = spawnInterval_;
    }

    if (!config_.bossRushMode && !wispSurgeDone_ && survivalTime_ >= 25.0f) {
        for (int i = 0; i < 5; ++i) {
            SpawnEnemyOfType(EnemyType::Wisp);
        }
        eventText_ = "WISP SURGE";
        eventTextTimer_ = 3.0f;
        wispSurgeDone_ = true;
    }

    if (!config_.bossRushMode && !spitterAmbushDone_ && survivalTime_ >= 52.0f) {
        for (int i = 0; i < 3; ++i) {
            SpawnEnemyOfType(EnemyType::Spitter);
        }
        eventText_ = "SPITTERS";
        eventTextTimer_ = 3.0f;
        spitterAmbushDone_ = true;
    }

    if (!config_.bossRushMode && !pouncerRushDone_ && survivalTime_ >= 82.0f) {
        for (int i = 0; i < 4; ++i) {
            SpawnEnemyOfType(EnemyType::Pouncer);
        }
        eventText_ = "POUNCER RUSH";
        eventTextTimer_ = 3.0f;
        pouncerRushDone_ = true;
    }

    if (!bossSpawned_ && survivalTime_ >= config_.bossSpawnTime) {
        SpawnEnemyOfType(EnemyType::Boss);
        if (!config_.bossRushMode) {
            for (int i = 0; i < 3; ++i) {
                SpawnEnemyOfType(i % 2 == 0 ? EnemyType::Wisp : EnemyType::Spitter);
            }
        }
        eventText_ = "GEOMETRY LORD";
        eventTextTimer_ = 4.0f;
        bossSpawned_ = true;
    }

    if (!slimeKingSpawned_ && survivalTime_ >= config_.slimeKingSpawnTime) {
        SpawnEnemyOfType(EnemyType::SlimeKing);
        eventText_ = "SLIME KING";
        eventTextTimer_ = 4.0f;
        slimeKingSpawned_ = true;
    }

    if (!bethlehemSpawned_ && survivalTime_ >= config_.bethlehemSpawnTime) {
        SpawnBethlehem();
        eventText_ = "STAR OF BETHLEHEM";
        eventTextTimer_ = 4.0f;
        bethlehemSpawned_ = true;
    }

    if (!config_.bossRushMode && waveIndex_ >= 4 && survivalTime_ >= nextMixedEventTime_) {
        int eventRoll = GetRandomValue(0, 99);
        if (eventRoll < 34) {
            for (int i = 0; i < 3; ++i) {
                SpawnEnemyOfType(EnemyType::Harrier);
            }
            eventText_ = "HARRIER SWARM";
        } else if (eventRoll < 67) {
            for (int i = 0; i < 2; ++i) {
                SpawnEnemyOfType(EnemyType::Blinker);
            }
            eventText_ = "BLINK STRIKE";
        } else {
            SpawnEnemyOfType(EnemyType::Spitter);
            SpawnEnemyOfType(EnemyType::Pouncer);
            SpawnEnemyOfType(EnemyType::Harrier);
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
    if (IsSphericalMap()) {
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
        if (distance > 18.0f) {
            enemy.weaponSlot = roll < 24 ? 0 : roll < 44 ? 6 : roll < 65 ? 2 : roll < 84 ? 7 : 4;
        } else if (distance > 8.0f) {
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
            FireEnemyProjectile(ProjectileKind::LaserShot, origin, aimDirection, 58.0f, config_.plasmaDamage, 1.6f, 0.17f, 0.17f, Color{255, 235, 145, 255}, enemy.world);
            enemy.cooldownTimer = 0.28f / rate;
        }
    } else if (enemy.weaponSlot == 1) {
        if (Vector3Distance(position, camera_.position) < config_.heatwaveRange * 0.72f && GetRandomValue(0, 99) < 34) {
            FireDuelistHeatwave(origin, aimDirection);
            enemy.cooldownTimer = 0.5f / rate;
        } else {
            Vector3 flameDirection = Vector3Normalize(Vector3Add(aimDirection, Vector3Add(Vector3Scale(right, RandomFloat(-0.12f, 0.12f)), Vector3Scale(up, RandomFloat(-0.04f, 0.08f)))));
            FireEnemyProjectile(ProjectileKind::Flame, origin, flameDirection, RandomFloat(17.0f, 22.0f), config_.flameDamage, config_.flameLifetime, 0.14f, config_.flameMaxRadius, Color{255, 120, 34, 235}, enemy.world);
            enemy.cooldownTimer = 0.12f / rate;
        }
    } else if (enemy.weaponSlot == 2) {
        if (GetRandomValue(0, 99) < 22) {
            for (int i = 0; i < 3; ++i) {
                Vector3 direction = Vector3Normalize(Vector3Add(aimDirection, Vector3Add(Vector3Scale(right, RandomFloat(-0.18f, 0.18f)), Vector3Scale(up, RandomFloat(-0.04f, 0.1f)))));
                FireEnemyProjectile(ProjectileKind::Rocket, origin, direction, 22.0f, config_.rocketImpactDamage, 2.6f, 0.28f, 0.28f, Color{245, 190, 130, 255}, enemy.world);
            }
            enemy.cooldownTimer = 1.65f / rate;
        } else {
            FireEnemyProjectile(ProjectileKind::Rocket, origin, aimDirection, 27.0f, config_.rocketImpactDamage, 2.8f, 0.34f, 0.34f, Color{245, 190, 130, 255}, enemy.world);
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
            FireEnemyProjectile(glass ? ProjectileKind::GlassShard : ProjectileKind::Pellet, origin, direction, glass ? config_.glassShardSpeed * 0.78f : RandomFloat(42.0f, 50.0f), glass ? config_.glassShardDamage : config_.shotgunPelletDamage, glass ? config_.glassShardLingerTime : 0.58f, glass ? 0.13f : 0.1f, glass ? 0.13f : 0.1f, glass ? Color{190, 245, 255, 255} : Color{255, 205, 130, 255}, enemy.world);
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
        FireEnemyProjectile(blackHole ? ProjectileKind::BlackHoleGrenade : ProjectileKind::GravityNail, origin, aimDirection, blackHole ? 22.0f : 58.0f, blackHole ? config_.blackHoleGrenadeDamage : config_.gravityNailDamage, blackHole ? 1.55f : 1.0f, blackHole ? 0.28f : 0.15f, blackHole ? 0.28f : 0.15f, blackHole ? Color{120, 70, 190, 255} : Color{170, 200, 255, 255}, enemy.world);
        enemy.cooldownTimer = (blackHole ? 1.55f : 0.9f) / rate;
        enemy.telegraphTimer = 0.3f;
    } else if (enemy.weaponSlot == 6) {
        int burst = GetRandomValue(0, 99) < 26 ? 2 : 1;
        for (int i = 0; i < burst; ++i) {
            Vector3 direction = Vector3Normalize(Vector3Add(aimDirection, Vector3Add(Vector3Scale(right, RandomFloat(-0.06f, 0.06f)), Vector3Scale(up, RandomFloat(-0.035f, 0.045f)))));
            FireEnemyProjectile(ProjectileKind::Lance, origin, direction, config_.longinusSpearSpeed * 0.82f, config_.longinusSpearDamage, 1.15f, 0.28f, 0.28f, Color{255, 175, 70, 255}, enemy.world);
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
}
void Game::DestroyBethlehem() {
    if (!bethlehem_.active) return;
    bethlehem_.active = false;
    bethlehem_.laserPhase = BethlehemLaserPhase::Inactive;
    SpawnShockwave(bethlehem_.position, 14.0f, Color{255, 180, 60, 255});
    SpawnHitBurst(bethlehem_.position, Color{255, 220, 140, 255}, 110);
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
        float dist = DistancePointToSegment(playerPos, beamStart, beamEnd);
        if (dist <= config_.bethlehemLaserRadius + playerRadius_) {
            ApplyPlayerHit(playerPos, Color{255, 160, 40, 255}, "STAR BURNT");
        }
        if (bethlehem_.phaseTimer >= config_.bethlehemLaserDuration) {
            bethlehem_.laserPhase = BethlehemLaserPhase::Inactive;
            bethlehem_.attackTimer = config_.bethlehemLaserCooldown;
            bethlehem_.phaseTimer = 0.0f;
        }
    }
}
