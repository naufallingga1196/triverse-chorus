#include "PluginEditor.h"
#include "gui/Palette.h"
#include "params/ParameterLayout.h"

using namespace juce;
using namespace trichorus::gui;
namespace ids = trichorus::params::id;

namespace
{
    constexpr int kEarWidth   = 38;
    constexpr int kBigKnob    = 56;
    constexpr int kSmallKnob  = 36;
    constexpr int kKnobRow    = 100;
}

//==============================================================================
FrontPanel::FrontPanel (TriChorusProcessor& p)
    : processor (p),
      shape (*p.getState().getParameter (ids::shape))
{
    setupKnob (int1, ids::int1, "I",   "MIN", "MAX", true);
    setupKnob (int2, ids::int2, "II",  "MIN", "MAX", true);
    setupKnob (int3, ids::int3, "III", "MIN", "MAX", true);
    setupKnob (delay, ids::delay, "ms", "2", "20", false);
    setupKnob (rate, ids::rate, {}, "SLOW", "FAST", true);

    setupKnob (tone, ids::tone, "Tone", "DARK", "BRIGHT", false);
    setupKnob (character, ids::character, "Character", {}, {}, false);
    setupKnob (width, ids::width, "Width", "MONO", "WIDE", false);

    setupKnob (input, ids::input, "Input", "-12", "+12", false);
    setupKnob (mix, ids::mix, "Mix", "DRY", "WET", false);
    setupKnob (output, ids::output, "Output", "-12", "+12", false);

    addAndMakeVisible (effect);
    effectAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getState(), ids::bypass, effect);

    addAndMakeVisible (shape);
    addAndMakeVisible (meter);

    setSize (kWidth, kHeight);
}

void FrontPanel::setupKnob (Knob& k, const char* paramId, String title, String minLabel, String maxLabel, bool big)
{
    k.title = std::move (title);
    k.minLabel = std::move (minLabel);
    k.maxLabel = std::move (maxLabel);
    k.big = big;

    auto& s = k.slider;
    s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (kRotaryStart, kRotaryEnd, true);
    s.setMouseDragSensitivity (220);
    s.setPopupDisplayEnabled (true, true, this);
    s.setScrollWheelEnabled (true);
    TriVerseLookAndFeel::setKnobStyle (s, big);
    addAndMakeVisible (s);

    k.attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (processor.getState(), paramId, s);

    if (auto* param = processor.getState().getParameter (paramId))
    {
        s.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
        s.setTooltip (param->getName (64) + " (double-click untuk reset)");
    }
}

void FrontPanel::placeKnob (Knob& k, Point<int> centre)
{
    const int d = k.big ? kBigKnob : kSmallKnob;
    k.slider.setBounds (Rectangle<int> (d + 6, d + 6).withCentre (centre));
}

void FrontPanel::resized()
{
    auto face = getLocalBounds().reduced (kEarWidth, 0);
    face.removeFromLeft (6);

    brandArea     = face.removeFromLeft (100);
    shapeArea     = face.removeFromLeft (100);
    voicingArea   = face.removeFromLeft (184);
    intensityArea = face.removeFromLeft (400);
    levelsArea    = face.removeFromLeft (184);
    meterArea     = face.removeFromLeft (190);
    effectArea    = face;

    auto column = [] (Rectangle<int> area, int index, int count)
    {
        const int w = area.getWidth() / count;
        return area.getX() + w * index + w / 2;
    };

    placeKnob (tone,      { column (voicingArea, 0, 3), kKnobRow });
    placeKnob (character, { column (voicingArea, 1, 3), kKnobRow });
    placeKnob (width,     { column (voicingArea, 2, 3), kKnobRow });

    // Intensity I–III (84 px tiap kolom) + trim Delay (64) + Rate (84)
    const int x0 = intensityArea.getX();
    placeKnob (int1,  { x0 + 42,  kKnobRow });
    placeKnob (int2,  { x0 + 126, kKnobRow });
    placeKnob (int3,  { x0 + 210, kKnobRow });
    placeKnob (delay, { x0 + 284, kKnobRow - 6 });
    placeKnob (rate,  { x0 + 358, kKnobRow });

    placeKnob (input,  { column (levelsArea, 0, 3), kKnobRow });
    placeKnob (mix,    { column (levelsArea, 1, 3), kKnobRow });
    placeKnob (output, { column (levelsArea, 2, 3), kKnobRow });

    shape.setBounds (shapeArea.getX() + 10, 56, shapeArea.getWidth() - 20, 92);
    meter.setBounds (meterArea.getX() + 10, 72, meterArea.getWidth() - 16, 56);
    effect.setBounds (Rectangle<int> (44, 44).withCentre (effectArea.getCentre().withY (kKnobRow)));
}

//==============================================================================
void FrontPanel::paintScrew (Graphics& g, Point<float> c, float r, float slotAngle)
{
    const auto head = Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c);
    g.setColour (Colours::black.withAlpha (0.7f));
    g.fillEllipse (head.expanded (1.2f).translated (0.0f, 0.8f));
    g.setGradientFill (ColourGradient (Colour (0xffb9bcc0), head.getX(), head.getY(),
                                       Colour (0xff3e4043), head.getRight(), head.getBottom(), false));
    g.fillEllipse (head);

    // Kepala phillips
    g.setColour (Colour (0xff1b1c1e));
    for (float a : { slotAngle, slotAngle + MathConstants<float>::halfPi })
    {
        const auto d = Point<float> (std::cos (a), std::sin (a)) * (r * 0.7f);
        g.drawLine ({ c - d, c + d }, r * 0.28f);
    }
}

void FrontPanel::paintEar (Graphics& g, Rectangle<float> ear) const
{
    g.setGradientFill (ColourGradient (palette::earTop, 0.0f, ear.getY(), palette::earBottom, 0.0f, ear.getBottom(), false));
    g.fillRect (ear);

    // Lubang rak berbentuk oval + sekrup
    for (float cy : { ear.getY() + 46.0f, ear.getBottom() - 46.0f })
    {
        const auto slot = Rectangle<float> (22.0f, 13.0f).withCentre ({ ear.getCentreX(), cy });
        g.setColour (Colour (0xff050505));
        g.fillRoundedRectangle (slot, 6.5f);
        g.setColour (Colours::white.withAlpha (0.08f));
        g.drawRoundedRectangle (slot.translated (0.0f, 0.7f), 6.5f, 0.8f);
        paintScrew (g, { ear.getCentreX() + 2.0f, cy }, 5.6f, 0.4f + cy * 0.01f);
    }

    // Tepi lipatan
    g.setColour (Colours::white.withAlpha (0.06f));
    g.drawVerticalLine ((int) ear.getX(), ear.getY(), ear.getBottom());
    g.setColour (Colours::black.withAlpha (0.6f));
    g.drawVerticalLine ((int) ear.getRight() - 1, ear.getY(), ear.getBottom());
}

void FrontPanel::paintSection (Graphics& g, Rectangle<int> area, const String& title) const
{
    const auto box = area.toFloat().reduced (4.0f, 12.0f);
    g.setColour (palette::sectionLine);
    g.drawRect (box, 1.0f);

    if (title.isNotEmpty())
    {
        g.setColour (palette::legend);
        g.setFont (legendFont (11.0f));
        g.drawText (title, box.withHeight (22.0f).translated (0.0f, 3.0f), Justification::centred);
    }
}

void FrontPanel::paintKnobDecor (Graphics& g, const Knob& k) const
{
    const auto b = k.slider.getBounds().toFloat();
    const auto c = b.getCentre();
    const float r = (k.big ? kBigKnob : kSmallKnob) * 0.5f;

    // Titik skala 0–10
    g.setColour (palette::legend);
    for (int i = 0; i <= 10; ++i)
    {
        const float a = kRotaryStart + (kRotaryEnd - kRotaryStart) * (float) i / 10.0f;
        const auto pt = c.getPointOnCircumference (r + 6.0f, a);
        const float dot = (i == 0 || i == 5 || i == 10) ? 1.6f : 1.0f;
        g.fillEllipse (Rectangle<float> (dot * 2.0f, dot * 2.0f).withCentre (pt));
    }

    const float labelY = c.y + r + 4.0f;
    g.setFont (legendFont (k.big ? 8.0f : 7.5f));
    if (k.minLabel.isNotEmpty())
        g.drawText (k.minLabel, Rectangle<float> (40.0f, 10.0f).withCentre ({ c.x - r * 0.85f, labelY }), Justification::centred);
    if (k.maxLabel.isNotEmpty())
        g.drawText (k.maxLabel, Rectangle<float> (40.0f, 10.0f).withCentre ({ c.x + r * 0.85f, labelY }), Justification::centred);

    if (k.big && k.title.isNotEmpty())
    {
        // Angka romawi / judul di atas knob
        g.setFont (legendFont (k.title.length() > 3 ? 11.0f : 10.0f));
        g.drawText (k.title, Rectangle<float> (60.0f, 12.0f).withCentre ({ c.x - (k.title.length() > 3 ? 0.0f : r * 0.9f), c.y - r - 6.0f }),
                    Justification::centred);
    }
    else if (! k.big)
    {
        g.setFont (legendFont (9.0f));
        g.drawText (k.title, Rectangle<float> (70.0f, 12.0f).withCentre ({ c.x, labelY + 14.0f }), Justification::centred);
    }
}

void FrontPanel::paintBrand (Graphics& g, Rectangle<float> area) const
{
    const auto box = area.reduced (4.0f, 12.0f);
    auto x = box.getX() + 8.0f;

    g.setColour (palette::legendDim);
    g.setFont (legendFont (8.0f));
    g.drawText ("TRIVERSE", Rectangle<float> (x, box.getY() + 48.0f, 100.0f, 10.0f), Justification::centredLeft);

    // Logotype "TV-3"
    g.setColour (palette::legend);
    g.setFont (legendFont (30.0f));
    g.drawText ("TV-3", Rectangle<float> (x - 1.0f, box.getY() + 56.0f, 100.0f, 32.0f), Justification::centredLeft);

    // Tiga gelombang bergeser 120° — simbol tri-phase
    const float waveY = box.getY() + 100.0f;
    const float waveW = 76.0f;
    const Colour waveColours[] = { palette::cyan, palette::cyanLight, palette::cyan.withAlpha (0.6f) };
    for (int v = 0; v < 3; ++v)
    {
        Path wave;
        for (int i = 0; i <= 60; ++i)
        {
            const float t = (float) i / 60.0f;
            const float yy = waveY - 5.0f * std::sin (MathConstants<float>::twoPi * (1.5f * t + (float) v / 3.0f));
            if (i == 0) wave.startNewSubPath (x + t * waveW, yy);
            else        wave.lineTo (x + t * waveW, yy);
        }
        g.setColour (waveColours[v]);
        g.strokePath (wave, PathStrokeType (1.4f));
    }

    g.setColour (palette::legend);
    g.setFont (legendFont (7.5f));
    g.drawText ("TRI-PHASE CHORUS", Rectangle<float> (x, waveY + 10.0f, 100.0f, 10.0f), Justification::centredLeft);
}

void FrontPanel::paint (Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto face = bounds.reduced ((float) kEarWidth, 0.0f);

    // Plat depan: hitam semi-matte dengan gradasi tipis dan tekstur sapuan halus
    g.setGradientFill (ColourGradient (palette::panelTop, 0.0f, 0.0f, palette::panelBottom, 0.0f, bounds.getBottom(), false));
    g.fillRect (face);

    Random grain (1234);
    for (int i = 0; i < 900; ++i)
    {
        g.setColour (Colours::white.withAlpha (0.012f + grain.nextFloat() * 0.015f));
        const float yy = face.getY() + grain.nextFloat() * face.getHeight();
        const float xx = face.getX() + grain.nextFloat() * face.getWidth();
        g.fillRect (xx, yy, 6.0f + grain.nextFloat() * 30.0f, 0.6f);
    }

    // Bevel atas/bawah plat
    g.setColour (Colours::white.withAlpha (0.10f));
    g.drawHorizontalLine (0, face.getX(), face.getRight());
    g.setColour (Colours::black);
    g.drawHorizontalLine (getHeight() - 1, face.getX(), face.getRight());

    paintEar (g, bounds.withWidth ((float) kEarWidth));
    paintEar (g, bounds.withLeft (bounds.getRight() - (float) kEarWidth));

    // Sekrup sudut plat
    for (float sx : { face.getX() + 10.0f, face.getRight() - 10.0f })
        for (float sy : { 9.0f, bounds.getBottom() - 9.0f })
            paintScrew (g, { sx, sy }, 4.2f, sx * 0.03f + sy * 0.05f);

    paintBrand (g, brandArea.toFloat());
    paintSection (g, brandArea, {});
    paintSection (g, shapeArea, "LFO Shape");
    paintSection (g, voicingArea, "Voicing");
    paintSection (g, levelsArea, "Levels");
    paintSection (g, meterArea, "Out Level");
    paintSection (g, effectArea, "Effect");

    // Kotak Intensity/Delay/Rate tanpa garis vertikal pemisah, seperti di hardware
    paintSection (g, intensityArea, {});
    {
        const float x0 = (float) intensityArea.getX();
        const float y = 36.0f;
        g.setColour (palette::legend);
        g.setFont (legendFont (11.0f));
        g.drawText ("Intensity", Rectangle<float> (x0 + 16.0f, 18.0f, 220.0f, 14.0f), Justification::centred);

        // Braket di atas I–III
        Path bracket;
        bracket.startNewSubPath (x0 + 22.0f, y + 5.0f);
        bracket.lineTo (x0 + 22.0f, y);
        bracket.lineTo (x0 + 230.0f, y);
        bracket.lineTo (x0 + 230.0f, y + 5.0f);
        g.strokePath (bracket, PathStrokeType (1.0f));

        g.drawText ("Delay", Rectangle<float> (x0 + 254.0f, 18.0f, 60.0f, 14.0f), Justification::centred);
        g.drawText ("Rate",  Rectangle<float> (x0 + 328.0f, 18.0f, 60.0f, 14.0f), Justification::centred);
    }

    for (auto* k : { &int1, &int2, &int3, &delay, &rate, &tone, &character, &width, &input, &mix, &output })
        paintKnobDecor (g, *k);

    // Label tambahan
    g.setColour (palette::legend);
    g.setFont (legendFont (7.5f));
    g.drawText ("SELECT", Rectangle<float> ((float) shapeArea.getX(), 154.0f, (float) shapeArea.getWidth(), 10.0f), Justification::centred);
    g.drawText ("ON", Rectangle<float> ((float) effectArea.getX(), 128.0f, (float) effectArea.getWidth(), 10.0f), Justification::centred);

    g.setColour (palette::legendDim);
    g.setFont (legendFont (7.0f, false));
    g.drawText (String (CharPointer_UTF8 ("STEREO BBD CHORUS  \xc2\xb7  3 VOICE / 120\xc2\xb0")), // · dan °
                Rectangle<float> ((float) meterArea.getX(), 150.0f, (float) meterArea.getWidth(), 10.0f), Justification::centred);
}

//==============================================================================
TriChorusEditor::TriChorusEditor (TriChorusProcessor& p)
    : AudioProcessorEditor (p), chorusProcessor (p), panel (p)
{
    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (panel);

    constexpr double aspect = (double) FrontPanel::kWidth / FrontPanel::kHeight;
    setResizable (true, true);
    setResizeLimits (FrontPanel::kWidth * 7 / 10, FrontPanel::kHeight * 7 / 10,
                     FrontPanel::kWidth * 3 / 2,  FrontPanel::kHeight * 3 / 2);
    getConstrainer()->setFixedAspectRatio (aspect);
    setSize (FrontPanel::kWidth * 9 / 10, FrontPanel::kHeight * 9 / 10);

    startTimerHz (30);
}

TriChorusEditor::~TriChorusEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void TriChorusEditor::resized()
{
    const float scale = (float) getWidth() / (float) FrontPanel::kWidth;
    panel.setTransform (AffineTransform::scale (scale));
    panel.setBounds (0, 0, FrontPanel::kWidth, FrontPanel::kHeight);
}

void TriChorusEditor::timerCallback()
{
    // Balistik meter: naik instan, turun ~20 dB/detik.
    constexpr float fallPerTick = 20.0f / 30.0f;
    for (int ch = 0; ch < 2; ++ch)
    {
        const float peak = chorusProcessor.takePeak (ch);
        const float db = Decibels::gainToDecibels (peak, -100.0f);
        meterDb[(size_t) ch] = jmax (db, meterDb[(size_t) ch] - fallPerTick);
    }
    panel.getMeter().setLevels (meterDb[0], meterDb[1]);
}
