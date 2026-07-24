#include "app/QuadBeatApplication.h"
#include "state/StateStore.h"

namespace qb {
QuadBeatApplication::MainWindow::MainWindow(MixerEngine& mixer, TempoEngine& tempo,
                                            juce::AudioDeviceManager& devices,
                                            const AppState& state)
    : DocumentWindow("QuadBeat FX", juce::Colour(0xff10151c), allButtons) {
    setUsingNativeTitleBar(true);
    setResizable(true, true);
    setResizeLimits(1100, 700, 3840, 2160);
    auto content = std::make_unique<MainComponent>(mixer, tempo, devices);
    content->restoreState(state);
    setContentOwned(content.release(), true);
    setBounds(state.windowX, state.windowY, state.windowWidth, state.windowHeight);
    if (state.windowX < 0 ||
        !juce::Desktop::getInstance().getDisplays().getTotalBounds(true).intersects(getBounds()))
        centreWithSize(state.windowWidth, state.windowHeight);
    setVisible(true);
}

void QuadBeatApplication::MainWindow::closeButtonPressed() {
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

juce::File QuadBeatApplication::stateFile() const {
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("QuadBeat Audio")
        .getChildFile("QuadBeatFX.state");
}

void QuadBeatApplication::initialise(const juce::String&) {
    const auto state = StateStore::loadOrDefault(stateFile());
    const auto savedDevice = state.audioDeviceXml.empty()
                                 ? std::unique_ptr<juce::XmlElement>{}
                                 : juce::XmlDocument::parse(juce::String(state.audioDeviceXml));
    auto error = deviceManager.initialise(8, 6, savedDevice.get(), true);
    if (error.isNotEmpty())
        error = deviceManager.initialise(0, 2, nullptr, true);
    deviceManager.addAudioCallback(&mixer);
    window = std::make_unique<MainWindow>(mixer, tempo, deviceManager, state);
    if (error.isNotEmpty())
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                               "Audio device unavailable", error);
}

void QuadBeatApplication::shutdown() {
    if (window != nullptr && window->content() != nullptr) {
        auto state = window->content()->captureState();
        state.windowX = window->getX();
        state.windowY = window->getY();
        state.windowWidth = window->getWidth();
        state.windowHeight = window->getHeight();
        if (const auto deviceState = deviceManager.createStateXml())
            state.audioDeviceXml = deviceState->toString().toStdString();
        const auto file = stateFile();
        file.getParentDirectory().createDirectory();
        StateStore::save(file, state);
    }
    deviceManager.removeAudioCallback(&mixer);
    window.reset();
}
} // namespace qb
