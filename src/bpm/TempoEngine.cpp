#include "bpm/TempoEngine.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <numeric>
#include <string>

namespace qb {
TempoEngine::TempoEngine() : juce::Thread("QuadBeat tempo analysis") {
    startThread(juce::Thread::Priority::low);
}

TempoEngine::~TempoEngine() {
    signalThreadShouldExit();
    stopThread(1000);
}

std::optional<double> TempoEngine::parseManualBpm(const std::string_view text) {
    double value{};
    const auto* begin = text.data();
    const auto* end = begin + text.size();
    const auto result = std::from_chars(begin, end, value);
    if (result.ec != std::errc{} || result.ptr != end || !std::isfinite(value) || value < 60.0 ||
        value > 200.0)
        return std::nullopt;
    return value;
}

void TempoEngine::publishForSource(const TempoSource selected, const double value) noexcept {
    if (activeSource.load() == selected && std::isfinite(value))
        activeBpm.store(std::clamp(value, 60.0, 200.0));
}

void TempoEngine::setManualBpm(const double value) noexcept {
    publishForSource(TempoSource::manual, value);
}

void TempoEngine::resetTap() noexcept {
    taps.fill(0.0);
    tapCount = 0;
    tapWrite = 0;
}

void TempoEngine::tap(const double now) noexcept {
    if (!std::isfinite(now))
        return;
    if (tapCount > 0) {
        const auto previous = taps[static_cast<size_t>((tapWrite + 7) % 8)];
        const auto interval = now - previous;
        if (interval > 2.0)
            resetTap();
        else if (interval < 0.25 || interval > 1.0)
            return;
    }
    taps[static_cast<size_t>(tapWrite)] = now;
    tapWrite = (tapWrite + 1) % 8;
    tapCount = std::min(tapCount + 1, 8);
    if (tapCount < 2)
        return;
    double total{};
    for (int i = 1; i < tapCount; ++i) {
        const auto newer = taps[static_cast<size_t>((tapWrite - i + 8) % 8)];
        const auto older = taps[static_cast<size_t>((tapWrite - i - 1 + 16) % 8)];
        total += newer - older;
    }
    publishForSource(TempoSource::tap, 60.0 / (total / static_cast<double>(tapCount - 1)));
}

void TempoEngine::analyseEnvelope(const std::span<const float> envelope,
                                  const double envelopeRate) noexcept {
    if (envelope.size() < 64 || !std::isfinite(envelopeRate) || envelopeRate <= 0.0)
        return;
    double best{};
    int bestLag{};
    const auto minLag = std::max(1, static_cast<int>(envelopeRate * 60.0 / 200.0));
    const auto maxLag = std::min(static_cast<int>(envelope.size() / 2),
                                 static_cast<int>(envelopeRate * 60.0 / 60.0));
    double energy{};
    for (const auto sample : envelope)
        energy += static_cast<double>(sample) * sample;
    if (energy < 1.0e-12)
        return;
    for (int lag = minLag; lag <= maxLag; ++lag) {
        double correlation{};
        for (size_t i = static_cast<size_t>(lag); i < envelope.size(); ++i)
            correlation += envelope[i] * envelope[i - static_cast<size_t>(lag)];
        if (correlation > best) {
            best = correlation;
            bestLag = lag;
        }
    }
    if (bestLag == 0)
        return;
    const auto estimate = 60.0 * envelopeRate / static_cast<double>(bestLag);
    autoConfidence.store(std::clamp(best / energy, 0.0, 1.0));
    if (autoConfidence.load() > 0.08) {
        const auto previous = lastAutoEstimate > 0.0 ? lastAutoEstimate : activeBpm.load();
        const auto corrected = std::abs(estimate * 2.0 - previous) < std::abs(estimate - previous)
                                   ? estimate * 2.0
                                   : estimate;
        lastAutoEstimate = lastAutoEstimate > 0.0 && std::abs(corrected - lastAutoEstimate) < 20.0
                               ? 0.8 * lastAutoEstimate + 0.2 * corrected
                               : corrected;
        publishForSource(TempoSource::automatic, lastAutoEstimate);
    }
}

void TempoEngine::prepareAnalysis(const double sampleRate) noexcept {
    const auto validRate = std::isfinite(sampleRate) && sampleRate > 0.0 ? sampleRate : 44100.0;
    samplesPerEnvelope = std::max(1, static_cast<int>(std::round(validRate / 100.0)));
    envelopeSampleCount = 0;
    envelopeAccumulator = 0.0F;
    previousEnvelopeEnergy = 0.0F;
    queueRead.store(0, std::memory_order_relaxed);
    queueWrite.store(0, std::memory_order_relaxed);
    resetGeneration.fetch_add(1, std::memory_order_release);
}

void TempoEngine::pushStereo(const float* left, const float* right, const int samples) noexcept {
    if (left == nullptr || right == nullptr || samples <= 0)
        return;
    for (int sample = 0; sample < samples; ++sample) {
        const auto energy = 0.5F * (std::abs(left[sample]) + std::abs(right[sample]));
        envelopeAccumulator += std::isfinite(energy) ? energy : 0.0F;
        ++envelopeSampleCount;
        if (envelopeSampleCount < samplesPerEnvelope)
            continue;
        const auto meanEnergy = envelopeAccumulator / static_cast<float>(envelopeSampleCount);
        const auto onset = std::max(0.0F, meanEnergy - previousEnvelopeEnergy);
        previousEnvelopeEnergy = meanEnergy;
        envelopeAccumulator = 0.0F;
        envelopeSampleCount = 0;
        const auto write = queueWrite.load(std::memory_order_relaxed);
        const auto next = (write + 1U) % queueCapacity;
        if (next == queueRead.load(std::memory_order_acquire))
            continue;
        onsetQueue[write] = onset;
        queueWrite.store(next, std::memory_order_release);
    }
}

bool TempoEngine::processPendingAnalysis() noexcept {
    const auto generation = resetGeneration.load(std::memory_order_acquire);
    if (generation != handledGeneration) {
        handledGeneration = generation;
        analysisFill = 0;
        lastAutoEstimate = 0.0;
    }
    bool analysed = false;
    auto read = queueRead.load(std::memory_order_relaxed);
    const auto write = queueWrite.load(std::memory_order_acquire);
    while (read != write) {
        analysisWindow[analysisFill++] = onsetQueue[read];
        read = (read + 1U) % queueCapacity;
        if (analysisFill == analysisWindow.size()) {
            analyseEnvelope(analysisWindow, 100.0);
            constexpr size_t retainedSamples = 400;
            std::copy(analysisWindow.end() - static_cast<std::ptrdiff_t>(retainedSamples),
                      analysisWindow.end(), analysisWindow.begin());
            analysisFill = retainedSamples;
            analysed = true;
        }
    }
    queueRead.store(read, std::memory_order_release);
    return analysed;
}

void TempoEngine::run() {
    while (!threadShouldExit()) {
        processPendingAnalysis();
        wait(20);
    }
}
} // namespace qb
