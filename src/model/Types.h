#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace qb {
constexpr int channelCount = 4;
constexpr std::array<double, 8> beatValues{0.0625, 0.125, 0.25, 0.5, 0.75, 1.0, 2.0, 4.0};
constexpr std::array<std::string_view, 8> beatLabels{"1/16", "1/8", "1/4", "1/2",
                                                     "3/4",  "1",   "2",   "4"};
constexpr float lowMidCrossoverHz = 250.0F;
constexpr float midHighCrossoverHz = 2500.0F;

enum class TempoSource : std::uint8_t { automatic, manual, tap };
enum class CrossfaderAssignment : std::uint8_t { a, b, thru };
enum class CrossfaderCurve : std::uint8_t { smooth, constantPower, cut };
enum class EqMode : std::uint8_t { classic, isolator };
enum class EffectBus : std::uint8_t { mic, ch1, ch2, ch3, ch4, crossfaderA, crossfaderB, master };
enum class EffectType : std::uint8_t {
    delay,
    echo,
    pingPong,
    spiral,
    reverb,
    trans,
    filter,
    flanger,
    phaser,
    pitch,
    slipRoll,
    roll,
    vinylBrake,
    helix,
    pan,
    count
};

constexpr std::array<std::string_view, 15> effectNames{
    "DELAY",  "ECHO",  "PING PONG", "SPIRAL", "REVERB",      "TRANS", "FILTER", "FLANGER",
    "PHASER", "PITCH", "SLIP ROLL", "ROLL",   "VINYL BRAKE", "HELIX", "PAN"};
} // namespace qb
