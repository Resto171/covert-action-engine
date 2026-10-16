#pragma once

#include <cstdint>

#include "../types.h"

// Common services shared by every minigame: the difficulty it is played at, a
// coarse step counter, and a finished flag plus the win/lose outcome.
//
// GameState owns the concrete instances and forwards player input to whichever
// game is active (onMove / onActivate / onCancel). A game reports that it is
// over by calling finish(...); GameState reads isFinished()/result() and maps
// that outcome back onto the main finite state machine.
class MiniGame {
public:
  virtual ~MiniGame() = default;

  // Begin a new round at `level`. Implementations must call begin(level) (see
  // below) so the difficulty and finished state are reset.
  virtual int start(Difficulty level) = 0;

  // The difficulty this round is played at (as passed to start()).
  Difficulty getGameDifficulty() const { return difficulty; }

  // Outcome plumbing: set by a game when it reaches a terminal state.
  bool isFinished() const { return finished; }
  ScreenExitCondition result() const { return exit_result; }

  // Coarse step counter, advanced by the input hooks. Games with a turn/time
  // limit can build on it instead of threading a clock through every method.
  std::uint32_t timer() const { return ticks; }

  // Input hooks. The defaults do nothing, so a game only implements the ones
  // it actually uses. dx/dy are grid steps (right and down are positive).
  virtual void onMove(int /*dx*/, int /*dy*/) {}
  virtual void onActivate() {}  // ENTER: confirm / rotate
  virtual void onCancel() {}    // ESC: abort the minigame

  // Render the game. Most currently defer to GameState::drawMiniGame().
  virtual void draw() = 0;

protected:
  MiniGame() = default;

  // Call first from a concrete start(): records the difficulty and clears the
  // finished/result state so a reused instance always starts clean.
  void begin(Difficulty level) {
    difficulty = level;
    finished = false;
    exit_result = ScreenExitCondition::None;
    ticks = 0;
  }

  // Record a terminal outcome.
  void finish(ScreenExitCondition condition) {
    finished = true;
    exit_result = condition;
  }

  // Advance the step counter (call from the input hooks).
  void tick() { ++ticks; }

private:
  Difficulty difficulty { Difficulty::Level1 };
  bool finished { false };
  ScreenExitCondition exit_result { ScreenExitCondition::None };
  std::uint32_t ticks { 0 };
};
