#include "PluginProcessor.h"
#include "PluginEditor.h"

MySynthAudioProcessorEditor::MySynthAudioProcessorEditor (MySynthAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      keyboardComponent (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    // Waveform selector
    waveformBox.addItemList ({ "Sine", "Square", "Sawtooth", "Triangle" }, 1);
    addAndMakeVisible (waveformBox);
    waveformAttachment = std::make_unique<ComboBoxAttachment> (
        processor.apvts, "waveform", waveformBox);

    waveformLabel.setText ("Waveform", juce::dontSendNotification);
    addAndMakeVisible (waveformLabel);

    // Sliders bound directly to the APVTS parameters
    setupSlider (attackSlider, attackLabel, "Attack");
    attackAttachment = std::make_unique<SliderAttachment> (
        processor.apvts, "attack", attackSlider);

    setupSlider (releaseSlider, releaseLabel, "Release");
    releaseAttachment = std::make_unique<SliderAttachment> (
        processor.apvts, "release", releaseSlider);

    setupSlider (cutoffSlider, cutoffLabel, "Cutoff");
    cutoffAttachment = std::make_unique<SliderAttachment> (
        processor.apvts, "cutoff", cutoffSlider);

    setupSlider (resonanceSlider, resonanceLabel, "Resonance");
    resonanceAttachment = std::make_unique<SliderAttachment> (
        processor.apvts, "resonance", resonanceSlider);

    setupSlider (gainSlider, gainLabel, "Volume");
    gainAttachment = std::make_unique<SliderAttachment> (
        processor.apvts, "gain", gainSlider);

    // On-screen piano keyboard - drives the synth the same way MIDI input does
    addAndMakeVisible (keyboardComponent);
    keyboardComponent.setAvailableRange (36, 96); // C2 - C7
    keyboardComponent.setOctaveForMiddleC (4);

    setResizable (true, true);
    setSize (700, 480);
}

void MySynthAudioProcessorEditor::setupSlider (juce::Slider& slider, juce::Label& label,
                                                const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);
}

void MySynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawFittedText ("MySynth", getLocalBounds().removeFromTop (40),
                       juce::Justification::centred, 1);
}

void MySynthAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (40); // title space

    auto waveformArea = area.removeFromTop (50);
    waveformLabel.setBounds (waveformArea.removeFromLeft (80));
    waveformBox.setBounds (waveformArea.removeFromLeft (150).reduced (0, 10));

    area.removeFromTop (10);

    auto knobArea = area.removeFromTop (140);
    auto knobWidth = knobArea.getWidth() / 5;

    auto layoutKnob = [&] (juce::Slider& slider, juce::Label& label)
    {
        auto col = knobArea.removeFromLeft (knobWidth);
        label.setBounds (col.removeFromTop (20));
        slider.setBounds (col.reduced (10));
    };

    layoutKnob (attackSlider, attackLabel);
    layoutKnob (releaseSlider, releaseLabel);
    layoutKnob (cutoffSlider, cutoffLabel);
    layoutKnob (resonanceSlider, resonanceLabel);
    layoutKnob (gainSlider, gainLabel);

    area.removeFromTop (20);

    // Remaining space goes to the on-screen keyboard.
    keyboardComponent.setBounds (area);

    // Scale key width to fill the available width, clamped so keys stay
    // comfortably sized. Below minKeyWidth the component shows scroll
    // arrows instead of shrinking further; above maxKeyWidth, instead of
    // keys growing huge, MORE of the available range (set in the
    // constructor) becomes visible automatically.
    constexpr float minKeyWidth = 24.0f;
    constexpr float maxKeyWidth = 56.0f;
    constexpr int   referenceWhiteKeys = 14; // ~2 octaves, our "comfortable" baseline

    float keyWidth = (float) area.getWidth() / (float) referenceWhiteKeys;
    keyWidth = juce::jlimit (minKeyWidth, maxKeyWidth, keyWidth);
    keyboardComponent.setKeyWidth (keyWidth);
}

