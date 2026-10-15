#pragma once

/// #include <functional>
/// #include <memory>
#include "../gui/gui.h"

class GameState : public GUI {
public:
  GameState() = default;
  ~GameState() = default;
  GameState(const GameState&) = delete;
  GameState& operator=(const GameState&) = delete;

  void initMiniGameClasses();
  void startGUI();
  void enterMiniGame(MiniGameType miniGame);
  void getNextScreen(ScreenExitCondition condition = ScreenExitCondition::None);
  GameScreen goBackFrom(OriginScreen os);
  Difficulty getGameDifficulty();  // TODO
  void setGameDifficulty(Difficulty gameDifficulty);

  MiniGameType currentMiniGame { MiniGameType::Inactive };
  GameScreen currentScreen { GameScreen::Splash1 };
  bool inMiniGame { false };
  bool isTwelve { true };
  bool shouldQuit { false };
  OriginScreen origScreen { OriginScreen::UNKNOWNORIGIN };
  int modalSequence { 0 };
  Difficulty gameDifficulty { Difficulty::Level1 };
};


// The game state for the running session (created in do_covert()).
extern std::unique_ptr<GameState> gameState;
