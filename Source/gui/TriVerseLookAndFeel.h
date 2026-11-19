#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace trichorus::gui
{

// Knob style is chosen via the slider property "knobStyle": "cyan" or "black".
class TriVerseLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    TriVerseLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    void drawBubble (juce::Graphics&, juce::BubbleComponent&, const juce::Point<float>& tip,
                     const juce::Rectangle<float>& body) override;

    juce::Font getSliderPopupFont (juce::Slider&) override;
    int getSliderPopupPlacement (juce::Slider&) override;

    static void setKnobStyle (juce::Slider& s, bool cyan);

private:
    static juce::Path flutedOutline (juce::Point<float> centre, float radius, int flutes, float depth);
};

} // namespace trichorus::gui
