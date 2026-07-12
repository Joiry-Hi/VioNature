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

void Game::DrawScavengerUfo() const {
    if (scavengerUfo_.state != ScavengerUfoState::Active
        && scavengerUfo_.state != ScavengerUfoState::Escaping
        && scavengerUfo_.state != ScavengerUfoState::DefeatedFalling
        && scavengerUfo_.state != ScavengerUfoState::DefeatedLanded
        && scavengerUfo_.state != ScavengerUfoState::ParkedHover) {
        return;
    }

    Vector3 up = UpForWorldAt(scavengerUfo_.position, 0);
    Vector3 right = SafeNormalize(Vector3CrossProduct(Vector3{0.0f, 0.0f, 1.0f}, up), Vector3{1.0f, 0.0f, 0.0f});
    Vector3 forward = SafeNormalize(Vector3CrossProduct(up, right), Vector3{0.0f, 0.0f, 1.0f});

    rlPushMatrix();
    Matrix basis = {
        right.x, up.x, forward.x, scavengerUfo_.position.x,
        right.y, up.y, forward.y, scavengerUfo_.position.y,
        right.z, up.z, forward.z, scavengerUfo_.position.z,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    rlMultMatrixf(MatrixToFloat(basis));

    if (scavengerUfoModelLoaded_) {
        Color tint = (scavengerUfo_.state == ScavengerUfoState::DefeatedFalling
                || scavengerUfo_.state == ScavengerUfoState::DefeatedLanded
                || scavengerUfo_.state == ScavengerUfoState::ParkedHover)
            ? Color{150, 160, 165, 255}
            : WHITE;
        DrawModel(scavengerUfoModel_, Vector3Zero(), 1.3f, tint);
    } else {
        Color body = (scavengerUfo_.state == ScavengerUfoState::DefeatedFalling
                || scavengerUfo_.state == ScavengerUfoState::DefeatedLanded
                || scavengerUfo_.state == ScavengerUfoState::ParkedHover)
            ? Color{115, 125, 128, 255}
            : Color{170, 185, 190, 255};
        DrawCube(Vector3Zero(), 7.2f, 1.6f, 7.2f, body);
        DrawCubeWires(Vector3Zero(), 7.4f, 1.8f, 7.4f, Color{60, 75, 82, 220});
        DrawSphereEx(Vector3{0.0f, 1.1f, 0.0f}, 1.9f, 10, 6, FadeColor(Color{120, 235, 255, 255}, 0.78f));
    }
    rlPopMatrix();

    if (scavengerUfo_.tractoring
        && scavengerUfo_.targetPickupIndex >= 0
        && scavengerUfo_.targetPickupIndex < static_cast<int>(pickups_.size())) {
        Vector3 underside = Vector3Subtract(scavengerUfo_.position, Vector3Scale(up, 0.62f));
        Vector3 target = pickups_[scavengerUfo_.targetPickupIndex].position;
        DrawCylinderEx(underside, target, 1.2f, 0.22f, 18, FadeColor(Color{120, 230, 255, 255}, 0.24f));
        DrawCylinderWiresEx(underside, target, 1.25f, 0.25f, 18, FadeColor(Color{170, 245, 255, 255}, 0.45f));
    }

    if (scavengerUfo_.state == ScavengerUfoState::Escaping) {
        Vector3 top = Vector3Add(scavengerUfo_.position, Vector3Scale(up, 18.0f));
        DrawCylinderEx(scavengerUfo_.position, top, 1.2f, 0.2f, 18, FadeColor(Color{150, 235, 255, 255}, 0.32f));
    }
}

void Game::DrawThroneAngel() const {
    if (!throneAngel_.active && !throneAngel_.defeated) return;
    Vector3 up = UpForWorldAt(throneAngel_.position, 0);
    Vector3 toPlayer = SafeNormalize(Vector3Subtract(camera_.position, throneAngel_.position), Vector3Scale(up, -1.0f));
    Vector3 right = SafeNormalize(Vector3CrossProduct(toPlayer, up), Vector3{1.0f, 0.0f, 0.0f});
    Vector3 eyeUp = SafeNormalize(Vector3CrossProduct(right, toPlayer), up);
    float flash = std::clamp(throneAngel_.hitFlash, 0.0f, 1.0f);
    auto lerpColor = [](Color a, Color b, float t) {
        t = std::clamp(t, 0.0f, 1.0f);
        return Color{
            static_cast<unsigned char>(static_cast<float>(a.r) + (static_cast<float>(b.r) - static_cast<float>(a.r)) * t),
            static_cast<unsigned char>(static_cast<float>(a.g) + (static_cast<float>(b.g) - static_cast<float>(a.g)) * t),
            static_cast<unsigned char>(static_cast<float>(a.b) + (static_cast<float>(b.b) - static_cast<float>(a.b)) * t),
            static_cast<unsigned char>(static_cast<float>(a.a) + (static_cast<float>(b.a) - static_cast<float>(a.a)) * t)
        };
    };
    float ladderProgress = throneAngel_.defeated
        ? std::clamp(throneAngel_.jacobLadderTimer / std::max(0.001f, config_.throneJacobLadderOpenTime), 0.0f, 1.0f)
        : 0.0f;
    ladderProgress = ladderProgress * ladderProgress * (3.0f - 2.0f * ladderProgress);
    Color shell = flash > 0.0f ? Color{255, 255, 255, 255}
        : throneAngel_.defeated
            ? lerpColor(Color{236, 240, 246, 255}, Color{255, 208, 0, 255}, ladderProgress)
            : Color{236, 240, 246, 255};

    if (ThroneJacobLadderActive()) {
        float lengthScale = 0.34f + 0.66f * ladderProgress;
        float radiusOpen = 0.18f + 0.82f * ladderProgress;
        float alphaOpen = 0.12f + 0.88f * ladderProgress;
        Vector3 bottom = Vector3Subtract(throneAngel_.position, Vector3Scale(up, config_.throneJacobLadderBeamLength * lengthScale));
        struct BeamLayer {
            float radiusScale;
            float alpha;
            Color color;
            int slices;
        };
        const BeamLayer layers[] = {
            {1.72f, 0.07f, Color{255, 246, 206, 255}, 36},
            {1.34f, 0.14f, Color{255, 226, 146, 255}, 34},
            {1.00f, 0.25f, Color{255, 204, 84, 255}, 32},
            {0.64f, 0.40f, Color{255, 212, 20, 255}, 28},
            {0.34f, 0.72f, Color{255, 242, 0, 255}, 24},
        };
        rlDrawRenderBatchActive();
        rlDisableDepthTest();
        for (const BeamLayer& layer : layers) {
            DrawCylinderEx(throneAngel_.position, bottom,
                config_.throneJacobLadderTopRadius * layer.radiusScale * radiusOpen,
                config_.throneJacobLadderBottomRadius * layer.radiusScale * radiusOpen,
                layer.slices,
                FadeColor(layer.color, layer.alpha * alphaOpen));
        }
        rlDrawRenderBatchActive();
        rlEnableDepthTest();
        for (int i = 0; i < 4; ++i) {
            float phase = throneAngel_.ringAngles[i % 3] + static_cast<float>(i) * 1.57f;
            float t = 0.18f + static_cast<float>(i) * 0.18f;
            Vector3 center = Vector3Subtract(throneAngel_.position, Vector3Scale(up, config_.throneJacobLadderBeamLength * lengthScale * t));
            float radius = config_.throneJacobLadderTopRadius
                + (config_.throneJacobLadderBottomRadius - config_.throneJacobLadderTopRadius) * t;
            Vector3 normal = RotateAroundAxis(up, toPlayer, phase);
            DrawCircle3D(center, radius * radiusOpen, normal, 90.0f, FadeColor(Color{255, 174, 0, 255}, 0.46f * alphaOpen));
        }
    }

    DrawSphereEx(throneAngel_.position, 2.25f, 18, 14, shell);
    DrawSphereWires(throneAngel_.position, 2.34f, 18, 12, FadeColor(Color{255, 255, 255, 255}, 0.55f));
    Vector3 pupil = Vector3Add(throneAngel_.position, Vector3Scale(toPlayer, 2.05f));
    DrawSphereEx(pupil, 0.66f, 12, 8, throneAngel_.defeated
        ? lerpColor(Color{5, 6, 10, 255}, Color{255, 242, 0, 255}, ladderProgress)
        : Color{5, 6, 10, 255});
    DrawSphereEx(Vector3Add(pupil, Vector3Add(Vector3Scale(right, -0.16f), Vector3Scale(eyeUp, 0.18f))), 0.13f, 6, 4,
        throneAngel_.defeated ? Color{255, 255, 185, 245} : Color{255, 255, 255, 220});

    for (int i = 0; i < 3; ++i) {
        float radius = 3.3f + static_cast<float>(i) * 1.45f;
        Vector3 axis = SafeNormalize(throneAngel_.ringAxes[i], up);
        Vector3 ringNormal = RotateAroundAxis(axis, toPlayer, throneAngel_.ringAngles[i]);
        Color ringColor = throneAngel_.defeated
            ? lerpColor((i == 0 ? Color{245, 248, 255, 220} : Color{205, 220, 245, 150}),
                (i == 0 ? Color{255, 190, 0, 235} : Color{255, 150, 0, 175}), ladderProgress)
            : (i == 0 ? Color{245, 248, 255, 220} : Color{205, 220, 245, 150});
        DrawCircle3D(throneAngel_.position, radius, ringNormal, 90.0f, ringColor);
        DrawCircle3D(throneAngel_.position, radius + 0.08f, ringNormal, 90.0f,
            FadeColor(throneAngel_.defeated ? Color{255, 190, 0, 255} : Color{255, 255, 255, 255}, 0.28f));
    }
}

void Game::DrawCherubs() const {
    for (const CherubMinion& cherub : cherubs_) {
        Vector3 up = UpForWorldAt(cherub.position, cherub.world);
        Vector3 forward = SafeNormalize(Vector3Subtract(camera_.position, cherub.position), Vector3Scale(up, -1.0f));
        Vector3 right = SafeNormalize(Vector3CrossProduct(forward, up), Vector3{1.0f, 0.0f, 0.0f});
        Vector3 ringUp = SafeNormalize(Vector3CrossProduct(right, forward), up);
        Color body = cherub.flashTimer > 0.0f ? Color{255, 255, 255, 255} : Color{238, 242, 248, 255};
        DrawSphereEx(cherub.position, 0.48f, 10, 8, body);
        float orbitTilt = 0.62f + std::sin(cherub.wingTimer * 0.31f) * 0.16f;
        Vector3 orbitAxis = SafeNormalize(Vector3Add(Vector3Scale(forward, std::cos(orbitTilt)), Vector3Scale(ringUp, std::sin(orbitTilt))), forward);
        Vector3 ringNormal = RotateAroundAxis(up, orbitAxis, cherub.wingTimer * 0.28f);
        Color ring = cherub.flashTimer > 0.0f ? Color{255, 255, 255, 245} : Color{232, 240, 255, 205};
        DrawCircle3D(cherub.position, 0.92f, ringNormal, 90.0f, ring);
        DrawCircle3D(cherub.position, 0.99f, ringNormal, 90.0f, FadeColor(Color{255, 255, 255, 255}, 0.22f));
        DrawSphereWires(cherub.position, 0.58f, 8, 6, FadeColor(Color{255, 255, 255, 255}, 0.35f));
    }
}

void Game::DrawSeraph() const {
    if (seraphs_.empty()) return;
    for (const SeraphBoss& seraph : seraphs_) {
        if (!seraph.active) continue;
    Vector3 up = UpForWorldAt(seraph.position, seraph.world);
    Vector3 toPlayer = SafeNormalize(Vector3Subtract(camera_.position, seraph.position), Vector3Scale(up, -1.0f));
    Vector3 right = SafeNormalize(Vector3CrossProduct(toPlayer, up), Vector3{1.0f, 0.0f, 0.0f});
    Vector3 wingUp = SafeNormalize(Vector3CrossProduct(right, toPlayer), up);
    float attack = std::clamp(seraph.attackFlash, 0.0f, 1.0f);
    float flash = std::clamp(seraph.hitFlash, 0.0f, 1.0f);
    float coreRadius = 1.62f + attack * 0.30f;
    Color core = flash > 0.0f ? Color{255, 255, 255, 255} : Color{255, 235, 150, 255};
    DrawSphereEx(seraph.position, coreRadius, 16, 12, core);
    DrawSphereWires(seraph.position, coreRadius * 1.14f, 16, 10, FadeColor(Color{255, 245, 190, 255}, 0.65f));

    float flap = std::sin(seraph.wingTimer) * 0.22f + attack * 0.36f;
    float wingBeat = std::sin(seraph.wingTimer * 1.35f) * (0.34f + attack * 0.22f);
    Color wingOuter = FadeColor(Color{255, 210, 95, 255}, 0.72f);
    Color wingInner = FadeColor(Color{255, 250, 210, 255}, 0.86f);
    for (int side = -1; side <= 1; side += 2) {
        for (int row = 0; row < 3; ++row) {
            float spread = static_cast<float>(row - 1) * 0.80f;
            Vector3 radial = SafeNormalize(Vector3Add(Vector3Scale(right, static_cast<float>(side)), Vector3Scale(wingUp, spread)), right);
            Vector3 tangent = SafeNormalize(Vector3Subtract(Vector3Scale(wingUp, 1.0f), Vector3Scale(right, side * spread)), wingUp);
            float pulse = 0.5f + 0.5f * std::sin(seraph.wingTimer * 1.7f + static_cast<float>(row) * 0.9f + side * 0.35f);
            float mainWing = row == 1 ? 1.0f : 0.0f;
            float innerRadius = coreRadius * 1.30f;
            float outerRadius = (mainWing > 0.0f ? 7.05f : 5.92f) + flap + pulse * 0.24f;
            float halfWidth = (mainWing > 0.0f ? 1.52f : 1.14f) + attack * 0.32f + pulse * 0.11f;
            float beat = wingBeat * static_cast<float>(side) * (mainWing > 0.0f ? 1.0f : 0.78f);
            Vector3 depth = Vector3Scale(toPlayer, -0.16f - row * 0.04f + beat);

            Vector3 innerTip = Vector3Add(seraph.position, Vector3Add(Vector3Scale(radial, innerRadius), depth));
            Vector3 outerTip = Vector3Add(seraph.position, Vector3Add(Vector3Scale(radial, outerRadius), depth));
            Vector3 mid = Vector3Add(seraph.position, Vector3Add(Vector3Scale(radial, innerRadius + (outerRadius - innerRadius) * 0.48f), depth));
            Vector3 sideA = Vector3Add(mid, Vector3Scale(tangent, halfWidth));
            Vector3 sideB = Vector3Subtract(mid, Vector3Scale(tangent, halfWidth));

            DrawTriangle3D(innerTip, sideA, outerTip, wingOuter);
            DrawTriangle3D(innerTip, outerTip, sideB, wingOuter);
            DrawTriangle3D(innerTip, outerTip, sideA, wingOuter);
            DrawTriangle3D(innerTip, sideB, outerTip, wingOuter);

            Vector3 glowInner = Vector3Add(seraph.position, Vector3Add(Vector3Scale(radial, innerRadius + 0.28f), depth));
            Vector3 glowOuter = Vector3Add(seraph.position, Vector3Add(Vector3Scale(radial, outerRadius - 0.40f), depth));
            Vector3 glowMid = Vector3Add(seraph.position, Vector3Add(Vector3Scale(radial, innerRadius + (outerRadius - innerRadius) * 0.50f), depth));
            Vector3 glowSideA = Vector3Add(glowMid, Vector3Scale(tangent, halfWidth * 0.46f));
            Vector3 glowSideB = Vector3Subtract(glowMid, Vector3Scale(tangent, halfWidth * 0.46f));
            DrawTriangle3D(glowInner, glowSideA, glowOuter, wingInner);
            DrawTriangle3D(glowInner, glowOuter, glowSideB, wingInner);
        }
    }
    if (attack > 0.0f) {
        DrawSphereWires(seraph.position, 3.2f + attack * 1.4f, 18, 10, FadeColor(Color{255, 235, 140, 255}, attack * 0.7f));
    }
    }
}

void Game::DrawWarRider() const {
    if (!warRider_.active) return;
    constexpr float kHorsemanVisualScale = 2.0f;
    Vector3 up = UpForWorldAt(warRider_.position, warRider_.world);
    Vector3 forward = warRider_.forward;
    if (IsSphericalMap()) {
        forward = SafeNormalize(ProjectOnSphericalTangent(forward, up), PlayerForward());
    } else {
        forward.y = 0.0f;
        forward = SafeNormalize(forward, Vector3{0.0f, 0.0f, 1.0f});
    }
    Vector3 right = SafeNormalize(Vector3CrossProduct(up, forward), Vector3{1.0f, 0.0f, 0.0f});
    float flash = std::clamp(warRider_.hitFlash, 0.0f, 1.0f);
    float charge = warRider_.chargeTimeLeft > 0.0f ? 1.0f : 0.0f;
    Color wood = flash > 0.0f ? Color{255, 228, 188, 255} : Color{158, 91, 43, 255};
    Color woodLight = flash > 0.0f ? Color{255, 245, 218, 255} : Color{214, 145, 76, 255};
    Color woodDark = Color{72, 43, 24, 255};
    Color flame = FadeColor(Color{255, 66, 34, 255}, 0.72f + charge * 0.18f);

    rlPushMatrix();
    rlTranslatef(warRider_.position.x, warRider_.position.y, warRider_.position.z);
    rlScalef(kHorsemanVisualScale, kHorsemanVisualScale, kHorsemanVisualScale);
    rlTranslatef(-warRider_.position.x, -warRider_.position.y, -warRider_.position.z);

    auto drawBox = [&](Vector3 center, float halfRight, float halfUp, float halfForward, Color fill, Color edgeColor) {
        Vector3 r = Vector3Scale(right, halfRight);
        Vector3 u = Vector3Scale(up, halfUp);
        Vector3 f = Vector3Scale(forward, halfForward);
        Vector3 p[8] = {
            Vector3Subtract(Vector3Subtract(Vector3Subtract(center, r), u), f),
            Vector3Subtract(Vector3Subtract(Vector3Add(center, r), u), f),
            Vector3Add(Vector3Subtract(Vector3Add(center, r), u), f),
            Vector3Add(Vector3Subtract(Vector3Subtract(center, r), u), f),
            Vector3Subtract(Vector3Add(Vector3Subtract(center, r), u), f),
            Vector3Subtract(Vector3Add(Vector3Add(center, r), u), f),
            Vector3Add(Vector3Add(Vector3Add(center, r), u), f),
            Vector3Add(Vector3Add(Vector3Subtract(center, r), u), f),
        };
        auto quad = [&](int a, int b, int c, int d) {
            DrawTriangle3D(p[a], p[b], p[c], fill);
            DrawTriangle3D(p[a], p[c], p[d], fill);
        };
        quad(0, 1, 2, 3);
        quad(4, 7, 6, 5);
        quad(0, 4, 5, 1);
        quad(1, 5, 6, 2);
        quad(2, 6, 7, 3);
        quad(3, 7, 4, 0);
        int edges[][2] = {{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
        for (auto& e : edges) DrawLine3D(p[e[0]], p[e[1]], edgeColor);
    };

    Vector3 body = Vector3Add(warRider_.position, Vector3Scale(up, 1.08f));
    drawBox(body, 0.62f, 0.48f, 1.72f, wood, FadeColor(woodLight, 0.78f));

    for (float plank : {-0.52f, 0.0f, 0.52f}) {
        Vector3 a = Vector3Add(body, Vector3Add(Vector3Scale(forward, plank), Vector3Scale(up, 0.50f)));
        DrawLine3D(Vector3Subtract(a, Vector3Scale(right, 0.61f)), Vector3Add(a, Vector3Scale(right, 0.61f)), FadeColor(woodDark, 0.58f));
    }
    for (int side = -1; side <= 1; side += 2) {
        Vector3 sideCenter = Vector3Add(body, Vector3Scale(right, side * 0.65f));
        DrawLine3D(Vector3Subtract(sideCenter, Vector3Scale(forward, 1.58f)), Vector3Add(sideCenter, Vector3Scale(forward, 1.58f)), FadeColor(woodLight, 0.7f));
    }

    Vector3 neck = Vector3Add(body, Vector3Add(Vector3Scale(forward, 1.45f), Vector3Scale(up, 0.56f)));
    drawBox(neck, 0.34f, 0.68f, 0.34f, woodLight, FadeColor(woodDark, 0.78f));
    Vector3 head = Vector3Add(body, Vector3Add(Vector3Scale(forward, 2.06f), Vector3Scale(up, 0.82f)));
    drawBox(head, 0.44f, 0.34f, 0.58f, wood, FadeColor(woodLight, 0.7f));
    Vector3 nose = Vector3Add(head, Vector3Scale(forward, 0.78f));
    Vector3 muzzleTop = Vector3Add(head, Vector3Scale(up, 0.18f));
    Vector3 muzzleBottom = Vector3Subtract(head, Vector3Scale(up, 0.22f));
    DrawTriangle3D(nose, Vector3Add(muzzleTop, Vector3Scale(right, 0.30f)), Vector3Subtract(muzzleTop, Vector3Scale(right, 0.30f)), woodLight);
    DrawTriangle3D(nose, Vector3Subtract(muzzleTop, Vector3Scale(right, 0.30f)), muzzleBottom, woodDark);
    Vector3 earBase = Vector3Add(head, Vector3Scale(up, 0.35f));
    for (int side = -1; side <= 1; side += 2) {
        Vector3 earRoot = Vector3Add(earBase, Vector3Scale(right, side * 0.24f));
        DrawTriangle3D(earRoot,
            Vector3Add(earRoot, Vector3Add(Vector3Scale(forward, 0.10f), Vector3Scale(up, 0.46f))),
            Vector3Add(earRoot, Vector3Scale(right, side * 0.20f)),
            woodLight);
    }

    for (int side = -1; side <= 1; side += 2) {
        for (int pair = -1; pair <= 1; pair += 2) {
            Vector3 leg = Vector3Add(warRider_.position,
                Vector3Add(Vector3Scale(right, side * 0.48f), Vector3Add(Vector3Scale(forward, pair * 1.04f), Vector3Scale(up, 0.54f))));
            drawBox(leg, 0.13f, 0.55f, 0.15f, woodDark, FadeColor(woodLight, 0.48f));
        }
    }
    for (int side = -1; side <= 1; side += 2) {
        Vector3 skid = Vector3Add(warRider_.position, Vector3Add(Vector3Scale(right, side * 0.50f), Vector3Scale(up, 0.06f)));
        drawBox(skid, 0.13f, 0.08f, 1.92f, woodDark, FadeColor(woodLight, 0.56f));
    }
    Vector3 frontCross = Vector3Add(warRider_.position, Vector3Add(Vector3Scale(forward, 1.20f), Vector3Scale(up, 0.22f)));
    Vector3 backCross = Vector3Add(warRider_.position, Vector3Add(Vector3Scale(forward, -1.20f), Vector3Scale(up, 0.22f)));
    drawBox(frontCross, 0.78f, 0.08f, 0.10f, woodDark, FadeColor(woodLight, 0.48f));
    drawBox(backCross, 0.78f, 0.08f, 0.10f, woodDark, FadeColor(woodLight, 0.48f));

    Vector3 rider = Vector3Add(body, Vector3Scale(up, 1.35f));
    DrawSphereEx(rider, 0.68f + charge * 0.12f, 4, 4, flash > 0.0f ? WHITE : Color{205, 32, 42, 255});
    DrawSphereWires(rider, 1.0f + charge * 0.28f, 8, 6, flame);
    Vector3 crown = Vector3Add(rider, Vector3Scale(up, 0.84f));
    DrawCylinderWires(crown, 0.55f, 0.55f, 0.08f, 5, FadeColor(Color{255, 190, 80, 255}, 0.85f));

    Vector3 swordGrip = Vector3Add(rider, Vector3Add(Vector3Scale(right, 0.72f), Vector3Scale(up, 0.06f)));
    Vector3 swordGuardLeft = Vector3Subtract(swordGrip, Vector3Scale(right, 0.42f));
    Vector3 swordGuardRight = Vector3Add(swordGrip, Vector3Scale(right, 0.42f));
    Vector3 swordBase = Vector3Add(swordGrip, Vector3Scale(forward, 0.42f));
    Vector3 swordTip = Vector3Add(swordGrip, Vector3Add(Vector3Scale(forward, 2.85f), Vector3Scale(up, 0.82f)));
    Vector3 bladeSide = Vector3Scale(right, 0.16f);
    DrawCylinderEx(Vector3Subtract(swordGrip, Vector3Scale(up, 0.42f)), swordGrip, 0.075f, 0.055f, 5, Color{80, 24, 20, 255});
    DrawCylinderEx(swordGuardLeft, swordGuardRight, 0.045f, 0.045f, 5, Color{255, 168, 72, 255});
    DrawTriangle3D(Vector3Subtract(swordBase, bladeSide), Vector3Add(swordBase, bladeSide), swordTip, Color{255, 38, 30, 255});
    DrawTriangle3D(Vector3Add(swordBase, bladeSide), Vector3Subtract(swordBase, bladeSide), swordTip, Color{135, 16, 18, 255});
    DrawLine3D(swordBase, swordTip, Color{255, 220, 110, 255});
    DrawSphereEx(swordTip, 0.16f, 5, 4, Color{255, 82, 44, 255});

    Vector3 maneBase = Vector3Add(body, Vector3Scale(up, 0.78f));
    for (int i = 0; i < 5; ++i) {
        float offset = -1.0f + static_cast<float>(i) * 0.55f;
        Vector3 root = Vector3Add(maneBase, Vector3Scale(forward, offset));
        DrawTriangle3D(root,
            Vector3Add(root, Vector3Add(Vector3Scale(up, 0.35f), Vector3Scale(forward, 0.16f))),
            Vector3Add(root, Vector3Scale(right, 0.08f)),
            i % 2 == 0 ? woodDark : flame);
    }
    if (charge > 0.0f) {
        DrawSphereWires(warRider_.position, 3.2f + std::sin(warRider_.gallopTimer * 18.0f) * 0.25f,
            10, 6, FadeColor(Color{255, 68, 40, 255}, 0.62f));
    }
    rlPopMatrix();
}

void Game::DrawConquestRider() const {
    if (!conquestRider_.active) return;
    constexpr float kHorsemanVisualScale = 2.0f;
    Vector3 up = UpForWorldAt(conquestRider_.position, conquestRider_.world);
    Vector3 forward = conquestRider_.forward;
    if (IsSphericalMap()) {
        forward = SafeNormalize(ProjectOnSphericalTangent(forward, up), PlayerForward());
    } else {
        forward.y = 0.0f;
        forward = SafeNormalize(forward, Vector3{0.0f, 0.0f, 1.0f});
    }
    Vector3 right = SafeNormalize(Vector3CrossProduct(up, forward), Vector3{1.0f, 0.0f, 0.0f});
    float flash = std::clamp(conquestRider_.hitFlash, 0.0f, 1.0f);
    Color horse = flash > 0.0f ? WHITE : Color{232, 232, 218, 255};
    Color horseShade = flash > 0.0f ? Color{255, 255, 245, 255} : Color{170, 176, 154, 255};
    Color plague = Color{145, 235, 72, 255};
    Color dark = Color{50, 72, 38, 255};

    rlPushMatrix();
    rlTranslatef(conquestRider_.position.x, conquestRider_.position.y, conquestRider_.position.z);
    rlScalef(kHorsemanVisualScale, kHorsemanVisualScale, kHorsemanVisualScale);
    rlTranslatef(-conquestRider_.position.x, -conquestRider_.position.y, -conquestRider_.position.z);

    auto drawBox = [&](Vector3 center, float halfRight, float halfUp, float halfForward, Color fill, Color edgeColor) {
        Vector3 r = Vector3Scale(right, halfRight);
        Vector3 u = Vector3Scale(up, halfUp);
        Vector3 f = Vector3Scale(forward, halfForward);
        Vector3 p[8] = {
            Vector3Subtract(Vector3Subtract(Vector3Subtract(center, r), u), f),
            Vector3Subtract(Vector3Subtract(Vector3Add(center, r), u), f),
            Vector3Add(Vector3Subtract(Vector3Add(center, r), u), f),
            Vector3Add(Vector3Subtract(Vector3Subtract(center, r), u), f),
            Vector3Subtract(Vector3Add(Vector3Subtract(center, r), u), f),
            Vector3Subtract(Vector3Add(Vector3Add(center, r), u), f),
            Vector3Add(Vector3Add(Vector3Add(center, r), u), f),
            Vector3Add(Vector3Add(Vector3Subtract(center, r), u), f),
        };
        auto quad = [&](int a, int b, int c, int d) {
            DrawTriangle3D(p[a], p[b], p[c], fill);
            DrawTriangle3D(p[a], p[c], p[d], fill);
        };
        quad(0, 1, 2, 3);
        quad(4, 7, 6, 5);
        quad(0, 4, 5, 1);
        quad(1, 5, 6, 2);
        quad(2, 6, 7, 3);
        quad(3, 7, 4, 0);
        int edges[][2] = {{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
        for (auto& e : edges) DrawLine3D(p[e[0]], p[e[1]], edgeColor);
    };

    Vector3 body = Vector3Add(conquestRider_.position, Vector3Scale(up, 1.08f));
    drawBox(body, 0.62f, 0.46f, 1.66f, horse, FadeColor(horseShade, 0.82f));
    Vector3 neck = Vector3Add(body, Vector3Add(Vector3Scale(forward, 1.42f), Vector3Scale(up, 0.56f)));
    drawBox(neck, 0.32f, 0.68f, 0.32f, horseShade, FadeColor(dark, 0.62f));
    Vector3 head = Vector3Add(body, Vector3Add(Vector3Scale(forward, 2.00f), Vector3Scale(up, 0.82f)));
    drawBox(head, 0.42f, 0.32f, 0.56f, horse, FadeColor(horseShade, 0.72f));
    Vector3 nose = Vector3Add(head, Vector3Scale(forward, 0.74f));
    DrawTriangle3D(nose, Vector3Add(head, Vector3Add(Vector3Scale(up, 0.15f), Vector3Scale(right, 0.28f))),
        Vector3Add(head, Vector3Add(Vector3Scale(up, 0.15f), Vector3Scale(right, -0.28f))), horseShade);
    Vector3 earBase = Vector3Add(head, Vector3Scale(up, 0.34f));
    for (int side = -1; side <= 1; side += 2) {
        Vector3 earRoot = Vector3Add(earBase, Vector3Scale(right, side * 0.22f));
        DrawTriangle3D(earRoot,
            Vector3Add(earRoot, Vector3Add(Vector3Scale(forward, 0.10f), Vector3Scale(up, 0.44f))),
            Vector3Add(earRoot, Vector3Scale(right, side * 0.18f)),
            horseShade);
    }
    for (int side = -1; side <= 1; side += 2) {
        for (int pair = -1; pair <= 1; pair += 2) {
            Vector3 leg = Vector3Add(conquestRider_.position,
                Vector3Add(Vector3Scale(right, side * 0.45f), Vector3Add(Vector3Scale(forward, pair * 1.0f), Vector3Scale(up, 0.54f))));
            drawBox(leg, 0.12f, 0.55f, 0.14f, horseShade, FadeColor(dark, 0.48f));
        }
    }

    Vector3 rider = Vector3Add(body, Vector3Scale(up, 1.35f));
    DrawSphereEx(rider, 0.62f, 6, 5, flash > 0.0f ? WHITE : Color{210, 230, 160, 255});
    Vector3 crown = Vector3Add(rider, Vector3Scale(up, 0.78f));
    DrawCylinderWires(crown, 0.46f, 0.46f, 0.08f, 7, FadeColor(Color{235, 255, 180, 255}, 0.9f));

    Vector3 bowHand = Vector3Add(rider, Vector3Add(Vector3Scale(right, 0.72f), Vector3Scale(up, 0.04f)));
    Vector3 bowTop = Vector3Add(bowHand, Vector3Add(Vector3Scale(up, 1.05f), Vector3Scale(forward, 0.20f)));
    Vector3 bowBottom = Vector3Subtract(Vector3Add(bowHand, Vector3Scale(forward, 0.20f)), Vector3Scale(up, 1.05f));
    DrawCylinderEx(bowTop, bowHand, 0.045f, 0.035f, 5, Color{220, 230, 190, 255});
    DrawCylinderEx(bowHand, bowBottom, 0.045f, 0.035f, 5, Color{220, 230, 190, 255});
    DrawLine3D(bowTop, bowBottom, FadeColor(Color{245, 255, 220, 255}, 0.82f));
    Vector3 arrowTip = Vector3Add(bowHand, Vector3Scale(forward, 2.3f));
    DrawCylinderEx(bowHand, arrowTip, 0.035f, 0.02f, 5, plague);
    DrawSphereEx(arrowTip, 0.13f, 5, 4, Color{185, 255, 80, 255});

    rlPopMatrix();
}

void Game::DrawFamineRider() const {
    if (!famineRider_.active) return;
    constexpr float kHorsemanVisualScale = 2.0f;
    Vector3 up = UpForWorldAt(famineRider_.position, famineRider_.world);
    Vector3 forward = famineRider_.forward;
    if (IsSphericalMap()) {
        forward = SafeNormalize(ProjectOnSphericalTangent(forward, up), PlayerForward());
    } else {
        forward.y = 0.0f;
        forward = SafeNormalize(forward, Vector3{0.0f, 0.0f, 1.0f});
    }
    Vector3 right = SafeNormalize(Vector3CrossProduct(up, forward), Vector3{1.0f, 0.0f, 0.0f});
    float flash = std::clamp(famineRider_.hitFlash, 0.0f, 1.0f);
    float pulse = std::clamp(famineRider_.scaleTipTimer, 0.0f, 1.0f);
    Color horse = flash > 0.0f ? Color{235, 230, 210, 255} : Color{20, 18, 16, 255};
    Color horseEdge = flash > 0.0f ? WHITE : Color{92, 78, 48, 255};
    Color bone = Color{154, 136, 88, 255};
    Color ash = Color{48, 44, 34, 255};
    Color gold = Color{188, 156, 78, 255};

    rlPushMatrix();
    rlTranslatef(famineRider_.position.x, famineRider_.position.y, famineRider_.position.z);
    rlScalef(kHorsemanVisualScale, kHorsemanVisualScale, kHorsemanVisualScale);
    rlTranslatef(-famineRider_.position.x, -famineRider_.position.y, -famineRider_.position.z);

    auto drawBox = [&](Vector3 center, float halfRight, float halfUp, float halfForward, Color fill, Color edgeColor) {
        Vector3 r = Vector3Scale(right, halfRight);
        Vector3 u = Vector3Scale(up, halfUp);
        Vector3 f = Vector3Scale(forward, halfForward);
        Vector3 p[8] = {
            Vector3Subtract(Vector3Subtract(Vector3Subtract(center, r), u), f),
            Vector3Subtract(Vector3Subtract(Vector3Add(center, r), u), f),
            Vector3Add(Vector3Subtract(Vector3Add(center, r), u), f),
            Vector3Add(Vector3Subtract(Vector3Subtract(center, r), u), f),
            Vector3Subtract(Vector3Add(Vector3Subtract(center, r), u), f),
            Vector3Subtract(Vector3Add(Vector3Add(center, r), u), f),
            Vector3Add(Vector3Add(Vector3Add(center, r), u), f),
            Vector3Add(Vector3Add(Vector3Subtract(center, r), u), f),
        };
        auto quad = [&](int a, int b, int c, int d) {
            DrawTriangle3D(p[a], p[b], p[c], fill);
            DrawTriangle3D(p[a], p[c], p[d], fill);
        };
        quad(0, 1, 2, 3);
        quad(4, 7, 6, 5);
        quad(0, 4, 5, 1);
        quad(1, 5, 6, 2);
        quad(2, 6, 7, 3);
        quad(3, 7, 4, 0);
        int edges[][2] = {{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
        for (auto& e : edges) DrawLine3D(p[e[0]], p[e[1]], edgeColor);
    };

    Vector3 body = Vector3Add(famineRider_.position, Vector3Scale(up, 1.08f));
    drawBox(body, 0.62f, 0.45f, 1.64f, horse, FadeColor(horseEdge, 0.82f));
    Vector3 neck = Vector3Add(body, Vector3Add(Vector3Scale(forward, 1.42f), Vector3Scale(up, 0.56f)));
    drawBox(neck, 0.30f, 0.66f, 0.30f, ash, FadeColor(bone, 0.68f));
    Vector3 head = Vector3Add(body, Vector3Add(Vector3Scale(forward, 2.00f), Vector3Scale(up, 0.80f)));
    drawBox(head, 0.40f, 0.31f, 0.55f, horse, FadeColor(horseEdge, 0.75f));
    Vector3 nose = Vector3Add(head, Vector3Scale(forward, 0.72f));
    DrawTriangle3D(nose,
        Vector3Add(head, Vector3Add(Vector3Scale(up, 0.14f), Vector3Scale(right, 0.26f))),
        Vector3Add(head, Vector3Add(Vector3Scale(up, 0.14f), Vector3Scale(right, -0.26f))),
        ash);
    Vector3 earBase = Vector3Add(head, Vector3Scale(up, 0.32f));
    for (int side = -1; side <= 1; side += 2) {
        Vector3 earRoot = Vector3Add(earBase, Vector3Scale(right, side * 0.22f));
        DrawTriangle3D(earRoot,
            Vector3Add(earRoot, Vector3Add(Vector3Scale(forward, 0.08f), Vector3Scale(up, 0.42f))),
            Vector3Add(earRoot, Vector3Scale(right, side * 0.18f)),
            ash);
    }
    for (int side = -1; side <= 1; side += 2) {
        for (int pair = -1; pair <= 1; pair += 2) {
            Vector3 leg = Vector3Add(famineRider_.position,
                Vector3Add(Vector3Scale(right, side * 0.44f), Vector3Add(Vector3Scale(forward, pair * 0.98f), Vector3Scale(up, 0.54f))));
            drawBox(leg, 0.11f, 0.55f, 0.13f, ash, FadeColor(bone, 0.5f));
        }
    }

    Vector3 rider = Vector3Add(body, Vector3Scale(up, 1.34f));
    DrawSphereEx(rider, 0.62f, 6, 5, flash > 0.0f ? WHITE : Color{38, 34, 28, 255});
    DrawSphereWires(rider, 0.9f + pulse * 0.24f, 8, 6, FadeColor(gold, 0.62f));
    Vector3 hood = Vector3Add(rider, Vector3Scale(up, 0.55f));
    DrawCylinderWires(hood, 0.52f, 0.32f, 0.72f, 6, FadeColor(Color{72, 64, 42, 255}, 0.85f));

    Vector3 staffBase = Vector3Add(rider, Vector3Add(Vector3Scale(right, 0.74f), Vector3Scale(up, -0.72f)));
    Vector3 staffTop = Vector3Add(staffBase, Vector3Scale(up, 2.0f));
    DrawCylinderEx(staffBase, staffTop, 0.045f, 0.035f, 6, gold);
    Vector3 beam = Vector3Add(staffTop, Vector3Scale(forward, 0.12f));
    DrawCylinderEx(Vector3Subtract(beam, Vector3Scale(right, 0.72f)), Vector3Add(beam, Vector3Scale(right, 0.72f)), 0.035f, 0.035f, 6, gold);
    for (int side = -1; side <= 1; side += 2) {
        Vector3 chainTop = Vector3Add(beam, Vector3Scale(right, side * 0.56f));
        Vector3 pan = Vector3Subtract(chainTop, Vector3Scale(up, 0.48f + pulse * 0.08f * side));
        DrawLine3D(chainTop, pan, FadeColor(gold, 0.82f));
        DrawCylinderWires(pan, 0.30f, 0.20f, 0.05f, 12, FadeColor(gold, 0.9f));
    }

    Vector3 maneBase = Vector3Add(body, Vector3Scale(up, 0.74f));
    for (int i = 0; i < 5; ++i) {
        float offset = -0.95f + static_cast<float>(i) * 0.5f;
        Vector3 root = Vector3Add(maneBase, Vector3Scale(forward, offset));
        DrawTriangle3D(root,
            Vector3Add(root, Vector3Add(Vector3Scale(up, 0.30f), Vector3Scale(forward, 0.14f))),
            Vector3Add(root, Vector3Scale(right, 0.08f)),
            i % 2 == 0 ? Color{70, 62, 42, 255} : Color{122, 104, 62, 255});
    }

    if (pulse > 0.0f) {
        DrawSphereWires(famineRider_.position,
            FamineWitherRadius() * (0.15f + 0.85f * (1.0f - pulse)),
            16, 8, FadeColor(Color{112, 92, 48, 255}, 0.42f * pulse));
    }
    rlPopMatrix();
}

void Game::DrawDeathSouls() const {
    for (const DeathSoul& soul : deathSouls_) {
        float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 8.0f + soul.life);
        DrawSphereEx(soul.position, soul.radius, 6, 5, Color{8, 8, 12, 230});
        DrawSphereWires(soul.position, soul.radius * (1.25f + pulse * 0.2f), 7, 5, FadeColor(Color{90, 90, 112, 255}, 0.48f));
    }
}

void Game::DrawDeathRider() const {
    if (!deathRider_.active) return;
    constexpr float kHorsemanVisualScale = 2.0f;
    Vector3 up = UpForWorldAt(deathRider_.position, deathRider_.world);
    Vector3 forward = deathRider_.forward;
    if (IsSphericalMap()) {
        forward = SafeNormalize(ProjectOnSphericalTangent(forward, up), PlayerForward());
    } else {
        forward.y = 0.0f;
        forward = SafeNormalize(forward, Vector3{0.0f, 0.0f, 1.0f});
    }
    Vector3 right = SafeNormalize(Vector3CrossProduct(up, forward), Vector3{1.0f, 0.0f, 0.0f});
    float flash = std::clamp(deathRider_.hitFlash, 0.0f, 1.0f);
    float charge = config_.deathRiderSoulThreshold > 0
        ? std::clamp(static_cast<float>(deathRider_.souls) / static_cast<float>(config_.deathRiderSoulThreshold), 0.0f, 1.0f)
        : 0.0f;
    Color horse = flash > 0.0f ? WHITE : Color{58, 60, 64, 255};
    Color horseEdge = flash > 0.0f ? Color{235, 235, 245, 255} : Color{118, 122, 132, 255};
    Color dark = Color{18, 18, 22, 255};
    Color steel = Color{138, 142, 152, 255};
    Color soul = Color{42, 42, 52, 255};

    rlPushMatrix();
    rlTranslatef(deathRider_.position.x, deathRider_.position.y, deathRider_.position.z);
    rlScalef(kHorsemanVisualScale, kHorsemanVisualScale, kHorsemanVisualScale);
    rlTranslatef(-deathRider_.position.x, -deathRider_.position.y, -deathRider_.position.z);

    auto drawBox = [&](Vector3 center, float halfRight, float halfUp, float halfForward, Color fill, Color edgeColor) {
        Vector3 r = Vector3Scale(right, halfRight);
        Vector3 u = Vector3Scale(up, halfUp);
        Vector3 f = Vector3Scale(forward, halfForward);
        Vector3 p[8] = {
            Vector3Subtract(Vector3Subtract(Vector3Subtract(center, r), u), f),
            Vector3Subtract(Vector3Subtract(Vector3Add(center, r), u), f),
            Vector3Add(Vector3Subtract(Vector3Add(center, r), u), f),
            Vector3Add(Vector3Subtract(Vector3Subtract(center, r), u), f),
            Vector3Subtract(Vector3Add(Vector3Subtract(center, r), u), f),
            Vector3Subtract(Vector3Add(Vector3Add(center, r), u), f),
            Vector3Add(Vector3Add(Vector3Add(center, r), u), f),
            Vector3Add(Vector3Add(Vector3Subtract(center, r), u), f),
        };
        auto quad = [&](int a, int b, int c, int d) {
            DrawTriangle3D(p[a], p[b], p[c], fill);
            DrawTriangle3D(p[a], p[c], p[d], fill);
        };
        quad(0, 1, 2, 3);
        quad(4, 7, 6, 5);
        quad(0, 4, 5, 1);
        quad(1, 5, 6, 2);
        quad(2, 6, 7, 3);
        quad(3, 7, 4, 0);
        int edges[][2] = {{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
        for (auto& e : edges) DrawLine3D(p[e[0]], p[e[1]], edgeColor);
    };

    Vector3 body = Vector3Add(deathRider_.position, Vector3Scale(up, 1.08f));
    drawBox(body, 0.64f, 0.46f, 1.68f, horse, FadeColor(horseEdge, 0.82f));
    Vector3 neck = Vector3Add(body, Vector3Add(Vector3Scale(forward, 1.44f), Vector3Scale(up, 0.58f)));
    drawBox(neck, 0.31f, 0.68f, 0.31f, Color{42, 43, 48, 255}, FadeColor(horseEdge, 0.62f));
    Vector3 head = Vector3Add(body, Vector3Add(Vector3Scale(forward, 2.02f), Vector3Scale(up, 0.82f)));
    drawBox(head, 0.42f, 0.32f, 0.56f, horse, FadeColor(horseEdge, 0.74f));
    Vector3 nose = Vector3Add(head, Vector3Scale(forward, 0.74f));
    DrawTriangle3D(nose,
        Vector3Add(head, Vector3Add(Vector3Scale(up, 0.14f), Vector3Scale(right, 0.28f))),
        Vector3Add(head, Vector3Add(Vector3Scale(up, 0.14f), Vector3Scale(right, -0.28f))),
        Color{34, 35, 40, 255});
    for (int side = -1; side <= 1; side += 2) {
        for (int pair = -1; pair <= 1; pair += 2) {
            Vector3 leg = Vector3Add(deathRider_.position,
                Vector3Add(Vector3Scale(right, side * 0.45f), Vector3Add(Vector3Scale(forward, pair * 1.0f), Vector3Scale(up, 0.54f))));
            drawBox(leg, 0.11f, 0.55f, 0.13f, Color{38, 39, 44, 255}, FadeColor(horseEdge, 0.5f));
        }
    }

    Vector3 rider = Vector3Add(body, Vector3Scale(up, 1.36f));
    DrawSphereEx(rider, 0.64f, 6, 5, flash > 0.0f ? WHITE : Color{30, 30, 36, 255});
    DrawSphereWires(rider, 0.9f + charge * 0.24f, 8, 6, FadeColor(Color{132, 132, 150, 255}, 0.48f + charge * 0.28f));
    Vector3 hood = Vector3Add(rider, Vector3Scale(up, 0.56f));
    DrawCylinderWires(hood, 0.54f, 0.28f, 0.76f, 6, FadeColor(steel, 0.78f));

    Vector3 haftBase = Vector3Add(rider, Vector3Add(Vector3Scale(right, 0.82f), Vector3Scale(up, -0.82f)));
    Vector3 haftTop = Vector3Add(haftBase, Vector3Add(Vector3Scale(up, 2.85f), Vector3Scale(forward, 0.35f)));
    DrawCylinderEx(haftBase, haftTop, 0.055f, 0.04f, 6, steel);
    Vector3 bladeRoot = Vector3Add(haftTop, Vector3Scale(right, 0.08f));
    Vector3 bladeTip = Vector3Add(haftTop, Vector3Add(Vector3Scale(right, 1.55f), Vector3Scale(up, -0.54f)));
    Vector3 bladeBack = Vector3Add(haftTop, Vector3Add(Vector3Scale(right, 0.34f), Vector3Scale(up, -1.08f)));
    DrawTriangle3D(bladeRoot, bladeTip, bladeBack, Color{172, 176, 188, 255});
    DrawTriangle3D(bladeBack, bladeTip, bladeRoot, Color{82, 84, 94, 255});
    DrawLine3D(bladeRoot, bladeTip, Color{230, 232, 242, 255});
    DrawSphereEx(Vector3Add(rider, Vector3Scale(up, 0.02f)), 0.18f + charge * 0.1f, 6, 4, soul);
    rlPopMatrix();
}

void Game::DrawDeathSkulls() const {
    for (const DeathSkull& skull : deathSkulls_) {
        float waitPulse = skull.waitTimer > 0.0f ? 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 12.0f) : 0.0f;
        float r = skull.radius;
        Color bone = skull.waitTimer > 0.0f
            ? FadeColor(Color{245, 245, 230, 255}, 0.72f + waitPulse * 0.22f)
            : Color{245, 245, 232, 255};
        Color shadowBone = skull.waitTimer > 0.0f
            ? FadeColor(Color{190, 190, 184, 255}, 0.72f + waitPulse * 0.18f)
            : Color{194, 194, 188, 255};
        Color socket = Color{12, 12, 16, 255};
        Vector3 forward = SafeNormalize(skull.forward, Vector3{0.0f, 0.0f, 1.0f});
        Vector3 up = UpForWorldAt(skull.position, skull.world);
        Vector3 right = SafeNormalize(Vector3CrossProduct(up, forward), Vector3{1.0f, 0.0f, 0.0f});
        up = SafeNormalize(Vector3CrossProduct(forward, right), up);

        auto drawBox = [&](Vector3 center, float halfRight, float halfUp, float halfForward, Color fill, Color edgeColor) {
            Vector3 rr = Vector3Scale(right, halfRight);
            Vector3 uu = Vector3Scale(up, halfUp);
            Vector3 ff = Vector3Scale(forward, halfForward);
            Vector3 p[8] = {
                Vector3Subtract(Vector3Subtract(Vector3Subtract(center, rr), uu), ff),
                Vector3Subtract(Vector3Subtract(Vector3Add(center, rr), uu), ff),
                Vector3Add(Vector3Subtract(Vector3Add(center, rr), uu), ff),
                Vector3Add(Vector3Subtract(Vector3Subtract(center, rr), uu), ff),
                Vector3Subtract(Vector3Add(Vector3Subtract(center, rr), uu), ff),
                Vector3Subtract(Vector3Add(Vector3Add(center, rr), uu), ff),
                Vector3Add(Vector3Add(Vector3Add(center, rr), uu), ff),
                Vector3Add(Vector3Add(Vector3Subtract(center, rr), uu), ff),
            };
            auto quad = [&](int a, int b, int c, int d) {
                DrawTriangle3D(p[a], p[b], p[c], fill);
                DrawTriangle3D(p[a], p[c], p[d], fill);
            };
            quad(0, 1, 2, 3);
            quad(4, 7, 6, 5);
            quad(0, 4, 5, 1);
            quad(1, 5, 6, 2);
            quad(2, 6, 7, 3);
            quad(3, 7, 4, 0);
            int edges[][2] = {{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
            for (auto& edge : edges) DrawLine3D(p[edge[0]], p[edge[1]], edgeColor);
        };

        Vector3 cranium = Vector3Add(skull.position, Vector3Scale(up, r * 0.14f));
        Vector3 face = Vector3Add(skull.position, Vector3Add(Vector3Scale(forward, r * 0.34f), Vector3Scale(up, -r * 0.14f)));
        DrawSphereEx(cranium, r * 0.92f, 9, 7, bone);
        DrawSphereEx(face, r * 0.58f, 7, 5, shadowBone);
        DrawSphereWires(cranium, r * 0.96f, 9, 6, FadeColor(Color{250, 250, 242, 255}, 0.36f));

        Vector3 brow = Vector3Add(skull.position, Vector3Add(Vector3Scale(forward, r * 0.62f), Vector3Scale(up, r * 0.15f)));
        DrawCylinderEx(Vector3Subtract(brow, Vector3Scale(right, r * 0.44f)),
            Vector3Add(brow, Vector3Scale(right, r * 0.44f)), r * 0.055f, r * 0.045f, 5, shadowBone);

        Vector3 eyeBase = Vector3Add(skull.position, Vector3Add(Vector3Scale(forward, r * 0.76f), Vector3Scale(up, r * 0.04f)));
        Vector3 leftEye = Vector3Add(eyeBase, Vector3Scale(right, r * 0.28f));
        Vector3 rightEye = Vector3Subtract(eyeBase, Vector3Scale(right, r * 0.28f));
        DrawSphereEx(leftEye, r * 0.20f, 6, 4, socket);
        DrawSphereEx(rightEye, r * 0.20f, 6, 4, socket);
        DrawSphereWires(leftEye, r * 0.22f, 6, 4, FadeColor(Color{235, 235, 225, 255}, 0.36f));
        DrawSphereWires(rightEye, r * 0.22f, 6, 4, FadeColor(Color{235, 235, 225, 255}, 0.36f));

        Vector3 noseTop = Vector3Add(skull.position, Vector3Add(Vector3Scale(forward, r * 0.85f), Vector3Scale(up, -r * 0.16f)));
        Vector3 noseBottom = Vector3Add(noseTop, Vector3Scale(up, -r * 0.28f));
        DrawTriangle3D(noseTop, Vector3Add(noseBottom, Vector3Scale(right, r * 0.13f)),
            Vector3Subtract(noseBottom, Vector3Scale(right, r * 0.13f)), socket);

        Vector3 cheek = Vector3Add(skull.position, Vector3Add(Vector3Scale(forward, r * 0.62f), Vector3Scale(up, -r * 0.28f)));
        DrawCylinderEx(Vector3Subtract(cheek, Vector3Scale(right, r * 0.48f)),
            Vector3Subtract(cheek, Vector3Scale(right, r * 0.14f)), r * 0.045f, r * 0.035f, 5, shadowBone);
        DrawCylinderEx(Vector3Add(cheek, Vector3Scale(right, r * 0.14f)),
            Vector3Add(cheek, Vector3Scale(right, r * 0.48f)), r * 0.045f, r * 0.035f, 5, shadowBone);

        Vector3 jaw = Vector3Add(skull.position, Vector3Add(Vector3Scale(forward, r * 0.26f), Vector3Scale(up, -r * 0.58f)));
        drawBox(jaw, r * 0.34f, r * 0.18f, r * 0.24f, shadowBone, FadeColor(Color{245, 245, 232, 255}, 0.48f));
        Vector3 teethBase = Vector3Add(jaw, Vector3Add(Vector3Scale(forward, r * 0.25f), Vector3Scale(up, r * 0.08f)));
        DrawLine3D(Vector3Subtract(teethBase, Vector3Scale(right, r * 0.27f)),
            Vector3Add(teethBase, Vector3Scale(right, r * 0.27f)), socket);
        for (int tooth = -2; tooth <= 2; ++tooth) {
            Vector3 root = Vector3Add(teethBase, Vector3Scale(right, static_cast<float>(tooth) * r * 0.12f));
            DrawLine3D(Vector3Add(root, Vector3Scale(up, r * 0.10f)), Vector3Subtract(root, Vector3Scale(up, r * 0.12f)), socket);
        }

        Vector3 trail = Vector3Subtract(skull.position, Vector3Scale(forward, r * (0.95f + waitPulse * 0.25f)));
        DrawCylinderEx(trail, skull.position, r * 0.08f, r * 0.02f, 6,
            FadeColor(Color{215, 215, 235, 255}, skull.waitTimer > 0.0f ? 0.42f : 0.24f));
        DrawSphereWires(skull.position, r * (1.28f + waitPulse * 0.22f), 8, 5,
            FadeColor(Color{230, 230, 245, 255}, skull.waitTimer > 0.0f ? 0.52f : 0.28f));
        if (skull.waitTimer > 0.0f) {
            DrawCircle3D(skull.position, r * (1.45f + waitPulse * 0.18f), forward, 90.0f,
                FadeColor(Color{238, 238, 255, 255}, 0.48f + waitPulse * 0.18f));
        }
    }
}

void Game::DrawPlagueArrows() const {
    for (const PlagueArrow& arrow : plagueArrows_) {
        float alpha = arrow.maxLife > 0.0f ? std::clamp(arrow.life / arrow.maxLife, 0.0f, 1.0f) : 1.0f;
        Vector3 forward = SafeNormalize(arrow.forward, Vector3{0.0f, 0.0f, 1.0f});
        Vector3 side = SafeNormalize(arrow.side, Vector3{1.0f, 0.0f, 0.0f});
        Vector3 up = SafeNormalize(Vector3CrossProduct(side, forward), Vector3{0.0f, 1.0f, 0.0f});
        Vector3 tail = Vector3Subtract(arrow.position, Vector3Scale(forward, 1.05f));
        Vector3 tip = Vector3Add(arrow.position, Vector3Scale(forward, 1.15f));
        DrawCylinderEx(tail, tip, arrow.radius * 0.22f, arrow.radius * 0.08f, 6, FadeColor(Color{185, 255, 90, 255}, alpha));
        DrawSphereEx(tip, arrow.radius * 0.62f, 5, 4, FadeColor(Color{155, 255, 62, 255}, alpha));
        Vector3 fletch = Vector3Subtract(arrow.position, Vector3Scale(forward, 0.82f));
        DrawTriangle3D(fletch, Vector3Add(fletch, Vector3Scale(side, arrow.radius * 1.3f)), Vector3Add(tail, Vector3Scale(up, arrow.radius * 0.9f)), FadeColor(Color{210, 255, 130, 255}, alpha * 0.82f));
        DrawTriangle3D(fletch, Vector3Subtract(fletch, Vector3Scale(side, arrow.radius * 1.3f)), Vector3Add(tail, Vector3Scale(up, arrow.radius * 0.9f)), FadeColor(Color{115, 190, 55, 255}, alpha * 0.82f));
        DrawSphereWires(arrow.position, arrow.radius * 1.45f, 6, 4, FadeColor(Color{160, 255, 70, 255}, alpha * 0.8f));
    }
}

void Game::DrawSeraphFireballs() const {
    auto drawSodomFireball = [&](const SeraphFireball& fireball) {
        float alpha = fireball.maxLife > 0.0f ? std::clamp(fireball.life / fireball.maxLife, 0.0f, 1.0f) : 1.0f;
        Vector3 forward = SafeNormalize(fireball.flightDirection,
            SafeNormalize(fireball.velocity, Vector3{0.0f, -1.0f, 0.0f}));
        Vector3 side = fireball.visualSide;
        side = Vector3Subtract(side, Vector3Scale(forward, Vector3DotProduct(side, forward)));
        side = SafeNormalize(side, PlayerRight());
        Vector3 up = SafeNormalize(Vector3CrossProduct(side, forward), PlayerUp());
        float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 18.0f + fireball.life * 2.7f);
        float r = fireball.radius;
        Color core = FadeColor(Color{255, 232, 136, 255}, alpha * (0.88f + pulse * 0.12f));
        Color shell = FadeColor(Color{255, 92, 42, 255}, alpha * 0.84f);
        Color dark = FadeColor(Color{128, 28, 20, 255}, alpha * 0.62f);
        Color trail = FadeColor(Color{255, 66, 30, 255}, alpha * 0.58f);

        DrawSphereEx(fireball.position, r * 1.16f, 8, 6, shell);
        DrawSphereEx(Vector3Add(fireball.position, Vector3Scale(forward, r * 0.18f)), r * 0.72f, 7, 5, core);
        DrawSphereWires(fireball.position, r * (1.34f + pulse * 0.18f), 8, 5, FadeColor(Color{255, 168, 64, 255}, alpha * 0.68f));

        Vector3 tail = Vector3Subtract(fireball.position, Vector3Scale(forward, r * 1.05f));
        Vector3 farTail = Vector3Subtract(fireball.position, Vector3Scale(forward, r * (4.8f + pulse * 0.8f)));
        DrawCylinderEx(tail, farTail, r * 0.52f, r * 0.06f, 7, trail);
        DrawTriangle3D(tail,
            Vector3Add(farTail, Vector3Scale(side, r * (0.68f + pulse * 0.18f))),
            Vector3Add(farTail, Vector3Scale(up, r * 0.38f)), trail);
        DrawTriangle3D(tail,
            Vector3Subtract(farTail, Vector3Scale(side, r * (0.68f + pulse * 0.18f))),
            Vector3Subtract(farTail, Vector3Scale(up, r * 0.38f)), dark);
    };
    auto drawFeather = [&](const SeraphFireball& fireball, bool sodomFire) {
        float alpha = fireball.maxLife > 0.0f ? std::clamp(fireball.life / fireball.maxLife, 0.0f, 1.0f) : 1.0f;
        Vector3 travel = Vector3Subtract(fireball.position, fireball.prevPosition);
        Vector3 forward = SafeNormalize(fireball.tipDirection,
            SafeNormalize(fireball.flightDirection, SafeNormalize(fireball.velocity, PlayerForward())));
        Vector3 side = fireball.visualSide;
        side = Vector3Subtract(side, Vector3Scale(forward, Vector3DotProduct(side, forward)));
        side = SafeNormalize(side, PlayerRight());
        Vector3 faceNormal = SafeNormalize(Vector3CrossProduct(forward, side), PlayerUp());
        float length = std::max(fireball.radius * 3.8f, 0.82f);
        float halfWidth = std::max(fireball.radius * 1.25f, 0.24f);
        Vector3 tip = Vector3Add(fireball.position, Vector3Scale(forward, length * 0.54f));
        Vector3 tail = Vector3Subtract(fireball.position, Vector3Scale(forward, length * 0.46f));
        Vector3 left = Vector3Add(fireball.position, Vector3Scale(side, halfWidth));
        Vector3 right = Vector3Subtract(fireball.position, Vector3Scale(side, halfWidth));
        Vector3 lift = Vector3Scale(faceNormal, fireball.radius * 0.10f);
        Color outer = FadeColor(sodomFire ? Color{255, 82, 48, 255} : Color{255, 190, 70, 255}, alpha * 0.78f);
        Color inner = FadeColor(sodomFire ? Color{255, 205, 118, 255} : Color{255, 248, 195, 255}, alpha * 0.88f);
        Color edge = FadeColor(sodomFire ? Color{255, 158, 64, 255} : Color{255, 225, 120, 255}, alpha * 0.92f);

        Vector3 tipA = Vector3Add(tip, lift);
        Vector3 tailA = Vector3Add(tail, lift);
        Vector3 leftA = Vector3Add(left, lift);
        Vector3 rightA = Vector3Add(right, lift);
        Vector3 tipB = Vector3Subtract(tip, lift);
        Vector3 tailB = Vector3Subtract(tail, lift);
        Vector3 leftB = Vector3Subtract(left, lift);
        Vector3 rightB = Vector3Subtract(right, lift);

        DrawTriangle3D(tipA, leftA, tailA, outer);
        DrawTriangle3D(tipA, tailA, rightA, outer);
        DrawTriangle3D(tipB, tailB, leftB, outer);
        DrawTriangle3D(tipB, rightB, tailB, outer);

        Vector3 glowTip = Vector3Add(fireball.position, Vector3Scale(forward, length * 0.32f));
        Vector3 glowTail = Vector3Subtract(fireball.position, Vector3Scale(forward, length * 0.25f));
        Vector3 glowLeft = Vector3Add(fireball.position, Vector3Scale(side, halfWidth * 0.42f));
        Vector3 glowRight = Vector3Subtract(fireball.position, Vector3Scale(side, halfWidth * 0.42f));
        DrawTriangle3D(glowTip, glowLeft, glowTail, inner);
        DrawTriangle3D(glowTip, glowTail, glowRight, inner);
        DrawLine3D(tip, left, edge);
        DrawLine3D(left, tail, edge);
        DrawLine3D(tail, right, edge);
        DrawLine3D(right, tip, edge);

        if (Vector3Length(travel) > 0.001f) {
            Vector3 trailDir = Vector3Scale(forward, -1.0f);
            DrawLine3D(tail, Vector3Add(tail, Vector3Scale(trailDir, length * 1.25f)),
                FadeColor(sodomFire ? Color{255, 54, 32, 255} : Color{255, 150, 55, 255}, alpha * 0.55f));
        }
    };
    for (const SeraphFireball& fireball : seraphFireballs_) {
        if (fireball.sodomFire || fireball.warFire) {
            drawSodomFireball(fireball);
        } else {
            drawFeather(fireball, false);
        }
    }
    for (const SeraphFireball& fireball : edenFireRain_) {
        drawSodomFireball(fireball);
    }
}

void Game::DrawEdenSkySphere() const {
    constexpr int kStarCount = 150;
    const float radius = std::max(config_.edenMapRadius * 4.0f, 420.0f);
    float apocalypse = edenForbiddenFruit_.claimed ? std::clamp(edenForbiddenFruit_.apocalypse, 0.0f, 1.0f) : 0.0f;
    auto mixColor = [](Color a, Color b, float t) {
        return Color{
            static_cast<unsigned char>(std::clamp(static_cast<float>(a.r) + (static_cast<float>(b.r) - static_cast<float>(a.r)) * t, 0.0f, 255.0f)),
            static_cast<unsigned char>(std::clamp(static_cast<float>(a.g) + (static_cast<float>(b.g) - static_cast<float>(a.g)) * t, 0.0f, 255.0f)),
            static_cast<unsigned char>(std::clamp(static_cast<float>(a.b) + (static_cast<float>(b.b) - static_cast<float>(a.b)) * t, 0.0f, 255.0f)),
            static_cast<unsigned char>(std::clamp(static_cast<float>(a.a) + (static_cast<float>(b.a) - static_cast<float>(a.a)) * t, 0.0f, 255.0f))
        };
    };
    rlDrawRenderBatchActive();
    rlDisableDepthTest();
    for (int i = 0; i < kStarCount; ++i) {
        float a = static_cast<float>(i) * 2.39996323f;
        float hNoise = std::fmod(std::sin(static_cast<float>(i) * 19.371f) * 43758.5453f + 43758.5453f, 1.0f);
        float h = 0.12f + 0.86f * hNoise;
        float r = std::sqrt(std::max(0.0f, 1.0f - h * h));
        Vector3 direction{
            std::cos(a) * r,
            h,
            std::sin(a) * r
        };
        float drift = std::fmod(std::sin(static_cast<float>(i) * 7.123f) * 91.73f + 91.73f, 1.0f) * 0.16f;
        direction = SafeNormalize(Vector3Add(direction, Vector3{drift, 0.0f, -drift * 0.7f}), Vector3{0.0f, 1.0f, 0.0f});
        Vector3 pos = Vector3Add(camera_.position, Vector3Scale(direction, radius));
        float twinkle = 0.58f + 0.26f * std::sin(static_cast<float>(GetTime()) * 0.45f + static_cast<float>(i) * 1.7f);
        float size = ((i % 13 == 0) ? 0.48f : ((i % 5 == 0) ? 0.34f : 0.22f)) * radius / 420.0f;
        Color outer = mixColor(Color{154, 120, 220, 255}, Color{255, 58, 46, 255}, apocalypse);
        Color mid = mixColor(Color{236, 220, 255, 255}, Color{255, 126, 70, 255}, apocalypse);
        Color core = mixColor(Color{255, 250, 235, 255}, Color{255, 224, 144, 255}, apocalypse);
        DrawSphereEx(pos, size * 2.8f, 6, 4, FadeColor(outer, twinkle * (0.13f + apocalypse * 0.09f)));
        DrawSphereEx(pos, size * 1.45f, 5, 4, FadeColor(mid, twinkle * (0.28f + apocalypse * 0.10f)));
        DrawSphereEx(pos, size * 0.52f, 4, 3, FadeColor(core, twinkle * 0.52f));
    }
    rlDrawRenderBatchActive();
    rlEnableDepthTest();
}

void Game::DrawEdenForbiddenFruit() const {
    if (!IsEdenMap() || !edenForbiddenFruit_.active) return;

    constexpr float phi = 1.61803398875f;
    const Vector3 rawVertices[12] = {
        {0.0f, 1.0f, phi}, {0.0f, -1.0f, phi}, {0.0f, 1.0f, -phi}, {0.0f, -1.0f, -phi},
        {1.0f, phi, 0.0f}, {-1.0f, phi, 0.0f}, {1.0f, -phi, 0.0f}, {-1.0f, -phi, 0.0f},
        {phi, 0.0f, 1.0f}, {-phi, 0.0f, 1.0f}, {phi, 0.0f, -1.0f}, {-phi, 0.0f, -1.0f}
    };
    const int faces[20][3] = {
        {0, 1, 8}, {0, 9, 1}, {0, 8, 4}, {0, 5, 9}, {0, 4, 5},
        {1, 6, 8}, {1, 9, 7}, {1, 7, 6}, {2, 10, 3}, {2, 3, 11},
        {2, 4, 10}, {2, 11, 5}, {2, 5, 4}, {3, 10, 6}, {3, 7, 11},
        {3, 6, 7}, {4, 8, 10}, {5, 11, 9}, {6, 10, 8}, {7, 9, 11}
    };

    float pulse = 0.5f + 0.5f * std::sin(edenForbiddenFruit_.spin * 3.1f);
    float absorbedGlow = std::clamp(static_cast<float>(edenForbiddenFruit_.absorbedEssence) / 24.0f, 0.0f, 1.0f);
    Color outer = edenForbiddenFruit_.claimed
        ? Color{205, 72, 255, 245}
        : Color{255, static_cast<unsigned char>(64 + absorbedGlow * 78.0f), static_cast<unsigned char>(42 + absorbedGlow * 68.0f), 245};
    Color inner = edenForbiddenFruit_.claimed
        ? Color{255, 220, 255, 255}
        : Color{255, static_cast<unsigned char>(178 + absorbedGlow * 55.0f), static_cast<unsigned char>(76 + absorbedGlow * 92.0f), 255};
    Color edge = edenForbiddenFruit_.claimed
        ? Color{255, 244, 255, 255}
        : Color{255, 226, 108, 255};

    Vector3 vertices[12];
    Vector3 axisA = SafeNormalize(Vector3{0.45f, 1.0f, 0.18f}, Vector3{0.0f, 1.0f, 0.0f});
    Vector3 axisB = SafeNormalize(Vector3{-0.24f, 0.35f, 1.0f}, Vector3{0.0f, 0.0f, 1.0f});
    float rawScale = config_.edenForbiddenFruitRadius / std::sqrt(1.0f + phi * phi);
    for (int i = 0; i < 12; ++i) {
        Vector3 v = Vector3Scale(rawVertices[i], rawScale);
        v = RotateAroundAxis(v, axisA, edenForbiddenFruit_.spin * 0.55f);
        v = RotateAroundAxis(v, axisB, edenForbiddenFruit_.spin * 0.27f);
        vertices[i] = Vector3Add(edenForbiddenFruit_.position, v);
    }

    DrawSphereEx(edenForbiddenFruit_.position, config_.edenForbiddenFruitRadius * (1.2f + absorbedGlow * 0.35f),
        12, 8, FadeColor(edenForbiddenFruit_.claimed ? Color{210, 95, 255, 255} : Color{255, 92, 38, 255}, 0.14f + pulse * 0.08f));
    DrawSphereWires(edenForbiddenFruit_.position, config_.edenForbiddenFruitRadius * (1.55f + absorbedGlow * 0.45f + pulse * 0.08f),
        14, 8, FadeColor(edge, 0.38f + absorbedGlow * 0.22f));

    Vector3 light = SafeNormalize(Vector3{0.35f, 0.84f, 0.42f}, Vector3{0.0f, 1.0f, 0.0f});
    for (const auto& face : faces) {
        Vector3 a = vertices[face[0]];
        Vector3 b = vertices[face[1]];
        Vector3 c = vertices[face[2]];
        Vector3 normal = SafeNormalize(Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a)), light);
        float lit = std::clamp(Vector3DotProduct(normal, light) * 0.5f + 0.5f, 0.0f, 1.0f);
        Color faceColor = Color{
            static_cast<unsigned char>(outer.r + (inner.r - outer.r) * lit),
            static_cast<unsigned char>(outer.g + (inner.g - outer.g) * lit),
            static_cast<unsigned char>(outer.b + (inner.b - outer.b) * lit),
            238
        };
        DrawTriangle3D(a, b, c, faceColor);
        DrawTriangle3D(a, c, b, faceColor);
        DrawLine3D(a, b, FadeColor(edge, 0.68f));
        DrawLine3D(b, c, FadeColor(edge, 0.68f));
        DrawLine3D(c, a, FadeColor(edge, 0.68f));
    }

    if (!edenForbiddenFruit_.claimed && edenForbiddenFruit_.absorbedEssence > 0) {
        DrawCircle3D(edenForbiddenFruit_.position, config_.edenForbiddenFruitRadius * (2.0f + pulse * 0.18f),
            Vector3{0.0f, 1.0f, 0.0f}, 90.0f, FadeColor(Color{255, 218, 92, 255}, 0.34f + absorbedGlow * 0.28f));
    }
}

void Game::DrawEdenArk() const {
    if (!edenArk_.active) return;
    Vector3 up = UpForWorldAt(edenArk_.position, 0);
    Vector3 rawForward{std::cos(edenArk_.heading), 0.0f, std::sin(edenArk_.heading)};
    Vector3 forward = IsSphericalMap()
        ? SafeNormalize(ProjectOnSphericalTangent(edenArk_.forward, up),
            SafeNormalize(ProjectOnSphericalTangent(rawForward, up), Vector3{0.0f, 0.0f, 1.0f}))
        : rawForward;
    Vector3 right = SafeNormalize(Vector3CrossProduct(forward, up), Vector3{1.0f, 0.0f, 0.0f});
    Vector3 base = edenArk_.position;
    float apocalypse = (IsEdenMap() && edenForbiddenFruit_.claimed) ? std::clamp(edenForbiddenFruit_.apocalypse, 0.0f, 1.0f) : 0.0f;
    auto mixColor = [](Color a, Color b, float t) {
        return Color{
            static_cast<unsigned char>(std::clamp(static_cast<float>(a.r) + (static_cast<float>(b.r) - static_cast<float>(a.r)) * t, 0.0f, 255.0f)),
            static_cast<unsigned char>(std::clamp(static_cast<float>(a.g) + (static_cast<float>(b.g) - static_cast<float>(a.g)) * t, 0.0f, 255.0f)),
            static_cast<unsigned char>(std::clamp(static_cast<float>(a.b) + (static_cast<float>(b.b) - static_cast<float>(a.b)) * t, 0.0f, 255.0f)),
            static_cast<unsigned char>(std::clamp(static_cast<float>(a.a) + (static_cast<float>(b.a) - static_cast<float>(a.a)) * t, 0.0f, 255.0f))
        };
    };
    Color hull = mixColor(Color{118, 74, 38, 255}, Color{80, 68, 60, 255}, apocalypse);
    Color side = mixColor(Color{152, 98, 48, 255}, Color{104, 84, 72, 255}, apocalypse);
    Color roof = mixColor(Color{92, 58, 34, 255}, Color{70, 64, 58, 255}, apocalypse);
    Color trim = mixColor(Color{232, 182, 88, 255}, Color{148, 112, 82, 255}, apocalypse);
    Color window = FadeColor(mixColor(Color{255, 232, 154, 255}, Color{172, 118, 92, 255}, apocalypse), 0.78f);
    auto local = [&](float x, float y, float z) {
        return Vector3{x, y, z};
    };

    rlPushMatrix();
    Matrix basis = {
        right.x, up.x, forward.x, base.x,
        right.y, up.y, forward.y, base.y,
        right.z, up.z, forward.z, base.z,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    rlMultMatrixf(MatrixToFloat(basis));

    DrawCube(local(0.0f, 2.8f, 0.0f), 15.0f, 5.4f, 31.0f, FadeColor(hull, 0.98f));
    DrawCubeWires(local(0.0f, 2.8f, 0.0f), 15.2f, 5.55f, 31.2f, FadeColor(Color{44, 30, 20, 255}, 0.82f));
    DrawCube(local(0.0f, 6.3f, 0.0f), 13.2f, 2.8f, 27.4f, FadeColor(side, 0.96f));
    DrawCubeWires(local(0.0f, 6.3f, 0.0f), 13.35f, 2.95f, 27.55f, FadeColor(trim, 0.48f));
    DrawCube(local(0.0f, 8.35f, 0.0f), 11.4f, 1.65f, 23.5f, FadeColor(roof, 0.96f));

    for (int layer = 0; layer < 3; ++layer) {
        float y = 1.45f + static_cast<float>(layer) * 2.25f;
        DrawLine3D(local(-7.7f, y, -15.7f), local(-7.7f, y, 15.7f), FadeColor(trim, 0.58f));
        DrawLine3D(local(7.7f, y, -15.7f), local(7.7f, y, 15.7f), FadeColor(trim, 0.58f));
        DrawLine3D(local(-7.7f, y, -15.7f), local(7.7f, y, -15.7f), FadeColor(trim, 0.48f));
        DrawLine3D(local(-7.7f, y, 15.7f), local(7.7f, y, 15.7f), FadeColor(trim, 0.48f));
    }
    for (int rib = -5; rib <= 5; ++rib) {
        float z = static_cast<float>(rib) * 2.7f;
        DrawLine3D(local(-7.65f, 0.35f, z), local(-7.65f, 8.5f, z), FadeColor(Color{62, 38, 24, 255}, 0.58f));
        DrawLine3D(local(7.65f, 0.35f, z), local(7.65f, 8.5f, z), FadeColor(Color{62, 38, 24, 255}, 0.58f));
    }

    DrawCube(local(-7.82f, 2.9f, -2.2f), 0.32f, 3.2f, 4.8f, FadeColor(Color{64, 38, 24, 255}, 0.92f));
    DrawCubeWires(local(-7.86f, 2.9f, -2.2f), 0.38f, 3.35f, 4.95f, FadeColor(trim, 0.62f));
    DrawCube(local(0.0f, 9.45f, -5.4f), 4.8f, 0.35f, 3.2f, window);
    DrawCube(local(0.0f, 9.48f, 4.8f), 4.2f, 0.32f, 2.8f, FadeColor(window, 0.62f));
    for (int w = -4; w <= 4; ++w) {
        if (w == 0) continue;
        DrawCube(local(-7.86f, 5.7f, static_cast<float>(w) * 2.8f), 0.24f, 0.64f, 1.05f, window);
        DrawCube(local(7.86f, 5.7f, static_cast<float>(w) * 2.8f), 0.24f, 0.64f, 1.05f, window);
    }

	    DrawCube(local(0.0f, 2.8f, 16.6f), 12.2f, 4.8f, 1.8f, FadeColor(hull, 0.92f));
	    DrawLine3D(local(-5.8f, 6.1f, 16.8f), local(5.8f, 6.1f, 16.8f), FadeColor(trim, 0.70f));
	    if (edenArk_.piloted) {
	        DrawCircle3D(local(0.0f, 0.2f, 0.0f), 9.2f, Vector3{0.0f, 1.0f, 0.0f}, 90.0f, FadeColor(Color{255, 228, 126, 255}, 0.32f));
	    }
	    float speedT = std::clamp(std::abs(edenArk_.speed) / std::max(1.0f, 16.0f * config_.arkShiftSpeedMult), 0.0f, 1.0f);
	    if (speedT > 0.04f) {
	        float wakeSign = edenArk_.speed >= 0.0f ? -1.0f : 1.0f;
	        float wakeAlpha = 0.18f + speedT * 0.36f;
	        Color wakeFill = FadeColor(mixColor(Color{140, 226, 255, 255}, Color{205, 205, 196, 255}, apocalypse), wakeAlpha);
	        Color wakeLine = FadeColor(mixColor(Color{230, 252, 255, 255}, Color{244, 225, 190, 255}, apocalypse), 0.30f + speedT * 0.42f);
	        float phase = static_cast<float>(GetTime()) * (3.5f + speedT * 4.0f);
	        for (int band = 0; band < 3; ++band) {
	            float bandT = static_cast<float>(band);
	            float z0 = wakeSign * (17.0f + bandT * 4.2f);
	            float z1 = wakeSign * (25.0f + bandT * 7.5f + speedT * 9.0f);
	            float inner = 3.2f + bandT * 1.25f;
	            float outer = 8.0f + bandT * 3.8f + speedT * 5.0f;
	            float ripple = std::sin(phase + bandT * 1.7f) * (0.18f + speedT * 0.18f);
	            Vector3 nearL = local(-inner, 0.12f + ripple, z0);
	            Vector3 nearR = local(inner, 0.12f - ripple * 0.45f, z0);
	            Vector3 farL = local(-outer, 0.06f - ripple * 0.25f, z1);
	            Vector3 farR = local(outer, 0.06f + ripple * 0.25f, z1);
	            DrawTriangle3D(nearL, farL, nearR, FadeColor(wakeFill, wakeAlpha * (1.0f - bandT * 0.22f)));
	            DrawTriangle3D(nearR, farL, farR, FadeColor(wakeFill, wakeAlpha * (1.0f - bandT * 0.22f)));
	            DrawLine3D(nearL, farL, FadeColor(wakeLine, 0.80f - bandT * 0.18f));
	            DrawLine3D(nearR, farR, FadeColor(wakeLine, 0.80f - bandT * 0.18f));
	            DrawLine3D(local(-outer * 0.55f, 0.10f + ripple * 0.30f, z1 - wakeSign * 1.2f),
	                       local(outer * 0.55f, 0.10f - ripple * 0.30f, z1 - wakeSign * 1.2f),
	                       FadeColor(wakeLine, 0.36f - bandT * 0.06f));
	        }
	    }
	    rlPopMatrix();
	}

void Game::DrawArena() const {
    if (IsEdenMap()) {
        constexpr int kRings = 44;
        constexpr int kSegments = 96;
        float visualMapRadius = config_.edenMapRadius + 320.0f;
        float maxR = visualMapRadius;
        float apocalypse = edenForbiddenFruit_.claimed ? std::clamp(edenForbiddenFruit_.apocalypse, 0.0f, 1.0f) : 0.0f;
        auto mixColor = [](Color a, Color b, float t) {
            return Color{
                static_cast<unsigned char>(std::clamp(static_cast<float>(a.r) + (static_cast<float>(b.r) - static_cast<float>(a.r)) * t, 0.0f, 255.0f)),
                static_cast<unsigned char>(std::clamp(static_cast<float>(a.g) + (static_cast<float>(b.g) - static_cast<float>(a.g)) * t, 0.0f, 255.0f)),
                static_cast<unsigned char>(std::clamp(static_cast<float>(a.b) + (static_cast<float>(b.b) - static_cast<float>(a.b)) * t, 0.0f, 255.0f)),
                static_cast<unsigned char>(std::clamp(static_cast<float>(a.a) + (static_cast<float>(b.a) - static_cast<float>(a.a)) * t, 0.0f, 255.0f))
            };
        };
        for (int ring = 0; ring < kRings; ++ring) {
            float r0 = maxR * static_cast<float>(ring) / static_cast<float>(kRings);
            float r1 = maxR * static_cast<float>(ring + 1) / static_cast<float>(kRings);
            float mid = (r0 + r1) * 0.5f / std::max(1.0f, maxR);
            Color inner = mixColor(Color{218, 214, 150, 255}, Color{82, 82, 82, 255}, apocalypse);
            Color outer = mixColor(Color{235, 224, 194, 255}, Color{132, 128, 120, 255}, apocalypse);
            Color fill = Color{
                static_cast<unsigned char>(static_cast<float>(inner.r) + (static_cast<float>(outer.r) - inner.r) * mid),
                static_cast<unsigned char>(static_cast<float>(inner.g) + (static_cast<float>(outer.g) - inner.g) * mid),
                static_cast<unsigned char>(static_cast<float>(inner.b) + (static_cast<float>(outer.b) - inner.b) * mid),
                255
            };
            for (int seg = 0; seg < kSegments; ++seg) {
                float a0 = static_cast<float>(seg) / static_cast<float>(kSegments) * 2.0f * PI;
                float a1 = static_cast<float>(seg + 1) / static_cast<float>(kSegments) * 2.0f * PI;
                Vector3 p00{std::cos(a0) * r0, 0.0f, std::sin(a0) * r0};
                Vector3 p01{std::cos(a1) * r0, 0.0f, std::sin(a1) * r0};
                Vector3 p10{std::cos(a0) * r1, 0.0f, std::sin(a0) * r1};
                Vector3 p11{std::cos(a1) * r1, 0.0f, std::sin(a1) * r1};
                p00.y = EdenGroundYAt(p00);
                p01.y = EdenGroundYAt(p01);
                p10.y = EdenGroundYAt(p10);
                p11.y = EdenGroundYAt(p11);
                DrawTriangle3D(p00, p10, p11, fill);
                DrawTriangle3D(p00, p11, p01, fill);
                DrawTriangle3D(p11, p10, p00, fill);
                DrawTriangle3D(p01, p11, p00, fill);
            }
        }
        for (int river = 0; river < 4; ++river) {
            float baseAngle = static_cast<float>(river) * PI * 0.5f + PI * 0.25f;
            Vector3 prevLeft{};
            Vector3 prevRight{};
            bool hasPrev = false;
            for (int step = 0; step <= 56; ++step) {
                float t = static_cast<float>(step) / 56.0f;
                float radius = 3.5f + t * (visualMapRadius - 7.0f);
                float bend = std::sin(t * PI * 2.0f + static_cast<float>(river) * 0.7f) * 0.08f;
                float angle = baseAngle + bend;
                Vector3 radial{std::cos(angle), 0.0f, std::sin(angle)};
                Vector3 tangent{-radial.z, 0.0f, radial.x};
                float width = 0.65f + t * 1.35f;
                Vector3 center = Vector3Scale(radial, radius);
                center.y = EdenGroundYAt(center) + 0.055f;
                Vector3 left = Vector3Add(center, Vector3Scale(tangent, width));
                Vector3 right = Vector3Subtract(center, Vector3Scale(tangent, width));
                left.y = EdenGroundYAt(left) + 0.06f;
                right.y = EdenGroundYAt(right) + 0.06f;
                if (hasPrev) {
                    Color waterBase = mixColor(Color{166, 226, 255, 255}, Color{82, 84, 88, 255}, apocalypse);
                    Color bankBase = mixColor(Color{255, 244, 186, 255}, Color{118, 112, 104, 255}, apocalypse);
                    Color shimmerBase = mixColor(Color{255, 255, 230, 255}, Color{145, 138, 128, 255}, apocalypse);
                    Color water = FadeColor(waterBase, 0.34f + 0.16f * std::sin(t * PI));
                    DrawTriangle3D(prevLeft, left, right, water);
                    DrawTriangle3D(prevLeft, right, prevRight, water);
                    DrawLine3D(prevLeft, left, FadeColor(bankBase, 0.42f));
                    DrawLine3D(prevRight, right, FadeColor(bankBase, 0.42f));
                    DrawLine3D(Vector3Scale(Vector3Add(prevLeft, prevRight), 0.5f),
                        Vector3Scale(Vector3Add(left, right), 0.5f),
                        FadeColor(shimmerBase, 0.25f));
                }
                prevLeft = left;
                prevRight = right;
                hasPrev = true;
            }
        }
        for (int ring = 1; ring <= 6; ++ring) {
            float radius = config_.edenPlayRadius * static_cast<float>(ring) / 6.0f;
            Vector3 prev{};
            for (int i = 0; i <= kSegments; ++i) {
                float a = static_cast<float>(i) / static_cast<float>(kSegments) * 2.0f * PI;
                Vector3 p{std::cos(a) * radius, 0.0f, std::sin(a) * radius};
                p.y = EdenGroundYAt(p) + 0.035f;
                Color ringColor = mixColor(Color{255, 238, 174, 255}, Color{130, 122, 112, 255}, apocalypse);
                if (i > 0) DrawLine3D(prev, p, FadeColor(ringColor, 0.35f));
                prev = p;
            }
        }
        Vector3 center{0.0f, EdenHeightAt(0.0f, 0.0f) + 0.25f, 0.0f};
        DrawCylinderWires(center, 2.0f, 2.0f, 0.04f, 36, FadeColor(mixColor(Color{255, 245, 190, 255}, Color{126, 118, 108, 255}, apocalypse), 0.65f));
        DrawSphereEx(Vector3Add(center, Vector3{0.0f, 1.4f, 0.0f}), 0.35f, 10, 8, FadeColor(mixColor(Color{255, 248, 190, 255}, Color{150, 140, 128, 255}, apocalypse), 0.72f));

        float time = static_cast<float>(GetTime());
        auto edenNoise = [](float seed) {
            return std::fmod(std::sin(seed * 12.9898f + 78.233f) * 43758.5453f + 43758.5453f, 1.0f);
        };
        auto drawEdenTree = [&](Vector3 root, float height, float canopyRadius, bool lifeTree, float seed) {
            root.y = EdenGroundYAt(root) + 0.03f;
            Vector3 top = Vector3Add(root, Vector3{0.0f, height, 0.0f});
            float pulse = 0.5f + 0.5f * std::sin(time * (lifeTree ? 0.72f : 0.38f) + seed);
            Color trunk = lifeTree
                ? mixColor(Color{172, 126, 58, 255}, Color{92, 82, 78, 255}, apocalypse)
                : mixColor(Color{112, 88, 52, 255}, Color{78, 72, 70, 255}, apocalypse);
            Color leaf = lifeTree
                ? mixColor(Color{116, 226, 126, 255}, Color{126, 118, 106, 255}, apocalypse)
                : mixColor(Color{142, 178, 98, 255}, Color{112, 108, 98, 255}, apocalypse);
            Color gold = mixColor(Color{255, 236, 128, 255}, Color{166, 130, 96, 255}, apocalypse);
            DrawCylinderEx(root, top, height * (lifeTree ? 0.055f : 0.038f), height * (lifeTree ? 0.026f : 0.018f),
                lifeTree ? 9 : 6, trunk);
            int branches = lifeTree ? 9 : 5;
            for (int b = 0; b < branches; ++b) {
                float a = seed + static_cast<float>(b) / static_cast<float>(branches) * 2.0f * PI + std::sin(time * 0.18f + seed) * 0.04f;
                float h = height * (0.42f + 0.42f * edenNoise(seed + static_cast<float>(b) * 3.1f));
                Vector3 start = Vector3Add(root, Vector3{0.0f, h, 0.0f});
                Vector3 dir{std::cos(a), 0.18f + edenNoise(seed + b) * 0.25f, std::sin(a)};
                dir = SafeNormalize(dir, Vector3{1.0f, 0.2f, 0.0f});
                Vector3 end = Vector3Add(start, Vector3Scale(dir, canopyRadius * (lifeTree ? 1.08f : 0.72f)));
                DrawCylinderEx(start, end, height * 0.012f, height * 0.004f, 5, trunk);
                if (lifeTree) {
                    DrawSphereEx(end, 0.42f + pulse * 0.12f, 8, 5, FadeColor(gold, 0.80f));
                }
            }
            DrawSphereEx(Vector3Add(top, Vector3{0.0f, lifeTree ? 0.8f : 0.2f, 0.0f}),
                canopyRadius, lifeTree ? 14 : 8, lifeTree ? 9 : 6, FadeColor(leaf, lifeTree ? 0.72f : 0.58f));
            DrawSphereEx(Vector3Add(top, Vector3{canopyRadius * 0.42f, -canopyRadius * 0.22f, 0.0f}),
                canopyRadius * 0.72f, 8, 6, FadeColor(leaf, lifeTree ? 0.58f : 0.46f));
            DrawSphereEx(Vector3Add(top, Vector3{-canopyRadius * 0.34f, -canopyRadius * 0.12f, canopyRadius * 0.32f}),
                canopyRadius * 0.62f, 8, 6, FadeColor(leaf, lifeTree ? 0.50f : 0.40f));
            if (lifeTree) {
                DrawSphereWires(Vector3Add(top, Vector3{0.0f, 0.8f, 0.0f}), canopyRadius * (1.18f + pulse * 0.04f),
                    16, 10, FadeColor(gold, 0.42f));
                DrawCircle3D(Vector3Add(top, Vector3{0.0f, 1.2f, 0.0f}), canopyRadius * 1.35f,
                    SafeNormalize(Vector3{0.35f, 1.0f, 0.18f}, Vector3{0.0f, 1.0f, 0.0f}), 90.0f,
                    FadeColor(Color{255, 246, 164, 255}, 0.36f + pulse * 0.16f));
            }
        };

        Vector3 lifeTreeRoot = EdenTreePosition(0);
        drawEdenTree(lifeTreeRoot, 18.0f, 5.4f, true, 9.0f);

        for (int i = 1; i <= 72; ++i) {
            float seed = static_cast<float>(i) + 41.0f;
            Vector3 root = EdenTreePosition(i);
            drawEdenTree(root, 5.0f + edenNoise(seed + 2.0f) * 6.0f,
                1.6f + edenNoise(seed + 4.0f) * 1.5f, false, seed);
        }

        for (int i = 0; i < 8; ++i) {
            float angle = static_cast<float>(i) / 8.0f * 2.0f * PI + 0.18f;
            float radius = config_.edenPlayRadius * 0.52f;
            Vector3 base{std::cos(angle) * radius, 0.0f, std::sin(angle) * radius};
            base.y = EdenGroundYAt(base) + 2.6f;
            Color stone = mixColor(Color{212, 205, 180, 255}, Color{92, 88, 84, 255}, apocalypse);
            Color rune = mixColor(Color{255, 232, 126, 255}, Color{174, 94, 70, 255}, apocalypse);
            DrawCube(base, 1.4f, 5.2f, 0.72f, FadeColor(stone, 0.82f));
            DrawCubeWires(base, 1.46f, 5.28f, 0.78f, FadeColor(Color{255, 246, 198, 255}, 0.28f));
            Vector3 up{0.0f, 1.0f, 0.0f};
            Vector3 side{-std::sin(angle), 0.0f, std::cos(angle)};
            Vector3 face = SafeNormalize(Vector3Scale(base, -1.0f), Vector3{0.0f, 0.0f, -1.0f});
            Vector3 front = Vector3Add(base, Vector3Scale(face, 0.43f));
            for (int mark = 0; mark < 3; ++mark) {
                Vector3 a = Vector3Add(front, Vector3Add(Vector3Scale(up, -1.45f + mark * 1.15f), Vector3Scale(side, -0.32f)));
                Vector3 b = Vector3Add(front, Vector3Add(Vector3Scale(up, -1.02f + mark * 1.15f), Vector3Scale(side, 0.32f)));
                DrawLine3D(a, b, FadeColor(rune, 0.74f));
            }
        }

        for (int i = 0; i < 37; ++i) {
            float seed = static_cast<float>(i) + 71.0f;
            Vector3 pos = EdenFloatingStonePosition(i);
            float cityScale = i == 0 ? 7.4f : (3.3f + static_cast<float>((i - 1) / 12) * 0.88f);
            float size = (1.05f + edenNoise(seed + 2.0f) * 1.35f) * cityScale;
            Color stone = mixColor(Color{236, 226, 190, 255}, Color{116, 104, 98, 255}, apocalypse);
            DrawCube(pos, size * 2.8f, size * 0.76f, size * 2.1f, FadeColor(stone, 0.68f));
            DrawCubeWires(pos, size * 2.86f, size * 0.80f, size * 2.16f, FadeColor(Color{255, 246, 208, 255}, 0.26f));
            DrawCircle3D(pos, size * 2.0f, SafeNormalize(Vector3{0.2f, 1.0f, 0.35f}, Vector3{0.0f, 1.0f, 0.0f}),
                90.0f, FadeColor(mixColor(Color{210, 240, 255, 255}, Color{148, 122, 118, 255}, apocalypse), 0.28f));
            if (i % 5 == 0) {
                Vector3 spireBase = Vector3Add(pos, Vector3{0.0f, size * 0.48f, 0.0f});
                Vector3 spireTip = Vector3Add(pos, Vector3{0.0f, size * (2.8f + edenNoise(seed) * 1.2f), 0.0f});
                DrawCylinderEx(spireBase, spireTip, size * 0.18f, 0.0f, 5,
                    FadeColor(mixColor(Color{255, 238, 178, 255}, Color{128, 108, 100, 255}, apocalypse), 0.58f));
            }
            if (i == 0 || i % 3 != 1) {
                int houseCount = i == 0 ? 5 : 1 + (i % 3);
                for (int h = 0; h < houseCount; ++h) {
                    float houseSeed = seed + static_cast<float>(h) * 9.7f;
                    float angle = houseSeed * 2.17f;
                    float offsetRadius = size * (i == 0 ? (0.45f + 0.18f * static_cast<float>(h)) : 0.34f);
                    Vector3 houseBase = Vector3Add(pos, Vector3{std::cos(angle) * offsetRadius, size * 0.62f, std::sin(angle) * offsetRadius});
                    float w = std::max(3.6f, size * (0.74f + edenNoise(houseSeed) * 0.26f));
                    float d = std::max(3.2f, size * (0.66f + edenNoise(houseSeed + 2.0f) * 0.24f));
                    float houseH = std::max(2.8f, size * (0.48f + edenNoise(houseSeed + 4.0f) * 0.24f));
                    Color wall = mixColor(Color{198, 188, 156, 255}, Color{104, 98, 90, 255}, apocalypse);
                    Color roofStone = mixColor(Color{156, 146, 124, 255}, Color{86, 82, 76, 255}, apocalypse);
                    float wallT = std::max(0.18f, std::min(w, d) * 0.13f);
                    float doorW = std::min(w * 0.46f, 2.2f);
                    float doorH = std::min(houseH * 0.76f, 2.45f);
                    DrawCube(Vector3Add(houseBase, Vector3{-w * 0.5f + wallT * 0.5f, houseH * 0.5f, 0.0f}),
                        wallT, houseH, d, FadeColor(wall, 0.82f));
                    DrawCube(Vector3Add(houseBase, Vector3{w * 0.5f - wallT * 0.5f, houseH * 0.5f, 0.0f}),
                        wallT, houseH, d, FadeColor(wall, 0.82f));
                    DrawCube(Vector3Add(houseBase, Vector3{0.0f, houseH * 0.5f, -d * 0.5f + wallT * 0.5f}),
                        w, houseH, wallT, FadeColor(wall, 0.82f));
                    DrawCube(Vector3Add(houseBase, Vector3{-(w + doorW) * 0.25f, doorH * 0.5f, d * 0.5f - wallT * 0.5f}),
                        (w - doorW) * 0.5f, doorH, wallT, FadeColor(wall, 0.82f));
                    DrawCube(Vector3Add(houseBase, Vector3{(w + doorW) * 0.25f, doorH * 0.5f, d * 0.5f - wallT * 0.5f}),
                        (w - doorW) * 0.5f, doorH, wallT, FadeColor(wall, 0.82f));
                    DrawCube(Vector3Add(houseBase, Vector3{0.0f, doorH + (houseH - doorH) * 0.5f, d * 0.5f - wallT * 0.5f}),
                        w, houseH - doorH, wallT, FadeColor(wall, 0.82f));
                    DrawCubeWires(Vector3Add(houseBase, Vector3{0.0f, houseH * 0.5f, 0.0f}), w * 1.03f, houseH * 1.04f, d * 1.03f,
                        FadeColor(Color{72, 64, 54, 255}, 0.34f));
                    DrawCube(Vector3Add(houseBase, Vector3{0.0f, houseH + size * 0.08f, 0.0f}), w * 1.16f, size * 0.16f, d * 1.16f,
                        FadeColor(roofStone, 0.86f));
                    DrawCube(Vector3Add(houseBase, Vector3{0.0f, doorH * 0.46f, d * 0.51f}), doorW * 0.86f, doorH * 0.86f, size * 0.035f,
                        FadeColor(Color{32, 24, 20, 255}, 0.66f));
                    if (h % 2 == 0) {
                        DrawLine3D(Vector3Add(houseBase, Vector3{-w * 0.32f, houseH * 0.68f, d * 0.53f}),
                            Vector3Add(houseBase, Vector3{w * 0.32f, houseH * 0.68f, d * 0.53f}),
                            FadeColor(Color{255, 236, 178, 255}, 0.32f));
                    }
                }
            }
        }

        DrawEdenArk();

        if (!edenForbiddenFruit_.claimed) {
        for (int i = 0; i < 18; ++i) {
            float seed = static_cast<float>(i);
            float angle = seed * 2.39996323f + 0.18f * std::sin(time * 0.11f + seed);
            float radius = std::sqrt(std::fmod(std::sin(seed * 17.31f) * 43758.5453f + 43758.5453f, 1.0f))
                * (config_.edenPlayRadius * 0.78f - 16.0f) + 16.0f;
            Vector3 pos{std::cos(angle) * radius, 0.0f, std::sin(angle) * radius};
            pos.y = EdenGroundYAt(pos) + 13.5f + 4.2f * std::sin(time * 0.22f + seed * 0.9f);
            Vector3 toCamera = SafeNormalize(Vector3Subtract(camera_.position, pos), Vector3{0.0f, 0.0f, -1.0f});
            Vector3 right = SafeNormalize(Vector3CrossProduct(Vector3{0.0f, 1.0f, 0.0f}, toCamera), Vector3{1.0f, 0.0f, 0.0f});
            Vector3 up = SafeNormalize(Vector3CrossProduct(toCamera, right), Vector3{0.0f, 1.0f, 0.0f});
            Color body = FadeColor(Color{255, 236, 192, 255}, 0.78f);
            DrawSphereEx(pos, 0.52f, 8, 6, body);
            DrawSphereEx(pos, 0.95f, 8, 6, FadeColor(Color{255, 214, 116, 255}, 0.18f));
            Vector3 ringNormal = SafeNormalize(Vector3Add(Vector3Scale(toCamera, 0.52f), Vector3Scale(up, 0.82f)), up);
            DrawCircle3D(pos, 1.12f, RotateAroundAxis(ringNormal, toCamera, time * 0.18f + seed), 90.0f,
                FadeColor(Color{255, 228, 148, 255}, 0.58f));
            DrawLine3D(Vector3Subtract(pos, Vector3Scale(up, 0.9f)), Vector3Add(pos, Vector3Scale(up, 0.9f)),
                FadeColor(Color{255, 248, 214, 255}, 0.32f));
            DrawLine3D(Vector3Subtract(pos, Vector3Scale(right, 0.62f)), Vector3Add(pos, Vector3Scale(right, 0.62f)),
                FadeColor(Color{255, 226, 166, 255}, 0.24f));
        }

        for (int i = 0; i < 12; ++i) {
            float seed = static_cast<float>(i) + 31.0f;
            float angle = seed * 1.914f;
            float radius = config_.edenPlayRadius * (0.34f + 0.52f * std::fmod(std::sin(seed * 5.17f) * 12345.67f + 12345.67f, 1.0f));
            Vector3 pos{std::cos(angle) * radius, 0.0f, std::sin(angle) * radius};
            pos.y = EdenGroundYAt(pos) + 0.35f;
            Vector3 toCamera = SafeNormalize(Vector3Subtract(camera_.position, pos), Vector3{0.0f, 0.0f, -1.0f});
            Vector3 right = SafeNormalize(Vector3CrossProduct(Vector3{0.0f, 1.0f, 0.0f}, toCamera), Vector3{1.0f, 0.0f, 0.0f});
            Vector3 up = Vector3{0.0f, 1.0f, 0.0f};
            float bob = std::sin(time * 0.55f + seed) * 0.18f;
            if (i % 3 == 0) {
                Vector3 body = Vector3Add(pos, Vector3Scale(up, 0.75f + bob));
                DrawSphereEx(body, 0.42f, 8, 5, FadeColor(Color{236, 224, 170, 255}, 0.62f));
                DrawSphereEx(Vector3Add(body, Vector3Scale(up, 0.52f)), 0.24f, 6, 4, FadeColor(Color{255, 244, 205, 255}, 0.70f));
                DrawLine3D(Vector3Subtract(pos, Vector3Scale(right, 0.85f)), Vector3Add(pos, Vector3Scale(right, 0.85f)),
                    FadeColor(Color{255, 246, 210, 255}, 0.36f));
            } else {
                Vector3 body = Vector3Add(pos, Vector3Scale(up, 0.45f + bob));
                DrawSphereEx(body, 0.28f, 6, 4, FadeColor(Color{230, 214, 170, 255}, 0.55f));
                DrawCircle3D(Vector3Add(body, Vector3Scale(up, 0.08f)), 0.52f,
                    SafeNormalize(Vector3Add(toCamera, Vector3Scale(up, 0.35f)), toCamera), 90.0f,
                    FadeColor(Color{255, 238, 170, 255}, 0.32f));
            }
        }
        for (int herd = 0; herd < 4; ++herd) {
            float herdAngle = static_cast<float>(herd) * PI * 0.5f + time * 0.035f;
            Vector3 herdCenter{std::cos(herdAngle) * config_.edenPlayRadius * 0.42f, 0.0f,
                std::sin(herdAngle) * config_.edenPlayRadius * 0.42f};
            for (int j = 0; j < 5; ++j) {
                float seed = static_cast<float>(herd * 7 + j) + 103.0f;
                Vector3 offset{std::cos(seed * 2.17f) * (2.5f + j * 0.42f), 0.0f,
                    std::sin(seed * 1.83f) * (2.0f + j * 0.36f)};
                Vector3 pos = Vector3Add(herdCenter, offset);
                pos.y = EdenGroundYAt(pos) + 0.85f;
                Vector3 right = SafeNormalize(Vector3{std::cos(herdAngle + PI * 0.5f), 0.0f, std::sin(herdAngle + PI * 0.5f)},
                    Vector3{1.0f, 0.0f, 0.0f});
                Vector3 forward = SafeNormalize(Vector3{std::cos(herdAngle), 0.0f, std::sin(herdAngle)}, Vector3{0.0f, 0.0f, 1.0f});
                Color bodyColor = FadeColor(Color{244, 226, 178, 255}, 0.54f);
                Vector3 body = Vector3Add(pos, Vector3{0.0f, 0.45f + std::sin(time * 0.8f + seed) * 0.05f, 0.0f});
                DrawCylinderEx(Vector3Subtract(body, Vector3Scale(forward, 0.46f)),
                    Vector3Add(body, Vector3Scale(forward, 0.58f)), 0.28f, 0.20f, 6, bodyColor);
                DrawSphereEx(Vector3Add(body, Vector3Scale(forward, 0.78f)), 0.22f, 6, 4, FadeColor(Color{255, 240, 198, 255}, 0.62f));
                DrawLine3D(Vector3Add(body, Vector3Scale(right, 0.18f)), Vector3Add(pos, Vector3Add(Vector3Scale(right, 0.28f), Vector3{0.0f, -0.34f, 0.0f})),
                    FadeColor(Color{255, 242, 204, 255}, 0.36f));
                DrawLine3D(Vector3Subtract(body, Vector3Scale(right, 0.18f)), Vector3Add(pos, Vector3Add(Vector3Scale(right, -0.28f), Vector3{0.0f, -0.34f, 0.0f})),
                    FadeColor(Color{255, 242, 204, 255}, 0.36f));
            }
        }
        for (int flock = 0; flock < 3; ++flock) {
            float baseAngle = static_cast<float>(flock) * 2.1f + time * 0.09f;
            Vector3 flockCenter{std::cos(baseAngle) * config_.edenPlayRadius * 0.34f, 0.0f,
                std::sin(baseAngle) * config_.edenPlayRadius * 0.34f};
            flockCenter.y = EdenGroundYAt(flockCenter) + 26.0f + static_cast<float>(flock) * 5.0f;
            for (int j = 0; j < 9; ++j) {
                float seed = static_cast<float>(flock * 11 + j) + 151.0f;
                Vector3 pos = Vector3Add(flockCenter, Vector3{std::cos(seed) * (2.0f + j * 0.18f),
                    std::sin(time * 0.42f + seed) * 1.0f,
                    std::sin(seed * 1.4f) * (1.8f + j * 0.16f)});
                Vector3 wing = Vector3{0.72f + 0.22f * std::sin(time * 2.2f + seed), 0.0f, 0.0f};
                DrawSphereEx(pos, 0.16f, 5, 3, FadeColor(Color{255, 232, 172, 255}, 0.52f));
                DrawLine3D(Vector3Subtract(pos, wing), Vector3Add(pos, wing), FadeColor(Color{255, 246, 214, 255}, 0.48f));
            }
        }
        for (int stream = 0; stream < 4; ++stream) {
            float riverAngle = static_cast<float>(stream) * PI * 0.5f + PI * 0.25f;
            for (int j = 0; j < 8; ++j) {
                float t = (static_cast<float>(j) + std::fmod(time * 0.12f + stream * 0.19f, 1.0f)) / 8.0f;
                float radius = config_.edenPlayRadius * (0.12f + t * 0.72f);
                float bend = std::sin(t * PI * 2.0f + static_cast<float>(stream) * 0.7f) * 0.08f;
                Vector3 radial{std::cos(riverAngle + bend), 0.0f, std::sin(riverAngle + bend)};
                Vector3 pos = Vector3Scale(radial, radius);
                pos.y = EdenGroundYAt(pos) + 0.34f;
                Vector3 tail = Vector3Subtract(pos, Vector3Scale(radial, 0.72f));
                DrawSphereEx(pos, 0.18f, 5, 3, FadeColor(Color{178, 232, 255, 255}, 0.50f));
                DrawLine3D(tail, pos, FadeColor(Color{220, 248, 255, 255}, 0.42f));
            }
        }
        }

        auto drawGuardian = [&](Vector3 pos, Vector3 radial, float phase) {
            pos.y = EdenGroundYAt(Vector3Scale(radial, config_.edenMapRadius * 0.98f));
            Vector3 right{-radial.z, 0.0f, radial.x};
            Vector3 up{0.0f, 1.0f, 0.0f};
            Vector3 torsoTop = Vector3Add(pos, Vector3{0.0f, 64.0f + std::sin(phase) * 1.2f, 0.0f});
            Vector3 head = Vector3Add(torsoTop, Vector3{0.0f, 12.0f, 0.0f});
            Color silhouette = edenForbiddenFruit_.claimed
                ? FadeColor(Color{255, 192, 82, 255}, 0.70f)
                : FadeColor(Color{26, 17, 48, 255}, 0.52f);
            Color edge = edenForbiddenFruit_.claimed
                ? FadeColor(Color{255, 246, 184, 255}, 0.76f)
                : FadeColor(Color{255, 230, 166, 255}, 0.34f);
            Color outerFlame = edenForbiddenFruit_.claimed
                ? FadeColor(Color{255, 78, 28, 255}, 0.54f)
                : FadeColor(Color{255, 170, 90, 255}, 0.20f);
            DrawCylinderEx(pos, torsoTop, 7.0f, 13.0f, 10, silhouette);
            if (edenForbiddenFruit_.claimed) {
                DrawCylinderEx(Vector3Add(pos, Vector3Scale(radial, -0.8f)),
                    Vector3Add(torsoTop, Vector3Scale(radial, -2.2f)),
                    9.5f, 17.5f, 10, FadeColor(Color{255, 86, 22, 255}, 0.26f));
            }
            DrawSphereEx(head, 8.5f, 12, 8, silhouette);
            DrawSphereEx(head, 12.0f, 12, 8, FadeColor(Color{255, 224, 128, 255}, edenForbiddenFruit_.claimed ? 0.20f : 0.08f));
            DrawLine3D(Vector3Add(torsoTop, Vector3Scale(radial, -8.0f)), Vector3Add(torsoTop, Vector3Scale(radial, 14.0f)), edge);
            if (edenForbiddenFruit_.claimed) {
                Vector3 wingRoot = Vector3Add(torsoTop, Vector3Add(Vector3Scale(radial, 4.0f), Vector3Scale(up, -5.0f)));
                float wingBeat = std::sin(phase * 1.15f) * 3.5f;
                for (int side = -1; side <= 1; side += 2) {
                    for (int row = -1; row <= 1; row += 2) {
                        Vector3 spread = SafeNormalize(Vector3Add(Vector3Scale(right, side * 1.0f), Vector3Scale(up, row * 0.42f)), right);
                        Vector3 tangent = SafeNormalize(Vector3Add(Vector3Scale(up, row * 1.0f), Vector3Scale(right, -side * 0.32f)), up);
                        Vector3 root = Vector3Add(wingRoot, Vector3Scale(up, row * 7.0f));
                        Vector3 tip = Vector3Add(root, Vector3Add(Vector3Scale(spread, 58.0f + wingBeat), Vector3Scale(radial, 12.0f)));
                        Vector3 mid = Vector3Add(root, Vector3Add(Vector3Scale(spread, 31.0f + wingBeat * 0.45f), Vector3Scale(radial, 6.0f)));
                        Vector3 a = Vector3Add(mid, Vector3Scale(tangent, 11.0f));
                        Vector3 b = Vector3Subtract(mid, Vector3Scale(tangent, 11.0f));
                        DrawTriangle3D(root, a, tip, outerFlame);
                        DrawTriangle3D(root, tip, b, outerFlame);
                        DrawTriangle3D(root, tip, a, outerFlame);
                        DrawTriangle3D(root, b, tip, outerFlame);
                        DrawLine3D(root, tip, edge);
                    }
                }
            }
            Vector3 swordBase = Vector3Add(Vector3Add(pos, Vector3Scale(right, 22.0f)), Vector3{0.0f, 8.0f, 0.0f});
            Vector3 swordTip = Vector3Add(Vector3Add(pos, Vector3Scale(right, 22.0f)), Vector3{0.0f, 96.0f, 0.0f});
            if (edenForbiddenFruit_.claimed) {
                float swing = std::sin(phase * 1.7f) * 12.0f;
                swordBase = Vector3Add(swordBase, Vector3Scale(radial, swing));
                swordTip = Vector3Add(swordTip, Vector3Scale(radial, -swing * 0.35f));
            }
            DrawCylinderEx(swordBase, swordTip, 1.0f, 2.8f, 7, FadeColor(Color{255, 190, 54, 255}, edenForbiddenFruit_.claimed ? 0.62f : 0.40f));
            DrawCylinderEx(Vector3Add(swordBase, Vector3Scale(radial, 0.7f)), Vector3Add(swordTip, Vector3Scale(radial, 6.5f)),
                2.2f, 7.0f, 8, FadeColor(Color{255, 86, 22, 255}, edenForbiddenFruit_.claimed ? 0.52f : 0.30f));
            DrawCylinderEx(Vector3Subtract(swordBase, Vector3Scale(radial, 0.5f)), Vector3Subtract(swordTip, Vector3Scale(radial, 4.2f)),
                1.3f, 4.6f, 8, FadeColor(Color{255, 236, 110, 255}, edenForbiddenFruit_.claimed ? 0.42f : 0.27f));
            DrawLine3D(Vector3Subtract(swordTip, Vector3Scale(up, 18.0f)), swordTip, FadeColor(Color{255, 248, 190, 255}, 0.68f));
        };

        if (edenForbiddenFruit_.claimed && !edenGuardians_.empty()) {
            for (const EdenGuardian& guardian : edenGuardians_) {
                drawGuardian(guardian.position, guardian.radial, guardian.stepPhase);
            }
        } else {
        for (int i = 0; i < 4; ++i) {
            float angle = static_cast<float>(i) / 4.0f * 2.0f * PI + 0.38f;
            Vector3 radial{std::cos(angle), 0.0f, std::sin(angle)};
            Vector3 pos = Vector3Scale(radial, visualMapRadius);
            drawGuardian(pos, radial, time * 0.25f + static_cast<float>(i));
        }
        }
        return;
    }

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

    if (IsLabyrinthMap()) {
        float cs = config_.labyrinthCellSize;
        float width = static_cast<float>(labyrinthWidth_) * cs;
        float height = static_cast<float>(labyrinthHeight_) * cs;
        float groundY = FlatGroundYForWorld(playerWorld_);
        Color floor = Color{26, 24, 23, 255};
        Color grid = Color{82, 68, 56, 150};
        Color border = Color{156, 118, 72, 210};
        DrawCube(Vector3{0.0f, groundY - 0.08f, 0.0f}, width + cs, 0.18f, height + cs, floor);
        DrawCubeWires(Vector3{0.0f, groundY + 0.02f, 0.0f}, width + cs, 0.2f, height + cs, border);
        float halfX = static_cast<float>(labyrinthWidth_ - 1) * cs * 0.5f;
        float halfZ = static_cast<float>(labyrinthHeight_ - 1) * cs * 0.5f;
        float y = groundY + 0.035f;
        for (int x = 0; x < labyrinthWidth_; ++x) {
            float px = (static_cast<float>(x) - static_cast<float>(labyrinthWidth_ - 1) * 0.5f) * cs;
            DrawLine3D(Vector3{px, y, -halfZ}, Vector3{px, y, halfZ}, grid);
        }
        for (int z = 0; z < labyrinthHeight_; ++z) {
            float pz = (static_cast<float>(z) - static_cast<float>(labyrinthHeight_ - 1) * 0.5f) * cs;
            DrawLine3D(Vector3{-halfX, y, pz}, Vector3{halfX, y, pz}, grid);
        }
        Vector3 entrance = LabyrinthCellCenter(1, 1);
        entrance.y = y + 0.03f;
        DrawCircle3D(entrance, cs * 0.34f, Vector3{0.0f, 1.0f, 0.0f}, 90.0f, Color{220, 180, 96, 210});
        Vector3 heart = LabyrinthCellCenter(labyrinthWidth_ - 2, labyrinthHeight_ - 2);
        heart.y = y + 0.04f;
        DrawCircle3D(heart, cs * 0.42f, Vector3{0.0f, 1.0f, 0.0f}, 90.0f, Color{180, 55, 38, 190});

        if (labyrinthShiftWarning_ && !labyrinthPendingGrid_.empty()) {
            float pulse = 0.45f + 0.25f * std::sin(static_cast<float>(GetTime()) * 9.0f);
            for (int gy = 0; gy < labyrinthHeight_; ++gy) {
                for (int gx = 0; gx < labyrinthWidth_; ++gx) {
                    bool nowOpen = LabyrinthCellOpen(gx, gy);
                    bool nextOpen = LabyrinthCellOpen(gx, gy, &labyrinthPendingGrid_);
                    if (nowOpen == nextOpen) continue;
                    Vector3 center = LabyrinthCellCenter(gx, gy);
                    center.y = config_.labyrinthWallHeight * 0.5f;
                    Color ghost = nextOpen
                        ? FadeColor(Color{255, 206, 96, 255}, 0.18f + pulse * 0.22f)
                        : FadeColor(Color{255, 68, 38, 255}, 0.24f + pulse * 0.28f);
                    DrawCube(center, cs * 0.78f, config_.labyrinthWallHeight, cs * 0.78f, ghost);
                    DrawCubeWires(center, cs * 0.78f, config_.labyrinthWallHeight, cs * 0.78f,
                        FadeColor(Color{255, 226, 128, 255}, 0.55f));
                }
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
        } else if (enemy.type == EnemyType::Minotaur) {
            float r = enemy.radius;
            bool charging = enemy.burstCount == 1 && enemy.burstTimer > 0.0f;
            Color bronze = charging ? Color{160, 82, 38, 255} : Color{92, 62, 42, 255};
            Color dark = Color{32, 24, 20, 255};
            Color horn = Color{224, 198, 148, 255};
            DrawCube(Vector3{0.0f, -r * 0.08f, 0.0f}, r * 1.28f, r * 1.55f, r * 0.92f, bronze);
            DrawCubeWires(Vector3{0.0f, -r * 0.08f, 0.0f}, r * 1.36f, r * 1.64f, r * 1.00f, dark);
            DrawSphereEx(Vector3{0.0f, r * 0.92f, 0.0f}, r * 0.58f, 8, 6, Color{74, 46, 32, 255});
            DrawCube(Vector3{0.0f, r * 0.86f, -r * 0.48f}, r * 0.56f, r * 0.34f, r * 0.58f, Color{48, 30, 24, 255});
            DrawSphereEx(Vector3{-r * 0.22f, r * 1.02f, -r * 0.50f}, r * 0.08f, 5, 4, Color{255, 28, 20, 255});
            DrawSphereEx(Vector3{ r * 0.22f, r * 1.02f, -r * 0.50f}, r * 0.08f, 5, 4, Color{255, 28, 20, 255});
            Vector3 leftHornBase{-r * 0.34f, r * 1.16f, -r * 0.10f};
            Vector3 rightHornBase{r * 0.34f, r * 1.16f, -r * 0.10f};
            DrawCylinderEx(leftHornBase, Vector3{-r * 0.94f, r * 1.45f, -r * 0.16f}, r * 0.10f, r * 0.02f, 6, horn);
            DrawCylinderEx(rightHornBase, Vector3{r * 0.94f, r * 1.45f, -r * 0.16f}, r * 0.10f, r * 0.02f, 6, horn);
            for (int side = -1; side <= 1; side += 2) {
                DrawCube(Vector3{side * r * 0.72f, r * 0.15f, 0.0f}, r * 0.24f, r * 1.2f, r * 0.24f, dark);
                DrawCube(Vector3{side * r * 0.32f, -r * 1.0f, 0.0f}, r * 0.28f, r * 0.82f, r * 0.28f, dark);
            }
            Vector3 axeBase{r * 0.88f, -r * 0.82f, r * 0.16f};
            Vector3 axeTip{r * 1.38f, r * 1.15f, -r * 0.22f};
            DrawCylinderEx(axeBase, axeTip, r * 0.05f, r * 0.08f, 6, Color{70, 56, 46, 255});
            DrawCube(Vector3{r * 1.42f, r * 1.18f, -r * 0.22f}, r * 0.52f, r * 0.30f, r * 0.18f, Color{142, 126, 104, 255});
            DrawSphereWires(Vector3Zero(), r * (charging ? 1.62f : 1.28f), 10, 7,
                charging ? Color{255, 80, 38, 230} : Color{158, 104, 62, 190});
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

        if (enemy.warCommandTimer > 0.0f) {
            float commandPulse = 1.0f + std::sin(static_cast<float>(GetTime()) * 13.0f + enemy.bobTimer) * 0.12f;
            DrawSphereWires(Vector3Zero(), enemy.radius * 1.72f * commandPulse, 9, 6, Color{255, 72, 28, 230});
            DrawCylinderWires(Vector3Zero(), enemy.radius * 1.38f, enemy.radius * 1.38f,
                enemy.radius * 0.08f, 18, Color{255, 198, 72, 220});
            DrawLine3D(Vector3{-enemy.radius * 1.45f, enemy.radius * 1.25f, 0.0f},
                Vector3{enemy.radius * 1.45f, enemy.radius * 1.25f, 0.0f}, Color{255, 230, 120, 220});
        }
        if (enemy.plagueTimer > 0.0f) {
            float plaguePulse = 1.0f + std::sin(static_cast<float>(GetTime()) * 10.0f + enemy.bobTimer) * 0.12f;
            DrawSphereWires(Vector3Zero(), enemy.radius * 1.46f * plaguePulse, 8, 5, Color{165, 255, 70, 220});
            DrawCylinderWires(Vector3Zero(), enemy.radius * 1.08f, enemy.radius * 1.08f,
                enemy.radius * 0.06f, 16, Color{95, 185, 45, 200});
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
            float lifeAlpha = 1.0f;
            if (pickup.maxLife > 0.0f && pickup.age > config_.droppedEssenceFadeStart) {
                float fadeWindow = std::max(0.001f, pickup.maxLife - config_.droppedEssenceFadeStart);
                lifeAlpha = 1.0f - std::clamp((pickup.age - config_.droppedEssenceFadeStart) / fadeWindow, 0.0f, 1.0f);
            }
            Color tint = ColorFromHSV(hue, 0.75f * lifeAlpha, 0.28f + 0.72f * lifeAlpha);
            tint.a = static_cast<unsigned char>(255.0f * lifeAlpha);
            for (int m = 0; m < essenceModel_.meshCount; ++m) {
                int matIdx = essenceModel_.meshMaterial[m];
                essenceModel_.materials[matIdx].maps[MATERIAL_MAP_DIFFUSE].color = tint;
            }
            DrawModel(essenceModel_, base, 0.65f, FadeColor(WHITE, lifeAlpha));
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
        } else if (projectile.kind == ProjectileKind::UfoOrb) {
            float spin = static_cast<float>(GetTime()) * 9.0f;
            DrawSphereEx(position, projectile.radius * 1.45f, 8, 6, projectile.color);
            DrawSphereWires(position, projectile.radius * (2.1f + std::sin(spin) * 0.22f), 10, 6, FadeColor(Color{130, 245, 255, 255}, 0.8f));
            DrawCylinderWires(position, projectile.radius * 2.3f, projectile.radius * 1.2f, 0.03f, 12, FadeColor(Color{80, 190, 255, 255}, 0.62f));
            DrawLine3D(position, Vector3Add(position, Vector3Scale(trail, 1.7f)), FadeColor(projectile.color, 0.85f));
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

        // Rainbow beam: derive all layer colors from hue
        bool rainbow = beam.hue > 0.0f;
        float hue = rainbow ? std::fmod(beam.hue + (1.0f - alpha) * 120.0f, 360.0f) : 0.0f;

        Color glow = rainbow
            ? FadeColor(ColorFromHSV(hue, 0.85f, 1.0f), alpha * 0.55f)
            : FadeColor(beam.color, alpha * 0.55f);
        Color shell = rainbow
            ? FadeColor(ColorFromHSV(std::fmod(hue + 30.0f, 360.0f), 0.7f, 1.0f), alpha * 0.75f)
            : FadeColor(Color{80, 190, 255, 255}, alpha * 0.75f);
        Color core = rainbow
            ? FadeColor(Color{255, 255, 255, 255}, alpha)
            : FadeColor(Color{230, 255, 255, 255}, alpha);
        Color wireColor = rainbow
            ? FadeColor(ColorFromHSV(std::fmod(hue + 60.0f, 360.0f), 0.9f, 1.0f), alpha * 0.9f)
            : FadeColor(Color{190, 245, 255, 255}, alpha * 0.9f);

        DrawCylinderEx(beam.start, beam.end, outerRadius, outerRadius * 0.82f, 7, glow);
        DrawCylinderEx(beam.start, beam.end, outerRadius * 0.58f, outerRadius * 0.45f, 7, shell);
        DrawCylinderEx(beam.start, beam.end, coreRadius, coreRadius, 6, core);
        DrawCylinderWiresEx(beam.start, beam.end, outerRadius * 1.08f, outerRadius * 0.92f, 7, wireColor);

        Color startColor = rainbow
            ? FadeColor(ColorFromHSV(hue, 0.8f, 1.0f), alpha * 0.8f)
            : FadeColor(Color{180, 240, 255, 255}, alpha * 0.8f);
        Color endColor = rainbow
            ? FadeColor(Color{255, 255, 255, 255}, alpha)
            : FadeColor(Color{220, 255, 255, 255}, alpha);

        DrawSphereEx(beam.start, outerRadius * (1.1f + beam.charge * 0.4f), 7, 5, startColor);
        DrawSphereEx(beam.end, outerRadius * (1.35f + beam.charge * 0.8f), 7, 5, endColor);
    }
}
void Game::DrawBallLightnings() const {
    for (const BallLightning& ball : ballLightnings_) {
        float pulse = 0.9f + std::sin(ball.life * 8.0f) * 0.1f;
        float r = ball.radius * pulse;

        Color coreColor = ColorFromHSV(ball.hue, 0.8f, 1.0f);
        Color glowColor = Color{coreColor.r, coreColor.g, coreColor.b, 140};
        Color wireColor1 = Color{coreColor.r, coreColor.g, coreColor.b, 178};
        Color wireColor2 = Color{coreColor.r, coreColor.g, coreColor.b, 89};

        // Core sphere (full opacity)
        DrawSphereEx(ball.position, r, 10, 8, coreColor);
        // Inner wireframe
        DrawSphereWires(ball.position, r * 1.35f, 12, 10, wireColor1);
        // Outer wireframe halo
        DrawSphereWires(ball.position, r * 1.8f, 8, 6, wireColor2);

        // Glow ring on tangent plane
        DrawCylinderWires(ball.position, r * 1.5f, r * 1.5f, 0.03f, 20, glowColor);
    }
}

// Helper: draw a vertically-elongated diamond (octahedron) wireframe
static void DrawDiamondFrame(Vector3 center, Vector3 up, Vector3 right, Vector3 fwd, float radius, Color color) {
    Vector3 top = Vector3Add(center, Vector3Scale(up, radius * 2.5f));
    Vector3 bottom = Vector3Add(center, Vector3Scale(up, -radius * 2.5f));
    Vector3 eq0 = Vector3Add(center, Vector3Scale(right, radius));
    Vector3 eq1 = Vector3Add(center, Vector3Scale(fwd, radius));
    Vector3 eq2 = Vector3Add(center, Vector3Scale(right, -radius));
    Vector3 eq3 = Vector3Add(center, Vector3Scale(fwd, -radius));

    // Top pyramid
    DrawLine3D(top, eq0, color); DrawLine3D(top, eq1, color);
    DrawLine3D(top, eq2, color); DrawLine3D(top, eq3, color);
    // Bottom pyramid
    DrawLine3D(bottom, eq0, color); DrawLine3D(bottom, eq1, color);
    DrawLine3D(bottom, eq2, color); DrawLine3D(bottom, eq3, color);
    // Equator
    DrawLine3D(eq0, eq1, color); DrawLine3D(eq1, eq2, color);
    DrawLine3D(eq2, eq3, color); DrawLine3D(eq3, eq0, color);
}

void Game::DrawWaterDropletCrafts() const {
    for (size_t i = 0; i < waterDropletCrafts_.size(); ++i) {
        const WaterDropletCraft& craft = waterDropletCrafts_[i];
        bool active = (waterDropletCraftTarget_ == (int)i && waterDropletCrafting_);

        Vector3 up = IsSphericalMap()
            ? SphericalUpAt(craft.position, craft.world)
            : Vector3{0.0f, 1.0f, 0.0f};
        Vector3 right = IsSphericalMap()
            ? Vector3Normalize(Vector3CrossProduct(up, Vector3{0.0f, 1.0f, 0.0f}))
            : Vector3{1.0f, 0.0f, 0.0f};
        if (Vector3Length(right) < 0.01f) right = Vector3{1.0f, 0.0f, 0.0f};
        Vector3 fwd = Vector3Normalize(Vector3CrossProduct(right, up));

        float r = config_.waterDropletRadius;
        float time = (float)GetTime();
        float prog = craft.progress;
        unsigned char alpha = active ? 255 : (unsigned char)(128 + prog * 127);

        // Diamond frame
        Color frameColor = {255, 200, 50, alpha};
        DrawDiamondFrame(craft.position, up, right, fwd, r, frameColor);

        // Inner glow sphere — golden, pulses with progress
        if (prog > 0.0f) {
            float pulse = 0.9f + std::sin(time * 5.0f) * 0.1f;
            float glowR = r * prog * 0.7f * pulse;
            Color coreColor = {255, 220, 60, alpha};
            DrawSphereEx(craft.position, glowR, 5, 4, FadeColor(coreColor, active ? 0.7f : 0.5f));
            DrawSphereWires(craft.position, glowR * 1.4f, 6, 5, FadeColor(coreColor, 0.4f));
        }

        // Active crafting — golden glow ring at base
        if (active) {
            Color ringColor = {255, 200, 50, 160};
            Vector3 ringBase = Vector3Add(craft.position, Vector3Scale(up, -0.01f));
            Vector3 ringTop = Vector3Add(craft.position, Vector3Scale(up, 0.01f));
            DrawCylinderWiresEx(ringBase, ringTop, r * 1.3f, r * 1.3f, 16, ringColor);
        }
    }
}

void Game::DrawWaterDroplets() const {
    for (const WaterDroplet& droplet : waterDroplets_) {
        Vector3 up = IsSphericalMap()
            ? SphericalUpAt(droplet.position, droplet.world)
            : Vector3{0.0f, 1.0f, 0.0f};
        Vector3 right = IsSphericalMap()
            ? Vector3Normalize(Vector3CrossProduct(up, Vector3{0.0f, 1.0f, 0.0f}))
            : Vector3{1.0f, 0.0f, 0.0f};
        if (Vector3Length(right) < 0.01f) right = Vector3{1.0f, 0.0f, 0.0f};
        Vector3 fwd = Vector3Normalize(Vector3CrossProduct(right, up));

        float r = droplet.radius;

        // Afterimage trail
        for (int t = 0; t < droplet.trailCount; ++t) {
            int idx = (droplet.trailHead - 1 - t + 8) % 8;
            float trailAlpha = 0.45f * (1.0f - (float)t / 8.0f);
            float trailScale = 1.0f - (float)t * 0.06f;
            Color trailColor = {255, 200, 50, (unsigned char)(255.0f * trailAlpha)};
            DrawDiamondFrame(droplet.trailHistory[idx], up, right, fwd, r * trailScale, trailColor);
        }

        // Main diamond frame - golden
        DrawDiamondFrame(droplet.position, up, right, fwd, r, Color{255, 220, 60, 255});

        // Core glow
        DrawSphereEx(droplet.position, r * 0.45f, 6, 4, Color{255, 240, 150, 255});
        DrawSphereWires(droplet.position, r * 1.1f, 6, 4, Color{255, 200, 50, 255});

        // Forward glow
        Vector3 speedDir = droplet.velocity;
        if (Vector3Length(speedDir) > 0.001f) {
            speedDir = Vector3Normalize(speedDir);
            Vector3 glowPos = Vector3Add(droplet.position, Vector3Scale(speedDir, r * 0.6f));
            DrawSphereEx(glowPos, r * 0.25f, 4, 3, Color{255, 255, 200, 200});
        }
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

void Game::DrawFirePatches() const {
    for (const FirePatch& patch : firePatches_) {
        float alpha = patch.maxLife > 0.0f ? patch.life / patch.maxLife : 0.0f;
        float flicker = 0.85f + std::sin(patch.life * 22.0f) * 0.15f;
        float r = patch.radius * flicker;

        float hoverH = config_.heatwaveFirePatchHeight;
        // Match gravity well ring positioning: ±0.02 around center height
        Vector3 ringBase = Vector3Add(patch.position, Vector3Scale(patch.up, hoverH - 0.02f));
        Vector3 ringTop = Vector3Add(patch.position, Vector3Scale(patch.up, hoverH + 0.02f));

        // Concentric fire/plague rings
        Color ringColor = FadeColor(patch.outerColor, alpha * (patch.plague ? 0.72f : 0.85f));
        Color innerColor = FadeColor(patch.innerColor, alpha * (patch.plague ? 0.58f : 0.7f));
        Color coreColor = FadeColor(patch.plague ? Color{190, 255, 95, 255} : Color{255, 245, 180, 255}, alpha * 0.5f);

        DrawCylinderEx(ringBase, ringTop, r, r, 20, ringColor);
        DrawCylinderEx(ringBase, ringTop, r * 0.65f, r * 0.65f, 16, innerColor);
        DrawCylinderEx(ringBase, ringTop, r * 0.3f, r * 0.3f, 10, coreColor);

        // Small glowing hemisphere at center
        Vector3 centerGlow = Vector3Add(patch.position, Vector3Scale(patch.up, hoverH));
        DrawSphereEx(centerGlow, r * 0.22f, 5, 3, FadeColor(patch.innerColor, alpha * 0.8f));
        if (patch.plague) {
            Vector3 wireBase = Vector3Subtract(centerGlow, Vector3Scale(patch.up, 0.025f));
            Vector3 wireTop = Vector3Add(centerGlow, Vector3Scale(patch.up, 0.025f));
            DrawCylinderWiresEx(wireBase, wireTop, r * 1.08f, r * 1.08f, 24, FadeColor(Color{205, 255, 105, 255}, alpha * 0.8f));
            DrawCylinderWiresEx(wireBase, wireTop, r * 0.42f, r * 0.42f, 14, FadeColor(Color{95, 190, 55, 255}, alpha * 0.7f));
        }
    }
}

void Game::DrawNapalmGrenades() const {
    for (const NapalmGrenade& g : napalmGrenades_) {
        float urgency = g.fuse < 0.5f ? (0.5f + std::sin(g.fuse * 40.0f) * 0.5f) : 1.0f;
        Color body = Color{180, 190, 140, 255};
        Color glow = Color{255, (unsigned char)(160 * urgency), 30, 255};
        DrawSphereEx(g.position, 0.2f, 5, 4, body);
        DrawSphereWires(g.position, 0.25f, 5, 4, glow);
        // Flickering fuse spark
        if (urgency > 0.6f) {
            DrawSphereEx(g.position, 0.12f * urgency, 4, 3, FadeColor(Color{255, 255, 200, 255}, 0.7f));
        }
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
void Game::DrawJudgmentStigmas() const {
    for (const JudgmentStigma& stigma : judgmentStigmas_) {
        float alpha = stigma.maxLife > 0.0f ? std::clamp(stigma.life / stigma.maxLife, 0.0f, 1.0f) : 0.0f;
        float birth = 1.0f - alpha;
        float width = stigma.radius * (0.65f + std::min(1.0f, birth * 6.0f) * 0.35f);
        Color core = FadeColor(Color{255, 246, 184, 255}, alpha * 0.72f);
        Color gold = FadeColor(Color{255, 198, 48, 255}, alpha * 0.58f);
        Color edge = FadeColor(Color{255, 130, 28, 255}, alpha * 0.42f);
        Vector3 right = SafeNormalize(stigma.right, PlayerRight());
        Vector3 up = SafeNormalize(stigma.up, PlayerUp());
        Vector3 forward = SafeNormalize(stigma.forward, Vector3Normalize(Vector3Subtract(stigma.end, stigma.start)));

        constexpr int kSegments = 18;
        Vector3 previousA = {};
        Vector3 previousB = {};
        bool hasPrevious = false;
        for (int i = 0; i <= kSegments; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(kSegments);
            Vector3 center = Vector3Add(stigma.start, Vector3Scale(Vector3Subtract(stigma.end, stigma.start), t));
            float wave = std::sin(t * 6.2831853f * 3.0f + static_cast<float>(GetTime()) * 2.4f) * width * 0.16f;
            float taper = std::sin(t * 3.1415926f);
            float localWidth = width * (0.45f + taper * 0.55f);
            Vector3 a = Vector3Add(center, Vector3Add(Vector3Scale(right, localWidth), Vector3Scale(up, wave)));
            Vector3 b = Vector3Add(center, Vector3Add(Vector3Scale(right, -localWidth), Vector3Scale(up, -wave * 0.35f)));
            if (hasPrevious) {
                DrawTriangle3D(previousA, a, previousB, gold);
                DrawTriangle3D(a, b, previousB, gold);
                DrawTriangle3D(previousB, a, previousA, gold);
                DrawTriangle3D(previousB, b, a, gold);
                DrawLine3D(previousA, a, edge);
                DrawLine3D(previousB, b, edge);
            }
            previousA = a;
            previousB = b;
            hasPrevious = true;
        }
        DrawCylinderEx(stigma.start, stigma.end, width * 0.22f, width * 0.14f, 8, core);
        for (int i = 0; i < 4; ++i) {
            float spin = static_cast<float>(GetTime()) * (1.1f + i * 0.27f) + i * 1.5708f;
            Vector3 offset = Vector3Add(Vector3Scale(right, std::cos(spin) * width * 0.55f),
                                        Vector3Scale(up, std::sin(spin) * width * 0.35f));
            DrawLine3D(Vector3Add(stigma.start, offset),
                       Vector3Add(stigma.end, Vector3Add(offset, Vector3Scale(forward, -width * 0.6f))),
                       FadeColor(Color{255, 235, 128, 255}, alpha * 0.34f));
        }
    }
}
void Game::DrawEdenFireSlashes() const {
    for (const EdenFireSlash& slash : edenFireSlashes_) {
        Vector3 right = Vector3Length(slash.right) > 0.001f ? Vector3Normalize(slash.right) : PlayerForward();
        Vector3 up = Vector3Length(slash.up) > 0.001f ? Vector3Normalize(slash.up) : Vector3{0.0f, 1.0f, 0.0f};
        float lifeRatio = slash.maxLife > 0.0f ? std::clamp(slash.life / slash.maxLife, 0.0f, 1.0f) : 0.0f;
        float birth = 1.0f - lifeRatio;
        float scale = 0.86f + std::min(1.0f, birth * 5.0f) * 0.22f;
        float outerRadius = slash.radius * scale;
        float innerRadius = std::max(0.05f, (slash.radius - slash.thickness) * scale);
        Color fill{255, 78, 24, 255};
        Color hot{255, 236, 126, 255};
        Color edge{255, 128, 32, 255};

        constexpr int kSegments = 20;
        Vector3 previousOuterFront = {};
        Vector3 previousInnerFront = {};
        Vector3 previousOuterBack = {};
        Vector3 previousInnerBack = {};
        Vector3 firstOuterFront = {};
        Vector3 firstInnerFront = {};
        Vector3 firstOuterBack = {};
        Vector3 firstInnerBack = {};
        bool hasPrevious = false;
        Vector3 depth = Vector3Scale(slash.normal, slash.planeThickness * 0.5f);
        for (int i = 0; i <= kSegments; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(kSegments);
            float angle = (-0.82f + t * 1.64f) * PI;
            float x = std::cos(angle);
            float y = std::sin(angle) * 0.76f + 0.28f;
            Vector3 outerMid = Vector3Add(slash.center, Vector3Add(Vector3Scale(right, x * outerRadius), Vector3Scale(up, y * outerRadius)));
            Vector3 innerMid = Vector3Add(slash.center, Vector3Add(Vector3Scale(right, x * innerRadius * 0.82f), Vector3Scale(up, y * innerRadius + slash.thickness * 0.45f * scale)));
            Vector3 outerFront = Vector3Add(outerMid, depth);
            Vector3 innerFront = Vector3Add(innerMid, depth);
            Vector3 outerBack = Vector3Subtract(outerMid, depth);
            Vector3 innerBack = Vector3Subtract(innerMid, depth);
            if (hasPrevious) {
                DrawTriangle3D(previousOuterFront, outerFront, previousInnerFront, fill);
                DrawTriangle3D(outerFront, innerFront, previousInnerFront, fill);
                DrawTriangle3D(previousOuterBack, previousInnerBack, outerBack, fill);
                DrawTriangle3D(outerBack, previousInnerBack, innerBack, fill);
                DrawTriangle3D(previousOuterFront, previousOuterBack, outerFront, fill);
                DrawTriangle3D(outerFront, previousOuterBack, outerBack, fill);
                DrawTriangle3D(previousInnerFront, innerFront, previousInnerBack, hot);
                DrawTriangle3D(innerFront, innerBack, previousInnerBack, hot);
                DrawLine3D(previousOuterFront, outerFront, edge);
                DrawLine3D(previousOuterBack, outerBack, edge);
                DrawLine3D(previousInnerFront, innerFront, hot);
                DrawLine3D(previousInnerBack, innerBack, hot);
            }
            if (!hasPrevious) {
                firstOuterFront = outerFront;
                firstInnerFront = innerFront;
                firstOuterBack = outerBack;
                firstInnerBack = innerBack;
            }
            previousOuterFront = outerFront;
            previousInnerFront = innerFront;
            previousOuterBack = outerBack;
            previousInnerBack = innerBack;
            hasPrevious = true;
        }
        if (hasPrevious) {
            DrawTriangle3D(firstOuterFront, firstInnerFront, firstOuterBack, fill);
            DrawTriangle3D(firstInnerFront, firstInnerBack, firstOuterBack, fill);
            DrawTriangle3D(previousOuterFront, previousOuterBack, previousInnerFront, fill);
            DrawTriangle3D(previousInnerFront, previousOuterBack, previousInnerBack, fill);
        }
        DrawSphereEx(slash.center, 0.32f + birth * 0.20f, 8, 5, Color{255, 208, 90, 255});
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
void Game::DrawUfoPilotWeapon() const {
    if (!UfoPilotActive()) return;
    Vector3 forward = PlayerForward();
    Vector3 right = PlayerRight();
    Vector3 up = PlayerUp();
    Vector3 base = Vector3Add(camera_.position,
        Vector3Add(Vector3Scale(forward, 1.25f), Vector3Add(Vector3Scale(right, 0.42f), Vector3Scale(up, -0.38f))));
    Color body = Color{75, 100, 112, 235};
    Color glow = ufoPilotWeapon_ == UfoPilotWeapon::Orb ? Color{120, 235, 255, 235} : Color{155, 255, 220, 235};

    DrawCube(base, 0.34f, 0.18f, 0.52f, body);
    DrawCubeWires(base, 0.36f, 0.2f, 0.54f, Color{170, 230, 245, 180});
    Vector3 muzzle = Vector3Add(base, Vector3Scale(forward, 0.34f));
    if (ufoPilotWeapon_ == UfoPilotWeapon::Orb) {
        DrawSphereEx(muzzle, 0.12f + std::sin(static_cast<float>(GetTime()) * 7.0f) * 0.025f, 8, 6, glow);
        DrawCylinderWiresEx(Vector3Subtract(muzzle, Vector3Scale(forward, 0.05f)),
            Vector3Add(muzzle, Vector3Scale(forward, 0.05f)), 0.18f, 0.18f, 18, FadeColor(glow, 0.78f));
    } else {
        Vector3 left = Vector3Add(muzzle, Vector3Scale(right, -0.16f));
        Vector3 rightTip = Vector3Add(muzzle, Vector3Scale(right, 0.16f));
        DrawCylinderEx(left, Vector3Add(left, Vector3Scale(forward, 0.26f)), 0.035f, 0.018f, 6, glow);
        DrawCylinderEx(rightTip, Vector3Add(rightTip, Vector3Scale(forward, 0.26f)), 0.035f, 0.018f, 6, glow);
        DrawLine3D(left, rightTip, FadeColor(glow, 0.65f));
    }
}
void Game::DrawUfoCockpitOverlay() const {
    int w = pixelWidth_;
    int h = pixelHeight_;
    int cx = w / 2;
    int cy = h / 2;
    Color cyan = Color{95, 235, 255, 235};
    Color dim = Color{70, 145, 165, 160};
    Color bright = Color{185, 250, 255, 245};

    DrawRectangle(0, 0, w, h, FadeColor(Color{0, 25, 32, 255}, 0.10f));
    for (int y = 0; y < h; y += 7) {
        DrawLine(0, y, w, y, FadeColor(cyan, 0.08f));
    }
    DrawText("SCAVENGER COCKPIT", 6, 5, 8, cyan);
    DrawLine(0, 18, w, 18, FadeColor(cyan, 0.35f));
    const char* playerEssenceText = TextFormat("P:%d", essence_);
    float essenceHue = std::fmod(survivalTime_ * 75.0f + static_cast<float>(essence_) * 18.0f, 360.0f);
    Color playerEssenceColor = ColorFromHSV(essenceHue, 0.78f, 1.0f);
    playerEssenceColor.a = 245;
    int playerEssenceX = w - MeasureText(playerEssenceText, 12) - 8;
    DrawRectangle(playerEssenceX - 3, 3, MeasureText(playerEssenceText, 12) + 6, 14, FadeColor(Color{4, 8, 18, 255}, 0.55f));
    DrawText(playerEssenceText, playerEssenceX + 1, 6, 12, FadeColor(Color{0, 8, 14, 255}, 0.65f));
    DrawText(playerEssenceText, playerEssenceX, 5, 12, playerEssenceColor);
    const char* weaponText = "WEAPON: TRACTOR";
    if (ufoPilotWeapon_ == UfoPilotWeapon::Orb) {
        weaponText = ufoOrbMode_ == UfoOrbMode::Laser ? "WEAPON: ORB / LASER" : "WEAPON: ORB / SHOT";
    }
    DrawText(weaponText, 6, 26, 8, bright);
    float altitude = IsSphericalMap()
        ? SphericalAltitudeAt(scavengerUfo_.position, 0)
        : std::abs(Vector3DotProduct(Vector3Subtract(scavengerUfo_.position, Vector3{0.0f, FlatGroundYForWorld(0), 0.0f}), FlatUpForWorld(0)));
    DrawText(TextFormat("ALT %.1f", altitude), 6, 38, 8, bright);
    DrawText(TextFormat("SPD %.1f", Vector3Length(scavengerUfo_.velocity)), 6, 50, 8, bright);

    DrawCircleLines(cx, cy, 18.0f, cyan);
    DrawCircleLines(cx, cy, 5.0f, FadeColor(cyan, 0.7f));
    DrawLine(cx - 30, cy, cx - 8, cy, cyan);
    DrawLine(cx + 8, cy, cx + 30, cy, cyan);
    DrawLine(cx, cy - 30, cx, cy - 8, cyan);
    DrawLine(cx, cy + 8, cx, cy + 30, cyan);

    int corner = 22;
    DrawLine(0, 0, corner, 0, dim);
    DrawLine(0, 0, 0, corner, dim);
    DrawLine(w, 0, w - corner, 0, dim);
    DrawLine(w, 0, w, corner, dim);
    DrawLine(0, h, corner, h, dim);
    DrawLine(0, h, 0, h - corner, dim);
    DrawLine(w, h, w - corner, h, dim);
    DrawLine(w, h, w, h - corner, dim);

    int barX = 8;
    int barY = h - 24;
    int barW = 120;
    int barH = 7;
    float ratio = config_.ufoPilotEssenceMax > 0
        ? std::clamp(static_cast<float>(scavengerUfo_.pilotEssence) / static_cast<float>(config_.ufoPilotEssenceMax), 0.0f, 1.0f)
        : 0.0f;
    DrawText("ESSENCE", barX, barY - 10, 7, dim);
    DrawRectangle(barX, barY, barW, barH, Color{5, 18, 22, 210});
    DrawRectangle(barX, barY, static_cast<int>(barW * ratio), barH, Color{90, 235, 255, 245});
    DrawRectangleLines(barX, barY, barW, barH, cyan);
    DrawText(TextFormat("%d/%d  TOTAL %d", scavengerUfo_.pilotEssence, config_.ufoPilotEssenceMax, scavengerUfo_.pilotTotalCollected),
        barX + barW + 8, barY - 1, 8, bright);

    bool jumpReady = scavengerUfo_.pilotEssence >= config_.ufoPilotJumpCost && scavengerUfo_.pilotJumpCooldown <= 0.0f;
    float holdRatio = config_.ufoHyperspaceHoldTime > 0.0f
        ? std::clamp(ufoHyperspaceHoldTimer_ / config_.ufoHyperspaceHoldTime, 0.0f, 1.0f)
        : 0.0f;
    const char* jump = jumpReady ? TextFormat("H HOLD HYPERSPACE -%d", config_.ufoPilotJumpCost)
        : TextFormat("HYPERSPACE CHARGING %d/%d", scavengerUfo_.pilotEssence, config_.ufoPilotJumpCost);
    DrawText(jump, w - MeasureText(jump, 8) - 8, h - 24, 8, jumpReady ? bright : dim);
    if (ufoTravelState_ == UfoTravelState::Charging) {
        int holdW = 92;
        int holdX = w - holdW - 8;
        int holdY = h - 34;
        DrawRectangle(holdX, holdY, holdW, 5, Color{5, 18, 22, 220});
        DrawRectangle(holdX, holdY, static_cast<int>(holdW * holdRatio), 5, Color{185, 250, 255, 245});
        DrawRectangleLines(holdX, holdY, holdW, 5, cyan);
    }
    DrawText("Q STORE  E EXTRACT  F EXIT", w - MeasureText("Q STORE  E EXTRACT  F EXIT", 7) - 8, h - 12, 7, dim);
}

void Game::DrawUfoHyperspace() {
    BeginTextureMode(pixelTarget_);
    ClearBackground(Color{2, 3, 12, 255});

    float tunnelRadius = config_.ufoHyperspaceTunnelRadius;
    float shipRadius = tunnelRadius - ufoHyperspaceAltitude_;
    Vector3 shipPos = Vector3{std::cos(ufoHyperspaceAngle_) * shipRadius, std::sin(ufoHyperspaceAngle_) * shipRadius, 0.0f};
    Vector3 radial = SafeNormalize(Vector3{shipPos.x, shipPos.y, 0.0f}, Vector3{0.0f, 1.0f, 0.0f});
    Vector3 localUp = Vector3Scale(radial, -1.0f);
    Vector3 localForward = Vector3{0.0f, 0.0f, 1.0f};
    Vector3 localRight = SafeNormalize(Vector3CrossProduct(localForward, localUp), Vector3{1.0f, 0.0f, 0.0f});
    Vector3 shipDraw = Vector3Add(shipPos, Vector3{0.0f, 0.0f, 4.0f});
    float bob = std::sin(static_cast<float>(GetTime()) * 9.0f) * 0.08f;
    shipDraw = Vector3Add(shipDraw, Vector3Scale(localUp, bob));

    Camera3D travelCamera = {};
    travelCamera.position = Vector3Add(
        Vector3Subtract(shipDraw, Vector3Scale(localForward, 11.5f)),
        Vector3Scale(localUp, 4.2f));
    travelCamera.target = Vector3Add(shipDraw, Vector3Add(Vector3Scale(localForward, 14.0f), Vector3Scale(localUp, 0.6f)));
    travelCamera.up = localUp;
    travelCamera.fovy = 62.0f;
    travelCamera.projection = CAMERA_PERSPECTIVE;

    BeginMode3D(travelCamera);
    float travel = ufoHyperspaceTimer_ * config_.ufoHyperspaceObstacleSpeed;
    float drawDistance = std::max(32.0f, config_.ufoHyperspaceTunnelDrawDistance);
    float ringSpacing = std::max(2.0f, config_.ufoHyperspaceTunnelRingSpacing);
    int ringCount = std::max(8, static_cast<int>(std::ceil(drawDistance / ringSpacing)) + 1);
    for (int ring = 0; ring < ringCount; ++ring) {
        float z = (ring + 1) * ringSpacing - std::fmod(travel, ringSpacing);
        float alpha = std::clamp(1.0f - z / drawDistance, 0.08f, 0.85f);
        Color ringColor = FadeColor(Color{95, 80, 255, 255}, alpha);
        DrawCircle3D(Vector3{0.0f, 0.0f, z}, tunnelRadius, Vector3{0.0f, 0.0f, 1.0f}, 90.0f, ringColor);
    }
    for (int line = 0; line < 18; ++line) {
        float angle = (static_cast<float>(line) / 18.0f) * 2.0f * PI + ufoHyperspaceTimer_ * 0.18f;
        Vector3 a = Vector3{std::cos(angle) * tunnelRadius, std::sin(angle) * tunnelRadius, 0.0f};
        Vector3 b = Vector3{std::cos(angle) * tunnelRadius, std::sin(angle) * tunnelRadius, drawDistance};
        DrawLine3D(a, b, FadeColor(Color{75, 220, 255, 255}, 0.22f));
    }

    rlPushMatrix();
    Matrix shipBasis = {
        localRight.x, localUp.x, localForward.x, shipDraw.x,
        localRight.y, localUp.y, localForward.y, shipDraw.y,
        localRight.z, localUp.z, localForward.z, shipDraw.z,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    rlMultMatrixf(MatrixToFloat(shipBasis));
    if (scavengerUfoModelLoaded_) {
        DrawModel(scavengerUfoModel_, Vector3Zero(), 0.75f, WHITE);
    } else {
        DrawCylinderEx(Vector3{-1.1f, 0.0f, 0.0f}, Vector3{1.1f, 0.0f, 0.0f}, 0.32f, 0.72f, 24, Color{150, 175, 185, 255});
        DrawSphereEx(Vector3{0.0f, 0.32f, 0.0f}, 0.45f, 12, 8, Color{100, 235, 255, 230});
    }
    DrawSphereEx(Vector3{0.0f, -0.65f, 0.0f}, 0.28f, 8, 6, Color{90, 235, 255, 170});
    rlPopMatrix();

    for (const UfoHyperspaceObstacle& obstacle : ufoHyperspaceObstacles_) {
        float r = tunnelRadius - obstacle.altitude;
        Vector3 pos = Vector3{std::cos(obstacle.angle) * r, std::sin(obstacle.angle) * r, obstacle.distance + 4.0f};
        Color color = obstacle.hit ? Color{255, 120, 210, 165} : Color{160, 80, 255, 220};
        float distanceFade = std::clamp(1.0f - obstacle.distance / std::max(1.0f, config_.ufoHyperspaceObstacleSpawnDistance), 0.18f, 1.0f);
        DrawCube(pos, obstacle.radius * 1.2f, obstacle.radius * 1.2f, obstacle.radius * 0.55f, FadeColor(color, 0.55f * distanceFade));
        DrawCubeWires(pos, obstacle.radius * 1.35f, obstacle.radius * 1.35f, obstacle.radius * 0.7f, FadeColor(color, distanceFade));
        DrawSphereWires(pos, obstacle.radius * 1.25f, 8, 6, FadeColor(Color{90, 235, 255, 255}, 0.45f * distanceFade));
    }

    EndMode3D();

    int w = pixelWidth_;
    int h = pixelHeight_;
    float progress = config_.ufoHyperspaceDuration > 0.0f
        ? std::clamp(ufoHyperspaceTimer_ / config_.ufoHyperspaceDuration, 0.0f, 1.0f)
        : 1.0f;
    Color cyan = Color{115, 235, 255, 235};
    Color purple = Color{160, 90, 255, 230};
    if (!hideUI_) {
        DrawRectangle(0, 0, w, h, FadeColor(Color{20, 0, 55, 255}, 0.10f));
        for (int y = 0; y < h; y += 6) {
            DrawLine(0, y, w, y, FadeColor(cyan, 0.06f));
        }
        const char* title = ufoTravelState_ == UfoTravelState::Charging ? "HYPERSPACE VECTOR LOCK" :
            ufoTravelState_ == UfoTravelState::Arriving ? "EXITING HYPERSPACE" : "HYPERSPACE CORRIDOR";
        DrawText(title, 8, 8, 10, cyan);
        int barW = w - 80;
        int barX = 40;
        int barY = h - 24;
        DrawRectangle(barX, barY, barW, 8, Color{5, 10, 25, 230});
        DrawRectangle(barX, barY, static_cast<int>(barW * progress), 8, purple);
        DrawRectangleLines(barX, barY, barW, 8, cyan);
        DrawText(TextFormat("ESSENCE %d/%d", scavengerUfo_.pilotEssence, config_.ufoPilotEssenceMax), 8, h - 12, 8, cyan);
        DrawText(TextFormat("ALT %.1f", ufoHyperspaceAltitude_), w - 74, h - 12, 8, cyan);

        if (ufoTravelState_ == UfoTravelState::Charging) {
            float hold = config_.ufoHyperspaceHoldTime > 0.0f
                ? std::clamp(ufoHyperspaceHoldTimer_ / config_.ufoHyperspaceHoldTime, 0.0f, 1.0f)
                : 0.0f;
            DrawCircleLines(w / 2, h / 2, 22.0f + hold * 38.0f, FadeColor(cyan, 0.7f));
            DrawText("HOLD H", w / 2 - MeasureText("HOLD H", 10) / 2, h / 2 - 5, 10, cyan);
        }
    }
    if (ufoHyperspaceFlash_ > 0.0f) {
        float flash = std::clamp(ufoHyperspaceFlash_, 0.0f, 1.0f);
        float vignette = 1.0f - flash;
        DrawRectangle(0, 0, w, h, FadeColor(Color{120, 190, 255, 255}, flash * 0.32f));
        DrawCircleLines(w / 2, h / 2, 34.0f + vignette * 92.0f, FadeColor(Color{130, 235, 255, 255}, flash * 0.65f));
        DrawCircleLines(w / 2, h / 2, 68.0f + vignette * 150.0f, FadeColor(Color{165, 90, 255, 255}, flash * 0.35f));
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

    if (exitHoldTimer_ > 0.0f) {
        float exitProgress = exitHoldTimer_ / kExitHoldDuration;
        unsigned char overlayAlpha = static_cast<unsigned char>(exitProgress * exitProgress * 180.0f);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, overlayAlpha});
        const char* exitText = "EXITING THE GAME";
        int fontSize = 32;
        int tw = MeasureText(exitText, fontSize);
        DrawText(exitText, (GetScreenWidth() - tw) / 2, GetScreenHeight() / 2 - 24, fontSize, WHITE);
        int exitBarW = 200;
        int exitBarH = 4;
        int exitBarX = (GetScreenWidth() - exitBarW) / 2;
        int exitBarY = GetScreenHeight() / 2 + 16;
        DrawRectangle(exitBarX, exitBarY, exitBarW, exitBarH, Color{40, 40, 40, 200});
        DrawRectangle(exitBarX, exitBarY, static_cast<int>(exitBarW * exitProgress), exitBarH, Color{200, 60, 40, 240});
    }
    DrawConsole();
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

    // Super charge muzzle glow — growing rainbow sphere
    if (activeWeapon_ == WeaponType::Laser && (superCharging_ || superChargePaused_ || superCharged_)) {
        Vector3 muzzle = WeaponMuzzlePosition();
        float ratio = config_.superEssenceThreshold > 0
            ? (float)superEssenceConsumed_ / (float)config_.superEssenceThreshold : 0.0f;
        float r = 0.15f + ratio * 1.2f;
        float pulse = 0.85f + std::sin((float)GetTime() * 7.0f) * 0.15f;
        float hue = std::fmod((float)GetTime() * 60.0f, 360.0f);
        Color core = ColorFromHSV(hue, 0.9f, 1.0f);
        DrawSphereEx(muzzle, r * pulse, 7, 5, FadeColor(core, 0.7f));
        DrawSphereWires(muzzle, r * pulse * 1.4f, 8, 6, FadeColor(core, 0.45f));
    }
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
    // Super charge indicator
    if (superCharging_ || superChargePaused_ || superCharged_) {
        int barWidth = 60;
        int barY = screenHeight - 24;
        float ratio = config_.superEssenceThreshold > 0
            ? static_cast<float>(superEssenceConsumed_) / static_cast<float>(config_.superEssenceThreshold)
            : 0.0f;
        int fillWidth = static_cast<int>(barWidth * std::min(ratio, 1.0f));

        Color barColor;
        if (superCharged_) {
            float hue = std::fmod(survivalTime_ * 120.0f, 360.0f);
            barColor = ColorFromHSV(hue, 0.9f, 1.0f);
        } else if (superChargePaused_) {
            barColor = Color{160, 140, 100, 220};
        } else {
            barColor = Color{255, 215, 60, 240};
        }

        DrawRectangle(screenWidth / 2 - barWidth / 2, barY, barWidth, 4, Color{18, 30, 36, 210});
        DrawRectangle(screenWidth / 2 - barWidth / 2, barY, fillWidth, 4, barColor);

        // Essence count text
        const char* label = superCharged_ ? "SUPER CHARGED"
            : superChargePaused_ ? "PAUSED"
            : TextFormat("%d/%d ESSENCE", superEssenceConsumed_, config_.superEssenceThreshold);
        int labelW = MeasureText(label, 6);
        DrawText(label, screenWidth / 2 - labelW / 2, barY - 8, 6, FadeColor(WHITE, 0.85f));
    }
    if (mysticStaffChanneling_) {
        int chargeWidth = static_cast<int>(48.0f * mysticStaffChannelProgress_);
        Color chargeColor = Color{200, 160, 255, 240};
        DrawRectangle(screenWidth / 2 - 24, screenHeight - 24, 48, 4, Color{18, 30, 36, 210});
        DrawRectangle(screenWidth / 2 - 24, screenHeight - 24, chargeWidth, 4, chargeColor);
    }
}
void Game::DrawHud() const {
    if (IsEdenMap() && !edenForbiddenFruit_.claimed) {
        float hue = std::fmod(static_cast<float>(GetTime()) * 24.0f + static_cast<float>(essence_) * 18.0f, 360.0f);
        Color essenceColor = ColorFromHSV(hue, 0.42f, 1.0f);
        DrawText(TextFormat("ESSENCE %d", essence_), 8, 8, 10, essenceColor);
        DrawText("EDEN", pixelWidth_ - MeasureText("EDEN", 8) - 8, 8, 8, FadeColor(Color{255, 245, 205, 255}, 0.72f));
        if (edenArk_.piloted) {
            DrawText("NOAH'S ARK", 8, 22, 8, FadeColor(Color{255, 224, 142, 255}, 0.86f));
            DrawText("W/S ROW  A/D TURN  SHIFT SPEED  F LEAVE", 8, 32, 6, FadeColor(Color{255, 246, 210, 255}, 0.72f));
        }
        if (edenExitFade_ > 0.01f) {
            const char* leaving = "LEAVING EDEN";
            DrawText(leaving, pixelWidth_ / 2 - MeasureText(leaving, 10) / 2, pixelHeight_ - 26, 10,
                FadeColor(Color{255, 248, 220, 255}, 0.35f + edenExitFade_ * 0.65f));
        }
        if (EdenArkEnterAvailable()) {
            const char* prompt = "F ENTER NOAH'S ARK";
            DrawText(prompt, pixelWidth_ / 2 - MeasureText(prompt, 10) / 2, pixelHeight_ - 43, 10,
                Color{255, 224, 132, 255});
        } else if (EdenForbiddenFruitInteractAvailable()) {
            const char* prompt = TextFormat("F TAKE FORBIDDEN FRUIT  +%d", edenForbiddenFruit_.absorbedEssence);
            DrawText(prompt, pixelWidth_ / 2 - MeasureText(prompt, 10) / 2, pixelHeight_ - 43, 10,
                Color{255, 214, 92, 255});
        }
        return;
    }

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
    if (playerPlagueTimer_ > 0.0f) {
        float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 9.0f);
        unsigned char edgeAlpha = static_cast<unsigned char>(70.0f + pulse * 55.0f);
        unsigned char washAlpha = static_cast<unsigned char>(18.0f + pulse * 10.0f);
        Color edge = Color{105, 255, 45, edgeAlpha};
        Color wash = Color{45, 120, 20, washAlpha};
        DrawRectangle(0, 0, pixelWidth_, pixelHeight_, wash);
        int edgeW = std::max(5, pixelWidth_ / 42);
        int edgeH = std::max(5, pixelHeight_ / 42);
        DrawRectangle(0, 0, pixelWidth_, edgeH, edge);
        DrawRectangle(0, pixelHeight_ - edgeH, pixelWidth_, edgeH, edge);
        DrawRectangle(0, 0, edgeW, pixelHeight_, edge);
        DrawRectangle(pixelWidth_ - edgeW, 0, edgeW, pixelHeight_, edge);
        DrawRectangleLinesEx(Rectangle{2.0f, 2.0f, static_cast<float>(pixelWidth_ - 4), static_cast<float>(pixelHeight_ - 4)},
            2.0f, Color{170, 255, 90, static_cast<unsigned char>(edgeAlpha * 0.75f)});
    }
    if (famineFireRateDebuffTimer_ > 0.0f) {
        float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 7.5f);
        unsigned char washAlpha = static_cast<unsigned char>(16.0f + pulse * 10.0f);
        unsigned char edgeAlpha = static_cast<unsigned char>(76.0f + pulse * 58.0f);
        unsigned char innerAlpha = static_cast<unsigned char>(38.0f + pulse * 32.0f);
        Color famineWash{32, 24, 12, washAlpha};
        Color famineEdge{196, 154, 70, edgeAlpha};
        Color famineInner{18, 12, 5, innerAlpha};
        DrawRectangle(0, 0, pixelWidth_, pixelHeight_, famineWash);
        int edgeW = std::max(6, pixelWidth_ / 38);
        int edgeH = std::max(6, pixelHeight_ / 38);
        DrawRectangle(0, 0, pixelWidth_, edgeH, famineInner);
        DrawRectangle(0, pixelHeight_ - edgeH, pixelWidth_, edgeH, famineInner);
        DrawRectangle(0, 0, edgeW, pixelHeight_, famineInner);
        DrawRectangle(pixelWidth_ - edgeW, 0, edgeW, pixelHeight_, famineInner);
        DrawRectangle(0, 0, pixelWidth_, std::max(2, edgeH / 3), famineEdge);
        DrawRectangle(0, pixelHeight_ - std::max(2, edgeH / 3), pixelWidth_, std::max(2, edgeH / 3), famineEdge);
        DrawRectangle(0, 0, std::max(2, edgeW / 3), pixelHeight_, famineEdge);
        DrawRectangle(pixelWidth_ - std::max(2, edgeW / 3), 0, std::max(2, edgeW / 3), pixelHeight_, famineEdge);
        DrawRectangleLinesEx(Rectangle{3.0f, 3.0f, static_cast<float>(pixelWidth_ - 6), static_cast<float>(pixelHeight_ - 6)},
            2.0f, Color{224, 176, 82, static_cast<unsigned char>(edgeAlpha * 0.7f)});
        DrawText("FAMINE: SLOW FIRE", 8, 26, 8, Color{214, 178, 96, 230});
    }
    if (IsEdenMap()) {
        if (edenArk_.piloted) {
            DrawText("NOAH'S ARK", 8, pixelHeight_ - 30, 8, FadeColor(Color{255, 224, 142, 255}, 0.86f));
            DrawText("W/S ROW  A/D TURN  SHIFT SPEED  F LEAVE", 8, pixelHeight_ - 18, 6, FadeColor(Color{255, 246, 210, 255}, 0.72f));
        } else if (EdenArkEnterAvailable()) {
            const char* prompt = "F ENTER NOAH'S ARK";
            DrawText(prompt, pixelWidth_ / 2 - MeasureText(prompt, 10) / 2, pixelHeight_ - 43, 10,
                Color{255, 224, 132, 255});
        }
    }

    DrawRectangle(0, 0, pixelWidth_, 22, Color{0, 0, 0, 150});
    DrawText(TextFormat("TIME %.1f", survivalTime_), 6, 7, 8, RAYWHITE);
    DrawText(TextFormat("DMG %.0f", totalDamageDealt_), 72, 7, 8, RAYWHITE);
    DrawText(TextFormat("E %d", static_cast<int>(enemies_.size())), 134, 7, 8, RAYWHITE);
    const char* modeName = WeaponModeName();
    DrawText(TextFormat("W %s%s%s", WeaponName(), modeName[0] != '\0' ? ":" : "", modeName), 174, 7, 8, RAYWHITE);
    Color gravityColor = playerAntigravityTimer_ > 0.0f ? Color{245, 245, 255, 255}
        : spaceSuitEnabled_ ? Color{120, 220, 255, 255} : hasSpaceSuit_ ? Color{110, 125, 135, 255} : RAYWHITE;
    float shownGravityScale = gravityScale_ * (playerAntigravityTimer_ > 0.0f ? config_.throneAntigravityScale : 1.0f);
    DrawText(TextFormat("G %.2fx", shownGravityScale), 250, 7, 8, gravityColor);
    const bool hasInactiveGear = (hasFlightRig_ && !flightRigEnabled_) || (hasSkates_ && !skatesEnabled_) || (hasSpaceSuit_ && !spaceSuitEnabled_);
    const char* stateText = timeStopped_ ? "STOP" : config_.invincible ? "GOD" : chargingLaser_ ? "LASER" : flightRigEnabled_ ? "FLIGHT" : skatesEnabled_ ? "SKATE" : spaceSuitEnabled_ ? "SUIT" : hasInactiveGear ? "GEAR" : grounded_ ? "GROUND" : "AIR";
    Color stateColor = timeStopped_ ? Color{190, 160, 255, 255} : config_.invincible ? Color{255, 230, 120, 255} : chargingLaser_ ? Color{120, 220, 255, 255} : flightRigEnabled_ ? Color{160, 245, 255, 255} : skatesEnabled_ ? Color{165, 255, 185, 255} : spaceSuitEnabled_ ? Color{120, 220, 255, 255} : hasInactiveGear ? Color{145, 150, 155, 255} : grounded_ ? Color{190, 255, 190, 255} : Color{180, 220, 255, 255};
    DrawText(stateText, 316, 7, 8, stateColor);
    DrawText(eventTextTimer_ > 0.0f ? eventText_ : WaveLabel(), 6, 29, 8,
        eventTextTimer_ > 0.0f ? Color{255, 220, 135, 255} : Color{180, 180, 180, 255});
    if (IsLabyrinthMap()) {
        const char* shiftText = labyrinthShiftWarning_
            ? "LABYRINTH SHIFTING"
            : TextFormat("SHIFT %.0fs", std::max(0.0f, labyrinthShiftTimer_));
        DrawText(shiftText, pixelWidth_ - MeasureText(shiftText, 8) - 6, 29, 8,
            labyrinthShiftWarning_ ? Color{255, 116, 66, 255} : Color{208, 166, 96, 230});
    }
    if (UfoEnterAvailable()) {
        const char* prompt = "F ENTER UFO";
        DrawText(prompt, pixelWidth_ / 2 - MeasureText(prompt, 12) / 2, pixelHeight_ - 48, 12, Color{135, 245, 255, 255});
    }
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
    if (activeWeapon_ == WeaponType::InfinityGauntlet && gauntletSnapCharging_) {
        const char* snapText = TextFormat("SNAP %d%%", static_cast<int>(gauntletSnapCharge_ * 100.0f));
        DrawText(snapText, 100, 117, 8, Color{255, 205, 95, 255});
    } else if (activeWeapon_ == WeaponType::LonginusSpear && longinusJudgmentCharging_) {
        const char* stigmaText = TextFormat("STIGMA %d%%", static_cast<int>(longinusJudgmentCharge_ * 100.0f));
        DrawText(stigmaText, 100, 117, 8, Color{255, 218, 88, 255});
    } else if (activeWeapon_ == WeaponType::InfinityGauntlet && gauntletMode_ == GauntletMode::Blink) {
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

    struct HudBossBar {
        float health;
        float activity;
        Color background;
        Color fill;
        Color border;
        Color labelColor;
        const char* label;
    };
    HudBossBar bossBars[32];
    int bossCount = 0;
    auto addBossBar = [&](float health, float activity, Color background, Color fill, Color border, Color labelColor, const char* label) {
        if (bossCount >= 32) return;
        bossBars[bossCount++] = {
            std::clamp(health, 0.0f, 1.0f),
            activity,
            background,
            fill,
            border,
            labelColor,
            label
        };
    };
    auto distanceActivity = [&](Vector3 position, float bonus = 0.0f) {
        float dist = Vector3Distance(camera_.position, position);
        return bonus + std::max(0.0f, 240.0f - dist);
    };

    if (bethlehem_.active) {
        float bh = bethlehem_.maxHealth > 0.0f ? bethlehem_.health / bethlehem_.maxHealth : 0.0f;
        Color bethFill = TutorialMode() ? Color{140, 140, 155, 255} : Color{255, 190, 60, 255};
        Color bethBorder = TutorialMode() ? Color{180, 180, 195, 210} : Color{255, 220, 140, 210};
        Color bethLabel = TutorialMode() ? Color{180, 180, 195, 255} : Color{255, 225, 150, 255};
        float laserBonus = bethlehem_.laserPhase != BethlehemLaserPhase::Inactive ? 120.0f : 0.0f;
        addBossBar(bh, distanceActivity(bethlehem_.position, laserBonus), Color{18, 10, 5, 220}, bethFill, bethBorder, bethLabel, "STAR OF BETHLEHEM");
    }
    if (throneAngel_.active) {
        float th = throneAngel_.maxHealth > 0.0f ? throneAngel_.health / throneAngel_.maxHealth : 0.0f;
        addBossBar(th, distanceActivity(throneAngel_.position, throneAngel_.pulseTimer < 1.5f ? 80.0f : 35.0f),
            Color{18, 18, 22, 220}, Color{238, 242, 248, 255}, Color{245, 250, 255, 220}, Color{245, 250, 255, 255},
            TextFormat("THRONE ANGEL  CHERUBS %d", static_cast<int>(cherubs_.size())));
    }
    for (size_t si = 0; si < seraphs_.size(); ++si) {
        const SeraphBoss& seraph = seraphs_[si];
        if (!seraph.active || seraph.edenApocalypse) continue;
        float sh = seraph.maxHealth > 0.0f ? seraph.health / seraph.maxHealth : 0.0f;
        addBossBar(sh, distanceActivity(seraph.position, seraph.attackFlash > 0.0f ? 95.0f : 20.0f),
            Color{36, 24, 8, 220}, Color{255, 210, 80, 255}, Color{255, 240, 170, 230}, Color{255, 235, 155, 255},
            TextFormat("SERAPH %d", static_cast<int>(si + 1)));
    }
    if (scavengerUfo_.state == ScavengerUfoState::Active
        || scavengerUfo_.state == ScavengerUfoState::Escaping
        || scavengerUfo_.state == ScavengerUfoState::DefeatedFalling) {
        float uh = scavengerUfo_.maxHealth > 0.0f ? scavengerUfo_.health / scavengerUfo_.maxHealth : 0.0f;
        float ufoBonus = scavengerUfo_.state == ScavengerUfoState::Escaping ? 130.0f
            : scavengerUfo_.state == ScavengerUfoState::DefeatedFalling ? 90.0f : 45.0f;
        const char* label = scavengerUfo_.state == ScavengerUfoState::Escaping ? "SCAVENGER UFO ESCAPING"
            : scavengerUfo_.state == ScavengerUfoState::DefeatedFalling ? "SCAVENGER UFO FALLING"
            : TextFormat("SCAVENGER UFO %d/%d", scavengerUfo_.collected, config_.ufoCollectRequired);
        addBossBar(uh, distanceActivity(scavengerUfo_.position, ufoBonus),
            Color{5, 18, 22, 220}, Color{95, 225, 255, 255}, Color{165, 245, 255, 220}, Color{170, 245, 255, 255}, label);
    }
    if (warRider_.active) {
        float wh = warRider_.maxHealth > 0.0f ? warRider_.health / warRider_.maxHealth : 0.0f;
        addBossBar(wh, distanceActivity(warRider_.position, warRider_.chargeTimeLeft > 0.0f ? 130.0f : 40.0f),
            Color{36, 8, 8, 220}, Color{255, 58, 38, 255}, Color{255, 150, 105, 230}, Color{255, 145, 95, 255}, "HORSEMAN: WAR");
    }
    if (conquestRider_.active) {
        float ch = conquestRider_.maxHealth > 0.0f ? conquestRider_.health / conquestRider_.maxHealth : 0.0f;
        addBossBar(ch, distanceActivity(conquestRider_.position, conquestRider_.summonTimer < 1.5f ? 95.0f : 25.0f),
            Color{12, 28, 8, 220}, Color{165, 245, 75, 255}, Color{220, 255, 150, 230}, Color{205, 255, 135, 255}, "HORSEMAN: CONQUEST");
    }
    if (famineRider_.active) {
        float fh = famineRider_.maxHealth > 0.0f ? famineRider_.health / famineRider_.maxHealth : 0.0f;
        addBossBar(fh, distanceActivity(famineRider_.position, famineRider_.scaleTipTimer > 0.0f ? 105.0f : 30.0f),
            Color{12, 10, 8, 220}, Color{142, 118, 62, 255}, Color{210, 178, 95, 220}, Color{210, 178, 100, 255}, "HORSEMAN: FAMINE");
    }
    if (deathRider_.active) {
        float dh = deathRider_.maxHealth > 0.0f ? deathRider_.health / deathRider_.maxHealth : 0.0f;
        float soulRatio = config_.deathRiderSoulThreshold > 0
            ? static_cast<float>(deathRider_.souls) / static_cast<float>(config_.deathRiderSoulThreshold)
            : 0.0f;
        addBossBar(dh, distanceActivity(deathRider_.position, soulRatio * 120.0f),
            Color{10, 10, 14, 220}, Color{132, 136, 148, 255}, Color{190, 194, 208, 220}, Color{190, 194, 208, 255},
            TextFormat("HORSEMAN: DEATH %d/%d", deathRider_.souls, config_.deathRiderSoulThreshold));
    }
    for (const Enemy& enemy : enemies_) {
        if (enemy.type != EnemyType::Boss && enemy.type != EnemyType::Duelist
            && enemy.type != EnemyType::DummyBoss && enemy.type != EnemyType::SlimeKing
            && enemy.type != EnemyType::Minotaur) continue;
        float health = enemy.maxHealth > 0.0f ? enemy.health / enemy.maxHealth : 0.0f;
        Color barColor = enemy.type == EnemyType::Duelist ? Color{255, 210, 105, 255}
            : enemy.type == EnemyType::DummyBoss ? Color{140, 140, 155, 255}
            : enemy.type == EnemyType::SlimeKing ? Color{100, 220, 140, 255}
            : enemy.type == EnemyType::Minotaur ? Color{198, 92, 48, 255}
            : health < 0.45f ? Color{255, 75, 160, 255}
            : Color{160, 115, 255, 255};
        Color borderColor = enemy.type == EnemyType::DummyBoss ? Color{180, 180, 195, 210}
            : enemy.type == EnemyType::Minotaur ? Color{238, 174, 98, 220}
            : Color{230, 210, 255, 210};
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
        } else if (enemy.type == EnemyType::Minotaur) {
            bossLabel = enemy.burstCount == 1 && enemy.burstTimer > 0.0f ? "MINOTAUR CHARGING" : "MINOTAUR";
            labelColor = Color{238, 174, 98, 255};
        } else {
            bossLabel = "GEOMETRY LORD";
            labelColor = Color{230, 210, 255, 255};
        }
        float hitBonus = std::max(0.0f, 5.0f - (survivalTime_ - enemy.lastDamageTime)) * 35.0f;
        addBossBar(health, distanceActivity(BodyPosition(enemy.body), hitBonus), Color{18, 10, 30, 220}, barColor, borderColor, labelColor, bossLabel);
    }

    for (int i = 0; i < bossCount - 1; ++i) {
        for (int j = i + 1; j < bossCount; ++j) {
            if (bossBars[j].activity > bossBars[i].activity) {
                std::swap(bossBars[i], bossBars[j]);
            }
        }
    }

    int barX = 76, barW = 270;
    int bossBarY0 = 29;
    int visibleBossBars = std::min(bossCount, 3);
    for (int slot = 0; slot < visibleBossBars; ++slot) {
        const HudBossBar& bar = bossBars[slot];
        int barY = bossBarY0 + slot * 18;
        DrawRectangle(barX, barY, barW, 6, bar.background);
        DrawRectangle(barX, barY, static_cast<int>(barW * bar.health), 6, bar.fill);
        DrawRectangleLines(barX, barY, barW, 6, bar.border);
        DrawText(bar.label, barX, barY + 9, 8, bar.labelColor);
    }

    if (bossCount > 3) {
        DrawText(TextFormat("+%d MORE BOSSES", bossCount - 3), barX, bossBarY0 + 3 * 18 + 9, 7, Color{160, 155, 170, 220});
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
        const char* kHint = "F11: Fullscreen\nK: Controls\n`/~: Console\nR: Restart";
        int kw = MeasureText(kHint, 9);
        int kx = pixelWidth_ - kw - 8, ky = pixelHeight_ - 150;
        DrawRectangle(kx - 3, ky - 1, kw + 6, 11, Color{0, 0, 0, 140});
        DrawText(kHint, kx, ky, 9, Color{255, 235, 140, 230});
    }
}
