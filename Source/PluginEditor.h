#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class Tape01AudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    explicit Tape01AudioProcessorEditor (Tape01AudioProcessor&);
    ~Tape01AudioProcessorEditor() override = default;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class Meter : public juce::Component
    {
    public:
        Meter (Tape01AudioProcessor& p, bool isInput) : processor (p), input (isInput) {}
        void paint (juce::Graphics& g) override;
    private:
        Tape01AudioProcessor& processor;
        bool input;
    };

    void timerCallback() override;
    void styleSlider (juce::Slider&, const juce::String&, const juce::String&);
    void addSlider (juce::Slider&, const juce::String&, const juce::String&, const juce::String&);
    void addChoice (juce::ComboBox&, const juce::String&, const juce::String&, const juce::StringArray&);

    Tape01AudioProcessor& processor;
    juce::Label title, subtitle, presetLabel;
    juce::ComboBox preset;
    juce::TextButton bypass { "BYPASS" };
    Meter inputMeter, outputMeter;

    juce::Slider input, output, drive, saturation, bias, compression, low, high, wow, flutter, mix;
    juce::ComboBox speed, formula, oversampling;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Tape01AudioProcessorEditor)
};
