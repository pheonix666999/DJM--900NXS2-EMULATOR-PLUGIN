#include "ui/SettingsComponent.h"

namespace qb {
namespace {
constexpr std::array<const char*, 9> inputNames{"CH1 Left",  "CH1 Right", "CH2 Left",
                                                "CH2 Right", "CH3 Left",  "CH3 Right",
                                                "CH4 Left",  "CH4 Right", "Microphone"};
constexpr std::array<const char*, 6> outputNames{"Master Left",     "Master Right",
                                                 "Booth Left",      "Booth Right",
                                                 "Headphones Left", "Headphones Right"};
} // namespace

SettingsComponent::SettingsComponent(MixerEngine& engine, juce::AudioDeviceManager& devices)
    : mixer(engine), deviceManager(devices) {
    deviceSelector = std::make_unique<juce::AudioDeviceSelectorComponent>(
        deviceManager, 0, 9, 2, 32, true, true, true, false);
    addAndMakeVisible(*deviceSelector);
    routingTitle.setText("PHYSICAL CHANNEL ROUTING", juce::dontSendNotification);
    routingTitle.setFont(juce::FontOptions(16.0F, juce::Font::bold));
    addAndMakeVisible(routingTitle);

    for (size_t index = 0; index < inputChoices.size(); ++index) {
        routingLabels[index].setText(inputNames[index], juce::dontSendNotification);
        addAndMakeVisible(routingLabels[index]);
        addAndMakeVisible(inputChoices[index]);
        inputChoices[index].onChange = [this, index] {
            const auto physical = inputChoices[index].getSelectedId() - 2;
            if (index < 8)
                mixer.setChannelInputMapping(static_cast<int>(index / 2),
                                             static_cast<int>(index % 2), physical);
            else
                mixer.setMicrophoneInputMapping(physical);
        };
    }
    for (size_t index = 0; index < outputChoices.size(); ++index) {
        auto& label = routingLabels[inputChoices.size() + index];
        label.setText(outputNames[index], juce::dontSendNotification);
        addAndMakeVisible(label);
        addAndMakeVisible(outputChoices[index]);
        outputChoices[index].onChange = [this, index] {
            mixer.setOutputMapping(static_cast<int>(index / 2), static_cast<int>(index % 2),
                                   outputChoices[index].getSelectedId() - 2);
        };
    }

    microphoneLevelLabel.setText("Microphone level", juce::dontSendNotification);
    addAndMakeVisible(microphoneLevelLabel);
    microphoneLevel.setRange(0.0, 2.0, 0.01);
    microphoneLevel.setValue(mixer.microphoneLevel.load(), juce::dontSendNotification);
    microphoneLevel.setSliderStyle(juce::Slider::LinearHorizontal);
    microphoneLevel.setTextBoxStyle(juce::Slider::TextBoxRight, false, 55, 22);
    microphoneLevel.onValueChange = [this] {
        mixer.microphoneLevel.store(static_cast<float>(microphoneLevel.getValue()));
    };
    addAndMakeVisible(microphoneLevel);
    microphoneMute.setToggleState(mixer.microphoneMute.load(), juce::dontSendNotification);
    microphoneMute.onClick = [this] {
        mixer.microphoneMute.store(microphoneMute.getToggleState());
    };
    addAndMakeVisible(microphoneMute);
    microphoneCue.setToggleState(mixer.microphoneCue.load(), juce::dontSendNotification);
    microphoneCue.onClick = [this] { mixer.microphoneCue.store(microphoneCue.getToggleState()); };
    addAndMakeVisible(microphoneCue);

    analysisLabel.setText("AUTO BPM source", juce::dontSendNotification);
    addAndMakeVisible(analysisLabel);
    analysisSource.addItemList({"MASTER", "CH1", "CH2", "CH3", "CH4", "MIC"}, 1);
    analysisSource.setSelectedItemIndex(static_cast<int>(mixer.analysisSource.load()),
                                        juce::dontSendNotification);
    analysisSource.onChange = [this] {
        mixer.analysisSource.store(
            static_cast<TempoAnalysisSource>(analysisSource.getSelectedItemIndex()));
    };
    addAndMakeVisible(analysisSource);
    refreshRoutingChoices();
    startTimerHz(2);
    setSize(760, 840);
}

void SettingsComponent::fillChannelChoices(juce::ComboBox& box, const int available,
                                           const int selected) {
    box.clear(juce::dontSendNotification);
    box.addItem("Unavailable", 1);
    for (int channel = 0; channel < available; ++channel)
        box.addItem("Physical " + juce::String(channel + 1), channel + 2);
    box.setSelectedId(selected >= 0 && selected < available ? selected + 2 : 1,
                      juce::dontSendNotification);
}

void SettingsComponent::refreshRoutingChoices() {
    auto* device = deviceManager.getCurrentAudioDevice();
    const auto inputCount = device != nullptr ? device->getInputChannelNames().size() : 0;
    const auto outputCount = device != nullptr ? device->getOutputChannelNames().size() : 0;
    if (inputCount == lastInputCount && outputCount == lastOutputCount)
        return;
    lastInputCount = inputCount;
    lastOutputCount = outputCount;
    for (size_t index = 0; index < inputChoices.size(); ++index) {
        const auto selected = index < 8 ? mixer.channelInputMapping(static_cast<int>(index / 2),
                                                                    static_cast<int>(index % 2))
                                        : mixer.microphoneInputMapping();
        fillChannelChoices(inputChoices[index], inputCount, selected);
    }
    for (size_t index = 0; index < outputChoices.size(); ++index)
        fillChannelChoices(
            outputChoices[index], outputCount,
            mixer.outputMapping(static_cast<int>(index / 2), static_cast<int>(index % 2)));
}

void SettingsComponent::timerCallback() { refreshRoutingChoices(); }

void SettingsComponent::resized() {
    auto area = getLocalBounds().reduced(12);
    deviceSelector->setBounds(area.removeFromTop(390));
    routingTitle.setBounds(area.removeFromTop(28));
    auto columns = area.removeFromTop(278);
    auto inputs = columns.removeFromLeft(columns.getWidth() / 2).reduced(4);
    auto outputs = columns.reduced(4);
    for (size_t index = 0; index < inputChoices.size(); ++index) {
        auto row = inputs.removeFromTop(30);
        routingLabels[index].setBounds(row.removeFromLeft(112));
        inputChoices[index].setBounds(row);
    }
    for (size_t index = 0; index < outputChoices.size(); ++index) {
        auto row = outputs.removeFromTop(30);
        routingLabels[inputChoices.size() + index].setBounds(row.removeFromLeft(125));
        outputChoices[index].setBounds(row);
    }
    auto micRow = area.removeFromTop(34);
    microphoneLevelLabel.setBounds(micRow.removeFromLeft(130));
    microphoneLevel.setBounds(micRow.removeFromLeft(230));
    microphoneMute.setBounds(micRow.removeFromLeft(150));
    microphoneCue.setBounds(micRow);
    auto analysisRow = area.removeFromTop(34);
    analysisLabel.setBounds(analysisRow.removeFromLeft(130));
    analysisSource.setBounds(analysisRow.removeFromLeft(180));
}
} // namespace qb
