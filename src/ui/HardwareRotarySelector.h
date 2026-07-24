#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace qb {
class HardwareRotarySelector final : public juce::Component {
  public:
    HardwareRotarySelector();

    void addItem(const juce::String& text, int itemId);
    void addItemList(const juce::StringArray& items, int firstItemId);
    int getNumItems() const noexcept;
    int getSelectedItemIndex() const noexcept;
    juce::String getText() const;
    void setSelectedId(int itemId, juce::NotificationType notification = juce::sendNotification);
    void setSelectedItemIndex(int index,
                              juce::NotificationType notification = juce::sendNotification);

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed(const juce::KeyPress&) override;

    std::function<void()> onChange;

  private:
    struct Item {
        juce::String text;
        int id{};
    };

    void step(int delta);
    juce::Rectangle<float> knobBounds() const;

    std::vector<Item> items;
    int selectedIndex{};
    int dragStartIndex{};
    int dragStartY{};
};
} // namespace qb
