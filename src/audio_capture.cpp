#include "audio_capture.hpp"
#include <iostream>
#include <cstring>

namespace whispr {

AudioCapture::AudioCapture(int sample_rate, int channels, int frames_per_buffer)
    : sample_rate_(sample_rate)
    , channels_(channels)
    , frames_per_buffer_(frames_per_buffer) {
}

AudioCapture::~AudioCapture() {
    shutdown();
}

bool AudioCapture::initialize() {
    if (initialized_.load()) return true;

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio init failed: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    // Open default input device
    PaStreamParameters input_params;
    input_params.device = Pa_GetDefaultInputDevice();
    if (input_params.device == paNoDevice) {
        std::cerr << "No default input device" << std::endl;
        Pa_Terminate();
        return false;
    }

    input_params.channelCount = channels_;
    input_params.sampleFormat = paFloat32;
    input_params.suggestedLatency = Pa_GetDeviceInfo(input_params.device)->defaultLowInputLatency;
    input_params.hostApiSpecificStreamInfo = nullptr;

    err = Pa_OpenStream(&stream_,
                        &input_params,
                        nullptr,  // No output
                        sample_rate_,
                        frames_per_buffer_,
                        paClipOff,
                        pa_callback,
                        this);

    if (err != paNoError) {
        std::cerr << "Failed to open stream: " << Pa_GetErrorText(err) << std::endl;
        Pa_Terminate();
        return false;
    }

    initialized_.store(true);
    return true;
}

void AudioCapture::shutdown() {
    if (!initialized_.load()) return;

    stop_recording();

    if (stream_) {
        Pa_CloseStream(stream_);
        stream_ = nullptr;
    }

    Pa_Terminate();
    initialized_.store(false);
}

bool AudioCapture::start_recording() {
    if (!initialized_.load() || recording_.load()) return false;

    clear_buffer();

    PaError err = Pa_StartStream(stream_);
    if (err != paNoError) {
        std::cerr << "Failed to start stream: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    recording_.store(true);
    return true;
}

bool AudioCapture::stop_recording() {
    if (!recording_.load()) return false;

    recording_.store(false);

    PaError err = Pa_StopStream(stream_);
    if (err != paNoError) {
        std::cerr << "Failed to stop stream: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    return true;
}

std::vector<float> AudioCapture::get_recorded_audio() {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    return audio_buffer_;
}

void AudioCapture::clear_buffer() {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    audio_buffer_.clear();
    audio_buffer_.reserve(sample_rate_ * 30);  // Reserve for 30 seconds
}

int AudioCapture::pa_callback(const void* input, void* output,
                              unsigned long frame_count,
                              const PaStreamCallbackTimeInfo* time_info,
                              PaStreamCallbackFlags status_flags,
                              void* user_data) {
    (void)output;
    (void)time_info;
    (void)status_flags;

    auto* capture = static_cast<AudioCapture*>(user_data);
    if (!capture->recording_.load()) return paContinue;

    const float* in = static_cast<const float*>(input);

    {
        std::lock_guard<std::mutex> lock(capture->buffer_mutex_);
        capture->audio_buffer_.insert(capture->audio_buffer_.end(), in, in + frame_count);
    }

    if (capture->callback_) {
        std::vector<float> chunk(in, in + frame_count);
        capture->callback_(chunk);
    }

    return paContinue;
}

} // namespace whispr
