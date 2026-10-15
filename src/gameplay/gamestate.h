#pragma once

/// #include <functional>
#include <memory>
#include "../gui/gui.h"
#include "../minigame/cryptography.h"
#include "../minigame/electronics.h"

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

  // Renders the running minigame (override of the GUI hook).
  void drawMiniGame() override;

  // The minigame instances (null until initMiniGameClasses runs). Exposed for
  // the renderer and tests.
  MiniGameElectronics* electronics() { return mGameElec.get(); }
  MiniGameCryptography* cryptography() { return mGameCrypto.get(); }

  // Minigame input, driven by the key handler. These only mutate game state;
  // the caller redraws afterwards.
  void miniGameMove(int dx, int dy);
  void miniGameRotate();
  void miniGameCancel();  // leave the minigame without a specific outcome

  // Resolve the running minigame: hand the outcome to the FSM, then clear the
  // minigame state. The caller redraws the resulting screen.
  void finishMiniGame(ScreenExitCondition condition);

  MiniGameType currentMiniGame { MiniGameType::Inactive };
  GameScreen currentScreen { GameScreen::Splash1 };
  bool inMiniGame { false };
  bool isTwelve { true };
  bool shouldQuit { false };
  OriginScreen origScreen { OriginScreen::UNKNOWNORIGIN };
  int modalSequence { 0 };
  Difficulty gameDifficulty { Difficulty::Level1 };

private:
  std::unique_ptr<MiniGameElectronics> mGameElec;
  std::unique_ptr<MiniGameCryptography> mGameCrypto;
};


// The game state for the running session (created in do_covert()).
extern std::unique_ptr<GameState> gameState;
