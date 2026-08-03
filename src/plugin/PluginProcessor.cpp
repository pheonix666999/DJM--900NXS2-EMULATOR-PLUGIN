#include "plugin/PluginProcessor.h"
#include "plugin/PluginEditor.h"

namespace qb {
QuadBeatAudioProcessor::QuadBeatAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Channel 1", juce::AudioChannelSet::stereo(), true)
                         .withInput("Channel 2", juce::AudioChannelSet::stereo(), false)
                         .withInput("Channel 3", juce::AudioChannelSet::stereo(), false)
                         .withInput("Channel 4", juce::AudioChannelSet::stereo(), false)
                         .withInput("Microphone", juce::AudioChannelSet::mono(), false)
                         .withOutput("Master", juce::AudioChannelSet::stereo(), true)) {
    editorState.windowWidth = 1200;
    editorState.windowHeight = 820;
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
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo() ||
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    for (int bus = 1; bus < 4; ++bus) {
        const auto set = layouts.getChannelSet(true, bus);
        if (!set.isDisabled() && set != juce::AudioChannelSet::stereo())
            return false;
    }

    const auto microphone = layouts.getChannelSet(true, 4);
    return microphone.isDisabled() || microphone == juce::AudioChannelSet::mono();
}

void QuadBeatAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const auto totalSamples = buffer.getNumSamples();

    for (int offset = 0; offset < totalSamples; offset += maximumInternalBlock) {
        const auto samples = std::min(maximumInternalBlock, totalSamples - offset);
        logicalInputs.setSize(internalInputChannels, samples, false, false, true);
        logicalOutputs.setSize(internalOutputChannels, samples, false, false, true);
        logicalInputs.clear();
        logicalOutputs.clear();

        for (int bus = 0; bus < 4; ++bus) {
            const auto input = getBusBuffer(buffer, true, bus);
            for (int side = 0; side < std::min(2, input.getNumChannels()); ++side)
                logicalInputs.copyFrom(bus * 2 + side, 0, input, side, offset, samples);
        }

        const auto microphone = getBusBuffer(buffer, true, 4);
        if (microphone.getNumChannels() > 0)
            logicalInputs.copyFrom(channelCount * 2, 0, microphone, 0, offset, samples);

        mixer.process(logicalInputs, logicalOutputs);
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
