/**
 * @file transcriber.hpp
 * @brief Speech-to-text transcription using whisper.cpp
 *
 * This module provides the core transcription functionality for VoxType,
 * wrapping the whisper.cpp library with a clean C++ interface.
 */

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

/**
 * @struct TranscriptionResult
 * @brief Result of a transcription operation
 */
struct TranscriptionResult {
    std::string text;       ///< Processed transcription text
    std::string raw_text;   ///< Original unprocessed text from whisper
    int64_t duration_ms;    ///< Time taken for transcription in milliseconds
    float confidence;       ///< Average token probability (0.0 - 1.0)
    bool success;           ///< Whether transcription succeeded
    std::string error;      ///< Error message if success is false
};

/**
 * @class Transcriber
 * @brief Main transcription engine using whisper.cpp
 *
 * The Transcriber class provides speech-to-text functionality using
 * OpenAI's Whisper model running locally via whisper.cpp.
 *
 * @note Thread-safety: Transcription operations are not thread-safe.
 *       Use a single Transcriber instance per thread.
 */
class Transcriber {
public:
    /// Callback type for progress updates (0-100)
    using ProgressCallback = std::function<void(int progress)>;

    Transcriber();
    ~Transcriber();

    /**
     * @brief Initialize the transcriber with a whisper model
     * @param model_path Path to the GGML model file (e.g., ggml-base.en.bin)
     * @param n_threads Number of CPU threads for inference
     * @return true if initialization succeeded, false otherwise
     */
    bool initialize(const std::string& model_path, int n_threads = 4);

    /**
     * @brief Shutdown the transcriber and release resources
     */
    void shutdown();
    /// @brief Check if transcriber is initialized
    /// @return true if ready to transcribe
    bool is_initialized() const { return ctx_ != nullptr; }

    /**
     * @brief Transcribe audio samples
     * @param audio Audio samples (16kHz mono float, range -1.0 to 1.0)
     * @return TranscriptionResult with text and metadata
     */
    TranscriptionResult transcribe(const std::vector<float>& audio);

    /**
     * @brief Transcribe with a specific quality profile
     * @param audio Audio samples (16kHz mono float)
     * @param profile Transcription profile (PROFILE_FAST, PROFILE_BALANCED, etc.)
     * @return TranscriptionResult with text and metadata
     */
    TranscriptionResult transcribe_with_profile(const std::vector<float>& audio,
                                                 const TranscriptionProfile& profile);

    /**
     * @brief Adaptive transcription with automatic quality escalation
     *
     * Starts with fast transcription and retries with higher quality
     * if the confidence score is below the threshold.
     *
     * @param audio Audio samples (16kHz mono float)
     * @param confidence_threshold Minimum confidence to accept (0.0 - 1.0)
     * @return TranscriptionResult with text and metadata
     */
    TranscriptionResult transcribe_adaptive(const std::vector<float>& audio,
                                            float confidence_threshold = 0.7f);

    /// @brief Set the language for transcription (e.g., "en", "es", "fr")
    void set_language(const std::string& lang) { language_ = lang; }

    /// @brief Enable translation to English (for non-English audio)
    void set_translate(bool translate) { translate_ = translate; }

    /// @brief Set the transcription quality profile
    void set_profile(const TranscriptionProfile& profile) { profile_ = profile; }

    /// @brief Set initial prompt for context (improves vocabulary recognition)
    void set_initial_prompt(const std::string& prompt) { initial_prompt_ = prompt; }

    /// @brief Set callback for progress updates during transcription
    void set_progress_callback(ProgressCallback cb) { progress_cb_ = cb; }

    /// @brief Enable/disable post-processing of transcribed text
    void set_text_processing(bool enabled) { process_text_ = enabled; }

    /// @brief Check if text processing is enabled
    bool get_text_processing() const { return process_text_; }

    /// @brief Set text processor configuration
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
