#include "electronics.h"

#include <utility>

#include "../random.h"


    #if defined(TEST_ACTIVE)
    #include <iostream>
    #endif


unsigned char junctionSides(Junction j) {
  switch (j) {
    case STRAIGHT:           return SIDE_W | SIDE_E;                    // horizontal
    case CROSS:              return SIDE_N | SIDE_E | SIDE_S | SIDE_W;  // all four
    case TOP_TO_ALL:         return SIDE_N | SIDE_E | SIDE_W;           // tee, open down
    case BOT_TO_ALL:         return SIDE_S | SIDE_E | SIDE_W;           // tee, open up
    case ALL_TO_TOP:         return SIDE_N | SIDE_E | SIDE_S;           // tee, open left
    case ALL_TO_BOT:         return SIDE_N | SIDE_S | SIDE_W;           // tee, open right
    case STRAIGHT_INV_TOP:   return SIDE_W | SIDE_N;                    // corner
    case STRAIGHT_INV_BOT:   return SIDE_E | SIDE_N;                    // corner
    case STRAIGHT_INV_ALL:   return SIDE_N | SIDE_S;                    // vertical
    case CROSS_INV_TOP_LEFT: return SIDE_W | SIDE_S;                    // corner
    case CROSS_INV_BOT_LEFT: return SIDE_E | SIDE_S;                    // corner
    case CROSS_INV_ALL:      return SIDE_N | SIDE_E | SIDE_S | SIDE_W;  // all four
  }
  return 0;
}


MiniGameElectronics::MiniGameElectronics() {
#if defined(TEST_ACTIVE)
  std::cout << "ELECTRONICS MINIGAME CLASS CREATED\n";
#endif
}


int MiniGameElectronics::start(Difficulty level) {
  begin(level);  // records the difficulty and clears any previous result
  const int lv = static_cast<int>(level);
  const int highest_random = 4 + 2 * lv;  // Level I..IV tile sets

  sx = 0;
  sy = 0;

  // Randomize junctions from the tile set allowed at this difficulty.
  for (auto& row : junction_matrix) {
    for (auto& elem : row) {
      elem.junction = static_cast<Junction>(cae::randInt(1, highest_random));
      elem.hidden = false;
      elem.locked = false;
    }
  }

  // Higher difficulties hide the occasional junction and weld others shut.
  // The source is never hidden or locked so the board always has a way in.
  for (int r = 0; r < kRows; ++r) {
    for (int c = 0; c < kCols; ++c) {
      if (r == kSourceRow && c == kSourceCol)  { continue; }
      junction_matrix[r][c].hidden = (cae::randPct() < 10 * lv);
      junction_matrix[r][c].locked = (cae::randPct() < 3 * lv);
    }
  }

  // A turn budget stands in for the trace timer: too many rotations trips it.
  rotations_left = 36 - 4 * lv;
  if (rotations_left < 12)  { rotations_left = 12; }

  recomputePower();
  draw();
  return 0;
}


void MiniGameElectronics::draw() {
  // Rendering is done by GameState::drawMiniGame(); this exists only to satisfy
  // the MiniGame interface (and for the TEST_ACTIVE trace below).
#if defined(TEST_ACTIVE)
  std::cout << "elec board " << kRows << "x" << kCols
            << " linked " << connectedTargets() << "/" << targetCount() << "\n";
#endif
}


void MiniGameElectronics::onMove(int dx, int dy) {
  moveCursor(dx, dy);
  tick();
}


void MiniGameElectronics::onActivate() {
  rotateCursor();  // a no-op on a locked junction
  tick();
  // Rotating can finish the puzzle (all targets linked) or exhaust the turn
  // budget (the trace completes); anything else keeps the board open.
  const ElecOutcome o = outcome();
  if (o == ElecOutcome::Success)  { finish(ScreenExitCondition::PhoneBugged); }
  else if (o == ElecOutcome::Alarm) { finish(ScreenExitCondition::Lose); }
}


void MiniGameElectronics::recomputePower() {
  for (auto& row : powered) {
    row.fill(false);
  }

  const int dr[4] = { -1, 0, 1, 0 };
  const int dc[4] = { 0, 1, 0, -1 };
  const unsigned char sideBit[4] = { SIDE_N, SIDE_E, SIDE_S, SIDE_W };

  // The source is always powered; it need not expose a side toward anything.
  std::array<std::pair<int, int>, kRows * kCols> stack {};
  int top = 0;
  powered[kSourceRow][kSourceCol] = true;
  stack[top++] = { kSourceRow, kSourceCol };

  while (top > 0) {
    const auto [r, c] = stack[--top];
    const unsigned char here = junctionSides(junction_matrix[r][c].junction);

    for (int d = 0; d < 4; ++d) {
      if (!(here & sideBit[d]))  { continue; }
      const int nr = r + dr[d];
      const int nc = c + dc[d];
      if (nr < 0 || nr >= kRows || nc < 0 || nc >= kCols)  { continue; }
      if (powered[nr][nc])  { continue; }

      const unsigned char there = junctionSides(junction_matrix[nr][nc].junction);
      const unsigned char opposite = sideBit[(d + 2) % 4];
      if (!(there & opposite))  { continue; }

      powered[nr][nc] = true;
      stack[top++] = { nr, nc };
    }
  }
}


int MiniGameElectronics::connectedTargets() const {
  int count = 0;
  for (int r = 0; r < kRows; ++r) {
    if (powered[r][kCols - 1])  { ++count; }
  }
  return count;
}


ElecOutcome MiniGameElectronics::outcome() const {
  if (isSolved())  { return ElecOutcome::Success; }
  if (rotations_left <= 0)  { return ElecOutcome::Alarm; }
  return ElecOutcome::InProgress;
}


void MiniGameElectronics::moveCursor(int dx, int dy) {
  int nx = (int)sx + dx;
  int ny = (int)sy + dy;
  if (nx < 0)  { nx = 0; }
  if (nx >= kCols)  { nx = kCols - 1; }
  if (ny < 0)  { ny = 0; }
  if (ny >= kRows)  { ny = kRows - 1; }
  sx = (unsigned char)nx;
  sy = (unsigned char)ny;
}


void MiniGameElectronics::rotateCursor() {
  Junc& j = junction_matrix[sy][sx];
  if (j.locked)  { return; }  // welded shut: cannot be turned

  int v = (int)j.junction + 1;
  if (v > (int)CROSS_INV_ALL)  { v = (int)STRAIGHT; }
  j.junction = (Junction)v;

  if (rotations_left > 0)  { --rotations_left; }
  recomputePower();
}


void MiniGameElectronics::setJunction(int row, int col, Junction j) {
  junction_matrix[row][col].junction = j;
  recomputePower();
}


void MiniGameElectronics::setLocked(int row, int col, bool locked) {
  junction_matrix[row][col].locked = locked;
}
