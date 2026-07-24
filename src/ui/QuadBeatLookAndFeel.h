#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace qb {
class QuadBeatLookAndFeel final : public juce::LookAndFeel_V4 {
  public:
    QuadBeatLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPosition, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted,
                              bool down) override;
    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPosition, float minSliderPosition, float maxSliderPosition,
                          juce::Slider::SliderStyle, juce::Slider&) override;
    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown, int buttonX,
                      int buttonY, int buttonWidth, int buttonHeight, juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;

    static juce::Colour background() { return juce::Colour(0xff07090b); }
    static juce::Colour panel() { return juce::Colour(0xff111417); }
    static juce::Colour accent() { return juce::Colour(0xff27a7ff); }
    static juce::Colour warm() { return juce::Colour(0xffff8a32); }
    static juce::Colour oled() { return juce::Colour(0xff9de4dc); }
};
} // namespace qb
