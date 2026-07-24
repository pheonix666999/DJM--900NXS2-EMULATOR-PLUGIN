#include "ui/QuadBeatLookAndFeel.h"
#include <cmath>

namespace qb {
QuadBeatLookAndFeel::QuadBeatLookAndFeel() {
    setColour(juce::Label::textColourId, juce::Colour(0xffe5e8e8));
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffd8dddd));
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff080a0c));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff52585d));
    setColour(juce::TextButton::textColourOffId, juce::Colour(0xffd1d5d6));
    setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff090b0d));
    setColour(juce::ComboBox::textColourId, juce::Colour(0xffe8eaea));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff555b60));
}

void QuadBeatLookAndFeel::drawRotarySlider(juce::Graphics& graphics, const int x, const int y,
                                           const int width, const int height, const float position,
                                           const float start, const float end, juce::Slider&) {
    const auto bounds =
        juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                               static_cast<float>(width), static_cast<float>(height))
            .reduced(7.0F);
    const auto diameter = std::min(bounds.getWidth(), bounds.getHeight());
    const auto knob = juce::Rectangle<float>(diameter, diameter).withCentre(bounds.getCentre());
    const auto angle = start + position * (end - start);

    const auto centre = knob.getCentre();
    for (int tick = 0; tick < 11; ++tick) {
        const auto tickAngle = start + static_cast<float>(tick) / 10.0F * (end - start);
        const auto inner = diameter * 0.50F;
        const auto outer = diameter * 0.58F;
        graphics.setColour(juce::Colour(tick == 5 ? 0xffc9cdce : 0xff62686c));
        graphics.drawLine(centre.x + std::sin(tickAngle) * inner,
                          centre.y - std::cos(tickAngle) * inner,
                          centre.x + std::sin(tickAngle) * outer,
                          centre.y - std::cos(tickAngle) * outer, tick == 5 ? 1.6F : 1.0F);
    }

    graphics.setColour(juce::Colour(0xff030405));
    graphics.fillEllipse(knob.expanded(2.0F));
    juce::ColourGradient face(juce::Colour(0xff45494b), knob.getX(), knob.getY(),
                              juce::Colour(0xff090a0b), knob.getRight(), knob.getBottom(), false);
    graphics.setGradientFill(face);
    graphics.fillEllipse(knob);
    graphics.setColour(juce::Colour(0xff74797b));
    graphics.drawEllipse(knob, 2.0F);
    graphics.setColour(juce::Colour(0xff151719));
    graphics.drawEllipse(knob.reduced(5.0F), 1.0F);
    const auto radius = diameter * 0.35F;
    graphics.setColour(juce::Colour(0xffefefea));
    graphics.drawLine(centre.x, centre.y, centre.x + std::sin(angle) * radius,
                      centre.y - std::cos(angle) * radius, 3.0F);
}

void QuadBeatLookAndFeel::drawButtonBackground(juce::Graphics& graphics, juce::Button& button,
                                               const juce::Colour&, const bool highlighted,
                                               const bool down) {
    const auto id = button.getComponentID();
    const auto active = button.getToggleState();
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0F);

    if (id == "tap") {
        const auto diameter = std::min(bounds.getWidth(), bounds.getHeight());
        const auto circle =
            juce::Rectangle<float>(diameter, diameter).withCentre(bounds.getCentre());
        graphics.setColour(juce::Colour(0xff020403));
        graphics.fillEllipse(circle);
        graphics.setColour(active || highlighted ? juce::Colour(0xff28e26f)
                                                 : juce::Colour(0xff137b3d));
        graphics.drawEllipse(circle.reduced(3.0F), down ? 5.0F : 3.0F);
        graphics.setColour(juce::Colour(0xff3a4140));
        graphics.drawEllipse(circle, 2.0F);
        return;
    }

    juce::Colour fill(0xff202326);
    juce::Colour outline(0xff60666a);
    if (id == "frequency") {
        fill = active ? juce::Colour(0xff1265a0) : juce::Colour(0xff0b1014);
        outline = active ? accent() : juce::Colour(0xff247ab7);
    } else if (id == "cue") {
        fill = active ? warm() : juce::Colour(0xff261b13);
        outline = active ? juce::Colour(0xffffb36b) : juce::Colour(0xff81502c);
    } else if (id == "mute") {
        fill = active ? juce::Colour(0xffc53b35) : juce::Colour(0xff241414);
        outline = active ? juce::Colour(0xffff6c62) : juce::Colour(0xff773a36);
    } else if (id == "pad") {
        fill = active ? juce::Colour(0xffdce1df) : juce::Colour(0xff1c2022);
        outline = active ? juce::Colour(0xfff5f6f3) : juce::Colour(0xff62686b);
        button.setColour(juce::TextButton::textColourOnId, juce::Colour(0xff080a0a));
    } else if (id == "quantize") {
        fill = active ? juce::Colour(0xff3a2226) : juce::Colour(0xff191b1d);
        outline = active ? juce::Colour(0xffff4050) : juce::Colour(0xff555b5e);
    } else if (active) {
        fill = juce::Colour(0xff334047);
        outline = accent();
    }

    if (down)
        fill = fill.darker(0.25F);
    else if (highlighted)
        fill = fill.brighter(0.12F);

    juce::ColourGradient face(fill.brighter(0.10F), bounds.getX(), bounds.getY(),
                              fill.darker(0.22F), bounds.getX(), bounds.getBottom(), false);
    graphics.setGradientFill(face);
    graphics.fillRoundedRectangle(bounds, 2.5F);
    graphics.setColour(button.hasKeyboardFocus(true) ? accent() : outline);
    graphics.drawRoundedRectangle(bounds, 2.5F, button.hasKeyboardFocus(true) ? 2.0F : 1.0F);
}

void QuadBeatLookAndFeel::drawLinearSlider(juce::Graphics& graphics, const int x, const int y,
                                           const int width, const int height, const float position,
                                           float, float, const juce::Slider::SliderStyle style,
                                           juce::Slider&) {
    if (style == juce::Slider::LinearVertical) {
        const auto centre = static_cast<float>(x + width / 2);
        graphics.setColour(juce::Colour(0xff020304));
        graphics.fillRect(centre - 4.0F, static_cast<float>(y), 8.0F, static_cast<float>(height));
        graphics.setColour(juce::Colour(0xff404548));
        graphics.drawLine(centre, static_cast<float>(y), centre, static_cast<float>(y + height),
                          1.0F);
        const auto cap = juce::Rectangle<float>(30.0F, 13.0F).withCentre({centre, position});
        graphics.setColour(juce::Colour(0xff08090a));
        graphics.fillRoundedRectangle(cap.expanded(2.0F), 2.0F);
        juce::ColourGradient face(juce::Colour(0xffb8bcbd), cap.getX(), cap.getY(),
                                  juce::Colour(0xff4c5153), cap.getX(), cap.getBottom(), false);
        graphics.setGradientFill(face);
        graphics.fillRoundedRectangle(cap, 1.5F);
        graphics.setColour(juce::Colour(0xff17191a));
        graphics.drawLine(cap.getX() + 3.0F, cap.getCentreY(), cap.getRight() - 3.0F,
                          cap.getCentreY(), 1.5F);
    } else {
        const auto centre = static_cast<float>(y + height / 2);
        graphics.setColour(juce::Colour(0xff020304));
        graphics.fillRect(static_cast<float>(x), centre - 4.0F, static_cast<float>(width), 8.0F);
        graphics.setColour(juce::Colour(0xff404548));
        graphics.drawLine(static_cast<float>(x), centre, static_cast<float>(x + width), centre,
                          1.0F);
        const auto cap = juce::Rectangle<float>(14.0F, 28.0F).withCentre({position, centre});
        juce::ColourGradient face(juce::Colour(0xffb8bcbd), cap.getX(), cap.getY(),
                                  juce::Colour(0xff4c5153), cap.getRight(), cap.getY(), false);
        graphics.setGradientFill(face);
        graphics.fillRoundedRectangle(cap, 1.5F);
        graphics.setColour(juce::Colour(0xff17191a));
        graphics.drawLine(cap.getCentreX(), cap.getY() + 3.0F, cap.getCentreX(),
                          cap.getBottom() - 3.0F, 1.5F);
    }
}

void QuadBeatLookAndFeel::drawComboBox(juce::Graphics& graphics, const int width, const int height,
                                       const bool isButtonDown, const int, const int,
                                       const int buttonWidth, const int, juce::ComboBox&) {
    auto bounds =
        juce::Rectangle<float>(0.0F, 0.0F, static_cast<float>(width), static_cast<float>(height))
            .reduced(1.0F);
    graphics.setColour(juce::Colour(isButtonDown ? 0xff171a1c : 0xff090b0d));
    graphics.fillRoundedRectangle(bounds, 2.0F);
    graphics.setColour(juce::Colour(0xff555b60));
    graphics.drawRoundedRectangle(bounds, 2.0F, 1.0F);

    const auto arrowX = static_cast<float>(width - buttonWidth / 2);
    const auto arrowY = static_cast<float>(height) * 0.5F;
    juce::Path arrow;
    arrow.startNewSubPath(arrowX - 4.0F, arrowY - 2.0F);
    arrow.lineTo(arrowX, arrowY + 2.0F);
    arrow.lineTo(arrowX + 4.0F, arrowY - 2.0F);
    graphics.setColour(juce::Colour(0xffd8dddd));
    graphics.strokePath(arrow, juce::PathStrokeType(1.5F));
}

juce::Font QuadBeatLookAndFeel::getComboBoxFont(juce::ComboBox&) {
    return juce::FontOptions(11.0F, juce::Font::bold);
}

juce::Font QuadBeatLookAndFeel::getTextButtonFont(juce::TextButton&, const int buttonHeight) {
    return juce::FontOptions(std::clamp(static_cast<float>(buttonHeight) * 0.34F, 9.0F, 13.0F),
                             juce::Font::bold);
}
} // namespace qb
