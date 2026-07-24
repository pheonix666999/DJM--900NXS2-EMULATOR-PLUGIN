#include "bpm/TempoEngine.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <numeric>
#include <string>

namespace qb {
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
} // namespace qb
