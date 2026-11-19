#include "TriVerseLookAndFeel.h"
#include "Palette.h"

namespace trichorus::gui
{

using namespace juce;

TriVerseLookAndFeel::TriVerseLookAndFeel()
{
    setColour (BubbleComponent::backgroundColourId, Colour (0xf0101112));
    setColour (BubbleComponent::outlineColourId, palette::cyan);
    setColour (TooltipWindow::textColourId, palette::legend);
}

void TriVerseLookAndFeel::setKnobStyle (Slider& s, bool cyan)
{
    s.getProperties().set ("knobStyle", cyan ? "cyan" : "black");
}

Path TriVerseLookAndFeel::flutedOutline (Point<float> centre, float radius, int flutes, float depth)
{
    Path p;
    constexpr int stepsPerFlute = 6;
    const int total = flutes * stepsPerFlute;

    for (int i = 0; i <= total; ++i)
    {
        const float a = MathConstants<float>::twoPi * (float) i / (float) total;
        // Profil gerigi bulat: cos dipangkas → alur sempit, punggung lebar.
        const float wave = 0.5f + 0.5f * std::cos (a * (float) flutes);
        const float r = radius - depth * (1.0f - std::pow (wave, 0.35f));
        const auto pt = centre.getPointOnCircumference (r, a);

        if (i == 0) p.startNewSubPath (pt);
        else        p.lineTo (pt);
    }
    p.closeSubPath();
    return p;
}

void TriVerseLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int width, int height,
                                            float sliderPos, float startAngle, float endAngle, Slider& slider)
{
    const bool cyan = slider.getProperties()["knobStyle"].toString() == "cyan";

    const auto bounds = Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (2.0f);
    const float radius = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    // Bayangan jatuh di panel
    {
        Path shadowShape;
        shadowShape.addEllipse (Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));
        DropShadow (Colours::black.withAlpha (0.85f), (int) (radius * 0.35f), { 0, (int) (radius * 0.12f) + 1 })
            .drawForPath (g, shadowShape);
    }

    // Rok bergerigi (skirt)
    const auto skirt = flutedOutline (centre, radius, cyan ? 18 : 22, radius * 0.07f);
    const Colour skirtBase = cyan ? palette::cyanDark : Colour (0xff0e0e10);
    g.setGradientFill (ColourGradient (skirtBase.brighter (cyan ? 0.55f : 0.35f), centre.x, centre.y - radius,
                                       skirtBase.darker (0.6f), centre.x, centre.y + radius, false));
    g.fillPath (skirt);
    g.setColour (Colours::black.withAlpha (0.6f));
    g.strokePath (skirt, PathStrokeType (0.8f));

    // Tutup (cap) dengan highlight radial dari kiri atas
    const float capR = radius * (cyan ? 0.80f : 0.72f);
    const auto cap = Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (centre);

    if (cyan)
    {
        ColourGradient cg (palette::cyanLight, centre.x - capR * 0.45f, centre.y - capR * 0.55f,
                           palette::cyanDark, centre.x + capR * 0.7f, centre.y + capR * 0.9f, true);
        cg.addColour (0.45, palette::cyan);
        g.setGradientFill (cg);
    }
    else
    {
        ColourGradient cg (Colour (0xff5a5c60), centre.x - capR * 0.4f, centre.y - capR * 0.5f,
                           Colour (0xff121214), centre.x + capR * 0.7f, centre.y + capR * 0.9f, true);
        cg.addColour (0.5, Colour (0xff2a2b2e));
        g.setGradientFill (cg);
    }
    g.fillEllipse (cap);

    // Cincin tepi cap
    g.setColour (Colours::black.withAlpha (0.45f));
    g.drawEllipse (cap, 1.0f);
    g.setColour (Colours::white.withAlpha (cyan ? 0.35f : 0.12f));
    g.drawEllipse (cap.reduced (1.0f), 0.7f);

    // Kilap specular kecil
    const auto gloss = Rectangle<float> (capR * 0.9f, capR * 0.5f)
                           .withCentre ({ centre.x - capR * 0.18f, centre.y - capR * 0.45f });
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (cyan ? 0.45f : 0.18f), gloss.getCentreX(), gloss.getY(),
                                       Colours::transparentWhite, gloss.getCentreX(), gloss.getBottom(), false));
    g.fillEllipse (gloss);

    // Garis penunjuk
    const auto inner = centre.getPointOnCircumference (capR * 0.25f, angle);
    const auto outer = centre.getPointOnCircumference (radius * 0.94f, angle);
    g.setColour (Colours::black.withAlpha (0.5f));
    g.drawLine ({ inner.translated (0.6f, 0.8f), outer.translated (0.6f, 0.8f) }, cyan ? 3.0f : 2.6f);
    g.setColour (Colours::white);
    g.drawLine ({ inner, outer }, cyan ? 2.4f : 2.0f);

    if (slider.hasKeyboardFocus (true) || slider.isMouseOverOrDragging())
    {
        g.setColour (palette::cyanLight.withAlpha (slider.isMouseButtonDown() ? 0.35f : 0.18f));
        g.drawEllipse (Rectangle<float> (radius * 2.0f + 3.0f, radius * 2.0f + 3.0f).withCentre (centre), 1.2f);
    }
}

void TriVerseLookAndFeel::drawBubble (Graphics& g, BubbleComponent&, const Point<float>&, const Rectangle<float>& body)
{
    g.setColour (findColour (BubbleComponent::backgroundColourId));
    g.fillRoundedRectangle (body, 3.0f);
    g.setColour (findColour (BubbleComponent::outlineColourId).withAlpha (0.8f));
    g.drawRoundedRectangle (body.reduced (0.5f), 3.0f, 1.0f);
}

Font TriVerseLookAndFeel::getSliderPopupFont (Slider&)
{
    return legendFont (12.0f);
}

int TriVerseLookAndFeel::getSliderPopupPlacement (Slider&)
{
    return BubbleComponent::above;
}

} // namespace trichorus::gui
