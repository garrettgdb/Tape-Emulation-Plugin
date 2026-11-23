#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    constexpr float pi = 3.14159265358979323846f;
    float smooth01 (float x) { x = juce::jlimit (0.0f, 1.0f, x); return x * x * (3.0f - 2.0f * x); }
}

Tape01AudioProcessor::Tape01AudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "TAPE01", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout Tape01AudioProcessor::createParameterLayout()
{
    using Float = juce::AudioParameterFloat;
    using Choice = juce::AudioParameterChoice;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto f = [&p] (const char* id, const char* name, float lo, float hi, float def, float step = 0.01f)
    {
        p.push_back (std::make_unique<Float> (id, name, juce::NormalisableRange<float> (lo, hi, step), def));
    };

    f ("input", "Input", -12.0f, 12.0f, 0.0f, 0.1f);
    f ("drive", "Drive", -12.0f, 18.0f, 3.0f, 0.1f);
    f ("saturation", "Saturation", 0.0f, 1.0f, 0.55f);
    f ("bias", "Bias", -1.0f, 1.0f, 0.0f);
    f ("compression", "Compression", 0.0f, 1.0f, 0.35f);
    f ("low", "Low", -6.0f, 6.0f, 0.0f, 0.1f);
    f ("high", "High", -6.0f, 6.0f, 0.0f, 0.1f);
    f ("wow", "Wow", 0.0f, 1.0f, 0.08f);
    f ("flutter", "Flutter", 0.0f, 1.0f, 0.05f);
    f ("mix", "Mix", 0.0f, 1.0f, 1.0f);
    f ("output", "Output", -18.0f, 12.0f, 0.0f, 0.1f);
    p.push_back (std::make_unique<juce::AudioParameterBool> ("bypass", "Bypass", false));
    p.push_back (std::make_unique<Choice> ("speed", "Tape Speed",
        juce::StringArray { "7.5 IPS", "15 IPS", "30 IPS" }, 1));
    p.push_back (std::make_unique<Choice> ("formula", "Tape Formula",
        juce::StringArray { "Vintage", "Studio", "Master" }, 0));
    p.push_back (std::make_unique<Choice> ("oversampling", "Oversampling",
        juce::StringArray { "1x", "2x", "4x" }, 1));
    return { p.begin(), p.end() };
}

void Tape01AudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    const int delaySize = juce::jmax (64, (int) std::ceil (sampleRate * 0.02));
    for (auto& line : delayLines) line.assign ((size_t) delaySize, 0.0f);
    writePositions = { 0, 0 };
    wowPhase = flutterPhase = 0.0f;
    lowState[0] = lowState[1] = 0.0f;
}

void Tape01AudioProcessor::releaseResources() {}

bool Tape01AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}

float Tape01AudioProcessor::readModulatedDelay (float input, int channel, float wowAmount, float flutterAmount)
{
    auto& line = delayLines[(size_t) channel];
    const int size = (int) line.size();
    if (size < 4) return input;

    const float wow = std::sin (wowPhase) * wowAmount * 0.0018f;
    const float flutter = std::sin (flutterPhase) * flutterAmount * 0.00045f;
    const float delaySamples = juce::jlimit (2.0f, (float) size - 3.0f,
        (float) currentSampleRate * (0.008f + wow + flutter));
    float readPos = (float) writePositions[(size_t) channel] - delaySamples;
    while (readPos < 0.0f) readPos += (float) size;
    const int i0 = (int) readPos;
    const int i1 = (i0 + 1) % size;
    const float frac = readPos - (float) i0;
    const float delayed = line[(size_t) i0] + frac * (line[(size_t) i1] - line[(size_t) i0]);

    line[(size_t) writePositions[(size_t) channel]] = input;
    writePositions[(size_t) channel] = (writePositions[(size_t) channel] + 1) % size;
    return delayed;
}

float Tape01AudioProcessor::processTapeSample (float input, int channel)
{
    const auto get = [this] (const char* id) { return parameters.getRawParameterValue (id)->load(); };
    const float inputGain = juce::Decibels::decibelsToGain (get ("input"));
    const float driveDb = get ("drive");
    const float drive = juce::Decibels::decibelsToGain (driveDb);
    const float saturation = get ("saturation");
    const float bias = get ("bias");
    const float compression = get ("compression");
    const int speed = (int) get ("speed");
    const int formula = (int) get ("formula");
    const float wow = get ("wow");
    const float flutter = get ("flutter");

    float x = input * inputGain;
    x = readModulatedDelay (x, channel, wow, flutter);

    // Tape-speed voicing: slower speeds retain more low-frequency weight and soften the top.
    const float speedTop = speed == 0 ? 0.78f : (speed == 1 ? 0.90f : 1.0f);
    const float cutoff = (float) (currentSampleRate * (speed == 0 ? 0.055 : speed == 1 ? 0.11 : 0.18));
    const float coeff = juce::jlimit (0.001f, 0.95f, cutoff / (float) currentSampleRate);
    lowState[channel] += coeff * (x - lowState[channel]);
    const float low = lowState[channel];
    const float high = x - low;
    x = low * (1.0f + get ("low") * 0.035f) + high * (speedTop + get ("high") * 0.025f);

    const float formulaDrive = formula == 0 ? 1.15f : (formula == 1 ? 1.0f : 0.88f);
    const float biasOffset = bias * 0.18f;
    const float driven = (x * drive * (1.0f + saturation * 2.4f) * formulaDrive) + biasOffset;

    // Smooth asymmetric transfer curve approximates magnetic saturation; it is not a circuit-level tape model.
    const float positive = std::tanh (driven * (1.0f + bias * 0.22f));
    const float negative = std::tanh (driven * (1.0f - bias * 0.22f));
    float saturated = 0.5f * (positive + negative);

    const float amount = smooth01 (compression);
    const float envelope = std::abs (saturated);
    const float compressed = saturated / (1.0f + amount * envelope * 1.7f);
    saturated = juce::jmap (amount, saturated, compressed);

    // Conservative makeup prevents the drive control from behaving like a simple volume boost.
    const float compensation = 1.0f / std::sqrt (1.0f + saturation * 1.8f);
    return saturated * compensation;
}

void Tape01AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = juce::jmin (2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();
    const float mix = parameters.getRawParameterValue ("mix")->load();
    const float outputGain = juce::Decibels::decibelsToGain (parameters.getRawParameterValue ("output")->load());
    const bool bypassed = parameters.getRawParameterValue ("bypass")->load() > 0.5f;

    float inPeak = 0.0f, outPeak = 0.0f;
    for (int ch = 0; ch < channels; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        for (int i = 0; i < samples; ++i)
        {
            const float dry = data[i];
            inPeak = juce::jmax (inPeak, std::abs (dry));
            const float wet = bypassed ? dry : processTapeSample (dry, ch);
            const float y = bypassed ? dry : juce::jmap (mix, dry, wet) * outputGain;
            data[i] = y;
            outPeak = juce::jmax (outPeak, std::abs (y));
        }
    }

    wowPhase += (float) (2.0 * pi * 0.32 / currentSampleRate) * (float) samples;
    flutterPhase += (float) (2.0 * pi * 6.1 / currentSampleRate) * (float) samples;
    if (wowPhase > 2.0f * pi) wowPhase = std::fmod (wowPhase, 2.0f * pi);
    if (flutterPhase > 2.0f * pi) flutterPhase = std::fmod (flutterPhase, 2.0f * pi);

    inputMeter.store (juce::jlimit (0.0f, 1.0f, inPeak));
    outputMeter.store (juce::jlimit (0.0f, 1.0f, outPeak));
}

juce::AudioProcessorEditor* Tape01AudioProcessor::createEditor()
{
    return new Tape01AudioProcessorEditor (*this);
}

void Tape01AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void Tape01AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Tape01AudioProcessor();
}
