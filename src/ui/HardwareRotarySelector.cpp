#include "ui/HardwareRotarySelector.h"
#include "ui/QuadBeatLookAndFeel.h"
#include <algorithm>
#include <cmath>

namespace qb {
namespace {
constexpr auto rotaryStart = juce::MathConstants<float>::pi * 1.22F;
constexpr auto rotaryEnd = juce::MathConstants<float>::pi * 2.78F;
} // namespace

HardwareRotarySelector::HardwareRotarySelector() {
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
}

void HardwareRotarySelector::addItem(const juce::String& text, const int itemId) {
    items.push_back({text, itemId});
    repaint();
}

void HardwareRotarySelector::addItemList(const juce::StringArray& newItems, const int firstItemId) {
    for (int index = 0; index < newItems.size(); ++index)
        addItem(newItems[index], firstItemId + index);
}

int HardwareRotarySelector::getNumItems() const noexcept { return static_cast<int>(items.size()); }

int HardwareRotarySelector::getSelectedItemIndex() const noexcept { return selectedIndex; }

juce::String HardwareRotarySelector::getText() const {
    if (items.empty())
        return {};
    return items[static_cast<size_t>(
                     std::clamp(selectedIndex, 0, static_cast<int>(items.size()) - 1))]
        .text;
}

void HardwareRotarySelector::setSelectedId(const int itemId,
                                           const juce::NotificationType notification) {
    const auto found = std::find_if(items.begin(), items.end(),
                                    [itemId](const auto& item) { return item.id == itemId; });
    if (found != items.end())
        setSelectedItemIndex(static_cast<int>(std::distance(items.begin(), found)), notification);
}

void HardwareRotarySelector::setSelectedItemIndex(const int index,
                                                  const juce::NotificationType notification) {
    if (items.empty())
        return;
    const auto next = std::clamp(index, 0, static_cast<int>(items.size()) - 1);
    if (next == selectedIndex)
        return;
    selectedIndex = next;
    repaint();
    if (notification != juce::dontSendNotification && onChange)
        onChange();
}

juce::Rectangle<float> HardwareRotarySelector::knobBounds() const {
    const auto dense = items.size() > 10;
    const auto diameter = std::min(
        dense ? 78.0F : 72.0F, std::min(static_cast<float>(getHeight()) * 0.48F,
                                        static_cast<float>(getWidth()) * (dense ? 0.24F : 0.52F)));
    return juce::Rectangle<float>(diameter, diameter)
        .withCentre({static_cast<float>(getWidth()) * 0.5F,
                     static_cast<float>(getHeight()) * (dense ? 0.52F : 0.58F)});
}

void HardwareRotarySelector::paint(juce::Graphics& graphics) {
    if (items.empty())
        return;

    const auto knob = knobBounds();
    const auto centre = knob.getCentre();
    const auto count = static_cast<int>(items.size());

    if (count > 10) {
        auto labelArea = getLocalBounds().reduced(5).withTrimmedBottom(19);
        const auto rows = (count + 1) / 2;
        const auto rowHeight = std::max(12, labelArea.getHeight() / rows);
        const auto leftWidth = std::max(0, static_cast<int>(knob.getX()) - labelArea.getX() - 8);
        const auto rightX = static_cast<int>(std::ceil(knob.getRight())) + 8;
        const auto rightWidth = std::max(0, labelArea.getRight() - rightX);
        graphics.setFont(juce::FontOptions(8.0F, juce::Font::bold));
        for (int index = 0; index < count; ++index) {
            const auto rightColumn = index >= rows;
            const auto row = rightColumn ? index - rows : index;
            const auto label = juce::Rectangle<int>(
                rightColumn ? rightX : labelArea.getX(), labelArea.getY() + row * rowHeight,
                rightColumn ? rightWidth : leftWidth, rowHeight);
            graphics.setColour(index == selectedIndex ? QuadBeatLookAndFeel::accent()
                                                      : juce::Colour(0xff9ca5aa));
            graphics.drawFittedText(items[static_cast<size_t>(index)].text, label,
                                    rightColumn ? juce::Justification::centredLeft
                                                : juce::Justification::centredRight,
                                    1, 0.76F);
        }
    } else {
        const auto labelRadiusX = std::max(42.0F, static_cast<float>(getWidth()) * 0.34F);
        const auto labelRadiusY = std::max(34.0F, static_cast<float>(getHeight()) * 0.32F);
        graphics.setFont(juce::FontOptions(9.0F, juce::Font::bold));
        for (int index = 0; index < count; ++index) {
            const auto proportion =
                count == 1 ? 0.5F : static_cast<float>(index) / static_cast<float>(count - 1);
            const auto angle = rotaryStart + proportion * (rotaryEnd - rotaryStart);
            const auto x = centre.x + std::sin(angle) * labelRadiusX;
            const auto y = centre.y - std::cos(angle) * labelRadiusY;
            const auto label = juce::Rectangle<float>(58.0F, 15.0F).withCentre({x, y});
            graphics.setColour(index == selectedIndex ? QuadBeatLookAndFeel::accent()
                                                      : juce::Colour(0xff9ca5aa));
            graphics.drawFittedText(items[static_cast<size_t>(index)].text, label.toNearestInt(),
                                    juce::Justification::centred, 1, 0.72F);
        }
    }

    for (int tick = 0; tick < count; ++tick) {
        const auto proportion =
            count == 1 ? 0.5F : static_cast<float>(tick) / static_cast<float>(count - 1);
        const auto angle = rotaryStart + proportion * (rotaryEnd - rotaryStart);
        const auto inner = knob.getWidth() * 0.56F;
        const auto outer = knob.getWidth() * 0.64F;
        graphics.setColour(tick == selectedIndex ? QuadBeatLookAndFeel::accent()
                                                 : juce::Colour(0xff596166));
        graphics.drawLine(centre.x + std::sin(angle) * inner, centre.y - std::cos(angle) * inner,
                          centre.x + std::sin(angle) * outer, centre.y - std::cos(angle) * outer,
                          tick == selectedIndex ? 2.0F : 1.0F);
    }

    juce::ColourGradient face(juce::Colour(0xff3c3f41), knob.getX(), knob.getY(),
                              juce::Colour(0xff090a0b), knob.getRight(), knob.getBottom(), false);
    graphics.setGradientFill(face);
    graphics.fillEllipse(knob);
    graphics.setColour(juce::Colour(0xff050607));
    graphics.drawEllipse(knob, 2.0F);
    graphics.setColour(juce::Colour(0xff6a6f72));
    graphics.drawEllipse(knob.reduced(4.0F), 1.0F);

    const auto position =
        count == 1 ? 0.5F : static_cast<float>(selectedIndex) / static_cast<float>(count - 1);
    const auto angle = rotaryStart + position * (rotaryEnd - rotaryStart);
    graphics.setColour(juce::Colour(0xffe5e5df));
    graphics.drawLine(centre.x, centre.y, centre.x + std::sin(angle) * knob.getWidth() * 0.35F,
                      centre.y - std::cos(angle) * knob.getWidth() * 0.35F, 3.0F);

    graphics.setFont(juce::FontOptions(10.0F, juce::Font::bold));
    graphics.setColour(juce::Colour(0xffe5e8e8));
    graphics.drawFittedText(getText(), getLocalBounds().removeFromBottom(18),
                            juce::Justification::centred, 1);
    if (hasKeyboardFocus(true)) {
        graphics.setColour(QuadBeatLookAndFeel::accent());
        graphics.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0F), 3.0F, 1.0F);
    }
}

void HardwareRotarySelector::mouseDown(const juce::MouseEvent& event) {
    grabKeyboardFocus();
    dragStartIndex = selectedIndex;
    dragStartY = event.getScreenY();
}

void HardwareRotarySelector::mouseDrag(const juce::MouseEvent& event) {
    const auto delta = (dragStartY - event.getScreenY()) / 12;
    setSelectedItemIndex(dragStartIndex + delta);
}

void HardwareRotarySelector::mouseWheelMove(const juce::MouseEvent&,
                                            const juce::MouseWheelDetails& wheel) {
    step(wheel.deltaY > 0.0F ? 1 : -1);
}

bool HardwareRotarySelector::keyPressed(const juce::KeyPress& key) {
    if (key == juce::KeyPress::leftKey || key == juce::KeyPress::downKey) {
        step(-1);
        return true;
    }
    if (key == juce::KeyPress::rightKey || key == juce::KeyPress::upKey) {
        step(1);
        return true;
    }
    return false;
}

void HardwareRotarySelector::step(const int delta) { setSelectedItemIndex(selectedIndex + delta); }
} // namespace qb
