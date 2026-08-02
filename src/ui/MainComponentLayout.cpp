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

    drawPanel(utilityBounds, true);
    graphics.setColour(juce::Colour(0xffe3e6e7));
    graphics.setFont(juce::FontOptions(11.0F, juce::Font::bold));
    graphics.drawText("MIC / PHONES", utilityBounds.withHeight(25), juce::Justification::centred);
    graphics.setColour(juce::Colour(0xff9ea5a8));
    graphics.setFont(juce::FontOptions(9.0F, juce::Font::bold));
    for (const auto* knob : {&microphone, &headphones, &cueMix})
        graphics.drawText(knob->getName(), knob->getBounds().translated(0, -15).withHeight(14),
                          juce::Justification::centred);
    const auto utilityDividerY = microphoneMute.getBottom() + 18;
    graphics.setColour(juce::Colour(0xff555b5e));
    graphics.drawHorizontalLine(utilityDividerY, static_cast<float>(utilityBounds.getX() + 10),
                                static_cast<float>(utilityBounds.getRight() - 10));

    graphics.setFont(juce::FontOptions(9.0F, juce::Font::bold));
    for (int index = 0; index < channelCount; ++index) {
        const auto& area = channelBounds[static_cast<size_t>(index)];
        drawPanel(area, false);
        const auto& controls = channelControls[static_cast<size_t>(index)];
        for (const auto* knob : {&controls.trim, &controls.high, &controls.mid, &controls.low}) {
            graphics.setColour(juce::Colour(0xffbec3c5));
            graphics.drawText(knob->getName(), knob->getBounds().translated(0, -12).withHeight(12),
                              juce::Justification::centred);
        }
        graphics.setColour(juce::Colour(0xff939a9d));
        graphics.drawText("CHANNEL FADER",
                          controls.fader.getBounds().translated(0, -14).withHeight(13),
                          juce::Justification::centred);

        const auto fader = controls.fader.getBounds();
        graphics.setColour(juce::Colour(0xff555b5f));
        for (int tick = 0; tick <= 10; ++tick) {
            const auto y = fader.getY() + tick * fader.getHeight() / 10;
            const auto half = tick % 5 == 0 ? 9.0F : 5.0F;
            graphics.drawLine(static_cast<float>(fader.getCentreX()) - half, static_cast<float>(y),
                              static_cast<float>(fader.getCentreX()) + half, static_cast<float>(y),
                              1.0F);
        }

        const auto meterTop = controls.trim.getY() + 4;
        const auto meterBottom = controls.fader.getBottom();
        drawMeter(graphics,
                  {area.getRight() - 10, meterTop, 4, std::max(1, meterBottom - meterTop)},
                  controls.meter, 26);
    }

    drawPanel(monitorBounds, true);
    graphics.setColour(juce::Colour(0xffe3e6e7));
    graphics.setFont(juce::FontOptions(11.0F, juce::Font::bold));
    graphics.drawText("MASTER", monitorBounds.withHeight(25), juce::Justification::centred);
    graphics.setColour(juce::Colour(0xffaeb4b6));
    graphics.setFont(juce::FontOptions(9.0F, juce::Font::bold));
    for (const auto* knob : {&master, &booth})
        graphics.drawText(knob->getName(), knob->getBounds().translated(0, -15).withHeight(14),
                          juce::Justification::centred);
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

    drawPanel(crossfaderBounds, false);
    graphics.setColour(juce::Colour(0xff9ca3a5));
    graphics.setFont(juce::FontOptions(9.0F, juce::Font::bold));
    graphics.drawText("CROSSFADER ASSIGNMENT", crossfaderBounds.withHeight(17),
                      juce::Justification::centred);
}

void MainComponent::layoutChannel(ChannelControls& channel, juce::Rectangle<int> area) {
    area.reduce(7, 5);
    area.removeFromRight(9);
    channel.title.setBounds(area.removeFromTop(22));
    channel.eqMode.setBounds(area.removeFromTop(24).reduced(3, 2));
    area.removeFromTop(9);
    channel.trim.setBounds(area.removeFromTop(62));
    for (auto* knob : {&channel.high, &channel.mid, &channel.low}) {
        area.removeFromTop(11);
        knob->setBounds(area.removeFromTop(54));
    }
    area.removeFromTop(5);
    auto buttons = area.removeFromTop(34);
    channel.cue.setBounds(buttons.removeFromLeft(buttons.getWidth() / 2).reduced(2));
    channel.mute.setBounds(buttons.reduced(2));
    area.removeFromTop(12);
    channel.assignment.setBounds(area.removeFromBottom(28).reduced(3, 1));
    channel.fader.setBounds(area.reduced(9, 2));
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

    const auto utilityWidth = std::clamp(bounds.getWidth() * 12 / 100, 150, 205);
    const auto rightWidth = std::clamp(bounds.getWidth() * 31 / 100, 370, 535);
    auto utilityArea = bounds.removeFromLeft(utilityWidth).reduced(3, 2);
    auto rightArea = bounds.removeFromRight(rightWidth).reduced(3, 2);
    auto footer = bounds.removeFromBottom(72);
    auto mixerArea = bounds.reduced(0, 2);

    const auto monitorWidth = std::clamp(rightArea.getWidth() * 36 / 100, 132, 190);
    auto monitorArea = rightArea.removeFromLeft(monitorWidth).reduced(0, 0);
    auto fxArea = rightArea.reduced(3, 0);

    utilityBounds = utilityArea;
    monitorBounds = monitorArea;
    fxBounds = fxArea;
    crossfaderBounds = footer.reduced(3, 2);
    crossfader.setBounds(crossfaderBounds.reduced(24, 16).withTrimmedTop(5));

    const auto stripWidth = mixerArea.getWidth() / channelCount;
    for (int index = 0; index < channelCount; ++index) {
        auto area =
            mixerArea.removeFromLeft(index == channelCount - 1 ? mixerArea.getWidth() : stripWidth)
                .reduced(3, 0);
        channelBounds[static_cast<size_t>(index)] = area;
        layoutChannel(channelControls[static_cast<size_t>(index)], area);
    }

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

    auto monitor = monitorArea.reduced(8, 8);
    monitor.removeFromTop(38);
    master.setBounds(monitor.removeFromTop(std::min(150, monitor.getHeight() / 4)).reduced(6, 0));
    monitor.removeFromTop(std::min(26, monitor.getHeight()));
    masterMeterBounds = monitor.removeFromTop(std::min(235, monitor.getHeight() / 2)).reduced(6, 0);
    monitor.removeFromTop(std::min(28, monitor.getHeight()));
    booth.setBounds(monitor.reduced(7, 2));

    auto fx = fxArea.reduced(10, 7);
    fx.removeFromTop(25);
    const auto panelHeight = fx.getHeight();
    const auto displayHeight = std::clamp(panelHeight * 16 / 100, 125, 165);
    const auto padHeight = std::clamp(panelHeight * 6 / 100, 42, 55);
    const auto effectHeight = std::clamp(panelHeight * 12 / 100, 82, 112);
    const auto busHeight = std::clamp(panelHeight * 9 / 100, 65, 86);

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

    fx.removeFromTop(15);
    auto bandsArea = fx.removeFromTop(33);
    lowButton.setBounds(bandsArea.removeFromLeft(bandsArea.getWidth() / 3).reduced(2));
    midButton.setBounds(bandsArea.removeFromLeft(bandsArea.getWidth() / 2).reduced(2));
    highButton.setBounds(bandsArea.reduced(2));

    fx.removeFromTop(15);
    effectSelector.setBounds(fx.removeFromTop(std::min(effectHeight, fx.getHeight())));
    fx.removeFromTop(std::min(15, fx.getHeight()));
    busSelector.setBounds(fx.removeFromTop(std::min(busHeight, fx.getHeight())));
    fx.removeFromTop(std::min(14, fx.getHeight()));
    const auto timeHeight = std::min(std::max(54, fx.getHeight() * 42 / 100), fx.getHeight());
    time.setBounds(fx.removeFromTop(timeHeight).reduced(10, 1));
    if (fx.getHeight() > 0) {
        fx.removeFromTop(std::min(14, fx.getHeight()));
        depth.setBounds(fx.reduced(10, 1));
    }
}
} // namespace qb
