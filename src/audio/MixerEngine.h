#pragma once

#include "dsp/EffectRack.h"
#include "model/Types.h"
#include <array>
#include <atomic>
#include <juce_audio_devices/juce_audio_devices.h>

namespace qb {
struct ChannelParameters {
    std::atomic<float> trim{0.0F};
    std::atomic<float> low{0.0F};
    std::atomic<float> mid{0.0F};
    std::atomic<float> high{0.0F};
    std::atomic<float> fader{1.0F};
    std::atomic<bool> cue{};
    std::atomic<bool> mute{};
    std::atomic<EqMode> eqMode{EqMode::classic};
    std::atomic<CrossfaderAssignment> assignment{CrossfaderAssignment::thru};
};

class MixerEngine final : public juce::AudioIODeviceCallback {
  public:
    MixerEngine();
    void prepare(double sampleRate, int maximumBlockSize);
    void process(const juce::AudioBuffer<float>& inputs,
                 juce::AudioBuffer<float>& outputs) noexcept;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels, float* const* outputChannelData,
                                          int numOutputChannels, int numSamples,
                                          const juce::AudioIODeviceCallbackContext&) override;

    std::array<ChannelParameters, channelCount> channels;
    std::atomic<float> masterLevel{0.8F};
    std::atomic<float> boothLevel{0.7F};
    std::atomic<float> headphonesLevel{0.7F};
    std::atomic<float> cueMix{0.5F};
    std::atomic<float> crossfader{};
    std::atomic<CrossfaderCurve> crossfaderCurve{CrossfaderCurve::constantPower};
    std::atomic<EffectBus> effectBus{EffectBus::master};
    EffectRack effectRack;

    [[nodiscard]] float channelPeak(int index, int side) const noexcept;
    [[nodiscard]] float masterPeak(int side) const noexcept;

  private:
    struct SplitState {
        float low{};
        float high{};
    };
    void processChannel(int channel, const juce::AudioBuffer<float>& input,
                        juce::AudioBuffer<float>& output) noexcept;
    std::pair<float, float> crossfaderGains() const noexcept;
    void applyEffect(juce::AudioBuffer<float>& bus) noexcept;
    static float safe(float value) noexcept;
    static float decibels(float normalised, float range) noexcept;

    double rate{44100.0};
    int maxBlock{2048};
    std::array<juce::AudioBuffer<float>, channelCount> channelBuffers;
    juce::AudioBuffer<float> busA;
    juce::AudioBuffer<float> busB;
    juce::AudioBuffer<float> thruBus;
    juce::AudioBuffer<float> cueBus;
    juce::AudioBuffer<float> masterBus;
    juce::AudioBuffer<float> callbackInput;
    std::array<std::array<SplitState, 2>, channelCount> eqStates{};
    std::array<std::array<std::atomic<float>, 2>, channelCount> peaks{};
    std::array<std::atomic<float>, 2> masterPeaks{};
};
} // namespace qb
