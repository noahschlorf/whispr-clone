#pragma once

#include <string>
#include <cstdint>

namespace whispr {

struct Config {
    // Audio settings
    int sample_rate = 16000;        // Whisper expects 16kHz
    int channels = 1;               // Mono
    int frames_per_buffer = 512;    // Low latency buffer

    // Whisper model
    std::string model_path = "models/ggml-base.en.bin";
    int n_threads = 4;              // CPU threads for inference

    // Hotkey (default: Right Option/Alt key)
    uint32_t hotkey_keycode = 0;    // Platform-specific
    uint32_t hotkey_modifiers = 0;

    // Behavior
    bool auto_paste = true;
    bool play_sound = false;
    int max_recording_seconds = 30;

    // Performance
    bool use_gpu = false;           // Metal/CUDA acceleration
    int beam_size = 1;              // Greedy decoding for speed
    bool translate = false;         // Just transcribe, don't translate
    std::string language = "en";    // English
};

// Default hotkey codes
#ifdef PLATFORM_MACOS
    constexpr uint32_t DEFAULT_HOTKEY = 61;  // Right Option key
#elif PLATFORM_LINUX
    constexpr uint32_t DEFAULT_HOTKEY = 108; // Right Alt key
#endif

} // namespace whispr
