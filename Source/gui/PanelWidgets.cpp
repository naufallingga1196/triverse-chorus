#include "PanelWidgets.h"
#include "Palette.h"

namespace trichorus::gui
{

using namespace juce;

namespace
{
    void drawLed (Graphics& g, Point<float> centre, float radius, Colour colour, bool lit)
    {
        const auto r = Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre);

        if (lit)
        {
            // Pendar
            g.setGradientFill (ColourGradient (colour.withAlpha (0.45f), centre.x, centre.y,
                                               colour.withAlpha (0.0f), centre.x + radius * 3.0f, centre.y, true));
            g.fillEllipse (r.expanded (radius * 2.0f));
        }

        g.setColour (Colours::black);
        g.fillEllipse (r.expanded (1.0f));
        g.setGradientFill (ColourGradient (lit ? colour.brighter (0.8f) : colour.darker (2.2f), centre.x - radius * 0.3f, centre.y - radius * 0.4f,
                                           lit ? colour : colour.darker (3.0f), centre.x + radius, centre.y + radius, true));
        g.fillEllipse (r);
    }
}

//==============================================================================
EffectButton::EffectButton() : Button ("Effect")
{
    setClickingTogglesState (true);
    setTooltip ("Effect on / bypass");
}

void EffectButton::paintButton (Graphics& g, bool highlighted, bool down)
{
    const bool active = ! getToggleState(); // toggle = bypass
    auto b = getLocalBounds().toFloat().reduced (3.0f);

    // Bezel
    g.setColour (Colours::black);
    g.fillRoundedRectangle (b.expanded (2.0f), 3.0f);
    g.setColour (Colour (0xff3a3b3e));
    g.drawRoundedRectangle (b.expanded (2.0f), 3.0f, 1.0f);

    if (down)
        b = b.reduced (1.0f).translated (0.0f, 0.6f);

    if (active)
    {
        g.setGradientFill (ColourGradient (palette::ledRed.withAlpha (0.35f), b.getCentreX(), b.getCentreY(),
                                           palette::ledRed.withAlpha (0.0f), b.getCentreX() + b.getWidth(), b.getCentreY(), true));
        g.fillRect (b.expanded (b.getWidth() * 0.5f));
    }

    const Colour base = active ? palette::ledRed : Colour (0xff4a0c09);
    ColourGradient cg (base.brighter (active ? 0.6f : 0.2f), b.getX(), b.getY(),
                       base.darker (active ? 0.4f : 0.6f), b.getRight(), b.getBottom(), false);
    g.setGradientFill (cg);
    g.fillRoundedRectangle (b, 2.0f);

    // Tekstur lensa: kisi halus
    g.setColour (Colours::black.withAlpha (0.12f));
    for (float yy = b.getY() + 2.0f; yy < b.getBottom(); yy += 3.0f)
        g.drawHorizontalLine ((int) yy, b.getX() + 1.0f, b.getRight() - 1.0f);

    g.setColour (Colours::white.withAlpha (active ? 0.45f : 0.12f));
    g.fillRoundedRectangle (b.withHeight (b.getHeight() * 0.32f).reduced (2.0f, 1.5f), 1.5f);

    if (highlighted)
    {
        g.setColour (Colours::white.withAlpha (0.08f));
        g.fillRoundedRectangle (b, 2.0f);
    }
}

//==============================================================================
ShapeSelector::ShapeSelector (RangedAudioParameter& shapeParam)
    : attachment (shapeParam, [this] (float v) { selected = jlimit (0, 2, roundToInt (v)); repaint(); })
{
    attachment.sendInitialUpdate();
    setTooltip ("LFO shape: Sine / Triangle / Analog");
    setMouseCursor (MouseCursor::PointingHandCursor);
}

Rectangle<float> ShapeSelector::buttonBounds() const
{
    const float d = 26.0f;
    return Rectangle<float> (d, d).withCentre ({ getWidth() * 0.5f, getHeight() - d * 0.5f - 4.0f });
}

void ShapeSelector::paint (Graphics& g)
{
    static const char* names[] = { "SINE", "TRI", "ANLG" };
    const float w = (float) getWidth();
    const float spacing = w / 3.0f;

    g.setFont (legendFont (8.0f));
    for (int i = 0; i < 3; ++i)
    {
        const float cx = spacing * ((float) i + 0.5f);
        g.setColour (palette::legend);
        g.drawText (names[i], Rectangle<float> (spacing, 10.0f).withCentre ({ cx, 6.0f }), Justification::centred);
        drawLed (g, { cx, 22.0f }, 3.6f, palette::ledRed, selected == i);
    }

    // Tombol tekan bulat
    auto b = buttonBounds();
    g.setColour (Colours::black);
    g.fillEllipse (b.expanded (2.5f));
    g.setGradientFill (ColourGradient (Colour (0xff505256), b.getX(), b.getY(),
                                       Colour (0xff0c0c0d), b.getRight(), b.getBottom(), false));
    g.fillEllipse (b);

    auto plunger = b.reduced (pressed ? 6.0f : 5.0f);
    g.setGradientFill (ColourGradient (Colour (pressed ? 0xff141415 : 0xff2b2c2f), plunger.getX(), plunger.getY(),
                                       Colour (pressed ? 0xff2a2b2e : 0xff0a0a0b), plunger.getRight(), plunger.getBottom(), false));
    g.fillEllipse (plunger);
    g.setColour (Colours::white.withAlpha (0.1f));
    g.drawEllipse (plunger, 0.8f);
}

void ShapeSelector::mouseDown (const MouseEvent& e)
{
    if (! buttonBounds().expanded (6.0f).contains (e.position) && e.position.y > 30.0f)
        return;

    pressed = true;
    // Klik pada LED/label memilih langsung, klik tombol berputar ke berikutnya.
    const int next = e.position.y <= 30.0f ? jlimit (0, 2, (int) (e.position.x / (getWidth() / 3.0f)))
                                           : (selected + 1) % 3;
    attachment.setValueAsCompleteGesture ((float) next);
    repaint();
}

void ShapeSelector::mouseUp (const MouseEvent&)
{
    pressed = false;
    repaint();
}

//==============================================================================
void LevelMeter::setLevels (float dbLeft, float dbRight)
{
    if (std::abs (dbLeft - levels[0]) < 0.05f && std::abs (dbRight - levels[1]) < 0.05f)
        return;

    levels = { dbLeft, dbRight };
    repaint();
}

void LevelMeter::paintBar (Graphics& g, Rectangle<float> window, float db) const
{
    // Jendela kaca gelap
    g.setColour (Colour (0xff2c2d30));
    g.fillRect (window.expanded (1.5f));
    g.setColour (palette::meterWindow);
    g.fillRect (window);

    const int n = (int) kScale.size();
    const float segW = window.getWidth() / (float) n;

    for (int i = 0; i < n; ++i)
    {
        const float threshold = kScale[(size_t) i];
        const Colour c = threshold < -3.0f ? palette::ledGreen
                       : threshold < 1.0f  ? palette::ledAmber
                                           : palette::ledRed;
        const bool lit = db >= threshold;
        auto seg = Rectangle<float> (window.getX() + segW * (float) i, window.getY(), segW, window.getHeight())
                       .reduced (1.4f, 2.0f);

        if (lit)
        {
            g.setColour (c.withAlpha (0.25f));
            g.fillRect (seg.expanded (1.2f));
        }
        g.setColour (lit ? c : c.darker (3.2f).withAlpha (0.8f));
        g.fillRect (seg);
        if (lit)
        {
            g.setColour (Colours::white.withAlpha (0.35f));
            g.fillRect (seg.withHeight (seg.getHeight() * 0.35f));
        }
    }

    // Pantulan kaca
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.10f), window.getX(), window.getY(),
                                       Colours::transparentWhite, window.getX(), window.getCentreY(), false));
    g.fillRect (window);
}

void LevelMeter::paint (Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    const float labelW = 26.0f;
    const float barH = 11.0f;

    const auto barL = Rectangle<float> (labelW + 2.0f, 6.0f, area.getWidth() - labelW - 20.0f, barH);
    const auto scale = barL.withY (barL.getBottom() + 4.0f).withHeight (10.0f);
    const auto barR = barL.withY (scale.getBottom() + 4.0f);

    g.setColour (palette::legend);
    g.setFont (legendFont (9.5f));
    g.drawText ("Ch.L", Rectangle<float> (0.0f, barL.getY(), labelW, barH), Justification::centredLeft);
    g.drawText ("Ch.R", Rectangle<float> (0.0f, barR.getY(), labelW, barH), Justification::centredLeft);

    paintBar (g, barL, levels[0]);
    paintBar (g, barR, levels[1]);

    const int n = (int) kScale.size();
    const float segW = scale.getWidth() / (float) n;
    g.setFont (legendFont (7.2f));
    for (int i = 0; i < n; ++i)
    {
        const float v = kScale[(size_t) i];
        // Label hanya titik utama agar tidak berdempetan; LED tetap 13.
        if (v == -15.0f || v == -7.0f || v == -2.0f || v == 2.0f)
            continue;
        const String text = v > 0.0f ? "+" + String ((int) v) : String ((int) v);
        g.setColour (v > 0.0f ? palette::ledRed.brighter (0.4f) : palette::legend);
        g.drawText (text, Rectangle<float> (scale.getX() + segW * (float) i - 2.0f, scale.getY(), segW + 4.0f, scale.getHeight()),
                    Justification::centred);
    }
    g.setColour (palette::legend);
    g.drawText ("dB", Rectangle<float> (scale.getRight() + 1.0f, scale.getY(), 14.0f, scale.getHeight()), Justification::centredLeft);
}

} // namespace trichorus::gui
