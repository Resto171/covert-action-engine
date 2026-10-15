#include "menu.h"

#include <utility>

std::vector<MenuChoice> buildMenu(std::vector<std::string> choice_strings) {
    std::vector<MenuChoice> menu_constructor;
    menu_constructor.reserve(choice_strings.size());

    for (std::size_t i = 0; i < choice_strings.size(); i++) {
        MenuChoice this_menu_choice;
        this_menu_choice.id = static_cast<int>(i);  // 0-based: matches GameState::selectedChoice
        this_menu_choice.txt = std::move(choice_strings[i]);
        this_menu_choice.selected = (i == 0);  // First choice selected by default
        this_menu_choice.prev = 0;
        this_menu_choice.next = 0;
        menu_constructor.push_back(std::move(this_menu_choice));
    }

    if (menu_constructor.empty()) {
        return menu_constructor;
    }

    const int last = static_cast<int>(menu_constructor.size()) - 1;
    // Link neighbours by index (no raw pointers into storage that may move).
    for (int i = 0; i <= last; i++) {
        menu_constructor[i].prev = (i == 0) ? 0 : i - 1;
        menu_constructor[i].next = (i == last) ? last : i + 1;
    }

    return menu_constructor;
}
