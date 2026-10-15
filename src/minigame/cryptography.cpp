#include "cryptography.h"

#include <algorithm>
#include <numeric>
#include <utility>


    #if defined(TEST_ACTIVE)
    #include <iostream>
    #endif


namespace {
// Messages to decode. Kept short so they fit the play area, and in the spirit
// of the game's intercepted traffic. TODO: load from PROSE.DTA once the data
// parsers land.
const char* const kMessages[] = {
  "MEET ME AT THE DOCKS AT MIDNIGHT",
  "THE SHIPMENT ARRIVES ON TUESDAY",
  "OUR AGENT IN CAIRO HAS BEEN COMPROMISED",
  "TRANSFER THE FUNDS TO THE USUAL ACCOUNT",
  "THE MASTERMIND IS HIDING IN MEDELLIN",
  "ABORT THE MISSION AND LEAVE THE CITY",
};
}  // namespace


MiniGameCryptography::MiniGameCryptography() {
#if defined(TEST_ACTIVE)
  std::cout << "CRYPTOGRAPHY MINIGAME CLASS CREATED\n";
#endif
}


int MiniGameCryptography::start(Difficulty level) {
  (void)level;  // TODO: difficulty affects message length and punctuation
  std::random_device seeder;
  std::mt19937 rng(seeder());

  // Shuffle the alphabet into a random substitution.
  std::string alphabet(26, ' ');
  std::iota(alphabet.begin(), alphabet.end(), 'A');
  std::shuffle(alphabet.begin(), alphabet.end(), rng);

  for (int i = 0; i < 26; ++i) {
    cipher_key[i] = alphabet[i];                 // plaintext 'A'+i -> cipher
    decipher_key[alphabet[i] - 'A'] = 'A' + i;   // cipher -> plaintext 'A'+i
  }

  // Pick a message and encrypt it (non-letters pass through unchanged).
  std::uniform_int_distribution<> pick(0, (int)(sizeof(kMessages) / sizeof(kMessages[0])) - 1);
  plain_message = kMessages[pick(rng)];

  cipher_message.clear();
  cipher_message.reserve(plain_message.size());
  letter_counts.fill(0);
  for (char p : plain_message) {
    if (p >= 'A' && p <= 'Z') {
      const char c = cipher_key[p - 'A'];
      cipher_message.push_back(c);
      ++letter_counts[c - 'A'];
    } else {
      cipher_message.push_back(p);
    }
  }

  user_decipher.fill(' ');
  cursor_idx = 0;
  return 0;
}


int MiniGameCryptography::correctCount() const {
  int count = 0;
  for (int i = 0; i < 26; ++i) {
    if (user_decipher[i] == decipher_key[i])  { ++count; }
  }
  return count;
}


void MiniGameCryptography::moveCursor(int dx, int dy) {
  // The alphabet is laid out as two rows of thirteen letters.
  cursor_idx = ((cursor_idx + dx + 13 * dy) % 26 + 26) % 26;
}


void MiniGameCryptography::cycleLetter() {
  char& guess = user_decipher[cursor_idx];
  if (guess == ' ')       { guess = 'A'; }
  else if (guess == 'Z')  { guess = ' '; }
  else                    { ++guess; }
}


void MiniGameCryptography::draw() {
  // Rendering is done by GameState::drawMiniGame().
#if defined(TEST_ACTIVE)
  std::cout << "crypto ciphertext: " << cipher_message << "\n";
#endif
}


MiniGameCryptography::~MiniGameCryptography() {}

Difficulty MiniGameCryptography::getGameDifficulty() {
  return Difficulty::Level1;  // TODO
}
