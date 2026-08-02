#include "ui/BeatFxDisplay.h"
#include "ui/QuadBeatLookAndFeel.h"
#include <algorithm>
#include <cmath>

namespace qb {
namespace {
juce::String beatLabel(const int index) {
    const auto& text = beatLabels[static_cast<size_t>(std::clamp(index, 0, 7))];
    return {text.data(), text.size()};
}
} // namespace

BeatFxDisplay::BeatFxDisplay() {
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setTitle("Beat FX touchscreen");
    setDescription("Displays tempo, effect, beat division, time, and depth");
}

void BeatFxDisplay::setState(const EffectType nextEffect, const double nextBpm,
                             const TempoSource nextSource, const double nextConfidence,
                             const int nextDivision, const double nextTime,
                             const double nextDepth) {
    effect = nextEffect;
    bpm = std::clamp(nextBpm, 60.0, 200.0);
    source = nextSource;
    confidence = std::clamp(nextConfidence, 0.0, 1.0);
    division = std::clamp(nextDivision, 0, 7);
    time = std::clamp(nextTime, 0.0, 1.0);
    depth = std::clamp(nextDepth, 0.0, 1.0);
    repaint();
}

juce::String BeatFxDisplay::sourceText() const {
    switch (source) {
    case TempoSource::automatic:
        return confidence > 0.01 ? "AUTO " + juce::String(std::round(confidence * 100.0), 0) + "%"
                                 : "AUTO";
    case TempoSource::manual:
        return "MANUAL";
    case TempoSource::tap:
        return "TAP";
    }
    return {};
}

juce::String BeatFxDisplay::timeText() const {
    switch (effect) {
    case EffectType::delay:
    case EffectType::echo:
    case EffectType::pingPong:
    case EffectType::spiral:
    case EffectType::flanger:
    case EffectType::helix:
        return juce::String(std::round(20.0 + time * 980.0), 0) + " msec";
    case EffectType::reverb:
    case EffectType::vinylBrake:
        return juce::String(0.1 + time * 5.9, 1) + " sec";
    case EffectType::pitch:
        return juce::String(std::round((time * 2.0 - 1.0) * 12.0), 0) + " st";
    case EffectType::slipRoll:
    case EffectType::roll:
    case EffectType::trans:
        return beatLabel(division) + " beat";
    case EffectType::filter:
    case EffectType::phaser:
    case EffectType::pan:
    case EffectType::count:
        return juce::String(std::round(time * 100.0), 0) + "%";
    }
    return {};
}

juce::Rectangle<int> BeatFxDisplay::beatRowBounds() const {
    return getLocalBounds().reduced(8).removeFromBottom(std::max(24, getHeight() * 23 / 100));
}

void BeatFxDisplay::paint(juce::Graphics& graphics) {
    auto bounds = getLocalBounds().toFloat().reduced(1.0F);
    graphics.setColour(juce::Colour(0xff050607));
    graphics.fillRoundedRectangle(bounds, 3.0F);
    graphics.setColour(hasKeyboardFocus(true) ? QuadBeatLookAndFeel::accent()
                                              : juce::Colour(0xff697177));
    graphics.drawRoundedRectangle(bounds, 3.0F, hasKeyboardFocus(true) ? 2.0F : 1.0F);

    auto content = getLocalBounds().reduced(8);
    const auto touchRow = beatRowBounds();
    content.removeFromBottom(touchRow.getHeight() + 3);

    const auto modeHeight = std::clamp(getHeight() * 8 / 100, 8, 14);
    const auto modeBand = content.removeFromBottom(std::min(modeHeight, content.getHeight()));
    content.removeFromBottom(std::min(2, content.getHeight()));

    graphics.setColour(juce::Colour(0xff292d35));
    graphics.fillRect(content.getUnion(modeBand));
    graphics.setColour(juce::Colour(0xff4a515d));
    graphics.drawRect(content.getUnion(modeBand), 1);

    const auto primaryHeight = content.getHeight();
    auto top = content.removeFromTop(
        std::min(content.getHeight(), std::clamp(primaryHeight * 38 / 100, 20, 42)));
    auto badge = top.removeFromLeft(std::max(46, top.getWidth() * 18 / 100)).reduced(3, 5);
    graphics.setColour(source == TempoSource::tap ? juce::Colour(0xff26d967)
                                                  : juce::Colour(0xff81b83c));
    graphics.drawRoundedRectangle(badge.toFloat(), 2.0F, 1.5F);
    graphics.setFont(
        juce::FontOptions(std::max(10.0F, badge.getHeight() * 0.47F), juce::Font::bold));
    graphics.drawFittedText(sourceText(), badge, juce::Justification::centred, 1);

    auto bpmArea = top.removeFromRight(std::max(92, top.getWidth() * 44 / 100));
    graphics.setColour(juce::Colour(0xfff0f0df));
    graphics.setFont(
        juce::FontOptions(std::max(17.0F, bpmArea.getHeight() * 0.62F), juce::Font::bold));
    graphics.drawFittedText(juce::String(bpm, 1), bpmArea.withTrimmedRight(28),
                            juce::Justification::centredRight, 1);
    graphics.setFont(juce::FontOptions(8.0F, juce::Font::bold));
    graphics.drawText("BPM", bpmArea.removeFromRight(27), juce::Justification::centredLeft);

    graphics.setColour(juce::Colour(0xfff3f3e9));
    graphics.setFont(juce::FontOptions(std::max(16.0F, top.getHeight() * 0.67F), juce::Font::bold));
    const auto effectIndex = std::min(static_cast<size_t>(effect), effectNames.size() - 1);
    const auto& effectName = effectNames[effectIndex];
    graphics.drawFittedText(juce::String(effectName.data(), effectName.size()), top,
                            juce::Justification::centred, 1);

    auto selector = content.removeFromTop(
        std::min(content.getHeight(), std::clamp(primaryHeight * 40 / 100, 20, 46)));
    const auto cellWidth = selector.getWidth() / 3;
    const std::array<int, 3> indices{std::max(0, division - 1), division,
                                     std::min(7, division + 1)};
    for (int cell = 0; cell < 3; ++cell) {
        auto area = selector.removeFromLeft(cell == 2 ? selector.getWidth() : cellWidth);
        const auto selected = cell == 1;
        graphics.setColour(selected ? juce::Colour(0xfff2f0dc) : juce::Colour(0xff30343d));
        graphics.fillRect(area);
        graphics.setColour(juce::Colour(0xff707884));
        graphics.drawRect(area, 1);
        graphics.setColour(selected ? juce::Colour(0xff202226) : juce::Colour(0xfff1f1e6));
        graphics.setFont(
            juce::FontOptions(std::max(13.0F, area.getHeight() * 0.45F), juce::Font::bold));
        graphics.drawText(beatLabel(indices[static_cast<size_t>(cell)]), area,
                          juce::Justification::centred);
    }

    graphics.setColour(juce::Colour(0xffd8d9d1));
    graphics.setFont(
        juce::FontOptions(std::max(9.0F, content.getHeight() * 0.28F), juce::Font::bold));
    graphics.drawFittedText("TIME   " + timeText() + "     DEPTH " +
                                juce::String(std::round(depth * 100.0), 0) + "%",
                            content, juce::Justification::centred, 1);

    const auto cell = touchRow.getWidth() / 8;
    for (int index = 0; index < 8; ++index) {
        auto area =
            touchRow.withX(touchRow.getX() + index * cell)
                .withWidth(index == 7 ? touchRow.getRight() - (touchRow.getX() + index * cell)
                                      : cell);
        const auto selected = index == division;
        graphics.setColour(selected ? juce::Colour(0xffeae9d5) : juce::Colour(0xff252932));
        graphics.fillRect(area);
        graphics.setColour(juce::Colour(0xff59616d));
        graphics.drawRect(area, 1);
        graphics.setColour(selected ? juce::Colour(0xff1e2024) : juce::Colour(0xffecece4));
        graphics.setFont(
            juce::FontOptions(std::max(8.0F, area.getHeight() * 0.34F), juce::Font::bold));
        graphics.drawFittedText(beatLabel(index), area, juce::Justification::centred, 1);
    }

    graphics.setColour(juce::Colour(0xff76dce7));
    graphics.fillRect(modeBand);
    graphics.setColour(juce::Colour(0xff18373c));
    graphics.setFont(
        juce::FontOptions(std::max(8.0F, modeBand.getHeight() * 0.48F), juce::Font::bold));
    graphics.drawText("BEAT", modeBand, juce::Justification::centred);
}

void BeatFxDisplay::mouseDown(const juce::MouseEvent& event) {
    grabKeyboardFocus();
    const auto row = beatRowBounds();
    if (!row.contains(event.getPosition()))
        return;
    const auto cellWidth = static_cast<double>(row.getWidth()) / 8.0;
    selectDivision(static_cast<int>((event.x - row.getX()) / cellWidth));
}

bool BeatFxDisplay::keyPressed(const juce::KeyPress& key) {
    if (key == juce::KeyPress::leftKey) {
        selectDivision(division - 1);
        return true;
    }
    if (key == juce::KeyPress::rightKey) {
        selectDivision(division + 1);
        return true;
    }
    return false;
}

void BeatFxDisplay::selectDivision(const int index) {
    const auto next = std::clamp(index, 0, 7);
    if (onDivisionSelected)
        onDivisionSelected(next);
}
} // namespace qb
