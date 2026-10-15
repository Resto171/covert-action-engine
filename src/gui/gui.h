#pragma once

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define SCREEN_SCALE 5

// UI text size, in logical (320x200 design) pixels, and the line spacing used
// to lay menus out. The font is rasterised at UI_FONT_SIZE * SCREEN_SCALE so
// the glyphs are crisp; this single value therefore controls how big the text
// looks on screen. Raise it to make the whole UI text bigger.
#define UI_FONT_SIZE 12
#define UI_LINE_HEIGHT 14

#include <array>
#include <cstdint>
/// #include <string>
#include <vector>
#include <memory>

#ifndef NO_SDL
#include <SDL_ttf.h>
#else
typedef struct SDL_Rect  { int x,y,w,h; }  SDL_Rect;
typedef struct SDL_Point  { int x,y; }  SDL_Point;
// SDL_Color normally comes from SDL_ttf.h/SDL.h; provide it for the NO_SDL build.
typedef struct SDL_Color  { uint8_t r,g,b,a; }  SDL_Color;
#endif

// The fixed UI palette. These are the original game's colours; they are exposed
// here so screens drawn outside gui.cpp (e.g. the minigames) can use them.
namespace CAColor {
    inline constexpr SDL_Color BLACK   = { 0x00, 0x00, 0x00, 0xFF };
    inline constexpr SDL_Color DKRED   = { 0xAA, 0x00, 0x00, 0xFF };
    inline constexpr SDL_Color DKGREEN = { 0x00, 0xAA, 0x00, 0xFF };
    inline constexpr SDL_Color BROWN   = { 0xAA, 0x55, 0x00, 0xFF };
    inline constexpr SDL_Color DKBLUE  = { 0x00, 0x00, 0xAA, 0xFF };
    inline constexpr SDL_Color UNK     = { 0x00, 0x00, 0x00, 0xFF };
    inline constexpr SDL_Color DKCYAN  = { 0x00, 0xAA, 0xAA, 0xFF };
    inline constexpr SDL_Color GREY    = { 0xAA, 0xAA, 0xAA, 0xFF };
    inline constexpr SDL_Color DKGREY  = { 0x55, 0x55, 0x55, 0xFF };
    inline constexpr SDL_Color RED     = { 0xFF, 0x55, 0x55, 0xFF };
    inline constexpr SDL_Color GREEN   = { 0x55, 0xFF, 0x55, 0xFF };
    inline constexpr SDL_Color YELLOW  = { 0xFF, 0xFF, 0x55, 0xFF };
    inline constexpr SDL_Color BLUE    = { 0x55, 0x55, 0xFF, 0xFF };
    inline constexpr SDL_Color MAGENTA = { 0xFF, 0x55, 0xFF, 0xFF };
    inline constexpr SDL_Color CYAN    = { 0x55, 0xFF, 0xFF, 0xFF };
    inline constexpr SDL_Color WHITE   = { 0xFF, 0xFF, 0xFF, 0xFF };
}

/// #include "../disk/loadsave.h"
#include "../types.h"
/// #include "point.h"
#include "menu.h"

/// #include "../gameplay/gamestate.h"


class GUI {
public:
    GUI() = default;
    ~GUI();
    GUI(const GUI&) = delete;
    GUI& operator=(const GUI&) = delete;
    void initGUI();
    void createGUI(GameScreen screen);
    void createGUIMenu(std::vector<std::string> choice_strings, SDL_Rect r0);
    void updateGUIMenu(std::vector<MenuChoice> choices, SDL_Rect r0);
    void screenMenu(GameScreen screen, std::vector<std::string> choice_strings, SDL_Rect r0);
    void setBGColor(const SDL_Color c);
    void fillBox(const SDL_Color c, const SDL_Rect r);
    void drawBox(const SDL_Color c, const SDL_Rect r);
    void drawText(const SDL_Color c, const char* txt, const SDL_Point pt);
    void centerText(const SDL_Color c, const char* txt, const int y);
    void chooseMenuPrev();
    void chooseMenuNext();
    void selectMenuState();
    void selectMenuDefault();
    bool guiShouldRefresh();
    // Rendered for minigame screens. The base class draws nothing; GameState
    // overrides it to draw the currently running minigame.
    virtual void drawMiniGame();
#ifdef PLACEHOLDERS
    void _imagePlaceholder(const int&, const int&, const int&, const int&);
#endif
///   void /*TODO*/choiceS  electionCallback()  { NULL; }
/// protected:
#ifndef NO_SDL
    SDL_Window* window { nullptr };
    SDL_Renderer* renderer { nullptr };
    TTF_Font* ttf { nullptr };
    Uint64 next_screen_tick { 0 };
#else
	uint64_t next_screen_tick { 0 };  // TODO
#endif
///   /* TODO: surfaces and textures and etc. */
/// private:
    std::vector<MenuChoice> currentMenu;
    GameScreen currentMenuScreen { GameScreen::UNKNOWN };
    int selectedChoice { 0 };
    DialogType dialogType { DialogType::INFOTIMER };
};


// The GUI for the running session (created in do_covert()).
extern std::unique_ptr<GUI> gui;
