#pragma once

#include "config.hpp"
#include "audio_capture.hpp"
#include "transcriber.hpp"
#include "hotkey_manager.hpp"
#include "clipboard.hpp"
#include "audio_processor.hpp"

#include <memory>
#include <atomic>
#include <string>
#include <chrono>

namespace whispr {

enum class AppState {
    Idle,
    Recording,
    Transcribing,
    Error
};

class App {
public:
    App();
    ~App();

    // Initialize all components
    bool initialize(const Config& config);
    void shutdown();

    // Run the application (blocking)
    int run();

    // Stop the application
    void quit() { should_quit_.store(true); }

    // Get current state
    AppState state() const { return state_.load(); }

    // Manual control (for menu bar actions)
    void start_recording();
    void stop_recording();

private:
    void on_hotkey(bool pressed);
    void on_transcription_complete(const std::string& text);

    Config config_;
    std::unique_ptr<AudioCapture> audio_;
    std::unique_ptr<Transcriber> transcriber_;
    std::unique_ptr<HotkeyManager> hotkey_;
    std::unique_ptr<AudioProcessor> audio_processor_;

    std::atomic<AppState> state_{AppState::Idle};
    std::atomic<bool> should_quit_{false};

    // Timestamp of last recording end (for cooldown)
    std::chrono::steady_clock::time_point last_recording_end_;
};

// Platform-specific tray icon
bool create_tray_icon(App* app);
void destroy_tray_icon();
void update_tray_state(AppState state);

} // namespace whispr
