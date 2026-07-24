#pragma once

#include "midi/MidiMapper.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace qb {
class MidiMappingEditor final : public juce::Component {
  public:
    explicit MidiMappingEditor(MidiMapper& mapper);
    void resized() override;

  private:
    void refreshList(int preferredIndex = 0);
    void loadSelection();
    void applySelection();
    void deleteSelection();

    MidiMapper& midiMapper;
    juce::Label title;
    juce::ComboBox mappingSelector;
    juce::Label messageInfo;
    juce::Label channelLabel;
    juce::ComboBox channel;
    juce::Label modeLabel;
    juce::ComboBox mode;
    juce::Label minimumLabel;
    juce::TextEditor minimum;
    juce::Label maximumLabel;
    juce::TextEditor maximum;
    juce::Label pickupLabel;
    juce::TextEditor pickup;
    juce::ToggleButton inverted{"Invert range"};
    juce::TextButton apply{"APPLY"};
    juce::TextButton remove{"DELETE"};
};
} // namespace qb
