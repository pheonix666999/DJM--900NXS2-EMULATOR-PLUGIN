#include "audio/MixerEngine.h"
#include "bpm/TempoEngine.h"
#include "dsp/BeatClock.h"
#include "dsp/EffectRack.h"
#include "midi/MidiMapper.h"
#include "state/StateStore.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <string>

namespace {
int failures{};
int checks{};

void expect(const bool condition, const std::string& name) {
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << name << '\n';
    }
}

float difference(const juce::AudioBuffer<float>& first, const juce::AudioBuffer<float>& second) {
    float total{};
    for (int channel = 0; channel < std::min(first.getNumChannels(), second.getNumChannels());
         ++channel)
        for (int sample = 0; sample < std::min(first.getNumSamples(), second.getNumSamples());
             ++sample)
            total += std::abs(first.getSample(channel, sample) - second.getSample(channel, sample));
    return total;
}

void testBeatAndTempo() {
    qb::BeatClock clock;
    clock.prepare(48000.0);
    clock.setTempo(120.0);
    clock.setDivision(5);
    expect(clock.samplesUntilBoundary() == 24000, "beat clock one beat");
    clock.advance(12000);
    expect(std::abs(clock.phase() - 0.5) < 1.0e-6, "beat clock phase");
    expect(qb::BeatClock::previousDivision(0) == 0, "division lower bound");
    expect(qb::BeatClock::nextDivision(7) == 7, "division upper bound");
    expect(qb::TempoEngine::parseManualBpm("128.5").value_or(0.0) == 128.5, "manual BPM decimal");
    expect(!qb::TempoEngine::parseManualBpm("220"), "manual BPM rejects range");
    qb::TempoEngine tempo;
    tempo.setSource(qb::TempoSource::tap);
    tempo.tap(1.0);
    tempo.tap(1.5);
    tempo.tap(2.0);
    expect(std::abs(tempo.bpm() - 120.0) < 0.01, "tap BPM");
    for (const auto target : {80.0, 100.0, 120.0, 128.0, 150.0}) {
        constexpr double envelopeRate = 100.0;
        std::vector<float> envelope(1200, 0.0F);
        const auto interval = static_cast<int>(std::round(envelopeRate * 60.0 / target));
        for (int sample = 0; sample < static_cast<int>(envelope.size()); sample += interval)
            envelope[static_cast<size_t>(sample)] = 1.0F;
        qb::TempoEngine detector;
        detector.analyseEnvelope(envelope, envelopeRate);
        expect(std::abs(detector.bpm() - target) < 4.0, "automatic BPM " + std::to_string(target));
    }
}

void testEffects() {
    constexpr int sampleCount = 32768;
    for (const auto rate : {44100.0, 48000.0, 88200.0, 96000.0}) {
        for (int type = 0; type < static_cast<int>(qb::EffectType::count); ++type) {
            qb::EffectRack rack;
            rack.prepare(rate, sampleCount);
            qb::EffectParameters parameters;
            parameters.type = static_cast<qb::EffectType>(type);
            parameters.enabled = true;
            parameters.depth = 0.7F;
            parameters.time = 0.45F;
            parameters.bpm = 128.0;
            parameters.division = 3;
            rack.setParameters(parameters);
            juce::AudioBuffer<float> buffer(2, sampleCount);
            juce::AudioBuffer<float> original(2, sampleCount);
            for (int sample = 0; sample < sampleCount; ++sample) {
                const auto value = 0.2F * std::sin(static_cast<float>(sample) * 0.071F) +
                                   (sample == 0 ? 0.5F : 0.0F);
                buffer.setSample(0, sample, value);
                buffer.setSample(1, sample, value * 0.8F);
            }
            original.makeCopyOf(buffer);
            rack.process(buffer);
            bool finite = true;
            float peak{};
            for (int channel = 0; channel < 2; ++channel)
                for (int sample = 0; sample < sampleCount; ++sample) {
                    const auto value = buffer.getSample(channel, sample);
                    finite = finite && std::isfinite(value);
                    peak = std::max(peak, std::abs(value));
                }
            expect(finite, "effect finite " + std::to_string(type));
            expect(peak > 1.0e-5F && peak < 8.01F, "effect bounded " + std::to_string(type));
            expect(difference(buffer, original) > 0.01F,
                   "effect modifies signal " + std::to_string(type));
        }
    }
    qb::EffectRack bypass;
    bypass.prepare(48000.0, 1024);
    qb::EffectParameters parameters;
    parameters.enabled = false;
    bypass.setParameters(parameters);
    juce::AudioBuffer<float> buffer(2, 1024);
    buffer.clear();
    buffer.setSample(0, 0, std::numeric_limits<float>::infinity());
    buffer.setSample(1, 0, std::numeric_limits<float>::quiet_NaN());
    bypass.process(buffer);
    expect(std::isfinite(buffer.getSample(0, 0)) && std::isfinite(buffer.getSample(1, 0)),
           "NaN infinity protection");
}

void testMixer() {
    qb::MixerEngine mixer;
    mixer.prepare(48000.0, 512);
    juce::AudioBuffer<float> inputs(8, 512);
    juce::AudioBuffer<float> outputs(6, 512);
    inputs.clear();
    inputs.setSample(0, 0, 1.0F);
    inputs.setSample(1, 0, 0.5F);
    mixer.process(inputs, outputs);
    expect(std::abs(outputs.getSample(0, 0) - 0.8F) < 0.02F, "channel to master routing");
    expect(std::abs(outputs.getSample(2, 0) - 0.7F) < 0.02F, "booth routing");
    mixer.channels[0].mute.store(true);
    mixer.process(inputs, outputs);
    expect(std::abs(outputs.getSample(0, 0)) < 1.0e-4F, "channel mute");
    mixer.channels[0].mute.store(false);
    mixer.channels[0].cue.store(true);
    mixer.cueMix.store(0.0F);
    mixer.process(inputs, outputs);
    expect(std::abs(outputs.getSample(4, 0)) > 0.1F, "channel cue routing");
    mixer.channels[0].assignment.store(qb::CrossfaderAssignment::a);
    mixer.crossfader.store(1.0F);
    mixer.process(inputs, outputs);
    expect(std::abs(outputs.getSample(0, 0)) < 0.02F, "crossfader A attenuation");
    mixer.channels[0].assignment.store(qb::CrossfaderAssignment::b);
    mixer.process(inputs, outputs);
    expect(std::abs(outputs.getSample(0, 0)) > 0.5F, "crossfader B routing");
    mixer.channels[0].assignment.store(qb::CrossfaderAssignment::thru);
    mixer.channels[0].eqMode.store(qb::EqMode::isolator);
    mixer.channels[0].low.store(-1.0F);
    mixer.channels[0].mid.store(-1.0F);
    mixer.channels[0].high.store(-1.0F);
    mixer.process(inputs, outputs);
    expect(std::abs(outputs.getSample(0, 0)) < 1.0e-3F, "isolator full kill");
}

void testStateAndMidi() {
    qb::AppState state;
    state.effect = qb::EffectType::helix;
    state.channels[2].mute = true;
    const auto encoded = qb::StateStore::toVar(state);
    const auto decoded = qb::StateStore::fromVar(encoded);
    expect(decoded.has_value() && decoded->effect == qb::EffectType::helix &&
               decoded->channels[2].mute,
           "state round trip");
    expect(!qb::StateStore::fromVar(juce::JSON::parse("{ broken")), "corrupt state recovery");
    qb::MidiMapper mapper;
    qb::MidiMapping mapping;
    mapping.parameterId = "master";
    mapping.channel = 1;
    mapping.number = 7;
    mapping.minimum = 0.2F;
    mapping.maximum = 0.8F;
    mapping.pickupTolerance = 1.0F;
    mapper.setMappings({mapping});
    const auto result = mapper.process(juce::MidiMessage::controllerEvent(1, 7, 127), 0.5F);
    expect(result.has_value() && std::abs(result->second - 0.8F) < 0.01F, "MIDI scaling");
    qb::MidiMapper restored;
    expect(restored.deserialise(mapper.serialise()) && restored.mappings().size() == 1,
           "MIDI mapping serialization");
    mapping.pickupTolerance = 0.01F;
    mapper.setMappings({mapping});
    expect(!mapper.process(juce::MidiMessage::controllerEvent(1, 7, 0), 0.8F),
           "MIDI soft takeover");
}
} // namespace

int main() {
    testBeatAndTempo();
    testEffects();
    testMixer();
    testStateAndMidi();
    std::cout << "QuadBeat FX validation summary: " << checks << " checks, " << failures
              << " failures\n";
    return failures == 0 ? 0 : 1;
}
