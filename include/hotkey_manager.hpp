#pragma once

#include <functional>
#include <atomic>
#include <thread>
#include <cstdint>

namespace whispr {

class HotkeyManager {
public:
    using HotkeyCallback = std::function<void(bool pressed)>;

    HotkeyManager();
    ~HotkeyManager();

    // Initialize the hotkey system
    bool initialize();
    void shutdown();

    // Set the hotkey (keycode is platform-specific)
    void set_hotkey(uint32_t keycode, uint32_t modifiers = 0);

    // Set callback for key press/release
    void set_callback(HotkeyCallback callback) { callback_ = callback; }

    // Start/stop listening
    bool start();
    void stop();
    bool is_running() const { return running_.load(); }

// Made public for platform-specific access
    uint32_t keycode_ = 0;
    uint32_t modifiers_ = 0;
    HotkeyCallback callback_;

private:
    void run_loop();

    std::atomic<bool> running_{false};
    std::thread listener_thread_;

    // Platform-specific handle
    void* platform_handle_ = nullptr;
};

} // namespace whispr
