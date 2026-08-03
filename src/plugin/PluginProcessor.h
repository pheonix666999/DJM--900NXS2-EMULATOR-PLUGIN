#pragma once

#include "audio/MixerEngine.h"
#include "bpm/TempoEngine.h"
#include "state/StateStore.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

namespace qb {
class QuadBeatAudioProcessor final : public juce::AudioProcessor {
  public:
    QuadBeatAudioProcessor();
    ~QuadBeatAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    MixerEngine& mixerEngine() noexcept { return mixer; }
    TempoEngine& tempoEngine() noexcept { return tempo; }
    juce::AudioDeviceManager& hostedMidiDeviceManager() noexcept { return midiDeviceManager; }
    AppState storedEditorState() const;
    void storeEditorState(const AppState&);

  private:
    static constexpr int internalInputChannels = channelCount * 2 + 1;
    static constexpr int internalOutputChannels = 6;
    static constexpr int maximumInternalBlock = 8192;

    MixerEngine mixer;
    TempoEngine tempo;
    juce::AudioDeviceManager midiDeviceManager;
    juce::AudioBuffer<float> logicalInputs;
    juce::AudioBuffer<float> logicalOutputs;
    mutable juce::CriticalSection stateLock;
    AppState editorState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuadBeatAudioProcessor)
};
} // namespace qb
