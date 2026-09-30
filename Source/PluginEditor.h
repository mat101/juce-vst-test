#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class MySynthAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MySynthAudioProcessorEditor (MySynthAudioProcessor&);
    ~MySynthAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    MySynthAudioProcessor& processor;

    juce::ComboBox waveformBox;
    juce::Slider attackSlider, releaseSlider, cutoffSlider, resonanceSlider, gainSlider;
    juce::Label waveformLabel, attackLabel, releaseLabel, cutoffLabel, resonanceLabel, gainLabel;

    juce::MidiKeyboardComponent keyboardComponent;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ComboBoxAttachment> waveformAttachment;
    std::unique_ptr<SliderAttachment> attackAttachment, releaseAttachment,
                                       cutoffAttachment, resonanceAttachment, gainAttachment;

    void setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MySynthAudioProcessorEditor)
};
