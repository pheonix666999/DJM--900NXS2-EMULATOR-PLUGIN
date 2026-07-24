#include "midi/MidiMapper.h"
#include <algorithm>
#include <cmath>

namespace qb {
void MidiMapper::setMappings(std::vector<MidiMapping> newMappings) {
    entries = std::move(newMappings);
}

std::optional<std::pair<std::string, float>> MidiMapper::process(const juce::MidiMessage& message,
                                                                 const float currentValue) {
    const auto isNote = message.isNoteOnOrOff();
    const auto isCc = message.isController();
    if (!isNote && !isCc)
        return std::nullopt;
    const auto channel = message.getChannel();
    const auto number = isNote ? message.getNoteNumber() : message.getControllerNumber();
    const auto raw = isNote ? (message.isNoteOn() ? 127 : 0) : message.getControllerValue();
    for (auto& mapping : entries) {
        if (mapping.channel != channel || mapping.number != number || mapping.note != isNote)
            continue;
        float normalised = static_cast<float>(raw) / 127.0F;
        if (mapping.mode == MidiMode::button)
            normalised = raw > 0 ? 1.0F : 0.0F;
        else if (mapping.mode != MidiMode::absolute) {
            const auto delta = mapping.mode == MidiMode::relativeTwosComplement
                                   ? (raw < 64 ? raw : raw - 128)
                                   : raw - 64;
            normalised = std::clamp(currentValue + static_cast<float>(delta) / 127.0F, 0.0F, 1.0F);
        } else if (!mapping.pickedUp) {
            mapping.pickedUp = std::abs(normalised - currentValue) <= mapping.pickupTolerance;
            if (!mapping.pickedUp)
                return std::nullopt;
        }
        if (mapping.inverted)
            normalised = 1.0F - normalised;
        const auto value = mapping.minimum + normalised * (mapping.maximum - mapping.minimum);
        return std::pair{mapping.parameterId, value};
    }
    return std::nullopt;
}

juce::var MidiMapper::serialise() const {
    juce::Array<juce::var> array;
    for (const auto& mapping : entries) {
        auto* object = new juce::DynamicObject();
        object->setProperty("parameter", juce::String(mapping.parameterId));
        object->setProperty("channel", mapping.channel);
        object->setProperty("number", mapping.number);
        object->setProperty("note", mapping.note);
        object->setProperty("inverted", mapping.inverted);
        object->setProperty("minimum", mapping.minimum);
        object->setProperty("maximum", mapping.maximum);
        object->setProperty("mode", static_cast<int>(mapping.mode));
        array.add(juce::var(object));
    }
    return array;
}

bool MidiMapper::deserialise(const juce::var& value) {
    if (!value.isArray())
        return false;
    std::vector<MidiMapping> loaded;
    for (const auto& item : *value.getArray()) {
        const auto* object = item.getDynamicObject();
        if (object == nullptr || !object->hasProperty("parameter"))
            return false;
        MidiMapping mapping;
        mapping.parameterId = object->getProperty("parameter").toString().toStdString();
        mapping.channel = std::clamp(static_cast<int>(object->getProperty("channel")), 1, 16);
        mapping.number = std::clamp(static_cast<int>(object->getProperty("number")), 0, 127);
        mapping.note = object->getProperty("note");
        mapping.inverted = object->getProperty("inverted");
        mapping.minimum = object->getProperty("minimum");
        mapping.maximum = object->getProperty("maximum");
        mapping.mode =
            static_cast<MidiMode>(std::clamp(static_cast<int>(object->getProperty("mode")), 0, 3));
        loaded.push_back(std::move(mapping));
    }
    entries = std::move(loaded);
    return true;
}
} // namespace qb
