#pragma once

#include <cstdint>
#include <random>

// The engine's single random number generator.
//
// Each minigame used to build its own std::random_device-backed engine, which
// made the sequence unseedable and unshareable. Everything that needs
// randomness now draws from rng() instead, so a single seed can make a whole
// run reproducible (useful for tests and for a future "granular randomization"
// settings file).
namespace cae {

// The shared generator, seeded once (lazily) from std::random_device.
std::mt19937& rng();

// Re-seed the shared generator so a run becomes reproducible.
void seedRng(std::uint32_t seed);

// A uniform integer in [lo, hi] (inclusive). Arguments are swapped if needed.
int randInt(int lo, int hi);

// A uniform integer in [0, 99].
int randPct();

}  // namespace cae
