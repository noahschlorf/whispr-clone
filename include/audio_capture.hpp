#pragma once

#include <vector>
#include <atomic>
#include <functional>
#include <mutex>
#include <portaudio.h>

namespace whispr {

class AudioCapture {
public:
    using AudioCallback = std::function<void(const std::vector<float>&)>;

    AudioCapture(int sample_rate = 16000, int channels = 1, int frames_per_buffer = 512);
    ~AudioCapture();

    bool initialize();
    void shutdown();

    bool start_recording();
    bool stop_recording();
    bool is_recording() const { return recording_.load(); }

    // Get all recorded audio since start_recording()
    std::vector<float> get_recorded_audio();

    // Clear the audio buffer
    void clear_buffer();

    // Set callback for real-time audio data
    void set_callback(AudioCallback callback) { callback_ = callback; }

private:
    static int pa_callback(const void* input, void* output,
                          unsigned long frame_count,
                          const PaStreamCallbackTimeInfo* time_info,
                          PaStreamCallbackFlags status_flags,
                          void* user_data);

    int sample_rate_;
    int channels_;
    int frames_per_buffer_;

    PaStream* stream_ = nullptr;
    std::atomic<bool> recording_{false};
    std::atomic<bool> initialized_{false};

    std::vector<float> audio_buffer_;
    std::mutex buffer_mutex_;

    AudioCallback callback_;
};

} // namespace whispr
