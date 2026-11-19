#pragma once

#include "dsp/TriChorusEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

class TriChorusProcessor final : public juce::AudioProcessor
{
public:
    TriChorusProcessor();
    ~TriChorusProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.05; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorParameter* getBypassParameter() const override { return bypassParam; }

    juce::AudioProcessorValueTreeState& getState() noexcept { return state; }

    // Output peak since the GUI last read it (read via exchange).
    float takePeak (int channel) noexcept { return peak[(size_t) channel].exchange (0.0f); }

private:
    trichorus::dsp::TriChorusParams readParameters() const noexcept;

    juce::AudioProcessorValueTreeState state;
    trichorus::dsp::TriChorusEngine engine;

    std::atomic<float>* input = nullptr;
    std::atomic<float>* rate = nullptr;
    std::array<std::atomic<float>*, 3> intensity {};
    std::atomic<float>* delay = nullptr;
    std::atomic<float>* shape = nullptr;
    std::atomic<float>* width = nullptr;
    std::atomic<float>* tone = nullptr;
    std::atomic<float>* character = nullptr;
    std::atomic<float>* mix = nullptr;
    std::atomic<float>* output = nullptr;
    juce::AudioParameterBool* bypassParam = nullptr;

    // Click-free bypass crossfade.
    juce::SmoothedValue<float> bypassMix;
    juce::AudioBuffer<float> dryBuffer;

    std::array<std::atomic<float>, 2> peak {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriChorusProcessor)
};
