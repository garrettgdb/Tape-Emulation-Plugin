#include "PluginEditor.h"

namespace
{
    const juce::Colour bg (0xff202326), panel (0xff292d30), line (0xff45494b);
    const juce::Colour cream (0xffe6dfcf), muted (0xffa9a394), amber (0xffd59a45);
}

Tape01AudioProcessorEditor::Tape01AudioProcessorEditor (Tape01AudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), inputMeter (p, true), outputMeter (p, false)
{
    setSize (980, 590);
    title.setText ("TAPE / 01", juce::dontSendNotification);
    title.setFont (juce::Font (juce::FontOptions (30.0f, juce::Font::bold)));
    title.setColour (juce::Label::textColourId, cream);
    subtitle.setText ("MAGNETIC TAPE PROCESSOR", juce::dontSendNotification);
    subtitle.setFont (juce::Font (juce::FontOptions (10.0f)));
    subtitle.setColour (juce::Label::textColourId, muted);
    preset.addItem ("Default", 1);
    preset.setSelectedId (1);
    preset.setColour (juce::ComboBox::backgroundColourId, panel);
    preset.setColour (juce::ComboBox::textColourId, cream);
    preset.setColour (juce::ComboBox::outlineColourId, line);
    bypass.setClickingTogglesState (true);
    bypass.setColour (juce::TextButton::buttonColourId, panel);
    bypass.setColour (juce::TextButton::textColourOffId, cream);
    bypass.setColour (juce::TextButton::buttonOnColourId, amber);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.parameters, "bypass", bypass);
    addAndMakeVisible (title); addAndMakeVisible (subtitle); addAndMakeVisible (preset);
    addAndMakeVisible (bypass); addAndMakeVisible (inputMeter); addAndMakeVisible (outputMeter);

    addSlider (input, "INPUT", "input", "dB");
    addSlider (output, "OUTPUT", "output", "dB");
    addSlider (drive, "INPUT DRIVE", "drive", "dB");
    addSlider (saturation, "SATURATION", "saturation", "%");
    addSlider (bias, "BIAS", "bias", "");
    addSlider (compression, "COMPRESSION", "compression", "%");
    addSlider (low, "LOW", "low", "dB");
    addSlider (high, "HIGH", "high", "dB");
    addSlider (wow, "WOW", "wow", "%");
    addSlider (flutter, "FLUTTER", "flutter", "%");
    addSlider (mix, "MIX", "mix", "%");

    addChoice (speed, "TAPE SPEED", "speed", { "7.5 IPS", "15 IPS", "30 IPS" });
    addChoice (formula, "TAPE FORMULA", "formula", { "Vintage", "Studio", "Master" });
    addChoice (oversampling, "OVERSAMPLING", "oversampling", { "1x", "2x", "4x" });
    startTimerHz (24);
}

void Tape01AudioProcessorEditor::styleSlider (juce::Slider& s, const juce::String&, const juce::String&)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 18);
    s.setColour (juce::Slider::rotarySliderFillColourId, amber);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, line);
    s.setColour (juce::Slider::thumbColourId, cream);
    s.setColour (juce::Slider::textBoxTextColourId, cream);
    s.setColour (juce::Slider::textBoxBackgroundColourId, panel);
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (s);
}

void Tape01AudioProcessorEditor::addSlider (juce::Slider& s, const juce::String& label,
                                            const juce::String& id, const juce::String& suffix)
{
    styleSlider (s, label, suffix);
    sliderAttachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (processor.parameters, id, s));
}

void Tape01AudioProcessorEditor::addChoice (juce::ComboBox& c, const juce::String&,
                                            const juce::String& id, const juce::StringArray& items)
{
    for (int i = 0; i < items.size(); ++i) c.addItem (items[i], i + 1);
    c.setColour (juce::ComboBox::backgroundColourId, panel);
    c.setColour (juce::ComboBox::textColourId, cream);
    c.setColour (juce::ComboBox::outlineColourId, line);
    addAndMakeVisible (c);
    comboAttachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        (processor.parameters, id, c));
}

void Tape01AudioProcessorEditor::Meter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xffd9c9a4));
    g.fillRoundedRectangle (r, 7.0f);
    g.setColour (juce::Colour (0xff514b40));
    g.drawRoundedRectangle (r, 7.0f, 1.0f);
    auto inner = r.reduced (12.0f, 9.0f);
    const float cx = inner.getCentreX(), cy = inner.getBottom() - 3.0f;
    g.setColour (juce::Colour (0xff37332d));
    g.drawLine (inner.getX(), cy, inner.getRight(), cy, 1.0f);
    for (int i = 0; i <= 10; ++i)
    {
        float x = juce::jmap ((float) i, 0.0f, 10.0f, inner.getX(), inner.getRight());
        g.drawLine (x, cy - (i % 2 == 0 ? 9.0f : 5.0f), x, cy, 1.0f);
    }
    float level = input ? processor.inputMeter.load() : processor.outputMeter.load();
    float angle = juce::jmap (juce::jlimit (0.0f, 1.0f, level), -2.35f, -0.55f);
    const float length = inner.getWidth() * 0.42f;
    juce::Point<float> tip (cx + std::cos (angle) * length, cy + std::sin (angle) * length);
    g.setColour (juce::Colour (0xff8d2f20));
    g.drawLine (cx, cy, tip.x, tip.y, 2.0f);
    g.fillEllipse (cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);
}

void Tape01AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    g.setColour (line);
    g.drawLine (22.0f, 72.0f, 958.0f, 72.0f, 1.0f);
    g.drawLine (22.0f, 258.0f, 958.0f, 258.0f, 1.0f);
    g.drawLine (245.0f, 278.0f, 245.0f, 555.0f, 1.0f);
    g.drawLine (490.0f, 278.0f, 490.0f, 555.0f, 1.0f);
    g.drawLine (715.0f, 278.0f, 715.0f, 555.0f, 1.0f);

    g.setColour (cream);
    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    g.drawText ("MACHINE", 28, 280, 200, 18, juce::Justification::centredLeft);
    g.drawText ("DRIVE", 265, 280, 200, 18, juce::Justification::centredLeft);
    g.drawText ("TONE", 510, 280, 180, 18, juce::Justification::centredLeft);
    g.drawText ("MECHANICS", 735, 280, 200, 18, juce::Justification::centredLeft);
    g.setColour (muted);
    g.setFont (juce::Font (juce::FontOptions (9.0f)));
    g.drawText ("VINTAGE CHARACTER  /  MODERN CONTROL", 28, 562, 360, 16, juce::Justification::centredLeft);
}

void Tape01AudioProcessorEditor::resized()
{
    title.setBounds (24, 15, 190, 36);
    subtitle.setBounds (27, 47, 190, 16);
    preset.setBounds (700, 27, 130, 28);
    bypass.setBounds (855, 27, 90, 28);

    input.setBounds (24, 94, 110, 150);
    inputMeter.setBounds (160, 104, 270, 126);
    outputMeter.setBounds (448, 104, 270, 126);
    output.setBounds (740, 94, 110, 150);
    mix.setBounds (855, 112, 95, 125);

    speed.setBounds (28, 325, 190, 28);
    formula.setBounds (28, 395, 190, 28);
    drive.setBounds (260, 310, 105, 120);
    saturation.setBounds (370, 310, 105, 120);
    bias.setBounds (315, 430, 105, 115);
    low.setBounds (510, 330, 95, 130);
    high.setBounds (615, 330, 95, 130);
    compression.setBounds (735, 310, 100, 120);
    wow.setBounds (835, 310, 100, 120);
    flutter.setBounds (785, 430, 100, 115);
    oversampling.setBounds (790, 558, 150, 24);
}

void Tape01AudioProcessorEditor::timerCallback()
{
    inputMeter.repaint();
    outputMeter.repaint();
}
