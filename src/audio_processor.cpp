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

} // namespace whispr
