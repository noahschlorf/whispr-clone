#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "text_processor.hpp"
#include "config.hpp"

// Forward declare whisper types
struct whisper_context;

namespace whispr {

struct TranscriptionResult {
    std::string text;
    std::string raw_text;  // Original unprocessed text
    int64_t duration_ms;
    float confidence;      // Average token probability (0.0 - 1.0)
    bool success;
    std::string error;
};

class Transcriber {
public:
    using ProgressCallback = std::function<void(int progress)>;

    Transcriber();
    ~Transcriber();

    // Initialize with model path
    bool initialize(const std::string& model_path, int n_threads = 4);
    void shutdown();
    bool is_initialized() const { return ctx_ != nullptr; }

    // Transcribe audio samples (16kHz mono float)
    TranscriptionResult transcribe(const std::vector<float>& audio);

    // Transcribe with specific profile (for adaptive quality)
    TranscriptionResult transcribe_with_profile(const std::vector<float>& audio,
                                                 const TranscriptionProfile& profile);

    // Adaptive transcription: starts fast, retries with higher quality if low confidence
    TranscriptionResult transcribe_adaptive(const std::vector<float>& audio,
                                            float confidence_threshold = 0.7f);

    // Settings
    void set_language(const std::string& lang) { language_ = lang; }
    void set_translate(bool translate) { translate_ = translate; }
    void set_profile(const TranscriptionProfile& profile) { profile_ = profile; }
    void set_initial_prompt(const std::string& prompt) { initial_prompt_ = prompt; }
    void set_progress_callback(ProgressCallback cb) { progress_cb_ = cb; }

    // Text processing settings
    void set_text_processing(bool enabled) { process_text_ = enabled; }
    bool get_text_processing() const { return process_text_; }
    void set_text_processor_config(const TextProcessorConfig& config) {
        text_processor_ = TextProcessor(config);
    }

private:
    whisper_context* ctx_ = nullptr;
    int n_threads_ = 4;
    std::string language_ = "en";
    bool translate_ = false;
    TranscriptionProfile profile_ = PROFILE_BALANCED;
    std::string initial_prompt_;
    ProgressCallback progress_cb_;

    // Text post-processing
    TextProcessor text_processor_;
    bool process_text_ = true;  // Enabled by default

    // Calculate confidence from token probabilities
    float calculate_confidence() const;
};

} // namespace whispr
