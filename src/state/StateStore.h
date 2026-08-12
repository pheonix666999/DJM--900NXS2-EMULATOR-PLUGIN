#pragma once

#include "model/Types.h"
#include <array>
#include <juce_data_structures/juce_data_structures.h>
#include <optional>
#include <string>

namespace qb {
struct ChannelState {
    float trim{};
    float low{};
    float mid{};
    float high{};
    float fader{1.0F};
    bool cue{};
    bool mute{};
    EqMode eqMode{EqMode::classic};
    CrossfaderAssignment assignment{CrossfaderAssignment::thru};
};

struct AppState {
    static constexpr int currentVersion = 3;
    int version{currentVersion};
    std::array<ChannelState, channelCount> channels{};
    float crossfader{};
    float master{0.8F};
    float booth{0.7F};
    float headphones{0.7F};
    float cueMix{0.5F};
    EffectType effect{EffectType::echo};
    EffectBus effectBus{EffectBus::master};
    int division{5};
    float effectTime{0.5F};
    float effectDepth{0.5F};
    bool effectEnabled{true};
    bool lowBand{true};
    bool midBand{true};
    bool highBand{true};
    bool quantize{true};
    TempoSource tempoSource{TempoSource::automatic};
    double manualBpm{120.0};
    double uiScale{1.0};
    int windowX{-1};
    int windowY{-1};
    int windowWidth{1100};
    int windowHeight{800};
    std::string audioDeviceXml;
    juce::var midiMappings;
    std::array<int, 9> inputMappings{0, 1, 2, 3, 4, 5, 6, 7, -1};
    std::array<int, 6> outputMappings{0, 1, 2, 3, 4, 5};
    float microphoneLevel{1.0F};
    bool microphoneMute{};
    bool microphoneCue{};
    TempoAnalysisSource analysisSource{TempoAnalysisSource::master};
};

class StateStore {
  public:
    static juce::var toVar(const AppState& state);
    static std::optional<AppState> fromVar(const juce::var& value);
    static bool save(const juce::File& file, const AppState& state);
    static AppState loadOrDefault(const juce::File& file);
};
} // namespace qb
