#include "cryptography.h"

#include <algorithm>
#include <numeric>
#include <utility>
#include <vector>

#include "../random.h"


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
  begin(level);  // records the difficulty and clears any previous result

  // Shuffle the alphabet into a random substitution.
  std::string alphabet(26, ' ');
  std::iota(alphabet.begin(), alphabet.end(), 'A');
  std::shuffle(alphabet.begin(), alphabet.end(), cae::rng());

  for (int i = 0; i < 26; ++i) {
    cipher_key[i] = alphabet[i];                 // plaintext 'A'+i -> cipher
    decipher_key[alphabet[i] - 'A'] = 'A' + i;   // cipher -> plaintext 'A'+i
  }

  // Pick a message to encrypt (non-letters pass through unchanged). Higher
  // difficulties draw from a wider pool that includes longer messages, so there
  // are more letters to decode.
  const int count = (int)(sizeof(kMessages) / sizeof(kMessages[0]));
  std::vector<int> order(count);
  std::iota(order.begin(), order.end(), 0);
  std::sort(order.begin(), order.end(), [](int a, int b) {
    return std::char_traits<char>::length(kMessages[a]) <
           std::char_traits<char>::length(kMessages[b]);
  });
  int pool = static_cast<int>(level) + 1;  // Level I -> 2 messages, Level IV -> 5
  if (pool > count)  { pool = count; }
  if (pool < 1)      { pool = 1; }
  plain_message = kMessages[order[cae::randInt(0, pool - 1)]];

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


void MiniGameCryptography::onMove(int dx, int dy) {
  moveCursor(dx, dy);
  tick();
}


void MiniGameCryptography::onActivate() {
  cycleLetter();
  tick();
  if (isSolved())  { finish(ScreenExitCondition::Win); }
}


MiniGameCryptography::~MiniGameCryptography() {}
