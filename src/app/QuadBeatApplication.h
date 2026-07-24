#pragma once

#include "audio/MixerEngine.h"
#include "bpm/TempoEngine.h"
#include "ui/MainComponent.h"
#include <juce_gui_extra/juce_gui_extra.h>

namespace qb {
class QuadBeatApplication final : public juce::JUCEApplication {
  public:
    const juce::String getApplicationName() override { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override { return false; }
    void initialise(const juce::String&) override;
    void shutdown() override;
    void systemRequestedQuit() override { quit(); }
    void anotherInstanceStarted(const juce::String&) override {}

  private:
    class MainWindow final : public juce::DocumentWindow {
      public:
        MainWindow(MixerEngine&, TempoEngine&, juce::AudioDeviceManager&, const AppState&);
        void closeButtonPressed() override;
        MainComponent* content() const noexcept {
            return dynamic_cast<MainComponent*>(getContentComponent());
        }
    };

    juce::File stateFile() const;
    MixerEngine mixer;
    TempoEngine tempo;
    juce::AudioDeviceManager deviceManager;
    std::unique_ptr<MainWindow> window;
};
} // namespace qb
