#include <cmath>
#include <iostream>
#include <juce_audio_utils/juce_audio_utils.h>

namespace {
int fail(const juce::String& message) {
    std::cerr << message << '\n';
    return 1;
}

class PluginWindow final : public juce::DocumentWindow {
  public:
    explicit PluginWindow(juce::AudioProcessorEditor* editor)
        : DocumentWindow("QuadBeat FX - VST3 Host Validation", juce::Colours::black,
                         juce::DocumentWindow::closeButton) {
        setUsingNativeTitleBar(true);
        setResizable(true, false);
        setContentOwned(editor, true);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
    }

    void closeButtonPressed() override { juce::MessageManager::getInstance()->stopDispatchLoop(); }
};

} // namespace

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3)
        return fail("Expected the VST3 bundle path and optional --show flag");
    const auto showEditor = argc == 3 && juce::String::fromUTF8(argv[2]) == "--show";

    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    juce::AudioPluginFormatManager formats;
    formats.addFormat(std::make_unique<juce::VST3PluginFormat>());

    juce::OwnedArray<juce::PluginDescription> descriptions;
    auto* format = formats.getFormat(0);
    format->findAllTypesForFile(descriptions, juce::String::fromUTF8(argv[1]));
    if (descriptions.size() != 1)
        return fail("VST3 scan did not find exactly one QuadBeat FX processor");

    juce::String error;
    auto plugin = formats.createPluginInstance(*descriptions[0], 48000.0, 512, error);
    if (plugin == nullptr)
        return fail("VST3 instance creation failed: " + error);
    if (plugin->getBusCount(true) != 1 || plugin->getBusCount(false) != 1)
        return fail("Unexpected VST3 bus count");
    if (plugin->getBus(true, 0)->getName() != "Input" ||
        plugin->getBus(false, 0)->getName() != "Output")
        return fail("Unexpected VST3 bus names");

    auto monoLayout = plugin->getBusesLayout();
    monoLayout.inputBuses.getReference(0) = juce::AudioChannelSet::mono();
    monoLayout.outputBuses.getReference(0) = juce::AudioChannelSet::mono();
    if (!plugin->setBusesLayout(monoLayout))
        return fail("The host could not negotiate a mono effect layout");

    auto layout = plugin->getBusesLayout();
    layout.inputBuses.getReference(0) = juce::AudioChannelSet::stereo();
    layout.outputBuses.getReference(0) = juce::AudioChannelSet::stereo();
    if (!plugin->setBusesLayout(layout))
        return fail("The host could not enable the standard stereo effect layout");

    plugin->prepareToPlay(48000.0, 512);
    juce::AudioBuffer<float> audio(
        std::max(plugin->getTotalNumInputChannels(), plugin->getTotalNumOutputChannels()), 512);
    audio.clear();
    for (int channel = 0; channel < audio.getNumChannels(); ++channel)
        audio.setSample(channel, 0, 0.1F);
    juce::MidiBuffer midi;
    plugin->processBlock(audio, midi);
    for (int channel = 0; channel < plugin->getTotalNumOutputChannels(); ++channel) {
        for (int sample = 0; sample < audio.getNumSamples(); ++sample) {
            if (!std::isfinite(audio.getSample(channel, sample)))
                return fail("VST3 produced a non-finite output sample");
        }
    }

    auto* editor = plugin->createEditorAndMakeActive();
    if (editor == nullptr)
        return fail("VST3 did not create its editor");
    if (editor->getWidth() < 340 || editor->getHeight() < 900)
        return fail("VST3 editor opened below its supported minimum size");
    if (editor->getHeight() <= editor->getWidth() * 2)
        return fail("VST3 editor did not open in the requested long vertical layout");
    editor->setSize(340, 900);
    if (editor->getWidth() != 340 || editor->getHeight() != 900)
        return fail("VST3 editor did not accept its documented minimum size");

    juce::MemoryBlock state;
    plugin->getStateInformation(state);
    if (state.isEmpty())
        return fail("VST3 returned empty state data");
    plugin->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    if (showEditor) {
        PluginWindow window(editor);
        juce::MessageManager::getInstance()->runDispatchLoop();
    } else {
        delete editor;
    }
    plugin->releaseResources();
    std::cout << "VST3 scan, instance, buses, processing, editor, and state passed\n";
    return 0;
}
