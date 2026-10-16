#include "utest.h"

UTEST_MAIN();



UTEST(cae, test_utest) {
    ASSERT_TRUE(1);
}


// The engine uses plain globals for the running session; make sure they can be
// created and destroyed without SDL being initialised (no window/audio in a
// unit test).
#include "../src/covert.h"
#include "../src/gameplay/gamestate.h"
#include "../src/gameplay/mainchar.h"

UTEST(cae, session_objects_construct) {
    mainChar = std::make_unique<MainChar>();
    gameState = std::make_unique<GameState>();

    mainChar->setSkillLevels(Aptitude::Average, Aptitude::Average, Aptitude::Average, Aptitude::Average);
    EXPECT_EQ((int)Aptitude::Average, (int)mainChar->getCombatAptitude());

    gameState->setGameDifficulty(Difficulty::Level2);
    EXPECT_EQ((int)Difficulty::Level2, (int)gameState->getGameDifficulty());

    EXPECT_EQ((int)GameScreen::Splash1, (int)gameState->currentScreen);
}


#include "../src/disk/loadsave.h"

UTEST(cae, disk_show_game_files_lists_slots) {
    // Regardless of whether any file exists, one entry is produced per slot.
    EXPECT_FALSE(show_game_files().empty());
}


UTEST(cae, disk_load_game_rejects_out_of_range_slot) {
    // Slots are 0..4. Out-of-range indices must report NotFound rather than
    // being used as an index into the slot table.
    EXPECT_EQ((int)FileStatus::NotFound, (int)load_game(5));
    EXPECT_EQ((int)FileStatus::NotFound, (int)load_game(100));
    EXPECT_EQ((int)FileStatus::NotFound, (int)load_game(-1));
}


#include <cstdio>
#include <fstream>

UTEST(cae, save_game_round_trips_all_fields) {
    const std::string path = "cae_save_roundtrip.tmp";

    SaveGame in;
    in.callsign = "Max Power";
    in.rank = 3;
    in.apt_combat = (int)Aptitude::Good;
    in.apt_driving = (int)Aptitude::Average;
    in.apt_crypto = (int)Aptitude::Excellent;
    in.apt_elec = (int)Aptitude::Awesome;
    in.difficulty = (int)Difficulty::Level3;
    in.screen = (int)GameScreen::Hotel;
    in.city = "Cairo";

    ASSERT_TRUE(writeSaveGame(path, in));

    SaveGame out;
    ASSERT_TRUE(readSaveGame(path, out));
    EXPECT_TRUE(in.callsign == out.callsign);
    EXPECT_EQ(in.rank, out.rank);
    EXPECT_EQ(in.apt_combat, out.apt_combat);
    EXPECT_EQ(in.apt_driving, out.apt_driving);
    EXPECT_EQ(in.apt_crypto, out.apt_crypto);
    EXPECT_EQ(in.apt_elec, out.apt_elec);
    EXPECT_EQ(in.difficulty, out.difficulty);
    EXPECT_EQ(in.screen, out.screen);
    EXPECT_TRUE(in.city == out.city);

    std::remove(path.c_str());
}


UTEST(cae, read_save_game_rejects_missing_and_foreign_files) {
    SaveGame out;
    EXPECT_TRUE(!readSaveGame("cae_definitely_missing.tmp", out));

    // A file without our magic header must be rejected, not misparsed.
    const std::string path = "cae_save_foreign.tmp";
    {
        std::ofstream o(path);
        o << "not one of our saves\n";
    }
    EXPECT_TRUE(!readSaveGame(path, out));
    std::remove(path.c_str());
}


UTEST(cae, save_and_load_restores_the_session) {
    mainChar = std::make_unique<MainChar>();
    mainChar->setCallsign("Nova");
    mainChar->setRank(2);
    mainChar->setSkillLevels(Aptitude::Good, Aptitude::Average,
                             Aptitude::Excellent, Aptitude::Awesome);
    gameState = std::make_unique<GameState>();
    gameState->setGameDifficulty(Difficulty::Level3);
    gameState->currentScreen = GameScreen::Hotel;

    ASSERT_EQ((int)FileStatus::Ok, (int)save_game_slot(0));
    EXPECT_TRUE(show_game_files()[0] == "Nova");

    // Wipe the session, then load it back.
    mainChar = std::make_unique<MainChar>();
    gameState = std::make_unique<GameState>();
    ASSERT_EQ((int)FileStatus::Ok, (int)load_game(0));

    EXPECT_TRUE(mainChar->getCallsign() == "Nova");
    EXPECT_EQ(2, mainChar->getRank());
    EXPECT_EQ((int)Aptitude::Good, (int)mainChar->getCombatAptitude());
    EXPECT_EQ((int)Aptitude::Awesome, (int)mainChar->getElecAptitude());
    EXPECT_EQ((int)Difficulty::Level3, (int)gameState->getGameDifficulty());
    EXPECT_EQ((int)GameScreen::Hotel, (int)gameState->currentScreen);

    std::remove("CV0.SVE");

    // An empty slot cannot be loaded.
    std::remove("CV1.SVE");
    EXPECT_EQ((int)FileStatus::NotFound, (int)load_game(1));
}


UTEST(cae, fsm_load_screen_loads_a_slot) {
    // Prepare a slot on disk.
    mainChar = std::make_unique<MainChar>();
    mainChar->setCallsign("Echo");
    gameState = std::make_unique<GameState>();
    gameState->currentScreen = GameScreen::Hotel;
    ASSERT_EQ((int)FileStatus::Ok, (int)save_game_slot(0));

    // From the main menu, selecting slot 0 loads it (and restores its screen).
    mainChar = std::make_unique<MainChar>();
    gameState = std::make_unique<GameState>();
    gameState->currentScreen = GameScreen::Load;
    gameState->selectedChoice = 0;
    gameState->getNextScreen();

    EXPECT_TRUE(mainChar->getCallsign() == "Echo");
    EXPECT_EQ((int)GameScreen::Hotel, (int)gameState->currentScreen);

    std::remove("CV0.SVE");

    // An empty slot falls back to the main menu.
    std::remove("CV3.SVE");
    gameState->currentScreen = GameScreen::Load;
    gameState->selectedChoice = 3;
    gameState->getNextScreen();
    EXPECT_EQ((int)GameScreen::BeginMenu, (int)gameState->currentScreen);
}


UTEST(cae, fsm_loadsave_screen_saves_the_session) {
    mainChar = std::make_unique<MainChar>();
    mainChar->setCallsign("Rook");
    gameState = std::make_unique<GameState>();
    gameState->currentScreen = GameScreen::LoadSave;
    gameState->selectedChoice = 2;
    gameState->getNextScreen();

    EXPECT_EQ((int)GameScreen::Hotel, (int)gameState->currentScreen);
    EXPECT_TRUE(show_game_files()[2] == "Rook");

    std::remove("CV2.SVE");
}


#include "../src/gameplay/basegenerator.h"
#include <ios>

namespace {
struct Failing {};
std::ostream& operator<<(std::ostream& os, const Failing&) {
    os.setstate(std::ios::failbit);
    return os;
}
}

UTEST(cae, basegenerator_stringify) {
    BaseGenerator generator;

    EXPECT_TRUE(generator.stringify(42) == "42");
    EXPECT_TRUE(generator.stringify(-7) == "-7");
    EXPECT_TRUE(generator.stringify(std::string("abc")) == "abc");
    EXPECT_TRUE(generator.stringify(true) == "1");

    // A stream insertion failure must surface as a runtime_error, not silently
    // produce a truncated string.
    bool threw = false;
    try {
        generator.stringify(Failing{});
    } catch (const std::runtime_error&) {
        threw = true;
    }
    EXPECT_TRUE(threw);
}


// ---------------------------------------------------------------------------
// Finite state machine
//
// These drive GameState::getNextScreen() directly (no SDL, no stdin) and assert
// the resulting screen. This is the automated equivalent of walking the menus
// by hand, and it pins down the transition table so it cannot silently regress.
// ---------------------------------------------------------------------------

#define CAE_EXPECT_SCREEN(expected) \
    EXPECT_EQ((int)(expected), (int)gameState->currentScreen)


UTEST(cae, fsm_splash_sequence) {
    gameState = std::make_unique<GameState>();

    CAE_EXPECT_SCREEN(GameScreen::Splash1);
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::Splash2);
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::BeginMenu);
}


UTEST(cae, fsm_begin_menu_routes) {
    gameState = std::make_unique<GameState>();

    const struct { int choice; GameScreen expected; } routes[] = {
        {0, GameScreen::NewCharacter},
        {1, GameScreen::Load},
        {2, GameScreen::SkillPractice},
        {3, GameScreen::HallOfFame},
    };
    for (const auto& route : routes) {
        gameState->currentScreen = GameScreen::BeginMenu;
        gameState->selectedChoice = route.choice;
        gameState->getNextScreen();
        CAE_EXPECT_SCREEN(route.expected);
    }
}


UTEST(cae, fsm_load_and_hall_of_fame_return_to_menu) {
    gameState = std::make_unique<GameState>();

    gameState->currentScreen = GameScreen::Load;
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::BeginMenu);

    gameState->currentScreen = GameScreen::HallOfFame;
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::BeginMenu);
}


// The full "new game" opening, from the splashes through to quitting from the
// city menu. This mirrors the exact sequence observed when walking the console
// build by hand.
UTEST(cae, fsm_new_game_opening_sequence) {
    gameState = std::make_unique<GameState>();

    gameState->getNextScreen();  // Splash2
    gameState->getNextScreen();  // BeginMenu
    gameState->selectedChoice = 0;
    gameState->getNextScreen();  // NewCharacter
    CAE_EXPECT_SCREEN(GameScreen::NewCharacter);

    // Name entry takes four modal steps, then moves to the skill upgrade screen.
    gameState->getNextScreen();
    gameState->getNextScreen();
    gameState->getNextScreen();
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::SkillUpgrade);

    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::Chief);

    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::MastermindLineup);
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::Chief);
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::ShowNewClues);
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::Chief);

    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::CIAContact);
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::CIASam);
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::CIAMainMenu);

    gameState->selectedChoice = 0;  // "Leave (return to city)"
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::CityMainMenu);

    gameState->selectedChoice = 4;  // "Quit"
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::GameExitModal);
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::GameExit);
    gameState->getNextScreen();
    EXPECT_TRUE(gameState->shouldQuit);
}


UTEST(cae, fsm_minigame_start_returns_to_skill_practice) {
    gameState = std::make_unique<GameState>();

    // Combat/Driving are still unimplemented and bounce back to practice.
    const GameScreen starts[] = {
        GameScreen::MiniGameCombatStart,
        GameScreen::MiniGameDrivingStart,
    };
    for (GameScreen start : starts) {
        gameState->currentScreen = start;
        gameState->modalSequence = 2;
        gameState->getNextScreen();
        CAE_EXPECT_SCREEN(GameScreen::SkillPractice);
        EXPECT_EQ(0, gameState->modalSequence);
    }
}


// The cryptography minigame is wired up: entering its start screen begins the
// minigame, and cancelling it returns to where it was launched from (the
// practice menu).
UTEST(cae, fsm_crypto_minigame_enter_and_cancel) {
    gameState = std::make_unique<GameState>();
    gameState->currentScreen = GameScreen::MiniGameCryptoStart;

    gameState->getNextScreen();
    EXPECT_TRUE(gameState->inMiniGame);
    EXPECT_EQ((int)MiniGameType::Crypto, (int)gameState->currentMiniGame);
    CAE_EXPECT_SCREEN(GameScreen::MiniGameCryptoStart);
    ASSERT_TRUE(gameState->cryptography() != nullptr);

    // Input methods must be safe and only mutate state.
    gameState->miniGameMove(1, 0);
    gameState->miniGameMove(0, 1);
    gameState->miniGameRotate();
    EXPECT_TRUE(gameState->inMiniGame);

    gameState->miniGameCancel();
    EXPECT_TRUE(!gameState->inMiniGame);
    EXPECT_EQ((int)MiniGameType::Inactive, (int)gameState->currentMiniGame);
    CAE_EXPECT_SCREEN(GameScreen::SkillPractice);
}


// Choosing "Cryptography" from the practice menu must launch the minigame.
UTEST(cae, fsm_skill_practice_starts_crypto) {
    gameState = std::make_unique<GameState>();
    gameState->currentScreen = GameScreen::SkillPractice;
    gameState->selectedChoice = 2;  // Cryptography
    gameState->modalSequence = 2;   // the selection fires on the third tick

    gameState->getNextScreen();
    EXPECT_TRUE(gameState->inMiniGame);
    EXPECT_EQ((int)MiniGameType::Crypto, (int)gameState->currentMiniGame);
    CAE_EXPECT_SCREEN(GameScreen::MiniGameCryptoStart);
}


// The electronics minigame is wired up: entering its start screen begins the
// minigame, and cancelling it returns to the city.
UTEST(cae, fsm_elec_minigame_enter_and_cancel) {
    gameState = std::make_unique<GameState>();
    gameState->currentScreen = GameScreen::MiniGameElecStart;

    gameState->getNextScreen();
    EXPECT_TRUE(gameState->inMiniGame);
    EXPECT_EQ((int)MiniGameType::Elec, (int)gameState->currentMiniGame);
    CAE_EXPECT_SCREEN(GameScreen::MiniGameElecStart);

    // Force a deterministic, unsolvable board so the input below cannot end the
    // minigame by accident (a random board can rarely start already linked).
    MiniGameElectronics* mg = gameState->electronics();
    ASSERT_TRUE(mg != nullptr);
    for (int r = 0; r < MiniGameElectronics::kRows; ++r) {
        for (int c = 0; c < MiniGameElectronics::kCols; ++c) {
            mg->setJunction(r, c, STRAIGHT);
        }
    }

    // Input methods must be safe and only mutate state.
    gameState->miniGameMove(1, 0);
    gameState->miniGameMove(0, 1);
    gameState->miniGameRotate();
    EXPECT_TRUE(gameState->inMiniGame);

    gameState->miniGameCancel();
    EXPECT_TRUE(!gameState->inMiniGame);
    EXPECT_EQ((int)MiniGameType::Inactive, (int)gameState->currentMiniGame);
    CAE_EXPECT_SCREEN(GameScreen::CityMainMenu);
}


UTEST(cae, fsm_minigame_exit_conditions) {
    gameState = std::make_unique<GameState>();
    gameState->inMiniGame = true;
    gameState->currentMiniGame = MiniGameType::Combat;

    gameState->getNextScreen(ScreenExitCondition::Arrest);
    CAE_EXPECT_SCREEN(GameScreen::CombatArrest);
    gameState->getNextScreen(ScreenExitCondition::Shot);
    CAE_EXPECT_SCREEN(GameScreen::CombatRecover);
    gameState->getNextScreen(ScreenExitCondition::Lose);
    CAE_EXPECT_SCREEN(GameScreen::CombatCaptive);
    gameState->getNextScreen(ScreenExitCondition::None);
    CAE_EXPECT_SCREEN(GameScreen::CityMainMenu);

    // An inactive minigame must not transition anywhere.
    gameState->currentMiniGame = MiniGameType::Inactive;
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::CityMainMenu);
}


UTEST(cae, fsm_building_menu_maps_to_correct_screens) {
    // The building menu is: Place wiretap, Break in, Watch, Check Data, Leave.
    // "Place wiretap" (0) must start the electronics minigame, and "Watch the
    // building" (2) must go to the watch screen (this used to be swapped).
    gameState = std::make_unique<GameState>();
    gameState->currentScreen = GameScreen::Building1;
    gameState->selectedChoice = 0;
    gameState->getNextScreen();
    EXPECT_TRUE(gameState->inMiniGame);
    EXPECT_EQ((int)MiniGameType::Elec, (int)gameState->currentMiniGame);
    CAE_EXPECT_SCREEN(GameScreen::MiniGameElecStart);

    gameState = std::make_unique<GameState>();
    gameState->currentScreen = GameScreen::Building2;
    gameState->selectedChoice = 1;
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::MiniGameCombatStart);

    gameState = std::make_unique<GameState>();
    gameState->currentScreen = GameScreen::Building2;
    gameState->selectedChoice = 2;
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::BuildingWatch);

    // "Check Data" (3) now opens the building data screen.
    gameState = std::make_unique<GameState>();
    gameState->currentScreen = GameScreen::Building2;
    gameState->selectedChoice = 3;
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::Data);
    EXPECT_EQ((int)OriginScreen::Data, (int)gameState->origScreen);

    gameState = std::make_unique<GameState>();
    gameState->currentScreen = GameScreen::Building2;
    gameState->selectedChoice = 4;
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::CityMainMenu);
}


// The building "Check Data" reviews must return to the Data screen (not the CIA
// office), and "Return to city" must leave.
UTEST(cae, fsm_data_screen_reviews_return_to_data) {
    gameState = std::make_unique<GameState>();
    gameState->currentScreen = GameScreen::Building1;
    gameState->selectedChoice = 3;
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::Data);

    gameState->selectedChoice = 1;  // Review Clues
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::ReviewClues);
    gameState->getNextScreen();     // review screen returns via goBackFrom
    CAE_EXPECT_SCREEN(GameScreen::Data);

    gameState->selectedChoice = 5;  // Organization Summary
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::ReviewOrg);
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::Data);

    gameState->selectedChoice = 0;  // Return to city
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::CityMainMenu);
}


UTEST(cae, fsm_unknown_screen_exits) {
    gameState = std::make_unique<GameState>();
    gameState->currentScreen = GameScreen::UNKNOWN;
    gameState->getNextScreen();
    CAE_EXPECT_SCREEN(GameScreen::GameExit);
}


UTEST(cae, fsm_go_back_from) {
    gameState = std::make_unique<GameState>();

    EXPECT_EQ((int)GameScreen::Chief, (int)gameState->goBackFrom(OriginScreen::ChiefCaseIntro));
    EXPECT_EQ((int)GameScreen::Chief, (int)gameState->goBackFrom(OriginScreen::ChiefInfoHandout));
    EXPECT_EQ((int)GameScreen::Chief, (int)gameState->goBackFrom(OriginScreen::ChiefKickMaxOut));
    EXPECT_EQ((int)GameScreen::CIAMainMenu, (int)gameState->goBackFrom(OriginScreen::CIA));
    EXPECT_EQ((int)GameScreen::CityMainMenu, (int)gameState->goBackFrom(OriginScreen::CityMenu));
    EXPECT_EQ((int)GameScreen::CityMainMenu, (int)gameState->goBackFrom(OriginScreen::UNKNOWNORIGIN));
}


// ---------------------------------------------------------------------------
// Electronics minigame
//
// The board is a grid of junctions; a signal floods from the source across
// matching sides. The player rotates junctions until every right-column target
// is linked (success) or the turn budget runs out (alarm).
// ---------------------------------------------------------------------------

#include "../src/minigame/electronics.h"

namespace {
void fillElec(MiniGameElectronics& g, Junction j) {
    for (int r = 0; r < MiniGameElectronics::kRows; ++r) {
        for (int c = 0; c < MiniGameElectronics::kCols; ++c) {
            g.setJunction(r, c, j);
            g.setLocked(r, c, false);
        }
    }
}
}  // namespace


UTEST(cae, elec_board_geometry_and_cursor_bounds) {
    MiniGameElectronics g;
    g.start(Difficulty::Level1);

    EXPECT_EQ(5, MiniGameElectronics::kRows);
    EXPECT_EQ(7, MiniGameElectronics::kCols);
    EXPECT_TRUE(g.connectedTargets() >= 0);
    EXPECT_TRUE(g.connectedTargets() <= MiniGameElectronics::targetCount());

    // The cursor is clamped to the board.
    g.moveCursor(-100, -100);
    EXPECT_EQ(0, (int)g.cursorX());
    EXPECT_EQ(0, (int)g.cursorY());
    g.moveCursor(100, 100);
    EXPECT_EQ(MiniGameElectronics::kCols - 1, (int)g.cursorX());
    EXPECT_EQ(MiniGameElectronics::kRows - 1, (int)g.cursorY());
}


UTEST(cae, elec_cross_board_solves) {
    MiniGameElectronics g;
    g.start(Difficulty::Level1);
    fillElec(g, CROSS);

    EXPECT_EQ(MiniGameElectronics::targetCount(), g.connectedTargets());
    EXPECT_TRUE(g.isSolved());
    EXPECT_EQ((int)ElecOutcome::Success, (int)g.outcome());
}


UTEST(cae, elec_straight_board_is_under_connected) {
    MiniGameElectronics g;
    g.start(Difficulty::Level1);
    fillElec(g, STRAIGHT);

    // Only the source's own row reaches a target; the others are isolated.
    EXPECT_EQ(1, g.connectedTargets());
    EXPECT_TRUE(!g.isSolved());
    EXPECT_EQ((int)ElecOutcome::InProgress, (int)g.outcome());
}


UTEST(cae, elec_locked_junction_cannot_be_rotated) {
    MiniGameElectronics g;
    g.start(Difficulty::Level1);
    fillElec(g, STRAIGHT);

    g.setJunction(0, 0, STRAIGHT);
    g.setLocked(0, 0, true);
    const int before = g.rotationsLeft();
    g.rotateCursor();  // cursor starts at (0,0)
    EXPECT_EQ((int)STRAIGHT, (int)g.junctionAt(0, 0));
    EXPECT_EQ(before, g.rotationsLeft());  // a no-op must not consume a turn

    g.setLocked(0, 0, false);
    g.rotateCursor();
    EXPECT_EQ((int)CROSS, (int)g.junctionAt(0, 0));
    EXPECT_EQ(before - 1, g.rotationsLeft());
}


UTEST(cae, elec_out_of_turns_is_alarm) {
    MiniGameElectronics g;
    g.start(Difficulty::Level1);
    fillElec(g, STRAIGHT);

    // Rotating an isolated cell can never solve the board, so the budget will
    // run out and the outcome becomes Alarm.
    int guard = 0;
    while (g.outcome() == ElecOutcome::InProgress && guard++ < 10000) {
        g.rotateCursor();
    }
    EXPECT_EQ((int)ElecOutcome::Alarm, (int)g.outcome());
}


UTEST(cae, elec_solving_finishes_with_phone_bugged) {
    gameState = std::make_unique<GameState>();
    gameState->currentScreen = GameScreen::MiniGameElecStart;
    gameState->getNextScreen();  // enter the minigame
    EXPECT_TRUE(gameState->inMiniGame);

    MiniGameElectronics* mg = gameState->electronics();
    ASSERT_TRUE(mg != nullptr);
    fillElec(*mg, CROSS);
    mg->setLocked(0, 0, true);  // rotate becomes a no-op; the board stays solved

    gameState->miniGameRotate();
    EXPECT_TRUE(!gameState->inMiniGame);
    EXPECT_EQ((int)GameScreen::ElecModalBugged, (int)gameState->currentScreen);
}


UTEST(cae, elec_running_out_of_turns_finishes_with_alarm) {
    gameState = std::make_unique<GameState>();
    gameState->currentScreen = GameScreen::MiniGameElecStart;
    gameState->getNextScreen();
    ASSERT_TRUE(gameState->electronics() != nullptr);
    fillElec(*gameState->electronics(), STRAIGHT);

    int guard = 0;
    while (gameState->electronics()->outcome() == ElecOutcome::InProgress && guard++ < 10000) {
        gameState->miniGameRotate();
    }
    EXPECT_TRUE(!gameState->inMiniGame);
    EXPECT_EQ((int)GameScreen::ElecModalAlarm, (int)gameState->currentScreen);
}


// ---------------------------------------------------------------------------
// Cryptography minigame
//
// A random substitution cipher encrypts a message; the player maps cipher
// letters to plaintext letters until the whole message decodes.
// ---------------------------------------------------------------------------

#include "../src/minigame/cryptography.h"

UTEST(cae, crypto_key_is_a_permutation_with_inverse) {
    MiniGameCryptography g;
    g.start(Difficulty::Level1);

    // The cipher must be a permutation of the alphabet...
    int seen[26] = { 0 };
    for (int i = 0; i < 26; ++i) {
        const char c = g.cipherLetter(i);
        EXPECT_TRUE(c >= 'A' && c <= 'Z');
        seen[c - 'A']++;
    }
    for (int i = 0; i < 26; ++i)  { EXPECT_EQ(1, seen[i]); }

    // ...and decipher_key must invert cipher_key.
    for (int i = 0; i < 26; ++i) {
        EXPECT_EQ((int)('A' + i), (int)g.decipherLetter(g.cipherLetter(i) - 'A'));
    }
}


UTEST(cae, crypto_is_solved_only_when_every_letter_is_mapped) {
    MiniGameCryptography g;
    g.start(Difficulty::Level1);

    EXPECT_TRUE(!g.isSolved());
    EXPECT_TRUE(g.correctCount() < 26);

    for (int i = 0; i < 26; ++i) {
        g.setUserLetter(i, g.decipherLetter(i));
    }
    EXPECT_TRUE(g.isSolved());
    EXPECT_EQ(26, g.correctCount());
}


UTEST(cae, crypto_cycle_and_cursor) {
    MiniGameCryptography g;
    g.start(Difficulty::Level1);

    const int start = g.cursor();
    g.moveCursor(1, 0);
    EXPECT_EQ((start + 1) % 26, g.cursor());
    g.moveCursor(-1, 0);
    EXPECT_EQ(start, g.cursor());

    // The guess cycles ' ' -> 'A' .. -> 'Z' -> ' '.
    g.setUserLetter(g.cursor(), ' ');
    g.cycleLetter();
    EXPECT_EQ((int)'A', (int)g.userLetter(g.cursor()));
    g.setUserLetter(g.cursor(), 'Z');
    g.cycleLetter();
    EXPECT_EQ((int)' ', (int)g.userLetter(g.cursor()));
}


UTEST(cae, crypto_solving_finishes_and_returns_to_practice) {
    gameState = std::make_unique<GameState>();
    gameState->currentScreen = GameScreen::MiniGameCryptoStart;
    gameState->getNextScreen();
    ASSERT_TRUE(gameState->cryptography() != nullptr);

    MiniGameCryptography* mg = gameState->cryptography();
    // Map every letter except the one under the cursor; cycling will finish it.
    const int cur = mg->cursor();
    for (int i = 0; i < 26; ++i) {
        if (i != cur)  { mg->setUserLetter(i, mg->decipherLetter(i)); }
    }

    int guard = 0;
    while (gameState->inMiniGame && guard++ < 64) {
        gameState->miniGameRotate();
    }
    EXPECT_TRUE(!gameState->inMiniGame);
    EXPECT_EQ((int)GameScreen::SkillPractice, (int)gameState->currentScreen);
}


#include "../src/gameplay/casefile.h"

#include <vector>

namespace {

void putU16(std::vector<uint8_t>& b, uint16_t value) {
    b.push_back(static_cast<uint8_t>(value & 0xFF));
    b.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void putFixed(std::vector<uint8_t>& b, const std::string& s, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
        b.push_back(i < s.size() ? static_cast<uint8_t>(s[i]) : 0);
    }
}

void putParticipant(std::vector<uint8_t>& b, const std::string& role,
                    uint8_t exposure, uint8_t mastermind, uint8_t clue, uint8_t rank) {
    putU16(b, 0xFFFF);
    b.push_back(exposure);
    b.push_back(0);
    putFixed(b, role, 32);
    const uint8_t trailer[12] = {0, 0, 0, mastermind, 0, 0, clue, 0, rank, 0, 0, 0};
    b.insert(b.end(), trailer, trailer + 12);
}

void putEvent(std::vector<uint8_t>& b, uint16_t who, uint16_t message, const std::string& text,
              uint8_t target, uint8_t type, uint8_t received, uint8_t destroyed, uint16_t value) {
    putU16(b, who);
    putU16(b, 0);
    putU16(b, message);
    putFixed(b, text, 32);
    b.push_back(target);
    b.push_back(type);
    b.push_back(received);
    b.push_back(destroyed);
    putU16(b, value);
}

void putObject(std::vector<uint8_t>& b, const std::string& name, uint8_t icon) {
    putFixed(b, name, 16);
    b.push_back(icon);
    b.push_back(0xFF);
}

// A minimal, well-formed crime file built from synthetic data (no game data is
// required, so the test stays hermetic).
std::vector<uint8_t> sampleCrimeFile() {
    std::vector<uint8_t> b;
    putU16(b, 2);  // participants
    putU16(b, 3);  // events (two real + one terminator)
    putParticipant(b, "Alpha", 5, 0, 1, 4);
    putParticipant(b, "Beta", 0, 1, 7, 7);
    putEvent(b, 0, 42, "Sent item", 1, 2, 4, 0, 7);
    putEvent(b, 1, 43, "Met contact", 0, 8, 0, 0, 0);
    putEvent(b, 0x00FF, 0, "", 0, 0, 0, 0, 0);  // blank terminator (link = 255)
    putObject(b, "Item", 9);
    putObject(b, "Prisoner", 7);
    putObject(b, "", 0xFF);  // empty slot
    putObject(b, "", 0xFF);  // empty slot
    return b;
}

}  // namespace


UTEST(cae, parse_case_file_reads_the_verified_layout) {
    const std::vector<uint8_t> bytes = sampleCrimeFile();
    // 4 + 2*48 + 3*44 + 4*18 = 304.
    EXPECT_EQ((std::size_t)304, bytes.size());

    CaseFile file;
    ASSERT_TRUE(parseCaseFile(bytes.data(), bytes.size(), file));

    ASSERT_EQ((std::size_t)2, file.participants.size());
    EXPECT_TRUE(file.participants[0].role == "Alpha");
    EXPECT_EQ((int)5, (int)file.participants[0].exposure);
    EXPECT_TRUE(!file.participants[0].mastermind);
    EXPECT_EQ((int)1, (int)file.participants[0].clue_type);
    EXPECT_EQ((int)4, (int)file.participants[0].rank);

    EXPECT_TRUE(file.participants[1].role == "Beta");
    EXPECT_EQ((int)0, (int)file.participants[1].exposure);
    EXPECT_TRUE(file.participants[1].mastermind);
    EXPECT_EQ((int)7, (int)file.participants[1].clue_type);
    EXPECT_EQ((int)7, (int)file.participants[1].rank);

    // The blank terminator is not returned as an event.
    ASSERT_EQ((std::size_t)2, file.events.size());
    EXPECT_EQ(0, file.events[0].participant);
    EXPECT_EQ(42, file.events[0].message_id);
    EXPECT_TRUE(file.events[0].text == "Sent item");
    EXPECT_EQ(1, file.events[0].target);
    EXPECT_EQ((int)2, (int)file.events[0].type);
    EXPECT_EQ((int)4, (int)file.events[0].received_mask);
    EXPECT_EQ(7, file.events[0].value);
    EXPECT_EQ(1, file.events[1].participant);
    EXPECT_EQ(43, file.events[1].message_id);
    EXPECT_EQ((int)8, (int)file.events[1].type);

    // Empty object slots are dropped.
    ASSERT_EQ((std::size_t)2, file.objects.size());
    EXPECT_TRUE(file.objects[0].name == "Item");
    EXPECT_EQ((int)9, (int)file.objects[0].icon);
    EXPECT_TRUE(file.objects[1].name == "Prisoner");
    EXPECT_EQ((int)7, (int)file.objects[1].icon);
}


UTEST(cae, parse_case_file_rejects_bad_input) {
    CaseFile file;

    EXPECT_TRUE(!parseCaseFile(nullptr, 0, file));

    const uint8_t too_short[] = {0x01, 0x00};
    EXPECT_TRUE(!parseCaseFile(too_short, sizeof(too_short), file));

    // The header claims one participant and one event, but there is no room.
    const uint8_t bogus[] = {0x01, 0x00, 0x01, 0x00, 0x00, 0x00};
    EXPECT_TRUE(!parseCaseFile(bogus, sizeof(bogus), file));

    // A buffer truncated inside the participant region must be rejected.
    std::vector<uint8_t> truncated = sampleCrimeFile();
    truncated.resize(100);
    EXPECT_TRUE(!parseCaseFile(truncated.data(), truncated.size(), file));
}


UTEST(cae, load_case_file_handles_missing_files) {
    CaseFile file;
    EXPECT_TRUE(!loadCaseFile("this_file_does_not_exist.dta", file));
}


#include "../src/gui/facegen.h"

#include <algorithm>
#include <set>

UTEST(cae, facegen_compose_has_expected_shape) {
    const facegen::Image img = facegen::compose(facegen::playerFace());
    EXPECT_EQ(facegen::kWidth, img.width);
    EXPECT_EQ(facegen::kHeight, img.height);
    EXPECT_EQ((std::size_t)(facegen::kWidth * facegen::kHeight), img.indices.size());

    const std::size_t pal_size = facegen::palette().size();
    ASSERT_TRUE(pal_size > 1);
    // Every produced index must address the palette.
    for (uint8_t idx : img.indices) {
        EXPECT_TRUE((std::size_t)idx < pal_size);
    }
    // Index 0 is the transparent background.
    EXPECT_EQ((unsigned)0, (unsigned)facegen::palette()[0].a);
}

UTEST(cae, facegen_is_deterministic) {
    const Face a = facegen::makeFace(12345);
    const Face b = facegen::makeFace(12345);
    EXPECT_EQ((int)a.gender_id, (int)b.gender_id);
    EXPECT_EQ((int)a.hair_id, (int)b.hair_id);
    EXPECT_EQ((int)a.eye_id, (int)b.eye_id);
    EXPECT_EQ((int)a.skin_id, (int)b.skin_id);
    EXPECT_EQ((int)a.clothing_id, (int)b.clothing_id);
    EXPECT_EQ((int)a.accessory_id, (int)b.accessory_id);

    EXPECT_TRUE(facegen::compose(a).indices == facegen::compose(b).indices);
    EXPECT_TRUE(facegen::candidateFace(3).hair_id == facegen::candidateFace(3).hair_id);
}

UTEST(cae, facegen_different_seeds_differ) {
    // Not every pair must differ, but a run of distinct seeds should not all
    // collapse to the same face.
    std::set<std::vector<uint8_t>> seen;
    for (uint32_t s = 0; s < 24; ++s) {
        seen.insert(facegen::compose(facegen::makeFace(s)).indices);
    }
    EXPECT_TRUE(seen.size() >= 12);
}

UTEST(cae, facegen_composes_a_visible_face) {
    const facegen::Image img = facegen::compose(facegen::chiefFace());
    // The middle of the canvas must be part of the face (not transparent).
    const std::size_t mid = (std::size_t)(facegen::kHeight / 2) * img.width + (facegen::kWidth / 2);
    EXPECT_TRUE(img.indices[mid] != 0);

    // A portrait is opaque somewhere: count the non-transparent pixels.
    const std::size_t drawn =
        std::count_if(img.indices.begin(), img.indices.end(), [](uint8_t i) { return i != 0; });
    EXPECT_TRUE(drawn > (std::size_t)(facegen::kWidth * facegen::kHeight) / 4);
}

UTEST(cae, facegen_tolerates_any_component_ids) {
    // Component ids come from data files and may be anything; out-of-range
    // values must still reduce cleanly rather than read past the palette.
    Face f{};
    f.gender_id = 0xFF;
    f.hair_id = 0xFF;
    f.eye_id = 0xFF;
    f.skin_id = 0xFF;
    f.clothing_id = 0xFF;
    f.accessory_id = 0xFF;

    const facegen::Image img = facegen::compose(f);
    const std::size_t pal_size = facegen::palette().size();
    for (uint8_t idx : img.indices) {
        EXPECT_TRUE((std::size_t)idx < pal_size);
    }
}

UTEST(cae, facegen_candidate_faces_are_distinct) {
    // The 15-slot lineup should show 15 different people.
    std::set<std::vector<uint8_t>> seen;
    for (int i = 0; i < 15; ++i) {
        seen.insert(facegen::compose(facegen::candidateFace(i)).indices);
    }
    EXPECT_EQ((std::size_t)15, seen.size());
}

namespace {
CaseParticipant makeParticipant(const std::string& role, uint8_t rank = 0,
                                uint8_t exposure = 0, uint8_t clue = 0,
                                bool mastermind = false) {
    CaseParticipant p;
    p.role = role;
    p.rank = rank;
    p.exposure = exposure;
    p.clue_type = clue;
    p.mastermind = mastermind;
    return p;
}
}  // namespace

UTEST(cae, facegen_participant_faces_are_stable) {
    // The same participant must always look the same, so a suspect stays
    // recognisable across the whole case.
    const CaseParticipant p = makeParticipant("Anton Keller", 4, 3, 2);
    const Face a = facegen::makeFaceFromParticipant(p);
    const Face b = facegen::makeFaceFromParticipant(p);
    EXPECT_EQ((int)a.gender_id, (int)b.gender_id);
    EXPECT_EQ((int)a.hair_id, (int)b.hair_id);
    EXPECT_EQ((int)a.eye_id, (int)b.eye_id);
    EXPECT_EQ((int)a.skin_id, (int)b.skin_id);
    EXPECT_EQ((int)a.clothing_id, (int)b.clothing_id);
    EXPECT_EQ((int)a.accessory_id, (int)b.accessory_id);
    EXPECT_TRUE(facegen::compose(a).indices == facegen::compose(b).indices);
}

UTEST(cae, facegen_participant_identities_differ) {
    // Different people should not collapse to the same portrait.
    std::set<std::vector<uint8_t>> seen;
    for (int i = 0; i < 16; ++i) {
        const CaseParticipant p = makeParticipant("Agent " + std::to_string(i));
        seen.insert(facegen::compose(facegen::makeFaceFromParticipant(p)).indices);
    }
    EXPECT_TRUE(seen.size() >= 14);
}

UTEST(cae, facegen_participant_field_changes_face) {
    // The plain look and the "weighted" look (rank/exposure/clue) must not be
    // identical, so those fields visibly matter.
    const CaseParticipant plain = makeParticipant("Maria Voss");
    const CaseParticipant seasoned = makeParticipant("Maria Voss", 8, 12, 5);
    EXPECT_TRUE(facegen::compose(facegen::makeFaceFromParticipant(plain)).indices !=
                facegen::compose(facegen::makeFaceFromParticipant(seasoned)).indices);
}

UTEST(cae, facegen_participant_mastermind_stands_out) {
    // The mastermind flag must give a visibly distinctive face.
    const CaseParticipant minion = makeParticipant("Otto Brandt", 3, 0, 1, false);
    const CaseParticipant boss = makeParticipant("Otto Brandt", 3, 0, 1, true);
    EXPECT_TRUE(facegen::compose(facegen::makeFaceFromParticipant(minion)).indices !=
                facegen::compose(facegen::makeFaceFromParticipant(boss)).indices);
}

UTEST(cae, facegen_participant_stays_in_range) {
    // Data files can carry any byte value; the produced components must still
    // reduce cleanly and address the palette.
    const CaseParticipant p = makeParticipant("J. Random Participant With A Long Name",
                                              0xFF, 0xFF, 0xFF, true);
    const Face f = facegen::makeFaceFromParticipant(p);
    EXPECT_TRUE(f.gender_id < 2);
    EXPECT_TRUE(f.hair_id < 60);
    EXPECT_TRUE(f.eye_id < 6);
    EXPECT_TRUE(f.skin_id < 4);
    EXPECT_TRUE(f.clothing_id < 3);
    EXPECT_TRUE(f.accessory_id < 16);

    const facegen::Image img = facegen::compose(f);
    const std::size_t pal_size = facegen::palette().size();
    for (uint8_t idx : img.indices) {
        EXPECT_TRUE((std::size_t)idx < pal_size);
    }
}


// ---------------------------------------------------------------------------
// P0: shared random generator
//
// Every subsystem must draw from one seeded engine so a run is reproducible.
// ---------------------------------------------------------------------------

#include "../src/random.h"

UTEST(cae, random_shared_generator_is_seeded_and_reproducible) {
    cae::seedRng(1234);
    const int a[8] = { cae::randInt(0, 100000), cae::randInt(0, 100000),
                       cae::randInt(0, 100000), cae::randInt(0, 100000),
                       cae::randInt(0, 100000), cae::randInt(0, 100000),
                       cae::randInt(0, 100000), cae::randInt(0, 100000) };

    // Reseeding with the same seed reproduces the same sequence.
    cae::seedRng(1234);
    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(a[i], cae::randInt(0, 100000));
    }

    // A different seed produces a different sequence (extremely unlikely to
    // collide across all eight draws).
    cae::seedRng(5678);
    bool differs = false;
    for (int i = 0; i < 8; ++i) {
        if (cae::randInt(0, 100000) != a[i])  { differs = true; }
    }
    EXPECT_TRUE(differs);
}


UTEST(cae, random_helpers_respect_their_bounds) {
    for (int i = 0; i < 256; ++i) {
        const int p = cae::randPct();
        EXPECT_TRUE(p >= 0 && p <= 99);
        const int v = cae::randInt(-3, 7);
        EXPECT_TRUE(v >= -3 && v <= 7);
        // Arguments are swapped rather than producing an empty range.
        const int w = cae::randInt(7, -3);
        EXPECT_TRUE(w >= -3 && w <= 7);
    }
}


// ---------------------------------------------------------------------------
// P0: MiniGame base — difficulty plumbing and outcome plumbing
// ---------------------------------------------------------------------------

#include "../src/minigame/combat.h"
#include "../src/minigame/driving.h"

UTEST(cae, minigame_difficulty_is_plumbed_and_reported) {
    // Every minigame records the difficulty passed to start() and reports it
    // back (previously each returned a hard-coded Level1).
    MiniGameElectronics e;
    e.start(Difficulty::Level3);
    EXPECT_EQ((int)Difficulty::Level3, (int)e.getGameDifficulty());

    MiniGameCryptography c;
    c.start(Difficulty::Level4);
    EXPECT_EQ((int)Difficulty::Level4, (int)c.getGameDifficulty());

    MiniGameDriving d;  // still a stub (P1) but must record difficulty
    d.start(Difficulty::Level2);
    EXPECT_EQ((int)Difficulty::Level2, (int)d.getGameDifficulty());

    MiniGameCombat::Instance().start(Difficulty::Level1);
    EXPECT_EQ((int)Difficulty::Level1, (int)MiniGameCombat::Instance().getGameDifficulty());
}


UTEST(cae, minigame_begin_resets_finished_state_on_restart) {
    MiniGameElectronics g;
    g.start(Difficulty::Level1);

    // Solve the board, then confirm the round reports a finished outcome.
    fillElec(g, CROSS);
    g.setLocked(0, 0, true);  // rotate is a no-op, so the board stays solved
    g.onActivate();
    EXPECT_TRUE(g.isFinished());
    EXPECT_EQ((int)ScreenExitCondition::PhoneBugged, (int)g.result());

    // Starting a new round must clear the previous outcome and step counter.
    g.start(Difficulty::Level2);
    EXPECT_TRUE(!g.isFinished());
    EXPECT_EQ((int)ScreenExitCondition::None, (int)g.result());
    EXPECT_EQ((int)Difficulty::Level2, (int)g.getGameDifficulty());
    EXPECT_EQ(0u, g.timer());
}


UTEST(cae, minigame_active_instance_and_input_routing) {
    gameState = std::make_unique<GameState>();
    gameState->initMiniGameClasses();
    EXPECT_TRUE(gameState->activeMiniGame() == nullptr);  // nothing running yet

    gameState->currentScreen = GameScreen::MiniGameElecStart;
    gameState->getNextScreen();  // enter the electronics minigame
    ASSERT_TRUE(gameState->activeMiniGame() == gameState->electronics());

    // Input must reach the active game and advance its step counter.
    MiniGame* mg = gameState->activeMiniGame();
    const std::uint32_t before = mg->timer();
    gameState->miniGameMove(1, 0);
    EXPECT_TRUE(mg->timer() > before);

    gameState->miniGameCancel();
    EXPECT_TRUE(gameState->activeMiniGame() == nullptr);
}


// ---------------------------------------------------------------------------
// P0: FSM reconciliation — a minigame outcome that names no specific screen
// must fall back to the city rather than leaving a stale screen behind.
// ---------------------------------------------------------------------------

UTEST(cae, fsm_minigame_unmapped_condition_falls_back_to_city) {
    gameState = std::make_unique<GameState>();
    gameState->inMiniGame = true;
    gameState->currentMiniGame = MiniGameType::Driving;
    gameState->getNextScreen(ScreenExitCondition::None);  // e.g. leaving a chase
    CAE_EXPECT_SCREEN(GameScreen::CityMainMenu);

    gameState->currentMiniGame = MiniGameType::Elec;
    gameState->getNextScreen(ScreenExitCondition::Win);  // unmapped for elec
    CAE_EXPECT_SCREEN(GameScreen::CityMainMenu);
}


// ---------------------------------------------------------------------------
// P0: save/load carries the full session state
// ---------------------------------------------------------------------------

UTEST(cae, save_game_round_trips_session_fields) {
    const std::string path = "cae_save_session.tmp";

    SaveGame in;
    in.callsign = "Session";
    in.screen = (int)GameScreen::ReviewClues;
    in.orig_screen = (int)OriginScreen::Data;
    in.modal_sequence = 2;
    in.is_twelve = 0;
    in.city = "Cairo";
    in.difficulty = (int)Difficulty::Level2;

    ASSERT_TRUE(writeSaveGame(path, in));
    SaveGame out;
    ASSERT_TRUE(readSaveGame(path, out));
    EXPECT_EQ(in.orig_screen, out.orig_screen);
    EXPECT_EQ(in.modal_sequence, out.modal_sequence);
    EXPECT_EQ(in.is_twelve, out.is_twelve);
    EXPECT_TRUE(in.city == out.city);

    std::remove(path.c_str());
}


UTEST(cae, save_and_load_restores_full_session_state) {
    mainChar = std::make_unique<MainChar>();
    mainChar->setCallsign("Deep");
    gameState = std::make_unique<GameState>();
    gameState->currentScreen = GameScreen::ReviewOrg;
    gameState->origScreen = OriginScreen::Data;
    gameState->modalSequence = 3;
    gameState->isTwelve = false;
    gameState->currentCity = "Santiago";
    gameState->setGameDifficulty(Difficulty::Level4);

    ASSERT_EQ((int)FileStatus::Ok, (int)save_game_slot(4));

    // Wipe the session, then load it back.
    mainChar = std::make_unique<MainChar>();
    gameState = std::make_unique<GameState>();
    ASSERT_EQ((int)FileStatus::Ok, (int)load_game(4));

    EXPECT_TRUE(gameState->currentCity == "Santiago");
    CAE_EXPECT_SCREEN(GameScreen::ReviewOrg);
    EXPECT_EQ((int)OriginScreen::Data, (int)gameState->origScreen);
    EXPECT_EQ(3, gameState->modalSequence);
    EXPECT_TRUE(!gameState->isTwelve);
    EXPECT_EQ((int)Difficulty::Level4, (int)gameState->getGameDifficulty());

    std::remove("CV4.SVE");
}


