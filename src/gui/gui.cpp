// TODO: Anything at all non-graphics related needs to be in a ui.h. GUI should only be the graphics components of UI. This would allow a console-only mode.
#ifdef NO_SDL
#include <cstdint>
uint64_t SDL_GetTicks64()  { return 0; }
#endif

#include "gui.h"

#include "../disk/loadsave.h"
#include "../gameplay/casegenerator.h"  // caseFiles: the loaded case templates
#include "facegen.h"

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

namespace {
// Screen names indexed by GameScreen (declaration order in types.h).
// Used for the console (NO_SDL) output and for placeholder labels.
const char* const kScreenNames[] = {
    "Splash1", "Splash2", "BeginMenu", "NewCharacter", "Load", "SkillPractice",
    "HallOfFame", "SkillUpgrade", "Chief", "MastermindLineup", "ShowNewClues",
    "CIAContact", "CIAMainMenu", "CIAData", "Data", "ReviewClues", "ReviewSuspects",
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
  if (SDL_CreateWindowAndRenderer(SCREEN_SCALE * SCREEN_WIDTH, SCREEN_SCALE * SCREEN_HEIGHT,
                                  SDL_WINDOW_RESIZABLE, &window, &renderer) != 0) {
    fprintf(stderr, "Could not create window/renderer: %s\n", SDL_GetError());
    exit(EXIT_FAILURE);
  }
  SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

  if (TTF_Init() != 0) {  // TODO: move to Font class
    fprintf(stderr, "Could not initialise SDL_ttf: %s\n", TTF_GetError());
    exit(EXIT_FAILURE);
  }

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
#endif
}


GUI::~GUI()
{
#ifndef NO_SDL
  for (auto& entry : faceCache) {
    if (entry.second != nullptr) SDL_DestroyTexture(entry.second);
  }
  faceCache.clear();

  if (ttf != nullptr) {
    TTF_CloseFont(ttf);
    ttf = nullptr;
  }
  TTF_Quit();

  if (renderer != nullptr) {
    SDL_DestroyRenderer(renderer);
    renderer = nullptr;
  }
  if (window != nullptr) {
    SDL_DestroyWindow(window);
    window = nullptr;
  }
#endif
}


void GUI::setBGColor(const SDL_Color c)
{
#ifndef NO_SDL
  SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);  // TODO: use a colorlist element
  SDL_RenderClear(renderer);
#else
  (void)c;
#endif
}


void GUI::fillBox(const SDL_Color c, const SDL_Rect r)
{
#ifndef NO_SDL
  SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, SDL_ALPHA_OPAQUE);
  SDL_RenderFillRect(renderer, &r);
#else
  (void)c;
  (void)r;
#endif
}


void GUI::drawBox(const SDL_Color c, const SDL_Rect r)
{
#ifndef NO_SDL
  SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, SDL_ALPHA_OPAQUE);
  SDL_RenderDrawRect(renderer, &r);
#else
  (void)c;
  (void)r;
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
  (void)c;
  (void)pt;
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


void GUI::drawMiniGame()  {}  // overridden by GameState to draw the active minigame


#ifndef NO_SDL
SDL_Texture* GUI::_faceTexture(const Face& face)
{
  // Pack the six components into a cache key.
  const uint64_t key = static_cast<uint64_t>(face.gender_id)
      | (static_cast<uint64_t>(face.hair_id) << 8)
      | (static_cast<uint64_t>(face.eye_id) << 16)
      | (static_cast<uint64_t>(face.skin_id) << 24)
      | (static_cast<uint64_t>(face.clothing_id) << 32)
      | (static_cast<uint64_t>(face.accessory_id) << 40);

  auto it = faceCache.find(key);
  if (it != faceCache.end()) return it->second;

  const facegen::Image img = facegen::compose(face);
  const std::vector<facegen::Color>& pal = facegen::palette();

  SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(
      0, img.width, img.height, 32, SDL_PIXELFORMAT_RGBA32);
  if (surf == nullptr) return nullptr;

  SDL_LockSurface(surf);
  const SDL_PixelFormat* fmt = surf->format;
  auto* dst = static_cast<uint32_t*>(surf->pixels);
  const int row_px = surf->pitch / static_cast<int>(sizeof(uint32_t));
  for (int y = 0; y < img.height; ++y) {
    for (int x = 0; x < img.width; ++x) {
      const facegen::Color c = pal[img.indices[static_cast<std::size_t>(y) * img.width + x]];
      dst[y * row_px + x] = SDL_MapRGBA(fmt, c.r, c.g, c.b, c.a);
    }
  }
  SDL_UnlockSurface(surf);

  SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
  SDL_FreeSurface(surf);
  if (tex != nullptr) SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

  faceCache[key] = tex;
  return tex;
}
#endif


void GUI::drawFace(const Face& face, const SDL_Rect& r)
{
#ifndef NO_SDL
  SDL_Texture* tex = _faceTexture(face);
  if (tex == nullptr) return;
  SDL_RenderCopy(renderer, tex, nullptr, &r);
#else
  (void)face;
  (void)r;
  fprintf(stdout, "<face subst>\n");
#endif
}


Face GUI::lineupFace(int slot)
{
  // The lineup holds this many suspects (a 5x3 grid, as in the original).
  constexpr int kLineupSlots = 15;

  // There is no "active case" selector yet, so the first loaded template stands
  // in for the case in play. Empty caseFiles (e.g. the data files are absent)
  // simply leaves the procedural lineup, which is the previous behaviour.
  const CaseFile* active = caseFiles.empty() ? nullptr : &caseFiles.front();
  const int count = active == nullptr ? 0 : static_cast<int>(active->participants.size());

  // Build once per case (and rebuild if the participant count changes).
  if (lineupSourceCount != count || lineupFaces.size() != static_cast<std::size_t>(kLineupSlots)) {
    lineupSourceCount = count;
    lineupFaces.clear();
    lineupFaces.reserve(kLineupSlots);
    for (int i = 0; i < kLineupSlots; ++i) {
      if (active != nullptr && i < count) {
        lineupFaces.push_back(facegen::makeFaceFromParticipant(active->participants[static_cast<std::size_t>(i)]));
      } else {
        // Fewer participants than slots (or no case at all): fill the rest with
        // stable procedural faces.
        lineupFaces.push_back(facegen::candidateFace(i));
      }
    }
  }

  if (slot < 0) slot = 0;
  if (slot >= kLineupSlots) slot = kLineupSlots - 1;
  return lineupFaces[static_cast<std::size_t>(slot)];
}


void GUI::createGUI(GameScreen screen)
{
  // Entering a different screen invalidates any menu state built for the previous one.
  const bool screen_changed = (screen != this->currentMenuScreen);
  if (screen_changed) {
    this->currentMenu.clear();
    this->currentMenuScreen = screen;
  }

#ifdef NO_SDL
  fprintf(stdout, " [ %s ]\n", screenName(screen));
#endif

  // Arm the auto-advance timer for INFOTIMER screens. This must not restart a
  // pending timer when we are only repainting the current screen (e.g. after a
  // window resize/expose), otherwise a splash would linger indefinitely. It is
  // re-armed once the timer has already fired so a self-transitioning timer
  // screen cannot spin the main loop. Screens wanting a different delay
  // overwrite next_screen_tick below.
  const uint64_t now = SDL_GetTicks64();
  if (screen_changed || next_screen_tick <= now) {
    next_screen_tick = 200 + now;
  }

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
      drawFace(facegen::playerFace(), SDL_Rect{12, 4, 34, 40});
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
      setBGColor(CAColor::BLACK);
      centerText(CAColor::WHITE, "Load a Saved Game", 20);
      // show_game_files() returns the callsign (or " - Empty -") for each slot;
      // the choice index is the slot number.
      screenMenu(screen, show_game_files(), SDL_Rect{100, 50, 120, 1});
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
      drawFace(facegen::chiefFace(), SDL_Rect{10, 6, 40, 46});
      dialogType = DialogType::INFO;
      drawBox(CAColor::WHITE, SDL_Rect{1, 101, 318, 98});
      drawText(CAColor::WHITE, "Chief says ...", SDL_Point{4, 104});
      break;
    case GameScreen::MastermindLineup:
      dialogType = DialogType::MENU;
      setBGColor(CAColor::BLACK);
      drawText(CAColor::WHITE, "Max, I'm sure you recognize the head of", SDL_Point{24, 4});
      drawText(CAColor::WHITE, "this organization among these faces.", SDL_Point{24, 4 + UI_LINE_HEIGHT});
      {
        // 15 candidates in a 5x3 grid, as in the original lineup.
        const int col_x[5] = {28, 83, 138, 193, 248};
        const int row_y[3] = {32, 88, 144};
        for (int row = 0; row < 3; ++row) {
          for (int col = 0; col < 5; ++col) {
            drawFace(lineupFace(row * 5 + col),
                     SDL_Rect{col_x[col], row_y[row], 44, 44});
          }
        }
      }
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
    case GameScreen::Data:
      // The building "Check Data" screen: the same review material, but it
      // belongs to the current building/city rather than the CIA office.
      setBGColor(CAColor::BLACK);
      centerText(CAColor::WHITE, "Check Data", 8);
      screenMenu(screen, {" Return to city", " Review Clues", " Review Suspects",
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
    case GameScreen::ReviewSuspects:
      dialogType = DialogType::INFO;
      setBGColor(CAColor::DKBLUE);
      drawBox(CAColor::WHITE, SDL_Rect{10, 10, 300, 180});
      drawText(CAColor::WHITE, screenName(screen), SDL_Point{14, 14});
      drawFace(facegen::candidateFace(7), SDL_Rect{24, 40, 64, 64});
      drawText(CAColor::WHITE, "Suspect", SDL_Point{104, 40});
      centerText(CAColor::GREY, "Press any key to go back", 176);
      break;
    case GameScreen::ReviewClues:
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
      setBGColor(CAColor::BLACK);
      centerText(CAColor::WHITE, "Save Game", 20);
      // Selecting a slot saves into it; the choice index is the slot number.
      screenMenu(screen, show_game_files(), SDL_Rect{100, 50, 120, 1});
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
      dialogType = DialogType::INFO;
      setBGColor(CAColor::DKRED);
      centerText(CAColor::WHITE, screenName(screen), 40);
      centerText(CAColor::GREY, "Minigames are not implemented yet", 40 + UI_LINE_HEIGHT);
      break;
    case GameScreen::MiniGameCryptoStart:
    case GameScreen::MiniGameElecStart:
      // The crypto and electronics minigames are implemented; they draw
      // themselves and wait for the key handler (see DialogType::MINIGAME in
      // covert.cpp).
      dialogType = DialogType::MINIGAME;
      setBGColor(CAColor::BLACK);
      drawMiniGame();
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
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  fprintf(stdout, "<img subst>\n");
#endif  // !NO_SDL
}
#endif  // PLACEHOLDERS
