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
#include <thread>

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

void testLiveTempoAnalysis() {
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr int totalSamples = static_cast<int>(sampleRate * 12.0);
    constexpr int clickInterval = static_cast<int>(sampleRate * 0.5);
    qb::TempoEngine tempo;
    tempo.prepareAnalysis(sampleRate);
    juce::AudioBuffer<float> block(2, blockSize);
    for (int position = 0; position < totalSamples; position += blockSize) {
        block.clear();
        for (int sample = 0; sample < blockSize && position + sample < totalSamples; ++sample)
            if ((position + sample) % clickInterval < 32) {
                block.setSample(0, sample, 1.0F);
                block.setSample(1, sample, 1.0F);
            }
        tempo.pushStereo(block.getReadPointer(0), block.getReadPointer(1), blockSize);
    }
    for (int attempt = 0; attempt < 100 && tempo.confidence() == 0.0; ++attempt)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    expect(tempo.confidence() > 0.08, "live onset queue produces confidence");
    expect(std::abs(tempo.bpm() - 120.0) < 2.0, "live onset queue detects 120 BPM");
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

void testReverbTail() {
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    qb::EffectRack rack;
    rack.prepare(sampleRate, blockSize);
    qb::EffectParameters parameters;
    parameters.type = qb::EffectType::reverb;
    parameters.enabled = true;
    parameters.depth = 1.0F;
    parameters.time = 0.75F;
    parameters.quantize = false;
    rack.setParameters(parameters);

    juce::AudioBuffer<float> block(2, blockSize);
    double earlyTailEnergy{};
    double lateTailEnergy{};
    for (int pass = 0; pass < 280; ++pass) {
        block.clear();
        if (pass == 0) {
            block.setSample(0, 0, 1.0F);
            block.setSample(1, 0, 0.35F);
        }
        rack.process(block);
        double blockEnergy{};
        for (int channel = 0; channel < block.getNumChannels(); ++channel)
            for (int sample = 0; sample < block.getNumSamples(); ++sample) {
                const auto value = block.getSample(channel, sample);
                blockEnergy += static_cast<double>(value) * static_cast<double>(value);
            }
        if (pass >= 12 && pass < 80)
            earlyTailEnergy += blockEnergy;
        if (pass >= 120)
            lateTailEnergy += blockEnergy;
    }
    expect(earlyTailEnergy > 1.0e-3, "reverb produces an audible diffuse tail");
    expect(lateTailEnergy > 1.0e-6, "reverb decay persists beyond the early reflections");
}

void testQuantizedActivation() {
    qb::EffectRack rack;
    rack.prepare(48000.0, 512);
    qb::EffectParameters parameters;
    parameters.type = qb::EffectType::trans;
    parameters.depth = 1.0F;
    parameters.enabled = false;
    parameters.quantize = true;
    parameters.bpm = 120.0;
    parameters.division = 5;
    rack.setParameters(parameters);
    juce::AudioBuffer<float> block(2, 512);
    juce::AudioBuffer<float> original(2, 512);
    const auto fill = [&] {
        for (int channel = 0; channel < 2; ++channel)
            for (int sample = 0; sample < block.getNumSamples(); ++sample)
                block.setSample(channel, sample,
                                0.2F * std::sin(static_cast<float>(sample) * 0.13F));
        original.makeCopyOf(block);
    };
    fill();
    rack.process(block);
    parameters.enabled = true;
    rack.setParameters(parameters);
    float beforeBoundaryDifference{};
    for (int pass = 0; pass < 40; ++pass) {
        fill();
        rack.process(block);
        beforeBoundaryDifference += difference(block, original);
    }
    expect(beforeBoundaryDifference < 0.01F, "quantized effect waits for beat boundary");
    float afterBoundaryDifference{};
    for (int pass = 0; pass < 10; ++pass) {
        fill();
        rack.process(block);
        afterBoundaryDifference += difference(block, original);
    }
    expect(afterBoundaryDifference > 0.1F, "quantized effect starts on beat boundary");
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

void testPhysicalRoutingAndMicrophone() {
    qb::MixerEngine mixer;
    mixer.prepare(48000.0, 256);
    for (int channel = 0; channel < qb::channelCount; ++channel)
        for (int side = 0; side < 2; ++side)
            mixer.setChannelInputMapping(channel, side, -1);
    mixer.setChannelInputMapping(0, 0, 8);
    mixer.setChannelInputMapping(0, 1, 9);
    mixer.setMicrophoneInputMapping(7);
    mixer.setOutputMapping(0, 0, 4);
    mixer.setOutputMapping(0, 1, 5);
    mixer.setOutputMapping(1, 0, -1);
    mixer.setOutputMapping(1, 1, -1);
    mixer.setOutputMapping(2, 0, -1);
    mixer.setOutputMapping(2, 1, -1);
    juce::AudioBuffer<float> physicalInputs(10, 256);
    juce::AudioBuffer<float> physicalOutputs(6, 256);
    physicalInputs.clear();
    physicalInputs.setSample(8, 0, 1.0F);
    physicalInputs.setSample(9, 0, 0.5F);
    physicalInputs.setSample(7, 0, 0.25F);
    mixer.processMappedDeviceBlock(physicalInputs, physicalOutputs);
    expect(std::abs(physicalOutputs.getSample(0, 0)) < 1.0e-5F,
           "unmapped physical output remains silent");
    expect(std::abs(physicalOutputs.getSample(4, 0) - 1.0F) < 0.03F,
           "mapped master left includes microphone bus");
    expect(std::abs(physicalOutputs.getSample(5, 0) - 0.6F) < 0.03F,
           "mapped master right includes mono microphone");
    mixer.microphoneMute.store(true);
    mixer.processMappedDeviceBlock(physicalInputs, physicalOutputs);
    expect(std::abs(physicalOutputs.getSample(4, 0) - 0.8F) < 0.03F,
           "microphone mute removes microphone bus");

    std::array<float, 256> callbackLeft{};
    std::array<float, 256> callbackRight{};
    std::array<float, 256> callbackOutputLeft{};
    std::array<float, 256> callbackOutputRight{};
    callbackLeft[0] = 1.0F;
    callbackRight[0] = 0.5F;
    std::array<const float*, 10> callbackInputs{};
    callbackInputs[8] = callbackLeft.data();
    callbackInputs[9] = callbackRight.data();
    std::array<float*, 6> callbackOutputs{};
    callbackOutputs[4] = callbackOutputLeft.data();
    callbackOutputs[5] = callbackOutputRight.data();
    mixer.audioDeviceIOCallbackWithContext(
        callbackInputs.data(), static_cast<int>(callbackInputs.size()), callbackOutputs.data(),
        static_cast<int>(callbackOutputs.size()), 256, {});
    expect(std::abs(callbackOutputLeft[0] - 0.8F) < 0.03F &&
               std::abs(callbackOutputRight[0] - 0.4F) < 0.03F,
           "device callback tolerates inactive channel pointers");
}

void testStateAndMidi() {
    qb::AppState state;
    state.effect = qb::EffectType::helix;
    state.effectEnabled = false;
    state.channels[2].mute = true;
    state.inputMappings[8] = 6;
    state.outputMappings[4] = -1;
    state.microphoneLevel = 1.4F;
    state.analysisSource = qb::TempoAnalysisSource::ch3;
    const auto encoded = qb::StateStore::toVar(state);
    const auto decoded = qb::StateStore::fromVar(encoded);
    expect(decoded.has_value() && decoded->effect == qb::EffectType::helix &&
               !decoded->effectEnabled && decoded->channels[2].mute &&
               decoded->inputMappings[8] == 6 && decoded->outputMappings[4] == -1 &&
               std::abs(decoded->microphoneLevel - 1.4F) < 0.001F &&
               decoded->analysisSource == qb::TempoAnalysisSource::ch3,
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
    mapping.inverted = true;
    mapping.minimum = 0.2F;
    mapping.maximum = 0.9F;
    mapping.mode = qb::MidiMode::relativeTwosComplement;
    mapper.setMappings({mapping});
    qb::MidiMapper edited;
    expect(edited.deserialise(mapper.serialise()) &&
               edited.mappings()[0].mode == qb::MidiMode::relativeTwosComplement &&
               edited.mappings()[0].inverted &&
               std::abs(edited.mappings()[0].pickupTolerance - 0.01F) < 0.001F,
           "MIDI editor fields serialize");
    mapping.mode = qb::MidiMode::absolute;
    mapping.inverted = true;
    mapping.pickupTolerance = 1.0F;
    mapper.setMappings({mapping});
    const auto inverted = mapper.process(juce::MidiMessage::controllerEvent(1, 7, 127), 0.5F);
    expect(inverted.has_value() && std::abs(inverted->second - 0.2F) < 0.01F,
           "MIDI inverted custom range");
    mapping.inverted = false;
    mapping.minimum = 0.0F;
    mapping.maximum = 1.0F;
    mapping.pickupTolerance = 0.01F;
    mapper.setMappings({mapping});
    expect(!mapper.process(juce::MidiMessage::controllerEvent(1, 7, 0), 0.8F),
           "MIDI soft takeover");
}
} // namespace

int main() {
    testBeatAndTempo();
    testLiveTempoAnalysis();
    testEffects();
    testReverbTail();
    testQuantizedActivation();
    testMixer();
    testPhysicalRoutingAndMicrophone();
    testStateAndMidi();
    std::cout << "QuadBeat FX validation summary: " << checks << " checks, " << failures
              << " failures\n";
    return failures == 0 ? 0 : 1;
}
