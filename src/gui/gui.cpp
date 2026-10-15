// TODO: Anything at all non-graphics related needs to be in a ui.h. GUI should only be the graphics components of UI. This would allow a console-only mode.
#ifdef NO_SDL
#include <cstdint>
uint64_t SDL_GetTicks64()  { return 0; }
#endif

#include "gui.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>

std::unique_ptr<GUI> gui;

// Glyphs are rasterised at the size they are actually displayed at (the
// logical size times SCREEN_SCALE). Drawing the resulting texture into a
// logical-sized rectangle then cancels the renderer's upscale, so the text
// lands 1:1 on screen instead of being blown up (which made it look thick).
static constexpr int kFontPx = UI_FONT_SIZE * SCREEN_SCALE;

namespace CAColor {
    SDL_Color BLACK = { 0x00, 0x00, 0x00 };
    SDL_Color DKRED = { 0xAA, 0x00, 0x00 };
    SDL_Color DKGREEN = { 0x00, 0xAA, 0x00 };
    SDL_Color BROWN = { 0xAA, 0x55, 0x00 };
    SDL_Color DKBLUE = { 0x00, 0x00, 0xAA };
    SDL_Color UNK = { 0x00, 0x00, 0x00 };
    SDL_Color DKCYAN = { 0x00, 0xAA, 0xAA };
    SDL_Color GREY = { 0xAA, 0xAA, 0xAA };
    SDL_Color DKGREY = { 0x55, 0x55, 0x55 };
    SDL_Color RED = { 0xFF, 0x55, 0x55 };
    SDL_Color GREEN = { 0x55, 0xFF, 0x55 };
    SDL_Color YELLOW = { 0xFF, 0xFF, 0x55 };
    SDL_Color BLUE = { 0x55, 0x55, 0xFF };
    SDL_Color MAGENTA = { 0xFF, 0x55, 0xFF };
    SDL_Color CYAN = { 0x55, 0xFF, 0xFF };
    SDL_Color WHITE = { 0xFF, 0xFF, 0xFF };
}

static const SDL_Color colors[] = {
    CAColor::BLACK, CAColor::DKRED, CAColor::DKGREEN, CAColor::BROWN,
    CAColor::DKBLUE, CAColor::BLACK, CAColor::DKCYAN, CAColor::GREY,
    CAColor::DKGREY, CAColor::RED, CAColor::GREEN, CAColor::YELLOW,
    CAColor::BLUE, CAColor::MAGENTA, CAColor::CYAN, CAColor::WHITE
};

namespace {
// Screen names indexed by GameScreen (declaration order in types.h).
// Used for the console (NO_SDL) output and for placeholder labels.
const char* const kScreenNames[] = {
    "Splash1", "Splash2", "BeginMenu", "NewCharacter", "Load", "SkillPractice",
    "HallOfFame", "SkillUpgrade", "Chief", "MastermindLineup", "ShowNewClues",
    "CIAContact", "CIAMainMenu", "CIAData", "ReviewClues", "ReviewSuspects",
    "ReviewInsideInfo", "ReviewNews", "ReviewOrg", "ReviewCity", "ReviewActivity",
    "CIAIntel", "CIAAddBan", "ReviewWiretaps", "CIASam", "CIACrypto",
    "CodedMessages", "CrimeChronology", "CityMainMenu", "Airport", "Hotel",
    "HotelLoungeModal", "NewGameMaybe", "HotelSleepThruCase", "LoadSave",
    "Building1", "Building2", "Building3", "Building4", "Building5", "BuildingWatch",
    "Typewriter", "MiniGameCombatStart", "CombatArrest", "CombatRecover", "CombatCaptive",
    "MiniGameDrivingStart", "DrivingCarSelect", "DrivingArrest", "DrivingTargetGone",
    "DrivingShootout", "DrivingEscapedPursuer", "MiniGameCryptoStart", "MiniGameElecStart",
    "ElecModalBugged", "ElecModalTrace", "ElecModalAlarm", "GameExitModal", "GameExit",
};

const char* screenName(GameScreen s) {
  const int idx = static_cast<int>(s);
  const int count = static_cast<int>(sizeof(kScreenNames) / sizeof(kScreenNames[0]));
  return (idx >= 0 && idx < count) ? kScreenNames[idx] : "UNKNOWN";
}
}  // namespace


void GUI::initGUI()
{
#ifndef NO_SDL
  // Nearest-neighbour scaling keeps the pixel art crisp now that the text is
  // rasterised at its final size instead of being blown up by the renderer.
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
  SDL_CreateWindowAndRenderer(SCREEN_SCALE * SCREEN_WIDTH, SCREEN_SCALE * SCREEN_HEIGHT,
                              SDL_WINDOW_RESIZABLE, &window, &renderer);
  SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

  TTF_Init();  // TODO: move to Font class

  // TODO: bundle a font with the game instead of relying on the system.
  const char* windir = std::getenv("WINDIR");
  const std::string win_fonts = std::string(windir ? windir : "C:\\Windows") + "\\Fonts\\";

  const std::string candidates[] = {
    win_fonts + "consola.ttf",  // Consolas
    win_fonts + "lucon.ttf",    // Lucida Console
    win_fonts + "cour.ttf",     // Courier New
    win_fonts + "arial.ttf",    // Arial
    "/usr/share/fonts/liberation-mono/LiberationMono-Regular.ttf",
    "test.ttf",
  };

  ttf = nullptr;
  for (const std::string& fname : candidates) {
    ttf = TTF_OpenFont(fname.c_str(), kFontPx);
    if (ttf != nullptr) {
      fprintf(stdout, "Loaded font: %s\n", fname.c_str());
      break;
    }
  }
  if (ttf == nullptr) {
    fprintf(stderr, "Could not load a font. Tried:\n");
    for (const std::string& fname : candidates) {
      fprintf(stderr, "  %s\n", fname.c_str());
    }
    exit(EXIT_FAILURE);
  }

  SDL_Palette* palette16 = SDL_AllocPalette(16);
  SDL_SetPaletteColors(palette16, colors, 0, 16);  // TODO: figure this out
#endif
}


void GUI::setBGColor(const SDL_Color c)
{
#ifndef NO_SDL
  SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);  // TODO: use a colorlist element
  SDL_RenderClear(renderer);
#endif
}


void GUI::fillBox(const SDL_Color c, const SDL_Rect r)
{
#ifndef NO_SDL
  SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, SDL_ALPHA_OPAQUE);
  SDL_RenderFillRect(renderer, &r);
#endif
}


void GUI::drawBox(const SDL_Color c, const SDL_Rect r)
{
#ifndef NO_SDL
  SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, SDL_ALPHA_OPAQUE);
  SDL_RenderDrawRect(renderer, &r);
#endif
}


void GUI::drawText(const SDL_Color c, const char* txt, const SDL_Point pt)
{
#ifndef NO_SDL
  // Render at full display resolution, then shrink the destination rectangle
  // back into logical pixels. The renderer's SCREEN_SCALE upscale cancels the
  // SCREEN_SCALE we rendered at, so the text is drawn 1:1 and stays crisp.
  SDL_Surface* textSurface = TTF_RenderText_Blended(ttf, txt, c);
  if (textSurface == nullptr) {
    return;
  }
  SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
  SDL_Rect textRect {
    pt.x, pt.y,
    textSurface->w / SCREEN_SCALE,
    textSurface->h / SCREEN_SCALE,
  };
  SDL_FreeSurface(textSurface);
  if (textTexture != nullptr) {
    SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
    SDL_DestroyTexture(textTexture);
  }
#else
  fprintf(stdout, "%s\n", txt);
#endif
}


void GUI::centerText(const SDL_Color c, const char* txt, const int y)
{
  int w = 0;
#ifndef NO_SDL
  int h = 0;
  TTF_SizeText(ttf, txt, &w, &h);
  w /= SCREEN_SCALE;  // logical pixels, matching drawText
#endif
  drawText(c, txt, SDL_Point{(SCREEN_WIDTH - w) >> 1, y});
}


#ifndef NO_SDL
bool GUI::guiShouldRefresh()  { return dialogType == DialogType::INFOTIMER && SDL_GetTicks64() >= next_screen_tick; }
#else
// Console-only: timer screens advance automatically each iteration; everything
// else waits for input. (No real clock in the NO_SDL build.)
bool GUI::guiShouldRefresh()  { return dialogType == DialogType::INFOTIMER; }
#endif


void GUI::createGUI(GameScreen screen)
{
  // Entering a different screen invalidates any menu state built for the previous one.
  if (screen != this->currentMenuScreen) {
    this->currentMenu.clear();
    this->currentMenuScreen = screen;
  }

#ifdef NO_SDL
  fprintf(stdout, " [ %s ]\n", screenName(screen));
#endif

  // Default timer delay for INFOTIMER screens. Screens that want a different
  // delay overwrite next_screen_tick below.
  next_screen_tick = 200 + SDL_GetTicks64();

  switch (screen) {
    case GameScreen::Splash1:
      dialogType = DialogType::INFOTIMER;
      setBGColor(CAColor::GREEN);
      centerText(CAColor::WHITE, "Covert Action", 5);
      break;
    case GameScreen::Splash2:
      dialogType = DialogType::INFOTIMER;
      setBGColor(CAColor::BLUE);
      centerText(CAColor::WHITE, "Credits", 5);
      break;
    case GameScreen::BeginMenu:
      setBGColor(CAColor::BLACK);
#ifdef PLACEHOLDERS
      _imagePlaceholder(15, 5, 20, 15);
#endif
      drawBox(CAColor::WHITE, SDL_Rect{80, 72, 165, 84});
      drawText(CAColor::WHITE, "Do you want to...", SDL_Point{85, 78});
      screenMenu(screen, {" Create a New Character", " Load a Saved Game", " Practice a skill", " Review Hall of Fame"},
                 SDL_Rect{85, 94, 155, 1});
      break;
    case GameScreen::NewCharacter:
      dialogType = DialogType::TEXTENTRY;
      setBGColor(CAColor::RED);
      centerText(CAColor::WHITE, "Enter your name ...", 40);
      // TODO: real text entry
      break;
    case GameScreen::SkillUpgrade:
      dialogType = DialogType::SKILLSELECT;
      setBGColor(CAColor::DKBLUE);
      centerText(CAColor::WHITE, "Assign skill points", 20);
      centerText(CAColor::GREY, "(left/right to change, any other key to continue)", 20 + UI_LINE_HEIGHT);
      // TODO: skill list and point allocation
      break;
    case GameScreen::Load:
      dialogType = DialogType::INFO;
      setBGColor(CAColor::DKGREY);
      centerText(CAColor::WHITE, "Loading is not implemented yet", 40);
      centerText(CAColor::GREY, "Press any key to return to the menu", 40 + UI_LINE_HEIGHT);
      break;
    case GameScreen::SkillPractice:
      setBGColor(CAColor::BLACK);
      centerText(CAColor::WHITE, "Practice a skill", 30);
      screenMenu(screen, {" Combat", " Driving", " Cryptography", " Electronics"},
                 SDL_Rect{120, 50, 100, 1});
      break;
    case GameScreen::HallOfFame:
      dialogType = DialogType::INFO;
      setBGColor(CAColor::DKBLUE);
      centerText(CAColor::WHITE, "Review Hall of Fame", 40);
      centerText(CAColor::GREY, "Press any key to return to the menu", 40 + UI_LINE_HEIGHT);
      break;
    case GameScreen::Chief:
      setBGColor(CAColor::BLACK);
#ifdef PLACEHOLDERS
      _imagePlaceholder(15, 5, 20, 15);
#endif
      dialogType = DialogType::INFO;
      drawBox(CAColor::WHITE, SDL_Rect{1, 101, 318, 98});
      drawText(CAColor::WHITE, "Chief says ...", SDL_Point{4, 104});
      break;
    case GameScreen::MastermindLineup:
      dialogType = DialogType::MENU;
      setBGColor(CAColor::BLACK);
      drawText(CAColor::WHITE, "Max, I'm sure you recognize the head of", SDL_Point{24, 4});
      drawText(CAColor::WHITE, "this organization among these faces.", SDL_Point{24, 4 + UI_LINE_HEIGHT});
#ifdef PLACEHOLDERS
      _imagePlaceholder( 28,  32, 44, 44);
      _imagePlaceholder( 83,  32, 44, 44);
      _imagePlaceholder(138,  32, 44, 44);
      _imagePlaceholder(193,  32, 44, 44);
      _imagePlaceholder(248,  32, 44, 44);
      _imagePlaceholder( 28,  88, 44, 44);
      _imagePlaceholder( 83,  88, 44, 44);
      _imagePlaceholder(138,  88, 44, 44);
      _imagePlaceholder(193,  88, 44, 44);
      _imagePlaceholder(248,  88, 44, 44);
      _imagePlaceholder( 28, 144, 44, 44);
      _imagePlaceholder( 83, 144, 44, 44);
      _imagePlaceholder(138, 144, 44, 44);
      _imagePlaceholder(193, 144, 44, 44);
      _imagePlaceholder(248, 144, 44, 44);
#endif
      screenMenu(screen, {" Confirm"}, SDL_Rect{130, 186, 60, 1});
      break;
    case GameScreen::ShowNewClues:
      dialogType = DialogType::INFO;
      setBGColor(CAColor::DKBLUE);
      drawBox(CAColor::WHITE, SDL_Rect{10, 10, 300, 180});
      drawText(CAColor::WHITE, "New clues have arrived.", SDL_Point{14, 14});
      centerText(CAColor::GREY, "Press any key to continue", 176);
      break;
    case GameScreen::CIAContact:
      dialogType = DialogType::INFO;
      setBGColor(CAColor::DKGREEN);
      centerText(CAColor::WHITE, "CIA field office", 40);
      centerText(CAColor::GREY, "Press any key ...", 40 + UI_LINE_HEIGHT);
      break;
    case GameScreen::CIAMainMenu:
      setBGColor(CAColor::BLACK);
      centerText(CAColor::WHITE, "Field Office", 8);
      screenMenu(screen, {" Leave (return to city)", " Review Data", " Intelligence", " Cryptography"},
                 SDL_Rect{80, 30, 160, 1});
      break;
    case GameScreen::CIAData:
      setBGColor(CAColor::BLACK);
      centerText(CAColor::WHITE, "Review Data", 8);
      screenMenu(screen, {" Return to Main Menu", " Review Clues", " Review Suspects",
                          " Inside Information", " News Bulletins", " Organization Summary",
                          " City Summary", " Activity Reports"},
                 SDL_Rect{80, 30, 160, 1});
      break;
    case GameScreen::CIAIntel:
      setBGColor(CAColor::BLACK);
      centerText(CAColor::WHITE, "Intelligence", 8);
      screenMenu(screen, {" Return to Main Menu", " Local Scan", " International Scan",
                          " Active Wire Taps", " Accuse Double Agent", " Check with Sam"},
                 SDL_Rect{80, 30, 160, 1});
      break;
    case GameScreen::CIACrypto:
      setBGColor(CAColor::BLACK);
      centerText(CAColor::WHITE, "Cryptography", 8);
      screenMenu(screen, {" Return to Main Menu", " Coded Messages", " Crime Chronology"},
                 SDL_Rect{80, 30, 160, 1});
      break;
    case GameScreen::CodedMessages:
    case GameScreen::CrimeChronology:
      dialogType = DialogType::INFO;
      setBGColor(CAColor::DKBLUE);
      centerText(CAColor::WHITE, screenName(screen), 40);
      centerText(CAColor::GREY, "Press any key to go back", 40 + UI_LINE_HEIGHT);
      break;
    case GameScreen::CIASam:
      dialogType = DialogType::INFO;
      setBGColor(CAColor::DKBLUE);
      centerText(CAColor::WHITE, "Sam: nothing new to report.", 40);
      centerText(CAColor::GREY, "Press any key ...", 40 + UI_LINE_HEIGHT);
      break;
    case GameScreen::ReviewClues:
    case GameScreen::ReviewSuspects:
    case GameScreen::ReviewInsideInfo:
    case GameScreen::ReviewNews:
    case GameScreen::ReviewOrg:
    case GameScreen::ReviewCity:
    case GameScreen::ReviewActivity:
    case GameScreen::ReviewWiretaps:
      dialogType = DialogType::INFO;
      setBGColor(CAColor::DKBLUE);
      drawBox(CAColor::WHITE, SDL_Rect{10, 10, 300, 180});
      drawText(CAColor::WHITE, screenName(screen), SDL_Point{14, 14});
      centerText(CAColor::GREY, "Press any key to go back", 176);
      break;
    case GameScreen::CIAAddBan:
      dialogType = DialogType::INFO;
      setBGColor(CAColor::DKRED);
      centerText(CAColor::WHITE, "You are banned from the CIA office", 40);
      break;
    case GameScreen::CityMainMenu:
      setBGColor(CAColor::BLACK);
      centerText(CAColor::WHITE, "City", 8);
      screenMenu(screen, {" CIA Office", " Airport", " Hotel", " Building 1", " Quit"},
                 SDL_Rect{120, 30, 90, 1});
      break;
    case GameScreen::Airport:
      dialogType = DialogType::INFO;
      setBGColor(CAColor::DKCYAN);
      centerText(CAColor::WHITE, "Airport", 40);
      centerText(CAColor::GREY, "Travel is not implemented yet", 40 + UI_LINE_HEIGHT);
      break;
    case GameScreen::Hotel:
      setBGColor(CAColor::BLACK);
      centerText(CAColor::WHITE, "Hotel", 8);
      // TODO: double check the order against the original game.
      screenMenu(screen, {" Leave the hotel", " Sit in the lounge", " Check out", " Save / Load",
                          " Sleep through the case", " Start a new game", " More...", " Quit"},
                 SDL_Rect{80, 30, 160, 1});
      break;
    case GameScreen::HotelLoungeModal:
    case GameScreen::NewGameMaybe:
    case GameScreen::HotelSleepThruCase:
      dialogType = DialogType::INFO;
      setBGColor(CAColor::DKGREY);
      centerText(CAColor::WHITE, screenName(screen), 40);
      centerText(CAColor::GREY, "Press any key to go back", 40 + UI_LINE_HEIGHT);
      break;
    case GameScreen::LoadSave:
      dialogType = DialogType::INFO;
      setBGColor(CAColor::DKGREY);
      centerText(CAColor::WHITE, "Save / Load", 40);
      centerText(CAColor::GREY, "Press any key to go back", 40 + UI_LINE_HEIGHT);
      break;
    case GameScreen::Building1:
    case GameScreen::Building2:
    case GameScreen::Building3:
    case GameScreen::Building4:
    case GameScreen::Building5:
      setBGColor(CAColor::BLACK);
      centerText(CAColor::WHITE, screenName(screen), 8);
      screenMenu(screen, {" Place wiretap", " Break into building", " Watch the building",
                          " Check Data", " Leave"},
                 SDL_Rect{95, 30, 140, 1});
      break;
    case GameScreen::BuildingWatch:
      dialogType = DialogType::INFO;
      setBGColor(CAColor::DKGREEN);
      centerText(CAColor::WHITE, "Watching the building ...", 40);
      centerText(CAColor::GREY, "Press any key to stop", 40 + UI_LINE_HEIGHT);
      break;
    case GameScreen::Typewriter:
      dialogType = DialogType::INFO;
      setBGColor(CAColor::BLACK);
      centerText(CAColor::WHITE, "Typewriter", 40);
      centerText(CAColor::GREY, "Press any key ...", 40 + UI_LINE_HEIGHT);
      break;
    case GameScreen::CombatArrest:
    case GameScreen::CombatRecover:
    case GameScreen::CombatCaptive:
    case GameScreen::DrivingCarSelect:
    case GameScreen::DrivingArrest:
    case GameScreen::DrivingTargetGone:
    case GameScreen::DrivingShootout:
    case GameScreen::DrivingEscapedPursuer:
    case GameScreen::ElecModalBugged:
    case GameScreen::ElecModalTrace:
    case GameScreen::ElecModalAlarm:
      dialogType = DialogType::INFO;
      setBGColor(CAColor::DKBLUE);
      centerText(CAColor::WHITE, screenName(screen), 40);
      centerText(CAColor::GREY, "Press any key ...", 40 + UI_LINE_HEIGHT);
      break;
    case GameScreen::MiniGameCombatStart:
    case GameScreen::MiniGameDrivingStart:
    case GameScreen::MiniGameCryptoStart:
    case GameScreen::MiniGameElecStart:
      dialogType = DialogType::INFO;
      setBGColor(CAColor::DKRED);
      centerText(CAColor::WHITE, screenName(screen), 40);
      centerText(CAColor::GREY, "Minigames are not implemented yet", 40 + UI_LINE_HEIGHT);
      break;
    case GameScreen::GameExitModal:
      dialogType = DialogType::INFO;
      setBGColor(CAColor::BLACK);
      centerText(CAColor::WHITE, "Thanks for playing", 40);
      break;
    case GameScreen::GameExit:
      dialogType = DialogType::INFO;
      setBGColor(CAColor::BLACK);
      centerText(CAColor::WHITE, "Exiting ...", 40);
      break;
    case GameScreen::UNKNOWN:
    default:
      // Never terminate the process from a render path; show a placeholder instead.
      dialogType = DialogType::INFO;
      setBGColor(CAColor::DKRED);
      centerText(CAColor::WHITE, "Unhandled screen (placeholder)", 40);
      centerText(CAColor::GREY, "Press any key to return to the city", 40 + UI_LINE_HEIGHT);
      break;
  }
#ifndef NO_SDL
  SDL_RenderPresent(renderer);
#else
  fprintf(stdout, "-----\n");
#endif
}


// TODO: remove if unused
void GUI::createGUIMenu(std::vector<std::string> choice_strings, const SDL_Rect r0)
{
  SDL_Point pt { r0.x , r0.y };
  int i = 0;
  for (auto& choice : choice_strings) {
    if (i == 0) {
      fillBox(CAColor::YELLOW, SDL_Rect{pt.x, pt.y, r0.w, UI_LINE_HEIGHT});
    }
    drawText(CAColor::GREY, choice.c_str(), pt);
    pt.y += UI_LINE_HEIGHT;
    i++;
  }
}


// Build (or reuse) the menu for a screen and draw it.
// The menu is only rebuilt when the screen changes, so the highlighted item
// is preserved across redraws.
void GUI::screenMenu(GameScreen screen, std::vector<std::string> choice_strings, const SDL_Rect r0)
{
  dialogType = DialogType::MINIMENU;
  if (currentMenu.empty() || currentMenuScreen != screen) {
    currentMenu = buildMenu(std::move(choice_strings));
    currentMenuScreen = screen;
    selectedChoice = 0;
  }
  updateGUIMenu(currentMenu, r0);
}


void GUI::updateGUIMenu(const std::vector<MenuChoice> choices, SDL_Rect r0)
{
  SDL_Point pt { r0.x , r0.y };
  for (auto& choice : choices) {
    if (choice.selected) {
      fillBox(CAColor::YELLOW, SDL_Rect{pt.x, pt.y, r0.w, UI_LINE_HEIGHT});
    }
    drawText(CAColor::GREY, choice.txt.c_str(), pt);
    pt.y += UI_LINE_HEIGHT;
  }
}


void GUI::chooseMenuPrev()
{
  for (auto& choice : currentMenu) {
    if (choice.selected) {
      choice.selected = false;
      currentMenu[choice.prev].selected = true;
      return;
    }
  }
}


void GUI::chooseMenuNext()
{
  for (auto& choice : currentMenu) {
    if (choice.selected) {
      choice.selected = false;
      currentMenu[choice.next].selected = true;
      return;
    }
  }
}


void GUI::selectMenuState()
{
  for (auto& choice : currentMenu) {
    if (choice.selected) {
      selectedChoice = choice.id;
      return;
    }
  }
}


void GUI::selectMenuDefault()
{
  selectedChoice = 0;
}



#ifdef PLACEHOLDERS
#include <iostream>
const unsigned char singlepixel_bmp[] = {
  0x42, 0x4d, 0x8e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8a, 0x00,
  0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
  0x00, 0x00, 0x23, 0x2e, 0x00, 0x00, 0x23, 0x2e, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0xff,
  0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x47,
  0x52, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x98, 0xf8, 0x00
};

void GUI::_imagePlaceholder(const int& x, const int& y, const int& w, const int& h)
{
#ifndef NO_SDL
  // NOTE: must be drawn through the renderer. Calling SDL_GetWindowSurface()
  // here (as this used to) is invalid while a renderer is active, and it never
  // actually blitted the placeholder, so images silently never appeared.
  SDL_Rect pxRect { x, y, w, h };
  SDL_RWops* pxIO = SDL_RWFromConstMem(singlepixel_bmp, sizeof(singlepixel_bmp));
  SDL_Surface* pxSurf = SDL_LoadBMP_RW(pxIO, 1);
  SDL_Texture* pxTex = SDL_CreateTextureFromSurface(renderer, pxSurf);
  SDL_FreeSurface(pxSurf);
  SDL_RenderCopy(renderer, pxTex, nullptr, &pxRect);
  SDL_DestroyTexture(pxTex);
#else
  fprintf(stdout, "<img subst>\n");
#endif  // !NO_SDL
}
#endif  // PLACEHOLDERS
