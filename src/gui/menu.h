#pragma once

#include <string>
#include <vector>

/* TODO: add menu defs, font, keyboard handler, anims, etc. */

struct MenuChoice {
    int id { 0 };
    bool selected { false };
    std::string txt;
    int prev { 0 };
    int next { 0 };
};

std::vector<MenuChoice> buildMenu(std::vector<std::string> choice_strings);
