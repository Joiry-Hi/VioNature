#include "Game.h"
#include "GameMath.h"

#include <algorithm>
#include <cmath>

void Game::UpdatePlayer(float dt) {
    Vector3 previousPosition = camera_.position;
    UpdateMovement(dt);

    if (IsSphericalMap()) {
        ResolveMapCollision(previousPosition);
    } else if (IsSquareMap()) {
        float maxCoord = squareHalfExtent_ - playerRadius_;
        if (camera_.position.x < -maxCoord || camera_.position.x > maxCoord) {
            camera_.position.x = std::clamp(camera_.position.x, -maxCoord, maxCoord);
            playerVelocity_.x = 0.0f;
        }
        if (camera_.position.z < -maxCoord || camera_.position.z > maxCoord) {
            camera_.position.z = std::clamp(camera_.position.z, -maxCoord, maxCoord);
            playerVelocity_.z = 0.0f;
        }
        ResolveMapCollision(previousPosition);
    } else if (IsSquareMap()) {
        Vector3 flatPosition = Vector3{camera_.position.x, 0.0f, camera_.position.z};
        float distanceFromCenter = Vector3Length(flatPosition);
        float maxDistance = arenaRadius_ - playerRadius_;
        if (distanceFromCenter > maxDistance) {
            Vector3 clamped = Vector3Scale(Vector3Normalize(flatPosition), maxDistance);
            Vector3 offset = Vector3Subtract(clamped, flatPosition);
            camera_.position.x += offset.x;
            camera_.position.z += offset.z;

            Vector3 inward = Vector3Normalize(Vector3{-camera_.position.x, 0.0f, -camera_.position.z});
            float outwardSpeed = Vector3DotProduct(playerVelocity_, Vector3{-inward.x, 0.0f, -inward.z});
            if (outwardSpeed > 0.0f) {
                playerVelocity_.x += inward.x * outwardSpeed;
                playerVelocity_.z += inward.z * outwardSpeed;
            }
        }
        ResolveMapCollision(previousPosition);
    } else {
        Vector3 flatPosition = Vector3{camera_.position.x, 0.0f, camera_.position.z};
        float distanceFromCenter = Vector3Length(flatPosition);
        float maxDistance = arenaRadius_ - playerRadius_;
        if (distanceFromCenter > maxDistance) {
            Vector3 clamped = Vector3Scale(Vector3Normalize(flatPosition), maxDistance);
            Vector3 offset = Vector3Subtract(clamped, flatPosition);
            camera_.position.x += offset.x;
            camera_.position.z += offset.z;

            Vector3 inward = Vector3Normalize(Vector3{-camera_.position.x, 0.0f, -camera_.position.z});
            float outwardSpeed = Vector3DotProduct(playerVelocity_, Vector3{-inward.x, 0.0f, -inward.z});
            if (outwardSpeed > 0.0f) {
                playerVelocity_.x += inward.x * outwardSpeed;
                playerVelocity_.z += inward.z * outwardSpeed;
            }
        }
        float floorY = FlatGroundYForWorld(playerWorld_) + FlatUpForWorld(playerWorld_).y * playerHeight_;
        if (playerWorld_ == 0) {
            if (camera_.position.y < floorY) {
                camera_.position.y = floorY;
                if (playerVelocity_.y < 0.0f) playerVelocity_.y = 0.0f;
            }
        } else if (camera_.position.y > floorY) {
            camera_.position.y = floorY;
            if (playerVelocity_.y > 0.0f) playerVelocity_.y = 0.0f;
        }
        ResolveMapCollision(previousPosition);
    }

    if (cameraShake_ > 0.0f) {
        float shake = cameraShake_ * 0.035f;
        camera_.position.x += RandomFloat(-shake, shake);
        camera_.position.y += RandomFloat(-shake, shake);
    }

    camera_.up = UpForWorldAt(camera_.position, playerWorld_);
    camera_.target = Vector3Add(camera_.position, PlayerForward());
}
void Game::UpdateLook(float dt) {
    Vector2 delta = GetMouseDelta();
    if (IsSphericalMap()) {
        Vector3 up = SphericalUpAt(camera_.position, playerWorld_);
        asteroidReferenceForward_ = ProjectOnSphericalTangent(asteroidReferenceForward_, up);
        if (Vector3Length(asteroidReferenceForward_) <= 0.001f) {
            asteroidReferenceForward_ = ProjectOnSphericalTangent(PlayerForward(), up);
        }
        if (Vector3Length(asteroidReferenceForward_) <= 0.001f) {
            asteroidReferenceForward_ = Vector3Normalize(Vector3CrossProduct(Vector3{1.0f, 0.0f, 0.0f}, up));
        } else {
            asteroidReferenceForward_ = Vector3Normalize(asteroidReferenceForward_);
        }
        asteroidReferenceForward_ = Vector3Normalize(RotateAroundAxis(asteroidReferenceForward_, up, -delta.x * kMouseSensitivity * kDegToRad));
    } else {
        yaw_ += delta.x * kMouseSensitivity * (playerWorld_ == 0 ? 1.0f : -1.0f);
    }
    pitch_ -= delta.y * kMouseSensitivity;
    pitch_ = std::clamp(pitch_, -89.0f, 89.0f);
    (void)dt;
}
void Game::UpdateFreeCamera(float dt) {
    Vector3 forward = PlayerForward();
    Vector3 right = PlayerRight();
    Vector3 up = UpForWorldAt(camera_.position, playerWorld_);

    Vector3 move = Vector3Zero();
    if (IsKeyDown(KEY_W)) {
        move = Vector3Add(move, forward);
    }
    if (IsKeyDown(KEY_S)) {
        move = Vector3Subtract(move, forward);
    }
    if (IsKeyDown(KEY_D)) {
        move = Vector3Add(move, right);
    }
    if (IsKeyDown(KEY_A)) {
        move = Vector3Subtract(move, right);
    }
    if (IsKeyDown(KEY_SPACE)) {
        move = Vector3Add(move, up);
    }
    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
        move = Vector3Subtract(move, up);
    }

    if (Vector3Length(move) > 0.001f) {
        move = Vector3Normalize(move);
        float speed = (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) ? 18.0f : 8.5f;
        camera_.position = Vector3Add(camera_.position, Vector3Scale(move, speed * dt));
    }
    camera_.up = up;
}
void Game::UpdateMovement(float dt) {
    constexpr float kCoyoteTime = 0.11f;
    constexpr float kJumpBufferTime = 0.14f;

    bool wasGrounded = grounded_;
    if (wasGrounded) {
        coyoteTimer_ = kCoyoteTime;
    } else {
        coyoteTimer_ = std::max(0.0f, coyoteTimer_ - dt);
    }
    if (!flightRigEnabled_ && IsKeyPressed(KEY_SPACE)) {
        jumpBufferTimer_ = kJumpBufferTime;
    } else {
        jumpBufferTimer_ = std::max(0.0f, jumpBufferTimer_ - dt);
    }

    if (IsSphericalMap()) {
        Vector3 up = SphericalUpAt(camera_.position, playerWorld_);
        camera_.up = up;
        asteroidReferenceForward_ = ProjectOnSphericalTangent(asteroidReferenceForward_, up);
        Vector3 forward = asteroidReferenceForward_;
        if (Vector3Length(forward) <= 0.001f) {
            forward = Vector3Normalize(Vector3CrossProduct(Vector3{1.0f, 0.0f, 0.0f}, up));
        } else {
            forward = Vector3Normalize(forward);
        }
        asteroidReferenceForward_ = forward;
        Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, up));

        Vector3 wishDirection = Vector3Zero();
        if (IsKeyDown(KEY_W)) {
            wishDirection = Vector3Add(wishDirection, forward);
        }
        if (IsKeyDown(KEY_S)) {
            wishDirection = Vector3Subtract(wishDirection, forward);
        }
        if (IsKeyDown(KEY_D)) {
            wishDirection = Vector3Add(wishDirection, right);
        }
        if (IsKeyDown(KEY_A)) {
            wishDirection = Vector3Subtract(wishDirection, right);
        }
        if (Vector3Length(wishDirection) > 0.001f) {
            wishDirection = Vector3Normalize(ProjectOnSphericalTangent(wishDirection, up));
        }

        bool running = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        float speed = (running ? config_.runSpeed : config_.walkSpeed) * (skatesEnabled_ ? config_.skatesMaxSpeedBonus : 1.0f);
        Vector3 radialVelocity = Vector3Scale(up, Vector3DotProduct(playerVelocity_, up));
        Vector3 tangentVelocity = ProjectOnSphericalTangent(playerVelocity_, up);
        Vector3 targetVelocity = Vector3Scale(wishDirection, speed);
        float acceleration = grounded_ ? config_.groundAcceleration : config_.airAcceleration;
        if (skatesEnabled_) {
            acceleration *= grounded_ ? config_.skatesGroundFriction : config_.skatesAirControl;
            if (Vector3Length(wishDirection) <= 0.001f && grounded_) {
                targetVelocity = tangentVelocity;
            }
        }
        float blend = thrustControlLockTimer_ > 0.0f ? 0.0f : std::clamp(acceleration * dt, 0.0f, 1.0f);
        tangentVelocity = Vector3Add(tangentVelocity, Vector3Scale(Vector3Subtract(targetVelocity, tangentVelocity), blend));
        playerVelocity_ = Vector3Add(tangentVelocity, radialVelocity);

        if (flightRigEnabled_) {
            if (IsKeyDown(KEY_SPACE)) {
                flightTargetAltitude_ += config_.flightVerticalSpeed * dt;
            }
            if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
                flightTargetAltitude_ -= config_.flightVerticalSpeed * dt;
            }
            flightTargetAltitude_ = std::clamp(flightTargetAltitude_, config_.flightMinAltitude, config_.flightMaxAltitude);
            float currentAltitude = SphericalAltitudeAt(camera_.position, playerWorld_);
            float altitudeError = flightTargetAltitude_ - currentAltitude;
            float radialSpeed = Vector3DotProduct(playerVelocity_, up);
            float altitudeSpeed = radialSpeed;
            float radialAcceleration = altitudeError * config_.flightHoverStrength - altitudeSpeed * config_.flightHoverDamping;
            radialVelocity = Vector3Scale(up, radialSpeed + radialAcceleration * dt);
            playerVelocity_ = Vector3Add(ProjectOnSphericalTangent(playerVelocity_, up), radialVelocity);
            grounded_ = false;
            coyoteTimer_ = 0.0f;
            jumpBufferTimer_ = 0.0f;
        } else if (jumpBufferTimer_ > 0.0f && coyoteTimer_ > 0.0f) {
            playerVelocity_ = Vector3Add(ProjectOnSphericalTangent(playerVelocity_, up), Vector3Scale(up, config_.jumpSpeed));
            grounded_ = false;
            coyoteTimer_ = 0.0f;
            jumpBufferTimer_ = 0.0f;
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.12f);
        }

        if (!flightRigEnabled_) {
            playerVelocity_ = Vector3Subtract(playerVelocity_, Vector3Scale(up, CurrentGravity() * dt));
        }
        camera_.position = Vector3Add(camera_.position, Vector3Scale(playerVelocity_, dt));
        up = SphericalUpAt(camera_.position, playerWorld_);
        float desiredRadius = SphericalSignedRadius(SphericalPlayerAltitude(), playerWorld_);
        float distance = Vector3Length(camera_.position);
        float radialSpeed = Vector3DotProduct(playerVelocity_, up);
        constexpr float kGroundTolerance = 0.2f;
        bool nearSurface = IsHollowPhysicsForWorld(playerWorld_) ? distance >= desiredRadius - kGroundTolerance : distance <= desiredRadius + kGroundTolerance;
        if (!flightRigEnabled_ && nearSurface && radialSpeed <= 0.0f) {
            camera_.position = SphericalSurfacePoint(camera_.position, SphericalPlayerAltitude(), playerWorld_);
            if (radialSpeed < 0.0f) {
                playerVelocity_ = ProjectOnSphericalTangent(playerVelocity_, up);
            }
            grounded_ = true;
            coyoteTimer_ = kCoyoteTime;
        } else {
            grounded_ = false;
        }

        float horizontalSpeed = Vector3Length(ProjectOnSphericalTangent(playerVelocity_, up));
        footstepBob_ += horizontalSpeed * dt * (running ? 1.5f : 1.0f);
        if (grounded_ && horizontalSpeed > 0.5f) {
            camera_.position = Vector3Add(camera_.position, Vector3Scale(up, std::sin(footstepBob_ * 7.0f) * 0.035f));
        }
        camera_.up = up;
        return;
    }

    Vector3 forward = PlayerForward();
    forward.y = 0.0f;
    if (Vector3Length(forward) > 0.001f) {
        forward = Vector3Normalize(forward);
    }

    Vector3 flatUp = FlatUpForWorld(playerWorld_);
    Vector3 right = PlayerRight();
    right = ProjectOnSphericalTangent(right, flatUp);
    if (Vector3Length(right) > 0.001f) {
        right = Vector3Normalize(right);
    }

    Vector3 wishDirection = Vector3Zero();
    if (IsKeyDown(KEY_W)) {
        wishDirection = Vector3Add(wishDirection, forward);
    }
    if (IsKeyDown(KEY_S)) {
        wishDirection = Vector3Subtract(wishDirection, forward);
    }
    if (IsKeyDown(KEY_D)) {
        wishDirection = Vector3Add(wishDirection, right);
    }
    if (IsKeyDown(KEY_A)) {
        wishDirection = Vector3Subtract(wishDirection, right);
    }
    if (Vector3Length(wishDirection) > 0.001f) {
        wishDirection = Vector3Normalize(wishDirection);
    }

    bool running = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    float speed = (running ? config_.runSpeed : config_.walkSpeed) * (skatesEnabled_ ? config_.skatesMaxSpeedBonus : 1.0f);
    Vector3 targetVelocity = Vector3Scale(wishDirection, speed);
    targetVelocity.y = playerVelocity_.y;

    float acceleration = grounded_ ? config_.groundAcceleration : config_.airAcceleration;
    if (skatesEnabled_) {
        acceleration *= grounded_ ? config_.skatesGroundFriction : config_.skatesAirControl;
        if (Vector3Length(wishDirection) <= 0.001f && grounded_) {
            targetVelocity.x = playerVelocity_.x;
            targetVelocity.z = playerVelocity_.z;
        }
    }
    float blend = thrustControlLockTimer_ > 0.0f ? 0.0f : std::clamp(acceleration * dt, 0.0f, 1.0f);
    playerVelocity_.x = playerVelocity_.x + (targetVelocity.x - playerVelocity_.x) * blend;
    playerVelocity_.z = playerVelocity_.z + (targetVelocity.z - playerVelocity_.z) * blend;

    if (flightRigEnabled_) {
        if (IsKeyDown(KEY_SPACE)) {
            flightTargetAltitude_ += config_.flightVerticalSpeed * dt;
        }
        if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
            flightTargetAltitude_ -= config_.flightVerticalSpeed * dt;
        }
        flightTargetAltitude_ = std::clamp(flightTargetAltitude_, config_.flightMinAltitude, config_.flightMaxAltitude);
        float currentAltitude = Vector3DotProduct(camera_.position, flatUp);
        float radialSpeed = Vector3DotProduct(playerVelocity_, flatUp);
        float altitudeError = flightTargetAltitude_ - currentAltitude;
        float verticalAcceleration = altitudeError * config_.flightHoverStrength - radialSpeed * config_.flightHoverDamping;
        playerVelocity_ = Vector3Add(ProjectOnSphericalTangent(playerVelocity_, flatUp), Vector3Scale(flatUp, radialSpeed + verticalAcceleration * dt));
        grounded_ = false;
        coyoteTimer_ = 0.0f;
        jumpBufferTimer_ = 0.0f;
    } else if (jumpBufferTimer_ > 0.0f && coyoteTimer_ > 0.0f) {
        playerVelocity_ = Vector3Add(ProjectOnSphericalTangent(playerVelocity_, flatUp), Vector3Scale(flatUp, config_.jumpSpeed));
        grounded_ = false;
        coyoteTimer_ = 0.0f;
        jumpBufferTimer_ = 0.0f;
        cameraShake_ = std::min(1.0f, cameraShake_ + 0.12f);
    }

    if (!flightRigEnabled_) {
        playerVelocity_ = Vector3Subtract(playerVelocity_, Vector3Scale(flatUp, CurrentGravity() * dt));
    }
    camera_.position = Vector3Add(camera_.position, Vector3Scale(playerVelocity_, dt));

    float floorHeight = FlatGroundYForWorld(playerWorld_) + FlatUpForWorld(playerWorld_).y * playerHeight_;
    constexpr float kGroundTolerance = 0.2f;
    float verticalSpeed = Vector3DotProduct(playerVelocity_, flatUp);
    float floorDistance = Vector3DotProduct(Vector3Subtract(camera_.position, Vector3{0.0f, floorHeight, 0.0f}), flatUp);
    if (!flightRigEnabled_ && verticalSpeed <= 0.0f && floorDistance <= kGroundTolerance) {
        camera_.position.y = floorHeight;
        if (verticalSpeed < 0.0f) {
            playerVelocity_ = Vector3Subtract(playerVelocity_, Vector3Scale(flatUp, verticalSpeed));
        }
        grounded_ = true;
        coyoteTimer_ = kCoyoteTime;
    } else {
        grounded_ = false;
    }

    float horizontalSpeed = std::sqrt(playerVelocity_.x * playerVelocity_.x + playerVelocity_.z * playerVelocity_.z);
    footstepBob_ += horizontalSpeed * dt * (running ? 1.5f : 1.0f);
    if (grounded_ && horizontalSpeed > 0.5f) {
        camera_.position = Vector3Add(camera_.position, Vector3Scale(flatUp, std::sin(footstepBob_ * 7.0f) * 0.035f));
    }
}
void Game::ToggleTimeStop() {
    if (timeStopped_) {
        RestoreDynamicObjects();
        timeStopped_ = false;
        eventText_ = "TIME FLOWS";
    } else {
        timeStopped_ = true;
        FreezeDynamicObjects();
        eventText_ = "TIME STOP";
    }
    eventTextTimer_ = 1.8f;
    timeStopTintTimer_ = 0.35f;
    cameraShake_ = std::min(1.0f, cameraShake_ + 0.18f);
}
void Game::FreezeDynamicObjects() {
    for (Enemy& enemy : enemies_) {
        if (enemy.frozen) {
            continue;
        }
        JPH::Vec3 velocity = physics_.Bodies().GetLinearVelocity(enemy.body);
        enemy.storedVelocity = Vector3{velocity.GetX(), velocity.GetY(), velocity.GetZ()};
        enemy.frozen = true;
        physics_.Bodies().SetLinearVelocity(enemy.body, JPH::Vec3::sZero());
    }

    for (Projectile& projectile : projectiles_) {
        if (projectile.frozen) {
            continue;
        }
        JPH::Vec3 velocity = physics_.Bodies().GetLinearVelocity(projectile.body);
        projectile.storedVelocity = Vector3{velocity.GetX(), velocity.GetY(), velocity.GetZ()};
        projectile.frozen = true;
        physics_.Bodies().SetLinearVelocity(projectile.body, JPH::Vec3::sZero());
    }
}
void Game::RestoreDynamicObjects() {
    for (Enemy& enemy : enemies_) {
        if (!enemy.frozen) {
            continue;
        }
        physics_.Bodies().SetLinearVelocity(enemy.body, JPH::Vec3(enemy.storedVelocity.x, enemy.storedVelocity.y, enemy.storedVelocity.z));
        enemy.storedVelocity = Vector3Zero();
        enemy.frozen = false;
    }

    for (Projectile& projectile : projectiles_) {
        if (!projectile.frozen) {
            continue;
        }
        physics_.Bodies().SetLinearVelocity(projectile.body, JPH::Vec3(projectile.storedVelocity.x, projectile.storedVelocity.y, projectile.storedVelocity.z));
        projectile.storedVelocity = Vector3Zero();
        projectile.frozen = false;
    }
}
void Game::Blink() {
    Vector3 start = camera_.position;
    Vector3 forward = PlayerForward();
    float travel = config_.blinkDistance * blinkDistanceScale_;
    float maxDistance = arenaRadius_ - playerRadius_;
    if (!IsSphericalMap()) {
        Vector3 flatStart = Vector3{start.x, 0.0f, start.z};
        Vector3 flatForward = Vector3{forward.x, 0.0f, forward.z};
        float a = Vector3DotProduct(flatForward, flatForward);
        if (a > 0.0001f) {
            float b = 2.0f * Vector3DotProduct(flatStart, flatForward);
            float c = Vector3DotProduct(flatStart, flatStart) - maxDistance * maxDistance;
            float discriminant = b * b - 4.0f * a * c;
            if (discriminant >= 0.0f) {
                float boundaryT = (-b + std::sqrt(discriminant)) / (2.0f * a);
                if (boundaryT >= 0.0f) {
                    travel = std::min(travel, std::max(0.0f, boundaryT - 0.15f));
                }
            }
        }
    }

    Vector3 target = Vector3Add(start, Vector3Scale(forward, travel));
    if (IsSphericalMap()) {
        float surfaceRadius = SphericalSignedRadius(SphericalPlayerAltitude(), playerWorld_);
        bool beyondSurface = IsHollowPhysicsForWorld(playerWorld_)
            ? Vector3Length(target) > surfaceRadius
            : Vector3Length(target) < surfaceRadius;
        if (beyondSurface) {
            target = SphericalSurfacePoint(target, SphericalPlayerAltitude(), playerWorld_);
        }
        if (!IsHollowPhysicsForWorld(playerWorld_) && Vector3Length(target) > SphericalCleanupDistance() * 0.72f) {
            target = SphericalSurfacePoint(target, SphericalCleanupDistance() * 0.72f - SphericalRadius(), playerWorld_);
        }
    } else {
        float floorHeight = FlatGroundYForWorld(playerWorld_) + FlatUpForWorld(playerWorld_).y * playerHeight_;
        target.y = playerWorld_ == 0
            ? std::max(floorHeight, target.y)
            : std::min(floorHeight, target.y);

        Vector3 flat = Vector3{target.x, 0.0f, target.z};
        if (Vector3Length(flat) > maxDistance) {
            flat = Vector3Scale(Vector3Normalize(flat), maxDistance);
            target.x = flat.x;
            target.z = flat.z;
        }
    }

    camera_.position = target;
    camera_.up = UpForWorldAt(camera_.position, playerWorld_);
    camera_.target = Vector3Add(camera_.position, PlayerForward());
    playerVelocity_ = Vector3Scale(playerVelocity_, 0.35f);

    for (size_t i = 0; i < enemies_.size();) {
        if (enemies_[i].world != playerWorld_) {
            ++i;
            continue;
        }
        Vector3 enemyPosition = BodyPosition(enemies_[i].body);
        if (Vector3Distance(enemyPosition, target) <= config_.blinkClearRadius + enemies_[i].radius) {
            score_ += enemies_[i].scoreValue;
            SpawnHitBurst(enemyPosition, Color{190, 160, 255, 255}, 24);
            DestroyEnemy(i);
            continue;
        }
        ++i;
    }

    SpawnShockwave(target, config_.blinkClearRadius, Color{175, 130, 255, 255});
    SpawnHitBurst(target, Color{120, 220, 255, 255}, 32);
    eventText_ = "SPACE SNAP";
    eventTextTimer_ = 1.4f;
    cameraShake_ = std::min(1.0f, cameraShake_ + 0.35f);
}
void Game::BlinkDuelist(Enemy& enemy, Vector3 awayFrom) {
    Vector3 position = BodyPosition(enemy.body);
    Vector3 direction = Vector3Length(awayFrom) > 0.001f ? Vector3Normalize(awayFrom) : Vector3{1.0f, 0.0f, 0.0f};
    if (IsSphericalMap()) {
        direction = SafeNormalize(ProjectOnSphericalTangent(direction, SphericalUpAt(position, enemy.world)), PlayerRight());
    }
    Vector3 target = Vector3Add(position, Vector3Scale(direction, 8.5f));
    if (IsSphericalMap()) {
        target = SphericalSurfacePoint(target, SphericalEnemyAltitude(EnemyType::Duelist), enemy.world);
    } else if (IsSquareMap()) {
        float limit = squareHalfExtent_ - enemy.radius - 1.0f;
        target.x = std::clamp(target.x, -limit, limit);
        target.z = std::clamp(target.z, -limit, limit);
    } else {
        Vector3 flat = Vector3{target.x, 0.0f, target.z};
        float maxDistance = arenaRadius_ - enemy.radius - 1.0f;
        if (Vector3Length(flat) > maxDistance) {
            flat = Vector3Scale(Vector3Normalize(flat), maxDistance);
            target.x = flat.x;
            target.z = flat.z;
        }
    }
    if (!IsSphericalMap()) {
        target.y = FlatGroundYForWorld(enemy.world) + FlatUpForWorld(enemy.world).y * 1.2f;
    }
    physics_.Bodies().SetPosition(enemy.body, ToJoltVector(target), JPH::EActivation::Activate);
    physics_.Bodies().SetLinearVelocity(enemy.body, JPH::Vec3::sZero());
    enemy.externalVelocity = Vector3Zero();
    SpawnShockwave(target, 3.2f, Color{255, 215, 130, 255});
    SpawnHitBurst(target, Color{255, 230, 150, 255}, 20);
}
void Game::ApplyExplosionImpulse(Vector3 position, float radius, float impulse) {
    Vector3 player = camera_.position;
    float distance = Vector3Distance(player, position);
    if (distance > radius) {
        return;
    }

    Vector3 direction = Vector3Subtract(player, position);
    if (Vector3Length(direction) <= 0.001f) {
        direction = UpForWorldAt(player, playerWorld_);
    }
    Vector3 up = UpForWorldAt(player, playerWorld_);
    direction = Vector3Add(direction, Vector3Scale(up, 0.75f));
    direction = Vector3Normalize(direction);
    float falloff = 1.0f - std::clamp(distance / std::max(0.001f, radius), 0.0f, 1.0f);
    float strength = impulse * (0.25f + falloff * 0.75f);
    playerVelocity_ = Vector3Add(playerVelocity_, Vector3Scale(direction, strength));
    if (IsSphericalMap()) {
        float radialSpeed = Vector3DotProduct(playerVelocity_, up);
        if (radialSpeed > 24.0f) {
            playerVelocity_ = Vector3Subtract(playerVelocity_, Vector3Scale(up, radialSpeed - 24.0f));
        }
    } else {
        playerVelocity_.y = std::min(playerVelocity_.y, 24.0f);
    }
    grounded_ = false;
}
void Game::ApplyShotgunRecoil(Vector3 direction) {
    Vector3 up = UpForWorldAt(camera_.position, playerWorld_);
    Vector3 recoil = Vector3Scale(direction, -config_.shotgunRecoilImpulse);
    recoil = Vector3Add(recoil, Vector3Scale(up, std::max(0.0f, -Vector3DotProduct(direction, up)) * config_.shotgunRecoilVerticalBonus));
    playerVelocity_ = Vector3Add(playerVelocity_, recoil);
    if (IsSphericalMap()) {
        float speed = Vector3Length(playerVelocity_);
        if (speed > 36.0f) {
            playerVelocity_ = Vector3Scale(Vector3Normalize(playerVelocity_), 36.0f);
        }
    } else {
        playerVelocity_.x = std::clamp(playerVelocity_.x, -28.0f, 28.0f);
        playerVelocity_.z = std::clamp(playerVelocity_.z, -28.0f, 28.0f);
        playerVelocity_.y = std::clamp(playerVelocity_.y, -22.0f, 24.0f);
    }
    grounded_ = false;
}
void Game::ApplySpearRecoil(Vector3 direction) {
    Vector3 up = UpForWorldAt(camera_.position, playerWorld_);
    Vector3 recoil = Vector3Scale(direction, -config_.longinusSpearImpulse);
    recoil = Vector3Add(recoil, Vector3Scale(up, std::max(0.0f, -Vector3DotProduct(direction, up)) * config_.longinusSpearImpulse * 0.42f));
    playerVelocity_ = Vector3Add(playerVelocity_, recoil);
    if (IsSphericalMap()) {
        float speed = Vector3Length(playerVelocity_);
        if (speed > 42.0f) {
            playerVelocity_ = Vector3Scale(Vector3Normalize(playerVelocity_), 42.0f);
        }
    } else {
        playerVelocity_.x = std::clamp(playerVelocity_.x, -36.0f, 36.0f);
        playerVelocity_.z = std::clamp(playerVelocity_.z, -36.0f, 36.0f);
        playerVelocity_.y = std::clamp(playerVelocity_.y, -24.0f, 28.0f);
    }
    grounded_ = false;
}
void Game::ApplyPlayerHit(Vector3 position, Color color, const char* text) {
    if (config_.invincible || state_ != State::Playing) {
        return;
    }

    if (longinusSpearThrustInvulnTimer_ > 0.0f) {
        return;
    }

    if (essenceInvulnTimer_ > 0.0f) {
        return;
    }

    if (DuelMode() && duelArmorInvulnTimer_ > 0.0f) {
        return;
    }

    if (DuelMode() && duelArmor_ > 0) {
        duelArmor_ -= 1;
        duelArmorInvulnTimer_ = config_.duelArmorHitInvuln;
        cameraShake_ = std::min(1.0f, cameraShake_ + 0.65f);
        damageFlash_ = 1.0f;
        SpawnHitBurst(position, color, 30);
        SpawnShockwave(camera_.position, 3.4f, Color{255, 215, 120, 255});
        eventText_ = duelArmor_ > 0 ? "ARMOR HIT" : "ARMOR BROKEN";
        eventTextTimer_ = 1.6f;
        return;
    }

    if (essence_ > 0) {
        essence_--;
        essenceInvulnTimer_ = config_.essenceHitInvuln;
        cameraShake_ = std::min(1.0f, cameraShake_ + 0.7f);
        damageFlash_ = 1.0f;
        SpawnHitBurst(position, Color{255, 200, 60, 255}, 30);
        SpawnShockwave(camera_.position, 3.4f, Color{255, 200, 60, 255});
        // Knockback enemies and enemy projectiles (shield-break-like)
        for (Enemy& enemy : enemies_) {
            Vector3 ep = BodyPosition(enemy.body);
            float dist = Vector3Distance(camera_.position, ep);
            if (dist <= 5.0f + enemy.radius) {
                Vector3 pushDir = Vector3Subtract(ep, camera_.position);
                float pushLen = Vector3Length(pushDir);
                if (pushLen > 0.01f) {
                    pushDir = Vector3Scale(pushDir, 1.0f / pushLen);
                    float falloff = 1.0f - std::clamp(dist / 5.0f, 0.0f, 1.0f);
                    AddEnemyImpulse(enemy, Vector3Scale(pushDir, 22.0f * falloff));
                }
            }
        }
        for (Projectile& proj : projectiles_) {
            if (proj.owner != ProjectileOwner::Enemy && proj.kind != ProjectileKind::EnemyShot) continue;
            Vector3 pp = BodyPosition(proj.body);
            float dist = Vector3Distance(camera_.position, pp);
            if (dist <= 5.0f + proj.radius) {
                Vector3 pushDir = Vector3Subtract(pp, camera_.position);
                float pushLen = Vector3Length(pushDir);
                if (pushLen > 0.01f) {
                    pushDir = Vector3Scale(pushDir, 1.0f / pushLen);
                    float falloff = 1.0f - std::clamp(dist / 5.0f, 0.0f, 1.0f);
                    AddProjectileImpulse(proj, Vector3Scale(pushDir, 22.0f * falloff));
                }
            }
        }
        eventText_ = "ESSENCE LOST";
        eventTextTimer_ = 1.6f;
        return;
    }

    state_ = State::Dead;
    cameraShake_ = 1.0f;
    damageFlash_ = 1.0f;
    SpawnHitBurst(position, color, 28);
    if (text != nullptr) {
        eventText_ = text;
        eventTextTimer_ = 2.0f;
    }
}
Vector3 Game::PlayerForward() const {
    float yaw = yaw_ * kDegToRad;
    float pitch = pitch_ * kDegToRad;
    if (IsSphericalMap()) {
        Vector3 up = SphericalUpAt(camera_.position, playerWorld_);
        Vector3 reference = ProjectOnSphericalTangent(asteroidReferenceForward_, up);
        if (Vector3Length(reference) <= 0.001f) {
            reference = ProjectOnSphericalTangent(Vector3{0.0f, 0.0f, -1.0f}, up);
        }
        if (Vector3Length(reference) <= 0.001f) {
            reference = Vector3Normalize(Vector3CrossProduct(Vector3{1.0f, 0.0f, 0.0f}, up));
        } else {
            reference = Vector3Normalize(reference);
        }
        return Vector3Normalize(Vector3Add(Vector3Scale(reference, std::cos(pitch)), Vector3Scale(up, std::sin(pitch))));
    }
    Vector3 flatForward = Vector3Normalize(Vector3{
        std::cos(pitch) * std::cos(yaw),
        std::sin(pitch),
        std::cos(pitch) * std::sin(yaw)
    });
    if (playerWorld_ == 1) {
        flatForward.y = -flatForward.y;
    }
    return flatForward;
}
Vector3 Game::PlayerRight() const {
    Vector3 up = UpForWorldAt(camera_.position, playerWorld_);
    Vector3 right = Vector3CrossProduct(PlayerForward(), up);
    if (Vector3Length(right) <= 0.001f) {
        right = Vector3{1.0f, 0.0f, 0.0f};
    }
    return Vector3Normalize(right);
}
Vector3 Game::PlayerUp() const {
    return UpForWorldAt(camera_.position, playerWorld_);
}
Vector3 Game::WeaponMuzzlePosition() const {
    Vector3 muzzle = weaponViewModel_.MuzzlePosition(camera_);
    if (activeWeapon_ == WeaponType::MysticStaff) {
        Vector3 up = UpForWorldAt(camera_.position, playerWorld_);
        muzzle = Vector3Add(muzzle, Vector3Scale(up, 0.5f));
    }
    return muzzle;
}
float Game::CurrentGravity() const {
    return config_.gravity * gravityScale_;
}
