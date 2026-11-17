#include "PluginEditor.h"

namespace
{
    // Layout (editor is 1000 x 620).
    constexpr int kMargin = 12;
    constexpr int kWidth = 1000;
    constexpr int kSectionsTop = 248;
    constexpr int kSectionH = 260;

    juce::Rectangle<int> carrierPanel()   { return { kMargin, kSectionsTop, 317, kSectionH }; }
    juce::Rectangle<int> autoTalkPanel()  { return { 341, kSectionsTop, 317, kSectionH }; }
    juce::Rectangle<int> drivePanel()     { return { 670, kSectionsTop, 318, kSectionH }; }

    const juce::Colour panelFill(0xff141210);
    const juce::Colour panelEdge(0xff6b5a26);
}

//==============================================================================
// GoldLookAndFeel
const juce::Colour GoldLookAndFeel::gold(0xffd4af37);
const juce::Colour GoldLookAndFeel::goldLight(0xfff6d47c);
const juce::Colour GoldLookAndFeel::goldDark(0xff9a7b1e);
const juce::Colour GoldLookAndFeel::black(0xff0a0a0a);
const juce::Colour GoldLookAndFeel::panel(0xff141414);
const juce::Colour GoldLookAndFeel::cream(0xfff5e6c8);

GoldLookAndFeel::GoldLookAndFeel()
{
    setColour(juce::Slider::rotarySliderFillColourId, gold);
    setColour(juce::Slider::rotarySliderOutlineColourId, goldDark);
    setColour(juce::Slider::thumbColourId, gold);
    setColour(juce::Slider::textBoxTextColourId, cream);
    setColour(juce::Slider::textBoxBackgroundColourId, panel);
    setColour(juce::Slider::textBoxOutlineColourId, goldDark);
    setColour(juce::ComboBox::backgroundColourId, panel);
    setColour(juce::ComboBox::textColourId, cream);
    setColour(juce::ComboBox::arrowColourId, gold);
    setColour(juce::ComboBox::outlineColourId, goldDark);
    setColour(juce::PopupMenu::backgroundColourId, panel);
    setColour(juce::PopupMenu::textColourId, cream);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, gold);
    setColour(juce::PopupMenu::highlightedTextColourId, black);
    setColour(juce::TextButton::buttonColourId, panel);
    setColour(juce::TextButton::buttonOnColourId, gold);
    setColour(juce::TextButton::textColourOffId, gold);
    setColour(juce::TextButton::textColourOnId, black);
}

juce::ColourGradient GoldLookAndFeel::goldGradient(float x0, float y0, float x1, float y1)
{
    juce::ColourGradient g(goldLight, x0, y0, goldDark, x1, y1, false);
    g.addColour(0.5, gold);
    return g;
}

juce::ColourGradient GoldLookAndFeel::goldRadial(float cx, float cy, float radius)
{
    juce::ColourGradient g(goldLight, cx - radius * 0.4f, cy - radius * 0.5f,
                           goldDark, cx + radius * 0.5f, cy + radius * 0.6f, true);
    g.addColour(0.55, gold);
    return g;
}

void GoldLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float rotaryStartAngle,
                                       float rotaryEndAngle, juce::Slider&)
{
    const float radius = juce::jmin(width / 2.0f, height / 2.0f) - 4.0f;
    const float centreX = x + width * 0.5f;
    const float centreY = y + height * 0.5f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float rw = radius * 2.0f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Background arc.
    juce::Path bgArc;
    bgArc.addArc(rx, ry, rw, rw, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(goldDark.withAlpha(0.4f));
    g.strokePath(bgArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));

    // Gold value arc.
    juce::Path valueArc;
    valueArc.addArc(rx, ry, rw, rw, rotaryStartAngle, angle, true);
    g.setColour(gold);
    g.strokePath(valueArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Metallic knob body.
    const float bodyR = radius - 7.0f;
    g.setGradientFill(goldRadial(centreX, centreY, bodyR));
    g.fillEllipse(centreX - bodyR, centreY - bodyR, bodyR * 2.0f, bodyR * 2.0f);

    // Gloss highlight.
    g.setColour(juce::Colours::white.withAlpha(0.38f));
    g.fillEllipse(centreX - bodyR * 0.55f, centreY - bodyR * 0.78f,
                  bodyR * 0.7f, bodyR * 0.45f);

    // Pointer line.
    juce::Path pointer;
    const float pointerLength = bodyR * 0.8f;
    pointer.addRectangle(-1.5f, -bodyR, 3.0f, pointerLength);
    g.setColour(black);
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centreX, centreY));

    // Rim.
    g.setColour(goldDark);
    g.drawEllipse(centreX - bodyR, centreY - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);
}

void GoldLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                           const juce::Colour& /*backgroundColour*/,
                                           bool isMouseOverButton, bool isButtonDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    const float corner = 6.0f;
    const bool isOn = button.getToggleState();

    if (isOn)
    {
        // Shiny gold.
        g.setGradientFill(goldGradient(bounds.getX(), bounds.getY(),
                                       bounds.getX(), bounds.getBottom()));
        g.fillRoundedRectangle(bounds, corner);
        g.setColour(juce::Colours::white.withAlpha(0.4f));
        g.fillRoundedRectangle(bounds.getX(), bounds.getY(),
                               bounds.getWidth(), bounds.getHeight() * 0.45f, corner);
    }
    else
    {
        juce::ColourGradient grad(juce::Colour(0xff1c1a14), bounds.getX(), bounds.getY(),
                                  black, bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(bounds, corner);
        g.setColour(isMouseOverButton ? gold : goldDark);
        g.drawRoundedRectangle(bounds, corner, isMouseOverButton ? 2.0f : 1.0f);
    }

    if (isButtonDown)
    {
        g.setColour(juce::Colours::black.withAlpha(0.25f));
        g.fillRoundedRectangle(bounds, corner);
    }
}

void GoldLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                   int buttonX, int buttonY, int buttonW, int buttonH,
                                   juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();
    g.setColour(panel);
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(box.hasKeyboardFocus(true) ? gold : goldDark);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    juce::Path arrow;
    const float ax = buttonX + buttonW * 0.5f;
    const float ay = buttonY + buttonH * 0.5f;
    arrow.addTriangle(ax - 5.0f, ay - 2.0f, ax + 5.0f, ay - 2.0f, ax, ay + 4.0f);
    g.setColour(gold);
    g.fillPath(arrow);
}

//==============================================================================
// VowelDisplayComponent
VowelDisplayComponent::VowelDisplayComponent(FamousBeatsTalkBoxProcessor& p)
    : processor(p)
{
    startTimerHz(15);
}

float VowelDisplayComponent::freqToX(float freq) const
{
    const double l = std::log10(juce::jlimit(100.0f, 8000.0f, freq));
    const double l0 = std::log10(100.0), l1 = std::log10(8000.0);
    const double t = (l - l0) / (l1 - l0);
    return 30.0f + (float) t * ((float) getWidth() - 60.0f);
}

void VowelDisplayComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(GoldLookAndFeel::panel);
    g.fillRoundedRectangle(bounds, 8.0f);
    g.setColour(GoldLookAndFeel::goldDark);
    g.drawRoundedRectangle(bounds, 8.0f, 1.5f);

    const float h = (float) getHeight();
    const float baseY = h - 18.0f;

    // Frequency gridlines.
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    for (float f : { 200.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f })
    {
        const float x = freqToX(f);
        g.setColour(GoldLookAndFeel::goldDark.withAlpha(0.35f));
        g.drawVerticalLine((int) x, 8.0f, baseY);
        g.setColour(GoldLookAndFeel::cream.withAlpha(0.55f));
        g.drawText(f >= 1000.0f ? juce::String(f / 1000.0f, 1) + "k" : juce::String((int) f),
                   (int) x - 20, (int) baseY + 2, 40, 14, juce::Justification::centred);
    }

    // Live formant bars.
    float freqs[4] = {};
    float gains[4] = {};
    processor.getFormantFreqs(freqs, 4);
    processor.getFormantGains(gains, 4);
    for (int i = 0; i < 4; ++i)
    {
        const float x = freqToX(freqs[i]);
        const float barH = juce::jlimit(4.0f, baseY - 14.0f, gains[i] * (baseY - 20.0f));
        juce::Rectangle<float> bar(x - 13.0f, baseY - barH, 26.0f, barH);
        g.setGradientFill(GoldLookAndFeel::goldGradient(bar.getX(), bar.getY(),
                                                        bar.getX(), bar.getBottom()));
        g.fillRoundedRectangle(bar, 4.0f);
        g.setColour(juce::Colours::white.withAlpha(0.35f));
        g.fillRoundedRectangle(bar.getX(), bar.getY(), bar.getWidth(), bar.getHeight() * 0.3f, 4.0f);
    }

    // Huge current vowel letter.
    const float v = processor.getCurrentVowel();
    const int nearest = juce::jlimit(0, 4, (int) std::lround(v));
    g.setFont(juce::Font(juce::FontOptions(72.0f, juce::Font::bold)));
    g.setColour(GoldLookAndFeel::gold.withAlpha(0.92f));
    g.drawText(juce::String(FamousBeatsTalkBoxProcessor::kVowelNames[nearest]),
               bounds, juce::Justification::centred, false);

    // Morph position readout (e.g. "E -> I" while between vowels).
    const float frac = v - std::floor(v);
    juce::String posText;
    if (frac > 0.06f && frac < 0.94f && nearest < 4)
        posText = juce::String(FamousBeatsTalkBoxProcessor::kVowelNames[(int) std::floor(v)])
            + " -> " + juce::String(FamousBeatsTalkBoxProcessor::kVowelNames[(int) std::floor(v) + 1]);
    else
        posText = juce::String(FamousBeatsTalkBoxProcessor::kVowelNames[nearest]);
    g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    g.setColour(GoldLookAndFeel::cream);
    g.drawText(posText, (int) bounds.getX() + 10, 8, 90, 16, juce::Justification::left);

    g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    g.setColour(GoldLookAndFeel::gold);
    g.drawText("VOWEL", (int) bounds.getRight() - 70, 8, 60, 16, juce::Justification::right);
}

//==============================================================================
// PluginEditor
PluginEditor::PluginEditor(FamousBeatsTalkBoxProcessor& p)
    : AudioProcessorEditor(p), processor(p), vowelDisplay(p)
{
    setLookAndFeel(&goldLaf);
    setSize(kWidth, 620);

    titleLabel.setText("FAMOUS BEATS TALK BOX", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(28.0f, juce::Font::bold)));
    titleLabel.setColour(juce::Label::textColourId, GoldLookAndFeel::gold);
    addAndMakeVisible(titleLabel);

    brandLabel.setText("FAMOUS BEATS STUDIO", juce::dontSendNotification);
    brandLabel.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    brandLabel.setColour(juce::Label::textColourId, GoldLookAndFeel::cream);
    brandLabel.setJustificationType(juce::Justification::right);
    addAndMakeVisible(brandLabel);

    addAndMakeVisible(vowelDisplay);

    for (int i = 0; i < 5; ++i)
    {
        makeVowelButton(vowelButtons[i],
                        juce::String(FamousBeatsTalkBoxProcessor::kVowelNames[i]), i);
        makePresetButton(presetButtons[i],
                         juce::String(FamousBeatsTalkBoxProcessor::getPresetName(i)), i);
    }

    makeKnob(morphKnob, morphLabel, "MORPH",
             FamousBeatsTalkBoxProcessor::kVowelParam, 52);

    makeSectionLabel(carrierSection, "CARRIER");
    makeSectionLabel(autoTalkSection, "AUTO-TALK");
    makeSectionLabel(driveSection, "DRIVE & MIX");

    // Carrier.
    makeKnob(waveBlendKnob, waveBlendLabel, "WAVE BLEND",
             FamousBeatsTalkBoxProcessor::kWaveBlendParam);
    makeKnob(subKnob, subLabel, "SUB",
             FamousBeatsTalkBoxProcessor::kSubLevelParam);
    makeKnob(glideKnob, glideLabel, "GLIDE",
             FamousBeatsTalkBoxProcessor::kGlideParam);
    makeCombo(octaveBox, octaveLabel, "OCTAVE",
              FamousBeatsTalkBoxProcessor::kOctaveParam);
    octaveBox.addItemList(juce::StringArray(FamousBeatsTalkBoxProcessor::kOctaveNames, 5), 1);
    monoButton.setButtonText("MONO");
    monoButton.setClickingTogglesState(true);
    monoButton.getProperties().set("fontSize", 13.0);
    addAndMakeVisible(monoButton);

    // Auto-Talk.
    autoTalkButton.setButtonText("AUTO-TALK");
    autoTalkButton.setClickingTogglesState(true);
    autoTalkButton.getProperties().set("fontSize", 12.0);
    addAndMakeVisible(autoTalkButton);
    makeKnob(talkRateKnob, talkRateLabel, "RATE",
             FamousBeatsTalkBoxProcessor::kAutoTalkRateParam);
    makeKnob(talkDepthKnob, talkDepthLabel, "DEPTH",
             FamousBeatsTalkBoxProcessor::kAutoTalkDepthParam);
    talkSyncButton.setButtonText("SYNC");
    talkSyncButton.setClickingTogglesState(true);
    talkSyncButton.getProperties().set("fontSize", 12.0);
    addAndMakeVisible(talkSyncButton);
    makeCombo(talkDivisionBox, talkDivisionLabel, "DIV",
              FamousBeatsTalkBoxProcessor::kAutoTalkDivisionParam);
    talkDivisionBox.addItemList(
        juce::StringArray(FamousBeatsTalkBoxProcessor::kAutoTalkDivisionNames, 3), 1);
    makeKnob(talkBpmKnob, talkBpmLabel, "BPM",
             FamousBeatsTalkBoxProcessor::kAutoTalkBpmParam, 56);
    makeCombo(talkShapeBox, talkShapeLabel, "SHAPE",
              FamousBeatsTalkBoxProcessor::kAutoTalkShapeParam);
    talkShapeBox.addItemList(
        juce::StringArray(FamousBeatsTalkBoxProcessor::kAutoTalkShapeNames, 3), 1);

    // Drive & Mix.
    makeKnob(driveKnob, driveLabel, "DRIVE",
             FamousBeatsTalkBoxProcessor::kDriveParam, 60);
    makeKnob(driveMixKnob, driveMixLabel, "DRIVE MIX",
             FamousBeatsTalkBoxProcessor::kDriveMixParam, 60);
    makeKnob(mixKnob, mixLabel, "MIX",
             FamousBeatsTalkBoxProcessor::kMixParam, 60);
    makeKnob(outputKnob, outputLabel, "OUTPUT",
             FamousBeatsTalkBoxProcessor::kOutputGainParam, 60);
    bypassButton.setButtonText("BYPASS");
    bypassButton.setClickingTogglesState(true);
    bypassButton.getProperties().set("fontSize", 13.0);
    addAndMakeVisible(bypassButton);

    hintLabel.setText("Play it from your MIDI keyboard — keys and talk box in one plugin. "
                      "No audio input needed.",
                      juce::dontSendNotification);
    hintLabel.setFont(juce::Font(juce::FontOptions(13.0f)));
    hintLabel.setColour(juce::Label::textColourId, GoldLookAndFeel::cream);
    hintLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(hintLabel);

    // Attachments.
    auto& state = processor.getParametersState();
    auto attachSlider = [&state, this](juce::Slider& s, const char* id) {
        sliderAttachments.push_back(std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(state, id, s));
    };
    auto attachCombo = [&state, this](juce::ComboBox& b, const char* id) {
        comboAttachments.push_back(std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(state, id, b));
    };
    auto attachButton = [&state, this](juce::Button& b, const char* id) {
        buttonAttachments.push_back(std::make_unique<
            juce::AudioProcessorValueTreeState::ButtonAttachment>(state, id, b));
    };

    attachSlider(morphKnob, FamousBeatsTalkBoxProcessor::kVowelParam);
    attachSlider(waveBlendKnob, FamousBeatsTalkBoxProcessor::kWaveBlendParam);
    attachSlider(subKnob, FamousBeatsTalkBoxProcessor::kSubLevelParam);
    attachSlider(glideKnob, FamousBeatsTalkBoxProcessor::kGlideParam);
    attachSlider(talkRateKnob, FamousBeatsTalkBoxProcessor::kAutoTalkRateParam);
    attachSlider(talkDepthKnob, FamousBeatsTalkBoxProcessor::kAutoTalkDepthParam);
    attachSlider(talkBpmKnob, FamousBeatsTalkBoxProcessor::kAutoTalkBpmParam);
    attachSlider(driveKnob, FamousBeatsTalkBoxProcessor::kDriveParam);
    attachSlider(driveMixKnob, FamousBeatsTalkBoxProcessor::kDriveMixParam);
    attachSlider(mixKnob, FamousBeatsTalkBoxProcessor::kMixParam);
    attachSlider(outputKnob, FamousBeatsTalkBoxProcessor::kOutputGainParam);
    attachCombo(octaveBox, FamousBeatsTalkBoxProcessor::kOctaveParam);
    attachCombo(talkDivisionBox, FamousBeatsTalkBoxProcessor::kAutoTalkDivisionParam);
    attachCombo(talkShapeBox, FamousBeatsTalkBoxProcessor::kAutoTalkShapeParam);
    attachButton(monoButton, FamousBeatsTalkBoxProcessor::kMonoParam);
    attachButton(autoTalkButton, FamousBeatsTalkBoxProcessor::kAutoTalkOnParam);
    attachButton(talkSyncButton, FamousBeatsTalkBoxProcessor::kAutoTalkSyncParam);
    attachButton(bypassButton, FamousBeatsTalkBoxProcessor::kBypassParam);

    selectPreset(processor.getCurrentPreset());
    startTimerHz(15);
}

void PluginEditor::timerCallback()
{
    const int nearest = juce::jlimit(0, 4, (int) std::lround(processor.getCurrentVowel()));
    for (int i = 0; i < 5; ++i)
        vowelButtons[i].setToggleState(i == nearest, juce::dontSendNotification);
    for (int i = 0; i < 5; ++i)
        presetButtons[i].setToggleState(i == processor.getCurrentPreset(),
                                        juce::dontSendNotification);
}

void PluginEditor::makeKnob(juce::Slider& knob, juce::Label& label, const juce::String& text,
                            const char* paramId, int knobSize)
{
    juce::ignoreUnused(paramId);
    knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    knob.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(knob);
    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    label.setColour(juce::Label::textColourId, GoldLookAndFeel::gold);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
    juce::ignoreUnused(knobSize);
}

void PluginEditor::makeCombo(juce::ComboBox& box, juce::Label& label, const juce::String& text,
                             const char* paramId)
{
    juce::ignoreUnused(paramId);
    addAndMakeVisible(box);
    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    label.setColour(juce::Label::textColourId, GoldLookAndFeel::gold);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
}

void PluginEditor::makeSectionLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    label.setColour(juce::Label::textColourId, GoldLookAndFeel::gold);
    addAndMakeVisible(label);
}

void PluginEditor::makePresetButton(juce::TextButton& button, const juce::String& text, int presetIndex)
{
    button.setButtonText(text);
    button.setClickingTogglesState(true);
    button.getProperties().set("fontSize", 14.0);
    button.onClick = [this, presetIndex] { selectPreset(presetIndex); };
    addAndMakeVisible(button);
}

void PluginEditor::makeVowelButton(juce::TextButton& button, const juce::String& text, int vowelIndex)
{
    button.setButtonText(text);
    button.setClickingTogglesState(true);
    button.getProperties().set("fontSize", 16.0);
    button.onClick = [this, vowelIndex] { setVowel(vowelIndex); };
    addAndMakeVisible(button);
}

void PluginEditor::selectPreset(int presetIndex)
{
    processor.applyPreset(presetIndex);
    for (int i = 0; i < 5; ++i)
        presetButtons[i].setToggleState(i == presetIndex, juce::dontSendNotification);
}

void PluginEditor::setVowel(int vowelIndex)
{
    auto* param = processor.getParametersState().getParameter(
        FamousBeatsTalkBoxProcessor::kVowelParam);
    if (param != nullptr)
        param->setValueNotifyingHost(param->convertTo0to1((float) vowelIndex));
}

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(GoldLookAndFeel::black);

    // Section panels.
    for (auto panel : { carrierPanel(), autoTalkPanel(), drivePanel() })
    {
        g.setColour(panelFill);
        g.fillRoundedRectangle(panel.toFloat(), 8.0f);
        g.setColour(panelEdge);
        g.drawRoundedRectangle(panel.toFloat(), 8.0f, 1.5f);
    }
}

void PluginEditor::resized()
{
    titleLabel.setBounds(kMargin, 8, 560, 40);
    brandLabel.setBounds(kWidth - 300, 14, 288, 28);

    vowelDisplay.setBounds(kMargin, 56, kWidth - 2 * kMargin, 120);

    // Vowel buttons + Morph knob row.
    for (int i = 0; i < 5; ++i)
        vowelButtons[i].setBounds(kMargin + i * 96, 194, 88, 36);
    morphKnob.setBounds(580, 184, 56, 56);
    morphLabel.setBounds(640, 200, 90, 24);
    morphLabel.setJustificationType(juce::Justification::left);

    // Section labels.
    carrierSection.setBounds(carrierPanel().getX() + 12, kSectionsTop + 8, 200, 20);
    autoTalkSection.setBounds(autoTalkPanel().getX() + 12, kSectionsTop + 8, 200, 20);
    driveSection.setBounds(drivePanel().getX() + 12, kSectionsTop + 8, 200, 20);

    // Carrier panel: 3 knobs, then Octave combo + Mono button.
    auto placeKnob = [](juce::Slider& knob, juce::Label& label, int x, int y, int size) {
        knob.setBounds(x, y, size, size);
        label.setBounds(x - 14, y + size + 2, size + 28, 16);
    };
    placeKnob(waveBlendKnob, waveBlendLabel, carrierPanel().getX() + 16, kSectionsTop + 36, 64);
    placeKnob(subKnob, subLabel, carrierPanel().getX() + 116, kSectionsTop + 36, 64);
    placeKnob(glideKnob, glideLabel, carrierPanel().getX() + 216, kSectionsTop + 36, 64);
    octaveLabel.setBounds(carrierPanel().getX() + 16, kSectionsTop + 158, 110, 16);
    octaveBox.setBounds(carrierPanel().getX() + 16, kSectionsTop + 176, 110, 26);
    monoButton.setBounds(carrierPanel().getX() + 150, kSectionsTop + 174, 110, 30);

    // Auto-Talk panel: On/Off button + Rate + Depth knobs, then Sync / Div / BPM / Shape.
    autoTalkButton.setBounds(autoTalkPanel().getX() + 14, kSectionsTop + 48, 96, 34);
    placeKnob(talkRateKnob, talkRateLabel, autoTalkPanel().getX() + 122, kSectionsTop + 36, 64);
    placeKnob(talkDepthKnob, talkDepthLabel, autoTalkPanel().getX() + 216, kSectionsTop + 36, 64);
    talkSyncButton.setBounds(autoTalkPanel().getX() + 14, kSectionsTop + 176, 76, 30);
    talkDivisionLabel.setBounds(autoTalkPanel().getX() + 100, kSectionsTop + 158, 62, 16);
    talkDivisionBox.setBounds(autoTalkPanel().getX() + 100, kSectionsTop + 176, 62, 26);
    talkBpmKnob.setBounds(autoTalkPanel().getX() + 172, kSectionsTop + 150, 56, 56);
    talkBpmLabel.setBounds(autoTalkPanel().getX() + 158, kSectionsTop + 206, 84, 16);
    talkShapeLabel.setBounds(autoTalkPanel().getX() + 240, kSectionsTop + 158, 66, 16);
    talkShapeBox.setBounds(autoTalkPanel().getX() + 240, kSectionsTop + 176, 66, 26);

    // Drive & Mix panel: 4 knobs, then Bypass.
    placeKnob(driveKnob, driveLabel, drivePanel().getX() + 8, kSectionsTop + 36, 60);
    placeKnob(driveMixKnob, driveMixLabel, drivePanel().getX() + 84, kSectionsTop + 36, 60);
    placeKnob(mixKnob, mixLabel, drivePanel().getX() + 160, kSectionsTop + 36, 60);
    placeKnob(outputKnob, outputLabel, drivePanel().getX() + 236, kSectionsTop + 36, 60);
    bypassButton.setBounds(drivePanel().getX() + 99, kSectionsTop + 174, 120, 32);

    // Presets row.
    for (int i = 0; i < 5; ++i)
        presetButtons[i].setBounds(kMargin + i * 197, 516, 187, 44);

    hintLabel.setBounds(kMargin, 568, kWidth - 2 * kMargin, 30);
}
