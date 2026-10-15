#include "loadsave.h"

#ifndef NO_SDL
#include <SDL.h>
#endif

#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

// Slot names for the five save games, in index order.
constexpr std::array<const char*, 5> kSaveFiles = {
    "CV0.SVE", "CV1.SVE", "CV2.SVE", "CV3.SVE", "CV4.SVE",
};

// The player's callsign sits near the start of a save file (offset 0x46A).
constexpr std::size_t kCallsignOffset = 0x46A;
constexpr std::size_t kCallsignReadSize = 11;
// One extra byte so the buffer is always NUL-terminated after a read.
constexpr std::size_t kCallsignBufferSize = kCallsignReadSize + 1;

constexpr char kEmptySlot[] = " - Empty -";

}  // namespace


std::vector<std::string> show_game_files() {
#ifndef NO_SDL
    std::vector<std::string> game_names;
    game_names.reserve(kSaveFiles.size());

    // Zero-initialised so a short or absent read is still a valid C string.
    std::array<char, kCallsignBufferSize> callsign {};

    for (const char* fname : kSaveFiles) {
        callsign.fill('\0');

        SDL_RWops* reader = SDL_RWFromFile(fname, "rb");
        if (reader != nullptr) {
            // Show name, probably at offset 0x46A
            SDL_RWseek(reader, kCallsignOffset, RW_SEEK_SET);
            SDL_RWread(reader, callsign.data(), kCallsignReadSize, 1);
            SDL_RWclose(reader);
        } else {
            // Name is empty
            std::memcpy(callsign.data(), kEmptySlot, sizeof(kEmptySlot));
        }

        game_names.emplace_back(callsign.data());
    }
    return game_names;
#else
    return std::vector<std::string>{"A", "B", "C", "D"};
#endif
}


// TODO
#ifndef NO_SDL
FileStatus load_game(int game_idx) {
    // Guard the index: slot names are only valid for 0..kSaveFiles.size()-1.
    // Before this check an out-of-range index left `fname` pointing at
    // uninitialised memory which was then handed to SDL_RWFromFile.
    if (game_idx < 0 || static_cast<std::size_t>(game_idx) >= kSaveFiles.size()) {
        // Out-of-range slot: nothing to load.
        return FileStatus::NotFound;
    }

    SDL_RWops* reader = SDL_RWFromFile(kSaveFiles[game_idx], "rb");
    if (reader == nullptr) {
        // Save file does not exist
///         std::cout << kSaveFiles[game_idx] << std::endl;
        return FileStatus::NotFound;
    }
    SDL_RWclose(reader);
    return FileStatus::Ok;
}


int save_game(const char* fname) {
    SDL_RWops* reader = SDL_RWFromFile(fname, "rb");
    if (reader == nullptr) {
        // File does not yet exist, create fresh data for new file.

        // TODO

    } else {
        // Import existing save file, modify

        // TODO

        SDL_RWclose(reader);
    }

    SDL_RWops* writer = SDL_RWFromFile(fname, "wb");
    if (writer == nullptr) {
        throw std::runtime_error("cannot write to save game file");
    }

    SDL_RWwrite(writer, "abcde", 6, 1);
    SDL_RWclose(writer);
    return 0;
}
#endif
