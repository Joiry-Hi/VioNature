#include "Game.h"

#include "raylib.h"

#ifdef _WIN32
extern "C" void DisableWindowsIME();
#endif

int main(int argc, char** argv) {
    constexpr int screenWidth = 1280;
    constexpr int screenHeight = 720;
    bool smokeTest = argc > 1 && TextIsEqual(argv[1], "--smoke-test");

    InitWindow(screenWidth, screenHeight, "VioNature - Arena Prototype");

#ifdef _WIN32
    DisableWindowsIME();
#else
    // Linux: use simple IME context to prevent IBus/Fcitx from intercepting keys
    setenv("GTK_IM_MODULE", "gtk-im-context-simple", 1);
    setenv("QT_IM_MODULE", "simple", 1);
    setenv("XMODIFIERS", "@im=none", 1);
#endif

    SetTargetFPS(60);
    DisableCursor();

    {
        Game game;
        int smokeFrames = 0;

        while (!game.WantsQuit()) {
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
