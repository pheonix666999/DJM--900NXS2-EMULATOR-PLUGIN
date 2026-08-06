#include "plugin/PluginEditor.h"
#include <cmath>

namespace qb {
QuadBeatAudioProcessorEditor::QuadBeatAudioProcessorEditor(QuadBeatAudioProcessor& owner)
    : AudioProcessorEditor(owner), processor(owner),
      content(owner.mixerEngine(), owner.tempoEngine(), owner.hostedMidiDeviceManager(), true) {
    addAndMakeVisible(content);
    setResizable(true, false);
    setResizeLimits(900, 720, 3840, 2160);
    content.onScaleRequested = [safe = juce::Component::SafePointer<QuadBeatAudioProcessorEditor>(
                                    this)](const float scale) {
        if (safe == nullptr)
            return;
        const auto width = std::clamp(static_cast<int>(std::lround(baseWidth * scale)), 900, 3840);
        const auto height =
            std::clamp(static_cast<int>(std::lround(baseHeight * scale)), 720, 2160);
        safe->setSize(width, height);
    };
    const auto state = processor.storedEditorState();
    content.restoreState(state);
    content.setHostAudioStatus(processor.getSampleRate(), processor.getBlockSize());
    setSize(std::clamp(state.windowWidth, 900, 3840), std::clamp(state.windowHeight, 720, 2160));
}

QuadBeatAudioProcessorEditor::~QuadBeatAudioProcessorEditor() {
    auto state = content.captureState();
    state.windowWidth = getWidth();
    state.windowHeight = getHeight();
    processor.storeEditorState(state);
}

void QuadBeatAudioProcessorEditor::resized() { content.setBounds(getLocalBounds()); }
} // namespace qb
