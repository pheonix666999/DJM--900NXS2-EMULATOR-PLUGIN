#include "audio/MixerEngine.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace qb {
namespace {
constexpr float pi = std::numbers::pi_v<float>;
}

MixerEngine::MixerEngine() {
    for (auto& channel : peaks)
        for (auto& peak : channel)
            peak.store(0.0F);
    for (auto& peak : masterPeaks)
        peak.store(0.0F);
}

void MixerEngine::prepare(const double sampleRate, const int maximumBlockSize) {
    rate = std::isfinite(sampleRate) && sampleRate > 0.0 ? sampleRate : 44100.0;
    maxBlock = std::clamp(maximumBlockSize, 32, 8192);
    for (auto& buffer : channelBuffers)
        buffer.setSize(2, maxBlock, false, true, false);
    for (auto* buffer : {&busA, &busB, &thruBus, &cueBus, &masterBus})
        buffer->setSize(2, maxBlock, false, true, false);
    callbackInput.setSize(channelCount * 2, maxBlock, false, true, false);
    eqStates = {};
    effectRack.prepare(rate, maxBlock);
}

float MixerEngine::safe(const float value) noexcept {
    return std::isfinite(value) ? std::clamp(value, -8.0F, 8.0F) : 0.0F;
}

float MixerEngine::decibels(const float normalised, const float range) noexcept {
    return std::pow(10.0F, normalised * range / 20.0F);
}

void MixerEngine::processChannel(const int index, const juce::AudioBuffer<float>& input,
                                 juce::AudioBuffer<float>& output) noexcept {
    const auto samples = output.getNumSamples();
    const auto& parameters = channels[static_cast<size_t>(index)];
    const auto trimGain = decibels(parameters.trim.load(), 12.0F);
    const auto lowControl = parameters.low.load();
    const auto midControl = parameters.mid.load();
    const auto highControl = parameters.high.load();
    const auto isolator = parameters.eqMode.load() == EqMode::isolator;
    const auto lowGain =
        isolator && lowControl <= -0.99F ? 0.0F : decibels(lowControl, isolator ? 48.0F : 12.0F);
    const auto midGain =
        isolator && midControl <= -0.99F ? 0.0F : decibels(midControl, isolator ? 48.0F : 12.0F);
    const auto highGain =
        isolator && highControl <= -0.99F ? 0.0F : decibels(highControl, isolator ? 48.0F : 12.0F);
    const auto finalGain = parameters.mute.load() ? 0.0F : parameters.fader.load() * trimGain;
    const auto lowCoefficient =
        1.0F - std::exp(-2.0F * pi * lowMidCrossoverHz / static_cast<float>(rate));
    const auto highCoefficient =
        1.0F - std::exp(-2.0F * pi * midHighCrossoverHz / static_cast<float>(rate));
    for (int side = 0; side < 2; ++side) {
        const auto sourceChannel = index * 2 + side;
        const auto* source =
            sourceChannel < input.getNumChannels() ? input.getReadPointer(sourceChannel) : nullptr;
        auto* destination = output.getWritePointer(side);
        auto& state = eqStates[static_cast<size_t>(index)][static_cast<size_t>(side)];
        float peak{};
        for (int sample = 0; sample < samples; ++sample) {
            const auto value = source != nullptr ? safe(source[sample]) : 0.0F;
            state.low += lowCoefficient * (value - state.low);
            state.high += highCoefficient * (value - state.high);
            const auto result = (state.low * lowGain + (state.high - state.low) * midGain +
                                 (value - state.high) * highGain) *
                                finalGain;
            destination[sample] = safe(result);
            peak = std::max(peak, std::abs(destination[sample]));
        }
        peaks[static_cast<size_t>(index)][static_cast<size_t>(side)].store(peak);
    }
}

std::pair<float, float> MixerEngine::crossfaderGains() const noexcept {
    const auto position = std::clamp((crossfader.load() + 1.0F) * 0.5F, 0.0F, 1.0F);
    switch (crossfaderCurve.load()) {
    case CrossfaderCurve::smooth:
        return {1.0F - position, position};
    case CrossfaderCurve::constantPower:
        return {std::cos(position * pi * 0.5F), std::sin(position * pi * 0.5F)};
    case CrossfaderCurve::cut:
        return {position < 0.9F ? 1.0F : 0.0F, position > 0.1F ? 1.0F : 0.0F};
    }
    return {1.0F, 1.0F};
}

void MixerEngine::applyEffect(juce::AudioBuffer<float>& bus) noexcept { effectRack.process(bus); }

void MixerEngine::process(const juce::AudioBuffer<float>& inputs,
                          juce::AudioBuffer<float>& outputs) noexcept {
    const auto samples = std::min({inputs.getNumSamples(), outputs.getNumSamples(), maxBlock});
    if (samples <= 0)
        return;
    outputs.clear();
    for (auto* bus : {&busA, &busB, &thruBus, &cueBus, &masterBus}) {
        bus->setSize(2, samples, true, false, true);
        bus->clear();
    }
    for (int index = 0; index < channelCount; ++index) {
        auto& channel = channelBuffers[static_cast<size_t>(index)];
        channel.setSize(2, samples, true, false, true);
        channel.clear();
        processChannel(index, inputs, channel);
        const auto selectedBus = effectBus.load();
        if (selectedBus == static_cast<EffectBus>(static_cast<int>(EffectBus::ch1) + index))
            applyEffect(channel);
        auto* destination = &thruBus;
        const auto assignment = channels[static_cast<size_t>(index)].assignment.load();
        if (assignment == CrossfaderAssignment::a)
            destination = &busA;
        else if (assignment == CrossfaderAssignment::b)
            destination = &busB;
        for (int side = 0; side < 2; ++side)
            destination->addFrom(side, 0, channel, side, 0, samples);
        if (channels[static_cast<size_t>(index)].cue.load())
            for (int side = 0; side < 2; ++side)
                cueBus.addFrom(side, 0, channel, side, 0, samples);
    }
    if (effectBus.load() == EffectBus::crossfaderA)
        applyEffect(busA);
    if (effectBus.load() == EffectBus::crossfaderB)
        applyEffect(busB);
    const auto [gainA, gainB] = crossfaderGains();
    for (int side = 0; side < 2; ++side) {
        masterBus.copyFrom(side, 0, thruBus, side, 0, samples);
        masterBus.addFrom(side, 0, busA, side, 0, samples, gainA);
        masterBus.addFrom(side, 0, busB, side, 0, samples, gainB);
    }
    if (effectBus.load() == EffectBus::master)
        applyEffect(masterBus);
    const auto masterGain = std::clamp(masterLevel.load(), 0.0F, 1.5F);
    const auto boothGain = std::clamp(boothLevel.load(), 0.0F, 1.5F);
    const auto phonesGain = std::clamp(headphonesLevel.load(), 0.0F, 1.5F);
    const auto mix = std::clamp(cueMix.load(), 0.0F, 1.0F);
    for (int side = 0; side < 2; ++side) {
        if (side < outputs.getNumChannels()) {
            outputs.copyFrom(side, 0, masterBus, side, 0, samples);
            outputs.applyGain(side, 0, samples, masterGain);
        }
        if (side + 2 < outputs.getNumChannels()) {
            outputs.copyFrom(side + 2, 0, masterBus, side, 0, samples);
            outputs.applyGain(side + 2, 0, samples, boothGain);
        }
        if (side + 4 < outputs.getNumChannels()) {
            outputs.copyFrom(side + 4, 0, cueBus, side, 0, samples);
            outputs.applyGain(side + 4, 0, samples, phonesGain * (1.0F - mix));
            outputs.addFrom(side + 4, 0, masterBus, side, 0, samples, phonesGain * mix);
        }
        float peak{};
        const auto* data = masterBus.getReadPointer(side);
        for (int sample = 0; sample < samples; ++sample)
            peak = std::max(peak, std::abs(data[sample] * masterGain));
        masterPeaks[static_cast<size_t>(side)].store(peak);
    }
}

void MixerEngine::audioDeviceAboutToStart(juce::AudioIODevice* device) {
    if (device != nullptr)
        prepare(device->getCurrentSampleRate(), device->getCurrentBufferSizeSamples());
}

void MixerEngine::audioDeviceStopped() {
    for (auto& peak : masterPeaks)
        peak.store(0.0F);
}

void MixerEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                   const int numInputChannels,
                                                   float* const* outputChannelData,
                                                   const int numOutputChannels,
                                                   const int numSamples,
                                                   const juce::AudioIODeviceCallbackContext&) {
    callbackInput.clear();
    const auto samples = std::min(numSamples, callbackInput.getNumSamples());
    for (int channel = 0; channel < std::min(numInputChannels, callbackInput.getNumChannels());
         ++channel)
        if (inputChannelData[channel] != nullptr)
            callbackInput.copyFrom(channel, 0, inputChannelData[channel], samples);
    juce::AudioBuffer<float> output(outputChannelData, numOutputChannels, numSamples);
    process(callbackInput, output);
}

float MixerEngine::channelPeak(const int index, const int side) const noexcept {
    if (index < 0 || index >= channelCount || side < 0 || side > 1)
        return 0.0F;
    return peaks[static_cast<size_t>(index)][static_cast<size_t>(side)].load();
}

float MixerEngine::masterPeak(const int side) const noexcept {
    return side >= 0 && side < 2 ? masterPeaks[static_cast<size_t>(side)].load() : 0.0F;
}
} // namespace qb
