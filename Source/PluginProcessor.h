#pragma once

#include <JuceHeader.h>

//==============================================================================
// A single playable "note" - mirrors the oscillator + gain-envelope pairing
// from the original Web Audio implementation (one per active MIDI note).
//==============================================================================
class SynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override        { return true; }
    bool appliesToChannel (int) override     { return true; }
};

class SynthVoice : public juce::SynthesiserVoice
{
public:
    SynthVoice() = default;

    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<SynthSound*> (sound) != nullptr;
    }

    void setWaveform (int waveformIndex) { currentWaveform = waveformIndex; }

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        oscillator.prepare (spec);
        adsr.setSampleRate (spec.sampleRate);
    }

    void setEnvelopeParameters (float attackSeconds, float releaseSeconds)
    {
        auto params = adsr.getParameters();
        params.attack  = attackSeconds;
        params.decay   = 0.001f;
        params.sustain = 1.0f;
        params.release = releaseSeconds;
        adsr.setParameters (params);
    }

    void startNote (int midiNoteNumber, float velocity,
                     juce::SynthesiserSound*, int /*pitchWheelPos*/) override
    {
        auto freq = (float) juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        oscillator.setFrequency (freq);
        level = velocity;
        adsr.noteOn();
    }

    void stopNote (float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            adsr.noteOff();
        }
        else
        {
            clearCurrentNote();
            adsr.reset();
        }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                           int startSample, int numSamples) override
    {
        if (! isVoiceActive() && ! adsr.isActive())
            return;

        tempBlock.setSize (1, numSamples, false, false, true);
        tempBlock.clear();

        juce::dsp::AudioBlock<float> block (tempBlock);
        juce::dsp::ProcessContextReplacing<float> context (block);

        setOscillatorType();
        oscillator.process (context);
        adsr.applyEnvelopeToBuffer (tempBlock, 0, numSamples);

        for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
            outputBuffer.addFrom (ch, startSample, tempBlock, 0, 0, numSamples, level);

        if (! adsr.isActive())
            clearCurrentNote();
    }

private:
    void setOscillatorType()
    {
        switch (currentWaveform)
        {
            case 0: oscillator.initialise ([] (float x) { return std::sin (x); }); break;                         // sine
            case 1: oscillator.initialise ([] (float x) { return x < 0.0f ? -1.0f : 1.0f; }); break;               // square
            case 2: oscillator.initialise ([] (float x) { return x / juce::MathConstants<float>::pi; }); break;    // sawtooth
            case 3: oscillator.initialise ([] (float x)                                                            // triangle
                    {
                        return (2.0f / juce::MathConstants<float>::pi)
                             * std::asin (std::sin (x));
                    }); break;
            default: oscillator.initialise ([] (float x) { return std::sin (x); }); break;
        }
    }

    juce::dsp::Oscillator<float> oscillator { [] (float x) { return std::sin (x); } };
    juce::ADSR adsr;
    juce::AudioBuffer<float> tempBlock;
    int currentWaveform = 0;
    float level = 1.0f;
};

//==============================================================================
// Main plugin processor - owns the Synthesiser (polyphonic voices) and the
// single shared low-pass filter the mixed output passes through, matching
// the paraphonic architecture of the original Web Audio prototype.
//==============================================================================
class MySynthAudioProcessor : public juce::AudioProcessor
{
public:
    MySynthAudioProcessor();
    ~MySynthAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override                        { return 1; }
    int getCurrentProgram() override                     { return 0; }
    void setCurrentProgram (int) override                {}
    const juce::String getProgramName (int) override     { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    // Parameters exposed to the host DAW (and bound to on-screen sliders)
    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Lets the editor drive the on-screen keyboard into the MIDI stream
    juce::MidiKeyboardState keyboardState;

private:
    juce::Synthesiser synth;
    juce::dsp::StateVariableTPTFilter<float> lowPassFilter;

    static constexpr int numVoices = 8;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MySynthAudioProcessor)
};
