#include "Game.h"
#include "GameMath.h"

#include <algorithm>
#include <cmath>

#include <cstring>

#include <rlgl.h>

void Game::DrawBethlehem() const {
    if (!bethlehem_.active) return;

    if (bethlehemModelLoaded_) {
        DrawModel(bethlehemModel_, bethlehem_.position, 1.0f,
            TutorialMode() ? Color{140, 140, 155, 255} : WHITE);
    } else {
        Color mainColor = TutorialMode() ? Color{140, 140, 155, 255} : Color{255, 210, 100, 255};
        Color wireColor = TutorialMode() ? Color{120, 120, 135, 220} : Color{255, 180, 50, 220};
        DrawSphereEx(bethlehem_.position, 2.5f, 12, 10, mainColor);
        DrawSphereWires(bethlehem_.position, 2.8f, 14, 12, wireColor);
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
        float groundY = FlatGroundYForWorld(playerWorld_);
        float worldSide = FlatUpForWorld(playerWorld_).y;
        float surfaceOffset = groundY + 0.03f * worldSide;
        DrawCube(Vector3{0.0f, groundY - 0.08f * worldSide, 0.0f}, size, 0.18f, size, Color{20, 20, 22, 255});
        DrawCubeWires(Vector3{0.0f, groundY + 0.02f * worldSide, 0.0f}, size, 0.2f, size, Color{135, 130, 122, 255});
        for (int i = -3; i <= 3; ++i) {
            float p = static_cast<float>(i) * squareHalfExtent_ / 3.0f;
            DrawLine3D(Vector3{-squareHalfExtent_, surfaceOffset, p}, Vector3{squareHalfExtent_, surfaceOffset, p}, Color{45, 45, 48, 255});
            DrawLine3D(Vector3{p, surfaceOffset, -squareHalfExtent_}, Vector3{p, surfaceOffset, squareHalfExtent_}, Color{45, 45, 48, 255});
        }
        DrawLine3D(Vector3{-squareHalfExtent_, surfaceOffset, -squareHalfExtent_}, Vector3{squareHalfExtent_, surfaceOffset, -squareHalfExtent_}, Color{160, 150, 135, 255});
        DrawLine3D(Vector3{squareHalfExtent_, surfaceOffset, -squareHalfExtent_}, Vector3{squareHalfExtent_, surfaceOffset, squareHalfExtent_}, Color{160, 150, 135, 255});
        DrawLine3D(Vector3{squareHalfExtent_, surfaceOffset, squareHalfExtent_}, Vector3{-squareHalfExtent_, surfaceOffset, squareHalfExtent_}, Color{160, 150, 135, 255});
        DrawLine3D(Vector3{-squareHalfExtent_, surfaceOffset, squareHalfExtent_}, Vector3{-squareHalfExtent_, surfaceOffset, -squareHalfExtent_}, Color{160, 150, 135, 255});
        return;
    }

    float groundY = FlatGroundYForWorld(playerWorld_);
    float worldSide = FlatUpForWorld(playerWorld_).y;
    float surfaceOffset = groundY + 0.018f * worldSide;
    DrawCylinder(Vector3{0.0f, groundY - 0.01f * worldSide, 0.0f}, arenaRadius_, arenaRadius_, 0.02f, 24, Color{20, 20, 22, 255});
    DrawCylinderWires(Vector3{0.0f, groundY - 0.01f * worldSide, 0.0f}, arenaRadius_, arenaRadius_, 0.025f, 24, Color{135, 130, 122, 255});

    for (int i = 0; i < 24; ++i) {
        float angle = (static_cast<float>(i) / 24.0f) * 6.2831853f;
        Vector3 end = Vector3{std::cos(angle) * arenaRadius_, surfaceOffset, std::sin(angle) * arenaRadius_};
        DrawLine3D(Vector3{0.0f, surfaceOffset, 0.0f}, end, Color{54, 52, 55, 255});
    }

    for (int ring = 1; ring <= 4; ++ring) {
        float radius = arenaRadius_ * static_cast<float>(ring) / 5.0f;
        DrawCylinderWires(Vector3{0.0f, surfaceOffset, 0.0f}, radius, radius, 0.01f, 24, Color{45, 45, 48, 255});
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
    Vector3 normal = IsSphericalMap() ? SafeNormalize(platform.normal, SphericalUpAt(platform.position)) : platform.normal;
    Vector3 right = IsSphericalMap() ? SafeNormalize(platform.right, PlayerRight()) : platform.right;
    Vector3 forward = IsSphericalMap() ? SafeNormalize(platform.forward, PlayerForward()) : platform.forward;
    Vector3 center = IsSphericalMap()
        ? Vector3Add(platform.position, Vector3Scale(normal, 0.035f))
        : Vector3{platform.position.x, platform.position.y + platform.scale.y * 0.5f, platform.position.z};
    Vector3 corners[4] = {
        Vector3Add(center, Vector3Add(Vector3Scale(right, -platform.scale.x * 0.5f), Vector3Scale(forward, -platform.scale.z * 0.5f))),
        Vector3Add(center, Vector3Add(Vector3Scale(right, platform.scale.x * 0.5f), Vector3Scale(forward, -platform.scale.z * 0.5f))),
        Vector3Add(center, Vector3Add(Vector3Scale(right, platform.scale.x * 0.5f), Vector3Scale(forward, platform.scale.z * 0.5f))),
        Vector3Add(center, Vector3Add(Vector3Scale(right, -platform.scale.x * 0.5f), Vector3Scale(forward, platform.scale.z * 0.5f)))
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
    if (activeWeapon_ == WeaponType::NanoConstructor && nanoConstructorMode_ == NanoConstructorMode::NanoPlatform && state_ == State::Playing) {
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
        Vector3 normal = IsSphericalMap() ? SafeNormalize(platform.normal, SphericalUpAt(platform.position)) : Vector3{0.0f, 1.0f, 0.0f};
        Vector3 right = IsSphericalMap() ? SafeNormalize(platform.right, PlayerRight()) : platform.right;
        Vector3 forward = IsSphericalMap() ? SafeNormalize(platform.forward, PlayerForward()) : platform.forward;
        Vector3 center = IsSphericalMap()
            ? platform.position
            : Vector3{platform.position.x, platform.position.y + platform.scale.y * 0.5f, platform.position.z};
        Vector3 top[4] = {
            Vector3Add(center, Vector3Add(Vector3Scale(right, -platform.scale.x * 0.5f), Vector3Add(Vector3Scale(forward, -platform.scale.z * 0.5f), Vector3Scale(normal, platform.scale.y * 0.5f)))),
            Vector3Add(center, Vector3Add(Vector3Scale(right, platform.scale.x * 0.5f), Vector3Add(Vector3Scale(forward, -platform.scale.z * 0.5f), Vector3Scale(normal, platform.scale.y * 0.5f)))),
            Vector3Add(center, Vector3Add(Vector3Scale(right, platform.scale.x * 0.5f), Vector3Add(Vector3Scale(forward, platform.scale.z * 0.5f), Vector3Scale(normal, platform.scale.y * 0.5f)))),
            Vector3Add(center, Vector3Add(Vector3Scale(right, -platform.scale.x * 0.5f), Vector3Add(Vector3Scale(forward, platform.scale.z * 0.5f), Vector3Scale(normal, platform.scale.y * 0.5f))))
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
        DrawTriangle3D(bottom[0], bottom[2], bottom[1], fill);
        DrawTriangle3D(bottom[0], bottom[3], bottom[2], fill);
        DrawTriangle3D(bottom[1], bottom[2], bottom[0], fill);
        DrawTriangle3D(bottom[2], bottom[3], bottom[0], fill);
        Color side = FadeColor(Color{255, 188, 68, 255}, 0.22f + alpha * 0.22f);
        for (int edge = 0; edge < 4; ++edge) {
            int next = (edge + 1) % 4;
            DrawTriangle3D(top[edge], bottom[edge], top[next], side);
            DrawTriangle3D(top[next], bottom[edge], bottom[next], side);
            DrawLine3D(top[edge], bottom[edge], FadeColor(Color{255, 244, 170, 255}, 0.68f));
        }
        DrawNanoPlatformFrame(platform, FadeColor(Color{255, 245, 170, 255}, 0.72f), false);
    }
}
void Game::DrawEnemies() const {
    for (const Enemy& enemy : enemies_) {
        Vector3 position = BodyPosition(enemy.body);
        float pulse = 1.0f + std::sin(enemy.bobTimer * 5.0f) * 0.06f;

        Vector3 surfUp = IsSphericalMap() ? SphericalUpAt(position) : Vector3{0.0f, 1.0f, 0.0f};
        rlPushMatrix();
        rlTranslatef(position.x, position.y, position.z);
        if (IsSphericalMap()) {
            Vector3 axis = Vector3CrossProduct(Vector3{0.0f, 1.0f, 0.0f}, surfUp);
            float axisLen = Vector3Length(axis);
            if (axisLen > 0.001f) {
                float angle = std::acos(Vector3DotProduct(Vector3{0.0f, 1.0f, 0.0f}, surfUp));
                rlRotatef(angle * RAD2DEG, axis.x / axisLen, axis.y / axisLen, axis.z / axisLen);
            } else if (surfUp.y < 0.0f) {
                rlRotatef(180.0f, 1.0f, 0.0f, 0.0f);
            }
        }

        if (enemy.type == EnemyType::Brute) {
            DrawCube(Vector3Zero(), enemy.radius * 1.65f, enemy.radius * 1.65f, enemy.radius * 1.65f, enemy.color);
            DrawCubeWires(Vector3Zero(), enemy.radius * 1.8f, enemy.radius * 1.8f, enemy.radius * 1.8f, Color{55, 8, 0, 255});
            for (int i = 0; i < 4; ++i) {
                float angle = static_cast<float>(i) * 1.5707963f;
                Vector3 spike = Vector3{std::cos(angle) * enemy.radius * 1.35f, enemy.radius * 0.5f, std::sin(angle) * enemy.radius * 1.35f};
                DrawLine3D(Vector3Zero(), spike, Color{255, 190, 120, 255});
            }
        } else if (enemy.type == EnemyType::Wisp) {
            DrawSphereEx(Vector3Zero(), enemy.radius * pulse, 6, 6, enemy.color);
            DrawSphereWires(Vector3Zero(), enemy.radius * 1.45f, 6, 6, Color{170, 235, 255, 200});
            DrawLine3D(Vector3{-enemy.radius, 0.0f, 0.0f}, Vector3{enemy.radius, 0.0f, 0.0f}, RAYWHITE);
        } else if (enemy.type == EnemyType::Spitter) {
            DrawSphereEx(Vector3Zero(), enemy.radius * pulse, 7, 5, enemy.color);
            DrawCylinderWires(Vector3Zero(), enemy.radius * 1.15f, enemy.radius * 0.55f, enemy.radius * 1.5f, 6, Color{35, 255, 190, 210});
            DrawSphereEx(Vector3{0.0f, enemy.radius * 0.3f, 0.0f}, enemy.radius * 0.35f, 5, 4, Color{210, 255, 235, 240});
        } else if (enemy.type == EnemyType::Pouncer) {
            float squash = enemy.cooldownTimer > config_.pouncerLeapInterval - 0.35f ? 0.62f : 1.0f;
            DrawSphereEx(Vector3Zero(), enemy.radius * pulse, 7, 5, enemy.color);
            DrawCube(Vector3{0.0f, -enemy.radius * 0.18f, 0.0f}, enemy.radius * 2.1f, enemy.radius * squash, enemy.radius * 1.35f, FadeColor(enemy.color, 0.75f));
            DrawSphereWires(Vector3Zero(), enemy.radius * 1.25f, 7, 5, Color{255, 160, 255, 190});
        } else if (enemy.type == EnemyType::Harrier) {
            Vector3 nose = Vector3{0.0f, enemy.radius * 0.12f, -enemy.radius * 1.6f};
            Vector3 tail = Vector3{0.0f, -enemy.radius * 0.06f, enemy.radius * 1.35f};
            Vector3 left = Vector3{-enemy.radius * 1.35f, 0.0f, enemy.radius * 0.15f};
            Vector3 right = Vector3{enemy.radius * 1.35f, 0.0f, enemy.radius * 0.15f};
            Vector3 top = Vector3{0.0f, enemy.radius * 0.75f, 0.0f};
            Vector3 bottom = Vector3{0.0f, -enemy.radius * 0.55f, 0.0f};
            Color fill = enemy.cooldownTimer < 0.22f ? Color{220, 255, 255, 255} : enemy.color;
            DrawTriangle3D(nose, left, top, fill);
            DrawTriangle3D(nose, top, right, fill);
            DrawTriangle3D(tail, top, left, FadeColor(fill, 0.75f));
            DrawTriangle3D(tail, right, top, FadeColor(fill, 0.75f));
            DrawTriangle3D(nose, bottom, left, FadeColor(fill, 0.72f));
            DrawTriangle3D(nose, right, bottom, FadeColor(fill, 0.72f));
            DrawLine3D(left, right, Color{230, 255, 255, 230});
            DrawSphereWires(Vector3Zero(), enemy.radius * 1.18f, 6, 5, Color{150, 245, 255, 190});
        } else if (enemy.type == EnemyType::Blinker) {
            float warning = enemy.telegraphTimer > 0.0f ? 1.0f : 0.0f;
            Color core = warning > 0.0f ? Color{255, 220, 245, 255} : enemy.color;
            DrawSphereEx(Vector3Zero(), enemy.radius * (0.82f + warning * 0.22f) * pulse, 6, 5, core);
            DrawCube(Vector3{-enemy.radius * 0.35f, enemy.radius * 0.12f, 0.0f}, enemy.radius * 0.85f, enemy.radius * 0.42f, enemy.radius * 1.25f, FadeColor(enemy.color, 0.82f));
            DrawCube(Vector3{enemy.radius * 0.42f, -enemy.radius * 0.16f, 0.0f}, enemy.radius * 0.7f, enemy.radius * 0.38f, enemy.radius * 1.05f, FadeColor(Color{120, 25, 90, 255}, 0.82f));
            DrawSphereWires(Vector3Zero(), enemy.radius * (1.2f + warning * 0.38f), 7, 5, warning > 0.0f ? Color{255, 235, 255, 230} : Color{255, 115, 225, 175});
            if (warning > 0.0f) {
                DrawLine3D(Vector3{-enemy.radius * 1.6f, 0.0f, 0.0f}, Vector3{enemy.radius * 1.6f, 0.0f, 0.0f}, Color{255, 220, 245, 255});
                DrawLine3D(Vector3{0.0f, -enemy.radius * 1.1f, 0.0f}, Vector3{0.0f, enemy.radius * 1.4f, 0.0f}, Color{255, 220, 245, 255});
            }
        } else if (enemy.type == EnemyType::Boss) {
            float rage = enemy.health < enemy.maxHealth * 0.45f ? 1.0f : 0.0f;
            DrawCube(Vector3Zero(), enemy.radius * 1.55f, enemy.radius * 1.15f, enemy.radius * 1.55f, enemy.color);
            DrawCubeWires(Vector3Zero(), enemy.radius * 1.75f, enemy.radius * 1.3f, enemy.radius * 1.75f, Color{35, 18, 75, 255});
            DrawSphereEx(Vector3{0.0f, enemy.radius * 0.85f, 0.0f}, enemy.radius * 0.72f, 8, 6, Color{175, 145, 255, 245});
            DrawSphereWires(Vector3Zero(), enemy.radius * (1.35f + std::sin(enemy.bobTimer * 3.0f) * 0.08f), 12, 8, Color{215, 180, 255, 220});
            for (int i = 0; i < 6; ++i) {
                float angle = static_cast<float>(i) / 6.0f * 6.2831853f + enemy.bobTimer * (0.7f + rage * 0.7f);
                Vector3 spike = Vector3{std::cos(angle) * enemy.radius * 1.35f, enemy.radius * 0.35f + std::sin(angle * 2.0f) * 0.35f, std::sin(angle) * enemy.radius * 1.35f};
                DrawLine3D(Vector3Zero(), spike, Color{230, 205, 255, 255});
                DrawSphereEx(spike, enemy.radius * 0.13f, 5, 4, rage > 0.0f ? Color{255, 90, 190, 230} : Color{160, 220, 255, 230});
            }
        } else if (enemy.type == EnemyType::SlimeKing) {
            float r = enemy.radius;
            float genScale = 1.0f - enemy.slimeGeneration * 0.15f;
            Color bodyColor = enemy.color;
            Color shellColor = Color{60, 180, 110, 230};
            Color glowColor = Color{140, 240, 180, 160};

            rlPushMatrix();
            rlScalef(1.0f, 0.55f, 1.0f);
            DrawSphereEx(Vector3{0.0f, -r * 0.25f, 0.0f}, r * genScale, 10, 8, bodyColor);
            DrawSphereWires(Vector3{0.0f, -r * 0.25f, 0.0f}, r * genScale * 1.12f, 10, 8, Color{30, 110, 70, 220});
            rlPopMatrix();
            DrawSphereEx(Vector3{0.0f, r * 0.25f, 0.0f}, r * 0.68f * genScale, 8, 6, shellColor);
            DrawSphereWires(Vector3{0.0f, r * 0.25f, 0.0f}, r * 0.72f * genScale, 8, 6, Color{40, 140, 90, 210});
            DrawSphereEx(Vector3{0.0f, r * 0.55f, 0.0f}, r * 0.22f * genScale, 6, 5, Color{80, 200, 130, 240});
            DrawCylinderWires(Vector3{0.0f, -r * 0.32f, 0.0f}, r * 0.95f * genScale, r * 0.95f * genScale, 0.02f, 16, Color{25, 95, 60, 210});
            float pulse = 1.0f + std::sin(static_cast<float>(GetTime()) * 3.0f + position.x * 0.5f) * 0.08f;
            DrawSphereWires(Vector3{0.0f, 0.0f, 0.0f}, r * (1.08f * pulse) * genScale, 10, 8, FadeColor(glowColor, 0.55f));
        } else if (enemy.type == EnemyType::Duelist) {
            Color core = enemy.telegraphTimer > 0.0f ? Color{255, 245, 150, 255} : enemy.color;
            DrawSphereEx(Vector3Zero(), enemy.radius * 0.92f * pulse, 7, 6, core);
            DrawCube(Vector3{0.0f, enemy.radius * 0.9f, 0.0f}, enemy.radius * 0.9f, enemy.radius * 0.34f, enemy.radius * 0.9f, Color{70, 62, 84, 255});
            DrawSphereWires(Vector3Zero(), enemy.radius * 1.32f, 8, 6, Color{255, 225, 135, 210});
            float weaponAngle = static_cast<float>(enemy.weaponSlot) / 8.0f * 6.2831853f + enemy.bobTimer * 2.0f;
            Vector3 focus = Vector3{std::cos(weaponAngle) * enemy.radius * 1.35f, enemy.radius * 0.35f, std::sin(weaponAngle) * enemy.radius * 1.35f};
            Color weaponColor = enemy.weaponSlot == 2 ? Color{255, 145, 80, 255} : enemy.weaponSlot == 4 ? Color{150, 115, 255, 255} : enemy.weaponSlot == 7 ? Color{255, 220, 95, 255} : Color{155, 235, 255, 255};
            DrawSphereEx(focus, enemy.radius * 0.18f, 5, 4, weaponColor);
            DrawLine3D(Vector3Zero(), focus, FadeColor(weaponColor, 0.85f));
        } else if (enemy.type == EnemyType::Dummy) {
            DrawSphereEx(Vector3Zero(), enemy.radius, 7, 6, enemy.color);
            DrawSphereWires(Vector3Zero(), enemy.radius * 1.05f, 7, 6, Color{80, 80, 95, 255});
            float crossLen = enemy.radius * 1.35f;
            Color crossColor = Color{170, 170, 185, 180};
            DrawLine3D(Vector3{-crossLen, 0.0f, 0.0f}, Vector3{crossLen, 0.0f, 0.0f}, crossColor);
            DrawLine3D(Vector3{0.0f, -crossLen, 0.0f}, Vector3{0.0f, crossLen, 0.0f}, crossColor);
            DrawLine3D(Vector3{0.0f, 0.0f, -crossLen}, Vector3{0.0f, 0.0f, crossLen}, crossColor);
        } else if (enemy.type == EnemyType::DummyBoss) {
            DrawCube(Vector3Zero(), enemy.radius * 1.55f, enemy.radius * 1.15f, enemy.radius * 1.55f, enemy.color);
            DrawCubeWires(Vector3Zero(), enemy.radius * 1.75f, enemy.radius * 1.3f, enemy.radius * 1.75f, Color{55, 55, 70, 255});
            DrawSphereEx(Vector3{0.0f, enemy.radius * 0.85f, 0.0f}, enemy.radius * 0.72f, 8, 6, Color{160, 160, 175, 245});
            DrawSphereWires(Vector3Zero(), enemy.radius * (1.35f + std::sin(enemy.bobTimer * 3.0f) * 0.08f), 12, 8, Color{180, 180, 195, 220});
            for (int i = 0; i < 6; ++i) {
                float angle = static_cast<float>(i) / 6.0f * 6.2831853f + enemy.bobTimer * 0.7f;
                Vector3 spike = Vector3{std::cos(angle) * enemy.radius * 1.35f, enemy.radius * 0.35f + std::sin(angle * 2.0f) * 0.35f, std::sin(angle) * enemy.radius * 1.35f};
                DrawLine3D(Vector3Zero(), spike, Color{200, 200, 215, 255});
                DrawSphereEx(spike, enemy.radius * 0.13f, 5, 4, Color{170, 170, 190, 230});
            }
        } else {
            DrawSphereEx(Vector3Zero(), enemy.radius, 7, 6, enemy.color);
            DrawSphereWires(Vector3Zero(), enemy.radius * 1.05f, 7, 6, Color{30, 0, 0, 255});
        }

        Vector3 hpTop = Vector3{0.0f, enemy.radius * 1.8f, 0.0f};
        DrawLine3D(Vector3Zero(), hpTop, Color{255, 220, 170, 255});
        rlPopMatrix();
    }
}
void Game::DrawPickups() {
    for (const Pickup& pickup : pickups_) {
        float bob = std::sin(pickup.bobTimer * 3.2f) * 0.12f;
        float pulse = 0.65f + std::sin(pickup.bobTimer * 5.0f) * 0.18f;
        Vector3 base = Vector3{0.0f, bob, 0.0f};

        Vector3 surfUp = IsSphericalMap() ? SphericalUpAt(pickup.position) : Vector3{0.0f, 1.0f, 0.0f};
        rlPushMatrix();
        rlTranslatef(pickup.position.x, pickup.position.y, pickup.position.z);
        if (IsSphericalMap()) {
            Vector3 axis = Vector3CrossProduct(Vector3{0.0f, 1.0f, 0.0f}, surfUp);
            float axisLen = Vector3Length(axis);
            if (axisLen > 0.001f) {
                float angle = std::acos(Vector3DotProduct(Vector3{0.0f, 1.0f, 0.0f}, surfUp));
                rlRotatef(angle * RAD2DEG, axis.x / axisLen, axis.y / axisLen, axis.z / axisLen);
            } else if (surfUp.y < 0.0f) {
                rlRotatef(180.0f, 1.0f, 0.0f, 0.0f);
            }
        }

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
        } else if (pickup.type == PickupType::Essence && essenceModelLoaded_) {
            // Spin around surface normal
            float spinSpeed = 120.0f + std::sin(pickup.bobTimer * 0.37f) * 40.0f;
            float spinAngle = pickup.bobTimer * spinSpeed;
            rlRotatef(spinAngle, surfUp.x, surfUp.y, surfUp.z);

            // Hue-cycle the model tint by modifying material color
            float hue = std::fmod(pickup.bobTimer * 45.0f, 360.0f);
            Color tint = ColorFromHSV(hue, 0.75f, 1.0f);
            for (int m = 0; m < essenceModel_.meshCount; ++m) {
                int matIdx = essenceModel_.meshMaterial[m];
                essenceModel_.materials[matIdx].maps[MATERIAL_MAP_DIFFUSE].color = tint;
            }
            DrawModel(essenceModel_, base, 0.65f, WHITE);
        }
        rlPopMatrix();
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
            float r = projectile.radius;

            // Local axes for prong spread (perpendicular to flight direction)
            Vector3 worldUp = {0.0f, 1.0f, 0.0f};
            Vector3 side = Vector3CrossProduct(forward, worldUp);
            if (Vector3Length(side) < 0.5f) {
                Vector3 refRight = {1.0f, 0.0f, 0.0f};
                side = Vector3CrossProduct(forward, refRight);
            }
            side = Vector3Normalize(side);
            Vector3 up = Vector3Normalize(Vector3CrossProduct(side, forward));

            // Spear geometry points
            Vector3 tail = Vector3Subtract(position, Vector3Scale(forward, r * 3.4f));
            Vector3 forkPoint = Vector3Add(position, Vector3Scale(forward, r * 1.6f));
            Vector3 tipCenter = Vector3Add(position, Vector3Scale(forward, r * 4.2f));

            // Main shaft — dark red-brown
            DrawCylinderEx(tail, forkPoint, r * 0.30f, r * 0.06f, 8, Color{140, 20, 20, 255});
            DrawCylinderWiresEx(tail, forkPoint, r * 0.34f, r * 0.10f, 8, FadeColor(Color{255, 140, 70, 255}, 0.5f));

            // Left prong — bright red, angled outward
            Vector3 leftProngTip = Vector3Add(Vector3Add(tipCenter, Vector3Scale(up, r * 0.55f)), Vector3Scale(forward, r * 0.3f));
            DrawCylinderEx(forkPoint, leftProngTip, r * 0.07f, r * 0.005f, 6, Color{209, 13, 13, 255});
            DrawCylinderWiresEx(forkPoint, leftProngTip, r * 0.085f, r * 0.015f, 6, FadeColor(Color{255, 160, 80, 255}, 0.5f));

            // Right prong — bright red, angled outward
            Vector3 rightProngTip = Vector3Add(Vector3Subtract(tipCenter, Vector3Scale(up, r * 0.55f)), Vector3Scale(forward, r * 0.3f));
            DrawCylinderEx(forkPoint, rightProngTip, r * 0.07f, r * 0.005f, 6, Color{209, 13, 13, 255});
            DrawCylinderWiresEx(forkPoint, rightProngTip, r * 0.085f, r * 0.015f, 6, FadeColor(Color{255, 160, 80, 255}, 0.5f));

            // Core glow at fork
            DrawSphereEx(forkPoint, r * 0.22f, 8, 6, Color{255, 60, 40, 240});

            // Trail
            DrawLine3D(tail, Vector3Add(tail, Vector3Scale(trail, 2.2f)), FadeColor(Color{255, 130, 50, 255}, 0.9f));
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
        } else if (projectile.kind == ProjectileKind::HomingShot) {
            float spin = static_cast<float>(GetTime()) * 8.0f;
            DrawSphereEx(position, projectile.radius * 1.3f, 7, 5, projectile.color);
            DrawSphereWires(position, projectile.radius * (1.9f + std::sin(spin) * 0.22f), 8, 5, FadeColor(Color{255, 200, 140, 255}, 0.75f));
            DrawCylinderWires(position, projectile.radius * 2.0f, projectile.radius * 1.2f, 0.03f, 10, FadeColor(Color{255, 180, 110, 255}, 0.6f));
            DrawLine3D(position, Vector3Add(position, Vector3Scale(trail, 1.5f)), FadeColor(projectile.color, 0.8f));
        } else if (projectile.kind == ProjectileKind::CurseOrb) {
            float spin = static_cast<float>(GetTime()) * 6.0f;
            DrawSphereEx(position, projectile.radius * 1.2f, 8, 6, projectile.color);
            DrawSphereWires(position, projectile.radius * (1.6f + std::sin(spin) * 0.25f), 10, 6,
                FadeColor(Color{220, 140, 255, 255}, 0.7f));
            DrawLine3D(position, Vector3Add(position, Vector3Scale(trail, 0.8f)),
                FadeColor(projectile.color, 0.8f));
        } else if (projectile.kind == ProjectileKind::SoulOrb) {
            float spin = static_cast<float>(GetTime()) * 5.0f;
            DrawSphereEx(position, projectile.radius * 0.9f, 5, 4, projectile.color);
            DrawSphereWires(position, projectile.radius * (1.4f + std::sin(spin) * 0.2f), 8, 5,
                FadeColor(Color{200, 120, 255, 255}, 0.6f));
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
void Game::DrawNanoBlades() const {
    for (const NanoBlade& blade : nanoBlades_) {
        Vector3 right = Vector3Length(blade.right) > 0.001f ? Vector3Normalize(blade.right) : PlayerRight();
        Vector3 up = Vector3Length(blade.up) > 0.001f ? Vector3Normalize(blade.up) : PlayerUp();
        if (blade.delay > 0.0f) {
            float pulse = 0.45f + std::sin(static_cast<float>(GetTime()) * 34.0f) * 0.18f;
            Color preflash = FadeColor(Color{255, 235, 150, 255}, pulse * 0.45f);
            DrawLine3D(Vector3Subtract(blade.center, Vector3Scale(right, blade.radius * 0.35f)), Vector3Add(blade.center, Vector3Scale(right, blade.radius * 0.35f)), preflash);
            DrawLine3D(Vector3Subtract(blade.center, Vector3Scale(up, blade.radius * 0.18f)), Vector3Add(blade.center, Vector3Scale(up, blade.radius * 0.18f)), preflash);
            continue;
        }

        constexpr int kSegments = 18;
        float alpha = blade.maxLife > 0.0f ? blade.life / blade.maxLife : 0.0f;
        float birth = 1.0f - alpha;
        float scale = 0.72f + std::min(1.0f, birth * 4.0f) * 0.28f;
        float outerRadius = blade.radius * scale;
        float innerRadius = std::max(0.05f, (blade.radius - blade.thickness) * scale);
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
            Vector3 outer = Vector3Add(blade.center, Vector3Add(Vector3Scale(right, x * outerRadius), Vector3Scale(up, y * outerRadius)));
            Vector3 inner = Vector3Add(blade.center, Vector3Add(Vector3Scale(right, x * innerRadius * 0.82f), Vector3Scale(up, (y * innerRadius) + blade.thickness * 0.38f * scale)));
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

        Vector3 centerGlow = Vector3Add(blade.center, Vector3Scale(up, blade.radius * 0.25f * scale));
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
        Vector3 rallyNormal = UpForWorldAt(rallyPoint_, playerWorld_);
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
        Vector3 aimNormal = UpForWorldAt(aimPoint, playerWorld_);
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
        if (d.state == DroneState::Active && d.world == playerWorld_) activeDrones++;
    }
    int enemyCount = 0;
    for (const Enemy& enemy : enemies_) {
        if (enemy.world == playerWorld_) ++enemyCount;
    }

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
void Game::DrawSlimeSpawnPods() const {
    for (const SlimeSpawnPod& pod : slimeSpawnPods_) {
        float alpha = pod.timer / pod.maxTimer;
        Color podColor = FadeColor(Color{80, 230, 130, 255}, 0.6f + alpha * 0.4f);
        float pulse = 1.0f + std::sin(static_cast<float>(GetTime()) * 12.0f + pod.position.x * 0.7f) * 0.12f;
        DrawSphereEx(pod.position, pod.radius * 0.35f * pulse, 6, 5, podColor);
        DrawSphereWires(pod.position, pod.radius * 0.42f * pulse, 6, 5, FadeColor(Color{40, 180, 90, 255}, alpha * 0.7f));
    }
}
void Game::DrawParticles() const {
    for (const Particle& particle : particles_) {
        float alpha = particle.maxLife > 0.0f ? particle.life / particle.maxLife : 0.0f;
        DrawCube(particle.position, particle.size, particle.size, particle.size, FadeColor(particle.color, alpha));
    }
}
void Game::DrawMagicCircles() const {
    for (const MagicCircle& circle : magicCircles_) {
        float alpha = circle.maxLife > 0.0f ? circle.life / circle.maxLife : 0.0f;
        if (circle.isWormhole) alpha = 1.0f;
        Color baseRing = circle.isWormhole ? Color{155, 45, 255, 255} : Color{180, 130, 255, 255};
        Color baseFrame = circle.isWormhole ? Color{230, 95, 255, 255} : Color{220, 180, 255, 255};
        Color ringColor = FadeColor(baseRing, alpha * 0.85f);
        Color frameColor = FadeColor(baseFrame, alpha * 0.75f);
        Vector3 up = UpForWorldAt(circle.position, circle.world);

        // Ground magic circle: concentric rings + pentagram
        // Slight lift above ground on flat maps to prevent z-fighting
        Vector3 groundPos = IsSphericalMap() ? circle.position
            : Vector3Add(circle.position, Vector3Scale(up, 0.1f));
        // Concentric rings
        for (int ri = 0; ri < 3; ++ri) {
            float r = circle.radius * (1.0f - ri * 0.25f);
            Color rc = FadeColor(ringColor, alpha * (0.9f - ri * 0.2f));
            DrawDashedCircle3D(groundPos, r, up, rc);
        }
        // Pentagram (5-pointed star)
        float starR = circle.radius * 0.65f;
        Vector3 rightA = {1, 0, 0}, fwdA = {0, 0, 1};
        if (IsSphericalMap() || circle.world != 0) {
            rightA = SafeNormalize(ProjectOnSphericalTangent(Vector3{1,0,0}, up), Vector3{1,0,0});
            fwdA = SafeNormalize(Vector3CrossProduct(up, rightA), Vector3{0,0,1});
        }
        Vector3 starPts[5];
        for (int vi = 0; vi < 5; ++vi) {
            float angle = 2.0f * 3.14159265f * vi / 5.0f - 3.14159265f / 2.0f;
            starPts[vi] = Vector3Add(groundPos,
                Vector3Add(Vector3Scale(rightA, std::cos(angle) * starR),
                          Vector3Scale(fwdA, std::sin(angle) * starR)));
        }
        Color starColor = FadeColor(circle.isWormhole ? Color{220, 85, 255, 255} : Color{200, 150, 255, 255}, alpha * 0.75f);
        for (int vi = 0; vi < 5; ++vi) {
            int next = (vi + 2) % 5;
            DrawLine3D(starPts[vi], starPts[next], starColor);
        }
        // Small center glow dot
        DrawSphere(groundPos, 0.08f, FadeColor(circle.isWormhole ? Color{210, 70, 255, 255} : Color{220, 200, 255, 255}, alpha * 0.9f));

        // Octahedron wireframe above
        float s = circle.radius * 0.3f;
        float octaHeight = 1.0f + circle.radius * 0.4f;
        Vector3 center = Vector3Add(circle.position, Vector3Scale(up, octaHeight));
        Vector3 vx = {s, 0, 0}, nvx = {-s, 0, 0};
        Vector3 vy = {0, s, 0}, nvy = {0, -s, 0};
        Vector3 vz = {0, 0, s}, nvz = {0, 0, -s};
        if (IsSphericalMap() || circle.world != 0) {
            Vector3 rightA = SafeNormalize(ProjectOnSphericalTangent(Vector3{1,0,0}, up), Vector3{1,0,0});
            Vector3 fwdA = SafeNormalize(Vector3CrossProduct(up, rightA), Vector3{0,0,1});
            vx = Vector3Scale(rightA, s); nvx = Vector3Scale(rightA, -s);
            vy = Vector3Scale(up, s); nvy = Vector3Scale(up, -s);
            vz = Vector3Scale(fwdA, s); nvz = Vector3Scale(fwdA, -s);
        }
        auto p = [&](Vector3 v) { return Vector3Add(center, v); };
        DrawLine3D(p(vx), p(vy), frameColor);
        DrawLine3D(p(vx), p(nvy), frameColor);
        DrawLine3D(p(vx), p(vz), frameColor);
        DrawLine3D(p(vx), p(nvz), frameColor);
        DrawLine3D(p(nvx), p(vy), frameColor);
        DrawLine3D(p(nvx), p(nvy), frameColor);
        DrawLine3D(p(nvx), p(vz), frameColor);
        DrawLine3D(p(nvx), p(nvz), frameColor);
        DrawLine3D(p(vy), p(vz), frameColor);
        DrawLine3D(p(vy), p(nvz), frameColor);
        DrawLine3D(p(nvy), p(vz), frameColor);
        DrawLine3D(p(nvy), p(nvz), frameColor);

        // Core indicator
        if (circle.isWormhole) {
            float pulse = 0.85f + std::sin(static_cast<float>(GetTime()) * 5.4f) * 0.16f;
            DrawSphere(center, config_.wormholeVisualRadius * 0.18f * pulse, FadeColor(Color{25, 0, 45, 255}, 0.92f));
            DrawSphereWires(center, config_.wormholeVisualRadius * pulse, 18, 8, FadeColor(Color{190, 55, 255, 255}, 0.78f));
            DrawDashedCircle3D(center, config_.wormholeVisualRadius * 1.25f * pulse, up, FadeColor(Color{215, 95, 255, 255}, 0.65f));
        } else if (circle.activated) {
            float pulse = 0.7f + std::sin(static_cast<float>(GetTime()) * 7.0f) * 0.3f;
            Color weaponGlow = MagicCircleTint(circle);
            DrawSphere(center, 0.22f + pulse * 0.15f, FadeColor(weaponGlow, 0.85f));
        }
    }

    for (const WormholePortal& portal : wormholes_) {
        Vector3 centers[2] = {portal.frontPosition, portal.backPosition};
        for (int world = 0; world < 2; ++world) {
            Vector3 center = centers[world];
            Vector3 up = UpForWorldAt(center, world);
            float active = playerWorld_ == world ? 1.0f : 0.38f;
            float pulse = 0.9f + std::sin(static_cast<float>(GetTime()) * 4.8f + world * 1.7f) * 0.12f;
            Color outer = FadeColor(Color{170, 45, 255, 255}, active * 0.8f);
            Color inner = FadeColor(Color{35, 0, 65, 255}, active * 0.95f);
            DrawSphere(center, config_.wormholeVisualRadius * 0.28f * pulse, inner);
            DrawSphereWires(center, config_.wormholeVisualRadius * pulse, 20, 10, outer);
            DrawDashedCircle3D(center, config_.wormholeVisualRadius * 1.35f * pulse, up, FadeColor(Color{230, 85, 255, 255}, active * 0.72f));
        }
    }
}
void Game::DrawMysticStaffShield() const {
    if (!mysticStaffShieldActive_) return;
    float pulse = 0.85f + std::sin(static_cast<float>(GetTime()) * 4.5f) * 0.08f;
    Color shieldColor = Color{160, 100, 255, 160};
    DrawSphereWires(mysticStaffShieldPosition_, mysticStaffShieldRadius_ * pulse, 20, 14,
        FadeColor(shieldColor, 0.7f));
    DrawSphere(mysticStaffShieldPosition_, 0.35f, FadeColor(Color{200, 140, 255, 255}, 0.4f));
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
    } else if (activeWeapon_ == WeaponType::LonginusSpear) {
        mode = WeaponVisualMode::LonginusSpear;
    } else if (activeWeapon_ == WeaponType::NanoConstructor) {
        mode = WeaponVisualMode::NanoConstructor;
    } else if (activeWeapon_ == WeaponType::MysticStaff) {
        mode = WeaponVisualMode::MysticStaff;
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
    if (mysticStaffChanneling_) {
        int chargeWidth = static_cast<int>(48.0f * mysticStaffChannelProgress_);
        Color chargeColor = Color{200, 160, 255, 240};
        DrawRectangle(screenWidth / 2 - 24, screenHeight - 24, 48, 4, Color{18, 30, 36, 210});
        DrawRectangle(screenWidth / 2 - 24, screenHeight - 24, chargeWidth, 4, chargeColor);
    }
}
void Game::DrawHud() const {
    for (const DamageNumber& dn : damageNumbers_) {
        Vector2 screen = GetWorldToScreenEx(dn.worldPosition, camera_, pixelWidth_, pixelHeight_);
        int sx = static_cast<int>(screen.x), sy = static_cast<int>(screen.y);
        if (sx > -30 && sx < pixelWidth_ + 30 && sy > -30 && sy < pixelHeight_ + 30) {
            const char* text = TextFormat("%.0f", dn.value);
            float dist = Vector3Distance(camera_.position, dn.worldPosition);
            float fontSize = std::clamp(48.0f / (dist * 0.4f + 1.0f), 7.0f, 14.0f);
            int fs = static_cast<int>(fontSize);
            float alpha = dn.maxLife > 0.0f ? dn.life / dn.maxLife : 0.0f;
            Color c = Color{255, 255, 255, static_cast<unsigned char>(255.0f * alpha)};
            Color shadow = Color{0, 0, 0, static_cast<unsigned char>(200.0f * alpha)};
            int tw = MeasureText(text, fs);
            int drawY = sy - static_cast<int>(dn.screenYOffset);
            DrawText(text, sx - tw / 2 + 1, drawY + 1, fs, shadow);
            DrawText(text, sx - tw / 2 - 1, drawY - 1, fs, shadow);
            DrawText(text, sx - tw / 2, drawY, fs, c);
        }
    }

    // Damage flash overlay
    if (damageFlash_ > 0.0f) {
        unsigned char alpha = static_cast<unsigned char>(damageFlash_ * 80.0f);
        DrawRectangle(0, 0, pixelWidth_, pixelHeight_, Color{220, 40, 20, alpha});
    }

    DrawRectangle(0, 0, pixelWidth_, 22, Color{0, 0, 0, 150});
    DrawText(TextFormat("TIME %.1f", survivalTime_), 6, 7, 8, RAYWHITE);
    DrawText(TextFormat("DMG %.0f", totalDamageDealt_), 72, 7, 8, RAYWHITE);
    DrawText(TextFormat("E %d", static_cast<int>(enemies_.size())), 134, 7, 8, RAYWHITE);
    const char* modeName = WeaponModeName();
    DrawText(TextFormat("W %s%s%s", WeaponName(), modeName[0] != '\0' ? ":" : "", modeName), 174, 7, 8, RAYWHITE);
    Color gravityColor = spaceSuitEnabled_ ? Color{120, 220, 255, 255} : hasSpaceSuit_ ? Color{110, 125, 135, 255} : RAYWHITE;
    DrawText(TextFormat("G %.2fx", gravityScale_), 250, 7, 8, gravityColor);
    const bool hasInactiveGear = (hasFlightRig_ && !flightRigEnabled_) || (hasSkates_ && !skatesEnabled_) || (hasSpaceSuit_ && !spaceSuitEnabled_);
    const char* stateText = timeStopped_ ? "STOP" : config_.invincible ? "GOD" : chargingLaser_ ? "LASER" : flightRigEnabled_ ? "FLIGHT" : skatesEnabled_ ? "SKATE" : spaceSuitEnabled_ ? "SUIT" : hasInactiveGear ? "GEAR" : grounded_ ? "GROUND" : "AIR";
    Color stateColor = timeStopped_ ? Color{190, 160, 255, 255} : config_.invincible ? Color{255, 230, 120, 255} : chargingLaser_ ? Color{120, 220, 255, 255} : flightRigEnabled_ ? Color{160, 245, 255, 255} : skatesEnabled_ ? Color{165, 255, 185, 255} : spaceSuitEnabled_ ? Color{120, 220, 255, 255} : hasInactiveGear ? Color{145, 150, 155, 255} : grounded_ ? Color{190, 255, 190, 255} : Color{180, 220, 255, 255};
    DrawText(stateText, 316, 7, 8, stateColor);
    DrawText(eventTextTimer_ > 0.0f ? eventText_ : WaveLabel(), 6, 29, 8,
        eventTextTimer_ > 0.0f ? Color{255, 220, 135, 255} : Color{180, 180, 180, 255});
    if (DuelMode() && state_ == State::Playing && duelArmor_ > 0) {
        Color armorStatus = duelArmorInvulnTimer_ > 0.0f ? Color{255, 230, 140, 255} : Color{160, 220, 255, 255};
        Color shieldFill = Color{190, 200, 210, 255};
        Color shieldDark = Color{130, 140, 150, 255};
        int ax = 8, ay = 45;
        for (int i = 0; i < duelArmor_; ++i) {
            // Body: rounded rectangle fill + manual outline
            DrawRectangleRounded(Rectangle{static_cast<float>(ax + 1), static_cast<float>(ay), 7.0f, 6.0f}, 0.6f, 5, shieldFill);
            DrawRectangleLinesEx(Rectangle{static_cast<float>(ax + 1), static_cast<float>(ay), 7.0f, 6.0f}, 1.0f, shieldDark);
            // Point: triangle below body
            DrawTriangle(
                Vector2{static_cast<float>(ax + 2), ay + 6.0f},
                Vector2{static_cast<float>(ax + 7), ay + 6.0f},
                Vector2{static_cast<float>(ax + 4.5f), ay + 9.5f},
                shieldFill);
            DrawLineEx(Vector2{static_cast<float>(ax + 2), ay + 6.0f},
                       Vector2{static_cast<float>(ax + 4.5f), ay + 9.5f}, 1.0f, shieldDark);
            DrawLineEx(Vector2{static_cast<float>(ax + 7), ay + 6.0f},
                       Vector2{static_cast<float>(ax + 4.5f), ay + 9.5f}, 1.0f, shieldDark);
            // Status glow outline
            DrawLineEx(Vector2{static_cast<float>(ax + 1), ay + 6.0f},
                       Vector2{static_cast<float>(ax + 2), ay + 6.0f}, 1.5f, armorStatus);
            ax += 12;
        }
    }
    // Essence life hexagram indicator
    if (essence_ > 0 && state_ == State::Playing) {
        int starY = 35; // DuelMode() ? 49 : 39;
        int maxStars = essence_ > 3 ? 3 : essence_;
        float hue = std::fmod(survivalTime_ * 60.0f, 360.0f);
        Color starFill = essenceInvulnTimer_ > 0.0f
            ? Color{255, 150, 60, 255}
            : ColorFromHSV(hue, 0.85f, 1.0f);
        for (int i = 0; i < maxStars; ++i) {
            int cx = 370 + i * 16;
            int cy = starY;
            float s = 6.5f;
            float c30 = 0.8660254f;
            float s30 = 0.5f;
            Vector2 upT   = {static_cast<float>(cx), cy - s};
            Vector2 upBL  = {cx - s * c30, cy + s * s30};
            Vector2 upBR  = {cx + s * c30, cy + s * s30};
            Vector2 dnB   = {static_cast<float>(cx), cy + s};
            Vector2 dnTL  = {cx - s * c30, cy - s * s30};
            Vector2 dnTR  = {cx + s * c30, cy - s * s30};

            DrawTriangle(upT, upBL, upBR, starFill);
            DrawTriangle(dnB, dnTR, dnTL, starFill);
            DrawTriangleLines(upT, upBL, upBR, FadeColor(Color{30, 15, 0, 255}, 0.45f));
            DrawTriangleLines(dnB, dnTR, dnTL, FadeColor(Color{30, 15, 0, 255}, 0.45f));
        }
        if (essence_ > 3) {
            DrawText(TextFormat("+%d", essence_ - 3), 363 + maxStars * 16, starY - 4, 8, starFill);
        }
    }
    if (activeWeapon_ == WeaponType::InfinityGauntlet && gauntletMode_ == GauntletMode::Blink) {
        const char* blinkText = TextFormat("BLINK %.1fm", config_.blinkDistance * blinkDistanceScale_);
        DrawText(blinkText, 100, 117, 8, Color{190, 160, 255, 255});
    }
    if (activeWeapon_ == WeaponType::MysticStaff && mysticStaffMode_ == MysticStaffMode::Shield
        && mysticStaffShieldCooldown_ > 0.0f) {
        const char* cdText = TextFormat("SHIELD CD %.1fs", mysticStaffShieldCooldown_);
        DrawText(cdText, 100, 117, 8, Color{180, 140, 255, 255});
    }
    // Right-edge indicator: FPS normally, world toggle after wormhole activation
    if (!wormholes_.empty() || playerWorld_ != 0) {
        const char* worldText = playerWorld_ == 0 ? "FRONT" : "BACK";
        Color worldColor = playerWorld_ == 0 ? Color{180, 190, 210, 255} : Color{205, 110, 255, 255};
        DrawText(worldText, pixelWidth_ - MeasureText(worldText, 8) - 6, 7, 8, worldColor);
    } else {
        const char* fpsText = TextFormat("FPS %d", GetFPS());
        DrawText(fpsText, pixelWidth_ - MeasureText(fpsText, 8) - 6, 7, 8, Color{170, 230, 170, 255});
    }

    if (bethlehem_.active) {
        float bh = bethlehem_.maxHealth > 0.0f ? std::clamp(bethlehem_.health / bethlehem_.maxHealth, 0.0f, 1.0f) : 0.0f;
        int barX = 76, barY = 29, barW = 270;
        DrawRectangle(barX, barY, barW, 6, Color{18, 10, 5, 220});
        Color bethFill = TutorialMode() ? Color{140, 140, 155, 255} : Color{255, 190, 60, 255};
        Color bethBorder = TutorialMode() ? Color{180, 180, 195, 210} : Color{255, 220, 140, 210};
        Color bethLabel = TutorialMode() ? Color{180, 180, 195, 255} : Color{255, 225, 150, 255};
        DrawRectangle(barX, barY, static_cast<int>(barW * bh), 6, bethFill);
        DrawRectangleLines(barX, barY, barW, 6, bethBorder);
        DrawText("STAR OF BETHLEHEM", barX, barY + 9, 8, bethLabel);
    }

    // Collect boss-type enemies with their health data
    struct BossBarEntry { const Enemy* enemy; float health; float lastTime; };
    BossBarEntry bossEntries[16];
    int bossCount = 0;
    for (const Enemy& enemy : enemies_) {
        if (enemy.type != EnemyType::Boss && enemy.type != EnemyType::Duelist
            && enemy.type != EnemyType::DummyBoss && enemy.type != EnemyType::SlimeKing) continue;
        if (bossCount < 16) {
            bossEntries[bossCount++] = {&enemy,
                enemy.maxHealth > 0.0f ? std::clamp(enemy.health / enemy.maxHealth, 0.0f, 1.0f) : 0.0f,
                enemy.lastDamageTime};
        }
    }
    // Sort by lastDamageTime descending (most recently hit first)
    for (int i = 0; i < bossCount - 1; ++i)
        for (int j = i + 1; j < bossCount; ++j)
            if (bossEntries[j].lastTime > bossEntries[i].lastTime)
                std::swap(bossEntries[i], bossEntries[j]);

    // Draw up to 3 bars, starting below Bethlehem if present
    int barX = 76, barW = 270;
    int bossBarY0 = bethlehem_.active ? 52 : 29;
    for (int slot = 0; slot < bossCount && slot < 3; ++slot) {
        const Enemy& enemy = *bossEntries[slot].enemy;
        float health = bossEntries[slot].health;
        int barY = bossBarY0 + slot * 18;
        DrawRectangle(barX, barY, barW, 6, Color{18, 10, 30, 220});
        Color barColor = enemy.type == EnemyType::Duelist ? Color{255, 210, 105, 255}
            : enemy.type == EnemyType::DummyBoss ? Color{140, 140, 155, 255}
            : enemy.type == EnemyType::SlimeKing ? Color{100, 220, 140, 255}
            : health < 0.45f ? Color{255, 75, 160, 255}
            : Color{160, 115, 255, 255};
        DrawRectangle(barX, barY, static_cast<int>(barW * health), 6, barColor);
        Color borderColor = enemy.type == EnemyType::DummyBoss ? Color{180, 180, 195, 210} : Color{230, 210, 255, 210};
        DrawRectangleLines(barX, barY, barW, 6, borderColor);
        const char* bossLabel;
        Color labelColor;
        if (enemy.type == EnemyType::Duelist) {
            const char* duelistWeapon = "LASER";
            if (enemy.weaponSlot == 1) duelistWeapon = "FLAME";
            else if (enemy.weaponSlot == 2) duelistWeapon = "ROCKET";
            else if (enemy.weaponSlot == 3) duelistWeapon = "SHOT";
            else if (enemy.weaponSlot == 4) duelistWeapon = "NAIL";
            else if (enemy.weaponSlot == 5) duelistWeapon = "GAUNT";
            else if (enemy.weaponSlot == 6) duelistWeapon = "SPEAR";
            else if (enemy.weaponSlot == 7) duelistWeapon = "NANO";
            else if (enemy.weaponSlot == 8) duelistWeapon = "STAFF";
            if (enemy.aiTier >= 1) {
                bossLabel = TextFormat("DUELIST* %s", duelistWeapon);
                labelColor = Color{255, 180, 60, 255};
            } else {
                bossLabel = TextFormat("DUELIST %s", duelistWeapon);
                labelColor = Color{255, 230, 150, 255};
            }
        } else if (enemy.type == EnemyType::DummyBoss) {
            bossLabel = "DUMMY LORD";
            labelColor = Color{180, 180, 195, 255};
        } else if (enemy.type == EnemyType::SlimeKing) {
            bossLabel = enemy.slimeGeneration == 0 ? "SLIME KING" : TextFormat("SLIME KING x%d", enemy.slimeGeneration + 1);
            labelColor = Color{140, 240, 180, 255};
        } else {
            bossLabel = "GEOMETRY LORD";
            labelColor = Color{230, 210, 255, 255};
        }
        DrawText(bossLabel, barX, barY + 9, 8, labelColor);
    }

    // If >3 boss-type enemies, show overflow hint
    if (bossCount > 3) {
        DrawText(TextFormat("+%d more", bossCount - 3), barX, bossBarY0 + 3 * 18 + 9, 7, Color{160, 155, 170, 220});
    }

    if (showKeybindOverlay_) {
        int px = 40, py = 58, fh = 8;
        int panelW = 346, panelH = 146;
        DrawRectangle(px - 8, py - 8, panelW, panelH, Color{0, 0, 0, 210});
        DrawRectangleLines(px - 8, py - 8, panelW, panelH, Color{140, 140, 160, 180});
        DrawText("CONTROLS  (K to close)", px + 80, py - 2, fh + 1, Color{220, 220, 240, 255});
        py += 16;

        auto Row = [&](const char* key, const char* desc, Color kc, int x) {
            DrawText(key, x, py, fh, kc);
            DrawText(desc, x + 66, py, fh, Color{190, 190, 205, 255});
            py += fh + 2;
        };
        auto Section = [&](const char* title, int x) {
            DrawText(title, x, py, fh, Color{120, 180, 220, 255});
            py += fh + 3;
        };
        Color k = Color{255, 225, 160, 255};

        int left = px, right = px + 178;
        int startPy = py;
        Section("MOVEMENT", left);
        Row("W A S D", "Move", k, left);
        Row("Space", "Jump / Fly up", k, left);
        Row("Shift", "Run", k, left);
        Row("Ctrl", "Fly down", k, left);
        py += 8;
        Section("EQUIPMENT", left);
        Row("Z", "Space suit", k, left);
        Row("X", "Flight rig", k, left);
        Row("C", "Skates", k, left);

        py = startPy;
        Section("WEAPONS", right);
        Row("0 - 8", "Select weapon", k, right);
        Row("LMB", "Fire", k, right);
        Row("RMB", "Alt / mode toggle", k, right);
        Row("Wheel", "Switch / adjust", k, right);
        py += 4;
        Section("SYSTEM", right);
        Row("P", "Hide HUD", k, right);
        Row("R", "Restart", k, right);
        Row("F11", "Fullscreen", k, right);
        Row("~", "Console", k, right);
        Row("K", "Keybindings", k, right);
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

    if (tutorialTipTimer_ > 0.0f && tutorialTip_[0] != '\0') {
        float elapsed = tutorialTipDuration_ - tutorialTipTimer_;
        float fadeIn = elapsed < 0.4f ? elapsed / 0.4f : 1.0f;
        float fadeOut = tutorialTipTimer_ < 0.6f ? tutorialTipTimer_ / 0.6f : 1.0f;
        float alpha = fadeIn * fadeOut;
        Color tipColor = Color{255, 230, 140, static_cast<unsigned char>(255.0f * alpha)};
        Color barColor = Color{0, 0, 0, static_cast<unsigned char>(90.0f * alpha)};
        if (cjkFontLoaded_) {
            constexpr float kFontSize = 12.0f;
            constexpr float kLineGap = 5.0f;
            constexpr float kBarPad = 12.0f;
            // Split on '\n'
            const char* line1 = tutorialTip_;
            const char* line2 = strchr(tutorialTip_, '\n');
            char line2Buf[256] = {};
            if (line2) {
                size_t len1 = line2 - line1;
                if (len1 < sizeof(line2Buf)) {
                    memcpy(line2Buf, line1, len1);
                    line2Buf[len1] = '\0';
                    line1 = line2Buf;
                }
                line2 = line2 + 1;
            }
            float w1 = MeasureTextEx(cjkFont_, line1, kFontSize, 1.0f).x;
            float w2 = line2 ? MeasureTextEx(cjkFont_, line2, kFontSize, 1.0f).x : 0.0f;
            float barH = line2 ? kBarPad * 2 + kFontSize * 2 + kLineGap : kBarPad * 2 + kFontSize;
            float barY = static_cast<float>(pixelHeight_) - barH;
            DrawRectangle(0, static_cast<int>(barY), pixelWidth_, static_cast<int>(barH), barColor);
            float y1 = barY + kBarPad;
            DrawTextEx(cjkFont_, line1, Vector2{pixelWidth_ / 2.0f - w1 / 2.0f, y1}, kFontSize, 1.0f, tipColor);
            if (line2) {
                DrawTextEx(cjkFont_, line2, Vector2{pixelWidth_ / 2.0f - w2 / 2.0f, y1 + kFontSize + kLineGap}, kFontSize, 1.0f, tipColor);
            }
        } else {
            DrawRectangle(0, pixelHeight_ - 38, pixelWidth_, 38, barColor);
            int tw = MeasureText(tutorialTip_, 9);
            DrawText(tutorialTip_, pixelWidth_ / 2 - tw / 2, pixelHeight_ - 28, 9, tipColor);
        }
    }

    if (TutorialMode() && !hideUI_ && !showKeybindOverlay_) {
        const char* kHint = "F11: Fullscreen\nK: Controls";
        int kw = MeasureText(kHint, 9);
        int kx = pixelWidth_ - kw - 8, ky = pixelHeight_ - 120;
        DrawRectangle(kx - 3, ky - 1, kw + 6, 11, Color{0, 0, 0, 140});
        DrawText(kHint, kx, ky, 9, Color{255, 235, 140, 230});
    }
}
