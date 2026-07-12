#include "Game.h"
#include "GameMath.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <random>
#include <vector>

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
bool Game::IsEdenMap() const {
    return config_.mapType == "eden";
}
float Game::EdenCombatBoundaryRadius() const {
    return edenForbiddenFruit_.claimed
        ? std::max(config_.edenMapRadius, config_.edenCorruptedFallRadius)
        : config_.edenMapRadius;
}
float Game::EdenHeightAt(float x, float z) const {
    float r2 = x * x + z * z;
    float outer = 1.0f / std::sqrt(config_.edenMapRadius * config_.edenMapRadius + config_.edenHeightEpsilon);
    float local = 1.0f / std::sqrt(r2 + config_.edenHeightEpsilon);
    return config_.edenHeightScale * std::max(0.0f, local - outer);
}
float Game::EdenGroundYAt(Vector3 position) const {
    return EdenHeightAt(position.x, position.z);
}
Vector3 Game::RandomEdenSpawnPoint() const {
    float minR = std::min(config_.edenSpawnMinRadius, config_.edenSpawnMaxRadius);
    float maxR = std::max(minR, config_.edenSpawnMaxRadius);
    float radius = std::sqrt(RandomFloat(minR * minR, maxR * maxR));
    float angle = RandomFloat(0.0f, 2.0f * PI);
    Vector3 p{std::cos(angle) * radius, 0.0f, std::sin(angle) * radius};
    p.y = EdenGroundYAt(p) + playerHeight_;
    return p;
}
Vector3 Game::EdenTreePosition(int index, bool* lifeTree) const {
    if (lifeTree) *lifeTree = index == 0;
    if (index == 0) {
        Vector3 p{config_.edenPlayRadius * 0.18f, 0.0f, -config_.edenPlayRadius * 0.10f};
        p.y = EdenGroundYAt(p);
        return p;
    }
    float seed = static_cast<float>(index) + 40.0f;
    float noise = std::fmod(std::sin(seed * 12.9898f + 78.233f) * 43758.5453f + 43758.5453f, 1.0f);
    float angle = seed * 2.39996323f;
    float radius = config_.edenPlayRadius * (0.18f + noise * 0.74f);
    Vector3 p{std::cos(angle) * radius, 0.0f, std::sin(angle) * radius};
    if (DistanceXZ(p, Vector3Zero()) < 18.0f) {
        p = Vector3Scale(SafeNormalize(p, Vector3{1.0f, 0.0f, 0.0f}), 18.0f);
    }
    Vector3 life = EdenTreePosition(0);
    if (Vector3Distance(p, life) < 10.0f) {
        p = Vector3Add(p, Vector3Scale(SafeNormalize(p, Vector3{1.0f, 0.0f, 0.0f}), 10.0f));
    }
    p.y = EdenGroundYAt(p);
    return p;
}
Vector3 Game::EdenFloatingStonePosition(int index) const {
    float fruitY = EdenForbiddenFruitPosition().y;
    if (index == 0) {
        return Vector3{0.0f, fruitY + 34.0f, 0.0f};
    }
    float seed = static_cast<float>(index) + 71.0f;
    float noise = std::fmod(std::sin(seed * 12.9898f + 78.233f) * 43758.5453f + 43758.5453f, 1.0f);
    int ring = 1 + (index - 1) / 12;
    int slot = (index - 1) % 12;
    float angle = static_cast<float>(slot) / 12.0f * 2.0f * PI + static_cast<float>(ring) * 0.34f + noise * 0.16f;
    float ringT = static_cast<float>(ring) / 3.0f;
    float baseRadius = config_.edenMapRadius * (0.12f + ringT * 0.74f);
    float spacingJitter = config_.edenMapRadius * (0.035f + ringT * 0.025f);
    float radius = std::clamp(baseRadius + (noise - 0.5f) * spacingJitter,
        config_.edenMapRadius * 0.18f,
        config_.edenMapRadius * 0.93f);
    Vector3 p{std::cos(angle) * radius, 0.0f, std::sin(angle) * radius};
    float altNoise = std::fmod(std::sin((seed + 1.0f) * 12.9898f + 78.233f) * 43758.5453f + 43758.5453f, 1.0f);
    p.y = fruitY + 30.0f + static_cast<float>(ring) * 12.0f + altNoise * 11.0f;
    return p;
}
float Game::EdenExitFade() const {
    if (!IsEdenMap()) return 0.0f;
    float r = std::sqrt(camera_.position.x * camera_.position.x + camera_.position.z * camera_.position.z);
    float denom = std::max(0.001f, config_.edenMapRadius - config_.edenPlayRadius);
    float t = std::clamp((r - config_.edenPlayRadius) / denom, 0.0f, 1.0f);
    t = t * t * (3.0f - 2.0f * t);
    return std::pow(t, config_.edenExitFadePower);
}
int Game::EdenRiverAt(Vector3 position) const {
    if (!IsEdenMap() || edenForbiddenFruit_.claimed) return -1;
    float radius = DistanceXZ(position, Vector3Zero());
    if (radius < 4.0f || radius > config_.edenPlayRadius * 0.985f) return -1;

    auto angleDelta = [](float a, float b) {
        float d = std::fmod(a - b + PI, 2.0f * PI);
        if (d < 0.0f) d += 2.0f * PI;
        return d - PI;
    };

    float angle = std::atan2(position.z, position.x);
    float denom = std::max(1.0f, config_.edenPlayRadius - 7.0f);
    float t = std::clamp((radius - 3.5f) / denom, 0.0f, 1.0f);
    for (int river = 0; river < 4; ++river) {
        float baseAngle = static_cast<float>(river) * PI * 0.5f + PI * 0.25f;
        float bend = std::sin(t * PI * 2.0f + static_cast<float>(river) * 0.7f) * 0.08f;
        float riverAngle = baseAngle + bend;
        float lateral = std::abs(std::sin(angleDelta(angle, riverAngle)) * radius);
        float width = 2.8f + t * 4.2f;
        if (lateral <= width) return river;
    }
    return -1;
}
void Game::UpdateEdenRiverBlessings(float dt) {
    edenRiverBlessingTimer_ = std::max(0.0f, edenRiverBlessingTimer_ - dt);
    if (edenRiverBlessingTimer_ <= 0.0f) {
        edenRiverBlessing_ = -1;
    }
    if (!IsEdenMap() || edenForbiddenFruit_.claimed || state_ != State::Playing) {
        return;
    }

    int river = EdenRiverAt(camera_.position);
    if (river < 0) return;

    static constexpr const char* kRiverNames[4] = {
        "RIVER OF LIFE",
        "RIVER OF LIGHTNESS",
        "RIVER OF HASTE",
        "RIVER OF CLARITY"
    };
    if (edenRiverBlessing_ != river || edenRiverBlessingTimer_ <= 0.05f) {
        eventText_ = kRiverNames[river];
        eventTextTimer_ = 0.9f;
    }
    edenRiverBlessing_ = river;
    edenRiverBlessingTimer_ = 0.45f;

    if (river == 0) {
        edenRiverEssenceTimer_ -= dt;
        if (edenRiverEssenceTimer_ <= 0.0f) {
            ++essence_;
            edenRiverEssenceTimer_ = 5.0f;
            PlaySfx(sfxEssence_);
            SpawnHitBurst(camera_.position, Color{255, 236, 148, 255}, 18);
        }
    } else {
        edenRiverEssenceTimer_ = std::min(edenRiverEssenceTimer_, 1.0f);
    }

    if (river == 1) {
        playerAntigravityTimer_ = std::max(playerAntigravityTimer_, 0.5f);
    } else if (river == 2) {
        Vector3 outward{camera_.position.x, 0.0f, camera_.position.z};
        outward = SafeNormalize(outward, PlayerForward());
        playerVelocity_ = Vector3Add(playerVelocity_, Vector3Scale(outward, 8.0f * dt));
    }
}
Vector3 Game::EdenForbiddenFruitPosition() const {
    return Vector3{0.0f, EdenHeightAt(0.0f, 0.0f) + config_.edenForbiddenFruitHeightOffset, 0.0f};
}
void Game::ResetEdenForbiddenFruit() {
    edenForbiddenFruit_ = {};
    edenForbiddenFruit_.active = IsEdenMap() && config_.edenForbiddenFruitEnabled;
    edenForbiddenFruit_.position = EdenForbiddenFruitPosition();
    ResetEdenArk();
}
void Game::ResetEdenArk() {
    physics_.DestroyBody(edenArk_.body);
    edenArk_ = {};
    if (!IsEdenMap() && !config_.startWithArk) return;
    float angle = PI * 0.25f;
    float radius = IsEdenMap() ? config_.edenPlayRadius * 0.46f : std::min(arenaRadius_ * 0.42f, 30.0f);
    Vector3 radial{std::cos(angle), 0.0f, std::sin(angle)};
    Vector3 tangent{-radial.z, 0.0f, radial.x};
    edenArk_.active = true;
    edenArk_.position = Vector3Add(Vector3Scale(radial, radius), Vector3Scale(tangent, 7.0f));
    if (IsEdenMap()) {
        edenArk_.position.y = EdenGroundYAt(edenArk_.position) + 1.15f;
    } else if (IsSphericalMap()) {
        edenArk_.position = SphericalSurfacePoint(edenArk_.position, 1.15f, 0);
    } else {
        edenArk_.position.y = FlatGroundYForWorld(0) + 1.15f;
    }
    edenArk_.heading = angle;
    edenArk_.forward = SafeNormalize(ProjectOnSphericalTangent(radial, UpForWorldAt(edenArk_.position, 0)), radial);
    PhysicsWorld::BodyConfig arkBodyConfig;
    arkBodyConfig.motionType = JPH::EMotionType::Static;
    arkBodyConfig.layer = Layers::NON_MOVING;
    arkBodyConfig.friction = 0.55f;
    arkBodyConfig.allowSleeping = false;
    edenArk_.body = physics_.CreateBody(
        edenArkShape_, ToJoltVector(edenArk_.position), JPH::Quat::sIdentity(),
        arkBodyConfig, JPH::EActivation::DontActivate);
    UpdateEdenArkBody();
}
bool Game::EdenArkEnterAvailable() const {
    if (!edenArk_.active || edenArk_.piloted) return false;
    Vector3 up = UpForWorldAt(edenArk_.position, 0);
    Vector3 rawForward{std::cos(edenArk_.heading), 0.0f, std::sin(edenArk_.heading)};
    Vector3 forward = IsSphericalMap()
        ? SafeNormalize(ProjectOnSphericalTangent(edenArk_.forward, up),
            SafeNormalize(ProjectOnSphericalTangent(rawForward, up), Vector3{0.0f, 0.0f, 1.0f}))
        : rawForward;
    Vector3 right = SafeNormalize(Vector3CrossProduct(forward, up), Vector3{1.0f, 0.0f, 0.0f});
    Vector3 center = Vector3Add(edenArk_.position, Vector3Scale(up, 4.9f));
    Vector3 offset = Vector3Subtract(camera_.position, center);
    float localX = Vector3DotProduct(offset, right);
    float localY = Vector3DotProduct(offset, up);
    float localZ = Vector3DotProduct(offset, forward);
    constexpr float kInteractPad = 3.2f;
    return std::abs(localX) <= 8.2f + kInteractPad
        && std::abs(localZ) <= 17.2f + kInteractPad
        && localY >= -4.9f - 1.0f
        && localY <= 4.9f + playerHeight_ + 2.2f;
}
void Game::EnterEdenArk() {
    if (!EdenArkEnterAvailable()) return;
    edenArk_.piloted = true;
    edenArk_.interactCooldown = 0.2f;
    edenArk_.forward = SafeNormalize(ProjectOnSphericalTangent(edenArk_.forward, UpForWorldAt(edenArk_.position, 0)),
        Vector3{std::cos(edenArk_.heading), 0.0f, std::sin(edenArk_.heading)});
    edenArk_.cameraYawOffset = PI;
    edenArk_.cameraPitch = -0.68f;
    config_.startWithArk = true;
    playerWorld_ = 0;
    playerVelocity_ = {};
    eventText_ = "NOAH'S ARK";
    eventTextTimer_ = 1.6f;
}
void Game::ExitEdenArk() {
    if (!edenArk_.piloted) return;
    edenArk_.piloted = false;
    edenArk_.interactCooldown = 0.2f;
    StopSfx(sfxArkFloodCurrent_);
    StopSfx(sfxArkFloodSurge_);
    Vector3 up = UpForWorldAt(edenArk_.position, 0);
    Vector3 fallbackForward{std::cos(edenArk_.heading), 0.0f, std::sin(edenArk_.heading)};
    Vector3 forward = SafeNormalize(ProjectOnSphericalTangent(edenArk_.forward, up),
        SafeNormalize(ProjectOnSphericalTangent(fallbackForward, up), fallbackForward));
    Vector3 right = SafeNormalize(Vector3CrossProduct(forward, up), Vector3{1.0f, 0.0f, 0.0f});
    camera_.position = Vector3Add(edenArk_.position,
        Vector3Add(Vector3Scale(right, 8.4f), Vector3Scale(up, playerHeight_ + 0.9f)));
    if (IsEdenMap()) {
        camera_.position.y = std::max(camera_.position.y, EdenGroundYAt(camera_.position) + playerHeight_);
    } else if (IsSphericalMap()) {
        camera_.position = SphericalSurfacePoint(camera_.position, SphericalPlayerAltitude(), 0);
    } else {
        camera_.position.y = std::max(camera_.position.y, FlatGroundYForWorld(0) + playerHeight_);
    }
    camera_.up = UpForWorldAt(camera_.position, playerWorld_);
    camera_.target = Vector3Add(camera_.position, PlayerForward());
    playerVelocity_ = Vector3Scale(forward, edenArk_.speed * 0.25f);
    grounded_ = false;
}
void Game::UpdateEdenArk(float dt) {
    if (!edenArk_.active) return;
    edenArk_.interactCooldown = std::max(0.0f, edenArk_.interactCooldown - dt);
    edenArk_.wakeTimer += dt;

    Vector2 mouseDelta = GetMouseDelta();
    edenArk_.cameraYawOffset -= mouseDelta.x * kMouseSensitivity * kDegToRad;
    edenArk_.cameraPitch -= mouseDelta.y * kMouseSensitivity * kDegToRad;
    edenArk_.cameraPitch = std::clamp(edenArk_.cameraPitch, -1.42f, 1.42f);

    float turn = 0.0f;
    if (IsKeyDown(KEY_A)) turn -= 1.0f;
    if (IsKeyDown(KEY_D)) turn += 1.0f;
    float speedInput = 0.0f;
    if (IsKeyDown(KEY_W)) speedInput += 1.0f;
    if (IsKeyDown(KEY_S)) speedInput -= 1.0f;

    Vector3 localUp = UpForWorldAt(edenArk_.position, 0);
    Vector3 forward = {};
    if (IsSphericalMap()) {
        Vector3 fallbackForward{std::cos(edenArk_.heading), 0.0f, std::sin(edenArk_.heading)};
        forward = SafeNormalize(ProjectOnSphericalTangent(edenArk_.forward, localUp),
            SafeNormalize(ProjectOnSphericalTangent(fallbackForward, localUp), Vector3{0.0f, 0.0f, 1.0f}));
        if (turn != 0.0f) {
            forward = SafeNormalize(RotateAroundAxis(forward, localUp, -turn * 1.25f * dt), forward);
        }
        edenArk_.forward = forward;
        Vector3 flatForward{forward.x, 0.0f, forward.z};
        if (Vector3Length(flatForward) > 0.001f) {
            edenArk_.heading = std::atan2(flatForward.z, flatForward.x);
        }
    } else {
        edenArk_.heading += turn * 1.25f * dt;
        forward = Vector3{std::cos(edenArk_.heading), 0.0f, std::sin(edenArk_.heading)};
        edenArk_.forward = forward;
    }
    bool boosting = (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) && std::abs(speedInput) > 0.01f;
    float arkMoveSpeed = 16.0f;
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
        arkMoveSpeed *= config_.arkShiftSpeedMult;
    }
    float targetSpeed = speedInput * arkMoveSpeed;
    edenArk_.speed += (targetSpeed - edenArk_.speed) * std::clamp(2.6f * dt, 0.0f, 1.0f);
    float speedT = std::clamp(std::abs(edenArk_.speed) / std::max(1.0f, 16.0f * config_.arkShiftSpeedMult), 0.0f, 1.0f);
    if (speedT > 0.08f) {
        UpdateLoopingSfxAt(sfxArkFloodCurrent_, edenArk_.position, 130.0f, 0.42f + speedT * 0.68f);
        float surgeT = boosting ? std::clamp((speedT - 0.25f) / 0.75f, 0.0f, 1.0f) : 0.0f;
        if (surgeT > 0.01f) {
            UpdateLoopingSfxAt(sfxArkFloodSurge_, edenArk_.position, 150.0f, 0.18f + surgeT * 0.82f);
        } else {
            StopSfx(sfxArkFloodSurge_);
        }
    } else {
        StopSfx(sfxArkFloodCurrent_);
        StopSfx(sfxArkFloodSurge_);
    }
    edenArk_.position = Vector3Add(edenArk_.position, Vector3Scale(forward, edenArk_.speed * dt));
    if (IsEdenMap()) {
        edenArk_.position.y = EdenGroundYAt(edenArk_.position) + 1.15f;
    } else if (IsSphericalMap()) {
        edenArk_.position = SphericalSurfacePoint(edenArk_.position, 1.15f, 0);
        Vector3 newUp = UpForWorldAt(edenArk_.position, 0);
        edenArk_.forward = SafeNormalize(ProjectOnSphericalTangent(forward, newUp),
            SafeNormalize(ProjectOnSphericalTangent(edenArk_.forward, newUp), forward));
        forward = edenArk_.forward;
    } else {
        edenArk_.position.y = FlatGroundYForWorld(0) + 1.15f;
    }
    UpdateEdenArkBody();

    Vector3 right = IsSphericalMap()
        ? SafeNormalize(Vector3CrossProduct(forward, UpForWorldAt(edenArk_.position, 0)), Vector3{1.0f, 0.0f, 0.0f})
        : Vector3{-forward.z, 0.0f, forward.x};
    Vector3 arkUp = UpForWorldAt(edenArk_.position, 0);
    if (speedT > 0.05f) {
        Vector3 travelForward = edenArk_.speed >= 0.0f ? forward : Vector3Scale(forward, -1.0f);
        float expected = dt * (34.0f + speedT * 78.0f + (boosting ? 72.0f : 0.0f));
        int particleCount = static_cast<int>(expected);
        if (RandomFloat(0.0f, 1.0f) < expected - static_cast<float>(particleCount)) {
            particleCount++;
        }
        particleCount = std::min(particleCount, boosting ? 18 : 12);
        Color foam = IsEdenMap() && edenForbiddenFruit_.claimed
            ? Color{210, 210, 202, 215}
            : Color{188, 236, 255, 215};
        Color brightFoam = IsEdenMap() && edenForbiddenFruit_.claimed
            ? Color{238, 226, 206, 230}
            : Color{232, 252, 255, 230};
        for (int i = 0; i < particleCount; ++i) {
            float sideSign = RandomFloat(0.0f, 1.0f) < 0.5f ? -1.0f : 1.0f;
            float sideOffset = RandomFloat(6.7f, 9.0f) * sideSign;
            float lengthOffset = RandomFloat(-11.5f, 13.8f);
            if (RandomFloat(0.0f, 1.0f) < 0.34f + speedT * 0.32f) {
                lengthOffset = RandomFloat(-19.0f, -8.5f);
            }
            Vector3 spawn = Vector3Add(edenArk_.position,
                Vector3Add(Vector3Scale(right, sideOffset),
                    Vector3Add(Vector3Scale(travelForward, lengthOffset),
                        Vector3Scale(arkUp, RandomFloat(0.05f, 0.9f)))));
            Vector3 outward = SafeNormalize(Vector3Add(Vector3Scale(right, sideSign * RandomFloat(0.6f, 1.2f)),
                                                       Vector3Scale(travelForward, RandomFloat(-0.7f, -0.15f))),
                                            Vector3Scale(right, sideSign));
            Vector3 velocity = Vector3Add(Vector3Scale(outward, RandomFloat(2.0f, 5.5f + speedT * 5.0f)),
                                          Vector3Scale(arkUp, RandomFloat(2.6f, 7.2f + speedT * 5.2f)));
            velocity = Vector3Add(velocity, Vector3Scale(travelForward, -std::abs(edenArk_.speed) * RandomFloat(0.08f, 0.22f)));
            particles_.push_back(Particle{
                spawn,
                velocity,
                RandomFloat(0.0f, 1.0f) < 0.36f ? brightFoam : foam,
                RandomFloat(0.32f, 0.72f),
                RandomFloat(0.32f, 0.72f),
                RandomFloat(0.08f, 0.22f + speedT * 0.13f)
            });
        }
    }
    Vector3 crushCenter = Vector3Add(edenArk_.position, Vector3Scale(forward, 10.5f));
    float crushLength = 12.0f;
    float crushWidth = 7.6f;
    if (std::abs(edenArk_.speed) > 2.0f) {
        for (size_t i = 0; i < enemies_.size();) {
            Vector3 pos = BodyPosition(enemies_[i].body);
            Vector3 rel = Vector3Subtract(pos, crushCenter);
            float localF = Vector3DotProduct(rel, forward);
            float localR = Vector3DotProduct(rel, right);
            if (enemies_[i].world == playerWorld_
                && std::abs(localF) <= crushLength
                && std::abs(localR) <= crushWidth + enemies_[i].radius
                && std::abs(pos.y - edenArk_.position.y) <= 7.0f) {
                SpawnHitBurst(pos, Color{255, 214, 132, 255}, 36);
                DestroyEnemy(i);
                cameraShake_ = std::min(1.0f, cameraShake_ + 0.18f);
                continue;
            }
            ++i;
        }
        for (SeraphBoss& seraph : seraphs_) {
            if (!seraph.active || !seraph.edenApocalypse) continue;
            Vector3 rel = Vector3Subtract(seraph.position, crushCenter);
            float localF = Vector3DotProduct(rel, forward);
            float localR = Vector3DotProduct(rel, right);
            if (std::abs(localF) <= crushLength + 2.5f
                && std::abs(localR) <= crushWidth + 2.5f
                && seraph.position.y <= edenArk_.position.y + 8.0f) {
                DamageSeraph(120.0f, seraph.position, Color{255, 214, 132, 255});
                cameraShake_ = std::min(1.0f, cameraShake_ + 0.22f);
            }
        }
    }

    Vector3 cameraUp = UpForWorldAt(edenArk_.position, 0);
    camera_.position = Vector3Add(edenArk_.position,
        Vector3Add(Vector3Scale(forward, -14.0f), Vector3Scale(cameraUp, 17.5f)));
    Vector3 baseView = SafeNormalize(Vector3Subtract(
        Vector3Add(edenArk_.position, Vector3Scale(cameraUp, 5.0f)),
        camera_.position), forward);
    Vector3 viewDirection = SafeNormalize(RotateAroundAxis(baseView, cameraUp, edenArk_.cameraYawOffset - PI), baseView);
    Vector3 viewRight = SafeNormalize(Vector3CrossProduct(viewDirection, cameraUp), right);
    viewDirection = SafeNormalize(RotateAroundAxis(viewDirection, viewRight, edenArk_.cameraPitch + 0.68f), viewDirection);
    Vector3 flatView = SafeNormalize(ProjectOnSphericalTangent(viewDirection, cameraUp), forward);
    yaw_ = std::atan2(flatView.z, flatView.x) / kDegToRad;
    pitch_ = std::asin(std::clamp(Vector3DotProduct(viewDirection, cameraUp), -1.0f, 1.0f)) / kDegToRad;
    camera_.up = cameraUp;
    camera_.target = Vector3Add(camera_.position, viewDirection);
    playerVelocity_ = Vector3Scale(forward, edenArk_.speed);
}
void Game::UpdateEdenArkBody() {
    if (!edenArk_.active || edenArk_.body.IsInvalid()) return;
    Vector3 up = UpForWorldAt(edenArk_.position, 0);
    Vector3 rawForward{std::cos(edenArk_.heading), 0.0f, std::sin(edenArk_.heading)};
    Vector3 forward = IsSphericalMap()
        ? SafeNormalize(ProjectOnSphericalTangent(edenArk_.forward, up),
            SafeNormalize(ProjectOnSphericalTangent(rawForward, up), Vector3{0.0f, 0.0f, 1.0f}))
        : rawForward;
    Vector3 right = SafeNormalize(Vector3CrossProduct(forward, up), Vector3{1.0f, 0.0f, 0.0f});
    Vector3 center = Vector3Add(edenArk_.position, Vector3Scale(up, 4.9f));
    Matrix rotMatrix = MatrixIdentity();
    rotMatrix.m0 = right.x;   rotMatrix.m4 = up.x;   rotMatrix.m8 = -forward.x;
    rotMatrix.m1 = right.y;   rotMatrix.m5 = up.y;   rotMatrix.m9 = -forward.y;
    rotMatrix.m2 = right.z;   rotMatrix.m6 = up.z;   rotMatrix.m10 = -forward.z;
    Quaternion rq = QuaternionFromMatrix(rotMatrix);
    JPH::Quat rotation(rq.x, rq.y, rq.z, rq.w);
    physics_.Bodies().SetPositionAndRotation(edenArk_.body, ToJoltVector(center), rotation, JPH::EActivation::DontActivate);
}
bool Game::EdenForbiddenFruitInteractAvailable() const {
    if (!IsEdenMap() || !edenForbiddenFruit_.active || edenForbiddenFruit_.claimed) return false;
    return Vector3Distance(camera_.position, edenForbiddenFruit_.position)
        <= config_.edenForbiddenFruitInteractRange + config_.edenForbiddenFruitRadius;
}
void Game::ClaimEdenForbiddenFruit() {
    if (!EdenForbiddenFruitInteractAvailable()) return;
    int gained = std::max(0, edenForbiddenFruit_.absorbedEssence);
    essence_ += gained;
    edenForbiddenFruit_.absorbedEssence = 0;
    edenForbiddenFruit_.claimed = true;
    config_.heavenFalls = true;
    cherubs_.clear();
    SpawnEdenApocalypseSeraphs();
    ResetEdenGuardians();
    float shockwaveRadius = config_.edenMapRadius;
    for (size_t i = 0; i < pickups_.size();) {
        if (pickups_[i].type == PickupType::Essence
            && Vector3Distance(pickups_[i].position, edenForbiddenFruit_.position) <= shockwaveRadius) {
            SpawnHitBurst(pickups_[i].position, Color{255, 112, 64, 255}, 10);
            pickups_[i] = pickups_.back();
            pickups_.pop_back();
            continue;
        }
        ++i;
    }
    mysticStaffShieldActive_ = false;
    mysticStaffChanneling_ = false;
    mysticStaffChannelProgress_ = 0.0f;
    StopSfx(sfxMysticCircleChannel_);
    PlaySfx(sfxEssence_);
    Vector3 outward{camera_.position.x, 0.0f, camera_.position.z};
    if (Vector3Length(outward) <= 0.001f) {
        Vector3 forward = PlayerForward();
        outward = Vector3{forward.x, 0.0f, forward.z};
    }
    outward = SafeNormalize(outward, Vector3{1.0f, 0.0f, 0.0f});
    playerVelocity_ = Vector3Add(playerVelocity_, Vector3Scale(outward, config_.edenForbiddenFruitExileImpulse));
    SpawnHitBurst(edenForbiddenFruit_.position, Color{255, 58, 42, 255}, 180);
    SpawnShockwave(edenForbiddenFruit_.position, shockwaveRadius, Color{255, 64, 35, 255});
    SpawnShockwave(edenForbiddenFruit_.position, shockwaveRadius * 0.62f, Color{255, 175, 72, 255});
    SpawnShockwave(edenForbiddenFruit_.position, shockwaveRadius * 0.34f, Color{255, 226, 120, 255});
    cameraShake_ = 1.0f;
    eventText_ = "FORBIDDEN FRUIT";
    eventTextTimer_ = 2.5f;
}
void Game::EnterEdenFromGate() {
    EdenReturnState saved;
    saved.valid = true;
    saved.gameMode = config_.gameMode;
    saved.mapType = config_.mapType;
    saved.position = camera_.position;
    saved.yaw = yaw_;
    saved.pitch = pitch_;
    saved.essence = essence_;
    saved.survivalTime = survivalTime_;

    ClearWorld();
    edenReturn_ = saved;
    config_.mapType = "eden";
    playerWorld_ = 0;
    camera_.position = RandomEdenSpawnPoint();
    yaw_ = RandomFloat(-180.0f, 180.0f);
    pitch_ = 0.0f;
    playerVelocity_ = {};
    grounded_ = true;
    camera_.up = Vector3{0.0f, 1.0f, 0.0f};
    camera_.target = Vector3Add(camera_.position, PlayerForward());
    essence_ = saved.essence;
    survivalTime_ = saved.survivalTime;
    edenExitFade_ = 0.0f;
    eventText_ = "EDEN";
    eventTextTimer_ = 3.0f;
    BuildMap();
    ResetEdenForbiddenFruit();
}
void Game::ExitEden(bool forceSurvivalMode) {
    int preservedEssence = essence_;
    if (edenReturn_.valid) {
        std::string mode = forceSurvivalMode ? "survival" : edenReturn_.gameMode;
        std::string map = edenReturn_.mapType;
        Vector3 position = edenReturn_.position;
        float yaw = edenReturn_.yaw;
        float pitch = edenReturn_.pitch;
        float savedTime = edenReturn_.survivalTime;
        edenReturn_ = {};
        config_.gameMode = mode;
        config_.mapType = map;
        Reset();
        essence_ = preservedEssence;
        survivalTime_ = savedTime;
        playerWorld_ = 0;
        camera_.position = position;
        yaw_ = yaw;
        pitch_ = pitch;
        if (IsSphericalMap()) {
            camera_.position = SphericalSurfacePoint(camera_.position, SphericalPlayerAltitude(), 0);
        } else if (IsLabyrinthMap()) {
            float limitX = static_cast<float>(std::max(1, labyrinthWidth_ - 1)) * config_.labyrinthCellSize * 0.5f - playerRadius_;
            float limitZ = static_cast<float>(std::max(1, labyrinthHeight_ - 1)) * config_.labyrinthCellSize * 0.5f - playerRadius_;
            camera_.position.x = std::clamp(camera_.position.x, -limitX, limitX);
            camera_.position.z = std::clamp(camera_.position.z, -limitZ, limitZ);
            camera_.position.y = playerHeight_;
            ResolveLabyrinthPlayerOverlap();
        } else if (IsSquareMap()) {
            float limit = squareHalfExtent_ - playerRadius_;
            camera_.position.x = std::clamp(camera_.position.x, -limit, limit);
            camera_.position.z = std::clamp(camera_.position.z, -limit, limit);
            camera_.position.y = FlatGroundYForWorld(0) + playerHeight_;
        } else {
            float r = DistanceXZ(camera_.position, Vector3Zero());
            float limit = arenaRadius_ - playerRadius_;
            if (r > limit && r > 0.001f) {
                float scale = limit / r;
                camera_.position.x *= scale;
                camera_.position.z *= scale;
            }
            camera_.position.y = FlatGroundYForWorld(0) + playerHeight_;
        }
    } else {
        static const std::vector<std::string> modes = {"survival", "duel", "tutorial"};
        static const std::vector<std::string> maps = {"circle", "square_obstacle", "asteroid", "hollow_world"};
        config_.gameMode = forceSurvivalMode ? "survival" : modes[GetRandomValue(0, static_cast<int>(modes.size()) - 1)];
        config_.mapType = maps[GetRandomValue(0, static_cast<int>(maps.size()) - 1)];
        Reset();
        essence_ = preservedEssence;
    }
    edenExitFade_ = 0.0f;
    edenFallOutTimer_ = 0.0f;
    playerVelocity_ = {};
    camera_.up = UpForWorldAt(camera_.position, playerWorld_);
    camera_.target = Vector3Add(camera_.position, PlayerForward());
    eventText_ = "LEFT EDEN";
    eventTextTimer_ = 2.5f;
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
    PlaySfxAt(sfxWormholeOpen_, octaCenter, 96.0f, 1.0f);
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

    PlaySfxAt(sfxWormholeClose_, position, 96.0f, 1.0f);
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
        PlaySfxAt(sfxWormholeTravel_, camera_.position, 12.0f, 1.0f);
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
    const bool faminePressure = FaminePressureActive();
    for (size_t i = 0; i < pickups_.size();) {
        Pickup& pickup = pickups_[i];
        pickup.bobTimer += dt;
        if (faminePressure && pickup.type == PickupType::Essence && pickup.maxLife > 0.0f) {
            float ageDelta = dt;
            if (famineRider_.active) {
                float auraRadius = FamineWitherRadius();
                if (Vector3Distance(pickup.position, famineRider_.position) <= auraRadius) {
                    ageDelta += dt * config_.famineRiderAuraWitherRate;
                }
            }
            pickup.age += ageDelta;
            if (pickup.age >= pickup.maxLife) {
                SpawnHitBurst(pickup.position, Color{70, 62, 50, 255}, 8);
                FeedFamineWitheredEssence();
                pickups_[i] = pickups_.back();
                pickups_.pop_back();
                continue;
            }
        }

        // Falling essence physics
        if (pickup.gravityScale > 0.0f) {
            if (IsSphericalMap()) {
                // Radial gravity toward surface
                Vector3 up = SphericalUpAt(pickup.position);
                pickup.velocity = Vector3Subtract(pickup.velocity, Vector3Scale(up, config_.gravity * pickup.gravityScale * dt));
                pickup.position = Vector3Add(pickup.position, Vector3Scale(pickup.velocity, dt));
                // Tangent-plane drag
                Vector3 tangentVel = ProjectOnSphericalTangent(pickup.velocity, up);
                float hSpeed = Vector3Length(tangentVel);
                if (hSpeed > 0.1f) {
                    float drag = pickup.horizontalDrag * dt;
                    float newSpeed = std::max(0.0f, hSpeed - drag);
                    float ratio = newSpeed / std::max(0.001f, hSpeed);
                    Vector3 newTangent = Vector3Scale(tangentVel, ratio);
                    Vector3 radialVel = Vector3Scale(up, Vector3DotProduct(pickup.velocity, up));
                    pickup.velocity = Vector3Add(radialVel, newTangent);
                } else {
                    // Stop tangent drift, keep only radial
                    pickup.velocity = Vector3Scale(up, Vector3DotProduct(pickup.velocity, up));
                }
                // Ground stop
                float alt = SphericalAltitudeAt(pickup.position, 0);
                if (alt <= pickup.radius + 0.1f) {
                    pickup.position = SphericalSurfacePoint(pickup.position, pickup.radius + 0.1f, 0);
                    pickup.velocity = Vector3Zero();
                    pickup.gravityScale = 0.0f;
                    pickup.horizontalDrag = 0.0f;
                }
            } else {
                pickup.velocity.y -= config_.gravity * pickup.gravityScale * dt;
                pickup.position.x += pickup.velocity.x * dt;
                pickup.position.y += pickup.velocity.y * dt;
                pickup.position.z += pickup.velocity.z * dt;
                float hSpeed = std::sqrt(pickup.velocity.x * pickup.velocity.x + pickup.velocity.z * pickup.velocity.z);
                if (hSpeed > 0.1f) {
                    float drag = pickup.horizontalDrag * dt;
                    float newSpeed = std::max(0.0f, hSpeed - drag);
                    float ratio = newSpeed / hSpeed;
                    pickup.velocity.x *= ratio;
                    pickup.velocity.z *= ratio;
                } else {
                    pickup.velocity.x = 0.0f;
                    pickup.velocity.z = 0.0f;
                }
                if (pickup.position.y <= pickup.radius) {
                    pickup.position.y = pickup.radius;
                    pickup.velocity = Vector3Zero();
                    pickup.gravityScale = 0.0f;
                    pickup.horizontalDrag = 0.0f;
                }
            }
        }

        bool touched = false;
        if (!UfoPilotActive()) {
            if (IsSphericalMap()) {
                touched = Vector3Distance(player, pickup.position) <= pickup.radius + playerRadius_ + SphericalPlayerAltitude() * 0.55f;
            } else {
                float verticalReach = std::abs(player.y - pickup.position.y);
                touched = DistanceXZ(player, pickup.position) <= pickup.radius + playerRadius_ && verticalReach < 1.9f;
            }
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
                    ShowTutorialTip(T("已拾取太空服!\n按Z键开关低重力 (重力降至0.24x, 可高跳远跃)",
                        "Space Suit collected!\nPress Z to toggle low gravity (0.24x)"));
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
                    ShowTutorialTip(T("已拾取飞行装置!\n按X键开关飞行 (空格升高, Ctrl降低, 悬停瞄准)",
                        "Flight Rig collected!\nPress X to toggle hover (Space/Ctrl to ascend/descend)"));
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
                    ShowTutorialTip(T("已拾取滑板!\n按C键开关滑板 (极低地面摩擦, 保持高动量滑行)",
                        "Skates collected!\nPress C to toggle ultra-low friction sliding"));
                }
            } else if (pickup.type == PickupType::Essence) {
                essence_++;
                PlaySfx(sfxEssence_);
                SpawnHitBurst(pickup.position, Color{255, 215, 60, 255}, 30);
                cameraShake_ = std::min(1.0f, cameraShake_ + 0.2f);
                eventText_ = "ESSENCE +1";
                eventTextTimer_ = 1.4f;
                if (TutorialMode() && !pickupTipShown_[3]) {
                    pickupTipShown_[3] = true;
                    ShowTutorialTip(T("已拾取本质/精华!\n额外生命+1  |  受伤时消耗一条命并短暂无敌\n地图上定时刷新",
                        "Essence collected!\nExtra life +1  |  Lose one on hit with brief invincibility\nRespawns periodically on the map"));
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
        } else if (IsLabyrinthMap()) {
            float limitX = static_cast<float>(std::max(1, labyrinthWidth_ - 1)) * config_.labyrinthCellSize * 0.5f + 2.0f;
            float limitZ = static_cast<float>(std::max(1, labyrinthHeight_ - 1)) * config_.labyrinthCellSize * 0.5f + 2.0f;
            if (std::abs(position.x) > limitX || std::abs(position.z) > limitZ) {
                Vector3 direction = Vector3Normalize(Vector3{-position.x, 0.0f, -position.z});
                physics_.Bodies().SetLinearVelocity(enemy.body, JPH::Vec3(direction.x * enemy.speed, 0.0f, direction.z * enemy.speed));
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
    if (IsSphericalMap() || IsEdenMap()) {
        return;
    }
    if (IsLabyrinthMap()) {
        labyrinthWidth_ = config_.labyrinthWidth;
        labyrinthHeight_ = config_.labyrinthHeight;
        labyrinthRuntimeSeed_ = config_.labyrinthSeed > 0
            ? static_cast<unsigned int>(config_.labyrinthSeed)
            : static_cast<unsigned int>(GetRandomValue(1, 0x3fffffff));
        GenerateLabyrinth(labyrinthRuntimeSeed_, labyrinthGrid_);
        labyrinthPendingGrid_.clear();
        labyrinthShiftTimer_ = config_.labyrinthShiftInterval;
        labyrinthShiftWarning_ = false;
        labyrinthMinotaurSpawned_ = false;
        BuildLabyrinthProps();
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
bool Game::IsLabyrinthMap() const {
    return config_.mapType == "labyrinth";
}
bool Game::LabyrinthCellOpen(int x, int y, const std::vector<unsigned char>* grid) const {
    const std::vector<unsigned char>& source = grid ? *grid : labyrinthGrid_;
    if (x < 0 || y < 0 || x >= labyrinthWidth_ || y >= labyrinthHeight_) return false;
    int index = y * labyrinthWidth_ + x;
    if (index < 0 || index >= static_cast<int>(source.size())) return false;
    return source[index] == 0;
}
Vector3 Game::LabyrinthCellCenter(int x, int y) const {
    float halfX = static_cast<float>(labyrinthWidth_ - 1) * 0.5f;
    float halfY = static_cast<float>(labyrinthHeight_ - 1) * 0.5f;
    return Vector3{
        (static_cast<float>(x) - halfX) * config_.labyrinthCellSize,
        playerHeight_,
        (static_cast<float>(y) - halfY) * config_.labyrinthCellSize
    };
}
Game::LabyrinthCell Game::LabyrinthCellForPosition(Vector3 position) const {
    float halfX = static_cast<float>(labyrinthWidth_ - 1) * 0.5f;
    float halfY = static_cast<float>(labyrinthHeight_ - 1) * 0.5f;
    int x = static_cast<int>(std::lround(position.x / config_.labyrinthCellSize + halfX));
    int y = static_cast<int>(std::lround(position.z / config_.labyrinthCellSize + halfY));
    return LabyrinthCell{std::clamp(x, 0, std::max(0, labyrinthWidth_ - 1)),
                         std::clamp(y, 0, std::max(0, labyrinthHeight_ - 1))};
}
void Game::GenerateLabyrinth(unsigned int seed, std::vector<unsigned char>& out) const {
    int width = labyrinthWidth_ > 0 ? labyrinthWidth_ : config_.labyrinthWidth;
    int height = labyrinthHeight_ > 0 ? labyrinthHeight_ : config_.labyrinthHeight;
    out.assign(static_cast<size_t>(width * height), 1);
    auto idx = [width](int x, int y) { return y * width + x; };
    std::mt19937 rng(seed);
    std::vector<LabyrinthCell> stack;
    stack.push_back(LabyrinthCell{1, 1});
    out[idx(1, 1)] = 0;
    const int dirs[4][2] = {{2, 0}, {-2, 0}, {0, 2}, {0, -2}};
    while (!stack.empty()) {
        LabyrinthCell current = stack.back();
        std::vector<int> order = {0, 1, 2, 3};
        std::shuffle(order.begin(), order.end(), rng);
        bool carved = false;
        for (int d : order) {
            int nx = current.x + dirs[d][0];
            int ny = current.y + dirs[d][1];
            if (nx <= 0 || ny <= 0 || nx >= width - 1 || ny >= height - 1) continue;
            if (out[idx(nx, ny)] == 0) continue;
            out[idx(current.x + dirs[d][0] / 2, current.y + dirs[d][1] / 2)] = 0;
            out[idx(nx, ny)] = 0;
            stack.push_back(LabyrinthCell{nx, ny});
            carved = true;
            break;
        }
        if (!carved) stack.pop_back();
    }
    out[idx(1, 1)] = 0;
    out[idx(width - 2, height - 2)] = 0;
}
void Game::BuildLabyrinthProps() {
    props_.clear();
    if (!IsLabyrinthMap() || labyrinthGrid_.empty()) return;
    float cs = config_.labyrinthCellSize;
    float h = config_.labyrinthWallHeight;
    float t = config_.labyrinthWallThickness;
    Color wallA{72, 62, 54, 255};
    Color wallB{88, 76, 62, 255};
    for (int y = 0; y < labyrinthHeight_; ++y) {
        for (int x = 0; x < labyrinthWidth_; ++x) {
            if (LabyrinthCellOpen(x, y)) continue;
            Vector3 center = LabyrinthCellCenter(x, y);
            center.y = 0.0f;
            bool horizontal = (x & 1) == 1 && (y & 1) == 0;
            bool vertical = (x & 1) == 0 && (y & 1) == 1;
            bool pillar = (x & 1) == 0 && (y & 1) == 0;
            float wallRun = cs * 2.0f + t * 1.5f;
            float pillarRun = t * 1.5f;
            Vector3 scale{
                horizontal ? wallRun : pillar ? pillarRun : t,
                h,
                vertical ? wallRun : pillar ? pillarRun : t
            };
            if (x == 0 || y == 0 || x == labyrinthWidth_ - 1 || y == labyrinthHeight_ - 1) {
                scale.x = (x == 0 || x == labyrinthWidth_ - 1) ? t * 1.5f : wallRun;
                scale.z = (y == 0 || y == labyrinthHeight_ - 1) ? t * 1.5f : wallRun;
            }
            Color color = ((x + y) & 1) ? wallA : wallB;
            props_.push_back(Prop{center, scale, 0.0f, color, 0, true});
        }
    }
}
Game::LabyrinthCell Game::LabyrinthFarthestCellFrom(int sx, int sy) const {
    LabyrinthCell start{sx, sy};
    if (!LabyrinthCellOpen(start.x, start.y)) start = LabyrinthCell{1, 1};
    std::vector<int> dist(static_cast<size_t>(labyrinthWidth_ * labyrinthHeight_), -1);
    auto idx = [&](int x, int y) { return y * labyrinthWidth_ + x; };
    std::queue<LabyrinthCell> q;
    q.push(start);
    dist[idx(start.x, start.y)] = 0;
    LabyrinthCell best = start;
    const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    while (!q.empty()) {
        LabyrinthCell c = q.front();
        q.pop();
        if (dist[idx(c.x, c.y)] > dist[idx(best.x, best.y)]) best = c;
        for (auto& d : dirs) {
            int nx = c.x + d[0], ny = c.y + d[1];
            if (!LabyrinthCellOpen(nx, ny) || dist[idx(nx, ny)] >= 0) continue;
            dist[idx(nx, ny)] = dist[idx(c.x, c.y)] + 1;
            q.push(LabyrinthCell{nx, ny});
        }
    }
    return best;
}
Game::LabyrinthCell Game::LabyrinthNextStepToward(LabyrinthCell from, LabyrinthCell to) const {
    if (!LabyrinthCellOpen(from.x, from.y) || !LabyrinthCellOpen(to.x, to.y)) return from;
    std::vector<int> parent(static_cast<size_t>(labyrinthWidth_ * labyrinthHeight_), -1);
    auto idx = [&](int x, int y) { return y * labyrinthWidth_ + x; };
    std::queue<LabyrinthCell> q;
    q.push(from);
    parent[idx(from.x, from.y)] = idx(from.x, from.y);
    const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    while (!q.empty()) {
        LabyrinthCell c = q.front();
        q.pop();
        if (c.x == to.x && c.y == to.y) break;
        for (auto& d : dirs) {
            int nx = c.x + d[0], ny = c.y + d[1];
            if (!LabyrinthCellOpen(nx, ny) || parent[idx(nx, ny)] >= 0) continue;
            parent[idx(nx, ny)] = idx(c.x, c.y);
            q.push(LabyrinthCell{nx, ny});
        }
    }
    int target = idx(to.x, to.y);
    if (parent[target] < 0) return from;
    int cur = target;
    int start = idx(from.x, from.y);
    while (parent[cur] != start && parent[cur] != cur) {
        cur = parent[cur];
    }
    return LabyrinthCell{cur % labyrinthWidth_, cur / labyrinthWidth_};
}
bool Game::LabyrinthHasLineOfSight(LabyrinthCell a, LabyrinthCell b) const {
    if (a.x == b.x) {
        int step = b.y >= a.y ? 1 : -1;
        for (int y = a.y; y != b.y + step; y += step) {
            if (!LabyrinthCellOpen(a.x, y)) return false;
        }
        return true;
    }
    if (a.y == b.y) {
        int step = b.x >= a.x ? 1 : -1;
        for (int x = a.x; x != b.x + step; x += step) {
            if (!LabyrinthCellOpen(x, a.y)) return false;
        }
        return true;
    }
    return false;
}
void Game::ApplyPendingLabyrinth() {
    if (labyrinthPendingGrid_.empty()) return;
    LabyrinthCell playerCell = LabyrinthCellForPosition(camera_.position);
    for (int y = playerCell.y - 1; y <= playerCell.y + 1; ++y) {
        for (int x = playerCell.x - 1; x <= playerCell.x + 1; ++x) {
            if (x > 0 && y > 0 && x < labyrinthWidth_ - 1 && y < labyrinthHeight_ - 1) {
                labyrinthPendingGrid_[static_cast<size_t>(y * labyrinthWidth_ + x)] = 0;
            }
        }
    }
    labyrinthGrid_ = labyrinthPendingGrid_;
    labyrinthPendingGrid_.clear();
    labyrinthShiftWarning_ = false;
    labyrinthShiftTimer_ = config_.labyrinthShiftInterval;
    BuildLabyrinthProps();
    ResolveLabyrinthPlayerOverlap();
    SpawnShockwave(camera_.position, 4.5f, Color{255, 166, 72, 255});
    eventText_ = "LABYRINTH SHIFT";
    eventTextTimer_ = 2.2f;
}
void Game::ResolveLabyrinthPlayerOverlap() {
    if (!IsLabyrinthMap()) return;
    LabyrinthCell cell = LabyrinthCellForPosition(camera_.position);
    if (LabyrinthCellOpen(cell.x, cell.y)) return;
    LabyrinthCell safe = LabyrinthFarthestCellFrom(1, 1);
    float bestDist = 1.0e30f;
    for (int y = 1; y < labyrinthHeight_ - 1; ++y) {
        for (int x = 1; x < labyrinthWidth_ - 1; ++x) {
            if (!LabyrinthCellOpen(x, y)) continue;
            Vector3 center = LabyrinthCellCenter(x, y);
            float d = Vector3DistanceSqr(center, camera_.position);
            if (d < bestDist) {
                bestDist = d;
                safe = LabyrinthCell{x, y};
            }
        }
    }
    camera_.position = LabyrinthCellCenter(safe.x, safe.y);
    playerVelocity_ = Vector3Zero();
}
void Game::UpdateLabyrinth(float dt) {
    if (!IsLabyrinthMap() || timeStopped_) return;
    labyrinthShiftTimer_ -= dt;
    if (!labyrinthShiftWarning_ && labyrinthShiftTimer_ <= config_.labyrinthShiftWarningTime) {
        labyrinthShiftWarning_ = true;
        unsigned int nextSeed = labyrinthRuntimeSeed_ + 9973u + static_cast<unsigned int>(std::max(0.0f, survivalTime_) * 31.0f);
        GenerateLabyrinth(nextSeed, labyrinthPendingGrid_);
        eventText_ = "LABYRINTH SHIFTING";
        eventTextTimer_ = config_.labyrinthShiftWarningTime;
    }
    if (labyrinthShiftTimer_ <= 0.0f) {
        ApplyPendingLabyrinth();
    }
}
bool Game::ResolveEdenArkCollision(Vector3 previousPosition) {
    if (!edenArk_.active || edenArk_.piloted || playerWorld_ != 0) return false;
    if (IsEdenMap() && edenForbiddenFruit_.claimed
        && DistanceXZ(camera_.position, Vector3Zero()) > EdenCombatBoundaryRadius()) {
        return false;
    }

    Vector3 up = UpForWorldAt(edenArk_.position, 0);
    Vector3 rawForward{std::cos(edenArk_.heading), 0.0f, std::sin(edenArk_.heading)};
    Vector3 forward = IsSphericalMap()
        ? SafeNormalize(ProjectOnSphericalTangent(edenArk_.forward, up),
            SafeNormalize(ProjectOnSphericalTangent(rawForward, up), Vector3{0.0f, 0.0f, 1.0f}))
        : rawForward;
    Vector3 right = SafeNormalize(Vector3CrossProduct(forward, up), Vector3{1.0f, 0.0f, 0.0f});
    Vector3 center = Vector3Add(edenArk_.position, Vector3Scale(up, 4.9f));
    constexpr float halfX = 8.2f;
    constexpr float halfY = 4.9f;
    constexpr float halfZ = 17.2f;
    float standingAltitude = IsSphericalMap() ? SphericalPlayerAltitude() : playerHeight_;

    Vector3 feet = Vector3Subtract(camera_.position, Vector3Scale(up, standingAltitude));
    Vector3 previousFeet = Vector3Subtract(previousPosition, Vector3Scale(up, standingAltitude));
    Vector3 feetOffset = Vector3Subtract(feet, center);
    Vector3 previousFeetOffset = Vector3Subtract(previousFeet, center);
    float localFeetX = Vector3DotProduct(feetOffset, right);
    float localFeetY = Vector3DotProduct(feetOffset, up);
    float localFeetZ = Vector3DotProduct(feetOffset, forward);
    float previousFeetY = Vector3DotProduct(previousFeetOffset, up);
    bool overDeck = std::abs(localFeetX) <= halfX + playerRadius_
        && std::abs(localFeetZ) <= halfZ + playerRadius_;
    float verticalSpeed = Vector3DotProduct(playerVelocity_, up);
    if (overDeck && verticalSpeed <= 0.0f && previousFeetY >= halfY - 0.45f && localFeetY <= halfY + 0.55f) {
        Vector3 contact = Vector3Add(center, Vector3Add(Vector3Scale(right, localFeetX), Vector3Scale(forward, localFeetZ)));
        camera_.position = Vector3Add(contact, Vector3Scale(up, halfY + standingAltitude));
        if (verticalSpeed < 0.0f) {
            playerVelocity_ = Vector3Subtract(playerVelocity_, Vector3Scale(up, verticalSpeed));
        }
        grounded_ = true;
        coyoteTimer_ = 0.11f;
        if (jumpBufferTimer_ > 0.0f) {
            playerVelocity_ = Vector3Add(ProjectOnSphericalTangent(playerVelocity_, up), Vector3Scale(up, config_.jumpSpeed));
            grounded_ = false;
            coyoteTimer_ = 0.0f;
            jumpBufferTimer_ = 0.0f;
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.12f);
        }
        return true;
    }

    Vector3 cameraOffset = Vector3Subtract(camera_.position, center);
    float localX = Vector3DotProduct(cameraOffset, right);
    float localY = Vector3DotProduct(cameraOffset, up);
    float localZ = Vector3DotProduct(cameraOffset, forward);
    float capsuleBottomY = localY - standingAltitude;
    float capsuleTopY = localY - playerRadius_ * 0.35f;
    bool verticalOverlap = capsuleBottomY <= halfY - 0.15f && capsuleTopY >= -halfY;
    bool horizontalOverlap = std::abs(localX) <= halfX + playerRadius_
        && std::abs(localZ) <= halfZ + playerRadius_;
    if (!verticalOverlap || !horizontalOverlap) {
        return false;
    }

    float penX = halfX + playerRadius_ - std::abs(localX);
    float penZ = halfZ + playerRadius_ - std::abs(localZ);
    Vector3 pushAxis;
    if (penX < penZ) {
        float sign = localX >= 0.0f ? 1.0f : -1.0f;
        pushAxis = Vector3Scale(right, sign);
        localX = sign * (halfX + playerRadius_ + 0.02f);
    } else {
        float sign = localZ >= 0.0f ? 1.0f : -1.0f;
        pushAxis = Vector3Scale(forward, sign);
        localZ = sign * (halfZ + playerRadius_ + 0.02f);
    }
    camera_.position = Vector3Add(center,
        Vector3Add(Vector3Scale(right, localX),
            Vector3Add(Vector3Scale(up, localY), Vector3Scale(forward, localZ))));
    float pushSpeed = Vector3DotProduct(playerVelocity_, pushAxis);
    if (pushSpeed < 0.0f) {
        playerVelocity_ = Vector3Subtract(playerVelocity_, Vector3Scale(pushAxis, pushSpeed));
    }
    return true;
}
void Game::ResolveMapCollision(Vector3 previousPosition) {
    if (IsEdenMap()) {
        if (ResolveEdenArkCollision(previousPosition)) {
            return;
        }
        if (edenForbiddenFruit_.claimed
            && DistanceXZ(camera_.position, Vector3Zero()) > EdenCombatBoundaryRadius()) {
            grounded_ = false;
            (void)previousPosition;
            return;
        }
        if (ResolveEdenFloatingStoneCollision(previousPosition)) {
            return;
        }
        float floorY = EdenGroundYAt(camera_.position) + playerHeight_;
        float distanceToGround = camera_.position.y - floorY;
        if (distanceToGround <= 1.0f) {
            if (camera_.position.y < floorY) {
                camera_.position.y = floorY;
            }
            if (playerVelocity_.y < 0.0f) playerVelocity_.y = 0.0f;
            grounded_ = true;
            coyoteTimer_ = 0.11f;
        }
        (void)previousPosition;
        return;
    }
    if (IsSphericalMap()) {
        if (ResolveEdenArkCollision(previousPosition)) {
            return;
        }
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

    if (ResolveEdenArkCollision(previousPosition)) {
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
bool Game::ResolveEdenFloatingStoneCollision(Vector3 previousPosition) {
    if (!IsEdenMap()) return false;
    for (int i = 0; i < 37; ++i) {
        Vector3 center = EdenFloatingStonePosition(i);
        float seed = static_cast<float>(i) + 71.0f;
        float noise = std::fmod(std::sin((seed + 2.0f) * 12.9898f + 78.233f) * 43758.5453f + 43758.5453f, 1.0f);
        float cityScale = i == 0 ? 7.4f : (3.3f + static_cast<float>((i - 1) / 12) * 0.88f);
        float size = (1.05f + noise * 1.35f) * cityScale;
        float halfX = size * 1.4f + playerRadius_;
        float halfZ = size * 1.05f + playerRadius_;
        float topY = center.y + size * 0.38f;
        float previousFeetY = previousPosition.y - playerHeight_;
        float feetY = camera_.position.y - playerHeight_;
        bool inside = std::abs(camera_.position.x - center.x) <= halfX
            && std::abs(camera_.position.z - center.z) <= halfZ;
        if (inside && playerVelocity_.y <= 0.0f && previousFeetY >= topY - 0.45f && feetY <= topY + 0.55f) {
            camera_.position.y = topY + playerHeight_;
            playerVelocity_.y = 0.0f;
            grounded_ = true;
            coyoteTimer_ = 0.11f;
            return true;
        }
    }
    return false;
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
    } else if (IsLabyrinthMap()) {
        LabyrinthCell cells[] = {
            LabyrinthCell{1, 1},
            LabyrinthCell{std::max(1, labyrinthWidth_ - 2), 1},
            LabyrinthCell{1, std::max(1, labyrinthHeight_ - 2)}
        };
        LabyrinthCell cell = cells[std::clamp(slot, 0, 2)];
        if (!LabyrinthCellOpen(cell.x, cell.y)) {
            cell = LabyrinthFarthestCellFrom(1, 1);
        }
        position = LabyrinthCellCenter(cell.x, cell.y);
        position.x += RandomFloat(-config_.labyrinthCellSize * 0.18f, config_.labyrinthCellSize * 0.18f);
        position.z += RandomFloat(-config_.labyrinthCellSize * 0.18f, config_.labyrinthCellSize * 0.18f);
        position.y = 1.0f;
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
            } else if (IsLabyrinthMap()) {
                LabyrinthCell playerCell = LabyrinthCellForPosition(camera_.position);
                LabyrinthCell cell = LabyrinthFarthestCellFrom(playerCell.x, playerCell.y);
                for (int attempt = 0; attempt < 24; ++attempt) {
                    int x = GetRandomValue(1, std::max(1, labyrinthWidth_ - 2));
                    int y = GetRandomValue(1, std::max(1, labyrinthHeight_ - 2));
                    if (!LabyrinthCellOpen(x, y)) continue;
                    Vector3 center = LabyrinthCellCenter(x, y);
                    if (DistanceXZ(center, camera_.position) > config_.labyrinthCellSize * 2.0f) {
                        cell = LabyrinthCell{x, y};
                        break;
                    }
                }
                position = LabyrinthCellCenter(cell.x, cell.y);
                position.y = 1.0f;
            } else if (IsSquareMap()) {
                float hx = squareHalfExtent_ - 3.0f;
                position = Vector3{RandomFloat(-hx, hx), 1.0f, RandomFloat(-hx, hx)};
            } else {
                float angle = RandomFloat(0.0f, 6.2831853f);
                float radius = arenaRadius_ * RandomFloat(0.15f, 0.75f);
                position = Vector3{std::cos(angle) * radius, 1.0f, std::sin(angle) * radius};
            }
            Pickup essencePickup;
            essencePickup.type = PickupType::Essence;
            essencePickup.position = position;
            essencePickup.radius = 0.85f;
            essencePickup.bobTimer = RandomFloat(0.0f, 6.28f);
            essencePickup.maxLife = config_.droppedEssenceLifetime;
            pickups_.push_back(essencePickup);
        }
    }
}
void Game::UpdateEdenEssenceField(float dt) {
    if (!IsEdenMap()) return;
    if (config_.edenForbiddenFruitEnabled && !edenForbiddenFruit_.active) {
        ResetEdenForbiddenFruit();
    }
    if (edenForbiddenFruit_.active) {
        edenForbiddenFruit_.position = EdenForbiddenFruitPosition();
        edenForbiddenFruit_.spin += dt;
    }
    if (edenForbiddenFruit_.active && edenForbiddenFruit_.claimed) {
        edenForbiddenFruit_.apocalypse = std::clamp(
            edenForbiddenFruit_.apocalypse + dt * config_.edenForbiddenFruitApocalypseSpeed,
            0.0f,
            1.0f);
        Vector3 outward{camera_.position.x, 0.0f, camera_.position.z};
        if (Vector3Length(outward) <= 0.001f) {
            Vector3 forward = PlayerForward();
            outward = Vector3{forward.x, 0.0f, forward.z};
        }
        outward = SafeNormalize(outward, Vector3{1.0f, 0.0f, 0.0f});
        float force = CurrentGravity() * config_.edenForbiddenFruitExileForceScale;
        playerVelocity_ = Vector3Add(playerVelocity_, Vector3Scale(outward, force * dt));
    }
    float fruitAttractionScale = EdenPlayerGravityScale();

    int essenceOnMap = 0;
    for (Pickup& pickup : pickups_) {
        pickup.bobTimer += dt;
        if (pickup.type == PickupType::Essence) ++essenceOnMap;
    }

    for (size_t i = 0; i < pickups_.size();) {
        Pickup& pickup = pickups_[i];
        if (pickup.type != PickupType::Essence) {
            ++i;
            continue;
        }
        if (fruitAttractionScale > 0.001f
            && edenForbiddenFruit_.active && !edenForbiddenFruit_.claimed
            && config_.edenForbiddenFruitAbsorbSpeed > 0.0f
            && config_.edenForbiddenFruitAbsorbRange > 0.0f) {
            Vector3 toFruit = Vector3Subtract(edenForbiddenFruit_.position, pickup.position);
            float fruitDistance = Vector3Length(toFruit);
            if (fruitDistance <= config_.edenForbiddenFruitAbsorbRange) {
                if (fruitDistance <= config_.edenForbiddenFruitRadius * 1.15f + pickup.radius * 0.45f) {
                    ++edenForbiddenFruit_.absorbedEssence;
                    SpawnHitBurst(edenForbiddenFruit_.position, Color{255, 215, 86, 255}, 12);
                    pickups_[i] = pickups_.back();
                    pickups_.pop_back();
                    --essenceOnMap;
                    continue;
                }
                float pullT = 1.0f - std::clamp(fruitDistance / std::max(0.001f, config_.edenForbiddenFruitAbsorbRange), 0.0f, 1.0f);
                float speed = config_.edenForbiddenFruitAbsorbSpeed * fruitAttractionScale * (0.35f + pullT * 1.65f);
                Vector3 step = Vector3Scale(Vector3Normalize(toFruit), std::min(fruitDistance, speed * dt));
                pickup.position = Vector3Add(pickup.position, step);
            }
        }
        if (edenRiverBlessing_ == 3 && edenRiverBlessingTimer_ > 0.0f) {
            Vector3 toPlayer = Vector3Subtract(camera_.position, pickup.position);
            float playerDistance = Vector3Length(toPlayer);
            if (playerDistance > 0.001f && playerDistance <= 42.0f) {
                float pullT = 1.0f - std::clamp(playerDistance / 42.0f, 0.0f, 1.0f);
                float speed = 7.0f + pullT * 15.0f;
                pickup.position = Vector3Add(pickup.position,
                    Vector3Scale(Vector3Normalize(toPlayer), std::min(playerDistance, speed * dt)));
            }
        }
        float verticalReach = std::abs(camera_.position.y - pickup.position.y);
        bool touched = DistanceXZ(camera_.position, pickup.position) <= pickup.radius + playerRadius_ + 0.35f
            && verticalReach <= 2.35f;
        if (touched) {
            essence_++;
            PlaySfx(sfxEssence_);
            SpawnHitBurst(pickup.position, Color{255, 230, 112, 255}, 30);
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.12f);
            eventText_ = "EDEN ESSENCE +1";
            eventTextTimer_ = 1.1f;
            pickups_[i] = pickups_.back();
            pickups_.pop_back();
            --essenceOnMap;
            continue;
        }
        ++i;
    }

    if (essenceOnMap >= config_.edenEssenceTargetCount) {
        edenEssenceRespawnTimer_ = std::min(edenEssenceRespawnTimer_, config_.edenEssenceRespawnInterval);
        return;
    }

    edenEssenceRespawnTimer_ -= dt;
    int spawnCount = 0;
    if (edenEssenceRespawnTimer_ <= 0.0f) {
        spawnCount = std::max(1, std::min(6, config_.edenEssenceTargetCount - essenceOnMap));
        edenEssenceRespawnTimer_ = config_.edenEssenceRespawnInterval;
    } else if (essenceOnMap == 0) {
        spawnCount = std::min(12, config_.edenEssenceTargetCount);
    }

    for (int n = 0; n < spawnCount; ++n) {
        int treeIndex = GetRandomValue(0, 72);
        bool lifeTree = false;
        Vector3 tree = EdenTreePosition(treeIndex, &lifeTree);
        float angle = RandomFloat(0.0f, 2.0f * PI);
        float scatter = lifeTree ? RandomFloat(1.5f, 6.5f) : RandomFloat(0.8f, 3.6f);
        Vector3 position{
            tree.x + std::cos(angle) * scatter,
            0.0f,
            tree.z + std::sin(angle) * scatter
        };
        float treeHeight = lifeTree ? 18.0f : RandomFloat(5.5f, 11.5f);
        float altitude = treeHeight + RandomFloat(2.0f, std::max(4.0f, config_.edenEssenceAltitudeMax * 0.45f));
        position.y = EdenGroundYAt(position) + altitude;
        Pickup pickup;
        pickup.type = PickupType::Essence;
        pickup.position = position;
        pickup.radius = 0.95f;
        pickup.bobTimer = RandomFloat(0.0f, 6.28f);
        pickups_.push_back(pickup);
    }
}
void Game::SpawnEdenFireRain() {
    if (!IsEdenMap() || !edenForbiddenFruit_.claimed) return;
    int count = config_.edenFireRainBurstCount;
    for (int i = 0; i < count; ++i) {
        float angle = RandomFloat(0.0f, 2.0f * PI);
        float radius = std::sqrt(RandomFloat(0.0f, 1.0f)) * std::min(68.0f, config_.edenMapRadius * 0.34f);
        Vector3 position{
            camera_.position.x + std::cos(angle) * radius,
            0.0f,
            camera_.position.z + std::sin(angle) * radius
        };
        float mapR = DistanceXZ(position, Vector3Zero());
        float maxR = std::max(1.0f, config_.edenMapRadius * 0.92f);
        if (mapR > maxR && mapR > 0.001f) {
            float scale = maxR / mapR;
            position.x *= scale;
            position.z *= scale;
        }
        float skyBase = camera_.position.y + config_.edenFireRainSpawnHeight;
        position.y = skyBase + config_.edenFireRainSpawnHeight * RandomFloat(0.0f, 0.35f);

        Vector3 target{
            position.x + RandomFloat(-10.0f, 10.0f),
            EdenGroundYAt(position),
            position.z + RandomFloat(-10.0f, 10.0f)
        };
        Vector3 dir = SafeNormalize(Vector3Subtract(target, position), Vector3{0.0f, -1.0f, 0.0f});
        Vector3 side = SafeNormalize(Vector3CrossProduct(dir, Vector3{0.0f, 1.0f, 0.0f}), Vector3{1.0f, 0.0f, 0.0f});
        side = RotateAroundAxis(side, dir, RandomFloat(0.0f, 2.0f * PI));

        SeraphFireball fireball;
        fireball.position = position;
        fireball.prevPosition = position;
        fireball.velocity = Vector3Scale(dir, config_.edenFireRainSpeed * RandomFloat(0.82f, 1.18f));
        fireball.flightDirection = dir;
        fireball.tipDirection = dir;
        fireball.visualSide = side;
        fireball.life = config_.edenFireRainLifetime * RandomFloat(0.82f, 1.12f);
        fireball.maxLife = fireball.life;
        fireball.radius = config_.edenFireRainRadius;
        fireball.damage = config_.edenFireRainDamage;
        fireball.world = 0;
        fireball.sodomFire = true;
        edenFireRain_.push_back(fireball);
    }
}
void Game::ExplodeEdenFireRain(Vector3 position, bool hitPlayer) {
    position.y = EdenGroundYAt(position) + 0.05f;
    if (!hitPlayer && Vector3Distance(camera_.position, position) <= config_.edenFireRainRadius + playerRadius_ + 0.55f) {
        ApplyPlayerHit(camera_.position, Color{255, 120, 64, 255}, "SODOM FIRE");
    }
    if (config_.edenFireRainFireDuration > 0.0f) {
        firePatches_.push_back(FirePatch{
            position,
            Vector3{0.0f, 1.0f, 0.0f},
            config_.edenFireRainFireDuration,
            config_.edenFireRainFireDuration,
            config_.edenFireRainFireRadius,
            config_.edenFireRainFireDps,
            0,
            JPH::BodyID(),
            true, false,
            Color{210, 72, 52, 255},
            Color{255, 152, 72, 255},
            Color{255, 82, 42, 220}
        });
    }
    PlaySfxAt(sfxNapalmExplosion_, position, 82.0f, 0.58f);
    SpawnShockwave(position, std::max(1.5f, config_.edenFireRainFireRadius * 0.9f), Color{255, 92, 40, 255});
    SpawnHitBurst(position, Color{255, 182, 86, 255}, 24);
}
void Game::UpdateEdenFireRain(float dt) {
    if (!IsEdenMap() || !edenForbiddenFruit_.claimed) {
        edenFireRain_.clear();
        edenFireRainTimer_ = 0.0f;
        return;
    }

    edenFireRainTimer_ -= dt;
    if (edenFireRainTimer_ <= 0.0f) {
        edenFireRainTimer_ = config_.edenFireRainInterval;
        SpawnEdenFireRain();
    }

    for (size_t i = 0; i < edenFireRain_.size();) {
        SeraphFireball& fireball = edenFireRain_[i];
        fireball.life -= dt;
        fireball.prevPosition = fireball.position;
        fireball.velocity.y -= config_.gravity * 0.06f * dt;
        fireball.position = Vector3Add(fireball.position, Vector3Scale(fireball.velocity, dt));

        bool hitPlayer = false;
        bool explode = fireball.life <= 0.0f;
        if (!explode && EnemyTouchesPlayer(fireball.position, fireball.radius)) {
            ApplyPlayerHit(camera_.position, Color{255, 120, 64, 255}, "SODOM FIRE");
            hitPlayer = true;
            explode = true;
        }
        if (!explode && fireball.position.y <= EdenGroundYAt(fireball.position) + fireball.radius * 0.55f) {
            explode = true;
        }
        if (!explode && DistanceXZ(fireball.position, Vector3Zero()) > config_.edenMapRadius + 8.0f) {
            explode = true;
        }

        if (explode) {
            ExplodeEdenFireRain(fireball.position, hitPlayer);
            edenFireRain_[i] = edenFireRain_.back();
            edenFireRain_.pop_back();
            continue;
        }

        if (RandomFloat(0.0f, 1.0f) < 0.75f) {
            particles_.push_back(Particle{
                fireball.position,
                Vector3{RandomFloat(-0.7f, 0.7f), RandomFloat(0.4f, 1.4f), RandomFloat(-0.7f, 0.7f)},
                Color{255, 132, 64, 190},
                RandomFloat(0.12f, 0.25f), RandomFloat(0.12f, 0.25f),
                RandomFloat(0.04f, 0.08f)
            });
        }
        ++i;
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
