#pragma once

#include <string>
#include <cstdint>

namespace whispr {

// Quality modes for accuracy/speed tradeoff
enum class ModelQuality {
    Fast,       // tiny.en - fastest, ~80% accuracy
    Balanced,   // base.en - good balance, ~85% accuracy
    Accurate,   // small.en - high accuracy, ~92% accuracy
    Best        // medium.en - highest accuracy, ~95% accuracy
};

// Transcription parameter profiles
struct TranscriptionProfile {
    int best_of;
    int beam_size;
    float entropy_thold;
    float no_speech_thold;
    float temperature;
    const char* name;
};

// Predefined profiles
// best_of: number of candidates, beam_size: beam search width
// entropy_thold: skip if entropy > threshold, no_speech_thold: skip if no_speech prob > threshold
inline const TranscriptionProfile PROFILE_FAST = {1, 1, 2.4f, 0.6f, 0.0f, "Fast"};
inline const TranscriptionProfile PROFILE_BALANCED = {5, 5, 2.8f, 0.5f, 0.0f, "Balanced"};  // More accurate than before
inline const TranscriptionProfile PROFILE_ACCURATE = {5, 8, 3.0f, 0.4f, 0.0f, "Accurate"};
inline const TranscriptionProfile PROFILE_BEST = {5, 10, 3.0f, 0.35f, 0.0f, "Best"};

// Get profile for quality level
inline const TranscriptionProfile& get_profile(ModelQuality quality) {
    switch (quality) {
        case ModelQuality::Fast: return PROFILE_FAST;
        case ModelQuality::Balanced: return PROFILE_BALANCED;
        case ModelQuality::Accurate: return PROFILE_ACCURATE;
        case ModelQuality::Best: return PROFILE_BEST;
        default: return PROFILE_BALANCED;
    }
}

// Get model filename for quality level
inline std::string get_model_filename(ModelQuality quality) {
    switch (quality) {
        case ModelQuality::Fast: return "ggml-tiny.en.bin";
        case ModelQuality::Balanced: return "ggml-base.en.bin";
        case ModelQuality::Accurate: return "ggml-small.en.bin";
        case ModelQuality::Best: return "ggml-medium.en.bin";
        default: return "ggml-base.en.bin";
    }
}

struct Config {
    // Audio settings
    int sample_rate = 16000;        // Whisper expects 16kHz
    int channels = 1;               // Mono
    int frames_per_buffer = 512;    // Low latency buffer

    // Whisper model
    std::string model_dir = "models";
    ModelQuality model_quality = ModelQuality::Balanced;  // base.en model
    int n_threads = 4;              // CPU threads for inference

    // Get full model path based on quality
    std::string get_model_path() const {
        return model_dir + "/" + get_model_filename(model_quality);
    }

    // Hotkey (default: Right Option/Alt key)
    uint32_t hotkey_keycode = 0;    // Platform-specific
    uint32_t hotkey_modifiers = 0;

    // Behavior
    bool auto_paste = true;
    bool play_sound = false;
    int max_recording_seconds = 30;

    // Performance & Accuracy
    bool use_gpu = true;            // Metal/CUDA acceleration
    bool adaptive_quality = true;   // Auto-retry with higher quality if low confidence
    bool translate = false;         // Just transcribe, don't translate
    std::string language = "en";    // English

    // Audio preprocessing
    bool audio_preprocessing = true;  // Enable noise reduction
    bool trim_silence = true;         // Trim silence from start/end (VAD)
    float silence_threshold = 0.01f;  // Silence detection threshold
    int min_silence_ms = 100;         // Minimum silence duration to trim

    // Initial prompt for context (helps accuracy and vocabulary recognition)
    // Add proper nouns and technical terms you commonly use
    std::string initial_prompt = "The following is a clear transcription of speech. "
                                  "Common terms: Ralph Wiggum, Claude, Anthropic, GitHub, "
                                  "macOS, Python, JavaScript, TypeScript, API.";
};

// Default hotkey codes
#ifdef PLATFORM_MACOS
    constexpr uint32_t DEFAULT_HOTKEY = 61;  // Right Option key
#elif PLATFORM_LINUX
    constexpr uint32_t DEFAULT_HOTKEY = 108; // Right Alt key
#endif

} // namespace whispr
