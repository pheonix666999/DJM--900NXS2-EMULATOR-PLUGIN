#include "ui/MainComponent.h"
#include "dsp/BeatClock.h"
#include "ui/MidiMappingEditor.h"
#include "ui/SettingsComponent.h"
#include <chrono>

namespace qb {

MainComponent::MainComponent(MixerEngine& mixer, TempoEngine& tempo,
                             juce::AudioDeviceManager& devices, const bool isHostedByPlugin)
    : engine(mixer), tempoEngine(tempo), deviceManager(devices), hostedByPlugin(isHostedByPlugin) {
    setLookAndFeel(&lookAndFeel);
    setWantsKeyboardFocus(true);
    productLabel.setText("QUADBEAT FX", juce::dontSendNotification);
    productLabel.setFont(juce::FontOptions(20.0F, juce::Font::bold));
    productLabel.setColour(juce::Label::textColourId, QuadBeatLookAndFeel::accent());
    addAndMakeVisible(productLabel);
    addAndMakeVisible(statusLabel);
    statusLabel.setFont(juce::FontOptions(11.0F));
    settingsButton.setComponentID("utility");
    midiButton.setComponentID("utility");
    midiEditButton.setComponentID("utility");
    addAndMakeVisible(settingsButton);
    addAndMakeVisible(midiButton);
    addAndMakeVisible(midiEditButton);
    scaleSelector.addItemList({"75%", "100%", "125%", "150%", "200%"}, 1);
    scaleSelector.setSelectedId(2);
    scaleSelector.setTooltip("Interface scale");
    scaleSelector.onChange = [this] {
        constexpr std::array<float, 5> scales{0.75F, 1.0F, 1.25F, 1.5F, 2.0F};
        const auto index = std::clamp(scaleSelector.getSelectedItemIndex(), 0, 4);
        const auto scale = scales[static_cast<size_t>(index)];
        if (onScaleRequested != nullptr)
            onScaleRequested(scale);
        else
            juce::Desktop::getInstance().setGlobalScaleFactor(scale);
    };
    addAndMakeVisible(scaleSelector);
    settingsButton.onClick = [this] { showSettings(); };
    midiButton.onClick = [this] {
        midiLearning = !midiLearning;
        statusLabel.setText(midiLearning ? "Learning " + juce::String(lastLearnTarget)
                                         : "MIDI learn cancelled",
                            juce::dontSendNotification);
    };
    midiEditButton.onClick = [this] { showMidiEditor(); };
    if (!hostedByPlugin) {
        midiDevices = juce::MidiInput::getAvailableDevices();
        for (const auto& device : midiDevices)
            deviceManager.addMidiInputDeviceCallback(device.identifier, this);
    } else {
        settingsButton.setButtonText("HOST I/O");
        statusLabel.setText("VST3 HOST AUDIO", juce::dontSendNotification);
    }
    for (int index = 0; index < channelCount; ++index) {
        auto& controls = channelControls[static_cast<size_t>(index)];
        controls.title.setText("CHANNEL " + juce::String(index + 1), juce::dontSendNotification);
        controls.title.setJustificationType(juce::Justification::centred);
        controls.title.setFont(juce::FontOptions(12.0F, juce::Font::bold));
        addAndMakeVisible(controls.title);
        configureKnob(controls.trim, " dB");
        configureKnob(controls.high);
        configureKnob(controls.mid);
        configureKnob(controls.low);
        controls.trim.setName("TRIM");
        controls.high.setName("HI");
        controls.mid.setName("MID");
        controls.low.setName("LOW");
        controls.fader.setSliderStyle(juce::Slider::LinearVertical);
        controls.fader.setRange(0.0, 1.0, 0.001);
        controls.fader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        for (auto* control :
             {&controls.trim, &controls.high, &controls.mid, &controls.low, &controls.fader})
            addAndMakeVisible(*control);
        controls.cue.setClickingTogglesState(true);
        controls.mute.setClickingTogglesState(true);
        controls.cue.setComponentID("cue");
        controls.mute.setComponentID("mute");
        addAndMakeVisible(controls.cue);
        addAndMakeVisible(controls.mute);
        controls.assignment.addItemList({"A", "B", "THRU"}, 1);
        controls.eqMode.addItemList({"EQ", "ISOLATOR"}, 1);
        addAndMakeVisible(controls.assignment);
        addAndMakeVisible(controls.eqMode);
        bindChannel(index);
    }
    configureKnob(microphone);
    microphone.setName("MIC LEVEL");
    microphone.setRange(0.0, 2.0, 0.001);
    microphone.setDoubleClickReturnValue(true, 1.0);
    microphone.onValueChange = [this] {
        lastLearnTarget = "microphoneLevel";
        engine.microphoneLevel.store(static_cast<float>(microphone.getValue()));
    };
    microphoneCue.setClickingTogglesState(true);
    microphoneMute.setClickingTogglesState(true);
    microphoneCue.setComponentID("cue");
    microphoneMute.setComponentID("mute");
    microphoneCue.onClick = [this] {
        lastLearnTarget = "mic.cue";
        engine.microphoneCue.store(microphoneCue.getToggleState());
    };
    microphoneMute.onClick = [this] {
        lastLearnTarget = "mic.mute";
        engine.microphoneMute.store(microphoneMute.getToggleState());
    };
    addAndMakeVisible(microphone);
    addAndMakeVisible(microphoneCue);
    addAndMakeVisible(microphoneMute);
    for (auto* slider : {&master, &booth, &headphones, &cueMix}) {
        configureKnob(*slider);
        slider->setRange(0.0, 1.0, 0.001);
        addAndMakeVisible(*slider);
    }
    master.setName("MASTER");
    booth.setName("BOOTH MONITOR");
    headphones.setName("PHONES LEVEL");
    cueMix.setName("CUE / MASTER");
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
    effectSelector.setName("BEAT FX");
    effectSelector.setSelectedId(2);
    effectSelector.onChange = [this] {
        lastLearnTarget = "effect";
        updateEffect();
    };
    busSelector.addItemList({"MIC", "CH1", "CH2", "CH3", "CH4", "XFADE A", "XFADE B", "MASTER"}, 1);
    busSelector.setName("FX ASSIGN");
    busSelector.setSelectedId(8);
    busSelector.onChange = [this] {
        lastLearnTarget = "effectBus";
        engine.effectBus.store(static_cast<EffectBus>(busSelector.getSelectedItemIndex()));
    };
    addAndMakeVisible(effectSelector);
    addAndMakeVisible(busSelector);
    configureKnob(time);
    configureKnob(depth);
    time.setName("TIME");
    depth.setName("LEVEL / DEPTH");
    time.setComponentID("effectKnob");
    depth.setComponentID("effectKnob");
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
    display.onDivisionSelected = [this](const int division) {
        lastLearnTarget = "beatDivision";
        selectedDivision = division;
        updateEffect();
    };
    addAndMakeVisible(display);
    beatLeft.onClick = [this] {
        lastLearnTarget = "beatPrevious";
        selectedDivision = BeatClock::previousDivision(selectedDivision);
        updateEffect();
    };
    beatRight.onClick = [this] {
        lastLearnTarget = "beatNext";
        selectedDivision = BeatClock::nextDivision(selectedDivision);
        updateEffect();
    };
    addAndMakeVisible(beatLeft);
    addAndMakeVisible(beatRight);
    beatLeft.setComponentID("beatArrow");
    beatRight.setComponentID("beatArrow");
    for (size_t index = 0; index < pads.size(); ++index) {
        pads[index].setButtonText(juce::String(beatLabels[index].data(), beatLabels[index].size()));
        pads[index].setClickingTogglesState(false);
        pads[index].setComponentID("pad");
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
    tapButton.setClickingTogglesState(false);
    autoButton.setComponentID("autoTap");
    tapButton.setComponentID("tap");
    quantizeButton.setComponentID("quantize");
    lowButton.setComponentID("frequency");
    midButton.setComponentID("frequency");
    highButton.setComponentID("frequency");
    autoButton.setToggleState(true, juce::dontSendNotification);
    quantizeButton.setToggleState(true, juce::dontSendNotification);
    lowButton.setToggleState(true, juce::dontSendNotification);
    midButton.setToggleState(true, juce::dontSendNotification);
    highButton.setToggleState(true, juce::dontSendNotification);
    autoButton.onClick = [this] {
        lastLearnTarget = "tempoSource";
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
    lowButton.onClick = [this] {
        lastLearnTarget = "fxLow";
        if (!lowButton.getToggleState() && !midButton.getToggleState() &&
            !highButton.getToggleState())
            midButton.setToggleState(true, juce::dontSendNotification);
        updateEffect();
    };
    midButton.onClick = [this] {
        lastLearnTarget = "fxMid";
        if (!lowButton.getToggleState() && !midButton.getToggleState() &&
            !highButton.getToggleState())
            midButton.setToggleState(true, juce::dontSendNotification);
        updateEffect();
    };
    highButton.onClick = [this] {
        lastLearnTarget = "fxHigh";
        if (!lowButton.getToggleState() && !midButton.getToggleState() &&
            !highButton.getToggleState())
            midButton.setToggleState(true, juce::dontSendNotification);
        updateEffect();
    };
    time.setValue(0.5);
    depth.setValue(0.5);
    microphone.setValue(engine.microphoneLevel.load());
    microphoneCue.setToggleState(engine.microphoneCue.load(), juce::dontSendNotification);
    microphoneMute.setToggleState(engine.microphoneMute.load(), juce::dontSendNotification);
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
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setTextValueSuffix(suffix);
    slider.setDoubleClickReturnValue(true, 0.0);
    slider.setTooltip(slider.getName());
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
    parameters.quantize = quantized;
    parameters.low = lowButton.getToggleState();
    parameters.mid = midButton.getToggleState();
    parameters.high = highButton.getToggleState();
    engine.effectRack.setParameters(parameters);
    for (size_t index = 0; index < pads.size(); ++index)
        pads[index].setToggleState(static_cast<int>(index) == selectedDivision,
                                   juce::dontSendNotification);
    display.setState(parameters.type, parameters.bpm, tempoEngine.source(),
                     tempoEngine.confidence(), selectedDivision, parameters.time, parameters.depth);
}

void MainComponent::timerCallback() {
    for (int channel = 0; channel < channelCount; ++channel) {
        const auto value = std::max(engine.channelPeak(channel, 0), engine.channelPeak(channel, 1));
        auto& meter = channelControls[static_cast<size_t>(channel)].meter;
        meter = std::max(value, meter * 0.9F);
    }
    for (int side = 0; side < 2; ++side)
        masterMeters[static_cast<size_t>(side)] =
            std::max(engine.masterPeak(side), masterMeters[static_cast<size_t>(side)] * 0.9F);
    if (!microphone.isMouseButtonDown())
        microphone.setValue(engine.microphoneLevel.load(), juce::dontSendNotification);
    microphoneCue.setToggleState(engine.microphoneCue.load(), juce::dontSendNotification);
    microphoneMute.setToggleState(engine.microphoneMute.load(), juce::dontSendNotification);
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
    if (hostedByPlugin) {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::InfoIcon, "Host audio routing",
            "Audio input and output routing is managed by the plugin host. Enable the auxiliary "
            "Channel 2, Channel 3, Channel 4, or Microphone input buses in the host when they are "
            "needed.");
        return;
    }
    auto* selector = new SettingsComponent(engine, deviceManager);
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(selector);
    options.dialogTitle = "QuadBeat FX Settings";
    options.dialogBackgroundColour = QuadBeatLookAndFeel::panel();
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;
    options.launchAsync();
}

void MainComponent::setHostAudioStatus(const double sampleRate, const int blockSize) {
    if (!hostedByPlugin)
        return;
    const auto rateText = sampleRate > 0.0 ? juce::String(sampleRate, 0) + " Hz" : "not prepared";
    const auto blockText = blockSize > 0 ? juce::String(blockSize) + " samples" : "block pending";
    statusLabel.setText("VST3 HOST AUDIO  |  " + rateText + "  |  " + blockText,
                        juce::dontSendNotification);
}

void MainComponent::showMidiEditor() {
    auto* editor = new MidiMappingEditor(midiMapper);
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(editor);
    options.dialogTitle = "QuadBeat FX MIDI Mappings";
    options.dialogBackgroundColour = QuadBeatLookAndFeel::panel();
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
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
                               lastLearnTarget == "tap" || lastLearnTarget == "quantize" ||
                               lastLearnTarget == "beatPrevious" || lastLearnTarget == "beatNext" ||
                               lastLearnTarget.starts_with("fx")
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
    if (id == "microphoneLevel")
        return static_cast<float>(microphone.getValue() * 0.5);
    if (id == "mic.cue")
        return microphoneCue.getToggleState() ? 1.0F : 0.0F;
    if (id == "mic.mute")
        return microphoneMute.getToggleState() ? 1.0F : 0.0F;
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
    if (id == "fxLow")
        return lowButton.getToggleState() ? 1.0F : 0.0F;
    if (id == "fxMid")
        return midButton.getToggleState() ? 1.0F : 0.0F;
    if (id == "fxHigh")
        return highButton.getToggleState() ? 1.0F : 0.0F;
    if (id == "tempoSource")
        return static_cast<float>(tempoEngine.source()) / 2.0F;
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
    if (id == "microphoneLevel")
        microphone.setValue(normalised * 2.0F);
    else if (id == "mic.cue")
        microphoneCue.setToggleState(normalised > 0.5F, juce::sendNotification);
    else if (id == "mic.mute")
        microphoneMute.setToggleState(normalised > 0.5F, juce::sendNotification);
    else if (id == "master")
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
    else if (id == "fxLow")
        lowButton.setToggleState(normalised > 0.5F, juce::sendNotification);
    else if (id == "fxMid")
        midButton.setToggleState(normalised > 0.5F, juce::sendNotification);
    else if (id == "fxHigh")
        highButton.setToggleState(normalised > 0.5F, juce::sendNotification);
    else if (id == "beatPrevious" && normalised > 0.5F)
        beatLeft.triggerClick();
    else if (id == "beatNext" && normalised > 0.5F)
        beatRight.triggerClick();
    else if (id == "tempoSource") {
        const auto source = static_cast<TempoSource>(
            std::clamp(static_cast<int>(std::round(normalised * 2.0F)), 0, 2));
        tempoEngine.setSource(source);
        autoButton.setToggleState(source == TempoSource::automatic, juce::dontSendNotification);
    } else {
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
    constexpr std::array<double, 5> scales{0.75, 1.0, 1.25, 1.5, 2.0};
    state.uiScale =
        scales[static_cast<size_t>(std::clamp(scaleSelector.getSelectedItemIndex(), 0, 4))];
    state.midiMappings = midiMapper.serialise();
    for (int channel = 0; channel < channelCount; ++channel)
        for (int side = 0; side < 2; ++side)
            state.inputMappings[static_cast<size_t>(channel * 2 + side)] =
                engine.channelInputMapping(channel, side);
    state.inputMappings[8] = engine.microphoneInputMapping();
    for (int output = 0; output < 3; ++output)
        for (int side = 0; side < 2; ++side)
            state.outputMappings[static_cast<size_t>(output * 2 + side)] =
                engine.outputMapping(output, side);
    state.microphoneLevel = engine.microphoneLevel.load();
    state.microphoneMute = engine.microphoneMute.load();
    state.microphoneCue = engine.microphoneCue.load();
    state.analysisSource = engine.analysisSource.load();
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
    constexpr std::array<double, 5> scales{0.75, 1.0, 1.25, 1.5, 2.0};
    auto nearestScale = 0;
    for (int index = 1; index < static_cast<int>(scales.size()); ++index)
        if (std::abs(scales[static_cast<size_t>(index)] - state.uiScale) <
            std::abs(scales[static_cast<size_t>(nearestScale)] - state.uiScale))
            nearestScale = index;
    scaleSelector.setSelectedItemIndex(nearestScale);
    midiMapper.deserialise(state.midiMappings);
    for (int channel = 0; channel < channelCount; ++channel)
        for (int side = 0; side < 2; ++side)
            engine.setChannelInputMapping(
                channel, side, state.inputMappings[static_cast<size_t>(channel * 2 + side)]);
    engine.setMicrophoneInputMapping(state.inputMappings[8]);
    for (int output = 0; output < 3; ++output)
        for (int side = 0; side < 2; ++side)
            engine.setOutputMapping(output, side,
                                    state.outputMappings[static_cast<size_t>(output * 2 + side)]);
    engine.microphoneLevel.store(state.microphoneLevel);
    engine.microphoneMute.store(state.microphoneMute);
    engine.microphoneCue.store(state.microphoneCue);
    microphone.setValue(state.microphoneLevel, juce::dontSendNotification);
    microphoneMute.setToggleState(state.microphoneMute, juce::dontSendNotification);
    microphoneCue.setToggleState(state.microphoneCue, juce::dontSendNotification);
    engine.analysisSource.store(state.analysisSource);
    updateEffect();
}
} // namespace qb
