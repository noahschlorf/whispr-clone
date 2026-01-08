#include "hotkey_manager.hpp"
#include <iostream>

namespace whispr {

HotkeyManager::HotkeyManager() = default;

HotkeyManager::~HotkeyManager() {
    shutdown();
}

void HotkeyManager::set_hotkey(uint32_t keycode, uint32_t modifiers) {
    keycode_ = keycode;
    modifiers_ = modifiers;
}

// Platform-specific implementations in platform/*/hotkey_*.cpp

} // namespace whispr
