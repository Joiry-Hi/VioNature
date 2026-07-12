#include "Game.h"

#include "raylib.h"

#include <memory>

#ifdef _WIN32
extern "C" void DisableWindowsIME();
#endif

#ifdef PLATFORM_WEB
#include <emscripten.h>

EM_JS(void, InstallWebMouseInput, (), {
    if (Module.__vioMouseInstalled) return;
    Module.__vioMouseInstalled = true;
    Module.__vioMouseDx = 0;
    Module.__vioMouseDy = 0;

    var canvas = Module.canvas;
    if (!canvas) return;
    canvas.setAttribute('tabindex', '0');
    canvas.style.cursor = 'none';

    function isLocked() {
        return document.pointerLockElement === canvas
            || document.mozPointerLockElement === canvas
            || document.webkitPointerLockElement === canvas;
    }

    function fullscreenElement() {
        return document.fullscreenElement
            || document.webkitFullscreenElement
            || document.mozFullScreenElement
            || document.msFullscreenElement;
    }

    function requestFullscreen() {
        var target = canvas.parentElement || canvas;
        if (fullscreenElement()) return;
        var request = target.requestFullscreen
            || target.webkitRequestFullscreen
            || target.mozRequestFullScreen
            || target.msRequestFullscreen;
        if (request) {
            try {
                var result = request.call(target);
                if (result && result.catch) result.catch(function() {});
            } catch (error) {}
        }
    }

    function requestControl() {
        canvas.focus();
        Module.__vioMouseDx = 0;
        Module.__vioMouseDy = 0;
        requestFullscreen();
        if (!isLocked() && canvas.requestPointerLock) {
            try {
                canvas.requestPointerLock();
            } catch (error) {}
        }
    }

    canvas.addEventListener('click', requestControl);
    canvas.addEventListener('mousedown', requestControl);
    document.addEventListener('pointerlockchange', function() {
        Module.__vioMouseDx = 0;
        Module.__vioMouseDy = 0;
    });
    document.addEventListener('fullscreenchange', function() {
        Module.__vioMouseDx = 0;
        Module.__vioMouseDy = 0;
    });
    window.addEventListener('mousemove', function(event) {
        if (!isLocked()) return;
        Module.__vioMouseDx += event.movementX || event.mozMovementX || event.webkitMovementX || 0;
        Module.__vioMouseDy += event.movementY || event.mozMovementY || event.webkitMovementY || 0;
    }, true);
});

EM_JS(float, ConsumeWebMouseDeltaX, (), {
    var value = Module.__vioMouseDx || 0;
    Module.__vioMouseDx = 0;
    return value;
});

EM_JS(float, ConsumeWebMouseDeltaY, (), {
    var value = Module.__vioMouseDy || 0;
    Module.__vioMouseDy = 0;
    return value;
});
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

static std::unique_ptr<Game> g_Game;
static int                   g_SmokeFrames = 0;
static bool                  g_SmokeTest = false;
static bool                  g_Running = true;
static bool                  g_Initialized = false;

extern "C" void EMSCRIPTEN_KEEPALIVE GameMainLoop() {
    if (!g_Game) {
        return;
    }

    float dt = GetFrameTime();
    g_Game->Update(dt);

    BeginDrawing();
    ClearBackground(Color{8, 8, 10, 255});
    g_Game->Draw();
    EndDrawing();

    ++g_SmokeFrames;
    if (g_SmokeTest && g_SmokeFrames > 180) {
        g_Running = false;
#ifdef PLATFORM_WEB
        emscripten_cancel_main_loop();
#endif
        return;
    }
    if (g_Game->WantsQuit()) {
        g_Running = false;
#ifdef PLATFORM_WEB
        emscripten_cancel_main_loop();
#endif
    }
}

extern "C" void EMSCRIPTEN_KEEPALIVE StartGame() {
    if (g_Initialized) {
        return;
    }

    constexpr int screenWidth = 1280;
    constexpr int screenHeight = 720;
    g_Initialized = true;
    g_Running = true;
    g_SmokeFrames = 0;

    InitWindow(screenWidth, screenHeight, "VioNature - Arena Prototype");

#ifdef _WIN32
    DisableWindowsIME();
#elif !defined(PLATFORM_WEB)
    setenv("GTK_IM_MODULE", "gtk-im-context-simple", 1);
    setenv("QT_IM_MODULE", "simple", 1);
    setenv("XMODIFIERS", "@im=none", 1);
#endif

    SetTargetFPS(60);
    DisableCursor();
#ifdef PLATFORM_WEB
    InstallWebMouseInput();
#endif
    g_Game = std::make_unique<Game>();

#ifdef PLATFORM_WEB
    emscripten_set_main_loop(GameMainLoop, 0, 1);
#endif
}

static void ShutdownGame() {
    g_Game.reset();
    EnableCursor();
    CloseWindow();
    g_Initialized = false;
}

int main(int argc, char** argv) {
    g_SmokeTest = argc > 1 && TextIsEqual(argv[1], "--smoke-test");

#ifdef PLATFORM_WEB
    return 0;
#else
    StartGame();

    while (g_Running) {
        GameMainLoop();
    }
    ShutdownGame();

    return 0;
#endif
}
