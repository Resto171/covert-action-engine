cmake_minimum_required(VERSION 3.20)
project(covert-action-engine VERSION 0.1.5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to a Debug build when the user does not pick one (single-config generators).
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Draw placeholder graphics instead of real game data (no data files required).
option(PLACEHOLDERS "Use generated placeholder graphics instead of real assets" ON)
# Build without SDL: console-only, no window/no rendering.
option(NO_SDL "Disable SDL entirely (console-only build)" OFF)
# Treat compiler warnings as errors.
option(WERROR "Fail the build on warnings" OFF)

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)
find_package(SDL2_mixer CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)
find_package(PNG REQUIRED)

# Engine sources, shared by the game executable and the unit tests. main.cpp is
# intentionally absent: each binary provides its own entry point.
set(CAE_CORE_SOURCES
        src/covert.cpp
        src/gameplay/casegenerator.cpp
        src/gameplay/casefile.cpp
        src/gameplay/agentgenerator.cpp
        src/gameplay/gamestate.cpp
        src/minigame/combat.cpp
        src/minigame/driving.cpp
        src/minigame/cryptography.cpp
        src/minigame/electronics.cpp
        src/disk/loadsave.cpp
        src/sound/music.cpp
        src/gui/gui.cpp
        src/gui/facegen.cpp
        src/gui/menu.cpp
        src/gui/point.cpp
)

if(MSVC)
    set(CAE_WARNINGS /W4)
    if(WERROR)
        list(APPEND CAE_WARNINGS /WX)
    endif()
else()
    set(CAE_WARNINGS -Wall -Wextra)
    if(WERROR)
        list(APPEND CAE_WARNINGS -Werror)
    endif()
endif()

add_library(cae_core STATIC ${CAE_CORE_SOURCES})

target_include_directories(cae_core PUBLIC src)

# SDL_MAIN_HANDLED keeps the standard main() entry point (no SDL2main / WinMain
# redirect). It is PUBLIC so the executable and test binaries inherit it.
target_compile_definitions(cae_core PUBLIC SDL_MAIN_HANDLED)

if(PLACEHOLDERS)
    target_compile_definitions(cae_core PUBLIC PLACEHOLDERS)
endif()

if(NO_SDL)
    target_compile_definitions(cae_core PUBLIC NO_SDL)
endif()

target_compile_options(cae_core PRIVATE ${CAE_WARNINGS})

target_link_libraries(cae_core PUBLIC
        SDL2::SDL2
        SDL2_ttf::SDL2_ttf
        SDL2_mixer::SDL2_mixer
        SDL2_image::SDL2_image
        PNG::PNG
)

add_executable(${PROJECT_NAME} src/main.cpp)
target_compile_options(${PROJECT_NAME} PRIVATE ${CAE_WARNINGS})
target_link_libraries(${PROJECT_NAME} PRIVATE cae_core)

# Unit tests.
enable_testing()
add_subdirectory(test)

# Copy runtime resources (currently just the placeholder sound) next to the binary.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/src/test.wav"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/test.wav"
        COMMENT "Copying runtime resources to ${CMAKE_CURRENT_BINARY_DIR}")
