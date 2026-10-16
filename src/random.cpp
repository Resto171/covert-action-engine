#include "random.h"

#include <utility>

namespace cae {
namespace {

// The shared engine. Seeded on first use so static-initialisation order does
// not matter, and so seedRng() can override the seed afterwards.
std::mt19937& engine() {
  static std::mt19937 e{std::random_device{}()};
  return e;
}

}  // namespace

std::mt19937& rng() { return engine(); }

void seedRng(std::uint32_t seed) { engine().seed(seed); }

int randInt(int lo, int hi) {
  if (lo > hi) { std::swap(lo, hi); }
  std::uniform_int_distribution<int> dist(lo, hi);
  return dist(engine());
}

int randPct() { return randInt(0, 99); }

}  // namespace cae
