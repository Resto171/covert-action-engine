#pragma once

#include <array>
#include <random>

#include "minigame.h"


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


// The four sides of a junction cell, as a bitmask. A junction connects a subset
// of them: a signal may travel between any two sides that are both set.
enum JunctionSide : unsigned char {
  SIDE_N = 0x1,
  SIDE_E = 0x2,
  SIDE_S = 0x4,
  SIDE_W = 0x8,
};


// Which sides a junction type exposes.
//
// NOTE: the original tile semantics have not been reverse-engineered, so this
// is a documented interpretation (see content/wiretaps.md) chosen so that every
// type is a sensible pipe shape and later types are strictly harder.
unsigned char junctionSides(Junction j);


// Outcome of the electronics puzzle.
enum class ElecOutcome {
  InProgress,  // still solvable within the remaining rotations
  Success,     // every target is connected to the source
  Alarm,       // ran out of rotations before the circuit was completed
};


struct Junc {
  Junction junction { STRAIGHT };
  bool hidden { false };
  bool locked { false };
};


class MiniGameElectronics : public MiniGame {
public:
  // Board geometry. The renderer (GameState::drawMiniGame) lays the board out
  // using these so the two stay in step.
  static constexpr int kRows = 5;
  static constexpr int kCols = 7;

  // The signal enters at this cell. Every cell in the rightmost column is a
  // connection point that must be linked to the source (content/wiretaps.md
  // notes six connections; the board is sized to that idea).
  static constexpr int kSourceRow = kRows / 2;
  static constexpr int kSourceCol = 0;

  MiniGameElectronics();
  ~MiniGameElectronics() = default;
  int start(Difficulty level) override;
  void draw() override;
  void onMove(int dx, int dy) override;
  void onActivate() override;

  // Inspection, used by the renderer and by tests.
  Junction junctionAt(int row, int col) const { return junction_matrix[row][col].junction; }
  bool isLocked(int row, int col) const { return junction_matrix[row][col].locked; }
  bool isHidden(int row, int col) const { return junction_matrix[row][col].hidden; }
  bool isPowered(int row, int col) const { return powered[row][col]; }
  bool isSource(int row, int col) const { return row == kSourceRow && col == kSourceCol; }
  bool isTarget(int /*row*/, int col) const { return col == kCols - 1; }
  static int targetCount() { return kRows; }
  int connectedTargets() const;
  bool isSolved() const { return connectedTargets() == targetCount(); }
  ElecOutcome outcome() const;
  int rotationsLeft() const { return rotations_left; }

  unsigned char cursorX() const { return sx; }
  unsigned char cursorY() const { return sy; }

  // Player actions.
  void moveCursor(int dx, int dy);
  void rotateCursor();  // advance the junction under the cursor to the next type

  // Tests/loaders: place a specific junction type. Mutations recompute the
  // connectivity so the outcome stays consistent.
  void setJunction(int row, int col, Junction j);
  void setLocked(int row, int col, bool locked);

private:
  // Flood-fill the power grid from the source across matching junction sides.
  void recomputePower();

  unsigned char sx { 0 };
  unsigned char sy { 0 };
  std::array<std::array<Junc, kCols>, kRows> junction_matrix {};
  std::array<std::array<bool, kCols>, kRows> powered {};
  int rotations_left { 0 };
};
