#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Gold, shiny LookAndFeel: metallic gold gradients, gloss highlights,
// black text on gold, white text on black. (Same Famous Beats Studio
// look as the Vocal Presets v2 interface.)
class GoldLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GoldLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;
    juce::Font getTextButtonFont(juce::TextButton& button, int buttonHeight) override
    {
        // Per-button size via a "fontSize" property; default scales with height.
        const auto& props = button.getProperties();
        const float size = props.contains("fontSize")
            ? (float)(double)props["fontSize"]
            : juce::jmin(14.0f, buttonHeight * 0.4f);
        return juce::Font(juce::FontOptions(size, juce::Font::bold));
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool isMouseOverButton, bool isButtonDown) override;
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    static juce::ColourGradient goldGradient(float x0, float y0, float x1, float y1);
    static juce::ColourGradient goldRadial(float cx, float cy, float radius);

    static const juce::Colour gold;
    static const juce::Colour goldLight;
    static const juce::Colour goldDark;
    static const juce::Colour black;
    static const juce::Colour panel;
    static const juce::Colour cream;
};

//==============================================================================
// Big vowel display: the current vowel as a huge gold letter, with the
// four live formant peaks drawn as gold bars on a log-frequency strip
// (100 Hz – 8 kHz) underneath it.
class VowelDisplayComponent : public juce::Component, private juce::Timer
{
public:
    explicit VowelDisplayComponent(FamousBeatsTalkBoxProcessor& p);
    ~VowelDisplayComponent() override = default;

    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override { repaint(); }
    float freqToX(float freq) const;

    FamousBeatsTalkBoxProcessor& processor;
};

//==============================================================================
// Black-and-gold editor: vowel display up top, vowel buttons + Morph
// knob, then one panel per section (Carrier / Auto-Talk / Drive & Mix),
// five big preset buttons, and a hint line. Five presets: Roger /
// G-Funk / Robot / Talk Wah / Clean Talk.
class PluginEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PluginEditor(FamousBeatsTalkBoxProcessor&);
    ~PluginEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    void makeKnob(juce::Slider& knob, juce::Label& label, const juce::String& text,
                  const char* paramId, int knobSize = 64);
    void makeCombo(juce::ComboBox& box, juce::Label& label, const juce::String& text,
                   const char* paramId);
    void makeSectionLabel(juce::Label& label, const juce::String& text);
    void makePresetButton(juce::TextButton& button, const juce::String& text, int presetIndex);
    void makeVowelButton(juce::TextButton& button, const juce::String& text, int vowelIndex);
    void selectPreset(int presetIndex);
    void setVowel(int vowelIndex);

    FamousBeatsTalkBoxProcessor& processor;
    GoldLookAndFeel goldLaf;

    juce::Label titleLabel;
    juce::Label brandLabel;
    juce::Label hintLabel;

    VowelDisplayComponent vowelDisplay;

    // Vowel buttons + Morph.
    juce::TextButton vowelButtons[5];
    juce::Slider morphKnob;  juce::Label morphLabel;

    // Section labels.
    juce::Label carrierSection, autoTalkSection, driveSection;

    // Carrier.
    juce::Slider waveBlendKnob; juce::Label waveBlendLabel;
    juce::Slider subKnob;       juce::Label subLabel;
    juce::Slider glideKnob;     juce::Label glideLabel;
    juce::ComboBox octaveBox;   juce::Label octaveLabel;
    juce::TextButton monoButton;

    // Auto-Talk.
    juce::TextButton autoTalkButton;
    juce::Slider talkRateKnob;  juce::Label talkRateLabel;
    juce::Slider talkDepthKnob; juce::Label talkDepthLabel;
    juce::TextButton talkSyncButton;
    juce::ComboBox talkDivisionBox; juce::Label talkDivisionLabel;
    juce::Slider talkBpmKnob;   juce::Label talkBpmLabel;
    juce::ComboBox talkShapeBox; juce::Label talkShapeLabel;

    // Drive & Mix.
    juce::Slider driveKnob;     juce::Label driveLabel;
    juce::Slider driveMixKnob;  juce::Label driveMixLabel;
    juce::Slider mixKnob;       juce::Label mixLabel;
    juce::Slider outputKnob;    juce::Label outputLabel;
    juce::TextButton bypassButton;

    // Presets.
    juce::TextButton presetButtons[5];

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
