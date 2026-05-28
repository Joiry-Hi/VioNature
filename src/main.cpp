#include "Game.h"

#include "raylib.h"

int main(int argc, char** argv) {
    constexpr int screenWidth = 1280;
    constexpr int screenHeight = 720;
    bool smokeTest = argc > 1 && TextIsEqual(argv[1], "--smoke-test");

    InitWindow(screenWidth, screenHeight, "VioNature - Arena Prototype");
    SetTargetFPS(60);
    DisableCursor();

    {
        Game game;
        int smokeFrames = 0;

        while (!WindowShouldClose()) {
            float dt = GetFrameTime();
            game.Update(dt);

            BeginDrawing();
            ClearBackground(Color{8, 8, 10, 255});
            game.Draw();
            EndDrawing();

            ++smokeFrames;
            if (smokeTest && smokeFrames > 180) {
                break;
            }
        }
    }

    EnableCursor();
    CloseWindow();
    return 0;
}
