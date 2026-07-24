#include "ui/MidiMappingEditor.h"
#include <algorithm>

namespace qb {
MidiMappingEditor::MidiMappingEditor(MidiMapper& mapper) : midiMapper(mapper) {
    title.setText("MIDI MAPPING EDITOR", juce::dontSendNotification);
    title.setFont(juce::FontOptions(18.0F, juce::Font::bold));
    addAndMakeVisible(title);
    addAndMakeVisible(mappingSelector);
    mappingSelector.onChange = [this] { loadSelection(); };
    addAndMakeVisible(messageInfo);

    channelLabel.setText("Channel", juce::dontSendNotification);
    addAndMakeVisible(channelLabel);
    for (int value = 1; value <= 16; ++value)
        channel.addItem(juce::String(value), value);
    addAndMakeVisible(channel);

    modeLabel.setText("Encoder mode", juce::dontSendNotification);
    addAndMakeVisible(modeLabel);
    mode.addItemList({"Absolute CC", "Relative 2's complement", "Relative binary offset", "Button"},
                     1);
    addAndMakeVisible(mode);

    minimumLabel.setText("Minimum", juce::dontSendNotification);
    maximumLabel.setText("Maximum", juce::dontSendNotification);
    pickupLabel.setText("Pickup tolerance", juce::dontSendNotification);
    for (auto* label : {&minimumLabel, &maximumLabel, &pickupLabel})
        addAndMakeVisible(*label);
    for (auto* editor : {&minimum, &maximum, &pickup}) {
        editor->setInputRestrictions(8, "-.0123456789");
        addAndMakeVisible(*editor);
    }
    addAndMakeVisible(inverted);
    addAndMakeVisible(apply);
    addAndMakeVisible(remove);
    apply.onClick = [this] { applySelection(); };
    remove.onClick = [this] { deleteSelection(); };
    refreshList();
    setSize(580, 300);
}

void MidiMappingEditor::refreshList(const int preferredIndex) {
    mappingSelector.clear(juce::dontSendNotification);
    const auto& mappings = midiMapper.mappings();
    for (size_t index = 0; index < mappings.size(); ++index) {
        const auto& mapping = mappings[index];
        mappingSelector.addItem(
            juce::String(mapping.parameterId) + "  |  Ch " + juce::String(mapping.channel) +
                (mapping.note ? " Note " : " CC ") + juce::String(mapping.number),
            static_cast<int>(index) + 1);
    }
    if (!mappings.empty())
        mappingSelector.setSelectedItemIndex(
            std::clamp(preferredIndex, 0, static_cast<int>(mappings.size()) - 1),
            juce::sendNotification);
    else
        messageInfo.setText("No mappings. Touch a control, click MIDI LEARN, then move hardware.",
                            juce::dontSendNotification);
}

void MidiMappingEditor::loadSelection() {
    const auto index = mappingSelector.getSelectedItemIndex();
    const auto& mappings = midiMapper.mappings();
    if (index < 0 || index >= static_cast<int>(mappings.size()))
        return;
    const auto& mapping = mappings[static_cast<size_t>(index)];
    messageInfo.setText(mapping.note ? "MIDI note mapping" : "MIDI CC mapping",
                        juce::dontSendNotification);
    channel.setSelectedId(mapping.channel, juce::dontSendNotification);
    mode.setSelectedItemIndex(static_cast<int>(mapping.mode), juce::dontSendNotification);
    minimum.setText(juce::String(mapping.minimum, 3), false);
    maximum.setText(juce::String(mapping.maximum, 3), false);
    pickup.setText(juce::String(mapping.pickupTolerance, 3), false);
    inverted.setToggleState(mapping.inverted, juce::dontSendNotification);
}

void MidiMappingEditor::applySelection() {
    const auto index = mappingSelector.getSelectedItemIndex();
    auto mappings = midiMapper.mappings();
    if (index < 0 || index >= static_cast<int>(mappings.size()))
        return;
    auto& mapping = mappings[static_cast<size_t>(index)];
    mapping.channel = std::clamp(channel.getSelectedId(), 1, 16);
    mapping.mode = static_cast<MidiMode>(std::clamp(mode.getSelectedItemIndex(), 0, 3));
    mapping.minimum = std::clamp(minimum.getText().getFloatValue(), -1.0F, 2.0F);
    mapping.maximum = std::clamp(maximum.getText().getFloatValue(), -1.0F, 2.0F);
    if (mapping.maximum < mapping.minimum)
        std::swap(mapping.minimum, mapping.maximum);
    mapping.pickupTolerance = std::clamp(pickup.getText().getFloatValue(), 0.001F, 0.5F);
    mapping.inverted = inverted.getToggleState();
    mapping.pickedUp = false;
    midiMapper.setMappings(std::move(mappings));
    refreshList(index);
}

void MidiMappingEditor::deleteSelection() {
    const auto index = mappingSelector.getSelectedItemIndex();
    auto mappings = midiMapper.mappings();
    if (index < 0 || index >= static_cast<int>(mappings.size()))
        return;
    mappings.erase(mappings.begin() + index);
    midiMapper.setMappings(std::move(mappings));
    refreshList(std::max(0, index - 1));
}

void MidiMappingEditor::resized() {
    auto area = getLocalBounds().reduced(16);
    title.setBounds(area.removeFromTop(30));
    mappingSelector.setBounds(area.removeFromTop(34));
    messageInfo.setBounds(area.removeFromTop(26));
    auto firstRow = area.removeFromTop(38);
    channelLabel.setBounds(firstRow.removeFromLeft(70));
    channel.setBounds(firstRow.removeFromLeft(90).reduced(3));
    modeLabel.setBounds(firstRow.removeFromLeft(110));
    mode.setBounds(firstRow.removeFromLeft(210).reduced(3));
    inverted.setBounds(firstRow);
    auto secondRow = area.removeFromTop(42);
    minimumLabel.setBounds(secondRow.removeFromLeft(70));
    minimum.setBounds(secondRow.removeFromLeft(90).reduced(3));
    maximumLabel.setBounds(secondRow.removeFromLeft(70));
    maximum.setBounds(secondRow.removeFromLeft(90).reduced(3));
    pickupLabel.setBounds(secondRow.removeFromLeft(110));
    pickup.setBounds(secondRow.reduced(3));
    auto buttons = area.removeFromBottom(38);
    remove.setBounds(buttons.removeFromLeft(120).reduced(3));
    apply.setBounds(buttons.removeFromRight(120).reduced(3));
}
} // namespace qb
