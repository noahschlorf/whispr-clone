#include "clipboard.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <array>
#include <memory>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/XTest.h>

namespace whispr {

// Helper to run a command and get output
static std::string exec_command(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

bool Clipboard::set_text(const std::string& text) {
    // Use xclip or xsel for clipboard
    // First try xclip
    FILE* pipe = popen("xclip -selection clipboard 2>/dev/null", "w");
    if (pipe) {
        fwrite(text.c_str(), 1, text.length(), pipe);
        int ret = pclose(pipe);
        if (ret == 0) return true;
    }

    // Try xsel
    pipe = popen("xsel --clipboard --input 2>/dev/null", "w");
    if (pipe) {
        fwrite(text.c_str(), 1, text.length(), pipe);
        int ret = pclose(pipe);
        if (ret == 0) return true;
    }

    std::cerr << "Failed to set clipboard. Install xclip or xsel." << std::endl;
    return false;
}

std::string Clipboard::get_text() {
    // Try xclip first
    std::string result = exec_command("xclip -selection clipboard -o 2>/dev/null");
    if (!result.empty()) return result;

    // Try xsel
    result = exec_command("xsel --clipboard --output 2>/dev/null");
    return result;
}

bool Clipboard::paste() {
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Failed to open X display" << std::endl;
        return false;
    }

    // Simulate Ctrl+V
    KeyCode ctrl_keycode = XKeysymToKeycode(display, XK_Control_L);
    KeyCode v_keycode = XKeysymToKeycode(display, XK_v);

    if (ctrl_keycode == 0 || v_keycode == 0) {
        std::cerr << "Failed to get keycodes" << std::endl;
        XCloseDisplay(display);
        return false;
    }

    // Press Ctrl
    XTestFakeKeyEvent(display, ctrl_keycode, True, 0);
    XFlush(display);

    // Press V
    XTestFakeKeyEvent(display, v_keycode, True, 0);
    XFlush(display);

    // Release V
    XTestFakeKeyEvent(display, v_keycode, False, 0);
    XFlush(display);

    // Release Ctrl
    XTestFakeKeyEvent(display, ctrl_keycode, False, 0);
    XFlush(display);

    XCloseDisplay(display);
    return true;
}

} // namespace whispr
