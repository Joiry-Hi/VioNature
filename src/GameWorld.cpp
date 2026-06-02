#include "Game.h"
#include "GameMath.h"

#include <algorithm>
#include <cmath>

bool Game::HasWormhole() const {
    return !wormholes_.empty();
}
Vector3 Game::MirrorPosition(Vector3 position, const WormholePortal& portal) const {
    Vector3 normal = SafeNormalize(portal.mirrorNormal, Vector3{0.0f, 1.0f, 0.0f});
    float d = Vector3DotProduct(Vector3Subtract(position, portal.mirrorAnchor), normal);
    return Vector3Subtract(position, Vector3Scale(normal, 2.0f * d));
}
Vector3 Game::TeleportThroughWormhole(Vector3 position, int targetWorld, float altitude) const {
    if (wormholes_.empty()) {
        return position;
    }
    Vector3 mirrored = MirrorPosition(position, wormholes_.front());
    if (IsSphericalMap()) {
        return SphericalSurfacePoint(mirrored, altitude, targetWorld);
    }
    float signedHeight = std::max(playerHeight_, std::abs(altitude));
    mirrored.y = FlatGroundYForWorld(targetWorld) + FlatUpForWorld(targetWorld).y * signedHeight;
    return mirrored;
}
Vector3 Game::ReflectVelocityThroughWormhole(Vector3 velocity, Vector3 targetPosition, int targetWorld) const {
    if (wormholes_.empty()) {
        return velocity;
    }
    Vector3 normal = SafeNormalize(wormholes_.front().mirrorNormal, Vector3{0.0f, 1.0f, 0.0f});
    float along = Vector3DotProduct(velocity, normal);
    Vector3 reflected = Vector3Subtract(velocity, Vector3Scale(normal, along * 2.0f));
    if (IsSphericalMap()) {
        Vector3 up = SphericalUpAt(targetPosition, targetWorld);
        reflected = Vector3Add(ProjectOnSphericalTangent(reflected, up), Vector3Scale(up, std::max(0.0f, Vector3DotProduct(reflected, up))));
    } else if (targetWorld == 1) {
        reflected.y = -std::abs(reflected.y);
    } else {
        reflected.y = std::abs(reflected.y);
    }
    return reflected;
}
Vector3 Game::WormholeCenterForWorld(const WormholePortal& portal, int world) const {
    return world == 0 ? portal.frontPosition : portal.backPosition;
}
float Game::FlatGroundYForWorld(int world) const {
    return world == 0 ? 0.0f : -kFlatBackWorldDepth;
}
Vector3 Game::FlatUpForWorld(int world) const {
    return world == 0 ? Vector3{0.0f, 1.0f, 0.0f} : Vector3{0.0f, -1.0f, 0.0f};
}
Vector3 Game::UpForWorldAt(Vector3 position, int world) const {
    return IsSphericalMap() ? SphericalUpAt(position, world) : FlatUpForWorld(world);
}
bool Game::ActivateWormhole(MagicCircle& circle, int circleIndex, Vector3 octaCenter) {
    if (HasWormhole()) {
        SpawnShockwave(octaCenter, circle.radius * 0.8f, Color{170, 70, 255, 255});
        SpawnHitBurst(octaCenter, Color{190, 80, 255, 255}, 18);
        return false;
    }

    Vector3 normal = IsSphericalMap() ? SphericalUpAt(circle.position, 0) : Vector3{0.0f, 1.0f, 0.0f};
    Vector3 backCenter = MirrorPosition(octaCenter, WormholePortal{octaCenter, {}, circle.position, normal, 0.0f, 0.0f, circleIndex});
    if (IsSphericalMap()) {
        backCenter = SphericalSurfacePoint(backCenter, SphericalAltitudeAt(octaCenter, 0), 1);
        backCenter = Vector3Add(backCenter, Vector3Scale(SphericalUpAt(backCenter, 1), 1.0f + circle.radius * 0.4f));
    } else {
        backCenter = octaCenter;
        backCenter.y = FlatGroundYForWorld(1) + FlatUpForWorld(1).y * std::max(playerHeight_, std::abs(octaCenter.y));
    }

    WormholePortal portal;
    portal.frontPosition = octaCenter;
    portal.backPosition = backCenter;
    portal.mirrorAnchor = circle.position;
    portal.mirrorNormal = normal;
    portal.circleIndex = circleIndex;
    wormholes_.push_back(portal);

    circle.isWormhole = true;
    circle.life = circle.maxLife = 999999.0f;
    circle.activated = false;
    circle.activatedByLaserBeam = false;
    SpawnShockwave(octaCenter, circle.radius * 1.35f, Color{150, 45, 255, 255});
    SpawnShockwave(backCenter, circle.radius * 1.15f, Color{150, 45, 255, 210});
    SpawnHitBurst(octaCenter, Color{190, 70, 255, 255}, 32);
    eventText_ = "WORMHOLE";
    eventTextTimer_ = 2.0f;
    return true;
}
bool Game::CloseWormhole(Vector3 position) {
    if (wormholes_.empty()) {
        return false;
    }

    WormholePortal portal = wormholes_.front();
    float closeRadius = std::max(config_.wormholeTriggerRadius, config_.wormholeVisualRadius) + 0.75f;
    bool hitFront = Vector3Distance(position, portal.frontPosition) <= closeRadius;
    bool hitBack = Vector3Distance(position, portal.backPosition) <= closeRadius;
    if (!hitFront && !hitBack) {
        return false;
    }

    if (playerWorld_ == 1) {
        float altitude = IsSphericalMap() ? SphericalPlayerAltitude() : std::abs(camera_.position.y);
        camera_.position = TeleportThroughWormhole(camera_.position, 0, altitude);
        playerVelocity_ = ReflectVelocityThroughWormhole(playerVelocity_, camera_.position, 0);
        playerWorld_ = 0;
        camera_.up = UpForWorldAt(camera_.position, 0);
        camera_.target = Vector3Add(camera_.position, PlayerForward());
    }

    SpawnShockwave(portal.frontPosition, config_.wormholeVisualRadius * 2.2f, Color{255, 150, 60, 255});
    SpawnShockwave(portal.backPosition, config_.wormholeVisualRadius * 3.4f, Color{170, 70, 255, 240});
    SpawnHitBurst(portal.frontPosition, Color{255, 175, 70, 255}, 28);
    SpawnHitBurst(portal.backPosition, Color{210, 80, 255, 255}, 70);

    for (size_t i = 0; i < enemies_.size();) {
        if (enemies_[i].world == 1) {
            Vector3 enemyPosition = BodyPosition(enemies_[i].body);
            SpawnHitBurst(enemyPosition, Color{185, 60, 255, 255}, 22);
            physics_.DestroyBody(enemies_[i].body);
            enemies_[i] = enemies_.back();
            enemies_.pop_back();
            continue;
        }
        enemies_[i].world = 0;
        ++i;
    }

    for (size_t i = 0; i < projectiles_.size();) {
        if (projectiles_[i].world == 1) {
            SpawnHitBurst(BodyPosition(projectiles_[i].body), Color{180, 70, 255, 255}, 6);
            DestroyProjectile(i);
            continue;
        }
        ++i;
    }

    for (size_t i = 0; i < nanoBlades_.size();) {
        if (nanoBlades_[i].world == 1) {
            SpawnHitBurst(nanoBlades_[i].center, Color{230, 190, 80, 255}, 8);
            nanoBlades_[i] = nanoBlades_.back();
            nanoBlades_.pop_back();
            continue;
        }
        ++i;
    }

    for (size_t i = 0; i < nanoPlatforms_.size();) {
        if (nanoPlatforms_[i].world == 1) {
            SpawnHitBurst(nanoPlatforms_[i].position, Color{255, 220, 120, 255}, 10);
            physics_.DestroyBody(nanoPlatforms_[i].platformBody);
            nanoPlatforms_[i] = nanoPlatforms_.back();
            nanoPlatforms_.pop_back();
            continue;
        }
        ++i;
    }

    for (size_t i = 0; i < drones_.size();) {
        if (drones_[i].world == 1) {
            SpawnHitBurst(drones_[i].position, Color{170, 215, 255, 255}, 12);
            drones_[i] = drones_.back();
            drones_.pop_back();
            continue;
        }
        ++i;
    }

    for (GravityWell& well : gravityWells_) {
        if (Vector3Distance(well.position, portal.backPosition) <= config_.wormholeTriggerRadius * 4.0f) {
            well.life = std::min(well.life, 0.15f);
        }
    }

    wormholes_.clear();
    playerWorld_ = 0;
    for (size_t i = 0; i < magicCircles_.size();) {
        MagicCircle& circle = magicCircles_[i];
        if (circle.world == 1 || circle.isWormhole) {
            SpawnHitBurst(circle.position, Color{205, 85, 255, 255}, 14);
            if (circle.world == 1) {
                magicCircles_[i] = magicCircles_.back();
                magicCircles_.pop_back();
                continue;
            }
            circle.isWormhole = false;
            circle.life = 0.0f;
        }
        circle.world = 0;
        ++i;
    }
    eventText_ = "BACKSIDE ANNIHILATED";
    eventTextTimer_ = 2.0f;
    return true;
}
bool Game::CloseWormholeAlongSegment(Vector3 start, Vector3 end, float extraRadius) {
    if (wormholes_.empty()) {
        return false;
    }

    const WormholePortal& portal = wormholes_.front();
    float closeRadius = std::max(config_.wormholeTriggerRadius, config_.wormholeVisualRadius) + 0.75f + extraRadius;
    if (DistancePointToSegment(portal.frontPosition, start, end) <= closeRadius) {
        return CloseWormhole(portal.frontPosition);
    }
    if (DistancePointToSegment(portal.backPosition, start, end) <= closeRadius) {
        return CloseWormhole(portal.backPosition);
    }
    return false;
}
void Game::UpdateWormholes(float dt) {
    for (WormholePortal& portal : wormholes_) {
        portal.playerCooldown = std::max(0.0f, portal.playerCooldown - dt);
        portal.enemyCooldown = std::max(0.0f, portal.enemyCooldown - dt);
    }
    if (wormholes_.empty()) {
        return;
    }

    WormholePortal& portal = wormholes_.front();
    Vector3 playerGate = WormholeCenterForWorld(portal, playerWorld_);
    if (portal.playerCooldown <= 0.0f && Vector3Distance(camera_.position, playerGate) <= config_.wormholeTriggerRadius + playerRadius_) {
        int nextWorld = 1 - playerWorld_;
        float altitude = IsSphericalMap() ? SphericalPlayerAltitude() : std::abs(camera_.position.y);
        camera_.position = TeleportThroughWormhole(camera_.position, nextWorld, altitude);
        playerVelocity_ = ReflectVelocityThroughWormhole(playerVelocity_, camera_.position, nextWorld);
        playerWorld_ = nextWorld;
        portal.playerCooldown = config_.wormholePlayerCooldown;
        camera_.up = UpForWorldAt(camera_.position, playerWorld_);
        camera_.target = Vector3Add(camera_.position, PlayerForward());
        SpawnShockwave(camera_.position, config_.wormholeVisualRadius * 1.6f, Color{170, 70, 255, 255});
        eventText_ = playerWorld_ == 0 ? "WORLD FRONT" : "WORLD BACK";
        eventTextTimer_ = 1.2f;
    }

    if (portal.enemyCooldown > 0.0f) {
        return;
    }
    for (Enemy& enemy : enemies_) {
        if (enemy.world == playerWorld_) {
            continue;
        }
        Vector3 gate = WormholeCenterForWorld(portal, enemy.world);
        Vector3 position = BodyPosition(enemy.body);
        if (Vector3Distance(position, gate) > config_.wormholeTriggerRadius + enemy.radius) {
            continue;
        }
        int nextWorld = 1 - enemy.world;
        float altitude = IsSphericalMap()
            ? SphericalEnemyAltitude(enemy.type)
            : std::max(0.8f, std::abs(Vector3DotProduct(Vector3Subtract(position, Vector3{0.0f, FlatGroundYForWorld(enemy.world), 0.0f}), FlatUpForWorld(enemy.world))));
        Vector3 target = WormholeCenterForWorld(portal, nextWorld);
        target = IsSphericalMap()
            ? SphericalSurfacePoint(target, altitude, nextWorld)
            : Vector3{target.x, FlatGroundYForWorld(nextWorld) + FlatUpForWorld(nextWorld).y * altitude, target.z};
        JPH::Vec3 currentVelocity = physics_.Bodies().GetLinearVelocity(enemy.body);
        Vector3 reflected = ReflectVelocityThroughWormhole(ToRayVector(currentVelocity), target, nextWorld);
        physics_.Bodies().SetPosition(enemy.body, ToJoltVector(target), JPH::EActivation::Activate);
        physics_.Bodies().SetLinearVelocity(enemy.body, ToJoltVelocity(reflected));
        enemy.world = nextWorld;
        portal.enemyCooldown = config_.wormholeEnemyCooldown;
        SpawnHitBurst(target, Color{180, 80, 255, 255}, 16);
        break;
    }
}
void Game::UpdatePickups(float dt) {
    Vector3 player = camera_.position;
    for (size_t i = 0; i < pickups_.size();) {
        Pickup& pickup = pickups_[i];
        pickup.bobTimer += dt;

        bool touched = false;
        if (IsSphericalMap()) {
            touched = Vector3Distance(player, pickup.position) <= pickup.radius + playerRadius_ + SphericalPlayerAltitude() * 0.55f;
        } else {
            float verticalReach = std::abs(player.y - pickup.position.y);
            touched = DistanceXZ(player, pickup.position) <= pickup.radius + playerRadius_ && verticalReach < 1.9f;
        }
        if (touched) {
            if (pickup.type == PickupType::SpaceSuit) {
                hasSpaceSuit_ = true;
                spaceSuitEnabled_ = true;
                gravityScale_ = config_.spaceSuitGravityScale;
                SpawnHitBurst(pickup.position, Color{130, 225, 255, 255}, 26);
                cameraShake_ = std::min(1.0f, cameraShake_ + 0.18f);
                eventText_ = "SPACE SUIT";
                eventTextTimer_ = 1.4f;
                if (TutorialMode() && !pickupTipShown_[0]) {
                    pickupTipShown_[0] = true;
                    ShowTutorialTip("已拾取太空服!\n按Z键开关低重力 (重力降至0.24x, 可高跳远跃)");
                }
            } else if (pickup.type == PickupType::FlightRig) {
                hasFlightRig_ = true;
                flightRigEnabled_ = true;
                flightTargetAltitude_ = std::clamp(
                    IsSphericalMap() ? SphericalAltitudeAt(camera_.position) : camera_.position.y,
                    config_.flightMinAltitude,
                    config_.flightMaxAltitude);
                SpawnHitBurst(pickup.position, Color{180, 245, 255, 255}, 30);
                cameraShake_ = std::min(1.0f, cameraShake_ + 0.2f);
                eventText_ = "FLIGHT RIG";
                eventTextTimer_ = 1.6f;
                if (TutorialMode() && !pickupTipShown_[1]) {
                    pickupTipShown_[1] = true;
                    ShowTutorialTip("已拾取飞行装置!\n按X键开关飞行 (空格升高, Ctrl降低, 悬停瞄准)");
                }
            } else if (pickup.type == PickupType::Skates) {
                hasSkates_ = true;
                skatesEnabled_ = true;
                SpawnHitBurst(pickup.position, Color{165, 255, 185, 255}, 28);
                cameraShake_ = std::min(1.0f, cameraShake_ + 0.16f);
                eventText_ = "SKATES";
                eventTextTimer_ = 1.4f;
                if (TutorialMode() && !pickupTipShown_[2]) {
                    pickupTipShown_[2] = true;
                    ShowTutorialTip("已拾取滑板!\n按C键开关滑板 (极低地面摩擦, 保持高动量滑行)");
                }
            } else if (pickup.type == PickupType::Essence) {
                essence_++;
                SpawnHitBurst(pickup.position, Color{255, 215, 60, 255}, 30);
                cameraShake_ = std::min(1.0f, cameraShake_ + 0.2f);
                eventText_ = "ESSENCE +1";
                eventTextTimer_ = 1.4f;
                if (TutorialMode() && !pickupTipShown_[3]) {
                    pickupTipShown_[3] = true;
                    ShowTutorialTip("已拾取本质/精华!\n额外生命+1  |  受伤时消耗一条命并短暂无敌\n地图上定时刷新");
                }
            }

            pickups_[i] = pickups_.back();
            pickups_.pop_back();
            continue;
        }

        ++i;
    }
}
void Game::UpdateArenaBounds() {
    for (Enemy& enemy : enemies_) {
        Vector3 position = BodyPosition(enemy.body);
        if (IsSphericalMap()) {
            float distance = Vector3Length(position);
            bool outside = IsHollowPhysicsForWorld(enemy.world)
                ? (distance > SphericalRadius() + 3.0f || distance < std::max(1.0f, SphericalRadius() - SphericalCleanupDistance()))
                : (distance > SphericalCleanupDistance() || distance < SphericalRadius() * 0.55f);
            if (outside) {
                Vector3 target = SphericalSurfacePoint(position, SphericalEnemyAltitude(enemy.type), enemy.world);
                physics_.Bodies().SetPosition(enemy.body, ToJoltVector(target), JPH::EActivation::Activate);
                physics_.Bodies().SetLinearVelocity(enemy.body, JPH::Vec3::sZero());
                enemy.externalVelocity = Vector3Zero();
            }
        } else if (IsSquareMap()) {
            if (std::abs(position.x) > squareHalfExtent_ + 2.0f || std::abs(position.z) > squareHalfExtent_ + 2.0f) {
                Vector3 direction = Vector3Normalize(Vector3{-position.x, 0.0f, -position.z});
                physics_.Bodies().SetLinearVelocity(enemy.body, JPH::Vec3(direction.x * enemy.speed, 0.0f, direction.z * enemy.speed));
            }
        } else if (DistanceXZ(position, Vector3Zero()) > arenaRadius_ + 2.0f) {
            Vector3 direction = Vector3Normalize(Vector3{-position.x, 0.0f, -position.z});
            physics_.Bodies().SetLinearVelocity(enemy.body, JPH::Vec3(direction.x * enemy.speed, 0.0f, direction.z * enemy.speed));
        }
    }
}
void Game::BuildMap() {
    props_.clear();
    if (IsSphericalMap()) {
        return;
    }

    if (!IsSquareMap()) {
        for (int i = 0; i < 36; ++i) {
            float angle = static_cast<float>(i) / 36.0f * 6.2831853f;
            float propRadius = arenaRadius_ + RandomFloat(1.5f, 5.2f);
            Vector3 position = Vector3{std::cos(angle) * propRadius, 0.0f, std::sin(angle) * propRadius};
            float height = RandomFloat(1.0f, 4.5f);
            Vector3 scale = Vector3{RandomFloat(0.5f, 1.4f), height, RandomFloat(0.5f, 1.6f)};
            Color color = i % 3 == 0 ? Color{48, 46, 50, 255} : i % 3 == 1 ? Color{70, 64, 60, 255} : Color{38, 45, 52, 255};
            props_.push_back(Prop{position, scale, angle + RandomFloat(-0.5f, 0.5f), color, GetRandomValue(0, 2), false});
        }
        return;
    }

    const Vector3 blocks[] = {
        {-16.0f, 0.0f, -12.0f}, {-6.0f, 0.0f, -18.0f}, {9.0f, 0.0f, -15.0f},
        {18.0f, 0.0f, -5.0f}, {-18.0f, 0.0f, 7.0f}, {-5.0f, 0.0f, 10.0f},
        {12.0f, 0.0f, 13.0f}, {2.0f, 0.0f, -4.0f}
    };
    const Vector3 blockScales[] = {
        {4.2f, 3.0f, 3.2f}, {5.8f, 1.8f, 2.6f}, {3.4f, 4.6f, 3.4f},
        {2.8f, 2.4f, 6.4f}, {3.0f, 5.2f, 3.0f}, {6.0f, 2.1f, 3.0f},
        {4.8f, 3.4f, 4.0f}, {3.6f, 2.6f, 3.6f}
    };
    for (int i = 0; i < 8; ++i) {
        Color color = i % 2 == 0 ? Color{48, 50, 54, 255} : Color{62, 58, 54, 255};
        props_.push_back(Prop{blocks[i], blockScales[i], RandomFloat(-0.4f, 0.4f), color, 0, true});
    }

    const Vector3 platforms[] = {
        {-13.0f, 4.0f, -2.0f}, {-2.0f, 5.8f, -12.0f}, {10.0f, 7.2f, -2.0f},
        {17.0f, 4.8f, 9.0f}, {-12.0f, 8.2f, 14.0f}, {0.0f, 10.2f, 6.0f},
        {8.0f, 12.6f, 16.0f}
    };
    const Vector3 platformScales[] = {
        {6.5f, 0.55f, 4.2f}, {4.2f, 0.55f, 4.2f}, {7.0f, 0.55f, 3.4f},
        {5.2f, 0.55f, 5.2f}, {4.8f, 0.55f, 3.6f}, {6.0f, 0.55f, 6.0f},
        {3.8f, 0.55f, 3.8f}
    };
    for (int i = 0; i < 7; ++i) {
        Color color = i % 2 == 0 ? Color{76, 82, 90, 255} : Color{72, 68, 86, 255};
        props_.push_back(Prop{platforms[i], platformScales[i], 0.0f, color, 0, true});
    }

    const Vector3 highPlatforms[] = {
        {-18.0f, 15.2f, 4.0f}, {-6.0f, 17.4f, 18.0f}, {7.0f, 19.8f, 5.0f},
        {18.0f, 22.4f, -8.0f}, {-2.0f, 25.6f, -18.0f}, {13.0f, 29.0f, 18.0f}
    };
    const Vector3 highPlatformScales[] = {
        {4.4f, 0.5f, 3.4f}, {3.8f, 0.5f, 3.8f}, {4.8f, 0.5f, 3.2f},
        {3.6f, 0.5f, 4.2f}, {3.2f, 0.5f, 3.2f}, {2.8f, 0.5f, 2.8f}
    };
    for (int i = 0; i < 6; ++i) {
        Color color = i % 2 == 0 ? Color{88, 94, 108, 255} : Color{92, 82, 112, 255};
        props_.push_back(Prop{highPlatforms[i], highPlatformScales[i], 0.0f, color, 0, true});
    }

    for (int i = 0; i < 14; ++i) {
        float x = RandomFloat(-squareHalfExtent_ + 4.0f, squareHalfExtent_ - 4.0f);
        float z = RandomFloat(-squareHalfExtent_ + 4.0f, squareHalfExtent_ - 4.0f);
        if (std::abs(x) < 5.0f && std::abs(z - 9.0f) < 7.0f) {
            z -= 12.0f;
        }
        Vector3 position = Vector3{x, 0.0f, z};
        Vector3 scale = Vector3{RandomFloat(0.6f, 1.5f), RandomFloat(1.2f, 4.0f), RandomFloat(0.6f, 1.5f)};
        props_.push_back(Prop{position, scale, RandomFloat(0.0f, 6.2831853f), Color{42, 46, 52, 255}, GetRandomValue(1, 2), false});
    }
}
void Game::ResolveMapCollision(Vector3 previousPosition) {
    if (IsSphericalMap()) {
        Vector3 playerUp = SphericalUpAt(camera_.position);
        Vector3 previousUp = SphericalUpAt(previousPosition);
        for (const NanoPlatform& platform : nanoPlatforms_) {
            if (platform.delay > 0.0f) {
                continue;
            }

            Vector3 normal = SafeNormalize(platform.normal, SphericalUpAt(platform.position));
            Vector3 right = SafeNormalize(ProjectOnSphericalTangent(platform.right, normal), PlayerRight());
            Vector3 forward = SafeNormalize(ProjectOnSphericalTangent(platform.forward, normal), Vector3Normalize(Vector3CrossProduct(normal, right)));
            Vector3 previousFeet = Vector3Subtract(previousPosition, Vector3Scale(previousUp, SphericalPlayerAltitude()));
            Vector3 feet = Vector3Subtract(camera_.position, Vector3Scale(playerUp, SphericalPlayerAltitude()));
            Vector3 previousOffset = Vector3Subtract(previousFeet, platform.position);
            Vector3 offset = Vector3Subtract(feet, platform.position);
            float previousPlane = Vector3DotProduct(previousOffset, normal);
            float plane = Vector3DotProduct(offset, normal);
            float localX = Vector3DotProduct(offset, right);
            float localZ = Vector3DotProduct(offset, forward);
            bool inside = std::abs(localX) <= platform.scale.x * 0.5f + playerRadius_ && std::abs(localZ) <= platform.scale.z * 0.5f + playerRadius_;
            float normalSpeed = Vector3DotProduct(playerVelocity_, normal);
            if (inside && normalSpeed <= 0.0f && previousPlane >= -0.22f && plane <= 0.35f) {
                Vector3 platformContact = Vector3Add(platform.position, Vector3Add(Vector3Scale(right, localX), Vector3Scale(forward, localZ)));
                camera_.position = Vector3Add(platformContact, Vector3Scale(normal, SphericalPlayerAltitude()));
                if (normalSpeed < 0.0f) {
                    playerVelocity_ = Vector3Subtract(playerVelocity_, Vector3Scale(normal, normalSpeed));
                }
                grounded_ = true;
                coyoteTimer_ = 0.11f;
                camera_.up = normal;
                asteroidReferenceForward_ = SafeNormalize(ProjectOnSphericalTangent(asteroidReferenceForward_, normal), platform.forward);
                if (jumpBufferTimer_ > 0.0f) {
                    playerVelocity_ = Vector3Add(ProjectOnSphericalTangent(playerVelocity_, normal), Vector3Scale(normal, config_.jumpSpeed));
                    grounded_ = false;
                    coyoteTimer_ = 0.0f;
                    jumpBufferTimer_ = 0.0f;
                    cameraShake_ = std::min(1.0f, cameraShake_ + 0.12f);
                }
                continue;
            }
        }
        return;
    }

    for (const Prop& prop : props_) {
        if (!prop.collidable || prop.shape != 0) {
            continue;
        }

        float minX = prop.position.x - prop.scale.x * 0.5f - playerRadius_;
        float maxX = prop.position.x + prop.scale.x * 0.5f + playerRadius_;
        float minZ = prop.position.z - prop.scale.z * 0.5f - playerRadius_;
        float maxZ = prop.position.z + prop.scale.z * 0.5f + playerRadius_;
        float topY = prop.position.y + prop.scale.y;
        float bottomY = prop.position.y;
        float feetY = camera_.position.y - playerHeight_;

        bool overlapsXZ = camera_.position.x >= minX && camera_.position.x <= maxX && camera_.position.z >= minZ && camera_.position.z <= maxZ;
        if (!overlapsXZ) {
            continue;
        }

        float previousFeetY = previousPosition.y - playerHeight_;
        if (playerVelocity_.y <= 0.0f && previousFeetY >= topY - 0.25f && feetY <= topY + 0.35f) {
            camera_.position.y = topY + playerHeight_;
            playerVelocity_.y = 0.0f;
            grounded_ = true;
            coyoteTimer_ = 0.11f;
            if (jumpBufferTimer_ > 0.0f) {
                playerVelocity_.y = config_.jumpSpeed;
                grounded_ = false;
                coyoteTimer_ = 0.0f;
                jumpBufferTimer_ = 0.0f;
                cameraShake_ = std::min(1.0f, cameraShake_ + 0.12f);
            }
            continue;
        }

        if (feetY < topY - 0.12f && camera_.position.y > bottomY + 0.15f) {
            float pushLeft = std::abs(camera_.position.x - minX);
            float pushRight = std::abs(maxX - camera_.position.x);
            float pushBack = std::abs(camera_.position.z - minZ);
            float pushFront = std::abs(maxZ - camera_.position.z);
            float best = std::min(std::min(pushLeft, pushRight), std::min(pushBack, pushFront));
            if (best == pushLeft) {
                camera_.position.x = minX;
                playerVelocity_.x = std::min(0.0f, playerVelocity_.x);
            } else if (best == pushRight) {
                camera_.position.x = maxX;
                playerVelocity_.x = std::max(0.0f, playerVelocity_.x);
            } else if (best == pushBack) {
                camera_.position.z = minZ;
                playerVelocity_.z = std::min(0.0f, playerVelocity_.z);
            } else {
                camera_.position.z = maxZ;
                playerVelocity_.z = std::max(0.0f, playerVelocity_.z);
            }
        }
    }

    for (const NanoPlatform& platform : nanoPlatforms_) {
        if (platform.delay > 0.0f) {
            continue;
        }

        Vector3 right = platform.right;
        Vector3 forward = platform.forward;
        float topY = platform.position.y + platform.scale.y;
        float bottomY = platform.position.y;
        float feetY = camera_.position.y - playerHeight_;
        float previousFeetY = previousPosition.y - playerHeight_;

        Vector3 offsetXZ = Vector3{camera_.position.x - platform.position.x, 0.0f, camera_.position.z - platform.position.z};
        float localX = Vector3DotProduct(offsetXZ, right);
        float localZ = Vector3DotProduct(offsetXZ, forward);
        float halfX = platform.scale.x * 0.5f + playerRadius_;
        float halfZ = platform.scale.z * 0.5f + playerRadius_;
        bool inside = std::abs(localX) <= halfX && std::abs(localZ) <= halfZ;
        if (!inside) {
            continue;
        }

        if (playerVelocity_.y <= 0.0f && previousFeetY >= topY - 0.25f && feetY <= topY + 0.35f) {
            camera_.position.y = topY + playerHeight_;
            playerVelocity_.y = 0.0f;
            grounded_ = true;
            coyoteTimer_ = 0.11f;
            if (jumpBufferTimer_ > 0.0f) {
                playerVelocity_.y = config_.jumpSpeed;
                grounded_ = false;
                coyoteTimer_ = 0.0f;
                jumpBufferTimer_ = 0.0f;
                cameraShake_ = std::min(1.0f, cameraShake_ + 0.12f);
            }
            continue;
        }
        // 玩家可从侧面/底部穿过平台（纳米机器人选择性阻挡），仅顶面提供支撑
    }
}
void Game::SpawnStartingPickups() {
    SpawnPickup(PickupType::SpaceSuit, 0);
    SpawnPickup(PickupType::FlightRig, 1);
    SpawnPickup(PickupType::Skates, 2);
}
void Game::SpawnPickup(PickupType type, int slot) {
    Vector3 position = {};
    if (IsSphericalMap()) {
        float theta = (static_cast<float>(slot) / 3.0f) * 6.2831853f + RandomFloat(-0.25f, 0.25f);
        float u = RandomFloat(-0.42f, 0.42f);
        float root = std::sqrt(std::max(0.0f, 1.0f - u * u));
        Vector3 normal = Vector3{root * std::cos(theta), u, root * std::sin(theta)};
        position = SphericalSurfacePoint(normal, SphericalPlayerAltitude() * 0.8f);
    } else if (IsSquareMap()) {
        const Vector3 anchors[] = {
            {-8.0f, 1.0f, 8.0f},
            {8.0f, 1.0f, 8.0f},
            {0.0f, 1.0f, -9.0f}
        };
        Vector3 anchor = anchors[std::clamp(slot, 0, 2)];
        position = Vector3{anchor.x + RandomFloat(-1.2f, 1.2f), anchor.y, anchor.z + RandomFloat(-1.2f, 1.2f)};
    } else {
        float angle = (static_cast<float>(slot) / 3.0f) * 6.2831853f + RandomFloat(-0.2f, 0.2f);
        float radius = arenaRadius_ * (0.42f + static_cast<float>(slot) * 0.08f);
        position = Vector3{std::cos(angle) * radius, 1.0f, std::sin(angle) * radius};
    }
    pickups_.push_back(Pickup{type, position, 0.85f, RandomFloat(0.0f, 6.28f)});
}
void Game::UpdateEssenceSpawn(float dt) {
    if (config_.essenceMaxOnMap <= 0) return;

    int essenceOnMap = 0;
    for (const Pickup& p : pickups_) {
        if (p.type == PickupType::Essence) essenceOnMap++;
    }

    if (essenceOnMap < config_.essenceMaxOnMap) {
        essenceSpawnTimer_ -= dt;
        if (essenceSpawnTimer_ <= 0.0f) {
            essenceSpawnTimer_ = config_.essenceRespawnTime;

            Vector3 position = {};
            if (IsSphericalMap()) {
                float theta = RandomFloat(0.0f, 6.2831853f);
                float u = RandomFloat(-0.42f, 0.42f);
                float root = std::sqrt(std::max(0.0f, 1.0f - u * u));
                Vector3 normal = Vector3{root * std::cos(theta), u, root * std::sin(theta)};
                position = SphericalSurfacePoint(normal, SphericalPlayerAltitude() * 0.8f);
            } else if (IsSquareMap()) {
                float hx = squareHalfExtent_ - 3.0f;
                position = Vector3{RandomFloat(-hx, hx), 1.0f, RandomFloat(-hx, hx)};
            } else {
                float angle = RandomFloat(0.0f, 6.2831853f);
                float radius = arenaRadius_ * RandomFloat(0.15f, 0.75f);
                position = Vector3{std::cos(angle) * radius, 1.0f, std::sin(angle) * radius};
            }
            pickups_.push_back(Pickup{PickupType::Essence, position, 0.85f, RandomFloat(0.0f, 6.28f)});
        }
    }
}
Game::NanoPlatform Game::MakeNanoPlatformTarget(Vector3 direction) const {
    Vector3 forward = Vector3Length(direction) > 0.001f ? Vector3Normalize(direction) : PlayerForward();
    Vector3 target = Vector3Add(WeaponMuzzlePosition(), Vector3Scale(forward, config_.nanoPlatformRange * nanoPlatformRangeScale_));
    float halfLength = config_.nanoPlatformLength * 0.5f;
    float halfWidth = config_.nanoPlatformWidth * 0.5f;

    if (IsSphericalMap()) {
        Vector3 normal = SphericalUpAt(target, playerWorld_);
        float platformRange = config_.nanoPlatformRange * nanoPlatformRangeScale_;
        float targetAltitude = std::max(SphericalPlayerAltitude(), SphericalAltitudeAt(target, playerWorld_));
        targetAltitude += platformRange * 0.18f;
        Vector3 center = SphericalSurfacePoint(target, targetAltitude, playerWorld_);
        Vector3 platformRight = Vector3CrossProduct(forward, normal);
        if (Vector3Length(platformRight) <= 0.001f) {
            platformRight = PlayerRight();
        } else {
            platformRight = Vector3Normalize(platformRight);
        }
        Vector3 platformForward = Vector3Normalize(Vector3CrossProduct(normal, platformRight));
        Vector3 scale = Vector3{config_.nanoPlatformLength, config_.nanoPlatformHeight, config_.nanoPlatformWidth};
        return NanoPlatform{center, scale, normal, platformRight, platformForward, config_.nanoPlatformDelay, config_.nanoPlatformLifetime, config_.nanoPlatformLifetime, playerWorld_};
    }

    if (IsSquareMap()) {
        float limitX = squareHalfExtent_ - halfLength - 0.25f;
        float limitZ = squareHalfExtent_ - halfWidth - 0.25f;
        target.x = std::clamp(target.x, -limitX, limitX);
        target.z = std::clamp(target.z, -limitZ, limitZ);
    } else {
        Vector3 flat = Vector3{target.x, 0.0f, target.z};
        float maxHalf = std::max(halfLength, halfWidth);
        float maxDistance = std::max(0.1f, arenaRadius_ - maxHalf - 0.25f);
        if (Vector3Length(flat) > maxDistance) {
            flat = Vector3Scale(Vector3Normalize(flat), maxDistance);
            target.x = flat.x;
            target.z = flat.z;
        }
    }

    float centerY = std::clamp(target.y, 1.2f, 34.0f);
    Vector3 scale = Vector3{config_.nanoPlatformLength, config_.nanoPlatformHeight, config_.nanoPlatformWidth};
    Vector3 position = Vector3{target.x, centerY - scale.y * 0.5f, target.z};
    Vector3 normal = Vector3{0.0f, 1.0f, 0.0f};
    Vector3 platformRight = Vector3CrossProduct(forward, normal);
    if (Vector3Length(platformRight) <= 0.001f) {
        platformRight = PlayerRight();
    } else {
        platformRight = Vector3Normalize(platformRight);
    }
    Vector3 platformForward = Vector3Normalize(Vector3CrossProduct(normal, platformRight));
    return NanoPlatform{position, scale, normal, platformRight, platformForward, config_.nanoPlatformDelay, config_.nanoPlatformLifetime, config_.nanoPlatformLifetime, playerWorld_};
}
Vector3 Game::GetFireControlAimPoint() const {
    Vector3 origin = camera_.position;
    Vector3 forward = Vector3Length(PlayerForward()) > 0.001f ? Vector3Normalize(PlayerForward()) : Vector3{0.0f, 0.0f, -1.0f};
    float altitude = config_.droneRallyMarkerAltitude;

    if (IsSphericalMap()) {
        float targetR = SphericalSignedRadius(altitude, playerWorld_);

        float a = Vector3DotProduct(forward, forward);
        float b = 2.0f * Vector3DotProduct(origin, forward);
        float c = Vector3DotProduct(origin, origin) - targetR * targetR;
        float det = b * b - 4.0f * a * c;

        if (det >= 0.0f && a > 0.0001f) {
            float sqrtDet = std::sqrt(det);
            float t0 = (-b - sqrtDet) / (2.0f * a);
            float t1 = (-b + sqrtDet) / (2.0f * a);
            float t = (t0 > 0.0f) ? t0 : ((t1 > 0.0f) ? t1 : -1.0f);
            if (t > 0.0f) {
                Vector3 hitPoint = Vector3Add(origin, Vector3Scale(forward, t));
                return SphericalSurfacePoint(hitPoint, altitude, playerWorld_);
            }
        }
        return SphericalSurfacePoint(Vector3Add(origin, Vector3Scale(forward, 20.0f)), altitude, playerWorld_);
    }

    // Flat maps: ray-plane intersection at current world's rally altitude.
    Vector3 flatUp = FlatUpForWorld(playerWorld_);
    float targetPlane = FlatGroundYForWorld(playerWorld_) + flatUp.y * altitude;
    if (std::abs(forward.y) > 0.0001f) {
        float t = (targetPlane - origin.y) / forward.y;
        if (t > 0.0f) {
            Vector3 point = Vector3Add(origin, Vector3Scale(forward, t));
            if (IsSquareMap()) {
                float limit = squareHalfExtent_ - 1.0f;
                point.x = std::clamp(point.x, -limit, limit);
                point.z = std::clamp(point.z, -limit, limit);
            } else {
                Vector3 flat = Vector3{point.x, 0.0f, point.z};
                float limit = arenaRadius_ - 1.5f;
                if (Vector3Length(flat) > limit) {
                    flat = Vector3Scale(Vector3Normalize(flat), limit);
                    point.x = flat.x;
                    point.z = flat.z;
                }
            }
            point.y = targetPlane;
            return point;
        }
    }
    Vector3 fallback = Vector3Add(origin, Vector3Scale(forward, 20.0f));
    fallback.y = targetPlane;
    if (IsSquareMap()) {
        float limit = squareHalfExtent_ - 1.0f;
        fallback.x = std::clamp(fallback.x, -limit, limit);
        fallback.z = std::clamp(fallback.z, -limit, limit);
    }
    return fallback;
}
bool Game::IsSphericalMap() const {
    return config_.mapType == "asteroid" || config_.mapType == "hollow_world";
}
bool Game::IsHollowWorldMap() const {
    return config_.mapType == "hollow_world";
}
bool Game::IsHollowPhysicsForWorld(int world) const {
    bool hollow = IsHollowWorldMap();
    if (IsSphericalMap() && world == 1) {
        hollow = !hollow;
    }
    return hollow;
}
bool Game::SphericalTouchesSurface(Vector3 position, float radius, int world) const {
    float distance = Vector3Length(position);
    return IsHollowPhysicsForWorld(world)
        ? distance >= SphericalRadius() - radius
        : distance <= SphericalRadius() + radius;
}
bool Game::SphericalOutOfBounds(Vector3 position, float padding, int world) const {
    float distance = Vector3Length(position);
    return IsHollowPhysicsForWorld(world)
        ? (distance > SphericalRadius() + padding || distance < std::max(1.0f, SphericalRadius() - SphericalCleanupDistance()))
        : distance > SphericalCleanupDistance();
}
float Game::SphericalRadius() const {
    return IsHollowWorldMap() ? config_.hollowWorldRadius : config_.asteroidRadius;
}
float Game::SphericalPlayerAltitude() const {
    return IsHollowWorldMap() ? config_.hollowWorldPlayerAltitude : config_.asteroidPlayerAltitude;
}
float Game::SphericalCleanupDistance() const {
    return IsHollowWorldMap() ? config_.hollowWorldCleanupDistance : config_.asteroidCleanupDistance;
}
float Game::SphericalAltitudeAt(Vector3 position, int world) const {
    if (world < 0) {
        world = playerWorld_;
    }
    float distance = Vector3Length(position);
    return IsHollowPhysicsForWorld(world) ? SphericalRadius() - distance : distance - SphericalRadius();
}
float Game::SphericalSignedRadius(float altitude, int world) const {
    if (world < 0) {
        world = playerWorld_;
    }
    return IsHollowPhysicsForWorld(world) ? SphericalRadius() - altitude : SphericalRadius() + altitude;
}
Vector3 Game::SphericalUpAt(Vector3 position, int world) const {
    if (world < 0) {
        world = playerWorld_;
    }
    if (Vector3Length(position) <= 0.001f) {
        return Vector3{0.0f, 1.0f, 0.0f};
    }
    Vector3 outward = Vector3Normalize(position);
    return IsHollowPhysicsForWorld(world) ? Vector3Scale(outward, -1.0f) : outward;
}
Vector3 Game::SphericalSurfacePoint(Vector3 position, float altitude, int world) const {
    if (world < 0) {
        world = playerWorld_;
    }
    Vector3 outward = Vector3Length(position) > 0.001f
        ? Vector3Normalize(position)
        : Vector3{0.0f, IsHollowPhysicsForWorld(world) ? -1.0f : 1.0f, 0.0f};
    return Vector3Scale(outward, SphericalSignedRadius(altitude, world));
}
Vector3 Game::ProjectOnSphericalTangent(Vector3 vector, Vector3 up) const {
    return Vector3Subtract(vector, Vector3Scale(up, Vector3DotProduct(vector, up)));
}
float Game::SphericalEnemyAltitude(EnemyType type) const {
    if (type == EnemyType::Harrier) {
        return config_.harrierTargetHeight;
    }
    if (type == EnemyType::Wisp || type == EnemyType::Spitter) {
        return 1.35f;
    }
    if (type == EnemyType::Boss || type == EnemyType::DummyBoss) {
        return 2.2f;
    }
    if (type == EnemyType::SlimeKing) {
        return 1.5f;
    }
    if (type == EnemyType::Duelist) {
        return 1.2f;
    }
    if (type == EnemyType::Blinker) {
        return 1.0f;
    }
    if (type == EnemyType::Pouncer) {
        return 0.9f;
    }
    return IsHollowWorldMap() ? config_.hollowWorldEnemyAltitude : config_.asteroidEnemyAltitude;
}
Vector3 Game::BodyPosition(JPH::BodyID id) const {
    return ToRayVector(physics_.Bodies().GetCenterOfMassPosition(id));
}
bool Game::IsSquareMap() const {
    return config_.mapType == "square_obstacle" || config_.mapType == "square";
}
bool Game::EnemyTouchesPlayer(Vector3 enemyPosition, float enemyRadius) const {
    Vector3 up = UpForWorldAt(camera_.position, playerWorld_);
    Vector3 capsuleBottom = IsSphericalMap()
        ? Vector3Subtract(camera_.position, Vector3Scale(up, SphericalPlayerAltitude() - playerRadius_))
        : Vector3Subtract(camera_.position, Vector3Scale(up, playerHeight_ - playerRadius_));
    Vector3 capsuleTop = IsSphericalMap()
        ? Vector3Subtract(camera_.position, Vector3Scale(up, playerRadius_ * 0.35f))
        : Vector3Subtract(camera_.position, Vector3Scale(up, playerRadius_ * 0.35f));
    float hitDistance = enemyRadius + playerRadius_;
    return DistancePointToSegment(enemyPosition, capsuleBottom, capsuleTop) <= hitDistance;
}
float Game::DistanceXZ(Vector3 a, Vector3 b) const {
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    return std::sqrt(dx * dx + dz * dz);
}
float Game::DistancePointToSegment(Vector3 point, Vector3 start, Vector3 end) const {
    Vector3 segment = Vector3Subtract(end, start);
    float lengthSq = Vector3DotProduct(segment, segment);
    if (lengthSq <= 0.0001f) {
        return Vector3Distance(point, start);
    }

    float t = Vector3DotProduct(Vector3Subtract(point, start), segment) / lengthSq;
    t = std::clamp(t, 0.0f, 1.0f);
    Vector3 closest = Vector3Add(start, Vector3Scale(segment, t));
    return Vector3Distance(point, closest);
}
const char* Game::WaveLabel() const {
    if (DuelMode()) {
        return duelWon_ ? "DUEL WON" : "DUEL";
    }
    switch (waveIndex_) {
        case 1:
            return "WAVE 1";
        case 2:
            return "WAVE 2";
        case 3:
            return "WAVE 3";
        default:
            return "WAVE 4";
    }
}
