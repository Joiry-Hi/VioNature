#include "Game.h"

#include <algorithm>
#include <cmath>

#include "raymath.h"
#include <rlgl.h>

namespace {
constexpr float kFixedFrame = 1.0f / 60.0f;
constexpr float kMouseSensitivity = 0.085f;
constexpr float kDegToRad = 0.017453292519943295f;

Vector3 ToRayVector(JPH::RVec3Arg value) {
    return Vector3{static_cast<float>(value.GetX()), static_cast<float>(value.GetY()), static_cast<float>(value.GetZ())};
}

JPH::RVec3 ToJoltVector(Vector3 value) {
    return JPH::RVec3(value.x, value.y, value.z);
}

JPH::Vec3 ToJoltVelocity(Vector3 value) {
    return JPH::Vec3(value.x, value.y, value.z);
}

Color FadeColor(Color color, float alpha) {
    color.a = static_cast<unsigned char>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f);
    return color;
}

float RandomFloat(float minValue, float maxValue) {
    return minValue + (maxValue - minValue) * (static_cast<float>(GetRandomValue(0, 10000)) / 10000.0f);
}

Vector3 RotateY(Vector3 value, float radians) {
    float c = std::cos(radians);
    float s = std::sin(radians);
    return Vector3{value.x * c - value.z * s, value.y, value.x * s + value.z * c};
}

Vector3 RotateAroundAxis(Vector3 value, Vector3 axis, float radians) {
    if (Vector3Length(axis) <= 0.001f) {
        return value;
    }
    axis = Vector3Normalize(axis);
    float c = std::cos(radians);
    float s = std::sin(radians);
    return Vector3Add(
        Vector3Add(Vector3Scale(value, c), Vector3Scale(Vector3CrossProduct(axis, value), s)),
        Vector3Scale(axis, Vector3DotProduct(axis, value) * (1.0f - c)));
}

Vector3 SafeNormalize(Vector3 value, Vector3 fallback) {
    if (Vector3Length(value) <= 0.001f) {
        return fallback;
    }
    return Vector3Normalize(value);
}
}

Game::Game() {
    config_ = LoadGameplayConfig();
    arenaRadius_ = config_.circleRadius;

    camera_.position = Vector3{0.0f, playerHeight_, 9.0f};
    camera_.target = Vector3{0.0f, playerHeight_, 0.0f};
    camera_.up = Vector3{0.0f, 1.0f, 0.0f};
    camera_.fovy = 72.0f;
    camera_.projection = CAMERA_PERSPECTIVE;

    float floorHalfExtent = std::max(arenaRadius_, squareHalfExtent_);
    floorShape_ = new JPH::BoxShape(JPH::Vec3(floorHalfExtent, 0.5f, floorHalfExtent));
    projectileShape_ = new JPH::SphereShape(0.18f);
    enemyShape_ = new JPH::SphereShape(0.65f);
    pixelTarget_ = LoadRenderTexture(pixelWidth_, pixelHeight_);
    SetTextureFilter(pixelTarget_.texture, TEXTURE_FILTER_POINT);

    bethlehemModel_ = LoadModel("assets/models/bosses/star_of_bethlehem.obj");
    bethlehemModelLoaded_ = IsModelValid(bethlehemModel_);

    Reset();
}

Game::~Game() {
    ClearWorld();
    if (pixelTarget_.id != 0) {
        UnloadRenderTexture(pixelTarget_);
    }
    if (bethlehemModelLoaded_) {
        UnloadModel(bethlehemModel_);
        bethlehemModelLoaded_ = false;
    }
}

void Game::Reset() {
    ClearWorld();

    camera_.position = IsSphericalMap()
        ? SphericalSurfacePoint(Vector3{0.0f, IsHollowWorldMap() ? -1.0f : 1.0f, 0.0f}, SphericalPlayerAltitude())
        : Vector3{0.0f, playerHeight_, 9.0f};
    yaw_ = -90.0f;
    pitch_ = 0.0f;
    playerVelocity_ = Vector3Zero();
    grounded_ = true;
    coyoteTimer_ = 0.0f;
    jumpBufferTimer_ = 0.0f;
    hasSpaceSuit_ = false;
    hasFlightRig_ = false;
    hasSkates_ = false;
    spaceSuitEnabled_ = false;
    flightRigEnabled_ = false;
    skatesEnabled_ = false;
    hideUI_ = false;
    gravityScale_ = 1.0f;
    flightTargetAltitude_ = std::clamp(
        IsSphericalMap() ? SphericalAltitudeAt(camera_.position) : camera_.position.y,
        config_.flightMinAltitude,
        config_.flightMaxAltitude);
    footstepBob_ = 0.0f;
    thrustControlLockTimer_ = 0.0f;
    asteroidReferenceForward_ = Vector3{0.0f, 0.0f, -1.0f};
    camera_.up = IsSphericalMap() ? SphericalUpAt(camera_.position) : Vector3{0.0f, 1.0f, 0.0f};
    camera_.target = Vector3Add(camera_.position, PlayerForward());

    PhysicsWorld::BodyConfig floorConfig;
    floorConfig.motionType = JPH::EMotionType::Static;
    floorConfig.layer = Layers::NON_MOVING;
    if (!IsSphericalMap()) {
        floorBody_ = physics_.CreateBody(
            floorShape_,
            JPH::RVec3(0.0f, -0.55f, 0.0f),
            JPH::Quat::sIdentity(),
            floorConfig,
            JPH::EActivation::DontActivate);
    } else {
        floorBody_ = JPH::BodyID();
    }

    state_ = State::Playing;
    activeWeapon_ = WeaponType::Laser;
    flamethrowerMode_ = FlamethrowerMode::FlameBall;
    rocketLauncherMode_ = RocketLauncherMode::Rocket;
    fireControlActive_ = false;
    rallyPhase_ = RallyPhase::Inactive;
    rallyPoint_ = {};
    rallyHoldTimer_ = 0.0f;
    drones_.clear();
    shotgunMode_ = ShotgunMode::Pellet;
    gravityNailerMode_ = GravityNailerMode::Nail;
    riftCutterMode_ = RiftCutterMode::BladeWave;
    recoilLanceMode_ = RecoilLanceMode::Throw;
    riftPlatformRangeScale_ = 1.0f;
    gauntletMode_ = GauntletMode::TimeStop;
    blinkDistanceScale_ = 1.0f;
    timeStopped_ = false;
    timeStopTintTimer_ = 0.0f;
    fireCooldown_ = 0.0f;
    chargingLaser_ = false;
    laserCharge_ = 0.0f;
    rightMouseHeld_ = 0.0f;
    spawnTimer_ = 0.6f;
    spawnInterval_ = 2.0f;
    waveIndex_ = 1;
    eventTextTimer_ = 2.0f;
    eventText_ = "WAVE 1";
    wispSurgeDone_ = false;
    spitterAmbushDone_ = false;
    pouncerRushDone_ = false;
    bossSpawned_ = false;
    bethlehemSpawned_ = false;
    bethlehem_ = {};
    bethlehem_.laserPhase = BethlehemLaserPhase::Inactive;
    duelWon_ = false;
    nextMixedEventTime_ = 104.0f;
    duelArmor_ = DuelMode() ? config_.duelPlayerArmor : 0;
    duelArmorInvulnTimer_ = 0.0f;
    survivalTime_ = 0.0f;
    cameraShake_ = 0.0f;
    score_ = 0;

    BuildMap();

    SpawnStartingPickups();
    if (DuelMode()) {
        SpawnEnemyOfType(EnemyType::Duelist);
        eventText_ = "DUEL";
        eventTextTimer_ = 2.0f;
    }
}

void Game::ClearWorld() {
    for (const Projectile& projectile : projectiles_) {
        physics_.DestroyBody(projectile.body);
    }
    projectiles_.clear();

    beams_.clear();
    shockwaves_.clear();
    heatwaves_.clear();
    gravityWells_.clear();
    rifts_.clear();
    nanoPlatforms_.clear();

    for (const Enemy& enemy : enemies_) {
        physics_.DestroyBody(enemy.body);
    }
    enemies_.clear();

    physics_.DestroyBody(floorBody_);
    floorBody_ = JPH::BodyID();
    particles_.clear();
    props_.clear();
    pickups_.clear();
}

void Game::Update(float dt) {
    if (IsKeyPressed(KEY_R)) {
        Reset();
    }

    if (IsKeyPressed(KEY_Z) && hasSpaceSuit_) {
        spaceSuitEnabled_ = !spaceSuitEnabled_;
        gravityScale_ = spaceSuitEnabled_ ? config_.spaceSuitGravityScale : 1.0f;
        eventText_ = spaceSuitEnabled_ ? "SUIT ON" : "SUIT OFF";
        eventTextTimer_ = 1.0f;
    }
    if (IsKeyPressed(KEY_X) && hasFlightRig_) {
            flightRigEnabled_ = !flightRigEnabled_;
            if (flightRigEnabled_) {
                flightTargetAltitude_ = std::clamp(
                IsSphericalMap() ? SphericalAltitudeAt(camera_.position) : camera_.position.y,
                config_.flightMinAltitude,
                config_.flightMaxAltitude);
            }
        eventText_ = flightRigEnabled_ ? "FLIGHT ON" : "FLIGHT OFF";
        eventTextTimer_ = 1.0f;
    }
    if (IsKeyPressed(KEY_C) && hasSkates_) {
        skatesEnabled_ = !skatesEnabled_;
        eventText_ = skatesEnabled_ ? "SKATES ON" : "SKATES OFF";
        eventTextTimer_ = 1.0f;
    }
    if (IsKeyPressed(KEY_P)) {
        hideUI_ = !hideUI_;
        eventText_ = hideUI_ ? "HUD OFF" : "HUD ON";
        eventTextTimer_ = 1.0f;
    }

    thrustControlLockTimer_ = std::max(0.0f, thrustControlLockTimer_ - dt);
    UpdateLook(dt);

    if (state_ == State::Playing) {
        if (!timeStopped_) {
            survivalTime_ += dt;
        }
        spawnInterval_ = std::max(0.45f, 1.9f - survivalTime_ * 0.025f);

        UpdatePlayer(dt);
        UpdateWeaponSwitching();
        UpdateShooting(dt);
        UpdateBeam(dt);
        UpdateShockwaves(dt);
        UpdateHeatwaves(dt);
        if (!timeStopped_) {
            UpdateGravityWells(dt);
            UpdateRifts(dt);
            UpdateNanoPlatforms(dt);
        }
        if (!timeStopped_ && !DuelMode()) {
            UpdateWaveDirector(dt);
        }
        if (!timeStopped_) {
            UpdateEnemies(dt);
            UpdateDrones(dt);
            UpdateBethlehem(dt);
            UpdateProjectiles(dt);
            UpdateCollisions();
        }
        UpdatePickups(dt);
        UpdateArenaBounds();

        if (!timeStopped_) {
            physics_.Step(kFixedFrame);
        }
    } else {
        UpdateFreeCamera(dt);
        camera_.target = Vector3Add(camera_.position, PlayerForward());
    }

    UpdateParticles(dt);
    eventTextTimer_ = std::max(0.0f, eventTextTimer_ - dt);
    timeStopTintTimer_ = std::max(0.0f, timeStopTintTimer_ - dt);
    duelArmorInvulnTimer_ = std::max(0.0f, duelArmorInvulnTimer_ - dt);
    cameraShake_ = std::max(0.0f, cameraShake_ - dt * 5.0f);
}

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
        ResolveMapCollision(previousPosition);
    }

    if (cameraShake_ > 0.0f) {
        float shake = cameraShake_ * 0.035f;
        camera_.position.x += RandomFloat(-shake, shake);
        camera_.position.y += RandomFloat(-shake, shake);
    }

    camera_.up = IsSphericalMap() ? SphericalUpAt(camera_.position) : Vector3{0.0f, 1.0f, 0.0f};
    camera_.target = Vector3Add(camera_.position, PlayerForward());
}

void Game::UpdateLook(float dt) {
    Vector2 delta = GetMouseDelta();
    if (IsSphericalMap()) {
        Vector3 up = SphericalUpAt(camera_.position);
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
        yaw_ += delta.x * kMouseSensitivity;
    }
    pitch_ -= delta.y * kMouseSensitivity;
    pitch_ = std::clamp(pitch_, -89.0f, 89.0f);
    (void)dt;
}

void Game::UpdateFreeCamera(float dt) {
    Vector3 forward = PlayerForward();
    Vector3 right = PlayerRight();
    Vector3 up = IsSphericalMap() ? SphericalUpAt(camera_.position) : Vector3{0.0f, 1.0f, 0.0f};

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
        Vector3 up = SphericalUpAt(camera_.position);
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
            float currentAltitude = SphericalAltitudeAt(camera_.position);
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
        up = SphericalUpAt(camera_.position);
        float desiredRadius = SphericalSignedRadius(SphericalPlayerAltitude());
        float distance = Vector3Length(camera_.position);
        float radialSpeed = Vector3DotProduct(playerVelocity_, up);
        bool touchesSurface = IsHollowWorldMap() ? distance >= desiredRadius : distance <= desiredRadius;
        if (!flightRigEnabled_ && touchesSurface) {
            camera_.position = SphericalSurfacePoint(camera_.position, SphericalPlayerAltitude());
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

    Vector3 right = PlayerRight();
    right.y = 0.0f;
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
        float altitudeError = flightTargetAltitude_ - camera_.position.y;
        float verticalAcceleration = altitudeError * config_.flightHoverStrength - playerVelocity_.y * config_.flightHoverDamping;
        playerVelocity_.y += verticalAcceleration * dt;
        grounded_ = false;
        coyoteTimer_ = 0.0f;
        jumpBufferTimer_ = 0.0f;
    } else if (jumpBufferTimer_ > 0.0f && coyoteTimer_ > 0.0f) {
        playerVelocity_.y = config_.jumpSpeed;
        grounded_ = false;
        coyoteTimer_ = 0.0f;
        jumpBufferTimer_ = 0.0f;
        cameraShake_ = std::min(1.0f, cameraShake_ + 0.12f);
    }

    if (!flightRigEnabled_) {
        playerVelocity_.y -= CurrentGravity() * dt;
    }
    camera_.position = Vector3Add(camera_.position, Vector3Scale(playerVelocity_, dt));

    float floorHeight = playerHeight_;
    if (!flightRigEnabled_ && camera_.position.y <= floorHeight) {
        camera_.position.y = floorHeight;
        playerVelocity_.y = 0.0f;
        grounded_ = true;
        coyoteTimer_ = kCoyoteTime;
    } else {
        grounded_ = false;
    }

    float horizontalSpeed = std::sqrt(playerVelocity_.x * playerVelocity_.x + playerVelocity_.z * playerVelocity_.z);
    footstepBob_ += horizontalSpeed * dt * (running ? 1.5f : 1.0f);
    if (grounded_ && horizontalSpeed > 0.5f) {
        camera_.position.y += std::sin(footstepBob_ * 7.0f) * 0.035f;
    }
}

void Game::UpdateWeaponSwitching() {
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        rightMouseHeld_ = 0.0f;
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        rightMouseHeld_ += GetFrameTime();
        if (activeWeapon_ == WeaponType::RocketLauncher && rightMouseHeld_ > 0.22f && state_ == State::Playing) {
            fireControlActive_ = true;
        }
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
        fireControlActive_ = false;
    }

    WeaponType previousWeapon = activeWeapon_;
    float wheel = GetMouseWheelMove();
    bool wheelAdjustedPlatformRange = false;
    if (wheel != 0.0f && activeWeapon_ == WeaponType::RiftCutter && riftCutterMode_ == RiftCutterMode::Platform) {
        riftPlatformRangeScale_ = std::clamp(riftPlatformRangeScale_ + (wheel > 0.0f ? 0.08f : -0.08f), 0.35f, 1.85f);
        wheelAdjustedPlatformRange = true;
    }
    if (wheel != 0.0f && activeWeapon_ == WeaponType::InfinityGauntlet && gauntletMode_ == GauntletMode::Blink) {
        constexpr float kBlinkStep = 1.2f;
        blinkDistanceScale_ = std::clamp(blinkDistanceScale_ * (wheel > 0.0f ? kBlinkStep : 1.0f / kBlinkStep), config_.blinkDistanceMin, config_.blinkDistanceMax);
        wheelAdjustedPlatformRange = true;
    }
    if (IsKeyPressed(KEY_ONE)) {
        activeWeapon_ = WeaponType::Laser;
    } else if (IsKeyPressed(KEY_TWO)) {
        activeWeapon_ = WeaponType::Flamethrower;
    } else if (IsKeyPressed(KEY_THREE)) {
        activeWeapon_ = WeaponType::RocketLauncher;
    } else if (IsKeyPressed(KEY_FOUR)) {
        activeWeapon_ = WeaponType::Shotgun;
    } else if (IsKeyPressed(KEY_FIVE)) {
        activeWeapon_ = WeaponType::GravityNailer;
    } else if (IsKeyPressed(KEY_SIX)) {
        activeWeapon_ = WeaponType::InfinityGauntlet;
    } else if (IsKeyPressed(KEY_SEVEN)) {
        activeWeapon_ = WeaponType::RecoilLance;
    } else if (IsKeyPressed(KEY_EIGHT)) {
        activeWeapon_ = WeaponType::RiftCutter;
    } else if (wheel != 0.0f && !wheelAdjustedPlatformRange) {
        int index = 0;
        if (activeWeapon_ == WeaponType::Flamethrower) {
            index = 1;
        } else if (activeWeapon_ == WeaponType::RocketLauncher) {
            index = 2;
        } else if (activeWeapon_ == WeaponType::Shotgun) {
            index = 3;
        } else if (activeWeapon_ == WeaponType::GravityNailer) {
            index = 4;
        } else if (activeWeapon_ == WeaponType::InfinityGauntlet) {
            index = 5;
        } else if (activeWeapon_ == WeaponType::RecoilLance) {
            index = 6;
        } else if (activeWeapon_ == WeaponType::RiftCutter) {
            index = 7;
        }

        index = (index + (wheel > 0.0f ? -1 : 1) + 8) % 8;
        if (index == 0) {
            activeWeapon_ = WeaponType::Laser;
        } else if (index == 1) {
            activeWeapon_ = WeaponType::Flamethrower;
        } else if (index == 2) {
            activeWeapon_ = WeaponType::RocketLauncher;
        } else if (index == 3) {
            activeWeapon_ = WeaponType::Shotgun;
        } else if (index == 4) {
            activeWeapon_ = WeaponType::GravityNailer;
        } else if (index == 5) {
            activeWeapon_ = WeaponType::InfinityGauntlet;
        } else if (index == 6) {
            activeWeapon_ = WeaponType::RecoilLance;
        } else {
            activeWeapon_ = WeaponType::RiftCutter;
        }
    }

    if (activeWeapon_ != WeaponType::Laser && IsMouseButtonReleased(MOUSE_BUTTON_RIGHT) && rightMouseHeld_ < 0.22f) {
        if (activeWeapon_ == WeaponType::Flamethrower) {
            flamethrowerMode_ = flamethrowerMode_ == FlamethrowerMode::FlameBall ? FlamethrowerMode::Heatwave : FlamethrowerMode::FlameBall;
            eventText_ = flamethrowerMode_ == FlamethrowerMode::Heatwave ? "HEATWAVE" : "FLAME BALL";
            eventTextTimer_ = 1.4f;
        } else if (activeWeapon_ == WeaponType::Shotgun) {
            shotgunMode_ = shotgunMode_ == ShotgunMode::Pellet ? ShotgunMode::GlassShard : ShotgunMode::Pellet;
            eventText_ = shotgunMode_ == ShotgunMode::GlassShard ? "GLASS SHARDS" : "PELLETS";
            eventTextTimer_ = 1.4f;
        } else if (activeWeapon_ == WeaponType::GravityNailer) {
            gravityNailerMode_ = gravityNailerMode_ == GravityNailerMode::Nail ? GravityNailerMode::BlackHole : GravityNailerMode::Nail;
            eventText_ = gravityNailerMode_ == GravityNailerMode::BlackHole ? "BLACK HOLE" : "GRAV NAIL";
            eventTextTimer_ = 1.4f;
        } else if (activeWeapon_ == WeaponType::RecoilLance) {
            recoilLanceMode_ = recoilLanceMode_ == RecoilLanceMode::Throw ? RecoilLanceMode::Thrust : RecoilLanceMode::Throw;
            eventText_ = recoilLanceMode_ == RecoilLanceMode::Thrust ? "SONIC THRUST" : "LANCE THROW";
            eventTextTimer_ = 1.4f;
        } else if (activeWeapon_ == WeaponType::RocketLauncher) {
            rocketLauncherMode_ = rocketLauncherMode_ == RocketLauncherMode::Rocket ? RocketLauncherMode::Drone : RocketLauncherMode::Rocket;
            eventText_ = rocketLauncherMode_ == RocketLauncherMode::Drone ? "DRONE" : "ROCKET";
            eventTextTimer_ = 1.4f;
        } else if (activeWeapon_ == WeaponType::RiftCutter) {
            riftCutterMode_ = riftCutterMode_ == RiftCutterMode::BladeWave ? RiftCutterMode::Platform : RiftCutterMode::BladeWave;
            eventText_ = riftCutterMode_ == RiftCutterMode::Platform ? "NANO PLATFORM" : "BLADE WAVE";
            eventTextTimer_ = 1.4f;
        } else if (activeWeapon_ == WeaponType::InfinityGauntlet) {
            gauntletMode_ = gauntletMode_ == GauntletMode::TimeStop ? GauntletMode::Blink : GauntletMode::TimeStop;
            eventText_ = gauntletMode_ == GauntletMode::Blink ? "BLINK" : "TIME STOP";
            eventTextTimer_ = 1.4f;
        }
    }

    if (activeWeapon_ != previousWeapon) {
        chargingLaser_ = false;
        laserCharge_ = 0.0f;
        fireCooldown_ = std::min(fireCooldown_, 0.12f);
    }
}

void Game::UpdateShooting(float dt) {
    fireCooldown_ = std::max(0.0f, fireCooldown_ - dt);

    if (fireControlActive_) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            rallyPoint_ = GetFireControlAimPoint();
            rallyPhase_ = RallyPhase::Assembling;
            rallyHoldTimer_ = config_.droneRallyHoldTime;
            fireControlActive_ = false;
            eventText_ = "RALLY SET";
            eventTextTimer_ = 1.4f;
        }
        return;
    }

    if (activeWeapon_ == WeaponType::InfinityGauntlet) {
        if (gauntletMode_ == GauntletMode::TimeStop) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && config_.timeStopEnabled) {
                ToggleTimeStop();
            }
        } else {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && config_.blinkEnabled) {
                Blink();
            }
        }
        return;
    }

    bool laserChord = activeWeapon_ == WeaponType::Laser && IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    if (laserChord && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        chargingLaser_ = true;
        laserCharge_ = 0.0f;
    }
    if (chargingLaser_) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && laserChord) {
            laserCharge_ = std::min(1.0f, laserCharge_ + dt * 0.72f);
            cameraShake_ = std::min(1.0f, cameraShake_ + dt * 0.25f);
        } else {
            FireLaser(laserCharge_);
            chargingLaser_ = false;
            laserCharge_ = 0.0f;
            fireCooldown_ = 0.18f;
        }
        return;
    }

    if (laserChord || !IsMouseButtonDown(MOUSE_BUTTON_LEFT) || fireCooldown_ > 0.0f) {
        return;
    }

    Vector3 forward = PlayerForward();
    Vector3 right = PlayerRight();
    Vector3 up = PlayerUp();

    if (activeWeapon_ == WeaponType::Laser) {
        FireProjectile(ProjectileKind::LaserShot, forward, 68.0f, config_.daggerDamage, 1.65f, 0.18f, 0.18f, Color{255, 240, 185, 255});
        fireCooldown_ = 0.075f;
        cameraShake_ = std::min(1.0f, cameraShake_ + 0.15f);
    } else if (activeWeapon_ == WeaponType::Flamethrower) {
        if (flamethrowerMode_ == FlamethrowerMode::Heatwave) {
            FireHeatwave(forward);
            fireCooldown_ = 0.16f;
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.16f);
        } else {
            float side = RandomFloat(-0.05f, 0.05f);
            float lift = RandomFloat(-0.035f, 0.035f);
            Vector3 direction = Vector3Normalize(Vector3Add(forward, Vector3Add(Vector3Scale(right, side), Vector3Scale(up, lift))));
            FireProjectile(ProjectileKind::Flame, direction, RandomFloat(19.0f, 23.0f), config_.flameDamage, 0.5f, 0.12f, 0.82f, Color{255, 112, 28, 235});
            fireCooldown_ = 0.045f;
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.045f);
        }
    } else if (activeWeapon_ == WeaponType::RocketLauncher) {
        if (rocketLauncherMode_ == RocketLauncherMode::Drone) {
            FireDroneCanister();
            fireCooldown_ = 1.8f;
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.25f);
        } else {
            FireProjectile(ProjectileKind::Rocket, forward, 34.0f, config_.rocketImpactDamage, 2.8f, 0.34f, 0.34f, Color{230, 235, 210, 255});
            fireCooldown_ = 0.82f;
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.45f);
        }
    } else if (activeWeapon_ == WeaponType::Shotgun) {
        if (shotgunMode_ == ShotgunMode::GlassShard) {
            for (int i = 0; i < config_.shotgunShardCount; ++i) {
                float side = RandomFloat(-0.09f, 0.09f);
                float lift = RandomFloat(-0.055f, 0.055f);
                Vector3 direction = Vector3Normalize(Vector3Add(forward, Vector3Add(Vector3Scale(right, side), Vector3Scale(up, lift))));
                FireProjectile(ProjectileKind::GlassShard, direction, config_.glassShardSpeed, config_.glassShardDamage, config_.glassShardLingerTime, 0.13f, 0.13f, Color{190, 245, 255, 255});
            }
            ApplyShotgunRecoil(Vector3Scale(forward, config_.glassShardRecoilScale));
            fireCooldown_ = 0.72f;
        } else {
            for (int i = 0; i < config_.shotgunPelletCount; ++i) {
                float side = RandomFloat(-0.18f, 0.18f);
                float lift = RandomFloat(-0.12f, 0.12f);
                Vector3 direction = Vector3Normalize(Vector3Add(forward, Vector3Add(Vector3Scale(right, side), Vector3Scale(up, lift))));
                FireProjectile(ProjectileKind::Pellet, direction, RandomFloat(48.0f, 58.0f), config_.shotgunPelletDamage, 0.62f, 0.11f, 0.11f, Color{255, 220, 150, 255});
            }
            ApplyShotgunRecoil(forward);
            fireCooldown_ = 0.58f;
        }
        cameraShake_ = std::min(1.0f, cameraShake_ + 0.42f);
    } else if (activeWeapon_ == WeaponType::GravityNailer) {
        if (gravityNailerMode_ == GravityNailerMode::BlackHole) {
            FireProjectile(ProjectileKind::BlackHoleGrenade, forward, 26.0f, config_.blackHoleGrenadeDamage, 1.65f, 0.28f, 0.28f, Color{90, 55, 165, 255});
            fireCooldown_ = 1.15f;
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.34f);
        } else {
            FireProjectile(ProjectileKind::GravityNail, forward, 76.0f, config_.gravityNailDamage, 1.1f, 0.15f, 0.15f, Color{165, 195, 255, 255});
            fireCooldown_ = 0.72f;
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.28f);
        }
    } else if (activeWeapon_ == WeaponType::RecoilLance) {
        if (recoilLanceMode_ == RecoilLanceMode::Thrust) {
            FireLanceThrust(forward);
            fireCooldown_ = 0.68f;
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.58f);
        } else {
            FireProjectile(ProjectileKind::Lance, forward, config_.recoilLanceSpeed, config_.recoilLanceDamage, 1.15f, 0.28f, 0.28f, Color{210, 245, 255, 255});
            ApplyLanceRecoil(forward);
            fireCooldown_ = 0.86f;
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.5f);
        }
    } else if (activeWeapon_ == WeaponType::RiftCutter) {
        if (riftCutterMode_ == RiftCutterMode::Platform) {
            FireNanoPlatform(forward);
            fireCooldown_ = 0.82f;
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.18f);
        } else {
            FireRiftCutter(forward);
            fireCooldown_ = 0.78f;
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.38f);
        }
    }

    SpawnHitBurst(WeaponMuzzlePosition(), Color{255, 230, 180, 255}, 2);
}

void Game::UpdateBeam(float dt) {
    for (size_t i = 0; i < beams_.size();) {
        beams_[i].life -= dt;
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
            Vector3 up = IsSphericalMap() ? SphericalUpAt(player) : Vector3{0.0f, 1.0f, 0.0f};
            Vector3 capsuleBottom = IsSphericalMap()
                ? Vector3Subtract(player, Vector3Scale(up, SphericalPlayerAltitude() - playerRadius_))
                : Vector3{player.x, player.y - playerHeight_ + playerRadius_, player.z};
            Vector3 capsuleTop = IsSphericalMap()
                ? Vector3Subtract(player, Vector3Scale(up, playerRadius_ * 0.35f))
                : Vector3{player.x, player.y - playerRadius_ * 0.35f, player.z};
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

void Game::UpdateRifts(float dt) {
    for (size_t riftIndex = 0; riftIndex < rifts_.size();) {
        RiftSlash& rift = rifts_[riftIndex];
        if (rift.delay > 0.0f) {
            rift.delay -= dt;
            if (rift.delay <= 0.0f) {
                SpawnHitBurst(rift.center, Color{255, 235, 150, 255}, 18);
            }
            ++riftIndex;
            continue;
        }

        rift.life -= dt;
        if (rift.life <= 0.0f) {
            rifts_[riftIndex] = rifts_.back();
            rifts_.pop_back();
            continue;
        }
        rift.center = Vector3Add(rift.center, Vector3Scale(rift.velocity, dt));

        bool outsideBounds = false;
        if (IsSphericalMap()) {
            float distance = Vector3Length(rift.center);
            outsideBounds = IsHollowWorldMap()
                ? (distance > SphericalRadius() + rift.radius || distance < std::max(1.0f, SphericalRadius() - SphericalCleanupDistance()))
                : (distance > SphericalCleanupDistance() || distance < SphericalRadius() - rift.radius);
        } else {
            outsideBounds = IsSquareMap()
                ? (std::abs(rift.center.x) > squareHalfExtent_ + rift.radius || std::abs(rift.center.z) > squareHalfExtent_ + rift.radius)
                : (Vector3Length(Vector3{rift.center.x, 0.0f, rift.center.z}) > arenaRadius_ + rift.radius);
            outsideBounds = outsideBounds || rift.center.y < -rift.radius || rift.center.y > 48.0f;
        }
        if (outsideBounds) {
            rifts_[riftIndex] = rifts_.back();
            rifts_.pop_back();
            continue;
        }

        auto insideRift = [&](Vector3 point, float radiusPadding) {
            Vector3 offset = Vector3Subtract(point, rift.center);
            float planeDistance = std::abs(Vector3DotProduct(offset, rift.normal));
            float horizontal = Vector3DotProduct(offset, rift.right);
            float vertical = Vector3DotProduct(offset, rift.up);
            float radius = std::sqrt(horizontal * horizontal + vertical * vertical);
            float normalizedVertical = vertical / std::max(0.001f, rift.radius);
            float crescentCenter = std::sqrt(std::max(0.0f, 1.0f - horizontal * horizontal / std::max(0.001f, rift.radius * rift.radius))) * rift.radius * 0.34f;
            float arcDistance = std::abs(vertical - crescentCenter);
            return planeDistance <= rift.planeThickness + radiusPadding * 0.45f
                && radius <= rift.radius + radiusPadding
                && radius >= rift.radius - rift.thickness - radiusPadding
                && normalizedVertical > -0.55f
                && arcDistance <= rift.thickness * 0.72f + radiusPadding * 0.45f;
        };

        if (rift.owner == ProjectileOwner::Enemy && insideRift(camera_.position, playerRadius_)) {
            ApplyPlayerHit(camera_.position, Color{255, 210, 120, 255});
        }

        if (rift.owner == ProjectileOwner::Player) {
            for (size_t enemyIndex = 0; enemyIndex < enemies_.size();) {
                Enemy& enemy = enemies_[enemyIndex];
                Vector3 enemyPosition = BodyPosition(enemy.body);
                if (insideRift(enemyPosition, enemy.radius)) {
                    enemy.health -= rift.damagePerSecond * dt;
                    Vector3 offset = Vector3Subtract(enemyPosition, rift.center);
                    float horizontal = Vector3DotProduct(offset, rift.right);
                    float vertical = Vector3DotProduct(offset, rift.up);
                    Vector3 push = Vector3Add(rift.normal, Vector3Scale(Vector3Normalize(Vector3Add(Vector3Scale(rift.right, horizontal), Vector3Scale(rift.up, vertical))), 0.35f));
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

        ++riftIndex;
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
            nanoPlatforms_[i] = nanoPlatforms_.back();
            nanoPlatforms_.pop_back();
            continue;
        }
        ++i;
    }
}

void Game::UpdateEnemies(float dt) {
    Vector3 player = camera_.position;

    for (Enemy& enemy : enemies_) {
        Vector3 position = BodyPosition(enemy.body);
        Vector3 direction = Vector3Subtract(player, position);
        Vector3 enemyUp = IsSphericalMap() ? SphericalUpAt(position) : Vector3{0.0f, 1.0f, 0.0f};
        if (IsSphericalMap()) {
            direction = ProjectOnSphericalTangent(direction, enemyUp);
        } else {
            direction.y = 0.0f;
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
            float distance = IsSphericalMap() ? Vector3Length(ProjectOnSphericalTangent(Vector3Subtract(player, position), enemyUp)) : DistanceXZ(position, player);
            float rangeBias = distance < 10.0f ? -0.65f : 0.35f;
            direction = Vector3Normalize(Vector3Add(Vector3Scale(direction, rangeBias), Vector3Scale(tangent, 0.9f)));
            speed = enemy.speed;
            if (enemy.cooldownTimer <= 0.0f) {
                Vector3 shotOrigin = Vector3Add(position, Vector3Scale(enemyUp, enemy.radius * 0.35f));
                Vector3 shotDirection = Vector3Normalize(Vector3Subtract(camera_.position, shotOrigin));
                FireEnemyShot(shotOrigin, shotDirection);
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
            float distance = IsSphericalMap() ? Vector3Length(ProjectOnSphericalTangent(Vector3Subtract(player, position), enemyUp)) : DistanceXZ(position, player);
            float rangeBias = distance < 13.0f ? -0.45f : 0.35f;
            float sway = std::sin(enemy.bobTimer * 2.1f) * 0.35f;
            direction = Vector3Normalize(Vector3Add(Vector3Scale(direction, rangeBias), Vector3Scale(tangent, 1.0f + sway)));
            speed = enemy.speed;
            if (enemy.cooldownTimer <= 0.0f) {
                Vector3 shotOrigin = Vector3Subtract(position, Vector3Scale(enemyUp, enemy.radius * 0.15f));
                Vector3 shotDirection = Vector3Normalize(Vector3Subtract(camera_.position, shotOrigin));
                FireEnemyProjectile(ProjectileKind::EnemyShot, shotOrigin, shotDirection, config_.enemyShotSpeed * 0.82f, config_.enemyShotDamage, 3.4f, 0.22f, 0.22f, Color{150, 245, 255, 255});
                enemy.cooldownTimer = config_.harrierFireInterval;
            }
        } else if (enemy.type == EnemyType::Blinker) {
            speed = enemy.speed;
            if (enemy.telegraphTimer > 0.0f) {
                enemy.telegraphTimer = std::max(0.0f, enemy.telegraphTimer - dt);
                direction = Vector3Scale(direction, 0.05f);
                if (enemy.telegraphTimer <= 0.0f) {
                    Vector3 playerForward = PlayerForward();
                    if (IsSphericalMap()) {
                        playerForward = ProjectOnSphericalTangent(playerForward, SphericalUpAt(camera_.position));
                    } else {
                        playerForward.y = 0.0f;
                    }
                    if (Vector3Length(playerForward) <= 0.001f) {
                        playerForward = direction;
                    } else {
                        playerForward = Vector3Normalize(playerForward);
                    }
                    Vector3 playerUp = IsSphericalMap() ? SphericalUpAt(camera_.position) : Vector3{0.0f, 1.0f, 0.0f};
                    Vector3 playerRight = SafeNormalize(Vector3CrossProduct(playerForward, playerUp), PlayerRight());
                    float side = GetRandomValue(0, 1) == 0 ? -1.0f : 1.0f;
                    Vector3 target = Vector3Add(camera_.position, Vector3Add(Vector3Scale(playerRight, side * RandomFloat(3.2f, 5.6f)), Vector3Scale(playerForward, RandomFloat(-4.2f, 2.0f))));
                    if (IsSphericalMap()) {
                        target = SphericalSurfacePoint(target, SphericalEnemyAltitude(enemy.type));
                    } else if (IsSquareMap()) {
                        target.y = 1.0f;
                        float limit = squareHalfExtent_ - enemy.radius - 0.8f;
                        target.x = std::clamp(target.x, -limit, limit);
                        target.z = std::clamp(target.z, -limit, limit);
                    } else {
                        target.y = 1.0f;
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
                        Vector3 targetUp = SphericalUpAt(target);
                        dash = Vector3Add(ProjectOnSphericalTangent(dash, targetUp), Vector3Scale(targetUp, 0.1f));
                    } else {
                        dash.y = 0.1f;
                    }
                    if (Vector3Length(dash) > 0.001f) {
                        dash = Vector3Normalize(dash);
                    }
                    Vector3 dashVelocity = IsSphericalMap()
                        ? Vector3Add(Vector3Scale(dash, config_.blinkerDashSpeed), Vector3Scale(SphericalUpAt(target), 1.6f))
                        : Vector3{dash.x * config_.blinkerDashSpeed, 1.6f, dash.z * config_.blinkerDashSpeed};
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
            float distance = IsSphericalMap() ? Vector3Length(ProjectOnSphericalTangent(Vector3Subtract(player, position), enemyUp)) : DistanceXZ(position, player);
            float rangeBias = distance < 16.0f ? -0.45f : 0.35f;
            direction = Vector3Normalize(Vector3Add(Vector3Scale(direction, rangeBias), Vector3Scale(tangent, 0.85f)));
            speed = enemy.speed;

            if (enemy.cooldownTimer <= 0.0f) {
                FireBossRing(Vector3Add(position, Vector3Scale(enemyUp, enemy.radius * 0.35f)), enemy.health < enemy.maxHealth * 0.45f ? 18 : 12, enemy.health < enemy.maxHealth * 0.45f ? 1.25f : 1.0f);
                enemy.cooldownTimer = enemy.health < enemy.maxHealth * 0.45f ? 1.55f : 2.15f;
                enemy.burstTimer = 0.46f;
                cameraShake_ = std::min(1.0f, cameraShake_ + 0.12f);
            } else if (enemy.burstTimer > 0.0f && enemy.burstTimer < 0.12f) {
                Vector3 shotOrigin = Vector3Add(position, Vector3Scale(enemyUp, enemy.radius * 0.55f));
                Vector3 shotDirection = Vector3Normalize(Vector3Subtract(camera_.position, shotOrigin));
                FireEnemyShot(shotOrigin, shotDirection);
                enemy.burstTimer = 0.0f;
            }
        } else if (enemy.type == EnemyType::Duelist) {
            UpdateDuelist(enemy, position, direction, dt, speed, skipVelocity);
        }

        if (skipVelocity) {
            continue;
        }

        Vector3 velocity = {};
        if (IsSphericalMap()) {
            float targetAltitude = enemy.type == EnemyType::Harrier ? config_.harrierTargetHeight + std::sin(enemy.bobTimer * 2.6f) * 1.2f : SphericalEnemyAltitude(enemy.type);
            float targetDistance = SphericalSignedRadius(targetAltitude);
            float radialCorrection = std::clamp((targetDistance - Vector3Length(position)) * 7.0f, -12.0f, 8.0f);
            if (IsHollowWorldMap()) {
                radialCorrection = -radialCorrection;
            }
            if (enemy.type == EnemyType::Pouncer) {
                JPH::Vec3 current = physics_.Bodies().GetLinearVelocity(enemy.body);
                radialCorrection = Vector3DotProduct(ToRayVector(current), enemyUp);
            }
            velocity = Vector3Add(Vector3Add(Vector3Scale(direction, speed), Vector3Scale(enemyUp, radialCorrection)), enemy.externalVelocity);
        } else {
            float verticalVelocity = 0.0f;
            if (enemy.type == EnemyType::Pouncer) {
                verticalVelocity = physics_.Bodies().GetLinearVelocity(enemy.body).GetY();
            } else {
                float targetHeight = enemy.type == EnemyType::Harrier ? config_.harrierTargetHeight + std::sin(enemy.bobTimer * 2.6f) * 1.2f : enemy.type == EnemyType::Blinker ? 1.0f : enemy.type == EnemyType::Wisp || enemy.type == EnemyType::Spitter ? 1.35f : enemy.type == EnemyType::Boss ? 2.2f : enemy.type == EnemyType::Duelist ? 1.2f : 0.8f;
                verticalVelocity = std::clamp((targetHeight - position.y) * 7.0f, -12.0f, 8.0f);
            }
            velocity = Vector3{
                direction.x * speed + enemy.externalVelocity.x,
                verticalVelocity + enemy.externalVelocity.y,
                direction.z * speed + enemy.externalVelocity.z
            };
        }
        enemy.externalVelocity = Vector3Scale(enemy.externalVelocity, std::pow(0.12f, dt));
        physics_.Bodies().SetLinearVelocity(enemy.body, ToJoltVelocity(velocity));

        if (EnemyTouchesPlayer(position, enemy.radius)) {
            ApplyPlayerHit(player, Color{255, 35, 25, 255});
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

void Game::UpdateProjectiles(float dt) {
    for (size_t i = 0; i < projectiles_.size();) {
        Projectile& projectile = projectiles_[i];
        projectile.life -= dt;
        Vector3 position = BodyPosition(projectile.body);

        if (IsSphericalMap() && (projectile.kind == ProjectileKind::Rocket || projectile.kind == ProjectileKind::BlackHoleGrenade)) {
            Vector3 gravity = Vector3Scale(SphericalUpAt(position), -CurrentGravity() * (projectile.kind == ProjectileKind::BlackHoleGrenade ? 0.45f : 0.02f) * dt);
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

        if (projectile.owner == ProjectileOwner::Enemy && projectile.kind != ProjectileKind::EnemyShot && EnemyTouchesPlayer(position, projectile.radius)) {
            if (projectile.kind == ProjectileKind::Rocket) {
                ExplodeRocket(position, projectile.owner);
            } else if (projectile.kind == ProjectileKind::BlackHoleGrenade) {
                SpawnGravityWell(position, true);
            } else {
                ApplyPlayerHit(camera_.position, projectile.color);
            }
            DestroyProjectile(i);
            continue;
        }

        bool detonatesOnGround = projectile.kind == ProjectileKind::Rocket || projectile.kind == ProjectileKind::GravityNail || projectile.kind == ProjectileKind::BlackHoleGrenade || projectile.kind == ProjectileKind::Lance || projectile.kind == ProjectileKind::DroneCanister;
        bool touchesGround = IsSphericalMap()
            ? (IsHollowWorldMap()
                ? Vector3Length(position) >= SphericalRadius() - projectile.radius
                : Vector3Length(position) <= SphericalRadius() + projectile.radius)
            : position.y <= 0.22f;
        bool hitGround = IsSphericalMap() ? touchesGround : detonatesOnGround && touchesGround;
        bool outOfBounds = false;
        if (IsSphericalMap()) {
            float distance = Vector3Length(position);
            outOfBounds = IsHollowWorldMap()
                ? (distance > SphericalRadius() + 6.0f || distance < std::max(1.0f, SphericalRadius() - SphericalCleanupDistance()))
                : distance > SphericalCleanupDistance();
        } else {
            outOfBounds = IsSquareMap()
                ? (std::abs(position.x) > squareHalfExtent_ + 6.0f || std::abs(position.z) > squareHalfExtent_ + 6.0f)
                : DistanceXZ(position, Vector3Zero()) > arenaRadius_ + 6.0f;
        }
        bool expired = projectile.life <= 0.0f || outOfBounds || hitGround;

        if (IsSphericalMap() && touchesGround && projectile.kind == ProjectileKind::LaserShot) {
            Vector3 normal = SphericalUpAt(position);
            Vector3 velocity = projectile.frozen ? projectile.storedVelocity : ToRayVector(physics_.Bodies().GetLinearVelocity(projectile.body));
            float inwardSpeed = Vector3DotProduct(velocity, normal);
            if (inwardSpeed < 0.0f) {
                velocity = Vector3Subtract(velocity, Vector3Scale(normal, inwardSpeed * 2.0f));
                Vector3 corrected = SphericalSurfacePoint(position, projectile.radius + 0.04f);
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
            SpawnGravityWell(position);
            DestroyProjectile(i);
            continue;
        }

        if (expired && projectile.kind == ProjectileKind::BlackHoleGrenade) {
            SpawnGravityWell(position, true);
            DestroyProjectile(i);
            continue;
        }

        if (projectile.kind == ProjectileKind::EnemyShot && projectile.owner == ProjectileOwner::Enemy && EnemyTouchesPlayer(position, projectile.radius)) {
            ApplyPlayerHit(camera_.position, Color{120, 245, 255, 255});
            DestroyProjectile(i);
            continue;
        }

        if (expired && projectile.kind == ProjectileKind::Rocket) {
            ExplodeRocket(position, projectile.owner);
            DestroyProjectile(i);
            continue;
        }

        if (expired && projectile.kind == ProjectileKind::Lance) {
            DetonateLance(position, projectile.owner);
            DestroyProjectile(i);
            continue;
        }

        if (expired && projectile.kind == ProjectileKind::DroneCanister) {
            drones_.push_back(Drone{position, Vector3Zero(), config_.droneDeployTime, 0.0f, config_.droneRocketInterval, RandomFloat(0.0f, 6.28f), config_.droneLifetime, DroneState::Deploying});
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
            if (p.kind != ProjectileKind::GlassShard) continue;
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
                if (p.kind != ProjectileKind::GlassShard) continue;
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
                        if (other.kind != ProjectileKind::GlassShard) continue;
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
            } else if (pickup.type == PickupType::Skates) {
                hasSkates_ = true;
                skatesEnabled_ = true;
                SpawnHitBurst(pickup.position, Color{165, 255, 185, 255}, 28);
                cameraShake_ = std::min(1.0f, cameraShake_ + 0.16f);
                eventText_ = "SKATES";
                eventTextTimer_ = 1.4f;
            }

            pickups_[i] = pickups_.back();
            pickups_.pop_back();
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
        if (rallyPhase_ == RallyPhase::Assembling || rallyPhase_ == RallyPhase::Holding) {
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
        Vector3 up = IsSphericalMap() ? SphericalUpAt(drones_[i].position) : Vector3{0.0f, 1.0f, 0.0f};
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
        Vector3 up = IsSphericalMap() ? SphericalUpAt(drone.position) : Vector3{0.0f, 1.0f, 0.0f};

        // Find nearest enemy
        Vector3 targetPos = drone.position;
        float nearestDist = 1000.0f;
        bool hasTarget = false;
        for (const Enemy& enemy : enemies_) {
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

        if (rallyPhase_ == RallyPhase::Assembling) {
            Vector3 toRally = Vector3Subtract(rallyPoint_, drone.position);
            Vector3 rallyDir = IsSphericalMap()
                ? ProjectOnSphericalTangent(toRally, up)
                : Vector3{toRally.x, 0.0f, toRally.z};
            float rallyDist = Vector3Length(rallyDir);
            if (rallyDist > 0.3f) {
                rallyDir = Vector3Normalize(rallyDir);
                desiredVel = Vector3Scale(rallyDir, config_.droneMoveSpeed);
            }
        } else if (rallyPhase_ == RallyPhase::Holding) {
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
        float currentAlt = IsSphericalMap() ? SphericalAltitudeAt(drone.position) : drone.position.y;
        float altError = targetAltitude - currentAlt;
        float verticalSpeed = Vector3DotProduct(drone.velocity, up);
        float verticalAccel = altError * 12.0f - verticalSpeed * 5.0f;
        float newVerticalSpeed = verticalSpeed + verticalAccel * dt;

        drone.velocity = Vector3Add(horizVel, Vector3Scale(up, newVerticalSpeed));

        drone.position = Vector3Add(drone.position, Vector3Scale(drone.velocity, dt));

        // Keep in bounds
        if (IsSphericalMap()) {
            drone.position = SphericalSurfacePoint(drone.position, config_.droneHoverAltitude);
            drone.velocity = ProjectOnSphericalTangent(drone.velocity, SphericalUpAt(drone.position));
        } else if (IsSquareMap()) {
            float limit = squareHalfExtent_ - 1.5f;
            drone.position.x = std::clamp(drone.position.x, -limit, limit);
            drone.position.z = std::clamp(drone.position.z, -limit, limit);
            drone.position.y = std::clamp(drone.position.y, 1.2f, 16.0f);
        } else {
            Vector3 flat = Vector3{drone.position.x, 0.0f, drone.position.z};
            float limit = arenaRadius_ - 1.5f;
            if (Vector3Length(flat) > limit) {
                flat = Vector3Scale(Vector3Normalize(flat), limit);
                drone.position.x = flat.x;
                drone.position.z = flat.z;
            }
            drone.position.y = std::clamp(drone.position.y, 1.2f, 16.0f);
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
            projectiles_.push_back(Projectile{bulletBody, ProjectileKind::DroneBullet, 1.2f, 1.2f, config_.droneBulletDamage, 0.06f, 0.06f, Color{255, 240, 140, 255}, 0, intendedVel, ProjectileOwner::Player, false});

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
            projectiles_.push_back(Projectile{rocketBody, ProjectileKind::Rocket, 2.8f, 2.8f, config_.rocketImpactDamage, 0.34f, 0.34f, Color{230, 235, 210, 255}, 0, rocketVel, ProjectileOwner::Player, false});

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
                if (projectiles_[projectileIndex].kind == ProjectileKind::Rocket) {
                    ExplodeRocket(projectilePosition, projectiles_[projectileIndex].owner);
                    DestroyProjectile(projectileIndex);
                    projectileDestroyed = true;
                    break;
                } else if (projectiles_[projectileIndex].kind == ProjectileKind::GravityNail) {
                    enemy.health -= projectiles_[projectileIndex].damage;
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
                    SpawnHitBurst(enemyPosition, Color{210, 245, 255, 255}, 10);
                    cameraShake_ = std::min(1.0f, cameraShake_ + 0.18f);
                    if (enemy.health <= 0.0f) {
                        SpawnHitBurst(enemyPosition, Color{245, 255, 255, 255}, 20);
                        score_ += enemy.scoreValue;
                        DestroyEnemy(enemyIndex);
                    }
                    continue;
                }

                enemy.health -= projectiles_[projectileIndex].damage;
                if (projectiles_[projectileIndex].kind == ProjectileKind::Flame) {
                    SpawnHitBurst(projectilePosition, Color{255, 135, 28, 255}, 4);
                } else {
                    SpawnHitBurst(enemyPosition, enemy.color, 10);
                }
                DestroyProjectile(projectileIndex);
                projectileDestroyed = true;
                cameraShake_ = std::min(1.0f, cameraShake_ + 0.3f);

                if (enemy.health <= 0.0f) {
                    SpawnHitBurst(enemyPosition, Color{255, 255, 255, 255}, 20);
                    score_ += enemy.scoreValue;
                    DestroyEnemy(enemyIndex);
                }

                break;
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
}

void Game::UpdateArenaBounds() {
    for (Enemy& enemy : enemies_) {
        Vector3 position = BodyPosition(enemy.body);
        if (IsSphericalMap()) {
            float distance = Vector3Length(position);
            bool outside = IsHollowWorldMap()
                ? (distance > SphericalRadius() + 3.0f || distance < std::max(1.0f, SphericalRadius() - SphericalCleanupDistance()))
                : (distance > SphericalCleanupDistance() || distance < SphericalRadius() * 0.55f);
            if (outside) {
                Vector3 target = SphericalSurfacePoint(position, SphericalEnemyAltitude(enemy.type));
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

        float minX = platform.position.x - platform.scale.x * 0.5f - playerRadius_;
        float maxX = platform.position.x + platform.scale.x * 0.5f + playerRadius_;
        float minZ = platform.position.z - platform.scale.z * 0.5f - playerRadius_;
        float maxZ = platform.position.z + platform.scale.z * 0.5f + playerRadius_;
        float topY = platform.position.y + platform.scale.y;
        float bottomY = platform.position.y;
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
    } else if (type == EnemyType::Duelist) {
        position.y = 1.2f;
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
    } else if (type == EnemyType::Duelist) {
        enemyRadius = 0.82f;
        health = config_.duelistHealth;
        speed = 4.9f;
        scoreValue = 900;
        color = Color{255, 225, 135, 255};
    }

    PhysicsWorld::BodyConfig enemyConfig;
    enemyConfig.motionType = JPH::EMotionType::Dynamic;
    enemyConfig.layer = Layers::MOVING;
    enemyConfig.gravityFactor = !IsSphericalMap() && type == EnemyType::Pouncer ? 0.75f : 0.0f;
    enemyConfig.linearDamping = 0.0f;
    enemyConfig.angularDamping = 1.0f;
    enemyConfig.friction = 0.0f;
    enemyConfig.allowSleeping = false;

    JPH::BodyID body = physics_.CreateBody(
        enemyShape_,
        ToJoltVector(position),
        JPH::Quat::sIdentity(),
        enemyConfig);

    enemies_.push_back(Enemy{body, type, enemyRadius, speed, health, health, RandomFloat(0.0f, 6.28f), RandomFloat(0.0f, 1.0f), RandomFloat(0.4f, 1.6f), RandomFloat(0.0f, 0.5f), 0, RandomFloat(config_.duelistWeaponSwitchMin, config_.duelistWeaponSwitchMax), 0.0f, scoreValue, color, Vector3Zero(), Vector3Zero(), false});
    if (timeStopped_) {
        Enemy& enemy = enemies_.back();
        enemy.frozen = true;
        enemy.storedVelocity = Vector3Zero();
        physics_.Bodies().SetLinearVelocity(enemy.body, JPH::Vec3::sZero());
    }
}

void Game::FireProjectile(ProjectileKind kind, Vector3 direction, float speed, float damage, float life, float radius, float maxRadius, Color color) {
    Vector3 spawn = WeaponMuzzlePosition();
    Vector3 intendedVelocity = Vector3{direction.x * speed, direction.y * speed, direction.z * speed};

    PhysicsWorld::BodyConfig projectileConfig;
    projectileConfig.motionType = JPH::EMotionType::Dynamic;
    projectileConfig.layer = Layers::PROJECTILE;
    projectileConfig.linearVelocity = timeStopped_ ? JPH::Vec3::sZero() : JPH::Vec3(intendedVelocity.x, intendedVelocity.y, intendedVelocity.z);
    projectileConfig.gravityFactor = IsSphericalMap() ? 0.0f : kind == ProjectileKind::Rocket ? 0.02f : kind == ProjectileKind::BlackHoleGrenade ? 0.45f : 0.0f;
    projectileConfig.linearDamping = 0.0f;
    projectileConfig.motionQuality = JPH::EMotionQuality::LinearCast;
    projectileConfig.allowSleeping = false;

    JPH::BodyID body = physics_.CreateBody(
        projectileShape_,
        ToJoltVector(spawn),
        JPH::Quat::sIdentity(),
        projectileConfig);

    projectiles_.push_back(Projectile{body, kind, life, life, damage, radius, maxRadius, color, 0, intendedVelocity, ProjectileOwner::Player, timeStopped_});
}

void Game::FireEnemyProjectile(ProjectileKind kind, Vector3 position, Vector3 direction, float speed, float damage, float life, float radius, float maxRadius, Color color) {
    Vector3 intendedVelocity = Vector3Scale(direction, speed);

    PhysicsWorld::BodyConfig projectileConfig;
    projectileConfig.motionType = JPH::EMotionType::Dynamic;
    projectileConfig.layer = Layers::PROJECTILE;
    projectileConfig.linearVelocity = timeStopped_ ? JPH::Vec3::sZero() : JPH::Vec3(intendedVelocity.x, intendedVelocity.y, intendedVelocity.z);
    projectileConfig.gravityFactor = IsSphericalMap() ? 0.0f : kind == ProjectileKind::Rocket ? 0.02f : kind == ProjectileKind::BlackHoleGrenade ? 0.45f : 0.0f;
    projectileConfig.linearDamping = 0.0f;
    projectileConfig.motionQuality = JPH::EMotionQuality::LinearCast;
    projectileConfig.allowSleeping = false;

    JPH::BodyID body = physics_.CreateBody(projectileShape_, ToJoltVector(position), JPH::Quat::sIdentity(), projectileConfig);
    projectiles_.push_back(Projectile{body, kind, life, life, damage, radius, maxRadius, color, 0, intendedVelocity, ProjectileOwner::Enemy, timeStopped_});
}

void Game::FireLaser(float charge) {
    float normalizedCharge = std::clamp(charge, 0.18f, 1.0f);
    Vector3 forward = PlayerForward();
    Vector3 start = WeaponMuzzlePosition();
    Vector3 end = Vector3Add(start, Vector3Scale(forward, 62.0f));
    float damage = config_.laserBaseDamage + normalizedCharge * config_.laserChargeDamage;
    float beamRadius = config_.laserBeamRadius + normalizedCharge * 0.45f;

    float beamLife = 0.16f + normalizedCharge * 0.1f;
    beams_.push_back(Beam{
        start,
        end,
        beamLife,
        beamLife,
        beamRadius,
        normalizedCharge,
        Color{120, 220, 255, 255}
    });

    for (size_t i = 0; i < enemies_.size();) {
        Vector3 enemyPosition = BodyPosition(enemies_[i].body);
        float hitDistance = beamRadius + enemies_[i].radius * 0.85f;
        if (DistancePointToSegment(enemyPosition, start, end) <= hitDistance) {
            enemies_[i].health -= damage;
            SpawnHitBurst(enemyPosition, Color{150, 235, 255, 255}, 18 + static_cast<int>(normalizedCharge * 10.0f));
            if (enemies_[i].health <= 0.0f) {
                score_ += enemies_[i].scoreValue;
                SpawnHitBurst(enemyPosition, Color{255, 255, 255, 255}, 22);
                DestroyEnemy(i);
                continue;
            }
        }
        ++i;
    }

    cameraShake_ = std::min(1.0f, cameraShake_ + 0.55f + normalizedCharge * 0.35f);
}

void Game::FireEnemyShot(Vector3 position, Vector3 direction) {
    Vector3 intendedVelocity = Vector3Scale(direction, config_.enemyShotSpeed);
    PhysicsWorld::BodyConfig projectileConfig;
    projectileConfig.motionType = JPH::EMotionType::Dynamic;
    projectileConfig.layer = Layers::PROJECTILE;
    projectileConfig.linearVelocity = timeStopped_ ? JPH::Vec3::sZero() : JPH::Vec3(intendedVelocity.x, intendedVelocity.y, intendedVelocity.z);
    projectileConfig.gravityFactor = 0.0f;
    projectileConfig.linearDamping = 0.0f;
    projectileConfig.motionQuality = JPH::EMotionQuality::LinearCast;
    projectileConfig.allowSleeping = false;

    JPH::BodyID body = physics_.CreateBody(
        projectileShape_,
        ToJoltVector(position),
        JPH::Quat::sIdentity(),
        projectileConfig);

    projectiles_.push_back(Projectile{body, ProjectileKind::EnemyShot, 3.0f, 3.0f, config_.enemyShotDamage, 0.2f, 0.2f, Color{105, 255, 220, 255}, 0, intendedVelocity, ProjectileOwner::Enemy, timeStopped_});
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

void Game::SpawnEnemyNanoPlatform(Vector3 origin, Vector3 direction) {
    Vector3 forward = Vector3Normalize(direction);
    Vector3 target = Vector3Add(origin, Vector3Scale(forward, config_.riftPlatformRange * 0.72f));
    float halfSize = config_.riftPlatformSize * 0.36f;
    Vector3 scale = Vector3{config_.riftPlatformSize * 0.72f, config_.riftPlatformThickness, config_.riftPlatformSize * 0.72f};
    if (IsSphericalMap()) {
        Vector3 normal = SphericalUpAt(target);
        float targetAltitude = std::max(SphericalPlayerAltitude(), SphericalAltitudeAt(target));
        targetAltitude += config_.riftPlatformRange * 0.72f * 0.18f;
        Vector3 center = SphericalSurfacePoint(target, targetAltitude);
        Vector3 platformRight = SafeNormalize(Vector3CrossProduct(forward, normal), PlayerRight());
        Vector3 platformForward = SafeNormalize(Vector3CrossProduct(normal, platformRight), PlayerForward());
        nanoPlatforms_.push_back(NanoPlatform{center, scale, normal, platformRight, platformForward, config_.riftPlatformDelay, config_.riftPlatformLifetime * 0.45f, config_.riftPlatformLifetime * 0.45f});
        SpawnHitBurst(center, Color{255, 220, 115, 255}, 8);
        return;
    }

    if (IsSquareMap()) {
        float limit = squareHalfExtent_ - halfSize - 0.25f;
        target.x = std::clamp(target.x, -limit, limit);
        target.z = std::clamp(target.z, -limit, limit);
    } else {
        Vector3 flat = Vector3{target.x, 0.0f, target.z};
        float limit = std::max(0.1f, arenaRadius_ - halfSize - 0.25f);
        if (Vector3Length(flat) > limit) {
            flat = Vector3Scale(Vector3Normalize(flat), limit);
            target.x = flat.x;
            target.z = flat.z;
        }
    }
    float topY = std::clamp(target.y, 1.2f, 16.0f);
    Vector3 position = Vector3{target.x, topY - scale.y, target.z};
    nanoPlatforms_.push_back(NanoPlatform{position, scale, Vector3{0.0f, 1.0f, 0.0f}, Vector3{1.0f, 0.0f, 0.0f}, Vector3{0.0f, 0.0f, 1.0f}, config_.riftPlatformDelay, config_.riftPlatformLifetime * 0.45f, config_.riftPlatformLifetime * 0.45f});
    SpawnHitBurst(Vector3{position.x, position.y + scale.y, position.z}, Color{255, 220, 115, 255}, 8);
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

bool Game::DuelWon() const {
    return DuelMode() && duelWon_;
}

void Game::SwitchDuelistWeapon(Enemy& enemy, float distance) {
    int roll = GetRandomValue(0, 99);
    if (distance > 18.0f) {
        enemy.weaponSlot = roll < 24 ? 0 : roll < 44 ? 6 : roll < 65 ? 2 : roll < 84 ? 7 : 4;
    } else if (distance > 8.0f) {
        enemy.weaponSlot = roll < 18 ? 0 : roll < 34 ? 3 : roll < 51 ? 4 : roll < 67 ? 2 : roll < 83 ? 7 : 5;
    } else {
        enemy.weaponSlot = roll < 28 ? 3 : roll < 50 ? 1 : roll < 72 ? 5 : roll < 88 ? 4 : 6;
    }
    enemy.weaponSwitchTimer = RandomFloat(config_.duelistWeaponSwitchMin, config_.duelistWeaponSwitchMax);
    enemy.telegraphTimer = enemy.weaponSlot == 2 || enemy.weaponSlot == 4 || enemy.weaponSlot == 6 || enemy.weaponSlot == 7 ? 0.35f : 0.16f;
}

void Game::UpdateDuelist(Enemy& enemy, Vector3 position, Vector3 direction, float dt, float& speed, bool& skipVelocity) {
    Vector3 toPlayer = Vector3Subtract(camera_.position, position);
    float distance = Vector3Length(toPlayer);
    Vector3 localUp = IsSphericalMap() ? SphericalUpAt(position) : Vector3{0.0f, 1.0f, 0.0f};
    Vector3 tangent = IsSphericalMap() ? SafeNormalize(Vector3CrossProduct(localUp, direction), PlayerRight()) : Vector3{-direction.z, 0.0f, direction.x};
    float rangeBias = distance < 9.0f ? -0.75f : distance > 18.0f ? 0.55f : 0.1f;
    direction = Vector3Normalize(Vector3Add(Vector3Scale(direction, rangeBias), Vector3Scale(tangent, std::sin(enemy.bobTimer * 1.7f) * 0.8f)));
    speed = enemy.speed;

    enemy.weaponSwitchTimer -= dt;
    enemy.telegraphTimer = std::max(0.0f, enemy.telegraphTimer - dt);
    if (enemy.weaponSwitchTimer <= 0.0f) {
        SwitchDuelistWeapon(enemy, distance);
    }

    if (enemy.weaponSlot == 5 && distance < 8.0f && enemy.cooldownTimer <= 0.0f) {
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
        float radialCorrection = std::clamp((SphericalSignedRadius(SphericalEnemyAltitude(EnemyType::Duelist)) - Vector3Length(position)) * 7.0f, -12.0f, 8.0f);
        if (IsHollowWorldMap()) {
            radialCorrection = -radialCorrection;
        }
        velocity = Vector3Add(Vector3Add(Vector3Scale(direction, speed), Vector3Scale(localUp, radialCorrection)), enemy.externalVelocity);
    } else {
        velocity = Vector3{
            direction.x * speed + enemy.externalVelocity.x,
            std::clamp((1.2f - position.y) * 7.0f, -12.0f, 8.0f) + enemy.externalVelocity.y,
            direction.z * speed + enemy.externalVelocity.z
        };
    }
    physics_.Bodies().SetLinearVelocity(enemy.body, ToJoltVelocity(velocity));
    skipVelocity = true;
}

void Game::FireDuelistWeapon(Enemy& enemy, Vector3 position, Vector3 toPlayer) {
    Vector3 localUp = IsSphericalMap() ? SphericalUpAt(position) : Vector3{0.0f, 1.0f, 0.0f};
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
            FireEnemyProjectile(ProjectileKind::LaserShot, origin, aimDirection, 58.0f, config_.daggerDamage, 1.6f, 0.17f, 0.17f, Color{255, 235, 145, 255});
            enemy.cooldownTimer = 0.28f / rate;
        }
    } else if (enemy.weaponSlot == 1) {
        if (Vector3Distance(position, camera_.position) < config_.heatwaveRange * 0.72f && GetRandomValue(0, 99) < 34) {
            FireDuelistHeatwave(origin, aimDirection);
            enemy.cooldownTimer = 0.5f / rate;
        } else {
            Vector3 flameDirection = Vector3Normalize(Vector3Add(aimDirection, Vector3Add(Vector3Scale(right, RandomFloat(-0.12f, 0.12f)), Vector3Scale(up, RandomFloat(-0.04f, 0.08f)))));
            FireEnemyProjectile(ProjectileKind::Flame, origin, flameDirection, RandomFloat(17.0f, 22.0f), config_.flameDamage, 0.48f, 0.14f, 0.72f, Color{255, 120, 34, 235});
            enemy.cooldownTimer = 0.12f / rate;
        }
    } else if (enemy.weaponSlot == 2) {
        if (GetRandomValue(0, 99) < 22) {
            for (int i = 0; i < 3; ++i) {
                Vector3 direction = Vector3Normalize(Vector3Add(aimDirection, Vector3Add(Vector3Scale(right, RandomFloat(-0.18f, 0.18f)), Vector3Scale(up, RandomFloat(-0.04f, 0.1f)))));
                FireEnemyProjectile(ProjectileKind::Rocket, origin, direction, 22.0f, config_.rocketImpactDamage, 2.6f, 0.28f, 0.28f, Color{245, 190, 130, 255});
            }
            enemy.cooldownTimer = 1.65f / rate;
        } else {
            FireEnemyProjectile(ProjectileKind::Rocket, origin, aimDirection, 27.0f, config_.rocketImpactDamage, 2.8f, 0.34f, 0.34f, Color{245, 190, 130, 255});
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
            FireEnemyProjectile(glass ? ProjectileKind::GlassShard : ProjectileKind::Pellet, origin, direction, glass ? config_.glassShardSpeed * 0.78f : RandomFloat(42.0f, 50.0f), glass ? config_.glassShardDamage : config_.shotgunPelletDamage, glass ? config_.glassShardLingerTime : 0.58f, glass ? 0.13f : 0.1f, glass ? 0.13f : 0.1f, glass ? Color{190, 245, 255, 255} : Color{255, 205, 130, 255});
        }
        AddEnemyImpulse(enemy, Vector3Scale(aimDirection, -5.0f));
        enemy.cooldownTimer = (glass ? 1.05f : 0.82f) / rate;
    } else if (enemy.weaponSlot == 4) {
        bool blackHole = GetRandomValue(0, 99) < 45;
        FireEnemyProjectile(blackHole ? ProjectileKind::BlackHoleGrenade : ProjectileKind::GravityNail, origin, aimDirection, blackHole ? 22.0f : 58.0f, blackHole ? config_.blackHoleGrenadeDamage : config_.gravityNailDamage, blackHole ? 1.55f : 1.0f, blackHole ? 0.28f : 0.15f, blackHole ? 0.28f : 0.15f, blackHole ? Color{120, 70, 190, 255} : Color{170, 200, 255, 255});
        enemy.cooldownTimer = (blackHole ? 1.55f : 0.9f) / rate;
        enemy.telegraphTimer = 0.3f;
    } else if (enemy.weaponSlot == 6) {
        int burst = GetRandomValue(0, 99) < 26 ? 2 : 1;
        for (int i = 0; i < burst; ++i) {
            Vector3 direction = Vector3Normalize(Vector3Add(aimDirection, Vector3Add(Vector3Scale(right, RandomFloat(-0.06f, 0.06f)), Vector3Scale(up, RandomFloat(-0.035f, 0.045f)))));
            FireEnemyProjectile(ProjectileKind::Lance, origin, direction, config_.recoilLanceSpeed * 0.82f, config_.recoilLanceDamage, 1.15f, 0.28f, 0.28f, Color{220, 245, 255, 255});
        }
        AddEnemyImpulse(enemy, Vector3Scale(aimDirection, -config_.recoilLanceImpulse * (burst > 1 ? 0.45f : 0.32f)));
        enemy.cooldownTimer = (burst > 1 ? 1.35f : 1.05f) / rate;
    } else if (enemy.weaponSlot == 7) {
        if (GetRandomValue(0, 99) < 28) {
            SpawnEnemyNanoPlatform(origin, aimDirection);
            enemy.cooldownTimer = 1.4f / rate;
            enemy.telegraphTimer = 0.28f;
        } else {
            SpawnEnemyRift(origin, aimDirection);
            enemy.cooldownTimer = 1.25f / rate;
            enemy.telegraphTimer = 0.35f;
        }
    } else if (enemy.weaponSlot == 5) {
        BlinkDuelist(enemy, Vector3Normalize(Vector3Subtract(position, camera_.position)));
        enemy.cooldownTimer = 2.35f / rate;
        enemy.telegraphTimer = 0.35f;
    }
}

void Game::FireBossRing(Vector3 position, int count, float speedScale) {
    float spin = static_cast<float>(GetTime()) * 0.65f;
    Vector3 up = IsSphericalMap() ? SphericalUpAt(position) : Vector3{0.0f, 1.0f, 0.0f};
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
        projectiles_.push_back(Projectile{body, ProjectileKind::EnemyShot, 4.2f, 4.2f, config_.enemyShotDamage, 0.24f, 0.24f, Color{180, 125, 255, 255}, 0, intendedVelocity, ProjectileOwner::Enemy, timeStopped_});
    }
    SpawnShockwave(position, 4.8f, Color{170, 115, 255, 255});
}

void Game::FireHeatwave(Vector3 direction) {
    Vector3 origin = WeaponMuzzlePosition();
    Vector3 forward = Vector3Normalize(direction);
    float range = config_.heatwaveRange;

    for (size_t i = 0; i < enemies_.size();) {
        Enemy& enemy = enemies_[i];
        Vector3 enemyPosition = BodyPosition(enemy.body);
        Vector3 offset = Vector3Subtract(enemyPosition, origin);
        float distance = Vector3Length(offset);
        if (distance <= 0.05f || distance > range) {
            ++i;
            continue;
        }
        Vector3 toEnemy = Vector3Scale(offset, 1.0f / distance);
        float facing = Vector3DotProduct(forward, toEnemy);
        if (facing < 0.55f) {
            ++i;
            continue;
        }

        float falloff = 1.0f - distance / range;
        enemy.health -= config_.heatwaveDamage * (0.35f + falloff * 0.65f);
        float impulse = config_.heatwaveForce * (0.45f + falloff * 0.9f);
        AddEnemyImpulse(enemy, Vector3{toEnemy.x * impulse, 0.35f + 1.15f * falloff, toEnemy.z * impulse});
        SpawnHitBurst(enemyPosition, Color{255, 155, 75, 255}, 5);
        if (enemy.health <= 0.0f) {
            score_ += enemy.scoreValue;
            SpawnHitBurst(enemyPosition, Color{255, 235, 190, 255}, 18);
            DestroyEnemy(i);
            continue;
        }
        ++i;
    }

    for (Projectile& projectile : projectiles_) {
        if (projectile.kind != ProjectileKind::EnemyShot) {
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
        float impulse = config_.heatwaveForce * (0.8f + (1.0f - distance / range) * 0.7f);
        AddProjectileImpulse(projectile, Vector3Scale(toProjectile, impulse));
    }

    heatwaves_.push_back(HeatwavePulse{
        origin,
        forward,
        0.18f,
        0.18f,
        range,
        0.95f,
        Color{255, 130, 65, 255}
    });
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

void Game::FireRiftCutter(Vector3 direction) {
    Vector3 forward = Vector3Normalize(direction);
    Vector3 planeNormal = PlayerRight();
    if (Vector3Length(planeNormal) <= 0.001f) {
        planeNormal = Vector3{1.0f, 0.0f, 0.0f};
    }
    Vector3 up = PlayerUp();
    Vector3 center = Vector3Add(WeaponMuzzlePosition(), Vector3Scale(forward, config_.riftCutterWaveSpawnDistance));
    rifts_.push_back(RiftSlash{
        center,
        planeNormal,
        forward,
        up,
        Vector3Scale(forward, config_.riftCutterWaveSpeed),
        config_.riftCutterDelay,
        config_.riftCutterLifetime,
        config_.riftCutterLifetime,
        config_.riftCutterRadius,
        config_.riftCutterThickness,
        config_.riftCutterPlaneThickness,
        config_.riftCutterDamage / config_.riftCutterLifetime,
        ProjectileOwner::Player
    });
    SpawnHitBurst(WeaponMuzzlePosition(), Color{255, 225, 140, 255}, 8);
    eventText_ = "NANO EDGE";
    eventTextTimer_ = 0.85f;
}

void Game::FireNanoPlatform(Vector3 direction) {
    NanoPlatform platform = MakeNanoPlatformTarget(direction);
    nanoPlatforms_.push_back(platform);
    Vector3 topCenter = IsSphericalMap()
        ? platform.position
        : Vector3{platform.position.x, platform.position.y + platform.scale.y, platform.position.z};
    SpawnHitBurst(topCenter, Color{255, 238, 160, 255}, 8);
    eventText_ = "NANO PLATFORM";
    eventTextTimer_ = 0.95f;
}

void Game::FireLanceThrust(Vector3 direction) {
    Vector3 forward = Vector3Normalize(direction);
    Vector3 origin = WeaponMuzzlePosition();
    float range = config_.recoilLanceThrustRange;
    float halfAngleCos = 0.54f;

    for (size_t i = 0; i < enemies_.size();) {
        Enemy& enemy = enemies_[i];
        Vector3 enemyPosition = BodyPosition(enemy.body);
        Vector3 offset = Vector3Subtract(enemyPosition, origin);
        float distance = Vector3Length(offset);
        if (distance <= 0.05f || distance > range + enemy.radius) {
            ++i;
            continue;
        }
        Vector3 toEnemy = Vector3Scale(offset, 1.0f / distance);
        float facing = Vector3DotProduct(forward, toEnemy);
        if (facing < halfAngleCos) {
            ++i;
            continue;
        }

        float falloff = 1.0f - std::clamp(distance / std::max(0.001f, range), 0.0f, 1.0f);
        enemy.health -= config_.recoilLanceThrustDamage * (0.35f + falloff * 0.65f);
        float impulse = config_.recoilLanceThrustForce * (0.45f + falloff * 0.9f);
        AddEnemyImpulse(enemy, Vector3Scale(toEnemy, impulse));
        SpawnHitBurst(enemyPosition, Color{210, 245, 255, 255}, 8);
        if (enemy.health <= 0.0f) {
            score_ += enemy.scoreValue;
            SpawnHitBurst(enemyPosition, Color{240, 255, 255, 255}, 18);
            DestroyEnemy(i);
            continue;
        }
        ++i;
    }

    Vector3 up = PlayerUp();
    Vector3 end = Vector3Add(origin, Vector3Scale(forward, range));
    beams_.push_back(Beam{
        origin,
        end,
        0.16f,
        0.16f,
        range * 0.72f,
        0.9f,
        Color{190, 245, 255, 255}
    });
    shockwaves_.push_back(Shockwave{Vector3Add(origin, Vector3Scale(forward, range * 0.55f)), 0.22f, 0.22f, range * 0.55f, Color{190, 245, 255, 255}});
    heatwaves_.push_back(HeatwavePulse{
        origin,
        forward,
        0.18f,
        0.18f,
        range,
        0.52f,
        Color{170, 235, 255, 255}
    });
    SpawnHitBurst(Vector3Add(origin, Vector3Scale(forward, 1.25f)), Color{225, 255, 255, 255}, 14);
    float impulse = config_.recoilLanceThrustImpulse;
    Vector3 thrust = Vector3Scale(forward, impulse);
    thrust = Vector3Add(thrust, Vector3Scale(up, std::max(0.0f, -Vector3DotProduct(forward, up)) * impulse * 0.28f));
    playerVelocity_ = Vector3Add(playerVelocity_, thrust);
    camera_.position = Vector3Add(camera_.position, Vector3Scale(forward, std::clamp(impulse * 0.055f, 0.35f, 1.25f)));
    if (IsSphericalMap()) {
        float surfaceRadius = SphericalSignedRadius(SphericalPlayerAltitude());
        bool pushedIntoSurface = IsHollowWorldMap()
            ? Vector3Length(camera_.position) > surfaceRadius
            : Vector3Length(camera_.position) < surfaceRadius;
        if (pushedIntoSurface) {
            camera_.position = SphericalSurfacePoint(camera_.position, SphericalPlayerAltitude());
        }
        camera_.up = SphericalUpAt(camera_.position);
    } else {
        camera_.position.y = std::max(playerHeight_, camera_.position.y);
    }
    camera_.target = Vector3Add(camera_.position, PlayerForward());
    thrustControlLockTimer_ = 0.18f;
    grounded_ = false;
    eventText_ = "SONIC THRUST";
    eventTextTimer_ = 0.85f;
}

void Game::DetonateLance(Vector3 position, ProjectileOwner owner) {
    float radius = config_.recoilLanceShockwaveRadius;
    if (owner == ProjectileOwner::Player) {
        for (size_t i = 0; i < enemies_.size();) {
            Enemy& enemy = enemies_[i];
            Vector3 enemyPosition = BodyPosition(enemy.body);
            float distance = Vector3Distance(enemyPosition, position);
            if (distance <= radius + enemy.radius) {
                float falloff = 1.0f - std::clamp(distance / std::max(0.001f, radius), 0.0f, 1.0f);
                enemy.health -= config_.recoilLanceShockwaveDamage * (0.25f + falloff * 0.75f);
                Vector3 direction = Vector3Subtract(enemyPosition, position);
                if (Vector3Length(direction) <= 0.001f) {
                    direction = IsSphericalMap() ? SphericalUpAt(position) : Vector3{0.0f, 1.0f, 0.0f};
                } else {
                    direction = Vector3Normalize(direction);
                }
                AddEnemyImpulse(enemy, Vector3Scale(direction, config_.recoilLanceShockwaveForce * (0.35f + falloff * 0.85f)));
                SpawnHitBurst(enemyPosition, Color{190, 245, 255, 255}, 7);
                if (enemy.health <= 0.0f) {
                    score_ += enemy.scoreValue;
                    SpawnHitBurst(enemyPosition, Color{240, 255, 255, 255}, 18);
                    DestroyEnemy(i);
                    continue;
                }
            }
            ++i;
        }
    } else if (Vector3Distance(camera_.position, position) <= radius + playerRadius_) {
        ApplyPlayerHit(camera_.position, Color{190, 245, 255, 255});
    }

    SpawnShockwave(position, radius, Color{190, 245, 255, 255});
    SpawnHitBurst(position, Color{220, 255, 255, 255}, 28);
    beams_.push_back(Beam{
        position,
        Vector3Add(position, Vector3Scale(IsSphericalMap() ? SphericalUpAt(position) : Vector3{0.0f, 1.0f, 0.0f}, 0.01f)),
        0.16f,
        0.16f,
        radius * 1.4f,
        1.0f,
        Color{190, 245, 255, 255}
    });
}

void Game::SpawnEnemyRift(Vector3 origin, Vector3 direction) {
    Vector3 forward = Vector3Normalize(direction);
    Vector3 planeNormal = Vector3Normalize(Vector3CrossProduct(forward, Vector3{0.0f, 1.0f, 0.0f}));
    if (Vector3Length(planeNormal) <= 0.001f) {
        planeNormal = Vector3{1.0f, 0.0f, 0.0f};
    }
    Vector3 up = Vector3Normalize(Vector3CrossProduct(planeNormal, forward));
    Vector3 center = Vector3Add(origin, Vector3Scale(forward, config_.riftCutterWaveSpawnDistance));
    rifts_.push_back(RiftSlash{center, planeNormal, forward, up, Vector3Scale(forward, config_.riftCutterWaveSpeed * 0.82f), config_.riftCutterDelay, config_.riftCutterLifetime, config_.riftCutterLifetime, config_.riftCutterRadius, config_.riftCutterThickness, config_.riftCutterPlaneThickness, config_.riftCutterDamage / config_.riftCutterLifetime, ProjectileOwner::Enemy});
    SpawnHitBurst(origin, Color{255, 210, 120, 255}, 6);
}

void Game::SpawnGravityWell(Vector3 position, bool blackHole) {
    float lifetime = blackHole ? config_.blackHoleLifetime : config_.gravityWellLifetime;
    float radius = blackHole ? config_.blackHoleRadius : config_.gravityWellRadius;
    float force = blackHole ? config_.blackHoleForce : config_.gravityWellForce;
    float damage = blackHole ? config_.blackHoleGrenadeDamage : 0.0f;
    gravityWells_.push_back(GravityWell{position, lifetime, lifetime, radius, force, damage, blackHole});
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
        float surfaceRadius = SphericalSignedRadius(SphericalPlayerAltitude());
        bool beyondSurface = IsHollowWorldMap()
            ? Vector3Length(target) > surfaceRadius
            : Vector3Length(target) < surfaceRadius;
        if (beyondSurface) {
            target = SphericalSurfacePoint(target, SphericalPlayerAltitude());
        }
        if (!IsHollowWorldMap() && Vector3Length(target) > SphericalCleanupDistance() * 0.72f) {
            target = SphericalSurfacePoint(target, SphericalCleanupDistance() * 0.72f - SphericalRadius());
        }
    } else {
        target.y = std::max(playerHeight_, target.y);

        Vector3 flat = Vector3{target.x, 0.0f, target.z};
        if (Vector3Length(flat) > maxDistance) {
            flat = Vector3Scale(Vector3Normalize(flat), maxDistance);
            target.x = flat.x;
            target.z = flat.z;
        }
    }

    camera_.position = target;
    camera_.up = IsSphericalMap() ? SphericalUpAt(camera_.position) : Vector3{0.0f, 1.0f, 0.0f};
    camera_.target = Vector3Add(camera_.position, PlayerForward());
    playerVelocity_ = Vector3Scale(playerVelocity_, 0.35f);

    for (size_t i = 0; i < enemies_.size();) {
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
        direction = SafeNormalize(ProjectOnSphericalTangent(direction, SphericalUpAt(position)), PlayerRight());
    }
    Vector3 target = Vector3Add(position, Vector3Scale(direction, 8.5f));
    if (IsSphericalMap()) {
        target = SphericalSurfacePoint(target, SphericalEnemyAltitude(EnemyType::Duelist));
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
        target.y = 1.2f;
    }
    physics_.Bodies().SetPosition(enemy.body, ToJoltVector(target), JPH::EActivation::Activate);
    physics_.Bodies().SetLinearVelocity(enemy.body, JPH::Vec3::sZero());
    enemy.externalVelocity = Vector3Zero();
    SpawnShockwave(target, 3.2f, Color{255, 215, 130, 255});
    SpawnHitBurst(target, Color{255, 230, 150, 255}, 20);
}

void Game::SpawnShockwave(Vector3 position, float radius, Color color) {
    shockwaves_.push_back(Shockwave{position, 0.34f, 0.34f, radius, color});
}

void Game::FireDroneCanister() {
    if (static_cast<int>(drones_.size()) >= config_.droneMaxCount) {
        eventText_ = "DRONE MAX";
        eventTextTimer_ = 1.2f;
        return;
    }

    // Fires directly forward like the black hole grenade — gravity does the arc.
    Vector3 forward = PlayerForward();
    Vector3 spawn = WeaponMuzzlePosition();
    float speed = config_.droneCanisterSpeed;
    Vector3 intendedVelocity = Vector3{forward.x * speed, forward.y * speed, forward.z * speed};

    PhysicsWorld::BodyConfig projectileConfig;
    projectileConfig.motionType = JPH::EMotionType::Dynamic;
    projectileConfig.layer = Layers::PROJECTILE;
    projectileConfig.linearVelocity = JPH::Vec3(intendedVelocity.x, intendedVelocity.y, intendedVelocity.z);
    projectileConfig.gravityFactor = IsSphericalMap() ? 0.0f : config_.droneCanisterGravity;
    projectileConfig.linearDamping = 0.0f;
    projectileConfig.motionQuality = JPH::EMotionQuality::LinearCast;
    projectileConfig.allowSleeping = false;

    JPH::BodyID body = physics_.CreateBody(
        projectileShape_,
        ToJoltVector(spawn),
        JPH::Quat::sIdentity(),
        projectileConfig);

    projectiles_.push_back(Projectile{body, ProjectileKind::DroneCanister, 4.0f, 4.0f, 0.0f, 0.35f, 0.35f, Color{140, 155, 170, 255}, 0, intendedVelocity, ProjectileOwner::Player, false});
    SpawnHitBurst(spawn, Color{180, 190, 200, 255}, 6);
}

void Game::ExplodeRocket(Vector3 position, ProjectileOwner owner) {
    float radius = config_.rocketExplosionRadius;
    if (owner == ProjectileOwner::Player) {
        for (size_t i = 0; i < enemies_.size();) {
            Vector3 enemyPosition = BodyPosition(enemies_[i].body);
            float distance = Vector3Distance(enemyPosition, position);
            if (distance <= radius + enemies_[i].radius) {
                float falloff = 1.0f - std::clamp(distance / std::max(0.001f, radius), 0.0f, 1.0f);
                enemies_[i].health -= config_.rocketExplosionDamage * (0.35f + falloff * 0.65f);
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
            Vector3 enemyPosition = BodyPosition(enemies_[i].body);
            float distance = Vector3Distance(enemyPosition, position);
            if (distance <= radius + enemies_[i].radius) {
                float falloff = 1.0f - std::clamp(distance / std::max(0.001f, radius), 0.0f, 1.0f);
                enemies_[i].health -= config_.rocketExplosionDamage * 0.5f * (0.35f + falloff * 0.65f);
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

void Game::ApplyExplosionImpulse(Vector3 position, float radius, float impulse) {
    Vector3 player = camera_.position;
    float distance = Vector3Distance(player, position);
    if (distance > radius) {
        return;
    }

    Vector3 direction = Vector3Subtract(player, position);
    if (Vector3Length(direction) <= 0.001f) {
        direction = IsSphericalMap() ? SphericalUpAt(player) : Vector3{0.0f, 1.0f, 0.0f};
    }
    Vector3 up = IsSphericalMap() ? SphericalUpAt(player) : Vector3{0.0f, 1.0f, 0.0f};
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
    Vector3 up = IsSphericalMap() ? SphericalUpAt(camera_.position) : Vector3{0.0f, 1.0f, 0.0f};
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

void Game::ApplyLanceRecoil(Vector3 direction) {
    Vector3 up = IsSphericalMap() ? SphericalUpAt(camera_.position) : Vector3{0.0f, 1.0f, 0.0f};
    Vector3 recoil = Vector3Scale(direction, -config_.recoilLanceImpulse);
    recoil = Vector3Add(recoil, Vector3Scale(up, std::max(0.0f, -Vector3DotProduct(direction, up)) * config_.recoilLanceImpulse * 0.42f));
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

    if (DuelMode() && duelArmorInvulnTimer_ > 0.0f) {
        return;
    }

    if (DuelMode() && duelArmor_ > 0) {
        duelArmor_ -= 1;
        duelArmorInvulnTimer_ = config_.duelArmorHitInvuln;
        cameraShake_ = std::min(1.0f, cameraShake_ + 0.65f);
        SpawnHitBurst(position, color, 30);
        SpawnShockwave(camera_.position, 3.4f, Color{255, 215, 120, 255});
        eventText_ = duelArmor_ > 0 ? "ARMOR HIT" : "ARMOR BROKEN";
        eventTextTimer_ = 1.6f;
        return;
    }

    state_ = State::Dead;
    cameraShake_ = 1.0f;
    SpawnHitBurst(position, color, 28);
    if (text != nullptr) {
        eventText_ = text;
        eventTextTimer_ = 2.0f;
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

void Game::DestroyEnemy(size_t index) {
    bool wasBoss = enemies_[index].type == EnemyType::Boss;
    bool wasDuelist = enemies_[index].type == EnemyType::Duelist;
    Vector3 position = BodyPosition(enemies_[index].body);
    physics_.DestroyBody(enemies_[index].body);
    enemies_[index] = enemies_.back();
    enemies_.pop_back();
    if (wasBoss) {
        SpawnShockwave(position, 12.0f, Color{210, 160, 255, 255});
        SpawnHitBurst(position, Color{230, 210, 255, 255}, 90);
        eventText_ = "BOSS SHATTERED";
        eventTextTimer_ = 4.0f;
        cameraShake_ = 1.0f;
    } else if (wasDuelist) {
        duelWon_ = true;
        SpawnShockwave(position, 9.0f, Color{255, 225, 130, 255});
        SpawnHitBurst(position, Color{255, 235, 170, 255}, 76);
        eventText_ = "DUEL WON";
        eventTextTimer_ = 5.0f;
        cameraShake_ = 1.0f;
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

void Game::DrawBethlehem() const {
    if (!bethlehem_.active) return;

    if (bethlehemModelLoaded_) {
        DrawModel(bethlehemModel_, bethlehem_.position, 1.0f, WHITE);
    } else {
        DrawSphereEx(bethlehem_.position, 2.5f, 12, 10, Color{255, 210, 100, 255});
        DrawSphereWires(bethlehem_.position, 2.8f, 14, 12, Color{255, 180, 50, 220});
    }

    if (bethlehem_.laserPhase == BethlehemLaserPhase::Inactive) return;

    Vector3 beamStart = bethlehem_.position;
    Vector3 beamEnd = Vector3Add(beamStart, Vector3Scale(bethlehem_.laserDirection, config_.bethlehemLaserRange));
    float r = config_.bethlehemLaserRadius;

    if (bethlehem_.laserPhase == BethlehemLaserPhase::Warning) {
        DrawCylinderEx(beamStart, beamEnd, r, r, 8, FadeColor(Color{255, 200, 80, 255}, 0.22f));
        DrawCylinderWiresEx(beamStart, beamEnd, r * 1.05f, r * 1.05f, 8, FadeColor(Color{255, 230, 160, 255}, 0.32f));
    } else {
        DrawCylinderEx(beamStart, beamEnd, r, r, 8, FadeColor(Color{255, 130, 30, 255}, 0.68f));
        DrawCylinderWiresEx(beamStart, beamEnd, r * 1.06f, r * 1.06f, 8, FadeColor(Color{255, 200, 90, 255}, 0.80f));
        DrawCylinderEx(beamStart, beamEnd, r * 0.38f, r * 0.38f, 6, FadeColor(Color{255, 255, 200, 255}, 0.92f));
    }
}

Vector3 Game::PlayerForward() const {
    float yaw = yaw_ * kDegToRad;
    float pitch = pitch_ * kDegToRad;
    if (IsSphericalMap()) {
        Vector3 up = SphericalUpAt(camera_.position);
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
    return Vector3Normalize(Vector3{
        std::cos(pitch) * std::cos(yaw),
        std::sin(pitch),
        std::cos(pitch) * std::sin(yaw)
    });
}

Vector3 Game::PlayerRight() const {
    Vector3 up = IsSphericalMap() ? SphericalUpAt(camera_.position) : Vector3{0.0f, 1.0f, 0.0f};
    Vector3 right = Vector3CrossProduct(PlayerForward(), up);
    if (Vector3Length(right) <= 0.001f) {
        right = Vector3{1.0f, 0.0f, 0.0f};
    }
    return Vector3Normalize(right);
}

Vector3 Game::PlayerUp() const {
    if (IsSphericalMap()) {
        return SphericalUpAt(camera_.position);
    }
    return Vector3Normalize(Vector3CrossProduct(PlayerRight(), PlayerForward()));
}

Vector3 Game::WeaponMuzzlePosition() const {
    return weaponViewModel_.MuzzlePosition(camera_);
}

Game::NanoPlatform Game::MakeNanoPlatformTarget(Vector3 direction) const {
    Vector3 forward = Vector3Length(direction) > 0.001f ? Vector3Normalize(direction) : PlayerForward();
    Vector3 target = Vector3Add(WeaponMuzzlePosition(), Vector3Scale(forward, config_.riftPlatformRange * riftPlatformRangeScale_));
    float halfSize = config_.riftPlatformSize * 0.5f;

    if (IsSphericalMap()) {
        Vector3 normal = SphericalUpAt(target);
        float platformRange = config_.riftPlatformRange * riftPlatformRangeScale_;
        float targetAltitude = std::max(SphericalPlayerAltitude(), SphericalAltitudeAt(target));
        targetAltitude += platformRange * 0.18f;
        Vector3 center = SphericalSurfacePoint(target, targetAltitude);
        Vector3 platformRight = Vector3CrossProduct(forward, normal);
        if (Vector3Length(platformRight) <= 0.001f) {
            platformRight = PlayerRight();
        } else {
            platformRight = Vector3Normalize(platformRight);
        }
        Vector3 platformForward = Vector3Normalize(Vector3CrossProduct(normal, platformRight));
        Vector3 scale = Vector3{config_.riftPlatformSize, config_.riftPlatformThickness, config_.riftPlatformSize};
        return NanoPlatform{center, scale, normal, platformRight, platformForward, config_.riftPlatformDelay, config_.riftPlatformLifetime, config_.riftPlatformLifetime};
    }

    if (IsSquareMap()) {
        float limit = squareHalfExtent_ - halfSize - 0.25f;
        target.x = std::clamp(target.x, -limit, limit);
        target.z = std::clamp(target.z, -limit, limit);
    } else {
        Vector3 flat = Vector3{target.x, 0.0f, target.z};
        float maxDistance = std::max(0.1f, arenaRadius_ - halfSize - 0.25f);
        if (Vector3Length(flat) > maxDistance) {
            flat = Vector3Scale(Vector3Normalize(flat), maxDistance);
            target.x = flat.x;
            target.z = flat.z;
        }
    }

    float topY = std::clamp(target.y, 1.2f, 34.0f);
    Vector3 scale = Vector3{config_.riftPlatformSize, config_.riftPlatformThickness, config_.riftPlatformSize};
    Vector3 position = Vector3{target.x, topY - scale.y, target.z};
    return NanoPlatform{position, scale, Vector3{0.0f, 1.0f, 0.0f}, Vector3{1.0f, 0.0f, 0.0f}, Vector3{0.0f, 0.0f, 1.0f}, config_.riftPlatformDelay, config_.riftPlatformLifetime, config_.riftPlatformLifetime};
}

Vector3 Game::GetFireControlAimPoint() const {
    Vector3 origin = camera_.position;
    Vector3 forward = Vector3Length(PlayerForward()) > 0.001f ? Vector3Normalize(PlayerForward()) : Vector3{0.0f, 0.0f, -1.0f};
    float altitude = config_.droneRallyMarkerAltitude;

    if (IsSphericalMap()) {
        float radius = SphericalRadius();
        float targetR = IsHollowWorldMap() ? (radius - altitude) : (radius + altitude);

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
                return SphericalSurfacePoint(hitPoint, altitude);
            }
        }
        return SphericalSurfacePoint(Vector3Add(origin, Vector3Scale(forward, 20.0f)), altitude);
    }

    // Flat maps: ray-plane intersection at y = altitude
    if (std::abs(forward.y) > 0.0001f) {
        float t = (altitude - origin.y) / forward.y;
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
            point.y = altitude;
            return point;
        }
    }
    Vector3 fallback = Vector3Add(origin, Vector3Scale(forward, 20.0f));
    fallback.y = altitude;
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

float Game::SphericalRadius() const {
    return IsHollowWorldMap() ? config_.hollowWorldRadius : config_.asteroidRadius;
}

float Game::SphericalPlayerAltitude() const {
    return IsHollowWorldMap() ? config_.hollowWorldPlayerAltitude : config_.asteroidPlayerAltitude;
}

float Game::SphericalCleanupDistance() const {
    return IsHollowWorldMap() ? config_.hollowWorldCleanupDistance : config_.asteroidCleanupDistance;
}

float Game::SphericalAltitudeAt(Vector3 position) const {
    float distance = Vector3Length(position);
    return IsHollowWorldMap() ? SphericalRadius() - distance : distance - SphericalRadius();
}

float Game::SphericalSignedRadius(float altitude) const {
    return IsHollowWorldMap() ? SphericalRadius() - altitude : SphericalRadius() + altitude;
}

Vector3 Game::SphericalUpAt(Vector3 position) const {
    if (Vector3Length(position) <= 0.001f) {
        return Vector3{0.0f, 1.0f, 0.0f};
    }
    Vector3 outward = Vector3Normalize(position);
    return IsHollowWorldMap() ? Vector3Scale(outward, -1.0f) : outward;
}

Vector3 Game::SphericalSurfacePoint(Vector3 position, float altitude) const {
    Vector3 outward = Vector3Length(position) > 0.001f
        ? Vector3Normalize(position)
        : Vector3{0.0f, IsHollowWorldMap() ? -1.0f : 1.0f, 0.0f};
    return Vector3Scale(outward, SphericalSignedRadius(altitude));
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
    if (type == EnemyType::Boss) {
        return 2.2f;
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

const char* Game::WeaponName() const {
    switch (activeWeapon_) {
        case WeaponType::Laser:
            return "LASER";
        case WeaponType::Flamethrower:
            return "FLAME";
        case WeaponType::RocketLauncher:
            return "ROCKET";
        case WeaponType::Shotgun:
            return "SHOTGUN";
        case WeaponType::GravityNailer:
            return "NAIL";
        case WeaponType::InfinityGauntlet:
            return "GAUNT";
        case WeaponType::RecoilLance:
            return "LANCE";
        case WeaponType::RiftCutter:
            return "NANO";
        default:
            return "UNKNOWN";
    }
}

const char* Game::WeaponModeName() const {
    if (activeWeapon_ == WeaponType::Flamethrower) {
        return flamethrowerMode_ == FlamethrowerMode::Heatwave ? "H" : "F";
    }
    if (activeWeapon_ == WeaponType::RocketLauncher) {
        return rocketLauncherMode_ == RocketLauncherMode::Drone ? "D" : "";
    }
    if (activeWeapon_ == WeaponType::Shotgun) {
        return shotgunMode_ == ShotgunMode::GlassShard ? "G" : "P";
    }
    if (activeWeapon_ == WeaponType::GravityNailer) {
        return gravityNailerMode_ == GravityNailerMode::BlackHole ? "BH" : "N";
    }
    if (activeWeapon_ == WeaponType::InfinityGauntlet) {
        if (timeStopped_) return "T";
        return gauntletMode_ == GauntletMode::Blink ? "B" : "TS";
    }
    if (activeWeapon_ == WeaponType::RiftCutter) {
        return riftCutterMode_ == RiftCutterMode::Platform ? "P" : "B";
    }
    if (activeWeapon_ == WeaponType::RecoilLance) {
        return recoilLanceMode_ == RecoilLanceMode::Thrust ? "T" : "L";
    }
    return "";
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

float Game::CurrentGravity() const {
    return config_.gravity * gravityScale_;
}

bool Game::IsSquareMap() const {
    return config_.mapType == "square_obstacle" || config_.mapType == "square";
}

bool Game::EnemyTouchesPlayer(Vector3 enemyPosition, float enemyRadius) const {
    Vector3 up = IsSphericalMap() ? SphericalUpAt(camera_.position) : Vector3{0.0f, 1.0f, 0.0f};
    Vector3 capsuleBottom = IsSphericalMap()
        ? Vector3Subtract(camera_.position, Vector3Scale(up, SphericalPlayerAltitude() - playerRadius_))
        : Vector3{camera_.position.x, camera_.position.y - playerHeight_ + playerRadius_, camera_.position.z};
    Vector3 capsuleTop = IsSphericalMap()
        ? Vector3Subtract(camera_.position, Vector3Scale(up, playerRadius_ * 0.35f))
        : Vector3{camera_.position.x, camera_.position.y - playerRadius_ * 0.35f, camera_.position.z};
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

void Game::Draw() const {
    BeginTextureMode(pixelTarget_);
    ClearBackground(Color{8, 8, 10, 255});

    BeginMode3D(camera_);
    DrawArena();
    DrawProps();
    DrawNanoPlatforms();
    DrawDrones();
    DrawEnemies();
    DrawBethlehem();
    DrawPickups();
    DrawProjectiles();
    DrawBeams();
    DrawShockwaves();
    DrawHeatwaves();
    DrawGravityWells();
    DrawRifts();
    DrawParticles();
    DrawRallyMarker();
    DrawBlinkIndicator();
    if (!fireControlActive_ && !hideUI_) DrawWeapon();
    EndMode3D();

    // Second 3D pass: X-ray octahedron markers visible through obstacles
    if (fireControlActive_) {
        BeginMode3D(camera_);
        rlDisableDepthTest();
        Color markerColor = Color{80, 235, 150, 190};
        for (const Enemy& enemy : enemies_) {
            Vector3 pos = BodyPosition(enemy.body);
            float s = enemy.radius * 1.6f;
            Vector3 vx = {s, 0, 0}, nvx = {-s, 0, 0};
            Vector3 vy = {0, s, 0}, nvy = {0, -s, 0};
            Vector3 vz = {0, 0, s}, nvz = {0, 0, -s};
            // Octahedron: 12 edges, each vertex connects to all except its opposite
            auto p = [&](Vector3 v) { return Vector3Add(pos, v); };
            DrawLine3D(p(vx), p(vy), markerColor);
            DrawLine3D(p(vx), p(nvy), markerColor);
            DrawLine3D(p(vx), p(vz), markerColor);
            DrawLine3D(p(vx), p(nvz), markerColor);
            DrawLine3D(p(nvx), p(vy), markerColor);
            DrawLine3D(p(nvx), p(nvy), markerColor);
            DrawLine3D(p(nvx), p(vz), markerColor);
            DrawLine3D(p(nvx), p(nvz), markerColor);
            DrawLine3D(p(vy), p(vz), markerColor);
            DrawLine3D(p(vy), p(nvz), markerColor);
            DrawLine3D(p(nvy), p(vz), markerColor);
            DrawLine3D(p(nvy), p(nvz), markerColor);
        }
        rlEnableDepthTest();
        EndMode3D();

        DrawFireControlOverlay();
    } else {
        if (!hideUI_) DrawCrosshair();
        if (!hideUI_) DrawHud();
    }

    EndTextureMode();

    float screenWidth = static_cast<float>(GetScreenWidth());
    float screenHeight = static_cast<float>(GetScreenHeight());
    float scale = std::min(screenWidth / static_cast<float>(pixelWidth_), screenHeight / static_cast<float>(pixelHeight_));
    float targetWidth = static_cast<float>(pixelWidth_) * scale;
    float targetHeight = static_cast<float>(pixelHeight_) * scale;
    Rectangle source = Rectangle{0.0f, 0.0f, static_cast<float>(pixelWidth_), -static_cast<float>(pixelHeight_)};
    Rectangle dest = Rectangle{(screenWidth - targetWidth) * 0.5f, (screenHeight - targetHeight) * 0.5f, targetWidth, targetHeight};
    DrawTexturePro(pixelTarget_.texture, source, dest, Vector2Zero(), 0.0f, WHITE);
    if (timeStopped_ || timeStopTintTimer_ > 0.0f) {
        float alpha = timeStopped_ ? 0.16f : timeStopTintTimer_ * 0.28f;
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), FadeColor(Color{80, 70, 145, 255}, alpha));
    }
}

void Game::DrawArena() const {
    if (IsSphericalMap()) {
        float radius = SphericalRadius();
        Color fillColor = IsHollowWorldMap() ? Color{12, 13, 18, 90} : Color{22, 21, 24, 255};
        Color wireColor = IsHollowWorldMap() ? Color{82, 95, 120, 190} : Color{86, 82, 90, 210};
        Color latColor = IsHollowWorldMap() ? Color{38, 48, 66, 170} : Color{48, 47, 52, 180};
        Color meridianColor = IsHollowWorldMap() ? Color{32, 42, 62, 155} : Color{42, 44, 50, 160};
        DrawSphereEx(Vector3Zero(), radius, 18, 12, fillColor);
        DrawSphereWires(Vector3Zero(), radius * (IsHollowWorldMap() ? 0.998f : 1.002f), 18, 12, wireColor);

        constexpr int kSegments = 72;
        for (int lat = -3; lat <= 3; ++lat) {
            float latitude = static_cast<float>(lat) * 0.32f;
            float y = std::sin(latitude) * radius;
            float ringRadius = std::cos(latitude) * radius;
            Vector3 previous = {};
            for (int i = 0; i <= kSegments; ++i) {
                float angle = static_cast<float>(i) / static_cast<float>(kSegments) * 6.2831853f;
                Vector3 point = Vector3{std::cos(angle) * ringRadius, y, std::sin(angle) * ringRadius};
                if (i > 0) {
                    DrawLine3D(previous, point, latColor);
                }
                previous = point;
            }
        }
        for (int meridian = 0; meridian < 8; ++meridian) {
            float longitude = static_cast<float>(meridian) / 8.0f * 6.2831853f;
            Vector3 previous = {};
            for (int i = 0; i <= kSegments; ++i) {
                float t = -1.5707963f + static_cast<float>(i) / static_cast<float>(kSegments) * 3.1415926f;
                Vector3 point = Vector3{
                    std::cos(t) * std::cos(longitude) * radius,
                    std::sin(t) * radius,
                    std::cos(t) * std::sin(longitude) * radius
                };
                if (i > 0) {
                    DrawLine3D(previous, point, meridianColor);
                }
                previous = point;
            }
        }
        return;
    }

    if (IsSquareMap()) {
        float size = squareHalfExtent_ * 2.0f;
        DrawCube(Vector3{0.0f, -0.08f, 0.0f}, size, 0.18f, size, Color{20, 20, 22, 255});
        DrawCubeWires(Vector3{0.0f, 0.02f, 0.0f}, size, 0.2f, size, Color{135, 130, 122, 255});
        for (int i = -3; i <= 3; ++i) {
            float p = static_cast<float>(i) * squareHalfExtent_ / 3.0f;
            DrawLine3D(Vector3{-squareHalfExtent_, 0.03f, p}, Vector3{squareHalfExtent_, 0.03f, p}, Color{45, 45, 48, 255});
            DrawLine3D(Vector3{p, 0.03f, -squareHalfExtent_}, Vector3{p, 0.03f, squareHalfExtent_}, Color{45, 45, 48, 255});
        }
        DrawLine3D(Vector3{-squareHalfExtent_, 0.08f, -squareHalfExtent_}, Vector3{squareHalfExtent_, 0.08f, -squareHalfExtent_}, Color{160, 150, 135, 255});
        DrawLine3D(Vector3{squareHalfExtent_, 0.08f, -squareHalfExtent_}, Vector3{squareHalfExtent_, 0.08f, squareHalfExtent_}, Color{160, 150, 135, 255});
        DrawLine3D(Vector3{squareHalfExtent_, 0.08f, squareHalfExtent_}, Vector3{-squareHalfExtent_, 0.08f, squareHalfExtent_}, Color{160, 150, 135, 255});
        DrawLine3D(Vector3{-squareHalfExtent_, 0.08f, squareHalfExtent_}, Vector3{-squareHalfExtent_, 0.08f, -squareHalfExtent_}, Color{160, 150, 135, 255});
        return;
    }

    DrawCylinder(Vector3{0.0f, -0.08f, 0.0f}, arenaRadius_, arenaRadius_, 0.18f, 24, Color{20, 20, 22, 255});
    DrawCylinderWires(Vector3{0.0f, 0.02f, 0.0f}, arenaRadius_, arenaRadius_, 0.2f, 24, Color{135, 130, 122, 255});

    for (int i = 0; i < 24; ++i) {
        float angle = (static_cast<float>(i) / 24.0f) * 6.2831853f;
        Vector3 end = Vector3{std::cos(angle) * arenaRadius_, 0.03f, std::sin(angle) * arenaRadius_};
        DrawLine3D(Vector3Zero(), end, Color{54, 52, 55, 255});
    }

    for (int ring = 1; ring <= 4; ++ring) {
        float radius = arenaRadius_ * static_cast<float>(ring) / 5.0f;
        DrawCylinderWires(Vector3{0.0f, 0.01f, 0.0f}, radius, radius, 0.02f, 24, Color{45, 45, 48, 255});
    }
}

void Game::DrawProps() const {
    for (const Prop& prop : props_) {
        if (IsSphericalMap()) {
            Vector3 normal = SphericalUpAt(prop.position);
            Vector3 center = Vector3Add(prop.position, Vector3Scale(normal, prop.scale.y * 0.35f));
            float radius = std::max(prop.scale.x, prop.scale.z);
            if (prop.shape == 1) {
                DrawSphereEx(center, radius, 5, 4, prop.color);
                DrawSphereWires(center, radius * 1.08f, 5, 4, Color{14, 14, 16, 210});
            } else {
                DrawCube(center, radius, radius * 0.85f, radius, prop.color);
                DrawCubeWires(center, radius, radius * 0.85f, radius, Color{14, 14, 16, 210});
            }
            continue;
        }

        Vector3 position = prop.position;
        position.y += prop.scale.y * 0.5f - 0.05f;

        if (prop.shape == 0) {
            DrawCube(position, prop.scale.x, prop.scale.y, prop.scale.z, prop.color);
            DrawCubeWires(position, prop.scale.x, prop.scale.y, prop.scale.z, Color{16, 16, 18, 255});
        } else if (prop.shape == 1) {
            DrawCylinder(position, prop.scale.x, prop.scale.x * 0.55f, prop.scale.y, 5, prop.color);
            DrawCylinderWires(position, prop.scale.x, prop.scale.x * 0.55f, prop.scale.y, 5, Color{16, 16, 18, 255});
        } else {
            Vector3 a = Vector3Add(prop.position, RotateY(Vector3{-prop.scale.x, 0.0f, -prop.scale.z}, prop.rotationY));
            Vector3 b = Vector3Add(prop.position, RotateY(Vector3{prop.scale.x, 0.0f, -prop.scale.z}, prop.rotationY));
            Vector3 c = Vector3Add(prop.position, RotateY(Vector3{0.0f, prop.scale.y, prop.scale.z}, prop.rotationY));
            DrawTriangle3D(a, b, c, prop.color);
            DrawLine3D(a, b, Color{16, 16, 18, 255});
            DrawLine3D(b, c, Color{16, 16, 18, 255});
            DrawLine3D(c, a, Color{16, 16, 18, 255});
        }
    }
}

void Game::DrawNanoPlatformFrame(const NanoPlatform& platform, Color color, bool dashed) const {
    Vector3 normal = IsSphericalMap() ? SafeNormalize(platform.normal, SphericalUpAt(platform.position)) : Vector3{0.0f, 1.0f, 0.0f};
    Vector3 right = IsSphericalMap() ? SafeNormalize(platform.right, PlayerRight()) : Vector3{1.0f, 0.0f, 0.0f};
    Vector3 forward = IsSphericalMap() ? SafeNormalize(platform.forward, PlayerForward()) : Vector3{0.0f, 0.0f, 1.0f};
    Vector3 topCenter = IsSphericalMap()
        ? Vector3Add(platform.position, Vector3Scale(normal, 0.035f))
        : Vector3{platform.position.x, platform.position.y + platform.scale.y + 0.035f, platform.position.z};
    Vector3 corners[4] = {
        Vector3Add(topCenter, Vector3Add(Vector3Scale(right, -platform.scale.x * 0.5f), Vector3Scale(forward, -platform.scale.z * 0.5f))),
        Vector3Add(topCenter, Vector3Add(Vector3Scale(right, platform.scale.x * 0.5f), Vector3Scale(forward, -platform.scale.z * 0.5f))),
        Vector3Add(topCenter, Vector3Add(Vector3Scale(right, platform.scale.x * 0.5f), Vector3Scale(forward, platform.scale.z * 0.5f))),
        Vector3Add(topCenter, Vector3Add(Vector3Scale(right, -platform.scale.x * 0.5f), Vector3Scale(forward, platform.scale.z * 0.5f)))
    };

    auto drawEdge = [&](Vector3 a, Vector3 b) {
        if (!dashed) {
            DrawLine3D(a, b, color);
            return;
        }
        constexpr int kSegments = 12;
        for (int i = 0; i < kSegments; i += 2) {
            float startT = static_cast<float>(i) / static_cast<float>(kSegments);
            float endT = static_cast<float>(i + 1) / static_cast<float>(kSegments);
            DrawLine3D(Vector3Lerp(a, b, startT), Vector3Lerp(a, b, endT), color);
        }
    };

    drawEdge(corners[0], corners[1]);
    drawEdge(corners[1], corners[2]);
    drawEdge(corners[2], corners[3]);
    drawEdge(corners[3], corners[0]);
    DrawLine3D(Vector3Lerp(corners[0], corners[2], 0.5f), corners[0], FadeColor(color, 0.45f));
    DrawLine3D(Vector3Lerp(corners[0], corners[2], 0.5f), corners[1], FadeColor(color, 0.45f));
    DrawLine3D(Vector3Lerp(corners[0], corners[2], 0.5f), corners[2], FadeColor(color, 0.45f));
    DrawLine3D(Vector3Lerp(corners[0], corners[2], 0.5f), corners[3], FadeColor(color, 0.45f));
}

void Game::DrawNanoPlatforms() const {
    if (activeWeapon_ == WeaponType::RiftCutter && riftCutterMode_ == RiftCutterMode::Platform && state_ == State::Playing) {
        NanoPlatform preview = MakeNanoPlatformTarget(PlayerForward());
        DrawNanoPlatformFrame(preview, FadeColor(Color{255, 238, 145, 255}, 0.68f), true);
    }

    for (const NanoPlatform& platform : nanoPlatforms_) {
        if (platform.delay > 0.0f) {
            float pulse = 0.55f + std::sin(static_cast<float>(GetTime()) * 18.0f) * 0.18f;
            DrawNanoPlatformFrame(platform, FadeColor(Color{255, 235, 140, 255}, pulse), true);
            continue;
        }

        float alpha = platform.maxLife > 0.0f ? std::clamp(platform.life / platform.maxLife, 0.0f, 1.0f) : 0.0f;
        Color fill = FadeColor(Color{255, 218, 92, 255}, 0.35f + alpha * 0.25f);
        if (IsSphericalMap()) {
            Vector3 normal = SafeNormalize(platform.normal, SphericalUpAt(platform.position));
            Vector3 right = SafeNormalize(platform.right, PlayerRight());
            Vector3 forward = SafeNormalize(platform.forward, PlayerForward());
            Vector3 top[4] = {
                Vector3Add(platform.position, Vector3Add(Vector3Scale(right, -platform.scale.x * 0.5f), Vector3Scale(forward, -platform.scale.z * 0.5f))),
                Vector3Add(platform.position, Vector3Add(Vector3Scale(right, platform.scale.x * 0.5f), Vector3Scale(forward, -platform.scale.z * 0.5f))),
                Vector3Add(platform.position, Vector3Add(Vector3Scale(right, platform.scale.x * 0.5f), Vector3Scale(forward, platform.scale.z * 0.5f))),
                Vector3Add(platform.position, Vector3Add(Vector3Scale(right, -platform.scale.x * 0.5f), Vector3Scale(forward, platform.scale.z * 0.5f)))
            };
            Vector3 bottomOffset = Vector3Scale(normal, -platform.scale.y);
            Vector3 bottom[4] = {
                Vector3Add(top[0], bottomOffset),
                Vector3Add(top[1], bottomOffset),
                Vector3Add(top[2], bottomOffset),
                Vector3Add(top[3], bottomOffset)
            };
            DrawTriangle3D(top[0], top[1], top[2], fill);
            DrawTriangle3D(top[0], top[2], top[3], fill);
            DrawTriangle3D(top[2], top[1], top[0], fill);
            DrawTriangle3D(top[3], top[2], top[0], fill);
            Color side = FadeColor(Color{255, 188, 68, 255}, 0.22f + alpha * 0.22f);
            for (int edge = 0; edge < 4; ++edge) {
                int next = (edge + 1) % 4;
                DrawTriangle3D(top[edge], bottom[edge], top[next], side);
                DrawTriangle3D(top[next], bottom[edge], bottom[next], side);
                DrawLine3D(top[edge], bottom[edge], FadeColor(Color{255, 244, 170, 255}, 0.68f));
            }
        } else {
            Vector3 center = platform.position;
            center.y += platform.scale.y * 0.5f;
            DrawCube(center, platform.scale.x, platform.scale.y, platform.scale.z, fill);
            DrawCubeWires(center, platform.scale.x, platform.scale.y, platform.scale.z, FadeColor(Color{255, 244, 170, 255}, 0.82f));
        }
        DrawNanoPlatformFrame(platform, FadeColor(Color{255, 245, 170, 255}, 0.72f), false);
    }
}

void Game::DrawEnemies() const {
    for (const Enemy& enemy : enemies_) {
        Vector3 position = BodyPosition(enemy.body);
        float pulse = 1.0f + std::sin(enemy.bobTimer * 5.0f) * 0.06f;

        if (enemy.type == EnemyType::Brute) {
            DrawCube(position, enemy.radius * 1.65f, enemy.radius * 1.65f, enemy.radius * 1.65f, enemy.color);
            DrawCubeWires(position, enemy.radius * 1.8f, enemy.radius * 1.8f, enemy.radius * 1.8f, Color{55, 8, 0, 255});
            for (int i = 0; i < 4; ++i) {
                float angle = static_cast<float>(i) * 1.5707963f;
                Vector3 spike = Vector3Add(position, Vector3{std::cos(angle) * enemy.radius * 1.35f, enemy.radius * 0.5f, std::sin(angle) * enemy.radius * 1.35f});
                DrawLine3D(position, spike, Color{255, 190, 120, 255});
            }
        } else if (enemy.type == EnemyType::Wisp) {
            DrawSphereEx(position, enemy.radius * pulse, 6, 6, enemy.color);
            DrawSphereWires(position, enemy.radius * 1.45f, 6, 6, Color{170, 235, 255, 200});
            DrawLine3D(Vector3Add(position, Vector3{-enemy.radius, 0.0f, 0.0f}), Vector3Add(position, Vector3{enemy.radius, 0.0f, 0.0f}), RAYWHITE);
        } else if (enemy.type == EnemyType::Spitter) {
            DrawSphereEx(position, enemy.radius * pulse, 7, 5, enemy.color);
            DrawCylinderWires(position, enemy.radius * 1.15f, enemy.radius * 0.55f, enemy.radius * 1.5f, 6, Color{35, 255, 190, 210});
            DrawSphereEx(Vector3Add(position, Vector3{0.0f, enemy.radius * 0.3f, 0.0f}), enemy.radius * 0.35f, 5, 4, Color{210, 255, 235, 240});
        } else if (enemy.type == EnemyType::Pouncer) {
            float squash = enemy.cooldownTimer > config_.pouncerLeapInterval - 0.35f ? 0.62f : 1.0f;
            DrawSphereEx(position, enemy.radius * pulse, 7, 5, enemy.color);
            DrawCube(Vector3Add(position, Vector3{0.0f, -enemy.radius * 0.18f, 0.0f}), enemy.radius * 2.1f, enemy.radius * squash, enemy.radius * 1.35f, FadeColor(enemy.color, 0.75f));
            DrawSphereWires(position, enemy.radius * 1.25f, 7, 5, Color{255, 160, 255, 190});
        } else if (enemy.type == EnemyType::Harrier) {
            Vector3 nose = Vector3Add(position, Vector3{0.0f, enemy.radius * 0.12f, -enemy.radius * 1.6f});
            Vector3 tail = Vector3Add(position, Vector3{0.0f, -enemy.radius * 0.06f, enemy.radius * 1.35f});
            Vector3 left = Vector3Add(position, Vector3{-enemy.radius * 1.35f, 0.0f, enemy.radius * 0.15f});
            Vector3 right = Vector3Add(position, Vector3{enemy.radius * 1.35f, 0.0f, enemy.radius * 0.15f});
            Vector3 top = Vector3Add(position, Vector3{0.0f, enemy.radius * 0.75f, 0.0f});
            Vector3 bottom = Vector3Add(position, Vector3{0.0f, -enemy.radius * 0.55f, 0.0f});
            Color fill = enemy.cooldownTimer < 0.22f ? Color{220, 255, 255, 255} : enemy.color;
            DrawTriangle3D(nose, left, top, fill);
            DrawTriangle3D(nose, top, right, fill);
            DrawTriangle3D(tail, top, left, FadeColor(fill, 0.75f));
            DrawTriangle3D(tail, right, top, FadeColor(fill, 0.75f));
            DrawTriangle3D(nose, bottom, left, FadeColor(fill, 0.72f));
            DrawTriangle3D(nose, right, bottom, FadeColor(fill, 0.72f));
            DrawLine3D(left, right, Color{230, 255, 255, 230});
            DrawSphereWires(position, enemy.radius * 1.18f, 6, 5, Color{150, 245, 255, 190});
        } else if (enemy.type == EnemyType::Blinker) {
            float warning = enemy.telegraphTimer > 0.0f ? 1.0f : 0.0f;
            Color core = warning > 0.0f ? Color{255, 220, 245, 255} : enemy.color;
            DrawSphereEx(position, enemy.radius * (0.82f + warning * 0.22f) * pulse, 6, 5, core);
            DrawCube(Vector3Add(position, Vector3{-enemy.radius * 0.35f, enemy.radius * 0.12f, 0.0f}), enemy.radius * 0.85f, enemy.radius * 0.42f, enemy.radius * 1.25f, FadeColor(enemy.color, 0.82f));
            DrawCube(Vector3Add(position, Vector3{enemy.radius * 0.42f, -enemy.radius * 0.16f, 0.0f}), enemy.radius * 0.7f, enemy.radius * 0.38f, enemy.radius * 1.05f, FadeColor(Color{120, 25, 90, 255}, 0.82f));
            DrawSphereWires(position, enemy.radius * (1.2f + warning * 0.38f), 7, 5, warning > 0.0f ? Color{255, 235, 255, 230} : Color{255, 115, 225, 175});
            if (warning > 0.0f) {
                DrawLine3D(Vector3Add(position, Vector3{-enemy.radius * 1.6f, 0.0f, 0.0f}), Vector3Add(position, Vector3{enemy.radius * 1.6f, 0.0f, 0.0f}), Color{255, 220, 245, 255});
                DrawLine3D(Vector3Add(position, Vector3{0.0f, -enemy.radius * 1.1f, 0.0f}), Vector3Add(position, Vector3{0.0f, enemy.radius * 1.4f, 0.0f}), Color{255, 220, 245, 255});
            }
        } else if (enemy.type == EnemyType::Boss) {
            float rage = enemy.health < enemy.maxHealth * 0.45f ? 1.0f : 0.0f;
            DrawCube(position, enemy.radius * 1.55f, enemy.radius * 1.15f, enemy.radius * 1.55f, enemy.color);
            DrawCubeWires(position, enemy.radius * 1.75f, enemy.radius * 1.3f, enemy.radius * 1.75f, Color{35, 18, 75, 255});
            DrawSphereEx(Vector3Add(position, Vector3{0.0f, enemy.radius * 0.85f, 0.0f}), enemy.radius * 0.72f, 8, 6, Color{175, 145, 255, 245});
            DrawSphereWires(position, enemy.radius * (1.35f + std::sin(enemy.bobTimer * 3.0f) * 0.08f), 12, 8, Color{215, 180, 255, 220});
            for (int i = 0; i < 6; ++i) {
                float angle = static_cast<float>(i) / 6.0f * 6.2831853f + enemy.bobTimer * (0.7f + rage * 0.7f);
                Vector3 spike = Vector3Add(position, Vector3{std::cos(angle) * enemy.radius * 1.35f, enemy.radius * 0.35f + std::sin(angle * 2.0f) * 0.35f, std::sin(angle) * enemy.radius * 1.35f});
                DrawLine3D(position, spike, Color{230, 205, 255, 255});
                DrawSphereEx(spike, enemy.radius * 0.13f, 5, 4, rage > 0.0f ? Color{255, 90, 190, 230} : Color{160, 220, 255, 230});
            }
        } else if (enemy.type == EnemyType::Duelist) {
            Color core = enemy.telegraphTimer > 0.0f ? Color{255, 245, 150, 255} : enemy.color;
            DrawSphereEx(position, enemy.radius * 0.92f * pulse, 7, 6, core);
            DrawCube(Vector3Add(position, Vector3{0.0f, enemy.radius * 0.9f, 0.0f}), enemy.radius * 0.9f, enemy.radius * 0.34f, enemy.radius * 0.9f, Color{70, 62, 84, 255});
            DrawSphereWires(position, enemy.radius * 1.32f, 8, 6, Color{255, 225, 135, 210});
            float weaponAngle = static_cast<float>(enemy.weaponSlot) / 8.0f * 6.2831853f + enemy.bobTimer * 2.0f;
            Vector3 focus = Vector3Add(position, Vector3{std::cos(weaponAngle) * enemy.radius * 1.35f, enemy.radius * 0.35f, std::sin(weaponAngle) * enemy.radius * 1.35f});
            Color weaponColor = enemy.weaponSlot == 2 ? Color{255, 145, 80, 255} : enemy.weaponSlot == 4 ? Color{150, 115, 255, 255} : enemy.weaponSlot == 7 ? Color{255, 220, 95, 255} : Color{155, 235, 255, 255};
            DrawSphereEx(focus, enemy.radius * 0.18f, 5, 4, weaponColor);
            DrawLine3D(position, focus, FadeColor(weaponColor, 0.85f));
        } else {
            DrawSphereEx(position, enemy.radius, 7, 6, enemy.color);
            DrawSphereWires(position, enemy.radius * 1.05f, 7, 6, Color{30, 0, 0, 255});
        }

        Vector3 top = Vector3Add(position, Vector3{0.0f, enemy.radius * 1.8f, 0.0f});
        DrawLine3D(position, top, Color{255, 220, 170, 255});
    }
}

void Game::DrawPickups() const {
    for (const Pickup& pickup : pickups_) {
        float bob = std::sin(pickup.bobTimer * 3.2f) * 0.12f;
        float pulse = 0.65f + std::sin(pickup.bobTimer * 5.0f) * 0.18f;
        Vector3 base = Vector3Add(pickup.position, Vector3{0.0f, bob, 0.0f});

        if (pickup.type == PickupType::SpaceSuit) {
            DrawCylinder(base, 0.26f, 0.32f, 0.62f, 6, Color{210, 220, 225, 255});
            DrawCylinderWires(base, 0.28f, 0.34f, 0.64f, 6, Color{35, 60, 70, 255});
            DrawSphereEx(Vector3Add(base, Vector3{0.0f, 0.52f, 0.0f}), 0.24f, 6, 5, Color{170, 225, 255, 235});
            DrawSphereWires(Vector3Add(base, Vector3{0.0f, 0.52f, 0.0f}), 0.27f, 6, 5, Color{225, 250, 255, 220});
            DrawCylinder(Vector3Add(base, Vector3{-0.22f, -0.02f, -0.18f}), 0.07f, 0.07f, 0.54f, 5, Color{70, 90, 105, 255});
            DrawCylinder(Vector3Add(base, Vector3{0.22f, -0.02f, -0.18f}), 0.07f, 0.07f, 0.54f, 5, Color{70, 90, 105, 255});
            DrawCylinderWires(Vector3Add(base, Vector3{0.0f, -0.42f, 0.0f}), 0.56f + pulse * 0.08f, 0.56f + pulse * 0.08f, 0.02f, 24, Color{95, 210, 255, 190});
        } else if (pickup.type == PickupType::FlightRig) {
            DrawSphereEx(base, 0.18f, 6, 5, Color{205, 250, 255, 235});
            DrawCylinderWires(base, 0.62f + pulse * 0.1f, 0.62f + pulse * 0.1f, 0.04f, 28, Color{135, 235, 255, 210});
            DrawCylinder(Vector3Add(base, Vector3{-0.42f, -0.12f, 0.0f}), 0.09f, 0.13f, 0.28f, 6, Color{85, 110, 120, 255});
            DrawCylinder(Vector3Add(base, Vector3{0.42f, -0.12f, 0.0f}), 0.09f, 0.13f, 0.28f, 6, Color{85, 110, 120, 255});
            DrawSphereEx(Vector3Add(base, Vector3{-0.42f, -0.34f, 0.0f}), 0.1f + pulse * 0.03f, 5, 4, Color{100, 235, 255, 210});
            DrawSphereEx(Vector3Add(base, Vector3{0.42f, -0.34f, 0.0f}), 0.1f + pulse * 0.03f, 5, 4, Color{100, 235, 255, 210});
        } else if (pickup.type == PickupType::Skates) {
            DrawCube(Vector3Add(base, Vector3{-0.2f, 0.06f, 0.0f}), 0.18f, 0.14f, 0.58f, Color{190, 220, 205, 255});
            DrawCube(Vector3Add(base, Vector3{0.2f, 0.06f, 0.0f}), 0.18f, 0.14f, 0.58f, Color{190, 220, 205, 255});
            DrawLine3D(Vector3Add(base, Vector3{-0.34f, -0.13f, -0.36f}), Vector3Add(base, Vector3{-0.06f, -0.13f, 0.36f}), Color{170, 255, 190, 230});
            DrawLine3D(Vector3Add(base, Vector3{0.06f, -0.13f, -0.36f}), Vector3Add(base, Vector3{0.34f, -0.13f, 0.36f}), Color{170, 255, 190, 230});
            DrawCylinderWires(Vector3Add(base, Vector3{0.0f, -0.18f, 0.0f}), 0.52f + pulse * 0.08f, 0.52f + pulse * 0.08f, 0.02f, 22, Color{155, 255, 185, 190});
        }
    }
}

void Game::DrawProjectiles() const {
    for (const Projectile& projectile : projectiles_) {
        Vector3 position = BodyPosition(projectile.body);
        Vector3 velocity = ToRayVector(physics_.Bodies().GetLinearVelocity(projectile.body));
        Vector3 displayVelocity = projectile.frozen || Vector3Length(velocity) <= 0.001f ? projectile.storedVelocity : velocity;
        Vector3 trail = Vector3Length(displayVelocity) > 0.001f ? Vector3Scale(Vector3Normalize(displayVelocity), -projectile.radius * 4.0f) : Vector3Zero();
        if (projectile.kind == ProjectileKind::Flame) {
            float age = projectile.maxLife > 0.0f ? 1.0f - std::clamp(projectile.life / projectile.maxLife, 0.0f, 1.0f) : 1.0f;
            Color core = Color{255, static_cast<unsigned char>(180 - age * 80.0f), 35, static_cast<unsigned char>(230 - age * 110.0f)};
            DrawSphereEx(position, projectile.radius, 6, 5, core);
            DrawSphereEx(position, projectile.radius * 0.62f, 5, 4, FadeColor(Color{255, 235, 130, 255}, 0.65f - age * 0.28f));
        } else if (projectile.kind == ProjectileKind::Rocket) {
            Vector3 forward = Vector3Length(displayVelocity) > 0.001f ? Vector3Normalize(displayVelocity) : PlayerForward();
            Vector3 nose = Vector3Add(position, Vector3Scale(forward, projectile.radius * 1.55f));
            Vector3 tail = Vector3Subtract(position, Vector3Scale(forward, projectile.radius * 1.05f));
            DrawCylinderEx(tail, nose, projectile.radius * 0.68f, projectile.radius * 0.28f, 6, projectile.color);
            DrawCylinderWiresEx(tail, nose, projectile.radius * 0.74f, projectile.radius * 0.34f, 6, Color{50, 55, 48, 255});
            DrawSphereEx(tail, projectile.radius * 0.58f, 5, 4, Color{255, 120, 28, 220});
            DrawLine3D(tail, Vector3Add(tail, Vector3Scale(trail, 1.5f)), FadeColor(Color{255, 155, 45, 255}, 0.85f));
        } else if (projectile.kind == ProjectileKind::EnemyShot) {
            DrawSphereEx(position, projectile.radius * 1.25f, 6, 4, projectile.color);
            DrawSphereWires(position, projectile.radius * 1.8f, 6, 4, FadeColor(Color{190, 255, 245, 255}, 0.8f));
            DrawLine3D(position, Vector3Add(position, Vector3Scale(trail, 1.3f)), FadeColor(projectile.color, 0.7f));
        } else if (projectile.kind == ProjectileKind::GravityNail) {
            Vector3 forward = Vector3Length(displayVelocity) > 0.001f ? Vector3Normalize(displayVelocity) : PlayerForward();
            Vector3 tip = Vector3Add(position, Vector3Scale(forward, projectile.radius * 2.4f));
            Vector3 tail = Vector3Subtract(position, Vector3Scale(forward, projectile.radius * 1.6f));
            DrawCylinderEx(tail, tip, projectile.radius * 0.28f, projectile.radius * 0.08f, 5, projectile.color);
            DrawLine3D(tail, Vector3Add(tail, Vector3Scale(trail, 1.2f)), FadeColor(Color{145, 165, 255, 255}, 0.75f));
        } else if (projectile.kind == ProjectileKind::BlackHoleGrenade) {
            float spin = static_cast<float>(GetTime()) * 6.0f;
            DrawSphereEx(position, projectile.radius * 1.25f, 7, 5, projectile.color);
            DrawSphereWires(position, projectile.radius * (2.0f + std::sin(spin) * 0.25f), 8, 5, FadeColor(Color{170, 90, 255, 255}, 0.85f));
            DrawCylinderWires(position, projectile.radius * 2.15f, projectile.radius * 1.25f, 0.03f, 12, FadeColor(Color{90, 190, 255, 255}, 0.65f));
        } else if (projectile.kind == ProjectileKind::Lance) {
            Vector3 forward = Vector3Length(displayVelocity) > 0.001f ? Vector3Normalize(displayVelocity) : PlayerForward();
            Vector3 tip = Vector3Add(position, Vector3Scale(forward, projectile.radius * 4.4f));
            Vector3 tail = Vector3Subtract(position, Vector3Scale(forward, projectile.radius * 3.4f));
            DrawCylinderEx(tail, tip, projectile.radius * 0.34f, projectile.radius * 0.08f, 6, projectile.color);
            DrawCylinderWiresEx(tail, tip, projectile.radius * 0.38f, projectile.radius * 0.1f, 6, FadeColor(Color{245, 255, 255, 255}, 0.8f));
            DrawSphereEx(tip, projectile.radius * 0.58f, 6, 4, Color{245, 255, 255, 240});
            DrawLine3D(tail, Vector3Add(tail, Vector3Scale(trail, 2.2f)), FadeColor(Color{190, 240, 255, 255}, 0.9f));
        } else if (projectile.kind == ProjectileKind::GlassShard) {
            Vector3 forward = Vector3Length(displayVelocity) > 0.001f ? Vector3Normalize(displayVelocity) : PlayerForward();
            Vector3 tip = Vector3Add(position, Vector3Scale(forward, projectile.radius * 1.7f));
            Vector3 left = Vector3Add(position, Vector3Scale(PlayerRight(), projectile.radius * 0.9f));
            Vector3 right = Vector3Subtract(position, Vector3Scale(PlayerRight(), projectile.radius * 0.9f));
            DrawTriangle3D(tip, left, right, FadeColor(projectile.color, 0.82f));
            DrawLine3D(position, Vector3Add(position, trail), FadeColor(Color{190, 245, 255, 255}, 0.55f));
        } else if (projectile.kind == ProjectileKind::Pellet) {
            DrawLine3D(position, Vector3Add(position, trail), FadeColor(projectile.color, 0.65f));
            DrawSphereEx(position, projectile.radius, 4, 3, projectile.color);
        } else if (projectile.kind == ProjectileKind::DroneCanister) {
            DrawCylinder(position, projectile.radius * 1.25f, projectile.radius * 1.7f, projectile.radius * 2.0f, 8, projectile.color);
            DrawCylinderWires(position, projectile.radius * 1.3f, projectile.radius * 1.75f, projectile.radius * 2.0f, 8, Color{100, 110, 125, 255});
        } else if (projectile.kind == ProjectileKind::DroneBullet) {
            DrawCube(position, projectile.radius * 3.2f, projectile.radius * 0.8f, projectile.radius * 0.8f, projectile.color);
            DrawLine3D(position, Vector3Add(position, trail), FadeColor(Color{255, 250, 200, 255}, 0.7f));
        } else {
            DrawLine3D(position, Vector3Add(position, trail), FadeColor(projectile.color, 0.7f));
            DrawSphereEx(position, projectile.radius, 5, 4, projectile.color);
            DrawSphereWires(position, projectile.radius * 1.35f, 6, 6, FadeColor(projectile.color, 0.7f));
        }
    }
}

void Game::DrawBeams() const {
    for (const Beam& beam : beams_) {
        float alpha = beam.maxLife > 0.0f ? beam.life / beam.maxLife : 0.0f;
        float pulse = 0.78f + std::sin((1.0f - alpha) * 18.0f) * 0.16f;
        float outerRadius = beam.width * 0.22f * pulse;
        float coreRadius = std::max(0.05f, outerRadius * 0.38f);
        Color glow = FadeColor(beam.color, alpha * 0.55f);
        Color shell = FadeColor(Color{80, 190, 255, 255}, alpha * 0.75f);
        Color core = FadeColor(Color{230, 255, 255, 255}, alpha);

        DrawCylinderEx(beam.start, beam.end, outerRadius, outerRadius * 0.82f, 7, glow);
        DrawCylinderEx(beam.start, beam.end, outerRadius * 0.58f, outerRadius * 0.45f, 7, shell);
        DrawCylinderEx(beam.start, beam.end, coreRadius, coreRadius, 6, core);
        DrawCylinderWiresEx(beam.start, beam.end, outerRadius * 1.08f, outerRadius * 0.92f, 7, FadeColor(Color{190, 245, 255, 255}, alpha * 0.9f));
        DrawSphereEx(beam.start, outerRadius * (1.1f + beam.charge * 0.4f), 7, 5, FadeColor(Color{180, 240, 255, 255}, alpha * 0.8f));
        DrawSphereEx(beam.end, outerRadius * (1.35f + beam.charge * 0.8f), 7, 5, FadeColor(Color{220, 255, 255, 255}, alpha));
    }
}

void Game::DrawShockwaves() const {
    for (const Shockwave& shockwave : shockwaves_) {
        float age = shockwave.maxLife > 0.0f ? 1.0f - shockwave.life / shockwave.maxLife : 1.0f;
        float radius = shockwave.radius * (0.18f + age * 0.82f);
        float alpha = shockwave.maxLife > 0.0f ? shockwave.life / shockwave.maxLife : 0.0f;
        Color color = FadeColor(shockwave.color, alpha * 0.85f);
        DrawSphereWires(shockwave.position, radius, 14, 8, color);
        DrawCylinderWires(shockwave.position, radius, radius, 0.03f, 30, FadeColor(Color{190, 230, 255, 255}, alpha * 0.55f));
    }
}

void Game::DrawHeatwaves() const {
    constexpr int kRayCount = 9;
    constexpr int kArcSteps = 16;
    constexpr int kLayerCount = 3;

    for (const HeatwavePulse& heatwave : heatwaves_) {
        float age = heatwave.maxLife > 0.0f ? 1.0f - heatwave.life / heatwave.maxLife : 1.0f;
        float alpha = heatwave.maxLife > 0.0f ? heatwave.life / heatwave.maxLife : 0.0f;
        float currentRange = heatwave.range * (0.18f + age * 0.82f);
        Vector3 forward = Vector3Length(heatwave.forward) > 0.001f ? Vector3Normalize(heatwave.forward) : PlayerForward();
        Vector3 localUp = IsSphericalMap() ? SphericalUpAt(heatwave.origin) : Vector3{0.0f, 1.0f, 0.0f};
        Vector3 right = Vector3CrossProduct(forward, localUp);
        if (Vector3Length(right) <= 0.001f) {
            right = PlayerRight();
        } else {
            right = Vector3Normalize(right);
        }
        Vector3 up = Vector3Normalize(Vector3CrossProduct(right, forward));
        Color rayColor = FadeColor(heatwave.color, alpha * 0.7f);
        Color edgeColor = FadeColor(Color{255, 215, 145, 255}, alpha * 0.45f);
        Color fillColor = FadeColor(Color{255, 118, 34, 255}, alpha * 0.48f);
        Color hotFillColor = FadeColor(Color{255, 220, 120, 255}, alpha * 0.38f);
        Vector3 previousLeftEdge = {};
        Vector3 previousRightEdge = {};
        Vector3 previousLayerOrigin = {};
        bool hasPreviousLayer = false;

        for (int layer = 0; layer < kLayerCount; ++layer) {
            float layerT = static_cast<float>(layer) / static_cast<float>(kLayerCount - 1);
            float verticalOffset = (layerT - 0.5f) * currentRange * 0.42f;
            float layerRange = currentRange * (1.0f - std::abs(layerT - 0.5f) * 0.12f);
            Color layerRay = FadeColor(rayColor, alpha * (0.45f + (1.0f - std::abs(layerT - 0.5f) * 2.0f) * 0.3f));
            Color layerEdge = FadeColor(edgeColor, alpha * (0.32f + (1.0f - std::abs(layerT - 0.5f) * 2.0f) * 0.28f));
            Color layerFill = layer == 1 ? hotFillColor : fillColor;
            Vector3 layerOrigin = Vector3Add(heatwave.origin, Vector3Scale(up, verticalOffset * 0.25f));

            Vector3 previousArc = {};
            bool hasPreviousArc = false;
            Vector3 leftMost = {};
            Vector3 rightMost = {};
            for (int i = 0; i < kRayCount; ++i) {
                float t = kRayCount > 1 ? static_cast<float>(i) / static_cast<float>(kRayCount - 1) : 0.5f;
                float angle = (t - 0.5f) * heatwave.halfAngle * 2.0f;
                Vector3 direction = Vector3Normalize(Vector3Add(Vector3Scale(forward, std::cos(angle)), Vector3Scale(right, std::sin(angle))));
                Vector3 crown = Vector3Scale(up, (std::sin(3.1415926f * t) * 0.18f * layerRange) + verticalOffset);
                Vector3 end = Vector3Add(heatwave.origin, Vector3Add(Vector3Scale(direction, layerRange), crown));
                DrawLine3D(layerOrigin, end, layerRay);
                if (i == 0) {
                    leftMost = end;
                } else if (i == kRayCount - 1) {
                    rightMost = end;
                }
            }

            for (int step = 0; step <= kArcSteps; ++step) {
                float t = static_cast<float>(step) / static_cast<float>(kArcSteps);
                float angle = (t - 0.5f) * heatwave.halfAngle * 2.0f;
                Vector3 direction = Vector3Normalize(Vector3Add(Vector3Scale(forward, std::cos(angle)), Vector3Scale(right, std::sin(angle))));
                Vector3 crown = Vector3Scale(up, (std::sin(3.1415926f * t) * 0.18f * layerRange) + verticalOffset);
                Vector3 arcPoint = Vector3Add(heatwave.origin, Vector3Add(Vector3Scale(direction, layerRange), crown));
                if (hasPreviousArc) {
                    DrawTriangle3D(layerOrigin, previousArc, arcPoint, layerFill);
                    DrawTriangle3D(layerOrigin, arcPoint, previousArc, layerFill);
                    DrawLine3D(previousArc, arcPoint, layerEdge);
                }
                previousArc = arcPoint;
                hasPreviousArc = true;
            }

            if (hasPreviousLayer) {
                Color sideFill = FadeColor(Color{255, 85, 28, 255}, alpha * 0.28f);
                DrawTriangle3D(previousLayerOrigin, previousLeftEdge, leftMost, sideFill);
                DrawTriangle3D(previousLayerOrigin, leftMost, previousLeftEdge, sideFill);
                DrawTriangle3D(previousLayerOrigin, leftMost, layerOrigin, sideFill);
                DrawTriangle3D(previousLayerOrigin, layerOrigin, leftMost, sideFill);
                DrawTriangle3D(previousLayerOrigin, previousRightEdge, rightMost, sideFill);
                DrawTriangle3D(previousLayerOrigin, rightMost, previousRightEdge, sideFill);
                DrawTriangle3D(previousLayerOrigin, rightMost, layerOrigin, sideFill);
                DrawTriangle3D(previousLayerOrigin, layerOrigin, rightMost, sideFill);
            }
            previousLeftEdge = leftMost;
            previousRightEdge = rightMost;
            previousLayerOrigin = layerOrigin;
            hasPreviousLayer = true;
        }

        Vector3 leftEdge = Vector3Normalize(Vector3Add(Vector3Scale(forward, std::cos(-heatwave.halfAngle)), Vector3Scale(right, std::sin(-heatwave.halfAngle))));
        Vector3 rightEdge = Vector3Normalize(Vector3Add(Vector3Scale(forward, std::cos(heatwave.halfAngle)), Vector3Scale(right, std::sin(heatwave.halfAngle))));
        for (float vertical : {-0.21f, 0.21f}) {
            Vector3 offset = Vector3Scale(up, currentRange * vertical);
            DrawLine3D(
                Vector3Add(heatwave.origin, offset),
                Vector3Add(heatwave.origin, Vector3Add(Vector3Scale(leftEdge, currentRange * 0.95f), offset)),
                FadeColor(Color{255, 95, 35, 255}, alpha * 0.45f));
            DrawLine3D(
                Vector3Add(heatwave.origin, offset),
                Vector3Add(heatwave.origin, Vector3Add(Vector3Scale(rightEdge, currentRange * 0.95f), offset)),
                FadeColor(Color{255, 95, 35, 255}, alpha * 0.45f));
        }

        Color curtain = FadeColor(Color{255, 145, 46, 255}, alpha * 0.34f);
        Vector3 topOffset = Vector3Scale(up, currentRange * 0.21f);
        Vector3 bottomOffset = Vector3Scale(up, -currentRange * 0.21f);
        Vector3 leftTop = Vector3Add(heatwave.origin, Vector3Add(Vector3Scale(leftEdge, currentRange * 0.95f), topOffset));
        Vector3 leftBottom = Vector3Add(heatwave.origin, Vector3Add(Vector3Scale(leftEdge, currentRange * 0.95f), bottomOffset));
        Vector3 rightTop = Vector3Add(heatwave.origin, Vector3Add(Vector3Scale(rightEdge, currentRange * 0.95f), topOffset));
        Vector3 rightBottom = Vector3Add(heatwave.origin, Vector3Add(Vector3Scale(rightEdge, currentRange * 0.95f), bottomOffset));
        DrawTriangle3D(leftTop, rightTop, leftBottom, curtain);
        DrawTriangle3D(rightTop, rightBottom, leftBottom, curtain);
        DrawTriangle3D(leftTop, leftBottom, rightTop, curtain);
        DrawTriangle3D(rightTop, leftBottom, rightBottom, curtain);
        DrawSphereEx(heatwave.origin, 0.18f + age * 0.18f, 6, 4, FadeColor(Color{255, 235, 175, 255}, alpha * 0.65f));
    }
}

void Game::DrawGravityWells() const {
    for (const GravityWell& well : gravityWells_) {
        float alpha = well.maxLife > 0.0f ? well.life / well.maxLife : 0.0f;
        float pulse = 0.7f + std::sin((1.0f - alpha) * 16.0f) * 0.18f;
        Color ring = FadeColor(well.blackHole ? Color{115, 55, 220, 255} : Color{150, 120, 255, 255}, alpha * 0.95f);
        Color core = well.blackHole ? BLACK : FadeColor(Color{90, 205, 255, 255}, alpha * 0.85f);
        Vector3 wellUp = IsSphericalMap() ? SphericalUpAt(well.position) : Vector3{0.0f, 1.0f, 0.0f};
        Vector3 ringStart = Vector3Subtract(well.position, Vector3Scale(wellUp, 0.02f));
        Vector3 ringEnd = Vector3Add(well.position, Vector3Scale(wellUp, 0.02f));
        DrawCylinderWiresEx(ringStart, ringEnd, well.radius * pulse, well.radius * pulse, 28, ring);
        DrawCylinderWiresEx(Vector3Add(ringStart, Vector3Scale(wellUp, 0.35f)), Vector3Add(ringEnd, Vector3Scale(wellUp, 0.35f)), well.radius * 0.45f * pulse, well.radius * 0.45f * pulse, 18, FadeColor(core, alpha * 0.75f));
        if (well.blackHole) {
            DrawSphere(well.position, config_.blackHoleEventHorizonRadius, BLACK);
            DrawSphereWires(well.position, config_.blackHoleEventHorizonRadius * (1.04f + std::sin(static_cast<float>(GetTime()) * 7.0f) * 0.025f), 12, 8, FadeColor(Color{80, 35, 130, 255}, alpha * 0.85f));
        } else {
            DrawSphereEx(well.position, 0.18f + (1.0f - alpha) * 0.06f, 6, 4, core);
        }
        int spokes = well.blackHole ? 10 : 6;
        Vector3 basisA = SafeNormalize(ProjectOnSphericalTangent(PlayerRight(), wellUp), Vector3{1.0f, 0.0f, 0.0f});
        Vector3 basisB = SafeNormalize(Vector3CrossProduct(wellUp, basisA), Vector3{0.0f, 0.0f, 1.0f});
        for (int i = 0; i < spokes; ++i) {
            float angle = (static_cast<float>(i) / static_cast<float>(spokes)) * 6.2831853f + static_cast<float>(GetTime()) * (well.blackHole ? -3.4f : 1.7f);
            Vector3 tangentOffset = Vector3Add(Vector3Scale(basisA, std::cos(angle) * well.radius * (well.blackHole ? 0.9f : 0.72f)), Vector3Scale(basisB, std::sin(angle) * well.radius * (well.blackHole ? 0.9f : 0.72f)));
            Vector3 outer = Vector3Add(well.position, Vector3Add(tangentOffset, Vector3Scale(wellUp, 0.08f + (well.blackHole ? std::sin(angle * 2.0f) * 0.28f : 0.0f))));
            DrawLine3D(outer, well.position, FadeColor(well.blackHole ? Color{180, 95, 255, 255} : Color{140, 190, 255, 255}, alpha * 0.55f));
        }
    }
}

void Game::DrawRifts() const {
    for (const RiftSlash& rift : rifts_) {
        Vector3 right = Vector3Length(rift.right) > 0.001f ? Vector3Normalize(rift.right) : PlayerRight();
        Vector3 up = Vector3Length(rift.up) > 0.001f ? Vector3Normalize(rift.up) : PlayerUp();
        if (rift.delay > 0.0f) {
            float pulse = 0.45f + std::sin(static_cast<float>(GetTime()) * 34.0f) * 0.18f;
            Color preflash = FadeColor(Color{255, 235, 150, 255}, pulse * 0.45f);
            DrawLine3D(Vector3Subtract(rift.center, Vector3Scale(right, rift.radius * 0.35f)), Vector3Add(rift.center, Vector3Scale(right, rift.radius * 0.35f)), preflash);
            DrawLine3D(Vector3Subtract(rift.center, Vector3Scale(up, rift.radius * 0.18f)), Vector3Add(rift.center, Vector3Scale(up, rift.radius * 0.18f)), preflash);
            continue;
        }

        constexpr int kSegments = 18;
        float alpha = rift.maxLife > 0.0f ? rift.life / rift.maxLife : 0.0f;
        float birth = 1.0f - alpha;
        float scale = 0.72f + std::min(1.0f, birth * 4.0f) * 0.28f;
        float outerRadius = rift.radius * scale;
        float innerRadius = std::max(0.05f, (rift.radius - rift.thickness) * scale);
        Color fill = FadeColor(Color{255, 210, 70, 255}, alpha * 0.42f);
        Color bright = FadeColor(Color{255, 245, 175, 255}, alpha * 0.78f);
        Color edge = FadeColor(Color{255, 170, 42, 255}, alpha * 0.9f);

        Vector3 previousOuter = {};
        Vector3 previousInner = {};
        bool hasPrevious = false;
        for (int i = 0; i <= kSegments; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(kSegments);
            float angle = (-0.82f + t * 1.64f) * 3.1415926f;
            float x = std::cos(angle);
            float y = std::sin(angle) * 0.72f + 0.3f;
            Vector3 outer = Vector3Add(rift.center, Vector3Add(Vector3Scale(right, x * outerRadius), Vector3Scale(up, y * outerRadius)));
            Vector3 inner = Vector3Add(rift.center, Vector3Add(Vector3Scale(right, x * innerRadius * 0.82f), Vector3Scale(up, (y * innerRadius) + rift.thickness * 0.38f * scale)));
            if (hasPrevious) {
                DrawTriangle3D(previousOuter, outer, previousInner, fill);
                DrawTriangle3D(previousOuter, previousInner, outer, fill);
                DrawTriangle3D(outer, inner, previousInner, fill);
                DrawTriangle3D(outer, previousInner, inner, fill);
                DrawLine3D(previousOuter, outer, edge);
                DrawLine3D(previousInner, inner, bright);
            }
            previousOuter = outer;
            previousInner = inner;
            hasPrevious = true;
        }

        Vector3 centerGlow = Vector3Add(rift.center, Vector3Scale(up, rift.radius * 0.25f * scale));
        DrawSphereEx(centerGlow, 0.08f + birth * 0.06f, 6, 4, FadeColor(Color{255, 245, 190, 255}, alpha * 0.7f));
    }
}

void Game::DrawDrones() const {
    for (const Drone& drone : drones_) {
        Vector3 up = IsSphericalMap() ? SphericalUpAt(drone.position) : Vector3{0.0f, 1.0f, 0.0f};
        Vector3 forward = IsSphericalMap()
            ? SafeNormalize(ProjectOnSphericalTangent(Vector3{0.0f, 0.0f, -1.0f}, up), Vector3{1.0f, 0.0f, 0.0f})
            : Vector3{0.0f, 0.0f, -1.0f};
        if (Vector3Length(forward) <= 0.001f) forward = Vector3{1.0f, 0.0f, 0.0f};
        forward = Vector3Normalize(forward);
        Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, up));

        if (drone.state == DroneState::Deploying) {
            float pulse = (1.0f - drone.deployTimer);
            float alpha = 0.45f + 0.55f * std::abs(std::sin(pulse * 10.0f));
            DrawCube(drone.position, 0.55f, 0.4f, 0.55f, FadeColor(Color{145, 160, 175, 255}, alpha));
            DrawCubeWires(drone.position, 0.55f, 0.4f, 0.55f, FadeColor(Color{200, 210, 220, 255}, alpha * 0.7f));
            continue;
        }

        // Center body
        Color bodyColor = Color{78, 88, 98, 255};
        Color armColor = Color{100, 110, 120, 255};
        Color rotorColor = Color{175, 185, 195, 255};
        DrawCube(Vector3Add(drone.position, Vector3Scale(up, 0.08f)), 0.5f, 0.22f, 0.5f, bodyColor);

        // 4 arms and rotors
        float armLen = 0.55f;
        float rotorR = 0.32f;
        float spin = drone.bobTimer * 14.0f;
        float armAngles[4] = {0.785398f, 2.356194f, -2.356194f, -0.785398f};

        for (int j = 0; j < 4; ++j) {
            float a = armAngles[j];
            Vector3 rotorCenter = Vector3Add(drone.position,
                Vector3Add(Vector3Scale(right, std::cos(a) * armLen),
                           Vector3Scale(forward, std::sin(a) * armLen)));
            // Arm
            DrawCylinderEx(rotorCenter,
                Vector3Add(rotorCenter, Vector3Scale(up, 0.05f)),
                0.04f, 0.04f, 6, armColor);
            DrawLine3D(Vector3Add(drone.position, Vector3Scale(up, 0.05f)), rotorCenter, armColor);

            // Spinning rotor disc
            DrawCylinder(rotorCenter, rotorR, rotorR, 0.04f, 8, rotorColor);
            // Rotor spin lines
            float r1 = spin + static_cast<float>(j) * 1.57f;
            DrawLine3D(
                Vector3Add(rotorCenter, Vector3Add(Vector3Scale(right, std::cos(r1) * rotorR * 0.8f), Vector3Scale(forward, std::sin(r1) * rotorR * 0.8f))),
                Vector3Add(rotorCenter, Vector3Add(Vector3Scale(right, -std::cos(r1) * rotorR * 0.8f), Vector3Scale(forward, -std::sin(r1) * rotorR * 0.8f))),
                FadeColor(rotorColor, 0.6f));
        }
    }
}

void Game::DrawDashedCircle3D(Vector3 center, float radius, Vector3 normal, Color color) const {
    Vector3 u, v;
    if (std::abs(normal.x) < 0.9f) {
        u = Vector3Normalize(Vector3CrossProduct(normal, Vector3{1.0f, 0.0f, 0.0f}));
    } else {
        u = Vector3Normalize(Vector3CrossProduct(normal, Vector3{0.0f, 1.0f, 0.0f}));
    }
    v = Vector3Normalize(Vector3CrossProduct(normal, u));

    constexpr int kSegments = 36;
    for (int i = 0; i < kSegments; i += 2) {
        float a1 = static_cast<float>(i) / kSegments * 2.0f * PI;
        float a2 = static_cast<float>(i + 1) / kSegments * 2.0f * PI;
        Vector3 p1 = Vector3Add(center, Vector3Add(Vector3Scale(u, std::cos(a1) * radius), Vector3Scale(v, std::sin(a1) * radius)));
        Vector3 p2 = Vector3Add(center, Vector3Add(Vector3Scale(u, std::cos(a2) * radius), Vector3Scale(v, std::sin(a2) * radius)));
        DrawLine3D(p1, p2, color);
    }
}

void Game::DrawRallyMarker() const {
    Color markerColor = Color{255, 210, 80, 230};
    Color lineColor = FadeColor(markerColor, 0.75f);
    float radius = 1.8f;
    float markerAltitude = 0.35f;

    if (rallyPhase_ != RallyPhase::Inactive) {
        // Draw rally point as a solid marker
        Vector3 rallyNormal = IsSphericalMap() ? SphericalUpAt(rallyPoint_) : Vector3{0.0f, 1.0f, 0.0f};
        Vector3 elevatedRally = Vector3Add(rallyPoint_, Vector3Scale(rallyNormal, markerAltitude));
        DrawDashedCircle3D(elevatedRally, radius, rallyNormal, lineColor);
        DrawDashedCircle3D(elevatedRally, radius * 0.42f, rallyNormal, FadeColor(lineColor, 0.7f));
        // Cross lines
        Vector3 u, v;
        if (std::abs(rallyNormal.x) < 0.9f) {
            u = Vector3Normalize(Vector3CrossProduct(rallyNormal, Vector3{1.0f, 0.0f, 0.0f}));
        } else {
            u = Vector3Normalize(Vector3CrossProduct(rallyNormal, Vector3{0.0f, 1.0f, 0.0f}));
        }
        v = Vector3Normalize(Vector3CrossProduct(rallyNormal, u));
        DrawLine3D(Vector3Add(elevatedRally, Vector3Scale(u, -radius)), Vector3Add(elevatedRally, Vector3Scale(u, radius)), FadeColor(markerColor, 0.5f));
        DrawLine3D(Vector3Add(elevatedRally, Vector3Scale(v, -radius)), Vector3Add(elevatedRally, Vector3Scale(v, radius)), FadeColor(markerColor, 0.5f));
        // Vertical stalk
        DrawLine3D(rallyPoint_, elevatedRally, FadeColor(markerColor, 0.45f));
    } else if (fireControlActive_) {
        // Preview marker at aim point
        Vector3 aimPoint = GetFireControlAimPoint();
        Vector3 aimNormal = IsSphericalMap() ? SphericalUpAt(aimPoint) : Vector3{0.0f, 1.0f, 0.0f};
        Vector3 elevatedAim = Vector3Add(aimPoint, Vector3Scale(aimNormal, markerAltitude));
        float pulse = 0.62f + std::sin(static_cast<float>(GetTime()) * 10.0f) * 0.16f;
        Color previewColor = FadeColor(markerColor, pulse);
        DrawDashedCircle3D(elevatedAim, radius, aimNormal, previewColor);
        DrawLine3D(aimPoint, elevatedAim, FadeColor(previewColor, 0.6f));
    }
}

void Game::DrawBlinkIndicator() const {
    if (activeWeapon_ != WeaponType::InfinityGauntlet || gauntletMode_ != GauntletMode::Blink) return;
    Vector3 start = camera_.position;
    Vector3 forward = PlayerForward();
    float travel = config_.blinkDistance * blinkDistanceScale_;
    // Clamp travel to arena boundaries (simplified, mirrors Blink() logic)
    if (!IsSphericalMap()) {
        float maxDistance = arenaRadius_ - playerRadius_;
        Vector3 flatStart = Vector3{start.x, 0.0f, start.z};
        Vector3 flatForward = Vector3{forward.x, 0.0f, forward.z};
        float a = Vector3DotProduct(flatForward, flatForward);
        if (a > 0.0001f) {
            float b = 2.0f * Vector3DotProduct(flatStart, flatForward);
            float c = Vector3DotProduct(flatStart, flatStart) - maxDistance * maxDistance;
            float det = b * b - 4.0f * a * c;
            if (det >= 0.0f) {
                float boundaryT = (-b + std::sqrt(det)) / (2.0f * a);
                if (boundaryT >= 0.0f) travel = std::min(travel, std::max(0.0f, boundaryT - 0.15f));
            }
        }
    }
    Vector3 target = Vector3Add(start, Vector3Scale(forward, travel));
    // Clamp Y for flat maps
    if (!IsSphericalMap()) {
        target.y = std::max(playerRadius_, std::min(target.y, config_.flightMaxAltitude));
        if (IsSquareMap()) {
            target.x = std::clamp(target.x, -squareHalfExtent_ + playerRadius_, squareHalfExtent_ - playerRadius_);
            target.z = std::clamp(target.z, -squareHalfExtent_ + playerRadius_, squareHalfExtent_ - playerRadius_);
        }
    }
    // Three mutually perpendicular dashed circles forming a sphere outline
    float r = 1.2f;
    Color c = FadeColor(Color{175, 130, 255, 255}, 0.55f);
    DrawDashedCircle3D(target, r, Vector3{1.0f, 0.0f, 0.0f}, c);  // YZ plane
    DrawDashedCircle3D(target, r, Vector3{0.0f, 1.0f, 0.0f}, c);  // XZ plane
    DrawDashedCircle3D(target, r, Vector3{0.0f, 0.0f, 1.0f}, c);  // XY plane
}

void Game::DrawFireControlOverlay() const {
    int w = pixelWidth_;
    int h = pixelHeight_;
    int cx = w / 2;
    int cy = h / 2;

    Color tacGreen = Color{100, 220, 160, 255};
    Color tacDim = Color{110, 140, 125, 200};
    Color tacBright = Color{190, 240, 215, 240};

    // ── Top command bar ──────────────────────────────────────────────
    DrawText("DRONE COMMAND", 6, 4, 8, tacGreen);
    DrawLine(0, 17, w, 17, FadeColor(tacGreen, 0.30f));

    // ── Info panel (top-left, below title bar) ───────────────────────
    int activeDrones = 0;
    for (const Drone& d : drones_) {
        if (d.state == DroneState::Active) activeDrones++;
    }
    int enemyCount = static_cast<int>(enemies_.size());

    DrawText(TextFormat("DRONES    %d/%d", activeDrones, config_.droneMaxCount), 6, 22, 7, tacBright);
    DrawText(TextFormat("HOSTILES  %d", enemyCount), 6, 32, 7, tacBright);

    Vector3 aimPoint = GetFireControlAimPoint();
    float dist = Vector3Distance(camera_.position, aimPoint);
    DrawText(TextFormat("RANGE     %.1f m", dist), 6, 42, 7, tacBright);

    // Mode indicator
    const char* modeLabel = (rocketLauncherMode_ == RocketLauncherMode::Drone) ? "MODE: DRONE" : "MODE: ROCKET";
    DrawText(modeLabel, 6, 54, 7, tacDim);

    // ── Corner decorations ───────────────────────────────────────────
    int cornerLen = 14;
    Color cornerColor = FadeColor(tacGreen, 0.40f);
    // Top-left
    DrawLine(0, 0, cornerLen, 0, cornerColor);
    DrawLine(0, 0, 0, cornerLen, cornerColor);
    // Top-right
    DrawLine(w, 0, w - cornerLen, 0, cornerColor);
    DrawLine(w, 0, w, cornerLen, cornerColor);
    // Bottom-left
    DrawLine(0, h, cornerLen, h, cornerColor);
    DrawLine(0, h, 0, h - cornerLen, cornerColor);
    // Bottom-right
    DrawLine(w, h, w - cornerLen, h, cornerColor);
    DrawLine(w, h, w, h - cornerLen, cornerColor);

    // ── Crosshair ────────────────────────────────────────────────────
    int chLen = 14;
    int chGap = 4;
    DrawLine(cx - chLen, cy, cx - chGap, cy, tacGreen);
    DrawLine(cx + chGap, cy, cx + chLen, cy, tacGreen);
    DrawLine(cx, cy - chLen, cx, cy - chGap, tacGreen);
    DrawLine(cx, cy + chGap, cx, cy + chLen, tacGreen);
    DrawCircle(cx, cy, 1.5f, tacGreen);

    // ── Bottom status bar ────────────────────────────────────────────
    DrawLine(0, h - 17, w, h - 17, FadeColor(tacGreen, 0.25f));

    if (rallyPhase_ != RallyPhase::Inactive) {
        const char* phaseStr = rallyPhase_ == RallyPhase::Assembling ? "RALLY: ASSEMBLING" :
                               rallyPhase_ == RallyPhase::Holding    ? "RALLY: HOLDING" : "RALLY: COMPLETE";
        Color rallyColor = Color{255, 210, 80, 240};
        DrawText(phaseStr, 6, h - 30, 8, rallyColor);
        if (rallyPhase_ == RallyPhase::Holding) {
            DrawText(TextFormat("HOLD  %.1fs", rallyHoldTimer_), 6, h - 14, 7, tacDim);
        } else {
            DrawText("LEFT-CLICK TO REASSIGN", 6, h - 14, 7, tacDim);
        }
    } else {
        DrawText("LEFT-CLICK TO SET RALLY POINT", 6, h - 14, 7, tacDim);
    }
}

void Game::DrawParticles() const {
    for (const Particle& particle : particles_) {
        float alpha = particle.maxLife > 0.0f ? particle.life / particle.maxLife : 0.0f;
        DrawCube(particle.position, particle.size, particle.size, particle.size, FadeColor(particle.color, alpha));
    }
}

void Game::DrawWeapon() const {
    WeaponVisualMode mode = WeaponVisualMode::Laser;
    if (chargingLaser_) {
        mode = WeaponVisualMode::LaserCharge;
    } else if (activeWeapon_ == WeaponType::Flamethrower) {
        mode = WeaponVisualMode::Flamethrower;
    } else if (activeWeapon_ == WeaponType::RocketLauncher) {
        mode = WeaponVisualMode::RocketLauncher;
    } else if (activeWeapon_ == WeaponType::Shotgun) {
        mode = WeaponVisualMode::Shotgun;
    } else if (activeWeapon_ == WeaponType::GravityNailer) {
        mode = WeaponVisualMode::GravityNailer;
    } else if (activeWeapon_ == WeaponType::InfinityGauntlet) {
        mode = WeaponVisualMode::InfinityGauntlet;
    } else if (activeWeapon_ == WeaponType::RecoilLance) {
        mode = WeaponVisualMode::RecoilLance;
    } else if (activeWeapon_ == WeaponType::RiftCutter) {
        mode = WeaponVisualMode::RiftCutter;
    }
    weaponViewModel_.Draw(camera_, mode, laserCharge_);
}

void Game::DrawCrosshair() const {
    int screenWidth = pixelWidth_;
    int screenHeight = pixelHeight_;
    int centerX = screenWidth / 2;
    int centerY = screenHeight / 2;
    DrawLine(centerX - 5, centerY, centerX - 2, centerY, Color{230, 230, 230, 220});
    DrawLine(centerX + 2, centerY, centerX + 5, centerY, Color{230, 230, 230, 220});
    DrawLine(centerX, centerY - 5, centerX, centerY - 2, Color{230, 230, 230, 220});
    DrawLine(centerX, centerY + 2, centerX, centerY + 5, Color{230, 230, 230, 220});

    if (chargingLaser_) {
        int chargeWidth = static_cast<int>(48.0f * laserCharge_);
        Color chargeColor = Color{120, 220, 255, 240};
        DrawCircleLines(centerX, centerY, 7.0f + laserCharge_ * 7.0f, FadeColor(chargeColor, 0.75f));
        DrawCircle(centerX, centerY, 1.0f + laserCharge_ * 2.0f, FadeColor(Color{230, 255, 255, 255}, 0.8f));
        DrawRectangle(screenWidth / 2 - 24, screenHeight - 18, 48, 3, Color{18, 30, 36, 210});
        DrawRectangle(screenWidth / 2 - 24, screenHeight - 18, chargeWidth, 3, chargeColor);
    }
}

void Game::DrawHud() const {
    DrawRectangle(0, 0, pixelWidth_, 22, Color{0, 0, 0, 150});
    DrawText(TextFormat("TIME %.1f", survivalTime_), 6, 7, 8, RAYWHITE);
    DrawText(TextFormat("SCORE %d", score_), 72, 7, 8, RAYWHITE);
    DrawText(TextFormat("E %d", static_cast<int>(enemies_.size())), 134, 7, 8, RAYWHITE);
    const char* modeName = WeaponModeName();
    DrawText(TextFormat("W %s%s%s", WeaponName(), modeName[0] != '\0' ? ":" : "", modeName), 174, 7, 8, RAYWHITE);
    Color gravityColor = spaceSuitEnabled_ ? Color{120, 220, 255, 255} : hasSpaceSuit_ ? Color{110, 125, 135, 255} : RAYWHITE;
    DrawText(TextFormat("G %.2fx", gravityScale_), 250, 7, 8, gravityColor);
    const bool hasInactiveGear = (hasFlightRig_ && !flightRigEnabled_) || (hasSkates_ && !skatesEnabled_) || (hasSpaceSuit_ && !spaceSuitEnabled_);
    const char* stateText = timeStopped_ ? "STOP" : config_.invincible ? "GOD" : chargingLaser_ ? "LASER" : flightRigEnabled_ ? "FLIGHT" : skatesEnabled_ ? "SKATE" : spaceSuitEnabled_ ? "SUIT" : hasInactiveGear ? "GEAR" : grounded_ ? "GROUND" : "AIR";
    Color stateColor = timeStopped_ ? Color{190, 160, 255, 255} : config_.invincible ? Color{255, 230, 120, 255} : chargingLaser_ ? Color{120, 220, 255, 255} : flightRigEnabled_ ? Color{160, 245, 255, 255} : skatesEnabled_ ? Color{165, 255, 185, 255} : spaceSuitEnabled_ ? Color{120, 220, 255, 255} : hasInactiveGear ? Color{145, 150, 155, 255} : grounded_ ? Color{190, 255, 190, 255} : Color{180, 220, 255, 255};
    DrawText(stateText, 316, 7, 8, stateColor);
    DrawText(eventTextTimer_ > 0.0f ? eventText_ : WaveLabel(), 6, 29, 8, eventTextTimer_ > 0.0f ? Color{255, 220, 135, 255} : Color{180, 180, 180, 255});
    if (DuelMode() && state_ == State::Playing) {
        Color armorColor = duelArmorInvulnTimer_ > 0.0f ? Color{255, 230, 140, 255} : duelArmor_ > 0 ? Color{160, 220, 255, 255} : Color{255, 105, 95, 255};
        DrawText(TextFormat("ARMOR %d", duelArmor_), 6, 39, 8, armorColor);
    }
    if (activeWeapon_ == WeaponType::InfinityGauntlet && gauntletMode_ == GauntletMode::Blink) {
        const char* blinkText = TextFormat("BLINK %.1fm", config_.blinkDistance * blinkDistanceScale_);
        DrawText(blinkText, 100, 117, 8, Color{190, 160, 255, 255});
    }
    const char* fpsText = TextFormat("FPS %d", GetFPS());
    DrawText(fpsText, pixelWidth_ - MeasureText(fpsText, 8) - 6, 7, 8, Color{170, 230, 170, 255});

    if (bethlehem_.active) {
        float bh = bethlehem_.maxHealth > 0.0f ? std::clamp(bethlehem_.health / bethlehem_.maxHealth, 0.0f, 1.0f) : 0.0f;
        int barX = 76, barY = 29, barW = 270;
        bool hasBossOrDuelist = false;
        for (const Enemy& enemy : enemies_) {
            if (enemy.type == EnemyType::Boss || enemy.type == EnemyType::Duelist) { hasBossOrDuelist = true; break; }
        }
        if (hasBossOrDuelist) barY = 46;
        DrawRectangle(barX, barY, barW, 6, Color{18, 10, 5, 220});
        DrawRectangle(barX, barY, static_cast<int>(barW * bh), 6, Color{255, 190, 60, 255});
        DrawRectangleLines(barX, barY, barW, 6, Color{255, 220, 140, 210});
        DrawText("STAR OF BETHLEHEM", barX, barY + 9, 8, Color{255, 225, 150, 255});
    }

    for (const Enemy& enemy : enemies_) {
        if (enemy.type != EnemyType::Boss && enemy.type != EnemyType::Duelist) {
            continue;
        }
        float health = enemy.maxHealth > 0.0f ? std::clamp(enemy.health / enemy.maxHealth, 0.0f, 1.0f) : 0.0f;
        int barX = 76;
        int barY = 29;
        int barW = 270;
        DrawRectangle(barX, barY, barW, 6, Color{18, 10, 30, 220});
        Color barColor = enemy.type == EnemyType::Duelist ? Color{255, 210, 105, 255} : health < 0.45f ? Color{255, 75, 160, 255} : Color{160, 115, 255, 255};
        DrawRectangle(barX, barY, static_cast<int>(barW * health), 6, barColor);
        DrawRectangleLines(barX, barY, barW, 6, Color{230, 210, 255, 210});
        const char* duelistWeapon = "LASER";
        if (enemy.weaponSlot == 1) {
            duelistWeapon = "FLAME";
        } else if (enemy.weaponSlot == 2) {
            duelistWeapon = "ROCKET";
        } else if (enemy.weaponSlot == 3) {
            duelistWeapon = "SHOT";
        } else if (enemy.weaponSlot == 4) {
            duelistWeapon = "NAIL";
        } else if (enemy.weaponSlot == 5) {
            duelistWeapon = "GAUNT";
        } else if (enemy.weaponSlot == 6) {
            duelistWeapon = "LANCE";
        } else if (enemy.weaponSlot == 7) {
            duelistWeapon = "NANO";
        }
        DrawText(enemy.type == EnemyType::Duelist ? TextFormat("DUELIST %s", duelistWeapon) : "GEOMETRY LORD", barX, barY + 9, 8, enemy.type == EnemyType::Duelist ? Color{255, 230, 150, 255} : Color{230, 210, 255, 255});
        break;
    }

    if (state_ == State::Dead) {
        const char* title = "YOU WERE TAKEN";
        const char* hint = "FREECAM  WASD/SPACE/CTRL  R TO RETURN";
        int titleWidth = MeasureText(title, 20);
        int hintWidth = MeasureText(hint, 9);
        DrawRectangle(0, 0, pixelWidth_, pixelHeight_, Color{80, 0, 0, 90});
        DrawText(title, pixelWidth_ / 2 - titleWidth / 2, pixelHeight_ / 2 - 20, 20, Color{255, 230, 220, 255});
        DrawText(hint, pixelWidth_ / 2 - hintWidth / 2, pixelHeight_ / 2 + 10, 9, Color{220, 220, 220, 255});
    }
}
