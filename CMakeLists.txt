cmake_minimum_required(VERSION 3.16)
project(MyShooter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================================================
# 1. 配置并编译 Raylib
# =========================================================
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
add_subdirectory(external/raylib-src)

# =========================================================
# 2. 配置并编译 Jolt Physics
# =========================================================
# 关闭所有不必要的测试和示例
set(TARGET_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(TARGET_HELLO_WORLD OFF CACHE BOOL "" FORCE)
set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
set(TARGET_VIEWER OFF CACHE BOOL "" FORCE)

# [关键修复] 强制关闭 Vulkan 支持
# 这将阻止 Jolt 寻找 glslc 编译器，也不再编译 shader
set(USE_VULKAN OFF CACHE BOOL "" FORCE)

# 关闭 SSE4.2 以外的高级指令集检测（可选，保持构建稳定）
set(USE_SSE4_2 ON CACHE BOOL "" FORCE)

# 指向 Build 目录
add_subdirectory(external/joltphysics-src/Build)

# =========================================================
# 3. 构建你的游戏
# =========================================================
add_executable(MyShooter
    src/main.cpp
    src/GameConfig.cpp
    src/Game.cpp
    src/GamePlayer.cpp
    src/GameWeapons.cpp
    src/GameProjectiles.cpp
    src/GameEnemies.cpp
    src/GameWorld.cpp
    src/GameRender.cpp
    src/PhysicsWorld.cpp
    src/WeaponViewModel.cpp
    src/platform_win.cpp
)
if(WIN32)
    target_sources(MyShooter PRIVATE src/resource.rc)
endif()

# 设置头文件路径
target_include_directories(MyShooter PRIVATE
    external/raylib-src/src
    external/joltphysics-src
)

# 链接库
# 注意：在 Linux 上 Jolt 可能依赖 pthread，Threads::Threads 会自动处理
find_package(Threads REQUIRED)
target_link_libraries(MyShooter PRIVATE raylib Jolt Threads::Threads)
if(WIN32)
    target_link_libraries(MyShooter PRIVATE imm32)
endif()

option(VIONATURE_NO_AUDIO "Build without audio loading/playback" OFF)
if(VIONATURE_NO_AUDIO)
    target_compile_definitions(MyShooter PRIVATE VIONATURE_NO_AUDIO)
endif()

if(EMSCRIPTEN)
    set(VIONATURE_WEB_CONFIG_DIR "${CMAKE_SOURCE_DIR}/config" CACHE PATH "Config directory preloaded into /config for web builds")
    set(VIONATURE_WEB_ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets" CACHE PATH "Assets directory preloaded into /assets for web builds")
    set_target_properties(MyShooter PROPERTIES
        OUTPUT_NAME "vionature"
        LINK_FLAGS "-sUSE_GLFW=3 -sALLOW_MEMORY_GROWTH=1 -sTOTAL_MEMORY=256MB -sINVOKE_RUN=0 -sEXPORTED_RUNTIME_METHODS=ccall --preload-file ${VIONATURE_WEB_CONFIG_DIR}@/config --preload-file ${VIONATURE_WEB_ASSETS_DIR}@/assets"
    )
    target_compile_definitions(MyShooter PRIVATE PLATFORM_WEB)
endif()

# =========================================================
# 4. 模型预览工具 (ModelViewer)
# =========================================================
add_executable(ModelViewer
    tools/ModelViewer.cpp
)

target_include_directories(ModelViewer PRIVATE
    external/raylib-src/src
)

target_link_libraries(ModelViewer PRIVATE raylib Threads::Threads)
