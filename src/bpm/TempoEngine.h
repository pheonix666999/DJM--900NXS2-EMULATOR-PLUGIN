#pragma once

#include "model/Types.h"
#include <array>
#include <atomic>
#include <chrono>
#include <optional>
#include <span>

namespace qb {
class TempoEngine {
  public:
    static std::optional<double> parseManualBpm(std::string_view text);
    void setManualBpm(double bpm) noexcept;
    void setSource(TempoSource source) noexcept { activeSource.store(source); }
    void tap(double timestampSeconds) noexcept;
    void resetTap() noexcept;
    void analyseEnvelope(std::span<const float> envelope, double envelopeRate) noexcept;

    [[nodiscard]] double bpm() const noexcept { return activeBpm.load(); }
    [[nodiscard]] double confidence() const noexcept { return autoConfidence.load(); }
    [[nodiscard]] TempoSource source() const noexcept { return activeSource.load(); }

  private:
    void publishForSource(TempoSource source, double value) noexcept;
    std::atomic<double> activeBpm{120.0};
    std::atomic<double> autoConfidence{};
    std::atomic<TempoSource> activeSource{TempoSource::automatic};
    double lastAutoEstimate{};
    std::array<double, 8> taps{};
    int tapCount{};
    int tapWrite{};
};
} // namespace qb
