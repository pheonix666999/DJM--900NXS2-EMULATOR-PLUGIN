#pragma once

#include "audio/MixerEngine.h"
#include <array>
#include <juce_audio_utils/juce_audio_utils.h>

namespace qb {
class SettingsComponent final : public juce::Component, private juce::Timer {
  public:
    SettingsComponent(MixerEngine& engine, juce::AudioDeviceManager& devices);
    void resized() override;

  private:
    void timerCallback() override;
    void refreshRoutingChoices();
    static void fillChannelChoices(juce::ComboBox& box, int available, int selected);

    MixerEngine& mixer;
    juce::AudioDeviceManager& deviceManager;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;
    std::array<juce::Label, 15> routingLabels;
    std::array<juce::ComboBox, 9> inputChoices;
    std::array<juce::ComboBox, 6> outputChoices;
    juce::Label routingTitle;
    juce::Label microphoneLevelLabel;
    juce::Slider microphoneLevel;
    juce::ToggleButton microphoneMute{"Mute microphone"};
    juce::ToggleButton microphoneCue{"Cue microphone"};
    juce::Label analysisLabel;
    juce::ComboBox analysisSource;
    int lastInputCount{-1};
    int lastOutputCount{-1};
};
} // namespace qb
