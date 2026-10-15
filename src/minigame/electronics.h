#pragma once

#include <random>
//#include <utility>
#include <array>

#include "minigame.h"
/// #include "../gameplay/gamestate.h"
/// #include "../covert.h"


/// #include "../covert.h"

enum Junction {
  STRAIGHT = 1,
  CROSS,
  TOP_TO_ALL,
  BOT_TO_ALL,
  ALL_TO_TOP,
  ALL_TO_BOT,          // Level I
  STRAIGHT_INV_TOP,
  STRAIGHT_INV_BOT,    // Level II
  STRAIGHT_INV_ALL,
  CROSS_INV_TOP_LEFT,  // Level III
  CROSS_INV_BOT_LEFT,
  CROSS_INV_ALL,       // Level IV
};


struct Junc {
  Junction junction { STRAIGHT };
  bool hidden { false };
  bool locked { false };
};  // TODO: typedef?


class MiniGameElectronics : public MiniGame {
public:
  // Board geometry. The renderer (GameState::drawMiniGame) lays the board out
  // using these so the two stay in step.
  static constexpr int kRows = 5;
  static constexpr int kCols = 7;

  MiniGameElectronics();
  ~MiniGameElectronics() = default;  // TODO
  int start(Difficulty level);
  void draw() override;
  Difficulty getGameDifficulty() override;

  // Inspection, used by the renderer.
  Junction junctionAt(int row, int col) const { return junction_matrix[row][col].junction; }
  unsigned char cursorX() const { return sx; }
  unsigned char cursorY() const { return sy; }

  // Player actions.
  void moveCursor(int dx, int dy);
  void rotateCursor();  // advance the junction under the cursor to the next type

private:
  unsigned char sx { 0 };
  unsigned char sy { 0 };
  std::array<std::array<Junc, kCols>, kRows> junction_matrix {};
  std::array<std::array<bool, 6>, 8> crossing_matrix {};
  Junc holding {};
};
