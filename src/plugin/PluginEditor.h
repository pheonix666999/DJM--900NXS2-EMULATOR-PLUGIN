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
    static constexpr int baseWidth = 1200;
    static constexpr int baseHeight = 820;
    QuadBeatAudioProcessor& processor;
    MainComponent content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuadBeatAudioProcessorEditor)
};
} // namespace qb
