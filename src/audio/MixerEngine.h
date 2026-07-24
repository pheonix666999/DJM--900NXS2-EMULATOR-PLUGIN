#pragma once

#include "dsp/EffectRack.h"
#include "model/Types.h"
#include <array>
#include <atomic>
#include <juce_audio_devices/juce_audio_devices.h>

namespace qb {
class TempoEngine;

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
    void processMappedDeviceBlock(const juce::AudioBuffer<float>& physicalInputs,
                                  juce::AudioBuffer<float>& physicalOutputs) noexcept;
    void setTempoEngine(TempoEngine* analyser) noexcept { tempoAnalyser = analyser; }
    void setChannelInputMapping(int logicalChannel, int side, int physicalChannel) noexcept;
    void setMicrophoneInputMapping(int physicalChannel) noexcept;
    void setOutputMapping(int logicalOutput, int side, int physicalChannel) noexcept;
    [[nodiscard]] int channelInputMapping(int logicalChannel, int side) const noexcept;
    [[nodiscard]] int microphoneInputMapping() const noexcept;
    [[nodiscard]] int outputMapping(int logicalOutput, int side) const noexcept;

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
    std::atomic<TempoAnalysisSource> analysisSource{TempoAnalysisSource::master};
    std::atomic<float> microphoneLevel{1.0F};
    std::atomic<bool> microphoneMute{};
    std::atomic<bool> microphoneCue{};
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
    void publishTempoAnalysis(int samples) noexcept;
    void mapPhysicalInputs(const juce::AudioBuffer<float>& physicalInputs, int samples) noexcept;
    void mapLogicalOutputs(juce::AudioBuffer<float>& physicalOutputs, int samples) noexcept;
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
    juce::AudioBuffer<float> microphoneBus;
    juce::AudioBuffer<float> callbackInput;
    juce::AudioBuffer<float> callbackOutput;
    std::array<std::atomic<int>, channelCount * 2> inputMappings;
    std::atomic<int> microphoneMapping{-1};
    std::array<std::atomic<int>, 6> outputMappings;
    TempoEngine* tempoAnalyser{};
    std::array<std::array<SplitState, 2>, channelCount> eqStates{};
    std::array<std::array<std::atomic<float>, 2>, channelCount> peaks{};
    std::array<std::atomic<float>, 2> masterPeaks{};
};
} // namespace qb
