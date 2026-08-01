#pragma once

#include "model/Types.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

namespace qb {
class BeatFxDisplay final : public juce::Component {
  public:
    BeatFxDisplay();

    void setState(EffectType effect, double bpm, TempoSource source, double confidence,
                  int division, double time, double depth);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    bool keyPressed(const juce::KeyPress&) override;

    std::function<void(int)> onDivisionSelected;

  private:
    juce::String timeText() const;
    juce::String sourceText() const;
    juce::Rectangle<int> beatRowBounds() const;
    void selectDivision(int index);

    EffectType effect{EffectType::echo};
    double bpm{120.0};
    double confidence{};
    double time{0.5};
    double depth{0.5};
    TempoSource source{TempoSource::automatic};
    int division{5};
};
} // namespace qb
