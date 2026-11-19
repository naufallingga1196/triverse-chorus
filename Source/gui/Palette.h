#pragma once

#include <juce_graphics/juce_graphics.h>

namespace trichorus::gui
{

namespace palette
{
    const juce::Colour panelTop     { 0xff1b1c1f };
    const juce::Colour panelBottom  { 0xff0d0e10 };
    const juce::Colour earTop       { 0xff232427 };
    const juce::Colour earBottom    { 0xff141517 };
    const juce::Colour sectionLine  { 0xff6d7075 };
    const juce::Colour legend       { 0xffe9e9e6 };
    const juce::Colour legendDim    { 0xffa6a8ab };
    const juce::Colour cyan         { 0xff22b8cc };
    const juce::Colour cyanLight    { 0xff7fe6f2 };
    const juce::Colour cyanDark     { 0xff0b6f7d };
    const juce::Colour knobBlack    { 0xff1a1a1c };
    const juce::Colour ledRed       { 0xffff3b2f };
    const juce::Colour ledGreen     { 0xff52ff6a };
    const juce::Colour ledAmber     { 0xffffc23a };
    const juce::Colour meterWindow  { 0xff0a0b0a };
}

// JUCE rotary angles: radians clockwise from 12 o'clock.
// 7:30 → 4:30, like a typical hardware knob.
constexpr float kRotaryStart = juce::MathConstants<float>::pi * 1.25f;
constexpr float kRotaryEnd   = juce::MathConstants<float>::pi * 2.75f;

inline juce::Font legendFont (float height, bool bold = true)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
}

} // namespace trichorus::gui
