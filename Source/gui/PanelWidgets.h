#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

namespace trichorus::gui
{

// Tombol kotak merah bercahaya (gaya tombol power hardware).
// Toggle state = parameter bypass, jadi lampu menyala saat efek AKTIF (bypass off).
class EffectButton final : public juce::Button
{
public:
    EffectButton();
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
};

// Tombol bulat + tiga LED untuk memilih bentuk LFO (klik = ganti ke berikutnya).
class ShapeSelector final : public juce::Component,
                            public juce::SettableTooltipClient
{
public:
    explicit ShapeSelector (juce::RangedAudioParameter& shapeParam);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> buttonBounds() const;

    juce::ParameterAttachment attachment;
    int selected = 0;
    bool pressed = false;
};

// Dua bar LED horizontal dengan skala −20 … +6 dB.
class LevelMeter final : public juce::Component
{
public:
    static constexpr std::array<float, 13> kScale { -20.0f, -15.0f, -10.0f, -7.0f, -5.0f, -3.0f, -2.0f,
                                                    -1.0f, 0.0f, 1.0f, 2.0f, 3.0f, 6.0f };

    void setLevels (float dbLeft, float dbRight);
    void paint (juce::Graphics&) override;

private:
    void paintBar (juce::Graphics&, juce::Rectangle<float> window, float db) const;
    std::array<float, 2> levels { -100.0f, -100.0f };
};

} // namespace trichorus::gui
