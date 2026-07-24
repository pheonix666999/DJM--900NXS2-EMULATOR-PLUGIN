#include "state/StateStore.h"
#include <algorithm>
#include <cmath>

namespace qb {
namespace {
template <typename T> int enumValue(T value) { return static_cast<int>(value); }

float safeFloat(const juce::var& value, const float fallback, const float low, const float high) {
    const auto parsed = static_cast<float>(value);
    return std::isfinite(parsed) ? std::clamp(parsed, low, high) : fallback;
}
} // namespace

juce::var StateStore::toVar(const AppState& state) {
    auto* root = new juce::DynamicObject();
    root->setProperty("version", state.version);
    root->setProperty("crossfader", state.crossfader);
    root->setProperty("master", state.master);
    root->setProperty("booth", state.booth);
    root->setProperty("headphones", state.headphones);
    root->setProperty("cueMix", state.cueMix);
    root->setProperty("effect", enumValue(state.effect));
    root->setProperty("effectBus", enumValue(state.effectBus));
    root->setProperty("division", state.division);
    root->setProperty("effectTime", state.effectTime);
    root->setProperty("effectDepth", state.effectDepth);
    root->setProperty("lowBand", state.lowBand);
    root->setProperty("midBand", state.midBand);
    root->setProperty("highBand", state.highBand);
    root->setProperty("quantize", state.quantize);
    root->setProperty("tempoSource", enumValue(state.tempoSource));
    root->setProperty("manualBpm", state.manualBpm);
    root->setProperty("uiScale", state.uiScale);
    root->setProperty("windowX", state.windowX);
    root->setProperty("windowY", state.windowY);
    root->setProperty("windowWidth", state.windowWidth);
    root->setProperty("windowHeight", state.windowHeight);
    root->setProperty("audioDeviceXml", juce::String(state.audioDeviceXml));
    root->setProperty("midiMappings", state.midiMappings);
    juce::Array<juce::var> channels;
    for (const auto& channel : state.channels) {
        auto* item = new juce::DynamicObject();
        item->setProperty("trim", channel.trim);
        item->setProperty("low", channel.low);
        item->setProperty("mid", channel.mid);
        item->setProperty("high", channel.high);
        item->setProperty("fader", channel.fader);
        item->setProperty("cue", channel.cue);
        item->setProperty("mute", channel.mute);
        item->setProperty("eqMode", enumValue(channel.eqMode));
        item->setProperty("assignment", enumValue(channel.assignment));
        channels.add(juce::var(item));
    }
    root->setProperty("channels", channels);
    return juce::var(root);
}

std::optional<AppState> StateStore::fromVar(const juce::var& value) {
    const auto* root = value.getDynamicObject();
    if (root == nullptr || !root->hasProperty("version"))
        return std::nullopt;
    const auto version = static_cast<int>(root->getProperty("version"));
    if (version < 1 || version > AppState::currentVersion)
        return std::nullopt;
    AppState state;
    state.version = version;
    state.crossfader = safeFloat(root->getProperty("crossfader"), 0.0F, -1.0F, 1.0F);
    state.master = safeFloat(root->getProperty("master"), 0.8F, 0.0F, 1.5F);
    state.booth = safeFloat(root->getProperty("booth"), 0.7F, 0.0F, 1.5F);
    state.headphones = safeFloat(root->getProperty("headphones"), 0.7F, 0.0F, 1.5F);
    state.cueMix = safeFloat(root->getProperty("cueMix"), 0.5F, 0.0F, 1.0F);
    state.effect =
        static_cast<EffectType>(std::clamp(static_cast<int>(root->getProperty("effect")), 0, 14));
    state.effectBus =
        static_cast<EffectBus>(std::clamp(static_cast<int>(root->getProperty("effectBus")), 0, 7));
    state.division = std::clamp(static_cast<int>(root->getProperty("division")), 0, 7);
    state.effectTime = safeFloat(root->getProperty("effectTime"), 0.5F, 0.0F, 1.0F);
    state.effectDepth = safeFloat(root->getProperty("effectDepth"), 0.5F, 0.0F, 1.0F);
    state.lowBand = root->getProperty("lowBand");
    state.midBand = root->getProperty("midBand");
    state.highBand = root->getProperty("highBand");
    if (!state.lowBand && !state.midBand && !state.highBand)
        state.lowBand = state.midBand = state.highBand = true;
    state.quantize = root->getProperty("quantize");
    state.tempoSource = static_cast<TempoSource>(
        std::clamp(static_cast<int>(root->getProperty("tempoSource")), 0, 2));
    state.manualBpm = std::clamp(static_cast<double>(root->getProperty("manualBpm")), 60.0, 200.0);
    state.uiScale = std::clamp(static_cast<double>(root->getProperty("uiScale")), 0.75, 2.0);
    state.windowX = root->getProperty("windowX");
    state.windowY = root->getProperty("windowY");
    state.windowWidth = std::clamp(static_cast<int>(root->getProperty("windowWidth")), 1100, 3840);
    state.windowHeight = std::clamp(static_cast<int>(root->getProperty("windowHeight")), 700, 2160);
    state.audioDeviceXml = root->getProperty("audioDeviceXml").toString().toStdString();
    state.midiMappings = root->getProperty("midiMappings");
    const auto channels = root->getProperty("channels");
    if (channels.isArray()) {
        for (int i = 0; i < std::min(channelCount, channels.getArray()->size()); ++i) {
            const auto* item = channels[i].getDynamicObject();
            if (item == nullptr)
                continue;
            auto& channel = state.channels[static_cast<size_t>(i)];
            channel.trim = safeFloat(item->getProperty("trim"), 0.0F, -1.0F, 1.0F);
            channel.low = safeFloat(item->getProperty("low"), 0.0F, -1.0F, 1.0F);
            channel.mid = safeFloat(item->getProperty("mid"), 0.0F, -1.0F, 1.0F);
            channel.high = safeFloat(item->getProperty("high"), 0.0F, -1.0F, 1.0F);
            channel.fader = safeFloat(item->getProperty("fader"), 1.0F, 0.0F, 1.0F);
            channel.cue = item->getProperty("cue");
            channel.mute = item->getProperty("mute");
            channel.eqMode = static_cast<EqMode>(
                std::clamp(static_cast<int>(item->getProperty("eqMode")), 0, 1));
            channel.assignment = static_cast<CrossfaderAssignment>(
                std::clamp(static_cast<int>(item->getProperty("assignment")), 0, 2));
        }
    }
    return state;
}

bool StateStore::save(const juce::File& file, const AppState& state) {
    const auto json = juce::JSON::toString(toVar(state), true);
    juce::TemporaryFile temporary(file);
    return temporary.getFile().replaceWithText(json) &&
           temporary.overwriteTargetFileWithTemporary();
}

AppState StateStore::loadOrDefault(const juce::File& file) {
    if (!file.existsAsFile())
        return {};
    const auto parsed = juce::JSON::parse(file.loadFileAsString());
    if (const auto state = fromVar(parsed))
        return *state;
    return {};
}
} // namespace qb
