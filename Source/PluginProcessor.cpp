#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MySynthAudioProcessor::MySynthAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    for (int i = 0; i < numVoices; ++i)
        synth.addVoice (new SynthVoice());

    synth.addSound (new SynthSound());
}

juce::AudioProcessorValueTreeState::ParameterLayout MySynthAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        "waveform", "Waveform",
        juce::StringArray { "Sine", "Square", "Sawtooth", "Triangle" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "attack", "Attack",
        juce::NormalisableRange<float> (0.0f, 0.5f, 0.001f), 0.01f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "release", "Release",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.1f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "cutoff", "Filter Cutoff",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.3f), 20000.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "resonance", "Filter Resonance",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f), 0.707f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "gain", "Master Gain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));

    return { params.begin(), params.end() };
}

void MySynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = (juce::uint32) getTotalNumOutputChannels();

    lowPassFilter.prepare (spec);
    lowPassFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
            voice->prepare (spec);
}

bool MySynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MySynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Route the on-screen keyboard component's events into the MIDI stream
    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);

    // Push current parameter values into the voices and the shared filter.
    // The filter is applied ONCE to the mixed output of all voices below -
    // this is the paraphonic behaviour (single filter, many oscillators).
    auto waveform  = (int) apvts.getRawParameterValue ("waveform")->load();
    auto attack    = apvts.getRawParameterValue ("attack")->load();
    auto release   = apvts.getRawParameterValue ("release")->load();
    auto cutoff    = apvts.getRawParameterValue ("cutoff")->load();
    auto resonance = apvts.getRawParameterValue ("resonance")->load();
    auto gain      = apvts.getRawParameterValue ("gain")->load();

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
        {
            voice->setWaveform (waveform);
            voice->setEnvelopeParameters (attack, release);
        }
    }

    lowPassFilter.setCutoffFrequency (cutoff);
    lowPassFilter.setResonance (resonance);

    // Render all voices (mixed together) into the buffer
    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    // Apply the single shared low-pass filter to the mixed signal
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    lowPassFilter.process (context);

    buffer.applyGain (gain);
}

juce::AudioProcessorEditor* MySynthAudioProcessor::createEditor()
{
    return new MySynthAudioProcessorEditor (*this);
}

void MySynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        std::unique_ptr<juce::XmlElement> xml (state.createXml());
        copyXmlToBinary (*xml, destData);
    }
}

void MySynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
// This creates the actual plugin instance for each format the host loads.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MySynthAudioProcessor();
}
