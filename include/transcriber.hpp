#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

// Forward declare whisper types
struct whisper_context;

namespace whispr {

struct TranscriptionResult {
    std::string text;
    int64_t duration_ms;
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

    // Settings
    void set_language(const std::string& lang) { language_ = lang; }
    void set_translate(bool translate) { translate_ = translate; }
    void set_beam_size(int size) { beam_size_ = size; }
    void set_progress_callback(ProgressCallback cb) { progress_cb_ = cb; }

private:
    whisper_context* ctx_ = nullptr;
    int n_threads_ = 4;
    std::string language_ = "en";
    bool translate_ = false;
    int beam_size_ = 1;  // Greedy for speed
    ProgressCallback progress_cb_;
};

} // namespace whispr
