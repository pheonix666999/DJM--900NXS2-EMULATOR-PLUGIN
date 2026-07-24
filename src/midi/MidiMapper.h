#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_data_structures/juce_data_structures.h>
#include <optional>
#include <string>
#include <vector>

namespace qb {
enum class MidiMode { absolute, relativeTwosComplement, relativeBinaryOffset, button };

struct MidiMapping {
    std::string parameterId;
    int channel{1};
    int number{};
    bool note{};
    bool inverted{};
    float minimum{};
    float maximum{1.0F};
    MidiMode mode{MidiMode::absolute};
    float pickupTolerance{0.03F};
    bool pickedUp{};
};

class MidiMapper {
  public:
    void setMappings(std::vector<MidiMapping> newMappings);
    [[nodiscard]] const std::vector<MidiMapping>& mappings() const noexcept { return entries; }
    std::optional<std::pair<std::string, float>> process(const juce::MidiMessage& message,
                                                         float currentValue);
    juce::var serialise() const;
    bool deserialise(const juce::var& value);

  private:
    std::vector<MidiMapping> entries;
};
} // namespace qb
