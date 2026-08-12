#include "plugin/PluginProcessor.h"
#include "plugin/PluginEditor.h"

namespace qb {
QuadBeatAudioProcessor::QuadBeatAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {
    editorState.windowWidth = 360;
    editorState.windowHeight = 1040;
    mixer.setTempoEngine(&tempo);
}

void QuadBeatAudioProcessor::prepareToPlay(const double sampleRate, const int) {
    logicalInputs.setSize(internalInputChannels, maximumInternalBlock, false, true, false);
    logicalOutputs.setSize(internalOutputChannels, maximumInternalBlock, false, true, false);
    mixer.prepare(sampleRate, maximumInternalBlock);
}

void QuadBeatAudioProcessor::releaseResources() {
    logicalInputs.clear();
    logicalOutputs.clear();
}

bool QuadBeatAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.inputBuses.size() != 1 || layouts.outputBuses.size() != 1)
        return false;
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    return input == output &&
           (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo());
}

void QuadBeatAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const auto totalSamples = buffer.getNumSamples();

    for (int offset = 0; offset < totalSamples; offset += maximumInternalBlock) {
        const auto samples = std::min(maximumInternalBlock, totalSamples - offset);
        logicalInputs.clear(0, samples);
        logicalOutputs.clear(0, samples);

        const auto input = getBusBuffer(buffer, true, 0);
        for (int side = 0; side < std::min(2, input.getNumChannels()); ++side)
            logicalInputs.copyFrom(side, 0, input, side, offset, samples);

        juce::AudioBuffer<float> inputView(logicalInputs.getArrayOfWritePointers(),
                                           internalInputChannels, samples);
        juce::AudioBuffer<float> outputView(logicalOutputs.getArrayOfWritePointers(),
                                            internalOutputChannels, samples);
        mixer.process(inputView, outputView);
        auto masterOutput = getBusBuffer(buffer, false, 0);
        masterOutput.clear(offset, samples);
        for (int side = 0; side < std::min(2, masterOutput.getNumChannels()); ++side)
            masterOutput.copyFrom(side, offset, logicalOutputs, side, 0, samples);
    }
}

juce::AudioProcessorEditor* QuadBeatAudioProcessor::createEditor() {
    return new QuadBeatAudioProcessorEditor(*this);
}

AppState QuadBeatAudioProcessor::storedEditorState() const {
    const juce::ScopedLock lock(stateLock);
    return editorState;
}

void QuadBeatAudioProcessor::storeEditorState(const AppState& state) {
    const juce::ScopedLock lock(stateLock);
    editorState = state;
}

void QuadBeatAudioProcessor::getStateInformation(juce::MemoryBlock& destination) {
    const auto json = juce::JSON::toString(StateStore::toVar(storedEditorState()), false);
    destination.replaceAll(json.toRawUTF8(), static_cast<size_t>(json.getNumBytesAsUTF8()));
}

void QuadBeatAudioProcessor::setStateInformation(const void* data, const int sizeInBytes) {
    if (data == nullptr || sizeInBytes <= 0)
        return;
    const auto json = juce::String::fromUTF8(static_cast<const char*>(data), sizeInBytes);
    if (const auto decoded = StateStore::fromVar(juce::JSON::parse(json)))
        storeEditorState(*decoded);
}
} // namespace qb

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new qb::QuadBeatAudioProcessor();
}
