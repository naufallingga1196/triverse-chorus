#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace trichorus::params
{

namespace id
{
    inline constexpr const char* input     = "input";
    inline constexpr const char* rate      = "rate";
    inline constexpr const char* int1      = "int1";
    inline constexpr const char* int2      = "int2";
    inline constexpr const char* int3      = "int3";
    inline constexpr const char* delay     = "delay";
    inline constexpr const char* shape     = "shape";
    inline constexpr const char* width     = "width";
    inline constexpr const char* tone      = "tone";
    inline constexpr const char* character = "char";
    inline constexpr const char* mix       = "mix";
    inline constexpr const char* output    = "output";
    inline constexpr const char* bypass    = "bypass";
}

inline const juce::StringArray shapeNames { "Sine", "Triangle", "Analog" };

// A true logarithmic range (not just a skew).
inline juce::NormalisableRange<float> logRange (float min, float max)
{
    return { min, max,
             [] (float start, float end, float t) { return start * std::pow (end / start, t); },
             [] (float start, float end, float v) { return std::log (v / start) / std::log (end / start); },
             [] (float start, float end, float v) { return juce::jlimit (start, end, v); } };
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    using Float = AudioParameterFloat;
    using Attr  = AudioParameterFloatAttributes;

    auto dB = Attr().withLabel ("dB").withStringFromValueFunction (
        [] (float v, int) { return String (v, 1) + " dB"; });

    auto percent = Attr().withLabel ("%").withStringFromValueFunction (
        [] (float v, int) { return String (roundToInt (v)) + " %"; });

    auto hz = Attr().withLabel ("Hz").withStringFromValueFunction (
        [] (float v, int) { return v < 1.0f ? String (v, 2) + " Hz" : String (v, 1) + " Hz"; });

    auto ms = Attr().withLabel ("ms").withStringFromValueFunction (
        [] (float v, int) { return String (v, 1) + " ms"; });

    auto kHz = Attr().withLabel ("Hz").withStringFromValueFunction (
        [] (float v, int) { return String (v / 1000.0f, 1) + " kHz"; });

    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<Float> (ParameterID { id::input, 1 }, "Input",
                                         NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, dB));
    layout.add (std::make_unique<Float> (ParameterID { id::rate, 1 }, "Rate",
                                         logRange (0.05f, 10.0f), 0.6f, hz));
    layout.add (std::make_unique<Float> (ParameterID { id::int1, 1 }, "Intensity I",
                                         NormalisableRange<float> (0.0f, 100.0f, 0.1f), 45.0f, percent));
    layout.add (std::make_unique<Float> (ParameterID { id::int2, 1 }, "Intensity II",
                                         NormalisableRange<float> (0.0f, 100.0f, 0.1f), 45.0f, percent));
    layout.add (std::make_unique<Float> (ParameterID { id::int3, 1 }, "Intensity III",
                                         NormalisableRange<float> (0.0f, 100.0f, 0.1f), 45.0f, percent));
    layout.add (std::make_unique<Float> (ParameterID { id::delay, 1 }, "Delay",
                                         logRange (2.0f, 20.0f), 7.0f, ms));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id::shape, 1 }, "LFO Shape",
                                                        shapeNames, 0));
    layout.add (std::make_unique<Float> (ParameterID { id::width, 1 }, "Width",
                                         NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, percent));
    layout.add (std::make_unique<Float> (ParameterID { id::tone, 1 }, "Tone",
                                         logRange (4000.0f, 14000.0f), 9000.0f, kHz));
    layout.add (std::make_unique<Float> (ParameterID { id::character, 1 }, "Character",
                                         NormalisableRange<float> (0.0f, 100.0f, 0.1f), 30.0f, percent));
    layout.add (std::make_unique<Float> (ParameterID { id::mix, 1 }, "Mix",
                                         NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, percent));
    layout.add (std::make_unique<Float> (ParameterID { id::output, 1 }, "Output",
                                         NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, dB));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { id::bypass, 1 }, "Bypass", false));

    return layout;
}

} // namespace trichorus::params
