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


#ifndef NO_SDL
UTEST(cae, disk_load_game_rejects_out_of_range_slot) {
    // Slots are 0..4. Out-of-range indices used to leave a wild pointer to be
    // handed to SDL_RWFromFile; they must instead report NotFound.
    EXPECT_EQ((int)FileStatus::NotFound, (int)load_game(5));
    EXPECT_EQ((int)FileStatus::NotFound, (int)load_game(100));
    EXPECT_EQ((int)FileStatus::NotFound, (int)load_game(-1));
}
#endif


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

    gameState = std::make_unique<GameState>();
    gameState->currentScreen = GameScreen::Building2;
    gameState->selectedChoice = 4;
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

