#include "ui/MainComponent.h"

namespace qb {
namespace {
void drawMeter(juce::Graphics& graphics, const juce::Rectangle<int> bounds, const float level,
               const int segments) {
    const auto gap = 2;
    const auto segmentHeight = std::max(2, (bounds.getHeight() - gap * (segments - 1)) / segments);
    const auto active = static_cast<int>(std::round(std::clamp(level, 0.0F, 1.0F) * segments));
    for (int segment = 0; segment < segments; ++segment) {
        const auto y = bounds.getBottom() - (segment + 1) * segmentHeight - segment * gap;
        auto colour = segment >= segments - 3   ? juce::Colour(0xffff3b43)
                      : segment >= segments - 7 ? juce::Colour(0xffffc728)
                                                : juce::Colour(0xff31df67);
        if (segment >= active)
            colour = colour.withAlpha(0.12F);
        graphics.setColour(colour);
        graphics.fillRect(bounds.getX(), y, bounds.getWidth(), segmentHeight);
    }
}
} // namespace

void MainComponent::paint(juce::Graphics& graphics) {
    juce::ColourGradient background(juce::Colour(0xff202326), 0.0F, 0.0F,
                                    QuadBeatLookAndFeel::background(), 0.0F,
                                    static_cast<float>(getHeight()), false);
    graphics.setGradientFill(background);
    graphics.fillAll();
    graphics.setColour(juce::Colour(0xff292d30).withAlpha(0.35F));
    for (int y = 52; y < getHeight(); y += 4)
        graphics.drawHorizontalLine(y, 0.0F, static_cast<float>(getWidth()));

    graphics.setColour(juce::Colour(0xff777d80));
    graphics.drawLine(8.0F, 49.0F, static_cast<float>(getWidth() - 8), 49.0F, 1.0F);

    const auto drawPanel = [&graphics](const juce::Rectangle<int> area, const bool raised) {
        juce::ColourGradient face(raised ? juce::Colour(0xff1d2023) : juce::Colour(0xff141719),
                                  static_cast<float>(area.getX()), static_cast<float>(area.getY()),
                                  juce::Colour(0xff07090a), static_cast<float>(area.getRight()),
                                  static_cast<float>(area.getBottom()), false);
        graphics.setGradientFill(face);
        graphics.fillRect(area);
        graphics.setColour(juce::Colour(0xff4a5054));
        graphics.drawRect(area, 1);
        graphics.setColour(juce::Colour(0xff050607));
        for (const auto point : {area.getTopLeft() + juce::Point<int>(7, 7),
                                 area.getTopRight() + juce::Point<int>(-8, 7),
                                 area.getBottomLeft() + juce::Point<int>(7, -8),
                                 area.getBottomRight() + juce::Point<int>(-8, -8)})
            graphics.fillEllipse(static_cast<float>(point.x - 2), static_cast<float>(point.y - 2),
                                 4.0F, 4.0F);
    };

    if (!hostedByPlugin) {
        drawPanel(utilityBounds, true);
        graphics.setColour(juce::Colour(0xffe3e6e7));
        graphics.setFont(juce::FontOptions(11.0F, juce::Font::bold));
        graphics.drawText("MIC / PHONES", utilityBounds.withHeight(25),
                          juce::Justification::centred);
        graphics.setColour(juce::Colour(0xff9ea5a8));
        graphics.setFont(juce::FontOptions(9.0F, juce::Font::bold));
        for (const auto* knob : {&microphone, &headphones, &cueMix})
            graphics.drawText(knob->getName(), knob->getBounds().translated(0, -15).withHeight(14),
                              juce::Justification::centred);
        const auto utilityDividerY = microphoneMute.getBottom() + 18;
        graphics.setColour(juce::Colour(0xff555b5e));
        graphics.drawHorizontalLine(utilityDividerY, static_cast<float>(utilityBounds.getX() + 10),
                                    static_cast<float>(utilityBounds.getRight() - 10));
    }

    drawPanel(monitorBounds, true);
    graphics.setColour(juce::Colour(0xffe3e6e7));
    graphics.setFont(juce::FontOptions(11.0F, juce::Font::bold));
    graphics.drawText("MASTER", monitorBounds.withHeight(25), juce::Justification::centred);
    graphics.setColour(juce::Colour(0xffaeb4b6));
    graphics.setFont(juce::FontOptions(9.0F, juce::Font::bold));
    for (const auto* knob : {&master, &booth}) {
        if (!knob->isVisible())
            continue;
        graphics.drawText(knob->getName(), knob->getBounds().translated(0, -15).withHeight(14),
                          juce::Justification::centred);
    }
    graphics.drawText("MASTER LEVEL", masterMeterBounds.translated(0, -17).withHeight(14),
                      juce::Justification::centred);
    auto meters = masterMeterBounds.reduced(masterMeterBounds.getWidth() / 4, 0);
    const auto meterWidth = std::max(5, meters.getWidth() / 4);
    drawMeter(graphics, meters.removeFromLeft(meterWidth), masterMeters[0], 24);
    meters.removeFromLeft(meterWidth);
    drawMeter(graphics, meters.removeFromLeft(meterWidth), masterMeters[1], 24);

    drawPanel(fxBounds, true);
    graphics.setColour(juce::Colour(0xffe4e7e8));
    graphics.setFont(juce::FontOptions(11.0F, juce::Font::bold));
    graphics.drawText("QUADBEAT / BEAT FX", fxBounds.withHeight(25), juce::Justification::centred);
    const auto displayFrame = display.getBounds().expanded(7, 5);
    graphics.setColour(juce::Colour(0xff020303));
    graphics.fillRect(displayFrame);
    graphics.setColour(juce::Colour(0xff5b6266));
    graphics.drawRect(displayFrame, 2);

    graphics.setColour(juce::Colour(0xffaeb4b6));
    graphics.setFont(juce::FontOptions(9.0F, juce::Font::bold));
    graphics.drawText("X-PAD",
                      pads.front().getBounds().translated(0, -15).withWidth(pads.back().getRight() -
                                                                            pads.front().getX()),
                      juce::Justification::centred);
    graphics.drawText("FX FREQUENCY",
                      lowButton.getBounds().translated(0, -15).withWidth(highButton.getRight() -
                                                                         lowButton.getX()),
                      juce::Justification::centred);
    graphics.drawText("BEAT FX SELECT",
                      effectSelector.getBounds().translated(0, -15).withHeight(13),
                      juce::Justification::centred);
    graphics.drawText("FX ASSIGN", busSelector.getBounds().translated(0, -15).withHeight(13),
                      juce::Justification::centred);
    graphics.drawText("TIME", time.getBounds().translated(0, -14).withHeight(13),
                      juce::Justification::centred);
    graphics.drawText("LEVEL / DEPTH", depth.getBounds().translated(0, -14).withHeight(13),
                      juce::Justification::centred);
}

void MainComponent::resized() {
    auto bounds = getLocalBounds().reduced(8);
    auto header = bounds.removeFromTop(42);
    productLabel.setBounds(header.removeFromLeft(230));
    settingsButton.setBounds(header.removeFromRight(92).reduced(2, 4));
    midiButton.setBounds(header.removeFromRight(105).reduced(2, 4));
    midiEditButton.setBounds(header.removeFromRight(92).reduced(2, 4));
    scaleSelector.setBounds(header.removeFromRight(76).reduced(2, 4));
    statusLabel.setBounds(header);

    auto utilityArea = juce::Rectangle<int>();
    if (!hostedByPlugin) {
        const auto utilityWidth = std::clamp(bounds.getWidth() * 20 / 100, 180, 230);
        utilityArea = bounds.removeFromLeft(utilityWidth).reduced(3, 2);
    }
    const auto monitorWidth = std::clamp(bounds.getWidth() * 25 / 100, 205, 275);
    auto monitorArea = bounds.removeFromLeft(monitorWidth).reduced(3, 2);
    auto fxArea = bounds.reduced(3, 2);

    utilityBounds = utilityArea;
    monitorBounds = monitorArea;
    fxBounds = fxArea;

    if (!hostedByPlugin) {
        auto utility = utilityArea.reduced(9, 8);
        utility.removeFromTop(39);
        microphone.setBounds(utility.removeFromTop(112).reduced(8, 0));
        auto micButtons = utility.removeFromTop(35);
        microphoneCue.setBounds(micButtons.removeFromLeft(micButtons.getWidth() / 2).reduced(2));
        microphoneMute.setBounds(micButtons.reduced(2));
        utility.removeFromTop(36);
        auto phonesSlot = utility.removeFromTop(std::min(150, utility.getHeight() / 2));
        phonesSlot.removeFromTop(16);
        headphones.setBounds(phonesSlot.reduced(7, 3));
        if (utility.getHeight() > 0) {
            utility.removeFromTop(std::min(18, utility.getHeight()));
            cueMix.setBounds(utility.reduced(7, 3));
        }
    }

    auto monitor = monitorArea.reduced(8, 8);
    monitor.removeFromTop(38);
    master.setBounds(
        monitor.removeFromTop(std::min(hostedByPlugin ? 190 : 150, monitor.getHeight() / 3))
            .reduced(6, 0));
    monitor.removeFromTop(std::min(26, monitor.getHeight()));
    masterMeterBounds =
        monitor.removeFromTop(std::min(hostedByPlugin ? 320 : 235, monitor.getHeight() * 2 / 3))
            .reduced(6, 0);
    if (!hostedByPlugin) {
        monitor.removeFromTop(std::min(28, monitor.getHeight()));
        booth.setBounds(monitor.reduced(7, 2));
    }

    auto fx = fxArea.reduced(10, 7);
    fx.removeFromTop(25);
    const auto panelHeight = fx.getHeight();
    const auto displayHeight = std::clamp(panelHeight * 24 / 100, 135, 175);
    const auto padHeight = std::clamp(panelHeight * 8 / 100, 46, 58);

    display.setBounds(fx.removeFromTop(displayHeight).reduced(7, 6));
    fx.removeFromTop(16);
    auto padArea = fx.removeFromTop(padHeight);
    const auto padRowHeight = padArea.getHeight() / 2;
    for (int row = 0; row < 2; ++row) {
        auto rowArea = padArea.removeFromTop(padRowHeight);
        for (int column = 0; column < 4; ++column) {
            const auto index = row * 4 + column;
            pads[static_cast<size_t>(index)].setBounds(
                rowArea.removeFromLeft(rowArea.getWidth() / (4 - column)).reduced(2));
        }
    }

    auto arrows = fx.removeFromTop(32);
    beatLeft.setBounds(arrows.removeFromLeft(std::min(60, arrows.getWidth() / 3)).reduced(2, 3));
    beatRight.setBounds(arrows.removeFromRight(std::min(60, arrows.getWidth() / 2)).reduced(2, 3));

    auto tempoArea = fx.removeFromTop(56);
    autoButton.setBounds(tempoArea.removeFromLeft(tempoArea.getWidth() / 3).reduced(3, 11));
    quantizeButton.setBounds(tempoArea.removeFromRight(tempoArea.getWidth() / 2).reduced(3, 11));
    tapButton.setBounds(tempoArea.reduced(2));

    fx.removeFromTop(14);
    auto bandsArea = fx.removeFromTop(33);
    lowButton.setBounds(bandsArea.removeFromLeft(bandsArea.getWidth() / 3).reduced(2));
    midButton.setBounds(bandsArea.removeFromLeft(bandsArea.getWidth() / 2).reduced(2));
    highButton.setBounds(bandsArea.reduced(2));

    fx.removeFromTop(16);
    const auto selectorHeight =
        std::min(std::clamp(panelHeight * 23 / 100, 140, 175), std::max(0, fx.getHeight() - 100));
    auto selectors = fx.removeFromTop(selectorHeight);
    effectSelector.setBounds(selectors.removeFromLeft(selectors.getWidth() * 68 / 100).reduced(4));
    busSelector.setBounds(selectors.reduced(4));
    fx.removeFromTop(std::min(10, fx.getHeight()));
    auto knobs = fx;
    time.setBounds(knobs.removeFromLeft(knobs.getWidth() / 2).reduced(14, 1));
    depth.setBounds(knobs.reduced(14, 1));
}
} // namespace qb
