#include "audio_processor.hpp"
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace whispr {

AudioProcessor::AudioProcessor(float sample_rate)
    : config_()
    , sample_rate_(sample_rate) {
    design_highpass_filter();
}

AudioProcessor::AudioProcessor(float sample_rate, const Config& config)
    : config_(config)
    , sample_rate_(sample_rate) {
    design_highpass_filter();
}

void AudioProcessor::design_highpass_filter() {
    // Biquad high-pass filter design (Butterworth, Q=0.707)
    float omega = 2.0f * static_cast<float>(M_PI) * config_.highpass_freq / sample_rate_;
    float cos_omega = std::cos(omega);
    float sin_omega = std::sin(omega);
    float alpha = sin_omega / (2.0f * 0.707f);  // Q = 0.707 for Butterworth

    float a0 = 1.0f + alpha;
    b0_ = (1.0f + cos_omega) / 2.0f / a0;
    b1_ = -(1.0f + cos_omega) / a0;
    b2_ = (1.0f + cos_omega) / 2.0f / a0;
    a1_ = -2.0f * cos_omega / a0;
    a2_ = (1.0f - alpha) / a0;
}

void AudioProcessor::reset() {
    x1_ = x2_ = y1_ = y2_ = 0.0f;
    gate_env_ = 0.0f;
    design_highpass_filter();
}

void AudioProcessor::process(std::vector<float>& audio) {
    if (audio.empty()) return;

    // Apply processing in order
    if (config_.enable_highpass) {
        apply_highpass(audio);
    }

    if (config_.enable_noise_gate) {
        apply_noise_gate(audio);
    }

    // Apply AGC before normalization for consistent levels
    if (config_.enable_agc) {
        apply_agc(audio);
    }

    if (config_.enable_normalization) {
        apply_normalization(audio);
    }
}

void AudioProcessor::apply_highpass(std::vector<float>& audio) {
    // Apply biquad filter: y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
    for (size_t i = 0; i < audio.size(); ++i) {
        float x0 = audio[i];
        float y0 = b0_ * x0 + b1_ * x1_ + b2_ * x2_ - a1_ * y1_ - a2_ * y2_;

        // Update state
        x2_ = x1_;
        x1_ = x0;
        y2_ = y1_;
        y1_ = y0;

        audio[i] = y0;
    }
}

void AudioProcessor::apply_noise_gate(std::vector<float>& audio) {
    // Envelope follower with attack/release for smooth gating
    float attack_coef = 1.0f - std::exp(-1.0f / (config_.noise_gate_attack * sample_rate_));
    float release_coef = 1.0f - std::exp(-1.0f / (config_.noise_gate_release * sample_rate_));

    for (size_t i = 0; i < audio.size(); ++i) {
        float abs_sample = std::abs(audio[i]);

        // Envelope follower
        if (abs_sample > gate_env_) {
            gate_env_ += attack_coef * (abs_sample - gate_env_);
        } else {
            gate_env_ += release_coef * (abs_sample - gate_env_);
        }

        // Gate: if envelope below threshold, attenuate
        if (gate_env_ < config_.noise_gate_threshold) {
            // Soft knee: gradual attenuation instead of hard cut
            float ratio = gate_env_ / config_.noise_gate_threshold;
            audio[i] *= ratio * ratio;  // Quadratic rolloff
        }
    }
}

void AudioProcessor::apply_normalization(std::vector<float>& audio) {
    if (audio.empty()) return;

    // Find peak
    float peak = 0.0f;
    for (float sample : audio) {
        float abs_sample = std::abs(sample);
        if (abs_sample > peak) {
            peak = abs_sample;
        }
    }

    // Avoid division by zero and don't amplify very quiet signals
    if (peak < 0.001f) return;  // -60dB threshold

    // Calculate gain to reach target peak
    float gain = config_.target_peak / peak;

    // Limit gain to avoid excessive amplification of quiet recordings
    gain = std::min(gain, 10.0f);  // Max 20dB boost

    // Apply gain
    for (float& sample : audio) {
        sample *= gain;
        // Soft clip to prevent any possibility of clipping
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
    }
}

void AudioProcessor::apply_agc(std::vector<float>& audio) {
    if (audio.empty()) return;

    // Calculate current RMS level
    float sum_sq = 0.0f;
    for (float sample : audio) {
        sum_sq += sample * sample;
    }
    float rms = std::sqrt(sum_sq / audio.size());

    // Avoid division by zero
    if (rms < 0.0001f) return;  // Too quiet to process

    // Calculate gain needed to reach target RMS
    float gain = config_.agc_target_rms / rms;

    // Clamp gain to configured limits
    gain = std::max(config_.agc_min_gain, std::min(gain, config_.agc_max_gain));

    // Apply gain with soft clipping
    for (float& sample : audio) {
        sample *= gain;
        // Soft clip using tanh for natural compression
        if (std::abs(sample) > 0.9f) {
            sample = 0.9f * std::tanh(sample / 0.9f);
        }
    }
}

std::vector<float> AudioProcessor::trim_silence(const std::vector<float>& audio,
                                                  float threshold,
                                                  int min_silence_samples,
                                                  int sample_rate) {
    if (audio.empty()) return audio;

    // Use a sliding window to detect voice activity
    int window_size = sample_rate / 100;  // 10ms window
    if (window_size < 1) window_size = 1;

    // Find start of speech (first window above threshold)
    size_t start_idx = 0;
    for (size_t i = 0; i + window_size <= audio.size(); i += window_size / 2) {
        // Calculate RMS energy of window
        float sum_sq = 0.0f;
        for (size_t j = i; j < i + window_size && j < audio.size(); ++j) {
            sum_sq += audio[j] * audio[j];
        }
        float rms = std::sqrt(sum_sq / window_size);

        if (rms > threshold) {
            // Found speech, back up a bit for attack
            start_idx = (i > static_cast<size_t>(min_silence_samples / 2))
                        ? i - min_silence_samples / 2 : 0;
            break;
        }
    }

    // Find end of speech (last window above threshold)
    size_t end_idx = audio.size();
    for (size_t i = audio.size(); i >= window_size; i -= window_size / 2) {
        size_t win_start = i - window_size;

        // Calculate RMS energy of window
        float sum_sq = 0.0f;
        for (size_t j = win_start; j < i && j < audio.size(); ++j) {
            sum_sq += audio[j] * audio[j];
        }
        float rms = std::sqrt(sum_sq / window_size);

        if (rms > threshold) {
            // Found speech, add some tail for release
            end_idx = std::min(i + min_silence_samples / 2, audio.size());
            break;
        }
    }

    // Sanity checks
    if (start_idx >= end_idx || end_idx - start_idx < static_cast<size_t>(sample_rate / 10)) {
        // Less than 100ms of audio or invalid range, return original
        return audio;
    }

    // Return trimmed audio
    return std::vector<float>(audio.begin() + start_idx, audio.begin() + end_idx);
}

std::vector<float> AudioProcessor::calculate_energy(const std::vector<float>& audio,
                                                     int window_size,
                                                     int hop_size) {
    if (audio.empty() || window_size <= 0) return {};

    std::vector<float> energy;
    energy.reserve(audio.size() / hop_size + 1);

    for (size_t i = 0; i + window_size <= audio.size(); i += hop_size) {
        float sum_sq = 0.0f;
        for (size_t j = i; j < i + window_size; ++j) {
            sum_sq += audio[j] * audio[j];
        }
        energy.push_back(std::sqrt(sum_sq / window_size));  // RMS
    }

    // Smooth the energy curve with a simple moving average
    if (energy.size() > 3) {
        std::vector<float> smoothed(energy.size());
        for (size_t i = 0; i < energy.size(); ++i) {
            float sum = 0.0f;
            int count = 0;
            for (int j = -2; j <= 2; ++j) {
                int idx = static_cast<int>(i) + j;
                if (idx >= 0 && idx < static_cast<int>(energy.size())) {
                    sum += energy[idx];
                    count++;
                }
            }
            smoothed[i] = sum / count;
        }
        return smoothed;
    }

    return energy;
}

std::vector<float> AudioProcessor::extract_speech(const std::vector<float>& audio,
                                                   float threshold,
                                                   int min_speech_ms,
                                                   int padding_ms,
                                                   int sample_rate) {
    if (audio.empty()) return audio;

    int window_size = sample_rate / 100;  // 10ms
    int hop_size = sample_rate / 200;     // 5ms

    // Calculate smoothed energy envelope
    auto energy = calculate_energy(audio, window_size, hop_size);
    if (energy.empty()) return audio;

    // Find speech segments (regions above threshold)
    int min_speech_frames = (min_speech_ms * sample_rate) / (1000 * hop_size);
    int padding_frames = (padding_ms * sample_rate) / (1000 * hop_size);

    struct Segment {
        size_t start;
        size_t end;
    };
    std::vector<Segment> segments;

    bool in_speech = false;
    size_t speech_start = 0;
    int consecutive_speech = 0;
    int consecutive_silence = 0;

    for (size_t i = 0; i < energy.size(); ++i) {
        if (energy[i] > threshold) {
            consecutive_speech++;
            consecutive_silence = 0;

            if (!in_speech && consecutive_speech >= 2) {
                // Start of speech segment
                in_speech = true;
                speech_start = i > 1 ? i - 1 : 0;
            }
        } else {
            consecutive_silence++;

            if (in_speech && consecutive_silence >= 3) {
                // End of speech segment
                in_speech = false;
                size_t speech_end = i;

                // Only keep segments longer than minimum
                if (speech_end - speech_start >= static_cast<size_t>(min_speech_frames)) {
                    segments.push_back({speech_start, speech_end});
                }
                consecutive_speech = 0;
            }
        }
    }

    // Handle case where speech extends to end
    if (in_speech) {
        if (energy.size() - speech_start >= static_cast<size_t>(min_speech_frames)) {
            segments.push_back({speech_start, energy.size()});
        }
    }

    // If no valid segments found, return original
    if (segments.empty()) {
        return audio;
    }

    // Merge close segments and add padding
    std::vector<Segment> merged_segments;
    for (const auto& seg : segments) {
        // Add padding
        size_t padded_start = seg.start > static_cast<size_t>(padding_frames) ?
                             seg.start - padding_frames : 0;
        size_t padded_end = std::min(seg.end + padding_frames, energy.size());

        // Check if we can merge with previous segment
        if (!merged_segments.empty() && padded_start <= merged_segments.back().end) {
            merged_segments.back().end = padded_end;
        } else {
            merged_segments.push_back({padded_start, padded_end});
        }
    }

    // Convert frame indices to sample indices and extract audio
    std::vector<float> result;
    for (const auto& seg : merged_segments) {
        size_t sample_start = seg.start * hop_size;
        size_t sample_end = std::min(seg.end * hop_size + window_size, audio.size());

        result.insert(result.end(),
                     audio.begin() + sample_start,
                     audio.begin() + sample_end);
    }

    return result.empty() ? audio : result;
}

} // namespace whispr
