#include "transcriber.hpp"
#include "whisper.h"
#include <iostream>
#include <chrono>

namespace whispr {

Transcriber::Transcriber() = default;

Transcriber::~Transcriber() {
    shutdown();
}

bool Transcriber::initialize(const std::string& model_path, int n_threads) {
    if (ctx_) return true;

    n_threads_ = n_threads;

    // Initialize whisper context with GPU acceleration
    struct whisper_context_params cparams = whisper_context_default_params();
#ifdef PLATFORM_MACOS
    cparams.use_gpu = true;   // Enable Metal acceleration on macOS
#else
    cparams.use_gpu = false;
#endif

    ctx_ = whisper_init_from_file_with_params(model_path.c_str(), cparams);
    if (!ctx_) {
        std::cerr << "Failed to load whisper model: " << model_path << std::endl;
        return false;
    }

    std::cout << "Loaded whisper model: " << model_path << std::endl;
    return true;
}

void Transcriber::shutdown() {
    if (ctx_) {
        whisper_free(ctx_);
        ctx_ = nullptr;
    }
}

TranscriptionResult Transcriber::transcribe(const std::vector<float>& audio) {
    return transcribe_with_profile(audio, profile_);
}

TranscriptionResult Transcriber::transcribe_with_profile(const std::vector<float>& audio,
                                                          const TranscriptionProfile& profile) {
    TranscriptionResult result;
    result.success = false;
    result.confidence = 0.0f;

    if (!ctx_) {
        result.error = "Transcriber not initialized";
        return result;
    }

    if (audio.empty()) {
        result.error = "No audio data";
        return result;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // Configure whisper parameters based on profile
    whisper_full_params wparams = whisper_full_default_params(
        profile.beam_size > 1 ? WHISPER_SAMPLING_BEAM_SEARCH : WHISPER_SAMPLING_GREEDY
    );

    wparams.print_progress   = false;
    wparams.print_special    = false;
    wparams.print_realtime   = false;
    wparams.print_timestamps = false;
    wparams.translate        = translate_;
    wparams.single_segment   = true;   // Faster for short audio
    wparams.no_context       = initial_prompt_.empty();  // Use context if prompt provided
    wparams.language         = language_.c_str();
    wparams.n_threads        = n_threads_;

    // Apply profile settings
    wparams.greedy.best_of        = profile.best_of;
    wparams.beam_search.beam_size = profile.beam_size;
    wparams.entropy_thold         = profile.entropy_thold;
    wparams.no_speech_thold       = profile.no_speech_thold;
    wparams.temperature           = profile.temperature;
    wparams.logprob_thold         = -1.0f;

    // Initial prompt for context
    if (!initial_prompt_.empty()) {
        wparams.initial_prompt = initial_prompt_.c_str();
    }

    // Progress callback
    if (progress_cb_) {
        wparams.progress_callback = [](struct whisper_context* ctx, struct whisper_state* state, int progress, void* user_data) {
            (void)ctx;
            (void)state;
            auto* cb = static_cast<ProgressCallback*>(user_data);
            (*cb)(progress);
        };
        wparams.progress_callback_user_data = &progress_cb_;
    }

    // Run inference
    int ret = whisper_full(ctx_, wparams, audio.data(), static_cast<int>(audio.size()));
    if (ret != 0) {
        result.error = "Whisper inference failed";
        return result;
    }

    // Get result
    const int n_segments = whisper_full_n_segments(ctx_);
    std::string text;
    for (int i = 0; i < n_segments; ++i) {
        const char* segment_text = whisper_full_get_segment_text(ctx_, i);
        if (segment_text) {
            text += segment_text;
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Trim whitespace
    size_t start = text.find_first_not_of(" \t\n\r");
    size_t end = text.find_last_not_of(" \t\n\r");
    if (start != std::string::npos && end != std::string::npos) {
        text = text.substr(start, end - start + 1);
    }

    // Store raw text before processing
    result.raw_text = text;

    // Post-process text (remove fillers, fix formatting)
    if (process_text_ && !text.empty()) {
        text = text_processor_.process(text);
    }

    result.text = text;
    result.duration_ms = duration.count();
    result.confidence = calculate_confidence();
    result.success = true;

    std::cout << "Transcription [" << profile.name << "] took " << result.duration_ms << "ms (conf: "
              << static_cast<int>(result.confidence * 100) << "%): \"" << result.text << "\"" << std::endl;
    if (process_text_ && result.raw_text != result.text) {
        std::cout << "  (raw: \"" << result.raw_text << "\")" << std::endl;
    }

    return result;
}

TranscriptionResult Transcriber::transcribe_adaptive(const std::vector<float>& audio,
                                                      float confidence_threshold) {
    // First pass: try with current (fast) profile
    auto result = transcribe_with_profile(audio, PROFILE_FAST);

    // If confidence is low and we're not already using the best profile, retry
    if (result.success && result.confidence < confidence_threshold && !result.text.empty()) {
        std::cout << "Low confidence (" << static_cast<int>(result.confidence * 100)
                  << "%), retrying with Accurate profile..." << std::endl;

        auto retry_result = transcribe_with_profile(audio, PROFILE_ACCURATE);

        // Use retry result if it's better
        if (retry_result.success && retry_result.confidence > result.confidence) {
            retry_result.duration_ms += result.duration_ms;  // Add total time
            return retry_result;
        }
    }

    return result;
}

float Transcriber::calculate_confidence() const {
    if (!ctx_) return 0.0f;

    const int n_segments = whisper_full_n_segments(ctx_);
    if (n_segments == 0) return 0.0f;

    float total_prob = 0.0f;
    int total_tokens = 0;

    for (int seg = 0; seg < n_segments; ++seg) {
        const int n_tokens = whisper_full_n_tokens(ctx_, seg);
        for (int tok = 0; tok < n_tokens; ++tok) {
            whisper_token_data token_data = whisper_full_get_token_data(ctx_, seg, tok);
            // Skip special tokens (negative IDs or very low probability)
            if (token_data.id >= 0 && token_data.p > 0.0f) {
                total_prob += token_data.p;
                total_tokens++;
            }
        }
    }

    return total_tokens > 0 ? total_prob / static_cast<float>(total_tokens) : 0.0f;
}

} // namespace whispr
