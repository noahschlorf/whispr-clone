#include "app.hpp"
#include <iostream>
#include <thread>
#include <chrono>

#ifdef PLATFORM_MACOS
extern "C" void run_macos_event_loop();
#endif

namespace whispr {

// Minimum time between recordings (ms) to prevent rapid re-recording glitches
static constexpr int64_t MIN_RECORDING_INTERVAL_MS = 100;

App::App() = default;

App::~App() {
    shutdown();
}

bool App::initialize(const Config& config) {
    config_ = config;

    // Initialize audio capture
    audio_ = std::make_unique<AudioCapture>(
        config_.sample_rate,
        config_.channels,
        config_.frames_per_buffer
    );

    if (!audio_->initialize()) {
        std::cerr << "Failed to initialize audio capture" << std::endl;
        return false;
    }
    std::cout << "Audio capture initialized" << std::endl;

    // Initialize audio processor if enabled
    if (config_.audio_preprocessing) {
        audio_processor_ = std::make_unique<AudioProcessor>(
            static_cast<float>(config_.sample_rate)
        );
        std::cout << "Audio preprocessing enabled" << std::endl;
    }

    // Initialize transcriber
    transcriber_ = std::make_unique<Transcriber>();
    if (!transcriber_->initialize(config_.get_model_path(), config_.n_threads)) {
        std::cerr << "Failed to initialize transcriber" << std::endl;
        return false;
    }
    transcriber_->set_language(config_.language);
    transcriber_->set_translate(config_.translate);
    transcriber_->set_profile(get_profile(config_.model_quality));
    if (!config_.initial_prompt.empty()) {
        transcriber_->set_initial_prompt(config_.initial_prompt);
    }
    std::cout << "Transcriber initialized (quality: " << get_profile(config_.model_quality).name << ")" << std::endl;

    // Initialize hotkey manager
    hotkey_ = std::make_unique<HotkeyManager>();
    if (!hotkey_->initialize()) {
        std::cerr << "Failed to initialize hotkey manager" << std::endl;
        return false;
    }

    // Set default hotkey if not specified
    uint32_t keycode = config_.hotkey_keycode;
    if (keycode == 0) {
#ifdef PLATFORM_MACOS
        keycode = 61;  // Right Option
#elif PLATFORM_LINUX
        keycode = 108; // Right Alt
#endif
    }
    hotkey_->set_hotkey(keycode, config_.hotkey_modifiers);
    hotkey_->set_callback([this](bool pressed) { on_hotkey(pressed); });
    std::cout << "Hotkey manager initialized" << std::endl;

    // Create tray icon
    if (!create_tray_icon(this)) {
        std::cerr << "Failed to create tray icon" << std::endl;
        // Continue anyway - not critical
    }

    state_.store(AppState::Idle);
    return true;
}

void App::shutdown() {
    should_quit_.store(true);

    if (hotkey_) {
        hotkey_->stop();
        hotkey_.reset();
    }

    if (audio_) {
        audio_->shutdown();
        audio_.reset();
    }

    if (transcriber_) {
        transcriber_->shutdown();
        transcriber_.reset();
    }

    destroy_tray_icon();
}

int App::run() {
    if (!hotkey_->start()) {
        std::cerr << "Failed to start hotkey listener" << std::endl;
        return 1;
    }

    std::cout << "\n=== VoxType Ready ===" << std::endl;
    std::cout << "Hold the hotkey to record, release to transcribe and paste." << std::endl;
    std::cout << "Menu bar icon should appear in your menu bar.\n" << std::endl;

#ifdef PLATFORM_MACOS
    // On macOS, run the NSApplication event loop
    run_macos_event_loop();
#else
    // Main loop for other platforms
    while (!should_quit_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
#endif

    return 0;
}

void App::on_hotkey(bool pressed) {
    // Skip hotkey if disabled
    if (!enabled_.load()) {
        return;
    }

    if (pressed) {
        start_recording();
    } else {
        stop_recording();
    }
}

void App::start_recording() {
    if (state_.load() != AppState::Idle) return;

    // Check cooldown to prevent rapid re-recording glitches
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_recording_end_).count();
    if (elapsed < MIN_RECORDING_INTERVAL_MS && last_recording_end_.time_since_epoch().count() > 0) {
        return;  // Too soon after last recording
    }

    std::cout << "Recording..." << std::endl;
    state_.store(AppState::Recording);
    update_tray_state(AppState::Recording);

    audio_->start_recording();
}

void App::stop_recording() {
    if (state_.load() != AppState::Recording) return;

    audio_->stop_recording();
    state_.store(AppState::Transcribing);
    update_tray_state(AppState::Transcribing);

    std::cout << "Transcribing..." << std::endl;

    // Get recorded audio
    auto audio_data = audio_->get_recorded_audio();

    if (audio_data.empty()) {
        std::cerr << "No audio recorded" << std::endl;
        state_.store(AppState::Idle);
        update_tray_state(AppState::Idle);
        return;
    }

    // Preprocess audio if enabled
    if (audio_processor_) {
        audio_processor_->process(audio_data);
        audio_processor_->reset();  // Reset filter state for next recording
    }

    // Transcribe (use adaptive mode if enabled)
    TranscriptionResult result;
    if (config_.adaptive_quality) {
        result = transcriber_->transcribe_adaptive(audio_data);
    } else {
        result = transcriber_->transcribe(audio_data);
    }

    if (result.success && !result.text.empty()) {
        on_transcription_complete(result.text);
    } else if (!result.success) {
        std::cerr << "Transcription failed: " << result.error << std::endl;
    }

    // Record timestamp for cooldown check
    last_recording_end_ = std::chrono::steady_clock::now();

    state_.store(AppState::Idle);
    update_tray_state(AppState::Idle);
}

void App::on_transcription_complete(const std::string& text) {
    // Add to history for menu bar display
    add_to_history(text);

    if (config_.auto_paste) {
        // Set clipboard and paste
        if (Clipboard::set_text(text)) {
            // Delay to ensure clipboard is fully set before pasting
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            Clipboard::paste();
        } else {
            std::cerr << "Failed to set clipboard" << std::endl;
        }
    } else {
        // Just set clipboard
        Clipboard::set_text(text);
    }
}

} // namespace whispr
