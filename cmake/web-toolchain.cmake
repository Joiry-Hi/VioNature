# Web / Emscripten toolchain for VioNature
set(CMAKE_SYSTEM_NAME Emscripten)
set(CMAKE_SYSTEM_VERSION 1)

set(CMAKE_C_COMPILER emcc)
set(CMAKE_CXX_COMPILER em++)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_GLFW=3 -s ASYNCIFY -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_MEMORY=256MB")
# Flags applied only at link time for the game target (see CMakeLists)

set(CMAKE_EXECUTABLE_SUFFIX ".html")
set(CMAKE_CXX_STANDARD 17)

# Force single-threaded Jolt + no DX12
set(JPH_USE_DX12 OFF CACHE BOOL "" FORCE)

add_definitions(-DPLATFORM_WEB)
