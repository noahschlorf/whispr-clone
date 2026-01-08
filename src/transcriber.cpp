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
    TranscriptionResult result;
    result.success = false;

    if (!ctx_) {
        result.error = "Transcriber not initialized";
        return result;
    }

    if (audio.empty()) {
        result.error = "No audio data";
        return result;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // Configure whisper parameters for speed
    whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

    wparams.print_progress   = false;
    wparams.print_special    = false;
    wparams.print_realtime   = false;
    wparams.print_timestamps = false;
    wparams.translate        = translate_;
    wparams.single_segment   = true;   // Faster for short audio
    wparams.no_context       = true;   // Don't use previous context
    wparams.language         = language_.c_str();
    wparams.n_threads        = n_threads_;

    // Speed optimizations
    wparams.greedy.best_of   = beam_size_;
    wparams.beam_search.beam_size = beam_size_;
    wparams.entropy_thold    = 2.4f;
    wparams.logprob_thold    = -1.0f;
    wparams.no_speech_thold  = 0.6f;

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

    result.text = text;
    result.duration_ms = duration.count();
    result.success = true;

    std::cout << "Transcription took " << result.duration_ms << "ms: \"" << result.text << "\"" << std::endl;

    return result;
}

} // namespace whispr
