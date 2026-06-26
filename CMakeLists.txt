cmake_minimum_required(VERSION 4.2)
project(shmup C)

set(CMAKE_C_STANDARD 11)

include(FetchContent)

FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)

add_executable(shmup src/main.c
        src/game/game.c
        src/game/game.h
        src/game/player.c
        src/game/player.h
        src/game/models.c
        src/game/models.h
        src/game/shoot/world.c
        src/game/shoot/world.h
        src/game/shoot/weapon.h
)

target_link_libraries(shmup PRIVATE raylib)

# Подключение АК
file(READ "${CMAKE_SOURCE_DIR}/assets/models/low-poly_ak-47_type_2.glb" AK_BYTES HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," AK_BYTES_C "${AK_BYTES}")

if(EXISTS "${CMAKE_SOURCE_DIR}/assets/textures/smoke.png")
    file(READ "${CMAKE_SOURCE_DIR}/assets/textures/smoke.png" SMOKE_BYTES HEX)
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," SMOKE_BYTES_C "${SMOKE_BYTES}")
    set(HAS_SMOKE_TEXTURE 1)
else()
    set(SMOKE_BYTES_C "0x00,")
    set(HAS_SMOKE_TEXTURE 0)
endif()

configure_file(
        "${CMAKE_SOURCE_DIR}/src/embedded_assets.h.in"
        "${CMAKE_BINARY_DIR}/generated/embedded_assets.h"
        @ONLY
)

configure_file(
        "${CMAKE_SOURCE_DIR}/src/smoke_assets.h.in"
        "${CMAKE_BINARY_DIR}/generated/smoke_assets.h"
        @ONLY
)

target_include_directories(shmup PRIVATE
        "${CMAKE_BINARY_DIR}/generated"
)
