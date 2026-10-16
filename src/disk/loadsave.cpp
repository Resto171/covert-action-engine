#include "loadsave.h"

#include "../gameplay/gamestate.h"
#include "../gameplay/mainchar.h"

#include <array>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <string>
#include <utility>

namespace {

// Slot names for the five save games, in index order.
constexpr std::array<const char*, 5> kSaveFiles = {
    "CV0.SVE", "CV1.SVE", "CV2.SVE", "CV3.SVE", "CV4.SVE",
};

// Every save starts with this line so a foreign/legacy file can be rejected
// rather than misparsed.
constexpr char kMagic[] = "CAESAVE1";

constexpr char kEmptySlot[] = " - Empty -";

// Read one line, tolerating CRLF (the saves may have been written on Windows).
bool readLine(std::istream& in, std::string& line) {
    if (!std::getline(in, line)) {
        return false;
    }
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    return true;
}

}  // namespace


bool writeSaveGame(const std::string& path, const SaveGame& save) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }

    out << kMagic << "\n";
    out << "callsign=" << save.callsign << "\n";
    out << "rank=" << save.rank << "\n";
    out << "apt_combat=" << save.apt_combat << "\n";
    out << "apt_driving=" << save.apt_driving << "\n";
    out << "apt_crypto=" << save.apt_crypto << "\n";
    out << "apt_elec=" << save.apt_elec << "\n";
    out << "difficulty=" << save.difficulty << "\n";
    out << "screen=" << save.screen << "\n";
    out << "orig_screen=" << save.orig_screen << "\n";
    out << "modal_sequence=" << save.modal_sequence << "\n";
    out << "is_twelve=" << save.is_twelve << "\n";
    out << "city=" << save.city << "\n";
    return static_cast<bool>(out);
}


bool readSaveGame(const std::string& path, SaveGame& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }

    std::string line;
    if (!readLine(in, line) || line != kMagic) {
        return false;  // not one of our saves
    }

    SaveGame save;
    while (readLine(in, line)) {
        const std::size_t eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        const std::string key = line.substr(0, eq);
        const std::string val = line.substr(eq + 1);

        if (key == "callsign")         { save.callsign = val; }
        else if (key == "rank")        { save.rank = std::atoi(val.c_str()); }
        else if (key == "apt_combat")  { save.apt_combat = std::atoi(val.c_str()); }
        else if (key == "apt_driving") { save.apt_driving = std::atoi(val.c_str()); }
        else if (key == "apt_crypto")  { save.apt_crypto = std::atoi(val.c_str()); }
        else if (key == "apt_elec")    { save.apt_elec = std::atoi(val.c_str()); }
        else if (key == "difficulty")  { save.difficulty = std::atoi(val.c_str()); }
        else if (key == "screen")      { save.screen = std::atoi(val.c_str()); }
        else if (key == "orig_screen")     { save.orig_screen = std::atoi(val.c_str()); }
        else if (key == "modal_sequence")  { save.modal_sequence = std::atoi(val.c_str()); }
        else if (key == "is_twelve")       { save.is_twelve = std::atoi(val.c_str()); }
        else if (key == "city")        { save.city = val; }
    }

    out = std::move(save);
    return true;
}


SaveGame snapshotSaveGame() {
    SaveGame save;

    if (mainChar) {
        save.callsign = mainChar->getCallsign();
        save.rank = mainChar->getRank();
        save.apt_combat = static_cast<int>(mainChar->getCombatAptitude());
        save.apt_driving = static_cast<int>(mainChar->getDrivingAptitude());
        save.apt_crypto = static_cast<int>(mainChar->getCryptoAptitude());
        save.apt_elec = static_cast<int>(mainChar->getElecAptitude());
    }

    if (gameState) {
        save.difficulty = static_cast<int>(gameState->gameDifficulty);
        save.screen = static_cast<int>(gameState->currentScreen);
        save.orig_screen = static_cast<int>(gameState->origScreen);
        save.modal_sequence = gameState->modalSequence;
        save.is_twelve = gameState->isTwelve ? 1 : 0;
        save.city = gameState->currentCity;
    }

    return save;
}


void applySaveGame(const SaveGame& save) {
    if (!mainChar) {
        mainChar = std::make_unique<MainChar>();
    }
    mainChar->setCallsign(save.callsign);
    mainChar->setRank(save.rank);
    mainChar->setSkillLevels(static_cast<Aptitude>(save.apt_combat),
                             static_cast<Aptitude>(save.apt_driving),
                             static_cast<Aptitude>(save.apt_crypto),
                             static_cast<Aptitude>(save.apt_elec));

    if (!gameState) {
        gameState = std::make_unique<GameState>();
    }
    gameState->setGameDifficulty(static_cast<Difficulty>(save.difficulty));
    gameState->currentScreen = static_cast<GameScreen>(save.screen);
    gameState->origScreen = static_cast<OriginScreen>(save.orig_screen);
    gameState->modalSequence = save.modal_sequence;
    gameState->isTwelve = (save.is_twelve != 0);
    gameState->currentCity = save.city;
}


std::vector<std::string> show_game_files() {
    std::vector<std::string> game_names;
    game_names.reserve(kSaveFiles.size());

    for (const char* fname : kSaveFiles) {
        SaveGame save;
        if (readSaveGame(fname, save) && !save.callsign.empty()) {
            game_names.push_back(save.callsign);
        } else {
            game_names.emplace_back(kEmptySlot);
        }
    }
    return game_names;
}


FileStatus load_game(int game_idx) {
    if (game_idx < 0 || static_cast<std::size_t>(game_idx) >= kSaveFiles.size()) {
        return FileStatus::NotFound;  // out-of-range slot
    }

    SaveGame save;
    if (!readSaveGame(kSaveFiles[game_idx], save)) {
        return FileStatus::NotFound;
    }

    applySaveGame(save);
    return FileStatus::Ok;
}


FileStatus save_game_slot(int game_idx) {
    if (game_idx < 0 || static_cast<std::size_t>(game_idx) >= kSaveFiles.size()) {
        return FileStatus::NotFound;
    }
    return writeSaveGame(kSaveFiles[game_idx], snapshotSaveGame())
               ? FileStatus::Ok
               : FileStatus::Invalid;
}


int save_game(const char* fname) {
    if (fname == nullptr) {
        return -1;
    }
    return writeSaveGame(fname, snapshotSaveGame()) ? 0 : -1;
}
