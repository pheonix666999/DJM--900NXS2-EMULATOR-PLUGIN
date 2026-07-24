#include "ui/QuadBeatLookAndFeel.h"
#include <cmath>

namespace qb {
QuadBeatLookAndFeel::QuadBeatLookAndFeel() {
    setColour(juce::Label::textColourId, juce::Colour(0xffe8f0f7));
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffdce8ef));
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff111820));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff35424f));
    setColour(juce::TextButton::textColourOffId, juce::Colour(0xffaab8c4));
    setColour(juce::TextButton::textColourOnId, juce::Colour(0xff071412));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff111820));
    setColour(juce::ComboBox::textColourId, juce::Colours::white);
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff35424f));
}

void QuadBeatLookAndFeel::drawRotarySlider(juce::Graphics& graphics, const int x, const int y,
                                           const int width, const int height, const float position,
                                           const float start, const float end, juce::Slider&) {
    const auto bounds =
        juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                               static_cast<float>(width), static_cast<float>(height))
            .reduced(5.0F);
    const auto diameter = std::min(bounds.getWidth(), bounds.getHeight());
    const auto knob = juce::Rectangle<float>(diameter, diameter).withCentre(bounds.getCentre());
    const auto angle = start + position * (end - start);
    graphics.setColour(juce::Colour(0xff0b0f14));
    graphics.fillEllipse(knob);
    graphics.setColour(juce::Colour(0xff34414d));
    graphics.drawEllipse(knob, 2.0F);
    juce::Path arc;
    arc.addCentredArc(knob.getCentreX(), knob.getCentreY(), diameter * 0.48F, diameter * 0.48F,
                      0.0F, start, angle, true);
    graphics.setColour(accent());
    graphics.strokePath(arc, juce::PathStrokeType(3.0F));
    const auto centre = knob.getCentre();
    const auto radius = diameter * 0.32F;
    graphics.setColour(juce::Colours::white);
    graphics.drawLine(centre.x, centre.y, centre.x + std::sin(angle) * radius,
                      centre.y - std::cos(angle) * radius, 2.0F);
}

void QuadBeatLookAndFeel::drawButtonBackground(juce::Graphics& graphics, juce::Button& button,
                                               const juce::Colour&, const bool highlighted,
                                               const bool down) {
    auto colour = button.getToggleState() ? accent() : juce::Colour(0xff26323e);
    if (down)
        colour = colour.darker(0.18F);
    else if (highlighted)
        colour = colour.brighter(0.12F);
    graphics.setColour(colour);
    graphics.fillRoundedRectangle(button.getLocalBounds().toFloat().reduced(1.0F), 5.0F);
    graphics.setColour(button.hasKeyboardFocus(true) ? warm() : juce::Colour(0xff465666));
    graphics.drawRoundedRectangle(button.getLocalBounds().toFloat().reduced(1.0F), 5.0F, 1.0F);
}

void QuadBeatLookAndFeel::drawLinearSlider(juce::Graphics& graphics, const int x, const int y,
                                           const int width, const int height, const float position,
                                           float, float, const juce::Slider::SliderStyle style,
                                           juce::Slider&) {
    if (style == juce::Slider::LinearVertical) {
        const auto centre = static_cast<float>(x + width / 2);
        graphics.setColour(juce::Colour(0xff0b1015));
        graphics.fillRoundedRectangle(centre - 3.0F, static_cast<float>(y), 6.0F,
                                      static_cast<float>(height), 3.0F);
        graphics.setColour(accent());
        graphics.fillRoundedRectangle(centre - 10.0F, position - 4.0F, 20.0F, 8.0F, 3.0F);
    } else {
        const auto centre = static_cast<float>(y + height / 2);
        graphics.setColour(juce::Colour(0xff0b1015));
        graphics.fillRoundedRectangle(static_cast<float>(x), centre - 3.0F,
                                      static_cast<float>(width), 6.0F, 3.0F);
        graphics.setColour(accent());
        graphics.fillRoundedRectangle(position - 4.0F, centre - 10.0F, 8.0F, 20.0F, 3.0F);
    }
}
} // namespace qb
