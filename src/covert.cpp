#include "covert.h"

#include <iostream>
#include <string>

#include "gameplay/gamestate.h"
#include "gameplay/mainchar.h"

#ifndef NO_SDL
#include <SDL_stdinc.h>
#include <SDL.h>
#else
typedef int32_t SDL_Keycode;
enum {
	SDLK_RETURN = 13,
	SDLK_ESCAPE = 27,
	SDLK_LEFT = 37,
	SDLK_UP = 38,
	SDLK_RIGHT = 39,
	SDLK_DOWN = 40,
};
void SDL_Quit()  {}
#endif

// TODO: enforce variable casing
// TODO: style guide
// TODO: consistency in spec (unique_ptr vs new)


std::unique_ptr<MainChar> mainChar;
std::unique_ptr<GameState> gameState;

#ifndef NO_SDL
extern void play_music(MidiFile);
#endif

// Move the finite state machine to its next screen and redraw. Shared by the
// SDL key handler and the console (NO_SDL) driver.
static void advanceScreen()
{
  gameState->getNextScreen();
  gameState->createGUI(gameState->currentScreen);
}

static const SDL_Keycode mapDOWN = SDLK_DOWN;  // TODO: key mapping
static const SDL_Keycode mapUP = SDLK_UP;
static const SDL_Keycode mapLEFT = SDLK_LEFT;
static const SDL_Keycode mapRIGHT = SDLK_RIGHT;
static const SDL_Keycode mapENTER = SDLK_RETURN;
static const SDL_Keycode mapESC = SDLK_ESCAPE;

#ifndef NO_SDL
namespace kbd {
void handler(SDL_Event& e);
}
#endif

int do_covert()
{
#ifndef NO_SDL
  SDL_SetMainReady();
  SDL_Init(SDL_INIT_EVERYTHING);
#endif

  mainChar = std::make_unique<MainChar>();

  gameState = std::make_unique<GameState>();
  gameState->initGUI();
  gameState->createGUI(gameState->currentScreen);

#ifndef NO_SDL
  play_music(MidiFile::Intro);
#endif

  bool quit = false;
#ifndef NO_SDL
  SDL_Event e;
#else
  std::string line;
#endif
  while(!quit) {
    if ( gameState->guiShouldRefresh() ) {  // TODO-debug
      gameState->getNextScreen();
      gameState->createGUI(gameState->currentScreen);
    }
#ifndef NO_SDL
    while(SDL_PollEvent(&e) != 0) {
      switch (e.type) {
        case SDL_QUIT:
          quit = true;
          break;
        case SDL_KEYDOWN:
          kbd::handler(e);
          break;
        case SDL_WINDOWEVENT:
          // The window is resizable; redraw when its size changes or it is
          // uncovered. SDL_RenderSetLogicalSize keeps the 320x200 aspect and
          // letterboxes, so this just repaints.
          if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
              e.window.event == SDL_WINDOWEVENT_EXPOSED) {
            gameState->createGUI(gameState->currentScreen);
          }
          break;
        default:
          break;
      }  // e->type
    }
#else
    // Console-only driver: read a line from stdin and translate it into the
    // same actions the SDL key handler performs. EOF exits the program.
    if (!std::getline(std::cin, line)) {
      quit = true;
      break;
    }
    const char ch = line.empty() ? '\n' : line[0];
    switch (gameState->dialogType) {
      case DialogType::INFOTIMER:
        break;  // advanced automatically by guiShouldRefresh()
      case DialogType::MENU:
      case DialogType::MINIMENU:
        if (ch == 'w' || ch == 'W') {
          gameState->chooseMenuPrev();
          gameState->createGUI(gameState->currentScreen);
        } else if (ch == 's' || ch == 'S') {
          gameState->chooseMenuNext();
          gameState->createGUI(gameState->currentScreen);
        } else if (ch == 'q') {
          gameState->selectMenuDefault();
          advanceScreen();
        } else {
          gameState->selectMenuState();
          advanceScreen();
        }
        break;
      default:
        advanceScreen();
        break;
    }
#endif
    if ( gameState->shouldQuit ) { quit = true; }
  }

  SDL_Quit();  // TODO: replace with each subsystem quit by ref?

  return EXIT_SUCCESS;
}

#ifndef NO_SDL
namespace kbd {
void advance() {
  advanceScreen();
}

void handler(SDL_Event& e) {
  switch ( gameState->dialogType ) {
    case DialogType::INFOTIMER:
      // Timer-driven screens (splashes) advance themselves; ignore key presses.
      break;
    case DialogType::INFO:
      // Informational screens advance on any key press.
      // TODO: distinguish per-screen behavior instead of "any key".
      advance();
      break;
    case DialogType::MENU:
    case DialogType::MINIMENU:
      switch (e.key.keysym.sym) {
        case mapDOWN:
          gameState->chooseMenuNext();
          gameState->createGUI(gameState->currentScreen);
          break;
        case mapUP:
          gameState->chooseMenuPrev();
          gameState->createGUI(gameState->currentScreen);
          break;
        case mapENTER:
          gameState->selectMenuState();
          advance();
          break;
        case mapESC:
          gameState->selectMenuDefault();
          advance();
          break;
        default:
          break;
      }  // ...keysym.sym
      break;
    case DialogType::TEXTENTRY:
      // TODO: real text entry. For now any key accepts and advances.
      advance();
      break;
    case DialogType::SKILLSELECT:
      switch (e.key.keysym.sym) {  // TODO
        case mapLEFT:
        case mapRIGHT:
          // TODO: change the highlighted skill level
          break;
        default:
          advance();
          break;
      }  // ...keysym.sym
      break;
    case DialogType::MINIGAME:
    default:
      break;
  }  // gameState->guiGetDialogType
}
}
#endif
