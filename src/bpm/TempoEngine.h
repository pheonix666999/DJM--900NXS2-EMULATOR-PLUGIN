#pragma once

#include "model/Types.h"
#include <array>
#include <atomic>
#include <chrono>
#include <juce_core/juce_core.h>
#include <optional>
#include <span>

namespace qb {
class TempoEngine : private juce::Thread {
  public:
    TempoEngine();
    ~TempoEngine() override;

    static std::optional<double> parseManualBpm(std::string_view text);
    void setManualBpm(double bpm) noexcept;
    void setSource(TempoSource source) noexcept { activeSource.store(source); }
    void tap(double timestampSeconds) noexcept;
    void resetTap() noexcept;
    void analyseEnvelope(std::span<const float> envelope, double envelopeRate) noexcept;
    void prepareAnalysis(double sampleRate) noexcept;
    void pushStereo(const float* left, const float* right, int samples) noexcept;

    [[nodiscard]] double bpm() const noexcept { return activeBpm.load(); }
    [[nodiscard]] double confidence() const noexcept { return autoConfidence.load(); }
    [[nodiscard]] TempoSource source() const noexcept { return activeSource.load(); }

  private:
    void run() override;
    bool processPendingAnalysis() noexcept;
    void publishForSource(TempoSource source, double value) noexcept;
    static constexpr unsigned queueCapacity = 4096;
    static constexpr size_t analysisWindowSize = 1200;
    std::atomic<double> activeBpm{120.0};
    std::atomic<double> autoConfidence{};
    std::atomic<TempoSource> activeSource{TempoSource::automatic};
    std::array<float, queueCapacity> onsetQueue{};
    std::atomic<unsigned> queueWrite{};
    std::atomic<unsigned> queueRead{};
    std::atomic<unsigned> resetGeneration{};
    std::array<float, analysisWindowSize> analysisWindow{};
    size_t analysisFill{};
    unsigned handledGeneration{};
    int samplesPerEnvelope{441};
    int envelopeSampleCount{};
    float envelopeAccumulator{};
    float previousEnvelopeEnergy{};
    double lastAutoEstimate{};
    std::array<double, 8> taps{};
    int tapCount{};
    int tapWrite{};
};
} // namespace qb
