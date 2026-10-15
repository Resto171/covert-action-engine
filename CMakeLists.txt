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

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)
find_package(SDL2_mixer CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)
find_package(PNG REQUIRED)

set(CAE_SOURCES
        src/main.cpp
        src/covert.cpp
        src/gameplay/casegenerator.cpp
        src/gameplay/agentgenerator.cpp
        src/gameplay/gamestate.cpp
        src/minigame/combat.cpp
        src/minigame/driving.cpp
        src/minigame/cryptography.cpp
        src/minigame/electronics.cpp
        src/disk/loadsave.cpp
        src/sound/music.cpp
        src/gui/gui.cpp
        src/gui/menu.cpp
        src/gui/point.cpp
)

add_executable(${PROJECT_NAME} ${CAE_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src)

# SDL_MAIN_HANDLED keeps the standard main() entry point (no SDL2main / WinMain redirect).
target_compile_definitions(${PROJECT_NAME} PRIVATE SDL_MAIN_HANDLED)

if(PLACEHOLDERS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLACEHOLDERS)
endif()

if(NO_SDL)
    target_compile_definitions(${PROJECT_NAME} PRIVATE NO_SDL)
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
        SDL2::SDL2
        SDL2_ttf::SDL2_ttf
        SDL2_mixer::SDL2_mixer
        SDL2_image::SDL2_image
        PNG::PNG
)

# Copy runtime resources (currently just the placeholder sound) next to the binary.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/src/test.wav"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/test.wav"
        COMMENT "Copying runtime resources to ${CMAKE_CURRENT_BINARY_DIR}")
