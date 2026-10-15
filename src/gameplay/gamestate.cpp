#include "gamestate.h"

#include <cstdio>



void GameState::initMiniGameClasses()
{
  mGameElec = std::make_unique<MiniGameElectronics>();
  mGameCrypto = std::make_unique<MiniGameCryptography>();
}


/*! \mainpage
 *
 *  Screen State Transition Chart
 *  \dot
 *  digraph GameScreenFSM {
 *    node [shape = ellipse];
 *
 *    Splash1;
 *    Splash2;
 *    BeginMenu;
 *    NewCharacter;
 *    Load;
 *    SkillPractice;
 *    HallOfFame;
 *    SkillUpgrade;
 *    Chief;
 *    MastermindLineup;
 *    ShowNewClues;
 *    CIAContact;
 *    CIAMainMenu;
 *    CIAData;
 *    Data;
 *    ReviewClues;
 *    ReviewSuspects;
 *    ReviewInsideInfo;
 *    ReviewNews;
 *    ReviewOrg;
 *    ReviewCity;
 *    ReviewActivity;
 *    CIAIntel;
 *    CIABanned;
 *    ReviewWiretaps;
 *    CIASam;
 *    CIACrypto;
 *    CodedMessages;
 *    CrimeChronology;
 *    CityMainMenu;
 *    Airport;
 *    Hotel;
 *    LoadSave;
 *    BuildingExterior;
 *    BuildingWatch;
 *    Typewriter;
 *    MiniGameCombatStart;
 *    CombatArrest;
 *    CombatRecover;
 *    CombatCaptive;
 *    MiniGameDrivingStart;
 *    DrivingCarSelect;
 *    MiniGameCryptoStart;
 *    MiniGameElecStart;
 *    GameExit;
 *
 *    node [shape = box3d];
 *
 *    Combat
 *    Driving
 *    Cryptography
 *    Electronics
 *
 *    Splash1 -> Splash2 -> BeginMenu -> {NewCharacter, Load, SkillPractice, HallOfFame};
 *    HallOfFame -> BeginMenu;
 *    SkillPractice -> {MiniGameCombatStart, MiniGameDrivingStart, MiniGameCryptoStart, MiniGameElecStart};
 *
 *    BuildingExterior -> {MiniGameElecStart, MiniGameCombatStart, BuildingWatch, Data, CityMainMenu};
 *    BuildingWatch -> {BuildingWatch, MiniGameDrivingStart, MiniGameElecStart, Data};
 *
 *    MiniGameCombatStart -> Combat;
 *    Combat -> {CombatArrest, CombatRecover, CombatCaptive, CityMainMenu, BeginMenu};
 *    MiniGameDrivingStart -> DrivingCarSelect -> Driving;
 *    MiniGameCryptoStart -> Cryptography;
 *    MiniGameElecStart -> Electronics;
 *
 *    NewCharacter -> SkillUpgrade -> Chief;
 *    Chief -> MastermindLineup;
 *    MastermindLineup -> Chief;
 *    Chief -> ShowNewClues -> CIAContact -> {CIASam, CIAMainMenu};
 *
 *    CIAMainMenu -> {CIAData, CIAIntel, CIACrypto};
 *    CIAData -> {ReviewClues, ReviewSuspects, ReviewInsideInfo, ReviewNews, ReviewOrg, ReviewCity, ReviewActivity};
 *    Data -> {ReviewClues, ReviewSuspects, ReviewInsideInfo, ReviewNews, ReviewOrg, ReviewCity, ReviewActivity};
 *
 *    CityMainMenu -> {Airport, Hotel, CIAContact, CIABanned, BuildingObserve};
 *    Hotel -> {CityMenu, Load, GameExit}
 *  }
 *  \enddot
 */


#include <iostream>  // TODO-debug
void GameState::getNextScreen(ScreenExitCondition condition)
{
  // TODO: possibly clear `origScreen` and set an overrideable default

  if (this->inMiniGame) {
    switch (this->currentMiniGame) {
      case MiniGameType::Inactive:
        // No minigame running; nothing to transition from.
        break;
      case MiniGameType::Combat:
        if (condition == ScreenExitCondition::None)  { currentScreen = GameScreen::CityMainMenu; }
        if (condition == ScreenExitCondition::Arrest)  { currentScreen = GameScreen::CombatArrest; }
        if (condition == ScreenExitCondition::Shot)  { currentScreen = GameScreen::CombatRecover; }
        if (condition == ScreenExitCondition::Lose)  { currentScreen = GameScreen::CombatCaptive; }
        break;
      case MiniGameType::Driving:
        /// TODO: what happens when pressing <Esc>?
        /// if (condition == ScreenExitCondition::None)  { currentScreen = GameScreen::CityMainMenu; }
        if (condition == ScreenExitCondition::Arrest)  { currentScreen = GameScreen::DrivingArrest; }
        if (condition == ScreenExitCondition::Lose)  { currentScreen = GameScreen::DrivingTargetGone; }
        if (condition == ScreenExitCondition::Intercepted)  { currentScreen = GameScreen::DrivingShootout; }
        if (condition == ScreenExitCondition::Win)  { currentScreen = GameScreen::DrivingEscapedPursuer; }
        /// TODO: differentiate between 'hunter' and 'hunted' outcome
        break;
      case MiniGameType::Crypto:
        currentScreen = goBackFrom(origScreen);
        /* TODO: back to referrer. Are all clues handled by clue routine? */
        break;
      case MiniGameType::Elec:
        /// TODO: differentiate between 'trace' and 'phone bug'
        if (condition == ScreenExitCondition::None)  { currentScreen = GameScreen::CityMainMenu; }
        if (condition == ScreenExitCondition::Timeout)  { currentScreen = GameScreen::CityMainMenu; }
        if (condition == ScreenExitCondition::PhoneBugged)  { currentScreen = GameScreen::ElecModalBugged; }
        if (condition == ScreenExitCondition::CarTraced)  { currentScreen = GameScreen::ElecModalTrace; }
        if (condition == ScreenExitCondition::Lose)  { currentScreen = GameScreen::ElecModalAlarm; }
        break;
    }
  } else {  // this->inMiniGame is false
    switch (this->currentScreen) {
      case GameScreen::Splash1:
        currentScreen = GameScreen::Splash2;
        break;
      case GameScreen::Splash2:
        currentScreen = GameScreen::BeginMenu;
        break;
      case GameScreen::BeginMenu:
        std::cout << "Leaving menu, choice " << selectedChoice << " active." << std::endl;
        if (this->selectedChoice == 0)  { currentScreen = GameScreen::NewCharacter; }
        if (this->selectedChoice == 1)  { currentScreen = GameScreen::Load; }
        if (this->selectedChoice == 2)  { currentScreen = GameScreen::SkillPractice; }
        if (this->selectedChoice == 3)  { currentScreen = GameScreen::HallOfFame; }
        break;
      case GameScreen::Load:
        // TODO: save/load not implemented; return to the main menu instead of dying.
        currentScreen = GameScreen::BeginMenu;
        break;
      case GameScreen::SkillPractice:  // TODO
        (this->modalSequence)++;  // TODO: this double-increments currently (FIXME) using ENTER but not ESC
        // TODO: set origScreen to BeginMenu
        if (this->modalSequence == 3) {
          if (this->selectedChoice == 0)  { currentScreen = GameScreen::MiniGameCombatStart; }
          if (this->selectedChoice == 1)  { currentScreen = GameScreen::MiniGameDrivingStart; }
          if (this->selectedChoice == 2)  { enterMiniGame(MiniGameType::Crypto); }
          if (this->selectedChoice == 3)  { enterMiniGame(MiniGameType::Elec); }
        }
        // TODO: default?
        break;
      case GameScreen::HallOfFame:  // TODO
        currentScreen = GameScreen::BeginMenu;
        break;
      case GameScreen::NewCharacter:
        (this->modalSequence)++;
        if (this->modalSequence > 3)  { currentScreen = GameScreen::SkillUpgrade; }
        break;
      case GameScreen::SkillUpgrade:
        (this->modalSequence)++;
        if (this->modalSequence > 4) {
          this->origScreen = OriginScreen::ChiefCaseIntro;
          currentScreen = GameScreen::Chief;
        }
        break;
      case GameScreen::Chief:
        if (this->origScreen == OriginScreen::ChiefCaseIntro)  { currentScreen = GameScreen::MastermindLineup; }
        if (this->origScreen == OriginScreen::ChiefInfoHandout) {
          this->isTwelve = true;
          currentScreen = GameScreen::ShowNewClues;
        }
        if (this->origScreen == OriginScreen::ChiefKickMaxOut)  { currentScreen = GameScreen::CIAContact; }
        break;
      case GameScreen::MastermindLineup:
        this->origScreen = OriginScreen::ChiefInfoHandout;
        currentScreen = GameScreen::Chief;
        break;
      case GameScreen::ShowNewClues:
        // TODO: when clues and updates (messages, etc.) are finished, set isTwelve to false
        if (this->origScreen == OriginScreen::ChiefInfoHandout) {
          this->origScreen = OriginScreen::ChiefKickMaxOut;
          currentScreen = GameScreen::Chief;
        }
        if (this->origScreen == OriginScreen::CIA) {
          currentScreen = GameScreen::CIAIntel; /* TODO: make sure new clues at 12 during crypto do not jump here! */
        }
        break;
      case GameScreen::CIAContact:
        this->origScreen = OriginScreen::CIA;
        currentScreen = GameScreen::CIASam;
        break;
      case GameScreen::CIAMainMenu:
        if (this->selectedChoice == 0)  { this->origScreen = OriginScreen::CityMenu; currentScreen = GameScreen::CityMainMenu; }
        if (this->selectedChoice == 1)  { currentScreen = GameScreen::CIAData; }
        if (this->selectedChoice == 2)  { currentScreen = GameScreen::CIAIntel; }
        if (this->selectedChoice == 3)  { currentScreen = GameScreen::CIACrypto; }
        break;
      case GameScreen::CIAData:
        if (this->selectedChoice == 0)  { currentScreen = GameScreen::CIAMainMenu; }
        if (this->selectedChoice == 1)  { currentScreen = GameScreen::ReviewClues; }
        if (this->selectedChoice == 2)  { currentScreen = GameScreen::ReviewSuspects; }
        if (this->selectedChoice == 3)  { currentScreen = GameScreen::ReviewInsideInfo; }
        if (this->selectedChoice == 4)  { currentScreen = GameScreen::ReviewNews; }
        if (this->selectedChoice == 5)  { currentScreen = GameScreen::ReviewOrg; }
        if (this->selectedChoice == 6)  { currentScreen = GameScreen::ReviewCity; }
        if (this->selectedChoice == 7)  { currentScreen = GameScreen::ReviewActivity; }
        break;
      case GameScreen::CIAIntel:
        if (this->selectedChoice == 0)  { currentScreen = GameScreen::CIAMainMenu; }
        if (this->selectedChoice == 1 ||
            this->selectedChoice == 2)  { currentScreen = GameScreen::ShowNewClues; }
        if (this->selectedChoice == 3)  { currentScreen = GameScreen::ReviewWiretaps; }
        if (this->selectedChoice == 4) {
          // TODO: check if current office has double agent
          if (0) {
            // TODO: DA found!
          } else {
            currentScreen = GameScreen::CIAAddBan;
          }
        }
        if (this->selectedChoice == 5)  { currentScreen = GameScreen::CIASam; }
        break;
      case GameScreen::CIASam:
        currentScreen = GameScreen::CIAMainMenu;
        break;
      case GameScreen::CIACrypto:
        if (this->selectedChoice == 0)  { currentScreen = GameScreen::CIAMainMenu; }
        if (this->selectedChoice == 1)  { currentScreen = GameScreen::CodedMessages; }
        if (this->selectedChoice == 2)  { currentScreen = GameScreen::CrimeChronology; }
        // TODO
        break;
      case GameScreen::CodedMessages:
        currentScreen = GameScreen::CIACrypto;
        break;
      case GameScreen::CrimeChronology:
        currentScreen = GameScreen::CIACrypto;
        break;
      case GameScreen::CIAAddBan:
        currentScreen = GameScreen::CityMainMenu;
        break;
      case GameScreen::CityMainMenu:
        if (this->selectedChoice == 0)  { currentScreen = GameScreen::CIAContact; }
        if (this->selectedChoice == 1)  { currentScreen = GameScreen::Airport; }
        if (this->selectedChoice == 2)  { currentScreen = GameScreen::Hotel; }
        if (this->selectedChoice == 3)  { currentScreen = GameScreen::Building1; }
        if (this->selectedChoice == 4)  { currentScreen = GameScreen::GameExitModal; }
        break;
      case GameScreen::Airport:
        /* TODO: show animation */
        /* TODO: change current city */
        this->origScreen = OriginScreen::CityMenu;
        currentScreen = GameScreen::CityMainMenu;
        break;
      case GameScreen::Hotel:
        // TODO: double check order
        // TODO: double check <Esc> case
        if (this->selectedChoice == 0)  { currentScreen = GameScreen::CityMainMenu; }
        if (this->selectedChoice == 1)  { currentScreen = GameScreen::HotelLoungeModal; }
        if (this->selectedChoice == 2)  { currentScreen = GameScreen::CityMainMenu; }
        if (this->selectedChoice == 3)  { currentScreen = GameScreen::LoadSave; }
        if (this->selectedChoice == 4)  { currentScreen = GameScreen::HotelSleepThruCase; }
        if (this->selectedChoice == 5)  { currentScreen = GameScreen::NewGameMaybe; }
        if (this->selectedChoice == 6)  { currentScreen = GameScreen::Hotel; }
        if (this->selectedChoice == 7)  { currentScreen = GameScreen::GameExitModal; }
        break;
      case GameScreen::LoadSave:
        // TODO: save/load not implemented yet; return to the hotel.
        currentScreen = GameScreen::Hotel;
        break;
      case GameScreen::ReviewClues:
      case GameScreen::ReviewSuspects:
      case GameScreen::ReviewInsideInfo:
      case GameScreen::ReviewNews:
      case GameScreen::ReviewOrg:
      case GameScreen::ReviewCity:
      case GameScreen::ReviewActivity:
        currentScreen = goBackFrom(origScreen);
        break;
      case GameScreen::Building1:
      case GameScreen::Building2:
      case GameScreen::Building3:
      case GameScreen::Building4:
      case GameScreen::Building5:
        // Menu order (see gui.cpp): Place wiretap, Break in, Watch, Check Data, Leave.
        // Entering the minigame here (rather than from its own screen case) means it
        // is initialised before the screen is first drawn.
        if (this->selectedChoice == 0)  { enterMiniGame(MiniGameType::Elec); }
        if (this->selectedChoice == 1)  { currentScreen = GameScreen::MiniGameCombatStart; }
        if (this->selectedChoice == 2)  { currentScreen = GameScreen::BuildingWatch; }
        // TODO: "Check Data" has no implemented screen yet; fall back to watching.
        if (this->selectedChoice == 3)  { currentScreen = GameScreen::BuildingWatch; }
        if (this->selectedChoice == 4)  { currentScreen = GameScreen::CityMainMenu; }
        break;
      case GameScreen::BuildingWatch:
        currentScreen = GameScreen::CityMainMenu;
        break;
      case GameScreen::CombatArrest:
        currentScreen = GameScreen::CityMainMenu;
        break;
      case GameScreen::CombatRecover:
        currentScreen = GameScreen::CityMainMenu;
        break;
      case GameScreen::CombatCaptive:
        currentScreen = GameScreen::CityMainMenu;
        break;
      case GameScreen::HotelLoungeModal:
      case GameScreen::HotelSleepThruCase:
      case GameScreen::NewGameMaybe:
        currentScreen = GameScreen::Hotel;
        break;
      case GameScreen::DrivingCarSelect:
      case GameScreen::DrivingArrest:
      case GameScreen::DrivingTargetGone:
      case GameScreen::DrivingShootout:
      case GameScreen::DrivingEscapedPursuer:
        currentScreen = GameScreen::CityMainMenu;
        break;
      case GameScreen::ElecModalBugged:
      case GameScreen::ElecModalTrace:
      case GameScreen::ElecModalAlarm:
        currentScreen = GameScreen::CityMainMenu;
        break;
      case GameScreen::Typewriter:
        currentScreen = GameScreen::CityMainMenu;
        break;
      case GameScreen::GameExitModal:
        currentScreen = GameScreen::GameExit;
        break;
      case GameScreen::GameExit:
        this->shouldQuit = true;
        break;
      case GameScreen::ReviewWiretaps:
        currentScreen = GameScreen::CIAIntel;
        break;
      case GameScreen::MiniGameCombatStart:
      case GameScreen::MiniGameDrivingStart:
        // TODO: actually run the minigame. For now return to skill practice.
        currentScreen = GameScreen::SkillPractice;
        this->modalSequence = 0;
        break;
      case GameScreen::MiniGameCryptoStart:
        // Normally started by SkillPractice; this is a fallback for reaching the
        // screen directly. Starting it sets inMiniGame, so later getNextScreen()
        // calls take the minigame branch above.
        enterMiniGame(MiniGameType::Crypto);
        break;
      case GameScreen::MiniGameElecStart:
        // Normally the minigame is started by the screen that links here (see the
        // Building case); this is a fallback for reaching the screen directly.
        // Starting it sets inMiniGame, so later getNextScreen() calls take the
        // minigame branch above.
        enterMiniGame(MiniGameType::Elec);
        break;
      case GameScreen::UNKNOWN:  // TODO
      default:
        currentScreen = GameScreen::GameExit;  // TODO
        break;
    }
  }
}


GameScreen GameState::goBackFrom(OriginScreen os)
{
  // Returns the screen that spawned the current modal/review screen.
  switch (os) {
    case OriginScreen::ChiefCaseIntro:
    case OriginScreen::ChiefInfoHandout:
    case OriginScreen::ChiefKickMaxOut:
      return GameScreen::Chief;
    case OriginScreen::CIA:
      return GameScreen::CIAMainMenu;
    case OriginScreen::CityMenu:
      return GameScreen::CityMainMenu;
    case OriginScreen::SkillPractice:
      return GameScreen::SkillPractice;
    case OriginScreen::UNKNOWNORIGIN:
    default:
      return GameScreen::CityMainMenu;
  }
}


void GameState::enterMiniGame(MiniGameType miniGame)
{
  if (!mGameElec || !mGameCrypto)  { initMiniGameClasses(); }

  switch (miniGame) {
    case MiniGameType::Elec:
      mGameElec->start(this->gameDifficulty);
      this->currentMiniGame = miniGame;
      this->inMiniGame = true;
      this->currentScreen = GameScreen::MiniGameElecStart;
      break;
    case MiniGameType::Crypto:
      mGameCrypto->start(this->gameDifficulty);
      this->currentMiniGame = miniGame;
      this->inMiniGame = true;
      this->currentScreen = GameScreen::MiniGameCryptoStart;
      this->origScreen = OriginScreen::SkillPractice;  // crypto is a practice minigame
      break;
    case MiniGameType::Combat:
    case MiniGameType::Driving:
    case MiniGameType::Inactive:
    default:
      // TODO: these minigames are not implemented; leave the caller where it was
      // rather than dropping into an unplayable state.
      break;
  }
}


void GameState::miniGameMove(int dx, int dy)
{
  if (!inMiniGame)  { return; }
  switch (currentMiniGame) {
    case MiniGameType::Elec:
      if (mGameElec)  { mGameElec->moveCursor(dx, dy); }
      break;
    case MiniGameType::Crypto:
      if (mGameCrypto)  { mGameCrypto->moveCursor(dx, dy); }
      break;
    default:
      break;
  }
}


void GameState::miniGameRotate()
{
  if (!inMiniGame)  { return; }
  switch (currentMiniGame) {
    case MiniGameType::Elec: {
      if (!mGameElec)  { return; }
      mGameElec->rotateCursor();
      // Rotating can finish the puzzle (all targets linked) or exhaust the turn
      // budget (the trace completes). Anything else keeps the board open.
      switch (mGameElec->outcome()) {
        case ElecOutcome::Success:  finishMiniGame(ScreenExitCondition::PhoneBugged);  break;
        case ElecOutcome::Alarm:    finishMiniGame(ScreenExitCondition::Lose);         break;
        case ElecOutcome::InProgress:  break;
      }
      break;
    }
    case MiniGameType::Crypto: {
      if (!mGameCrypto)  { return; }
      mGameCrypto->cycleLetter();
      if (mGameCrypto->isSolved())  { finishMiniGame(ScreenExitCondition::Win); }
      break;
    }
    default:
      break;
  }
}


void GameState::finishMiniGame(ScreenExitCondition condition)
{
  if (!inMiniGame)  { return; }
  // Ask the FSM for the exit screen first (it reads inMiniGame), then leave the
  // minigame state. The caller redraws the resulting screen.
  getNextScreen(condition);
  inMiniGame = false;
  currentMiniGame = MiniGameType::Inactive;
}


void GameState::miniGameCancel()
{
  finishMiniGame(ScreenExitCondition::None);
}


namespace {
// One character per junction type, used to draw the board.
const char* junctionGlyph(Junction j)
{
  switch (j) {
    case STRAIGHT:            return "-";
    case CROSS:               return "+";
    case TOP_TO_ALL:          return "T";
    case BOT_TO_ALL:          return "b";
    case ALL_TO_TOP:          return "t";
    case ALL_TO_BOT:          return "B";
    case STRAIGHT_INV_TOP:    return "=";
    case STRAIGHT_INV_BOT:    return "_";
    case STRAIGHT_INV_ALL:    return "|";
    case CROSS_INV_TOP_LEFT:  return "x";
    case CROSS_INV_BOT_LEFT:  return "o";
    case CROSS_INV_ALL:       return "*";
  }
  return "?";
}
}  // namespace


void GameState::drawMiniGame()
{
  if (currentMiniGame == MiniGameType::Crypto && mGameCrypto) {
    centerText(CAColor::WHITE, "Cryptography - decode the message", 4);

    // The intercepted message.
    drawBox(CAColor::GREY, SDL_Rect{ 8, 18, 304, 58 });
    const std::string& ct = mGameCrypto->cipherMessage();
    const int wrapChars = 44;
    for (int i = 0, line = 0; i < (int)ct.size(); i += wrapChars, ++line) {
      drawText(CAColor::GREEN, ct.substr((std::size_t)i, (std::size_t)wrapChars).c_str(),
               SDL_Point{ 12, 22 + line * UI_LINE_HEIGHT });
    }

    // The cipher alphabet, laid out as two rows of thirteen. Each cell shows the
    // cipher letter above the player's guessed plaintext letter.
    const int cellW = 24;
    const int cellH = 30;
    const int gridX = 4;
    const int gridY = 88;
    for (int i = 0; i < 26; ++i) {
      const int col = i % 13;
      const int row = i / 13;
      const SDL_Rect cell{ gridX + col * cellW, gridY + row * cellH, cellW - 2, cellH - 6 };
      const bool onCursor = (i == mGameCrypto->cursor());
      fillBox(onCursor ? CAColor::YELLOW : CAColor::BLACK, cell);
      drawBox(CAColor::DKGREY, cell);

      const std::string top(1, (char)('A' + i));
      const std::string bot(1, mGameCrypto->userLetter(i));
      drawText(onCursor ? CAColor::BLACK : CAColor::WHITE, top.c_str(), SDL_Point{ cell.x + 7, cell.y + 1 });
      drawText(onCursor ? CAColor::BLACK : CAColor::GREEN, bot.c_str(), SDL_Point{ cell.x + 7, cell.y + 13 });
    }

    char status[96];
    std::snprintf(status, sizeof(status), "Cipher %c = %c  (%u in text)   Correct %d/26",
                  (char)('A' + mGameCrypto->cursor()),
                  mGameCrypto->userLetter(mGameCrypto->cursor()),
                  mGameCrypto->frequency(mGameCrypto->cursor()),
                  mGameCrypto->correctCount());
    centerText(CAColor::GREY, status, 164);
    centerText(CAColor::GREY, "Arrows pick  ENTER change  ESC leave", 180);
    return;
  }

  if (currentMiniGame != MiniGameType::Elec || !mGameElec) {
    centerText(CAColor::WHITE, "Minigame not implemented", 90);
    return;
  }

  // Board layout, in logical (320x200) pixels. Keep in step with the geometry
  // exposed by MiniGameElectronics (kRows/kCols).
  const int cellW = 26;
  const int cellH = 22;
  const int gridX = 44;
  const int gridY = 40;
  const int cx = mGameElec->cursorX();
  const int cy = mGameElec->cursorY();

  centerText(CAColor::WHITE, "Electronics - place the wiretap", 4);

  for (int row = 0; row < MiniGameElectronics::kRows; ++row) {
    for (int col = 0; col < MiniGameElectronics::kCols; ++col) {
      const SDL_Rect cell{ gridX + col * cellW, gridY + row * cellH, cellW, cellH };
      const bool onCursor = (row == cy && col == cx);
      const bool powered = mGameElec->isPowered(row, col);
      const bool hiddenUnknown = mGameElec->isHidden(row, col) && !powered;

      fillBox(onCursor ? CAColor::YELLOW : CAColor::BLACK, cell);

      // Frame: source is cyan, targets are red until linked, then green.
      SDL_Color frame = CAColor::DKGREY;
      if (mGameElec->isSource(row, col))  { frame = CAColor::CYAN; }
      else if (mGameElec->isTarget(row, col))  { frame = powered ? CAColor::GREEN : CAColor::RED; }
      drawBox(frame, cell);

      const char* glyph = hiddenUnknown ? "?" : junctionGlyph(mGameElec->junctionAt(row, col));
      SDL_Color ink = powered ? CAColor::GREEN : CAColor::DKGREY;
      if (onCursor)  { ink = CAColor::BLACK; }
      if (hiddenUnknown && !onCursor)  { ink = CAColor::DKGREY; }
      drawText(ink, glyph, SDL_Point{ cell.x + 9, cell.y + 4 });
    }
  }

  // Status line: progress and remaining turns.
  char status[64];
  std::snprintf(status, sizeof(status), "Linked %d/%d    Turns left %d",
                mGameElec->connectedTargets(), MiniGameElectronics::targetCount(),
                mGameElec->rotationsLeft());
  centerText(CAColor::GREY, status, 156);
  centerText(CAColor::GREY, "Arrows move  ENTER rotate  ESC leave", 172);
}

// TODO: make order of functions match the class order in the header
Difficulty GameState::getGameDifficulty()
{
  return this->gameDifficulty;
}

void GameState::setGameDifficulty(Difficulty gd)
{
  this->gameDifficulty = gd;
}
