#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "params/ParameterLayout.h"

namespace ids = trichorus::params::id;

TriChorusProcessor::TriChorusProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "TriVerseChorus", trichorus::params::createLayout())
{
    input        = state.getRawParameterValue (ids::input);
    rate         = state.getRawParameterValue (ids::rate);
    intensity[0] = state.getRawParameterValue (ids::int1);
    intensity[1] = state.getRawParameterValue (ids::int2);
    intensity[2] = state.getRawParameterValue (ids::int3);
    delay        = state.getRawParameterValue (ids::delay);
    shape        = state.getRawParameterValue (ids::shape);
    width        = state.getRawParameterValue (ids::width);
    tone         = state.getRawParameterValue (ids::tone);
    character    = state.getRawParameterValue (ids::character);
    mix          = state.getRawParameterValue (ids::mix);
    output       = state.getRawParameterValue (ids::output);
    bypassParam  = dynamic_cast<juce::AudioParameterBool*> (state.getParameter (ids::bypass));
    jassert (bypassParam != nullptr);
}

trichorus::dsp::TriChorusParams TriChorusProcessor::readParameters() const noexcept
{
    trichorus::dsp::TriChorusParams p;
    p.inputDb   = input->load();
    p.rateHz    = rate->load();
    for (size_t v = 0; v < 3; ++v)
        p.intensity[v] = intensity[v]->load() * 0.01f;
    p.delayMs   = delay->load();
    p.shape     = (trichorus::dsp::LfoShape) juce::jlimit (0, 2, (int) shape->load());
    p.width     = width->load() * 0.01f;
    p.toneHz    = tone->load();
    p.character = character->load() * 0.01f;
    p.mix       = mix->load() * 0.01f;
    p.outputDb  = output->load();
    return p;
}

void TriChorusProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    engine.setParameters (readParameters(), true);

    bypassMix.reset (sampleRate, 0.02);
    bypassMix.setCurrentAndTargetValue (bypassParam->get() ? 1.0f : 0.0f);
    dryBuffer.setSize (2, samplesPerBlock, false, false, true);
}

bool TriChorusProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo()
        && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

void TriChorusProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numInputs  = getTotalNumInputChannels();

    if (buffer.getNumChannels() < 2 || numSamples == 0)
        return;

    // Mono input → copy to the right channel.
    if (numInputs == 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);

    if (dryBuffer.getNumSamples() < numSamples)
        dryBuffer.setSize (2, numSamples, false, false, true);

    for (int ch = 0; ch < 2; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    engine.setParameters (readParameters());
    auto* left  = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);
    engine.process (left, right, left, right, numSamples);

    // Crossfade to the dry signal when bypassed.
    bypassMix.setTargetValue (bypassParam->get() ? 1.0f : 0.0f);
    if (bypassMix.isSmoothing() || bypassMix.getTargetValue() > 0.0f)
    {
        const auto* dryL = dryBuffer.getReadPointer (0);
        const auto* dryR = dryBuffer.getReadPointer (1);
        for (int n = 0; n < numSamples; ++n)
        {
            const float b = bypassMix.getNextValue();
            left[n]  += b * (dryL[n] - left[n]);
            right[n] += b * (dryR[n] - right[n]);
        }
    }

    for (int ch = 0; ch < 2; ++ch)
    {
        const float blockPeak = buffer.getMagnitude (ch, 0, numSamples);
        auto& p = peak[(size_t) ch];
        float prev = p.load();
        while (blockPeak > prev && ! p.compare_exchange_weak (prev, blockPeak)) {}
    }
}

juce::AudioProcessorEditor* TriChorusProcessor::createEditor()
{
    return new TriChorusEditor (*this);
}

void TriChorusProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void TriChorusProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TriChorusProcessor();
}
