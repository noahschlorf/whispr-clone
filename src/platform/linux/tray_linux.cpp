#include "app.hpp"
#include <iostream>

// Linux tray implementation - basic version without GUI dependencies
// For a full implementation, would use libappindicator or Qt

namespace whispr {

static App* g_app = nullptr;

bool create_tray_icon(App* app) {
    g_app = app;
    // Linux tray icon would require GTK or Qt
    // For now, just print status to console
    std::cout << "Tray icon not implemented on Linux - use console output" << std::endl;
    return true;
}

void destroy_tray_icon() {
    g_app = nullptr;
}

void update_tray_state(AppState state) {
    const char* state_str = "";
    switch (state) {
        case AppState::Idle:
            state_str = "Ready";
            break;
        case AppState::Recording:
            state_str = "Recording...";
            break;
        case AppState::Transcribing:
            state_str = "Transcribing...";
            break;
        case AppState::Error:
            state_str = "Error";
            break;
    }
    std::cout << "[Whispr] " << state_str << std::endl;
}

} // namespace whispr
