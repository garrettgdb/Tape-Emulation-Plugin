#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>

class Tape01AudioProcessor : public juce::AudioProcessor
{
public:
    Tape01AudioProcessor();
    ~Tape01AudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float> inputMeter { 0.0f };
    std::atomic<float> outputMeter { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    float processTapeSample (float input, int channel);
    float readModulatedDelay (float input, int channel, float wowAmount, float flutterAmount);

    double currentSampleRate = 44100.0;
    std::array<std::vector<float>, 2> delayLines;
    std::array<int, 2> writePositions { 0, 0 };
    float wowPhase = 0.0f;
    float flutterPhase = 0.0f;
    float lowState[2] { 0.0f, 0.0f };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Tape01AudioProcessor)
};
