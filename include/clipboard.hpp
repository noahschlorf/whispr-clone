#pragma once

#include <string>

namespace whispr {

class Clipboard {
public:
    // Set text to clipboard
    static bool set_text(const std::string& text);

    // Get text from clipboard
    static std::string get_text();

    // Paste clipboard content (simulates Cmd+V / Ctrl+V)
    static bool paste();
};

} // namespace whispr
