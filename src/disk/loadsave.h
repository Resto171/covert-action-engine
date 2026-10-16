#pragma once

#include <string>
#include <vector>

enum class FileStatus : int {
    Ok = 0,
    NotFound = 1,
    Invalid = 2,
};

// A snapshot of the persisted parts of a session. Extend as more of the game
// becomes saveable (mission state, clues, current city, ...).
struct SaveGame {
    std::string callsign;
    int rank { 0 };
    int apt_combat { 0 };
    int apt_driving { 0 };
    int apt_crypto { 0 };
    int apt_elec { 0 };
    int difficulty { 1 };
    int screen { 0 };
    int orig_screen { -1 };   // OriginScreen as int (UNKNOWNORIGIN == -1)
    int modal_sequence { 0 };
    int is_twelve { 1 };      // bool as int
    std::string city;
};

// Pure serialization: no globals and no SDL, so it can be unit-tested in
// isolation. The format is a small tagged text file (magic line + key=value);
// files that don't carry the magic are treated as foreign and rejected.
bool writeSaveGame(const std::string& path, const SaveGame& save);
bool readSaveGame(const std::string& path, SaveGame& out);

// Snapshot/restore the running session (mainChar/gameState globals).
SaveGame snapshotSaveGame();
void applySaveGame(const SaveGame& save);

// Slot-based wrappers over the running session. There are five slots
// (CV0.SVE .. CV4.SVE).
std::vector<std::string> show_game_files();
FileStatus load_game(int game_idx);
FileStatus save_game_slot(int game_idx);
int save_game(const char* fname);
