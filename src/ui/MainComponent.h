#pragma once

#include "audio/MixerEngine.h"
#include "bpm/TempoEngine.h"
#include "midi/MidiMapper.h"
#include "state/StateStore.h"
#include "ui/BeatFxDisplay.h"
#include "ui/HardwareRotarySelector.h"
#include "ui/QuadBeatLookAndFeel.h"
#include <array>
#include <functional>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

namespace qb {
class MainComponent final : public juce::Component,
                            private juce::Timer,
                            private juce::MidiInputCallback {
  public:
    MainComponent(MixerEngine& mixer, TempoEngine& tempo, juce::AudioDeviceManager& devices,
                  bool hostedByPlugin = false);
    ~MainComponent() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
    AppState captureState() const;
    void restoreState(const AppState& state);
    void setHostAudioStatus(double sampleRate, int blockSize);
    std::function<void(float)> onScaleRequested;

  private:
    struct ChannelControls {
        juce::Label title;
        juce::Slider trim;
        juce::Slider high;
        juce::Slider mid;
        juce::Slider low;
        juce::Slider fader;
        juce::TextButton cue{"CUE"};
        juce::TextButton mute{"MUTE"};
        juce::ComboBox assignment;
        juce::ComboBox eqMode;
        float meter{};
    };

    void configureKnob(juce::Slider& slider, juce::String suffix = {});
    void bindChannel(int index);
    void updateEffect();
    void showSettings();
    void showMidiEditor();
    void timerCallback() override;
    void layoutChannel(ChannelControls&, juce::Rectangle<int>);
    void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage&) override;
    void handleMidiOnMessageThread(juce::MidiMessage);
    float parameterValue(const std::string& id) const;
    void setParameterValue(const std::string& id, float value);

    MixerEngine& engine;
    TempoEngine& tempoEngine;
    juce::AudioDeviceManager& deviceManager;
    bool hostedByPlugin{};
    QuadBeatLookAndFeel lookAndFeel;
    std::array<ChannelControls, channelCount> channelControls;
    juce::Label productLabel;
    juce::Label statusLabel;
    juce::TextButton settingsButton{"SETTINGS"};
    juce::TextButton midiButton{"MIDI LEARN"};
    juce::TextButton midiEditButton{"MIDI EDIT"};
    juce::ComboBox scaleSelector;
    juce::Slider microphone;
    juce::TextButton microphoneCue{"MIC CUE"};
    juce::TextButton microphoneMute{"MIC OFF"};
    juce::Slider master;
    juce::Slider booth;
    juce::Slider headphones;
    juce::Slider cueMix;
    juce::Slider crossfader;
    HardwareRotarySelector effectSelector;
    HardwareRotarySelector busSelector;
    juce::Slider time;
    juce::Slider depth;
    BeatFxDisplay display;
    juce::TextButton beatLeft{"<"};
    juce::TextButton beatRight{">"};
    std::array<juce::TextButton, 8> pads;
    juce::TextButton autoButton{"AUTO"};
    juce::TextButton tapButton{"TAP"};
    juce::TextButton quantizeButton{"QUANTIZE"};
    juce::TextButton lowButton{"LOW"};
    juce::TextButton midButton{"MID"};
    juce::TextButton highButton{"HIGH"};
    int selectedDivision{5};
    bool quantized{true};
    MidiMapper midiMapper;
    std::string lastLearnTarget{"effectDepth"};
    bool midiLearning{};
    juce::Array<juce::MidiDeviceInfo> midiDevices;
    std::unique_ptr<juce::DialogWindow> settingsWindow;
    std::array<juce::Rectangle<int>, channelCount> channelBounds;
    juce::Rectangle<int> utilityBounds;
    juce::Rectangle<int> monitorBounds;
    juce::Rectangle<int> masterMeterBounds;
    juce::Rectangle<int> fxBounds;
    juce::Rectangle<int> crossfaderBounds;
    std::array<float, 2> masterMeters{};
};
} // namespace qb
