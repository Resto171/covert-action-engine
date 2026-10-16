#pragma once

#include <cstdint>
#include <vector>

#include "../types.h"

// The original game drew portraits by compositing artwork from its .PIC files.
// That artwork is copyrighted and is not shipped with this engine, so faces are
// generated procedurally from the same components the Face struct carries
// (gender, hair, eyes, skin, clothing, accessory).
//
// Everything here is pure C++ --- no SDL, no globals --- so it compiles and can
// be unit-tested in the console-only (NO_SDL) build. The result is deterministic:
// the same Face always composes to the same image.
namespace facegen {

struct Color {
  uint8_t r, g, b, a;
};

// A composed face: `width * height` palette indices into palette().
struct Image {
  int width = 0;
  int height = 0;
  std::vector<uint8_t> indices;
};

// Faces are always composed at this canonical size; callers scale them to
// whatever rectangle they need.
constexpr int kWidth = 48;
constexpr int kHeight = 48;

// The fixed palette used by compose(). Indices are stable and are what the
// returned Image refers to.
const std::vector<Color>& palette();

// Compose `face` into a kWidth x kHeight indexed image. Any Face value is
// accepted; the component ids are reduced modulo the number of variants.
Image compose(const Face& face);

// Deterministic face generator: the same seed always yields the same face.
Face makeFace(uint32_t seed);

// A fixed face for the chief, and the player's default face.
Face chiefFace();
Face playerFace();

// One of the candidate/mastermind faces for the lineup. `index` may be any
// value; it selects a stable face for that slot.
Face candidateFace(int index);

}  // namespace facegen
