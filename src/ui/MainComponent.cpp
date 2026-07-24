#include "ui/MainComponent.h"
#include "dsp/BeatClock.h"
#include <chrono>

namespace qb {
namespace {
void addLabelled(juce::Component& owner, juce::Slider& slider, const juce::String& name) {
    slider.setName(name);
    slider.setTooltip(name);
    owner.addAndMakeVisible(slider);
}
} // namespace

MainComponent::MainComponent(MixerEngine& mixer, TempoEngine& tempo,
                             juce::AudioDeviceManager& devices)
    : engine(mixer), tempoEngine(tempo), deviceManager(devices) {
    setLookAndFeel(&lookAndFeel);
    setWantsKeyboardFocus(true);
    productLabel.setText("QUADBEAT FX", juce::dontSendNotification);
    productLabel.setFont(juce::FontOptions(24.0F, juce::Font::bold));
    productLabel.setColour(juce::Label::textColourId, QuadBeatLookAndFeel::accent());
    addAndMakeVisible(productLabel);
    addAndMakeVisible(statusLabel);
    addAndMakeVisible(settingsButton);
    addAndMakeVisible(midiButton);
    settingsButton.onClick = [this] { showSettings(); };
    midiButton.onClick = [this] {
        midiLearning = !midiLearning;
        statusLabel.setText(midiLearning ? "Learning " + juce::String(lastLearnTarget)
                                         : "MIDI learn cancelled",
                            juce::dontSendNotification);
    };
    midiDevices = juce::MidiInput::getAvailableDevices();
    for (const auto& device : midiDevices)
        deviceManager.addMidiInputDeviceCallback(device.identifier, this);
    for (int index = 0; index < channelCount; ++index) {
        auto& controls = channelControls[static_cast<size_t>(index)];
        controls.title.setText("CHANNEL " + juce::String(index + 1), juce::dontSendNotification);
        controls.title.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(controls.title);
        configureKnob(controls.trim, " dB");
        configureKnob(controls.high);
        configureKnob(controls.mid);
        configureKnob(controls.low);
        controls.fader.setSliderStyle(juce::Slider::LinearVertical);
        controls.fader.setRange(0.0, 1.0, 0.001);
        controls.fader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        for (auto* control :
             {&controls.trim, &controls.high, &controls.mid, &controls.low, &controls.fader})
            addAndMakeVisible(*control);
        controls.cue.setClickingTogglesState(true);
        controls.mute.setClickingTogglesState(true);
        addAndMakeVisible(controls.cue);
        addAndMakeVisible(controls.mute);
        controls.assignment.addItemList({"A", "B", "THRU"}, 1);
        controls.eqMode.addItemList({"EQ", "ISOLATOR"}, 1);
        addAndMakeVisible(controls.assignment);
        addAndMakeVisible(controls.eqMode);
        bindChannel(index);
    }
    for (auto* slider : {&master, &booth, &headphones, &cueMix}) {
        configureKnob(*slider);
        slider->setRange(0.0, 1.0, 0.001);
        addAndMakeVisible(*slider);
    }
    crossfader.setSliderStyle(juce::Slider::LinearHorizontal);
    crossfader.setRange(-1.0, 1.0, 0.001);
    crossfader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    crossfader.onValueChange = [this] {
        lastLearnTarget = "crossfader";
        engine.crossfader.store(static_cast<float>(crossfader.getValue()));
    };
    addAndMakeVisible(crossfader);
    master.onValueChange = [this] {
        lastLearnTarget = "master";
        engine.masterLevel.store(static_cast<float>(master.getValue()));
    };
    booth.onValueChange = [this] {
        lastLearnTarget = "booth";
        engine.boothLevel.store(static_cast<float>(booth.getValue()));
    };
    headphones.onValueChange = [this] {
        lastLearnTarget = "headphones";
        engine.headphonesLevel.store(static_cast<float>(headphones.getValue()));
    };
    cueMix.onValueChange = [this] {
        lastLearnTarget = "cueMix";
        engine.cueMix.store(static_cast<float>(cueMix.getValue()));
    };

    for (const auto name : effectNames)
        effectSelector.addItem(juce::String(name.data(), name.size()),
                               effectSelector.getNumItems() + 1);
    effectSelector.setSelectedId(2);
    effectSelector.onChange = [this] {
        lastLearnTarget = "effect";
        updateEffect();
    };
    busSelector.addItemList({"MIC", "CH1", "CH2", "CH3", "CH4", "XFADE A", "XFADE B", "MASTER"}, 1);
    busSelector.setSelectedId(8);
    busSelector.onChange = [this] {
        lastLearnTarget = "effectBus";
        engine.effectBus.store(static_cast<EffectBus>(busSelector.getSelectedItemIndex()));
    };
    addAndMakeVisible(effectSelector);
    addAndMakeVisible(busSelector);
    configureKnob(time);
    configureKnob(depth);
    time.onValueChange = [this] {
        lastLearnTarget = "effectTime";
        updateEffect();
    };
    depth.onValueChange = [this] {
        lastLearnTarget = "effectDepth";
        updateEffect();
    };
    addAndMakeVisible(time);
    addAndMakeVisible(depth);
    display.setJustificationType(juce::Justification::centred);
    display.setColour(juce::Label::backgroundColourId, juce::Colour(0xff061917));
    display.setColour(juce::Label::textColourId, QuadBeatLookAndFeel::accent());
    display.setFont(juce::FontOptions(18.0F, juce::Font::bold));
    addAndMakeVisible(display);
    beatLeft.onClick = [this] {
        lastLearnTarget = "beatDivision";
        selectedDivision = BeatClock::previousDivision(selectedDivision);
        updateEffect();
    };
    beatRight.onClick = [this] {
        lastLearnTarget = "beatDivision";
        selectedDivision = BeatClock::nextDivision(selectedDivision);
        updateEffect();
    };
    addAndMakeVisible(beatLeft);
    addAndMakeVisible(beatRight);
    for (size_t index = 0; index < pads.size(); ++index) {
        pads[index].setButtonText(juce::String(beatLabels[index].data(), beatLabels[index].size()));
        pads[index].setClickingTogglesState(false);
        pads[index].onClick = [this, index] {
            lastLearnTarget = "beatDivision";
            selectedDivision = static_cast<int>(index);
            updateEffect();
        };
        addAndMakeVisible(pads[index]);
    }
    for (auto* button :
         {&autoButton, &tapButton, &quantizeButton, &lowButton, &midButton, &highButton}) {
        button->setClickingTogglesState(true);
        addAndMakeVisible(*button);
    }
    autoButton.setToggleState(true, juce::dontSendNotification);
    quantizeButton.setToggleState(true, juce::dontSendNotification);
    lowButton.setToggleState(true, juce::dontSendNotification);
    midButton.setToggleState(true, juce::dontSendNotification);
    highButton.setToggleState(true, juce::dontSendNotification);
    autoButton.onClick = [this] {
        tempoEngine.setSource(autoButton.getToggleState() ? TempoSource::automatic
                                                          : TempoSource::manual);
        updateEffect();
    };
    tapButton.onClick = [this] {
        lastLearnTarget = "tap";
        const auto now =
            std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch())
                .count();
        tempoEngine.setSource(TempoSource::tap);
        tempoEngine.tap(now);
        updateEffect();
    };
    quantizeButton.onClick = [this] {
        lastLearnTarget = "quantize";
        quantized = quantizeButton.getToggleState();
    };
    lowButton.onClick = midButton.onClick = highButton.onClick = [this] {
        lastLearnTarget = "fxBands";
        if (!lowButton.getToggleState() && !midButton.getToggleState() &&
            !highButton.getToggleState())
            midButton.setToggleState(true, juce::dontSendNotification);
        updateEffect();
    };
    time.setValue(0.5);
    depth.setValue(0.5);
    master.setValue(0.8);
    booth.setValue(0.7);
    headphones.setValue(0.7);
    cueMix.setValue(0.5);
    startTimerHz(30);
    updateEffect();
}

MainComponent::~MainComponent() {
    for (const auto& device : midiDevices)
        deviceManager.removeMidiInputDeviceCallback(device.identifier, this);
    settingsWindow.reset();
    setLookAndFeel(nullptr);
}

void MainComponent::configureKnob(juce::Slider& slider, const juce::String suffix) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setRange(-1.0, 1.0, 0.001);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 54, 18);
    slider.setTextValueSuffix(suffix);
    slider.setDoubleClickReturnValue(true, 0.0);
}

void MainComponent::bindChannel(const int index) {
    auto& ui = channelControls[static_cast<size_t>(index)];
    auto& parameters = engine.channels[static_cast<size_t>(index)];
    ui.fader.setValue(1.0);
    ui.assignment.setSelectedId(3);
    ui.eqMode.setSelectedId(1);
    ui.trim.onValueChange = [this, index, &ui, &parameters] {
        lastLearnTarget = "ch" + std::to_string(index + 1) + ".trim";
        parameters.trim.store(static_cast<float>(ui.trim.getValue()));
    };
    ui.low.onValueChange = [this, index, &ui, &parameters] {
        lastLearnTarget = "ch" + std::to_string(index + 1) + ".low";
        parameters.low.store(static_cast<float>(ui.low.getValue()));
    };
    ui.mid.onValueChange = [this, index, &ui, &parameters] {
        lastLearnTarget = "ch" + std::to_string(index + 1) + ".mid";
        parameters.mid.store(static_cast<float>(ui.mid.getValue()));
    };
    ui.high.onValueChange = [this, index, &ui, &parameters] {
        lastLearnTarget = "ch" + std::to_string(index + 1) + ".high";
        parameters.high.store(static_cast<float>(ui.high.getValue()));
    };
    ui.fader.onValueChange = [this, index, &ui, &parameters] {
        lastLearnTarget = "ch" + std::to_string(index + 1) + ".fader";
        parameters.fader.store(static_cast<float>(ui.fader.getValue()));
    };
    ui.cue.onClick = [this, index, &ui, &parameters] {
        lastLearnTarget = "ch" + std::to_string(index + 1) + ".cue";
        parameters.cue.store(ui.cue.getToggleState());
    };
    ui.mute.onClick = [this, index, &ui, &parameters] {
        lastLearnTarget = "ch" + std::to_string(index + 1) + ".mute";
        parameters.mute.store(ui.mute.getToggleState());
    };
    ui.assignment.onChange = [this, index, &ui, &parameters] {
        lastLearnTarget = "ch" + std::to_string(index + 1) + ".assignment";
        parameters.assignment.store(
            static_cast<CrossfaderAssignment>(ui.assignment.getSelectedItemIndex()));
    };
    ui.eqMode.onChange = [&ui, &parameters] {
        parameters.eqMode.store(static_cast<EqMode>(ui.eqMode.getSelectedItemIndex()));
    };
}

void MainComponent::updateEffect() {
    EffectParameters parameters;
    parameters.type = static_cast<EffectType>(std::max(0, effectSelector.getSelectedItemIndex()));
    parameters.time = static_cast<float>((time.getValue() + 1.0) * 0.5);
    parameters.depth = static_cast<float>((depth.getValue() + 1.0) * 0.5);
    parameters.bpm = tempoEngine.bpm();
    parameters.division = selectedDivision;
    parameters.enabled = depth.getValue() > -0.99;
    parameters.low = lowButton.getToggleState();
    parameters.mid = midButton.getToggleState();
    parameters.high = highButton.getToggleState();
    engine.effectRack.setParameters(parameters);
    for (size_t index = 0; index < pads.size(); ++index)
        pads[index].setToggleState(static_cast<int>(index) == selectedDivision,
                                   juce::dontSendNotification);
    const auto source = tempoEngine.source() == TempoSource::automatic ? "AUTO"
                        : tempoEngine.source() == TempoSource::manual  ? "MANUAL"
                                                                       : "TAP";
    display.setText(effectSelector.getText() + "\n" + juce::String(tempoEngine.bpm(), 1) +
                        " BPM  " + source + "  " +
                        juce::String(beatLabels[static_cast<size_t>(selectedDivision)].data(),
                                     beatLabels[static_cast<size_t>(selectedDivision)].size()),
                    juce::dontSendNotification);
}

void MainComponent::paint(juce::Graphics& graphics) {
    graphics.fillAll(QuadBeatLookAndFeel::background());
    const auto channelArea = getLocalBounds().reduced(12).withTrimmedTop(58);
    graphics.setColour(juce::Colour(0xff26333f));
    for (int index = 0; index < channelCount; ++index) {
        const auto x = 12 + index * ((getWidth() * 58 / 100 - 18) / 4);
        graphics.drawRoundedRectangle(
            juce::Rectangle<float>(static_cast<float>(x), 70.0F,
                                   static_cast<float>((getWidth() * 58 / 100 - 24) / 4 - 4),
                                   static_cast<float>(getHeight() - 135)),
            8.0F, 1.0F);
    }
    const auto meterX = getWidth() * 58 / 100 - 20;
    for (int channel = 0; channel < channelCount; ++channel) {
        const auto x = 20 + channel * ((meterX - 20) / 4);
        const auto height = static_cast<float>(getHeight() - 160) *
                            channelControls[static_cast<size_t>(channel)].meter;
        graphics.setColour(QuadBeatLookAndFeel::accent());
        graphics.fillRect(static_cast<float>(x + 8), static_cast<float>(getHeight() - 84) - height,
                          3.0F, height);
    }
    juce::ignoreUnused(channelArea);
}

void MainComponent::layoutChannel(ChannelControls& channel, juce::Rectangle<int> area) {
    channel.title.setBounds(area.removeFromTop(28));
    channel.trim.setBounds(area.removeFromTop(78));
    channel.high.setBounds(area.removeFromTop(78));
    channel.mid.setBounds(area.removeFromTop(78));
    channel.low.setBounds(area.removeFromTop(78));
    channel.eqMode.setBounds(area.removeFromTop(28).reduced(4, 2));
    auto buttons = area.removeFromTop(34);
    channel.cue.setBounds(buttons.removeFromLeft(buttons.getWidth() / 2).reduced(3));
    channel.mute.setBounds(buttons.reduced(3));
    channel.fader.setBounds(area.removeFromTop(std::max(100, area.getHeight() - 44)).reduced(8));
    channel.assignment.setBounds(area.removeFromTop(30).reduced(4, 2));
}

void MainComponent::resized() {
    auto bounds = getLocalBounds().reduced(12);
    auto header = bounds.removeFromTop(46);
    productLabel.setBounds(header.removeFromLeft(230));
    settingsButton.setBounds(header.removeFromRight(100).reduced(2));
    midiButton.setBounds(header.removeFromRight(110).reduced(2));
    statusLabel.setBounds(header);
    auto footer = bounds.removeFromBottom(58);
    crossfader.setBounds(footer.removeFromLeft(getWidth() * 58 / 100).reduced(20, 5));
    auto mixerArea = bounds.removeFromLeft(getWidth() * 58 / 100);
    const auto stripWidth = mixerArea.getWidth() / channelCount;
    for (auto& channel : channelControls)
        layoutChannel(channel, mixerArea.removeFromLeft(stripWidth).reduced(5));
    auto fxArea = bounds.reduced(10, 2);
    display.setBounds(fxArea.removeFromTop(72));
    auto padArea = fxArea.removeFromTop(88).reduced(0, 5);
    for (int index = 0; index < 8; ++index)
        pads[static_cast<size_t>(index)].setBounds(
            padArea.removeFromLeft(padArea.getWidth() / (8 - index)).reduced(2));
    auto arrows = fxArea.removeFromTop(38);
    beatLeft.setBounds(arrows.removeFromLeft(50).reduced(2));
    beatRight.setBounds(arrows.removeFromRight(50).reduced(2));
    autoButton.setBounds(arrows.removeFromLeft(arrows.getWidth() / 3).reduced(2));
    tapButton.setBounds(arrows.removeFromLeft(arrows.getWidth() / 2).reduced(2));
    quantizeButton.setBounds(arrows.reduced(2));
    auto bandsArea = fxArea.removeFromTop(42);
    lowButton.setBounds(bandsArea.removeFromLeft(bandsArea.getWidth() / 3).reduced(3));
    midButton.setBounds(bandsArea.removeFromLeft(bandsArea.getWidth() / 2).reduced(3));
    highButton.setBounds(bandsArea.reduced(3));
    effectSelector.setBounds(fxArea.removeFromTop(34).reduced(3));
    busSelector.setBounds(fxArea.removeFromTop(34).reduced(3));
    auto knobArea = fxArea.removeFromTop(110);
    time.setBounds(knobArea.removeFromLeft(knobArea.getWidth() / 2).reduced(8));
    depth.setBounds(knobArea.reduced(8));
    auto monitor = fxArea;
    master.setName("MASTER");
    booth.setName("BOOTH");
    headphones.setName("PHONES");
    cueMix.setName("CUE MIX");
    const auto monitorWidth = monitor.getWidth() / 4;
    for (auto* knob : {&master, &booth, &headphones, &cueMix})
        knob->setBounds(monitor.removeFromLeft(monitorWidth).reduced(4));
}

void MainComponent::timerCallback() {
    for (int channel = 0; channel < channelCount; ++channel) {
        const auto value = std::max(engine.channelPeak(channel, 0), engine.channelPeak(channel, 1));
        auto& meter = channelControls[static_cast<size_t>(channel)].meter;
        meter = std::max(value, meter * 0.9F);
    }
    if (auto* device = deviceManager.getCurrentAudioDevice())
        statusLabel.setText(device->getName() + "  |  " +
                                juce::String(device->getCurrentSampleRate(), 0) + " Hz  |  " +
                                juce::String(device->getCurrentBufferSizeSamples()) + " samples",
                            juce::dontSendNotification);
    updateEffect();
    repaint();
}

bool MainComponent::keyPressed(const juce::KeyPress& key) {
    if (key == juce::KeyPress::leftKey) {
        beatLeft.triggerClick();
        return true;
    }
    if (key == juce::KeyPress::rightKey) {
        beatRight.triggerClick();
        return true;
    }
    if (key.getTextCharacter() == 't' || key.getTextCharacter() == 'T') {
        tapButton.triggerClick();
        return true;
    }
    return false;
}

void MainComponent::showSettings() {
    auto* selector =
        new juce::AudioDeviceSelectorComponent(deviceManager, 0, 9, 2, 6, true, true, true, false);
    selector->setSize(680, 560);
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(selector);
    options.dialogTitle = "QuadBeat FX Settings";
    options.dialogBackgroundColour = QuadBeatLookAndFeel::panel();
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;
    options.launchAsync();
}

void MainComponent::handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message) {
    juce::MessageManager::callAsync(
        [safe = juce::Component::SafePointer<MainComponent>(this), message] {
            if (safe != nullptr)
                safe->handleMidiOnMessageThread(message);
        });
}

void MainComponent::handleMidiOnMessageThread(const juce::MidiMessage message) {
    if (!message.isController() && !message.isNoteOnOrOff())
        return;
    if (midiLearning) {
        MidiMapping mapping;
        mapping.parameterId = lastLearnTarget;
        mapping.channel = message.getChannel();
        mapping.note = message.isNoteOnOrOff();
        mapping.number = mapping.note ? message.getNoteNumber() : message.getControllerNumber();
        mapping.mode = mapping.note || lastLearnTarget.find("cue") != std::string::npos ||
                               lastLearnTarget.find("mute") != std::string::npos ||
                               lastLearnTarget == "tap" || lastLearnTarget == "quantize"
                           ? MidiMode::button
                           : MidiMode::absolute;
        auto mappings = midiMapper.mappings();
        mappings.erase(std::remove_if(mappings.begin(), mappings.end(),
                                      [&](const auto& existing) {
                                          return existing.parameterId == mapping.parameterId;
                                      }),
                       mappings.end());
        mappings.push_back(mapping);
        midiMapper.setMappings(std::move(mappings));
        midiLearning = false;
        statusLabel.setText("Mapped " + juce::String(lastLearnTarget), juce::dontSendNotification);
        return;
    }
    for (const auto& mapping : midiMapper.mappings()) {
        const auto note = message.isNoteOnOrOff();
        const auto number = note ? message.getNoteNumber() : message.getControllerNumber();
        if (mapping.channel != message.getChannel() || mapping.note != note ||
            mapping.number != number)
            continue;
        const auto result = midiMapper.process(message, parameterValue(mapping.parameterId));
        if (result) {
            setParameterValue(result->first, result->second);
            break;
        }
    }
}

float MainComponent::parameterValue(const std::string& id) const {
    if (id == "master")
        return static_cast<float>(master.getValue());
    if (id == "booth")
        return static_cast<float>(booth.getValue());
    if (id == "headphones")
        return static_cast<float>(headphones.getValue());
    if (id == "cueMix")
        return static_cast<float>(cueMix.getValue());
    if (id == "crossfader")
        return static_cast<float>((crossfader.getValue() + 1.0) * 0.5);
    if (id == "effectTime")
        return static_cast<float>((time.getValue() + 1.0) * 0.5);
    if (id == "effectDepth")
        return static_cast<float>((depth.getValue() + 1.0) * 0.5);
    if (id == "effect")
        return static_cast<float>(effectSelector.getSelectedItemIndex()) / 14.0F;
    if (id == "effectBus")
        return static_cast<float>(busSelector.getSelectedItemIndex()) / 7.0F;
    if (id == "beatDivision")
        return static_cast<float>(selectedDivision) / 7.0F;
    if (id == "quantize")
        return quantized ? 1.0F : 0.0F;
    for (int index = 0; index < channelCount; ++index) {
        const auto prefix = "ch" + std::to_string(index + 1) + ".";
        if (!id.starts_with(prefix))
            continue;
        const auto control = id.substr(prefix.size());
        const auto& channel = channelControls[static_cast<size_t>(index)];
        if (control == "fader")
            return static_cast<float>(channel.fader.getValue());
        if (control == "cue")
            return channel.cue.getToggleState() ? 1.0F : 0.0F;
        if (control == "mute")
            return channel.mute.getToggleState() ? 1.0F : 0.0F;
        if (control == "assignment")
            return static_cast<float>(channel.assignment.getSelectedItemIndex()) / 2.0F;
        const auto* slider = control == "trim"   ? &channel.trim
                             : control == "low"  ? &channel.low
                             : control == "mid"  ? &channel.mid
                             : control == "high" ? &channel.high
                                                 : nullptr;
        if (slider != nullptr)
            return static_cast<float>((slider->getValue() + 1.0) * 0.5);
    }
    return 0.0F;
}

void MainComponent::setParameterValue(const std::string& id, const float value) {
    const auto normalised = std::clamp(value, 0.0F, 1.0F);
    if (id == "master")
        master.setValue(normalised);
    else if (id == "booth")
        booth.setValue(normalised);
    else if (id == "headphones")
        headphones.setValue(normalised);
    else if (id == "cueMix")
        cueMix.setValue(normalised);
    else if (id == "crossfader")
        crossfader.setValue(normalised * 2.0F - 1.0F);
    else if (id == "effectTime")
        time.setValue(normalised * 2.0F - 1.0F);
    else if (id == "effectDepth")
        depth.setValue(normalised * 2.0F - 1.0F);
    else if (id == "effect")
        effectSelector.setSelectedItemIndex(
            std::clamp(static_cast<int>(std::round(normalised * 14.0F)), 0, 14));
    else if (id == "effectBus")
        busSelector.setSelectedItemIndex(
            std::clamp(static_cast<int>(std::round(normalised * 7.0F)), 0, 7));
    else if (id == "beatDivision") {
        selectedDivision = std::clamp(static_cast<int>(std::round(normalised * 7.0F)), 0, 7);
        updateEffect();
    } else if (id == "tap" && normalised > 0.5F)
        tapButton.triggerClick();
    else if (id == "quantize")
        quantizeButton.setToggleState(normalised > 0.5F, juce::sendNotification);
    else {
        for (int index = 0; index < channelCount; ++index) {
            const auto prefix = "ch" + std::to_string(index + 1) + ".";
            if (!id.starts_with(prefix))
                continue;
            const auto control = id.substr(prefix.size());
            auto& channel = channelControls[static_cast<size_t>(index)];
            if (control == "fader")
                channel.fader.setValue(normalised);
            else if (control == "cue")
                channel.cue.setToggleState(normalised > 0.5F, juce::sendNotification);
            else if (control == "mute")
                channel.mute.setToggleState(normalised > 0.5F, juce::sendNotification);
            else if (control == "assignment")
                channel.assignment.setSelectedItemIndex(
                    std::clamp(static_cast<int>(std::round(normalised * 2.0F)), 0, 2));
            else {
                auto* slider = control == "trim"   ? &channel.trim
                               : control == "low"  ? &channel.low
                               : control == "mid"  ? &channel.mid
                               : control == "high" ? &channel.high
                                                   : nullptr;
                if (slider != nullptr)
                    slider->setValue(normalised * 2.0F - 1.0F);
            }
            break;
        }
    }
}

AppState MainComponent::captureState() const {
    AppState state;
    for (int index = 0; index < channelCount; ++index) {
        const auto& ui = channelControls[static_cast<size_t>(index)];
        auto& channel = state.channels[static_cast<size_t>(index)];
        channel.trim = static_cast<float>(ui.trim.getValue());
        channel.low = static_cast<float>(ui.low.getValue());
        channel.mid = static_cast<float>(ui.mid.getValue());
        channel.high = static_cast<float>(ui.high.getValue());
        channel.fader = static_cast<float>(ui.fader.getValue());
        channel.cue = ui.cue.getToggleState();
        channel.mute = ui.mute.getToggleState();
        channel.eqMode = static_cast<EqMode>(ui.eqMode.getSelectedItemIndex());
        channel.assignment =
            static_cast<CrossfaderAssignment>(ui.assignment.getSelectedItemIndex());
    }
    state.crossfader = static_cast<float>(crossfader.getValue());
    state.master = static_cast<float>(master.getValue());
    state.booth = static_cast<float>(booth.getValue());
    state.headphones = static_cast<float>(headphones.getValue());
    state.cueMix = static_cast<float>(cueMix.getValue());
    state.effect = static_cast<EffectType>(effectSelector.getSelectedItemIndex());
    state.effectBus = static_cast<EffectBus>(busSelector.getSelectedItemIndex());
    state.division = selectedDivision;
    state.effectTime = static_cast<float>((time.getValue() + 1.0) * 0.5);
    state.effectDepth = static_cast<float>((depth.getValue() + 1.0) * 0.5);
    state.lowBand = lowButton.getToggleState();
    state.midBand = midButton.getToggleState();
    state.highBand = highButton.getToggleState();
    state.quantize = quantized;
    state.tempoSource = tempoEngine.source();
    state.manualBpm = tempoEngine.bpm();
    state.midiMappings = midiMapper.serialise();
    return state;
}

void MainComponent::restoreState(const AppState& state) {
    for (int index = 0; index < channelCount; ++index) {
        auto& ui = channelControls[static_cast<size_t>(index)];
        const auto& channel = state.channels[static_cast<size_t>(index)];
        ui.trim.setValue(channel.trim);
        ui.low.setValue(channel.low);
        ui.mid.setValue(channel.mid);
        ui.high.setValue(channel.high);
        ui.fader.setValue(channel.fader);
        ui.cue.setToggleState(channel.cue, juce::sendNotification);
        ui.mute.setToggleState(channel.mute, juce::sendNotification);
        ui.eqMode.setSelectedItemIndex(static_cast<int>(channel.eqMode));
        ui.assignment.setSelectedItemIndex(static_cast<int>(channel.assignment));
    }
    crossfader.setValue(state.crossfader);
    master.setValue(state.master);
    booth.setValue(state.booth);
    headphones.setValue(state.headphones);
    cueMix.setValue(state.cueMix);
    effectSelector.setSelectedItemIndex(static_cast<int>(state.effect));
    busSelector.setSelectedItemIndex(static_cast<int>(state.effectBus));
    selectedDivision = state.division;
    time.setValue(state.effectTime * 2.0F - 1.0F);
    depth.setValue(state.effectDepth * 2.0F - 1.0F);
    lowButton.setToggleState(state.lowBand, juce::dontSendNotification);
    midButton.setToggleState(state.midBand, juce::dontSendNotification);
    highButton.setToggleState(state.highBand, juce::dontSendNotification);
    quantized = state.quantize;
    quantizeButton.setToggleState(quantized, juce::dontSendNotification);
    tempoEngine.setSource(state.tempoSource);
    tempoEngine.setManualBpm(state.manualBpm);
    midiMapper.deserialise(state.midiMappings);
    updateEffect();
}
} // namespace qb
