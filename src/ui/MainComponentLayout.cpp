#include "ui/MainComponent.h"

namespace qb {
void MainComponent::paint(juce::Graphics& graphics) {
    juce::ColourGradient background(juce::Colour(0xff171a1c), 0.0F, 0.0F,
                                    QuadBeatLookAndFeel::background(), 0.0F,
                                    static_cast<float>(getHeight()), false);
    graphics.setGradientFill(background);
    graphics.fillAll();

    graphics.setColour(juce::Colour(0xff33383b));
    graphics.drawLine(8.0F, 49.0F, static_cast<float>(getWidth() - 8), 49.0F, 1.0F);

    const auto drawPanel = [&graphics](const juce::Rectangle<int> area, const bool raised) {
        juce::ColourGradient face(raised ? juce::Colour(0xff181b1d) : juce::Colour(0xff101214),
                                  static_cast<float>(area.getX()), static_cast<float>(area.getY()),
                                  juce::Colour(0xff070809), static_cast<float>(area.getRight()),
                                  static_cast<float>(area.getBottom()), false);
        graphics.setGradientFill(face);
        graphics.fillRoundedRectangle(area.toFloat(), 3.0F);
        graphics.setColour(juce::Colour(0xff3d4245));
        graphics.drawRoundedRectangle(area.toFloat().reduced(0.5F), 3.0F, 1.0F);
        graphics.setColour(juce::Colour(0xff050607));
        graphics.fillEllipse(static_cast<float>(area.getX() + 6),
                             static_cast<float>(area.getY() + 6), 4.0F, 4.0F);
        graphics.fillEllipse(static_cast<float>(area.getRight() - 10),
                             static_cast<float>(area.getY() + 6), 4.0F, 4.0F);
    };

    graphics.setFont(juce::FontOptions(10.0F, juce::Font::bold));
    for (int index = 0; index < channelCount; ++index) {
        const auto& area = channelBounds[static_cast<size_t>(index)];
        drawPanel(area, false);
        const auto& controls = channelControls[static_cast<size_t>(index)];
        for (const auto* knob : {&controls.trim, &controls.high, &controls.mid, &controls.low}) {
            auto label = knob->getBounds().translated(0, -13).withHeight(13);
            graphics.setColour(juce::Colour(0xffb8bdbf));
            graphics.drawText(knob->getName(), label, juce::Justification::centred);
        }
        graphics.setColour(juce::Colour(0xff8f9699));
        graphics.drawText("CH FADER", controls.fader.getBounds().translated(0, -14).withHeight(13),
                          juce::Justification::centred);

        const auto meterTop = controls.trim.getY() + 5;
        const auto meterBottom = controls.fader.getBottom();
        const auto meterX = area.getRight() - 12;
        constexpr auto segmentCount = 24;
        const auto gap = 2;
        const auto segmentHeight =
            std::max(2, (meterBottom - meterTop - gap * (segmentCount - 1)) / segmentCount);
        const auto activeSegments =
            static_cast<int>(std::round(std::clamp(controls.meter, 0.0F, 1.0F) * segmentCount));
        for (int segment = 0; segment < segmentCount; ++segment) {
            const auto y = meterBottom - (segment + 1) * segmentHeight - segment * gap;
            const auto active = segment < activeSegments;
            auto colour = segment >= 21   ? juce::Colour(0xffff4545)
                          : segment >= 17 ? juce::Colour(0xffffcf35)
                                          : juce::Colour(0xff73e637);
            if (!active)
                colour = colour.withAlpha(0.13F);
            graphics.setColour(colour);
            graphics.fillRoundedRectangle(static_cast<float>(meterX), static_cast<float>(y), 5.0F,
                                          static_cast<float>(segmentHeight), 1.0F);
        }
    }

    drawPanel(monitorBounds, true);
    graphics.setColour(juce::Colour(0xffc9cdce));
    graphics.setFont(juce::FontOptions(11.0F, juce::Font::bold));
    graphics.drawText("MONITOR", monitorBounds.withHeight(24), juce::Justification::centred);
    for (const auto* knob : {&master, &booth, &headphones, &cueMix}) {
        graphics.setColour(juce::Colour(0xffb8bdbf));
        graphics.drawText(knob->getName(), knob->getBounds().translated(0, -15).withHeight(15),
                          juce::Justification::centred);
    }

    drawPanel(fxBounds, true);
    graphics.setColour(juce::Colour(0xffe1e4e4));
    graphics.setFont(juce::FontOptions(12.0F, juce::Font::bold));
    graphics.drawText("QUADBEAT  /  BEAT FX", fxBounds.withHeight(25),
                      juce::Justification::centred);

    const auto oledFrame = display.getBounds().expanded(7, 5);
    graphics.setColour(juce::Colour(0xff020303));
    graphics.fillRoundedRectangle(oledFrame.toFloat(), 3.0F);
    graphics.setColour(juce::Colour(0xff414849));
    graphics.drawRoundedRectangle(oledFrame.toFloat(), 3.0F, 2.0F);
    juce::ColourGradient oledFace(juce::Colour(0xff15302e), static_cast<float>(oledFrame.getX()),
                                  static_cast<float>(oledFrame.getY()), juce::Colour(0xff06100f),
                                  static_cast<float>(oledFrame.getRight()),
                                  static_cast<float>(oledFrame.getBottom()), false);
    graphics.setGradientFill(oledFace);
    graphics.fillRect(display.getBounds().toFloat());
    graphics.setColour(QuadBeatLookAndFeel::oled().withAlpha(0.10F));
    for (auto y = display.getY() + 3; y < display.getBottom(); y += 3)
        graphics.drawHorizontalLine(y, static_cast<float>(display.getX()),
                                    static_cast<float>(display.getRight()));

    graphics.setColour(juce::Colour(0xffaeb4b6));
    graphics.setFont(juce::FontOptions(10.0F, juce::Font::bold));
    graphics.drawText("X-PAD",
                      pads.front().getBounds().translated(0, -18).withWidth(pads.back().getRight() -
                                                                            pads.front().getX()),
                      juce::Justification::centred);
    graphics.drawText("FX FREQUENCY",
                      lowButton.getBounds().translated(0, -18).withWidth(highButton.getRight() -
                                                                         lowButton.getX()),
                      juce::Justification::centred);
    graphics.drawText("BEAT FX SELECT",
                      effectSelector.getBounds().translated(0, -17).withHeight(15),
                      juce::Justification::centred);
    graphics.drawText("FX ASSIGN", busSelector.getBounds().translated(0, -17).withHeight(15),
                      juce::Justification::centred);
    graphics.drawText("TIME", time.getBounds().translated(0, -16).withHeight(15),
                      juce::Justification::centred);
    graphics.drawText("LEVEL / DEPTH", depth.getBounds().translated(0, -16).withHeight(15),
                      juce::Justification::centred);
    graphics.drawText("BEAT",
                      beatLeft.getBounds().withX(beatLeft.getRight()).withRight(beatRight.getX()),
                      juce::Justification::centred);

    drawPanel(crossfaderBounds, false);
    graphics.setColour(juce::Colour(0xff9ca3a5));
    graphics.drawText("CROSSFADER", crossfaderBounds.withHeight(18), juce::Justification::centred);
}

void MainComponent::layoutChannel(ChannelControls& channel, juce::Rectangle<int> area) {
    area.reduce(8, 5);
    area.removeFromRight(11);
    channel.title.setBounds(area.removeFromTop(24));
    area.removeFromTop(13);
    channel.trim.setBounds(area.removeFromTop(66));
    for (auto* knob : {&channel.high, &channel.mid, &channel.low}) {
        area.removeFromTop(13);
        knob->setBounds(area.removeFromTop(58));
    }
    channel.eqMode.setBounds(area.removeFromTop(27).reduced(3, 2));
    auto buttons = area.removeFromTop(34);
    channel.cue.setBounds(buttons.removeFromLeft(buttons.getWidth() / 2).reduced(2));
    channel.mute.setBounds(buttons.reduced(2));
    area.removeFromTop(13);
    channel.assignment.setBounds(area.removeFromBottom(28).reduced(3, 1));
    channel.fader.setBounds(area.reduced(7, 2));
}

void MainComponent::resized() {
    auto bounds = getLocalBounds().reduced(8);
    auto header = bounds.removeFromTop(42);
    productLabel.setBounds(header.removeFromLeft(205));
    settingsButton.setBounds(header.removeFromRight(92).reduced(2, 4));
    midiButton.setBounds(header.removeFromRight(105).reduced(2, 4));
    midiEditButton.setBounds(header.removeFromRight(92).reduced(2, 4));
    scaleSelector.setBounds(header.removeFromRight(76).reduced(2, 4));
    statusLabel.setBounds(header);

    const auto fxWidth = std::clamp(bounds.getWidth() * 27 / 100, 340, 430);
    const auto monitorWidth = std::clamp(bounds.getWidth() * 12 / 100, 145, 205);
    auto fxArea = bounds.removeFromRight(fxWidth).reduced(3, 2);
    auto monitorArea = bounds.removeFromRight(monitorWidth).reduced(3, 2);
    auto footer = bounds.removeFromBottom(58);
    auto mixerArea = bounds.reduced(0, 2);

    fxBounds = fxArea;
    monitorBounds = monitorArea;
    crossfaderBounds = footer.reduced(3, 2);
    crossfader.setBounds(crossfaderBounds.reduced(22, 14).withTrimmedTop(6));

    const auto stripWidth = mixerArea.getWidth() / channelCount;
    for (int index = 0; index < channelCount; ++index) {
        auto area =
            mixerArea.removeFromLeft(index == channelCount - 1 ? mixerArea.getWidth() : stripWidth)
                .reduced(3, 0);
        channelBounds[static_cast<size_t>(index)] = area;
        layoutChannel(channelControls[static_cast<size_t>(index)], area);
    }

    auto monitor = monitorArea.reduced(8, 8);
    monitor.removeFromTop(24);
    std::array<juce::Slider*, 4> monitorKnobs{&master, &booth, &headphones, &cueMix};
    for (size_t index = 0; index < monitorKnobs.size(); ++index) {
        auto slot = monitor.removeFromTop(monitor.getHeight() /
                                          static_cast<int>(monitorKnobs.size() - index));
        slot.removeFromTop(16);
        monitorKnobs[index]->setBounds(slot.reduced(8, 2));
    }

    auto fx = fxArea.reduced(11, 7);
    fx.removeFromTop(24);
    const auto panelHeight = fx.getHeight();
    const auto displayHeight = std::clamp(panelHeight * 17 / 100, 125, 165);
    const auto padHeight = std::clamp(panelHeight * 6 / 100, 42, 54);
    const auto effectHeight = std::clamp(panelHeight * 13 / 100, 86, 112);
    const auto busHeight = std::clamp(panelHeight * 10 / 100, 68, 86);

    display.setBounds(fx.removeFromTop(displayHeight).reduced(8, 6));
    fx.removeFromTop(17);
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

    auto arrows = fx.removeFromTop(34);
    beatLeft.setBounds(arrows.removeFromLeft(70).reduced(2, 3));
    beatRight.setBounds(arrows.removeFromRight(70).reduced(2, 3));

    auto tempoArea = fx.removeFromTop(58);
    autoButton.setBounds(tempoArea.removeFromLeft(tempoArea.getWidth() / 3).reduced(5, 12));
    quantizeButton.setBounds(tempoArea.removeFromRight(tempoArea.getWidth() / 2).reduced(5, 12));
    tapButton.setBounds(tempoArea.reduced(3));

    fx.removeFromTop(17);
    auto bandsArea = fx.removeFromTop(35);
    lowButton.setBounds(bandsArea.removeFromLeft(bandsArea.getWidth() / 3).reduced(3, 2));
    midButton.setBounds(bandsArea.removeFromLeft(bandsArea.getWidth() / 2).reduced(3, 2));
    highButton.setBounds(bandsArea.reduced(3, 2));

    fx.removeFromTop(17);
    effectSelector.setBounds(fx.removeFromTop(effectHeight));
    fx.removeFromTop(17);
    busSelector.setBounds(fx.removeFromTop(busHeight));

    fx.removeFromTop(16);
    const auto timeHeight = std::max(48, fx.getHeight() * 40 / 100);
    time.setBounds(fx.removeFromTop(std::min(timeHeight, fx.getHeight())).reduced(14, 2));
    if (fx.getHeight() > 0) {
        fx.removeFromTop(std::min(16, fx.getHeight()));
        depth.setBounds(fx.reduced(14, 2));
    }
}
} // namespace qb
