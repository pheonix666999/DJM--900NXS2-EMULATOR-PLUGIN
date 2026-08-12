#include "plugin/PluginEditor.h"
#include <cmath>

namespace qb {
QuadBeatAudioProcessorEditor::QuadBeatAudioProcessorEditor(QuadBeatAudioProcessor& owner)
    : AudioProcessorEditor(owner), processor(owner),
      content(owner.mixerEngine(), owner.tempoEngine(), owner.hostedMidiDeviceManager(), true) {
    addAndMakeVisible(content);
    setResizable(true, false);
    setResizeLimits(minimumWidth, minimumHeight, maximumWidth, maximumHeight);
    content.onScaleRequested = [safe = juce::Component::SafePointer<QuadBeatAudioProcessorEditor>(
                                    this)](const float scale) {
        if (safe == nullptr)
            return;
        const auto width = std::clamp(static_cast<int>(std::lround(baseWidth * scale)),
                                      minimumWidth, maximumWidth);
        const auto height = std::clamp(static_cast<int>(std::lround(baseHeight * scale)),
                                       minimumHeight, maximumHeight);
        safe->setSize(width, height);
    };
    const auto state = processor.storedEditorState();
    content.restoreState(state);
    content.setHostAudioStatus(processor.getSampleRate(), processor.getBlockSize());
    const auto legacyWideLayout = state.windowWidth >= state.windowHeight;
    setSize(legacyWideLayout ? baseWidth
                             : std::clamp(state.windowWidth, minimumWidth, maximumWidth),
            legacyWideLayout ? baseHeight
                             : std::clamp(state.windowHeight, minimumHeight, maximumHeight));
}

QuadBeatAudioProcessorEditor::~QuadBeatAudioProcessorEditor() {
    auto state = content.captureState();
    state.windowWidth = getWidth();
    state.windowHeight = getHeight();
    processor.storeEditorState(state);
}

void QuadBeatAudioProcessorEditor::resized() { content.setBounds(getLocalBounds()); }
} // namespace qb
