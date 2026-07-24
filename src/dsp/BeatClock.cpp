#include "dsp/BeatClock.h"

namespace qb {
void BeatClock::prepare(const double newSampleRate) noexcept {
    sampleRate = std::isfinite(newSampleRate) && newSampleRate > 0.0 ? newSampleRate : 44100.0;
    reset();
}

void BeatClock::setTempo(const double newBpm) noexcept {
    if (std::isfinite(newBpm))
        currentBpm = std::clamp(newBpm, 60.0, 200.0);
}

void BeatClock::setDivision(const int newIndex) noexcept {
    division = std::clamp(newIndex, 0, static_cast<int>(beatValues.size()) - 1);
}

void BeatClock::reset(const double phaseValue) noexcept {
    currentPhase = std::isfinite(phaseValue) ? phaseValue - std::floor(phaseValue) : 0.0;
}

void BeatClock::advance(const int samples) noexcept {
    if (samples <= 0)
        return;
    const auto beats = static_cast<double>(samples) * currentBpm / (60.0 * sampleRate);
    currentPhase = std::fmod(currentPhase + beats / beatValues[static_cast<size_t>(division)], 1.0);
}

std::int64_t BeatClock::samplesUntilBoundary() const noexcept {
    const auto beatsRemaining = (1.0 - currentPhase) * beatValues[static_cast<size_t>(division)];
    return static_cast<std::int64_t>(std::ceil(beatsRemaining * 60.0 * sampleRate / currentBpm));
}

bool BeatClock::crossesBoundary(const int samples) const noexcept {
    return samples > 0 && static_cast<std::int64_t>(samples) >= samplesUntilBoundary();
}
} // namespace qb
