#pragma once

#include "model/Types.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace qb {
class BeatClock {
  public:
    void prepare(double newSampleRate) noexcept;
    void setTempo(double newBpm) noexcept;
    void setDivision(int newIndex) noexcept;
    void reset(double phase = 0.0) noexcept;
    void advance(int samples) noexcept;

    [[nodiscard]] double phase() const noexcept { return currentPhase; }
    [[nodiscard]] double bpm() const noexcept { return currentBpm; }
    [[nodiscard]] int divisionIndex() const noexcept { return division; }
    [[nodiscard]] std::int64_t samplesUntilBoundary() const noexcept;
    [[nodiscard]] bool crossesBoundary(int samples) const noexcept;

    static int previousDivision(int index) noexcept { return std::max(0, index - 1); }
    static int nextDivision(int index) noexcept {
        return std::min(static_cast<int>(beatValues.size()) - 1, index + 1);
    }

  private:
    double sampleRate{44100.0};
    double currentBpm{120.0};
    double currentPhase{};
    int division{5};
};
} // namespace qb
