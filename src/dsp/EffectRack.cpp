#include "dsp/EffectRack.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace qb {
namespace {
constexpr float pi = std::numbers::pi_v<float>;
constexpr float safetyLimit = 8.0F;
} // namespace

void EffectRack::Biquad::lowPass(const double sr, const float frequency, const float q) noexcept {
    const auto omega = 2.0F * pi * std::clamp(frequency, 20.0F, static_cast<float>(sr * 0.45)) /
                       static_cast<float>(sr);
    const auto alpha = std::sin(omega) / (2.0F * q);
    const auto cosine = std::cos(omega);
    const auto scale = 1.0F / (1.0F + alpha);
    b0 = (1.0F - cosine) * 0.5F * scale;
    b1 = (1.0F - cosine) * scale;
    b2 = b0;
    a1 = -2.0F * cosine * scale;
    a2 = (1.0F - alpha) * scale;
}

void EffectRack::Biquad::allPass(const double sr, const float frequency, const float q) noexcept {
    const auto omega = 2.0F * pi * std::clamp(frequency, 20.0F, static_cast<float>(sr * 0.45)) /
                       static_cast<float>(sr);
    const auto alpha = std::sin(omega) / (2.0F * q);
    const auto cosine = std::cos(omega);
    const auto scale = 1.0F / (1.0F + alpha);
    b0 = (1.0F - alpha) * scale;
    b1 = -2.0F * cosine * scale;
    b2 = 1.0F;
    a1 = b1;
    a2 = b0;
}

float EffectRack::Biquad::process(const float input) noexcept {
    const auto output = input * b0 + z1;
    z1 = input * b1 - output * a1 + z2;
    z2 = input * b2 - output * a2;
    return output;
}

void EffectRack::prepare(const double sampleRate, const int maximumBlockSize) {
    rate = std::isfinite(sampleRate) && sampleRate > 0.0 ? sampleRate : 44100.0;
    maximumBlock = std::clamp(maximumBlockSize, 1, 8192);
    const auto maximumDelay = static_cast<size_t>(std::ceil(rate * 8.0));
    for (auto& channel : delay)
        channel.assign(maximumDelay, 0.0F);
    reverb.setSampleRate(rate);
    reset();
}

void EffectRack::reset() noexcept {
    for (auto& channel : delay)
        std::fill(channel.begin(), channel.end(), 0.0F);
    for (auto& bank : phaser)
        for (auto& stage : bank)
            stage.reset();
    lowState.fill(0.0F);
    highState.fill(0.0F);
    reverb.reset();
    writePosition = 0;
    lfoPhase = 0.0F;
    smoothedTime = time.load();
    smoothedDepth = 0.0F;
    gate = 0.0F;
    brakeRead = 0.0F;
    brakeSpeed = 1.0F;
    loopCaptureStart = 0.0F;
    loopPhase = 0.0F;
    samplesToQuantizeBoundary = 0.0;
    activeEnabled = false;
    pendingEnabled = false;
    hasPendingEnable = false;
    wasEnabled = false;
}

void EffectRack::setParameters(const EffectParameters& parameters) noexcept {
    type.store(parameters.type);
    time.store(std::clamp(parameters.time, 0.0F, 1.0F));
    depth.store(std::clamp(parameters.depth, 0.0F, 1.0F));
    bpm.store(std::isfinite(parameters.bpm) ? std::clamp(parameters.bpm, 60.0, 200.0) : 120.0);
    division.store(std::clamp(parameters.division, 0, 7));
    enabled.store(parameters.enabled);
    quantized.store(parameters.quantize);
    bands.store((parameters.low ? 1U : 0U) | (parameters.mid ? 2U : 0U) |
                (parameters.high ? 4U : 0U));
}

float EffectRack::readDelay(const int channel, const float delaySamples) const noexcept {
    const auto& buffer = delay[static_cast<size_t>(channel)];
    if (buffer.empty())
        return 0.0F;
    auto position = static_cast<float>(writePosition) -
                    std::clamp(delaySamples, 1.0F, static_cast<float>(buffer.size() - 2));
    while (position < 0.0F)
        position += static_cast<float>(buffer.size());
    const auto first = static_cast<int>(position) % static_cast<int>(buffer.size());
    const auto second = (first + 1) % static_cast<int>(buffer.size());
    const auto fraction = position - std::floor(position);
    return buffer[static_cast<size_t>(first)] * (1.0F - fraction) +
           buffer[static_cast<size_t>(second)] * fraction;
}

float EffectRack::readAt(const int channel, float position) const noexcept {
    const auto& buffer = delay[static_cast<size_t>(channel)];
    if (buffer.empty())
        return 0.0F;
    const auto length = static_cast<float>(buffer.size());
    position = std::fmod(position, length);
    if (position < 0.0F)
        position += length;
    const auto first = static_cast<int>(position) % static_cast<int>(buffer.size());
    const auto second = (first + 1) % static_cast<int>(buffer.size());
    const auto fraction = position - std::floor(position);
    return buffer[static_cast<size_t>(first)] * (1.0F - fraction) +
           buffer[static_cast<size_t>(second)] * fraction;
}

void EffectRack::writeDelay(const float left, const float right) noexcept {
    if (delay[0].empty())
        return;
    delay[0][static_cast<size_t>(writePosition)] = finite(left);
    delay[1][static_cast<size_t>(writePosition)] = finite(right);
    writePosition = (writePosition + 1) % static_cast<int>(delay[0].size());
}

void EffectRack::splitBands(const float input, const int channel, float& low, float& mid,
                            float& high) noexcept {
    const auto lowCoefficient =
        1.0F - std::exp(-2.0F * pi * lowMidCrossoverHz / static_cast<float>(rate));
    const auto highCoefficient =
        1.0F - std::exp(-2.0F * pi * midHighCrossoverHz / static_cast<float>(rate));
    lowState[static_cast<size_t>(channel)] +=
        lowCoefficient * (input - lowState[static_cast<size_t>(channel)]);
    highState[static_cast<size_t>(channel)] +=
        highCoefficient * (input - highState[static_cast<size_t>(channel)]);
    low = lowState[static_cast<size_t>(channel)];
    high = input - highState[static_cast<size_t>(channel)];
    mid = highState[static_cast<size_t>(channel)] - low;
}

float EffectRack::finite(const float value) noexcept {
    return std::isfinite(value) ? std::clamp(value, -safetyLimit, safetyLimit) : 0.0F;
}

void EffectRack::processWet(const float left, const float right, float& wetLeft, float& wetRight,
                            const EffectParameters& p) noexcept {
    const auto beatSeconds = 60.0 / p.bpm * beatValues[static_cast<size_t>(p.division)];
    const auto beatSamples = static_cast<float>(beatSeconds * rate);
    const auto phaseIncrement = static_cast<float>(1.0 / std::max(1.0, beatSeconds * rate));
    const auto feedback = std::min(0.88F, 0.25F + p.depth * 0.6F);
    wetLeft = left;
    wetRight = right;
    switch (p.type) {
    case EffectType::delay: {
        wetLeft = readDelay(0, beatSamples);
        wetRight = readDelay(1, beatSamples);
        writeDelay(left, right);
        break;
    }
    case EffectType::echo: {
        wetLeft = readDelay(0, beatSamples);
        wetRight = readDelay(1, beatSamples);
        writeDelay(left + wetLeft * feedback, right + wetRight * feedback);
        break;
    }
    case EffectType::pingPong: {
        wetLeft = readDelay(1, beatSamples);
        wetRight = readDelay(0, beatSamples);
        writeDelay(left + wetLeft * feedback, right + wetRight * feedback);
        break;
    }
    case EffectType::spiral: {
        const auto modulation = std::sin(lfoPhase * 2.0F * pi) * beatSamples * 0.08F;
        wetLeft = readDelay(1, beatSamples * 0.5F + modulation);
        wetRight = -readDelay(0, beatSamples * 0.75F - modulation);
        writeDelay(left + wetLeft * feedback, right + wetRight * feedback);
        break;
    }
    case EffectType::reverb: {
        wetLeft = left;
        wetRight = right;
        reverb.processStereo(&wetLeft, &wetRight, 1);
        break;
    }
    case EffectType::trans: {
        const auto target = lfoPhase < (0.15F + p.time * 0.7F) ? 1.0F : 0.0F;
        gate += 0.02F * (target - gate);
        wetLeft = left * gate;
        wetRight = right * gate;
        writeDelay(left, right);
        break;
    }
    case EffectType::filter: {
        const auto cutoff = 80.0F * std::pow(200.0F, p.time);
        phaser[0][0].lowPass(rate, cutoff, 0.7F + p.depth * 5.0F);
        phaser[1][0].lowPass(rate, cutoff, 0.7F + p.depth * 5.0F);
        wetLeft = phaser[0][0].process(left);
        wetRight = phaser[1][0].process(right);
        writeDelay(left, right);
        break;
    }
    case EffectType::flanger: {
        const auto modulation = (0.002F + 0.004F * (0.5F + 0.5F * std::sin(lfoPhase * 2.0F * pi))) *
                                static_cast<float>(rate);
        wetLeft = left + readDelay(0, modulation) * 0.8F;
        wetRight = right + readDelay(1, modulation * 1.07F) * 0.8F;
        writeDelay(left + wetLeft * feedback * 0.4F, right + wetRight * feedback * 0.4F);
        break;
    }
    case EffectType::phaser: {
        const auto centre = 180.0F + 1800.0F * (0.5F + 0.5F * std::sin(lfoPhase * 2.0F * pi));
        wetLeft = left;
        wetRight = right;
        for (size_t stage = 0; stage < 4; ++stage) {
            const auto frequency = centre * (0.7F + 0.2F * static_cast<float>(stage));
            phaser[0][stage].allPass(rate, frequency, 0.6F);
            phaser[1][stage].allPass(rate, frequency * 1.03F, 0.6F);
            wetLeft = phaser[0][stage].process(wetLeft);
            wetRight = phaser[1][stage].process(wetRight);
        }
        writeDelay(left, right);
        break;
    }
    case EffectType::pitch: {
        const auto semitones = (p.time * 2.0F - 1.0F) * 12.0F;
        const auto ratio = std::pow(2.0F, semitones / 12.0F);
        const auto window = static_cast<float>(rate * 0.05);
        const auto positionA = std::fmod(lfoPhase * window, window);
        const auto positionB = std::fmod(positionA + window * 0.5F, window);
        const auto gainA = 0.5F - 0.5F * std::cos(2.0F * pi * positionA / window);
        const auto gainB = 1.0F - gainA;
        wetLeft = readDelay(0, 1.0F + positionA * ratio) * gainA +
                  readDelay(0, 1.0F + positionB * ratio) * gainB;
        wetRight = readDelay(1, 1.0F + positionA * ratio) * gainA +
                   readDelay(1, 1.0F + positionB * ratio) * gainB;
        writeDelay(left, right);
        break;
    }
    case EffectType::slipRoll:
    case EffectType::roll: {
        const auto loopSamples = std::max(32.0F, beatSamples);
        wetLeft = readAt(0, loopCaptureStart + loopPhase);
        wetRight = readAt(1, loopCaptureStart + loopPhase);
        loopPhase = std::fmod(loopPhase + 1.0F, loopSamples);
        writeDelay(left, right);
        if (p.type == EffectType::slipRoll) {
            wetLeft = 0.85F * wetLeft + 0.15F * left;
            wetRight = 0.85F * wetRight + 0.15F * right;
        }
        break;
    }
    case EffectType::vinylBrake: {
        if (!wasEnabled) {
            brakeRead = static_cast<float>(std::max(1, writePosition));
            brakeSpeed = 1.0F;
        }
        const auto duration = static_cast<float>(rate * (0.15 + p.time * 3.85));
        brakeSpeed = std::max(0.0F, brakeSpeed - 1.0F / duration);
        brakeRead -= brakeSpeed;
        if (brakeRead < 0.0F)
            brakeRead += static_cast<float>(delay[0].size());
        const auto distance = static_cast<float>(writePosition) - brakeRead;
        wetLeft = readDelay(0, distance < 1.0F ? distance + static_cast<float>(delay[0].size())
                                               : distance);
        wetRight = readDelay(1, distance < 1.0F ? distance + static_cast<float>(delay[1].size())
                                                : distance);
        writeDelay(left, right);
        break;
    }
    case EffectType::helix: {
        const auto loop = beatSamples * (0.5F + 0.5F * p.time);
        const auto rotating = std::fmod(lfoPhase * loop, loop);
        const auto firstLeft = readDelay(0, loop - rotating);
        const auto firstRight = readDelay(1, loop - rotating);
        const auto secondLeft = readDelay(0, loop * 0.5F + rotating * 0.25F);
        const auto secondRight = readDelay(1, loop * 0.5F + rotating * 0.25F);
        wetLeft = firstLeft * 0.65F - secondRight * 0.35F;
        wetRight = firstRight * 0.65F + secondLeft * 0.35F;
        writeDelay(left + wetRight * feedback * 0.45F, right - wetLeft * feedback * 0.45F);
        break;
    }
    case EffectType::pan: {
        const auto pan = std::sin(lfoPhase * 2.0F * pi);
        wetLeft = left * std::sqrt(0.5F * (1.0F - pan));
        wetRight = right * std::sqrt(0.5F * (1.0F + pan));
        writeDelay(left, right);
        break;
    }
    case EffectType::count:
        break;
    }
    lfoPhase = std::fmod(lfoPhase + phaseIncrement, 1.0F);
}

void EffectRack::process(juce::AudioBuffer<float>& stereo) noexcept {
    if (stereo.getNumChannels() < 2 || stereo.getNumSamples() <= 0 || delay[0].empty())
        return;
    EffectParameters p;
    p.type = type.load();
    p.time = time.load();
    p.depth = depth.load();
    p.bpm = bpm.load();
    p.division = division.load();
    p.enabled = enabled.load();
    p.quantize = quantized.load();
    const auto bandMask = bands.load();
    p.low = (bandMask & 1U) != 0U;
    p.mid = (bandMask & 2U) != 0U;
    p.high = (bandMask & 4U) != 0U;
    if (p.type == EffectType::reverb) {
        juce::Reverb::Parameters reverbParameters;
        reverbParameters.roomSize = 0.35F + p.time * 0.63F;
        reverbParameters.damping = 0.62F - p.time * 0.38F;
        reverbParameters.wetLevel = 0.33F;
        reverbParameters.dryLevel = 0.0F;
        reverbParameters.width = 0.92F;
        reverbParameters.freezeMode = 0.0F;
        reverb.setParameters(reverbParameters);
    }
    const auto requestedEnabled = p.enabled;
    const auto quantizePeriod = 60.0 / p.bpm * beatValues[static_cast<size_t>(p.division)] * rate;
    if (samplesToQuantizeBoundary <= 0.0)
        samplesToQuantizeBoundary = std::max(1.0, quantizePeriod);
    if (!p.quantize) {
        activeEnabled = requestedEnabled;
        hasPendingEnable = false;
    } else if (requestedEnabled != activeEnabled) {
        pendingEnabled = requestedEnabled;
        hasPendingEnable = true;
    }
    auto* left = stereo.getWritePointer(0);
    auto* right = stereo.getWritePointer(1);
    for (int sample = 0; sample < stereo.getNumSamples(); ++sample) {
        if (p.quantize) {
            samplesToQuantizeBoundary -= 1.0;
            if (samplesToQuantizeBoundary <= 0.0) {
                samplesToQuantizeBoundary += std::max(1.0, quantizePeriod);
                if (hasPendingEnable) {
                    activeEnabled = pendingEnabled;
                    hasPendingEnable = false;
                }
            }
        }
        if (activeEnabled && !wasEnabled &&
            (p.type == EffectType::roll || p.type == EffectType::slipRoll)) {
            const auto loopSamples = static_cast<float>(std::max(32.0, quantizePeriod));
            loopCaptureStart = static_cast<float>(writePosition) - loopSamples;
            loopPhase = 0.0F;
        }
        p.enabled = activeEnabled;
        const auto targetDepth = activeEnabled ? p.depth : 0.0F;
        smoothedTime += 0.002F * (p.time - smoothedTime);
        smoothedDepth += 0.002F * (targetDepth - smoothedDepth);
        p.time = smoothedTime;
        float lowL{}, midL{}, highL{}, lowR{}, midR{}, highR{};
        splitBands(finite(left[sample]), 0, lowL, midL, highL);
        splitBands(finite(right[sample]), 1, lowR, midR, highR);
        const auto selectedL =
            (p.low ? lowL : 0.0F) + (p.mid ? midL : 0.0F) + (p.high ? highL : 0.0F);
        const auto selectedR =
            (p.low ? lowR : 0.0F) + (p.mid ? midR : 0.0F) + (p.high ? highR : 0.0F);
        const auto bypassL =
            (p.low ? 0.0F : lowL) + (p.mid ? 0.0F : midL) + (p.high ? 0.0F : highL);
        const auto bypassR =
            (p.low ? 0.0F : lowR) + (p.mid ? 0.0F : midR) + (p.high ? 0.0F : highR);
        float wetL{}, wetR{};
        processWet(selectedL, selectedR, wetL, wetR, p);
        const auto dryGain = std::cos(smoothedDepth * pi * 0.5F);
        const auto wetGain = std::sin(smoothedDepth * pi * 0.5F);
        left[sample] = finite(bypassL + selectedL * dryGain + wetL * wetGain);
        right[sample] = finite(bypassR + selectedR * dryGain + wetR * wetGain);
        wasEnabled = activeEnabled;
    }
}
} // namespace qb
