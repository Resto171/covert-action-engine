#pragma once

#include <array>
#include <random>
#include <string>

#include "minigame.h"


// A substitution-cipher code-breaking puzzle. The engine generates a random
// cipher and encrypts a message with it; the player assigns a plaintext letter
// to each cipher letter until the message decodes.
class MiniGameCryptography : public MiniGame {
public:
  MiniGameCryptography();
  ~MiniGameCryptography() override;
  int start(Difficulty level);
  void draw() override;
  Difficulty getGameDifficulty() override;

  // Inspection, used by the renderer and by tests.
  const std::string& cipherMessage() const { return cipher_message; }
  char cipherLetter(int plainIdx) const { return cipher_key[plainIdx]; }        // plaintext 'A'+i -> cipher char
  char decipherLetter(int cipherIdx) const { return decipher_key[cipherIdx]; }  // cipher char 'A'+i -> plaintext
  char userLetter(int cipherIdx) const { return user_decipher[cipherIdx]; }
  unsigned int frequency(int cipherIdx) const { return letter_counts[cipherIdx]; }
  int cursor() const { return cursor_idx; }
  int correctCount() const;
  bool isSolved() const { return correctCount() == 26; }

  // Player actions.
  void moveCursor(int dx, int dy);  // dx steps one letter, dy steps one grid row
  void cycleLetter();               // advance the current letter's guess

  // Tests.
  void setUserLetter(int cipherIdx, char c) { user_decipher[cipherIdx] = c; }

private:
  std::string plain_message;
  std::string cipher_message;
  std::array<char, 26> cipher_key {};     // plaintext 'A'+i -> cipher char
  std::array<char, 26> decipher_key {};   // cipher char 'A'+i -> plaintext char
  std::array<char, 26> user_decipher {};  // the player's guesses, indexed by cipher char
  std::array<unsigned int, 26> letter_counts {};
  int cursor_idx { 0 };
};
