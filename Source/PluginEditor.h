#pragma once

#include "PluginProcessor.h"
#include "gui/PanelWidgets.h"
#include "gui/TriVerseLookAndFeel.h"

#include <juce_audio_processors/juce_audio_processors.h>

// 1U rack panel drawn on a fixed logical canvas, then scaled to the editor size.
class FrontPanel final : public juce::Component
{
public:
    static constexpr int kWidth  = 1320;
    static constexpr int kHeight = 200;

    explicit FrontPanel (TriChorusProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;

    trichorus::gui::LevelMeter& getMeter() noexcept { return meter; }

private:
    struct Knob
    {
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        juce::String title, minLabel, maxLabel;
        bool big = false;
    };

    void setupKnob (Knob&, const char* paramId, juce::String title, juce::String minLabel, juce::String maxLabel, bool big);
    void placeKnob (Knob&, juce::Point<int> centre);

    void paintEar (juce::Graphics&, juce::Rectangle<float>) const;
    void paintSection (juce::Graphics&, juce::Rectangle<int>, const juce::String& title) const;
    void paintKnobDecor (juce::Graphics&, const Knob&) const;
    void paintBrand (juce::Graphics&, juce::Rectangle<float>) const;
    static void paintScrew (juce::Graphics&, juce::Point<float> centre, float radius, float slotAngle);

    TriChorusProcessor& processor;

    Knob int1, int2, int3, delay, rate;
    Knob tone, character, width;
    Knob input, mix, output;

    trichorus::gui::EffectButton effect;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> effectAttachment;
    trichorus::gui::ShapeSelector shape;
    trichorus::gui::LevelMeter meter;

    juce::Rectangle<int> brandArea, shapeArea, voicingArea, intensityArea, levelsArea, meterArea, effectArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrontPanel)
};

class TriChorusEditor final : public juce::AudioProcessorEditor,
                              private juce::Timer
{
public:
    explicit TriChorusEditor (TriChorusProcessor&);
    ~TriChorusEditor() override;

    void paint (juce::Graphics&) override {}
    void resized() override;

private:
    void timerCallback() override;

    TriChorusProcessor& chorusProcessor;
    trichorus::gui::TriVerseLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltips { this, 600 };
    FrontPanel panel;
    std::array<float, 2> meterDb { -100.0f, -100.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriChorusEditor)
};
