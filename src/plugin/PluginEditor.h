#pragma once

#include "plugin/PluginProcessor.h"
#include "ui/MainComponent.h"

namespace qb {
class QuadBeatAudioProcessorEditor final : public juce::AudioProcessorEditor {
  public:
    explicit QuadBeatAudioProcessorEditor(QuadBeatAudioProcessor&);
    ~QuadBeatAudioProcessorEditor() override;
    void resized() override;

  private:
    static constexpr int baseWidth = 360;
    static constexpr int baseHeight = 1040;
    static constexpr int minimumWidth = 340;
    static constexpr int minimumHeight = 900;
    static constexpr int maximumWidth = 720;
    static constexpr int maximumHeight = 2080;
    QuadBeatAudioProcessor& processor;
    MainComponent content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuadBeatAudioProcessorEditor)
};
} // namespace qb
