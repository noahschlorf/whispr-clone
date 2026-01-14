#pragma once

#include <string>
#include <cstdint>

namespace whispr {

// ============================================================================
// Named Constants (replace magic numbers)
// ============================================================================

/// Audio settings
constexpr int WHISPER_SAMPLE_RATE = 16000;    ///< Whisper requires 16kHz audio
constexpr int AUDIO_CHANNELS_MONO = 1;         ///< Mono audio channel count
constexpr int DEFAULT_BUFFER_FRAMES = 512;     ///< Low-latency audio buffer size
constexpr int DEFAULT_THREAD_COUNT = 4;        ///< Default CPU threads for inference

/// Recording limits
constexpr int DEFAULT_MAX_RECORDING_SEC = 30;  ///< Maximum recording duration
constexpr int MIN_AUDIO_DURATION_MS = 100;     ///< Minimum audio for transcription

/// VAD (Voice Activity Detection) settings
constexpr float DEFAULT_SILENCE_THRESHOLD = 0.01f;  ///< Silence detection threshold
constexpr int DEFAULT_MIN_SILENCE_MS = 100;         ///< Minimum silence to trim
constexpr int DEFAULT_VAD_PADDING_MS = 50;          ///< Padding around speech

/// Confidence thresholds
constexpr float DEFAULT_CONFIDENCE_THRESHOLD = 0.7f;  ///< Min confidence for adaptive mode

// ============================================================================
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

// Optimized profile based on OpenAI recommendations for maximum accuracy
// Lower no_speech_thold (0.3) = more sensitive to speech
// Higher beam_size (8) = better search but slower
inline const TranscriptionProfile PROFILE_OPTIMIZED = {5, 8, 2.4f, 0.3f, 0.0f, "Optimized"};

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

/// @struct Config
/// @brief Application configuration settings
struct Config {
    // Audio settings (using named constants)
    int sample_rate = WHISPER_SAMPLE_RATE;       ///< Audio sample rate (Hz)
    int channels = AUDIO_CHANNELS_MONO;          ///< Number of audio channels
    int frames_per_buffer = DEFAULT_BUFFER_FRAMES;  ///< Audio buffer size

    // Whisper model
    std::string model_dir = "models";
    ModelQuality model_quality = ModelQuality::Balanced;
    int n_threads = DEFAULT_THREAD_COUNT;        ///< CPU threads for inference

    /// @brief Get full path to whisper model file
    std::string get_model_path() const {
        return model_dir + "/" + get_model_filename(model_quality);
    }

    // Hotkey (default: Right Option/Alt key)
    uint32_t hotkey_keycode = 0;    ///< Platform-specific keycode
    uint32_t hotkey_modifiers = 0;  ///< Modifier keys (shift, ctrl, etc.)

    // Behavior
    bool auto_paste = true;         ///< Automatically paste after transcription
    int max_recording_seconds = DEFAULT_MAX_RECORDING_SEC;

    // Sound feedback options
    bool sound_on_record_start = false;   ///< Play sound when recording starts
    bool sound_on_record_stop = false;    ///< Play sound when recording stops
    bool sound_on_transcription = false;  ///< Play sound when transcription completes
    bool sound_on_error = false;          ///< Play sound on error

    // Performance & Accuracy
    bool use_gpu = true;            ///< Metal/CUDA acceleration
    bool adaptive_quality = true;   ///< Auto-retry with higher quality if low confidence
    bool translate = false;         ///< Translate to English (vs. transcribe)
    std::string language = "en";    ///< Source language code

    // Audio preprocessing
    bool audio_preprocessing = true;  ///< Enable noise reduction pipeline
    bool trim_silence = true;         ///< Trim silence from start/end (VAD)
    bool enhanced_vad = true;         ///< Use enhanced multi-segment speech extraction
    float silence_threshold = DEFAULT_SILENCE_THRESHOLD;
    int min_silence_ms = DEFAULT_MIN_SILENCE_MS;
    int vad_padding_ms = DEFAULT_VAD_PADDING_MS;

    // Initial prompt for context (helps accuracy and vocabulary recognition)
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
