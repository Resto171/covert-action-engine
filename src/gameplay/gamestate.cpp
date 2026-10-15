#include "gamestate.h"



void GameState::initMiniGameClasses()
{
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
        if (condition == ScreenExitCondition::None)  { /*TODO: back to prev menu? */ }
        if (condition == ScreenExitCondition::Timeout)  { /*TODO: back to prev menu? */ }
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
        std::cout << this->modalSequence << std::endl;
        // TODO: set origScreen to BeginMenu
        // TODO: how to deal with the transition from not in minigame into the minigame??
        if (this->modalSequence == 3) {
          if (this->selectedChoice == 0)  { currentScreen = GameScreen::MiniGameCombatStart; }
          if (this->selectedChoice == 1)  { currentScreen = GameScreen::MiniGameDrivingStart; }
          if (this->selectedChoice == 2)  { currentScreen = GameScreen::MiniGameCryptoStart; }
          if (this->selectedChoice == 3)  { currentScreen = GameScreen::MiniGameElecStart; }
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
        if (this->selectedChoice == 0)  { currentScreen = GameScreen::CityMainMenu; }
        if (this->selectedChoice == 1)  { currentScreen = GameScreen::MiniGameCombatStart; }
        if (this->selectedChoice == 2)  { currentScreen = GameScreen::MiniGameElecStart; }
        // TODO: watch should go back to the correct building
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
      case GameScreen::MiniGameCryptoStart:
      case GameScreen::MiniGameElecStart:
        // TODO: actually run the minigame. For now return to skill practice.
        currentScreen = GameScreen::SkillPractice;
        this->modalSequence = 0;
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
    case OriginScreen::UNKNOWNORIGIN:
    default:
      return GameScreen::CityMainMenu;
  }
}


void GameState::enterMiniGame(MiniGameType miniGame)
{
  /* TODO-debug
  this->currentMiniGame = miniGame;
  this->inMiniGame = true;
  switch (miniGame) {
    case MiniGameType::Combat:  // TODO: define breakin, breakout, or ambush
      mGameCombat->start(Difficulty::Level1);  // TODO: determine difficulty within class
      break;
    case MiniGameType::Driving:  // TODO: define chase or evade
      mGameDriving->start(Difficulty::Level1);
      break;
    case MiniGameType::Crypto:
      mGameCrypto->start(Difficulty::Level1);
      break;
    case MiniGameType::Elec:  // TODO: define tap or trace
      mGameElec->start(Difficulty::Level1);
      break;
  }

// */
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
