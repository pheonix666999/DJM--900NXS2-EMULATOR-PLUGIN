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

    static juce::Colour background() { return juce::Colour(0xff10151c); }
    static juce::Colour panel() { return juce::Colour(0xff19222c); }
    static juce::Colour accent() { return juce::Colour(0xff20d7c2); }
    static juce::Colour warm() { return juce::Colour(0xffffb547); }
};
} // namespace qb
