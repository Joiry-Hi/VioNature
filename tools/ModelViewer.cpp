// VioNature Model Viewer — standalone .obj / .glb preview tool.
// Renders with the same 426x240 pixel-art pipeline as the game.
//
// Usage:  ModelViewer <path/to/model.obj|model.glb>
//   LMB drag  – orbit
//   Wheel     – zoom
//   F         – toggle wireframe
//   H         – toggle info overlay
//   R         – reset view
//   Space     – pause auto-spin
//   P         – toggle HUD / grid
//   Tab       – show cursor (hold)

#include "raylib.h"
#include "raymath.h"
#include <cmath>
#include <cstdlib>

static constexpr int kPixelW = 426;
static constexpr int kPixelH = 240;


int main(int argc, char** argv) {
    if (argc < 2) {
        TraceLog(LOG_WARNING, "Usage: ModelViewer <model.obj|model.glb>");
        TraceLog(LOG_INFO, "  Drag & drop a model file, or pass its path as the first argument.");
        return 1;
    }

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "VioNature Model Viewer");
    SetTargetFPS(60);
    DisableCursor();

    RenderTexture2D pixelTarget = LoadRenderTexture(kPixelW, kPixelH);
    SetTextureFilter(pixelTarget.texture, TEXTURE_FILTER_POINT);

    Model model = LoadModel(argv[1]);
    if (!IsModelValid(model)) {
        TraceLog(LOG_ERROR, "Failed to load: %s", argv[1]);
        UnloadRenderTexture(pixelTarget);
        CloseWindow();
        return 1;
    }

    // Bounding sphere
    BoundingBox bb = GetMeshBoundingBox(model.meshes[0]);
    for (int i = 1; i < model.meshCount; ++i) {
        BoundingBox mi = GetMeshBoundingBox(model.meshes[i]);
        bb.min.x = std::fmin(bb.min.x, mi.min.x);
        bb.min.y = std::fmin(bb.min.y, mi.min.y);
        bb.min.z = std::fmin(bb.min.z, mi.min.z);
        bb.max.x = std::fmax(bb.max.x, mi.max.x);
        bb.max.y = std::fmax(bb.max.y, mi.max.y);
        bb.max.z = std::fmax(bb.max.z, mi.max.z);
    }
    Vector3 center = Vector3Scale(Vector3Add(bb.min, bb.max), 0.5f);
    float fitRadius = Vector3Distance(bb.min, bb.max) * 0.55f;
    if (fitRadius < 0.01f) fitRadius = 1.0f;

    int triCount = 0;
    for (int i = 0; i < model.meshCount; ++i) triCount += model.meshes[i].triangleCount;
    TraceLog(LOG_INFO, "Loaded  meshes=%d  tris=%d  radius=%.1f", model.meshCount, triCount, fitRadius);

    Camera3D camera = {};
    camera.position = Vector3{0.0f, fitRadius * 0.35f, fitRadius * 2.2f};
    camera.target = center;
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    float orbitYaw = 0.0f, orbitPitch = 0.0f, orbitDist = fitRadius * 2.2f;
    bool wireframe = false, paused = false, showInfo = true, hideUI = false;
    float autoSpin = 0.0f, infoTimer = 2.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (!paused) autoSpin += dt * 0.22f;

        if (IsKeyPressed(KEY_F)) wireframe = !wireframe;
        if (IsKeyPressed(KEY_H)) showInfo = !showInfo;
        if (IsKeyPressed(KEY_SPACE)) paused = !paused;
        if (IsKeyPressed(KEY_P)) hideUI = !hideUI;
        if (IsKeyPressed(KEY_R)) { orbitYaw = 0.0f; orbitPitch = 0.0f; orbitDist = fitRadius * 2.2f; autoSpin = 0.0f; }
        if (IsKeyPressed(KEY_TAB)) EnableCursor();
        if (IsKeyReleased(KEY_TAB)) DisableCursor();

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 delta = GetMouseDelta();
            orbitYaw -= delta.x * 0.28f;
            orbitPitch += delta.y * 0.28f;
            orbitPitch = Clamp(orbitPitch, -85.0f, 85.0f);
            autoSpin = 0.0f; infoTimer = 2.0f;
        }
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            orbitDist -= wheel * fitRadius * 0.35f;
            orbitDist = Clamp(orbitDist, fitRadius * 0.5f, fitRadius * 8.0f);
            infoTimer = 2.0f;
        }
        infoTimer = std::fmax(0.0f, infoTimer - dt);

        float yawRad = (paused ? orbitYaw : orbitYaw + autoSpin) * DEG2RAD;
        float pitchRad = orbitPitch * DEG2RAD;
        camera.position = Vector3Add(center, Vector3{
            std::cos(yawRad) * std::cos(pitchRad) * orbitDist,
            std::sin(pitchRad) * orbitDist,
            std::sin(yawRad) * std::cos(pitchRad) * orbitDist
        });
        camera.target = center;

        // ── Render to pixel buffer ───────────────────────────────
        BeginTextureMode(pixelTarget);
        ClearBackground(Color{8, 8, 10, 255});

        BeginMode3D(camera);
        if (wireframe) {
            DrawModelWires(model, Vector3Zero(), 1.0f, Color{130, 145, 160, 200});
        }
        DrawModel(model, Vector3Zero(), 1.0f, WHITE);
        if (!hideUI) DrawGrid(20, fitRadius * 0.18f);
        EndMode3D();

        // HUD
        int hudAlpha = (showInfo && !hideUI) ? 235 : static_cast<int>(Clamp(infoTimer, 0.0f, 1.0f) * 235.0f);
        if (hudAlpha > 0) {
            unsigned char a = static_cast<unsigned char>(hudAlpha);
            DrawRectangle(3, 3, 210, 56, Fade(Color{0, 0, 0, 190}, a / 255.0f));
            DrawText(TextFormat("File  %s", GetFileName(argv[1])), 7, 6, 8, Fade(RAYWHITE, a / 255.0f));
            DrawText(TextFormat("Tris  %d   Meshes  %d   Scale  %.1f", triCount, model.meshCount, fitRadius), 7, 17, 7, Fade(Color{190, 200, 210, 255}, a / 255.0f));
            const char* mode = paused ? "PAUSED" : wireframe ? "WIRE" : "SOLID";
            DrawText(TextFormat("Mode  %s   FPS %d", mode, GetFPS()), 7, 28, 7, Fade(Color{165, 175, 185, 255}, a / 255.0f));
            DrawText("LMB orbit  Wheel zoom  F wire  H info  P clean  R reset  Space pause  Tab cursor",
                     7, 41, 6, Fade(Color{130, 140, 150, 255}, static_cast<unsigned char>(a * 0.55f)));
        }

        EndTextureMode();

        // ── Upscale blit ─────────────────────────────────────────
        BeginDrawing();
        ClearBackground(BLACK);
        float scale = std::fmin(static_cast<float>(GetScreenWidth()) / kPixelW,
                                static_cast<float>(GetScreenHeight()) / kPixelH);
        int dw = static_cast<int>(kPixelW * scale), dh = static_cast<int>(kPixelH * scale);
        DrawTexturePro(pixelTarget.texture,
            Rectangle{0, 0, static_cast<float>(kPixelW), -static_cast<float>(kPixelH)},
            Rectangle{static_cast<float>((GetScreenWidth() - dw) / 2),
                      static_cast<float>((GetScreenHeight() - dh) / 2),
                      static_cast<float>(dw), static_cast<float>(dh)},
            Vector2Zero(), 0.0f, WHITE);
        EndDrawing();
    }

    UnloadModel(model);
    UnloadRenderTexture(pixelTarget);
    CloseWindow();
    return 0;
}
