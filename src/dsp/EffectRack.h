#pragma once

#include "model/Types.h"
#include <array>
#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

namespace qb {
struct EffectParameters {
    EffectType type{EffectType::echo};
    float time{0.5F};
    float depth{0.5F};
    double bpm{120.0};
    int division{5};
    bool enabled{};
    bool low{true};
    bool mid{true};
    bool high{true};
};

class EffectRack {
  public:
    void prepare(double sampleRate, int maximumBlockSize);
    void reset() noexcept;
    void setParameters(const EffectParameters& parameters) noexcept;
    void process(juce::AudioBuffer<float>& stereo) noexcept;

  private:
    struct Biquad {
        void lowPass(double sampleRate, float frequency, float q) noexcept;
        void allPass(double sampleRate, float frequency, float q) noexcept;
        float process(float input) noexcept;
        void reset() noexcept { z1 = z2 = 0.0F; }
        float b0{}, b1{}, b2{}, a1{}, a2{}, z1{}, z2{};
    };

    float readDelay(int channel, float delaySamples) const noexcept;
    void writeDelay(float left, float right) noexcept;
    void processWet(float left, float right, float& wetLeft, float& wetRight,
                    const EffectParameters& p) noexcept;
    void splitBands(float input, int channel, float& low, float& mid, float& high) noexcept;
    static float finite(float value) noexcept;

    std::atomic<EffectType> type{EffectType::echo};
    std::atomic<float> time{0.5F};
    std::atomic<float> depth{0.5F};
    std::atomic<double> bpm{120.0};
    std::atomic<int> division{5};
    std::atomic<bool> enabled{};
    std::atomic<unsigned> bands{7U};
    double rate{44100.0};
    int maximumBlock{2048};
    int writePosition{};
    std::array<std::vector<float>, 2> delay;
    std::array<float, 2> lowState{};
    std::array<float, 2> highState{};
    std::array<float, 2> reverbA{};
    std::array<float, 2> reverbB{};
    std::array<std::array<Biquad, 4>, 2> phaser{};
    float lfoPhase{};
    float smoothedTime{0.5F};
    float smoothedDepth{};
    float gate{};
    float brakeRead{};
    float brakeSpeed{1.0F};
    bool wasEnabled{};
};
} // namespace qb
