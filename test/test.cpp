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
