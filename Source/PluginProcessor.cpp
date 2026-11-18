#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>

namespace
{
    // Soft knee above 0.95, then a hard cap at 1.0 in processBlock.
    float softClip(float x)
    {
        constexpr float t = 0.95f;
        const float ax = std::abs(x);
        if (ax <= t)
            return x;
        const float over = ax - t;
        return std::copysign(t + over / (1.0f + over), x);
    }
}

//==============================================================================
FamousBeatsTalkBoxProcessor::FamousBeatsTalkBoxProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "FamousBeatsTalkBox",
                 [] {
                     juce::AudioProcessorValueTreeState::ParameterLayout layout;

                     layout.add(std::make_unique<juce::AudioParameterBool>(
                         juce::ParameterID{ kBypassParam, 1 }, "Bypass", false));
                     layout.add(std::make_unique<juce::AudioParameterFloat>(
                         juce::ParameterID{ kOutputGainParam, 1 }, "Output Gain",
                         juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f, "dB"));

                     // Mode: Talk Box (MIDI synth, no audio in) vs
                     // Vocoder (mic in + MIDI carrier).
                     layout.add(std::make_unique<juce::AudioParameterChoice>(
                         juce::ParameterID{ kModeParam, 1 }, "Mode",
                         juce::StringArray{ "Talk Box", "Vocoder" }, 0));

                     // Carrier synth.
                     layout.add(std::make_unique<juce::AudioParameterFloat>(
                         juce::ParameterID{ kWaveBlendParam, 1 }, "Wave Blend",
                         juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.15f));
                     layout.add(std::make_unique<juce::AudioParameterFloat>(
                         juce::ParameterID{ kSubLevelParam, 1 }, "Sub Level",
                         juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.30f));
                     layout.add(std::make_unique<juce::AudioParameterFloat>(
                         juce::ParameterID{ kGlideParam, 1 }, "Glide",
                         juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.05f));
                     layout.add(std::make_unique<juce::AudioParameterChoice>(
                         juce::ParameterID{ kOctaveParam, 1 }, "Octave",
                         juce::StringArray{ "-2", "-1", "0", "+1", "+2" }, 2));
                     layout.add(std::make_unique<juce::AudioParameterBool>(
                         juce::ParameterID{ kMonoParam, 1 }, "Mono / Legato", false));

                     // Formant talk box.
                     layout.add(std::make_unique<juce::AudioParameterFloat>(
                         juce::ParameterID{ kVowelParam, 1 }, "Vowel",
                         juce::NormalisableRange<float>(0.0f, 4.0f, 0.01f), 1.0f));

                     // Auto-Talk LFO.
                     layout.add(std::make_unique<juce::AudioParameterBool>(
                         juce::ParameterID{ kAutoTalkOnParam, 1 }, "Auto-Talk", true));
                     juce::NormalisableRange<float> rateRange(0.05f, 8.0f, 0.01f);
                     rateRange.setSkewForCentre(1.0f);
                     layout.add(std::make_unique<juce::AudioParameterFloat>(
                         juce::ParameterID{ kAutoTalkRateParam, 1 }, "Auto-Talk Rate",
                         rateRange, 1.0f, "Hz"));
                     layout.add(std::make_unique<juce::AudioParameterFloat>(
                         juce::ParameterID{ kAutoTalkDepthParam, 1 }, "Auto-Talk Depth",
                         juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
                     layout.add(std::make_unique<juce::AudioParameterBool>(
                         juce::ParameterID{ kAutoTalkSyncParam, 1 }, "Tempo Sync", false));
                     layout.add(std::make_unique<juce::AudioParameterChoice>(
                         juce::ParameterID{ kAutoTalkDivisionParam, 1 }, "Sync Division",
                         juce::StringArray{ "1/4", "1/8", "1/16" }, 1));
                     layout.add(std::make_unique<juce::AudioParameterFloat>(
                         juce::ParameterID{ kAutoTalkBpmParam, 1 }, "Manual BPM",
                         juce::NormalisableRange<float>(40.0f, 240.0f, 1.0f), 120.0f, "BPM"));
                     layout.add(std::make_unique<juce::AudioParameterChoice>(
                         juce::ParameterID{ kAutoTalkShapeParam, 1 }, "Auto-Talk Shape",
                         juce::StringArray{ "Smooth", "Triangle", "Steps" }, 0));

                     // Drive + mix.
                     layout.add(std::make_unique<juce::AudioParameterFloat>(
                         juce::ParameterID{ kDriveParam, 1 }, "Drive",
                         juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f), 30.0f, "%"));
                     layout.add(std::make_unique<juce::AudioParameterFloat>(
                         juce::ParameterID{ kDriveMixParam, 1 }, "Drive Mix",
                         juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f), 80.0f, "%"));
                     layout.add(std::make_unique<juce::AudioParameterFloat>(
                         juce::ParameterID{ kMixParam, 1 }, "Mix",
                         juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f), 75.0f, "%"));

                     // Vocoder controls.
                     layout.add(std::make_unique<juce::AudioParameterFloat>(
                         juce::ParameterID{ kModLevelParam, 1 }, "Modulator Level",
                         juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));
                     layout.add(std::make_unique<juce::AudioParameterFloat>(
                         juce::ParameterID{ kCarrierLevelParam, 1 }, "Carrier Level",
                         juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.80f));
                     layout.add(std::make_unique<juce::AudioParameterFloat>(
                         juce::ParameterID{ kVocoderMixParam, 1 }, "Vocoder Mix",
                         juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f), 100.0f, "%"));

                     return layout;
                 }())
{
    auto cache = [this](const char* id) { return parameters.getRawParameterValue(id); };
    bypassParam = cache(kBypassParam);
    outputGainParam = cache(kOutputGainParam);
    modeParam = cache(kModeParam);
    waveBlendParam = cache(kWaveBlendParam);
    subLevelParam = cache(kSubLevelParam);
    glideParam = cache(kGlideParam);
    octaveParam = cache(kOctaveParam);
    monoParam = cache(kMonoParam);
    vowelParam = cache(kVowelParam);
    autoTalkOnParam = cache(kAutoTalkOnParam);
    autoTalkRateParam = cache(kAutoTalkRateParam);
    autoTalkDepthParam = cache(kAutoTalkDepthParam);
    autoTalkSyncParam = cache(kAutoTalkSyncParam);
    autoTalkDivisionParam = cache(kAutoTalkDivisionParam);
    autoTalkBpmParam = cache(kAutoTalkBpmParam);
    autoTalkShapeParam = cache(kAutoTalkShapeParam);
    driveParam = cache(kDriveParam);
    driveMixParam = cache(kDriveMixParam);
    mixParam = cache(kMixParam);
    modLevelParam = cache(kModLevelParam);
    carrierLevelParam = cache(kCarrierLevelParam);
    vocoderMixParam = cache(kVocoderMixParam);

    for (auto& f : formantFilters)
        f = std::make_unique<juce::dsp::IIR::Filter<float>>();
    for (auto& f : modFilters)
        f = std::make_unique<juce::dsp::IIR::Filter<float>>();
    for (auto& f : carrierFilters)
        f = std::make_unique<juce::dsp::IIR::Filter<float>>();

    applyPreset(0); // Roger
}

FamousBeatsTalkBoxProcessor::~FamousBeatsTalkBoxProcessor() = default;

void FamousBeatsTalkBoxProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sampleRateHz = sampleRate;
    if ((int) carrierScratch.size() < samplesPerBlock)
        carrierScratch.resize((size_t) samplesPerBlock);
    if ((int) modScratch.size() < samplesPerBlock)
        modScratch.resize((size_t) samplesPerBlock);

    for (auto& v : voices)
    {
        v.active = false;
        v.releasing = false;
        v.env = 0.0f;
    }
    monoNotes.clear();
    pitchBendSemitones = 0.0f;
    lfoPhase = 0.0;

    resetFormantState();
    resetVocoderState();

    // Vocoder band centres: log-spaced 100 Hz – 8 kHz.
    const double ratio = std::pow((double) kVocoderFreqMax / (double) kVocoderFreqMin,
                                  1.0 / (double) (kNumVocoderBands - 1));
    for (int i = 0; i < kNumVocoderBands; ++i)
    {
        vocoderBandCentres[i] = (float) (kVocoderFreqMin * std::pow(ratio, i));
        modFilters[i]->reset();
        carrierFilters[i]->reset();
        modFilters[i]->coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass(
            sampleRateHz, vocoderBandCentres[i], kVocoderBandQ);
        carrierFilters[i]->coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass(
            sampleRateHz, vocoderBandCentres[i], kVocoderBandQ);
    }

    // Dry delay line: long enough for the vocoder latency at any
    // supported sample rate (up to 192 kHz).
    const int delaySize = (int) std::ceil(kVocoderLatencySeconds * 192000.0) + (int) samplesPerBlock + 64;
    dryDelayLine.assign((size_t) delaySize, 0.0f);
    dryDelayPos = 0;

    lastMode = isVocoderMode() ? 1 : 0;
    modeFade = 1.0f;
    setLatencySamples(lastMode == 1 ? getVocoderLatencySamples() : 0);
}

void FamousBeatsTalkBoxProcessor::releaseResources() {}

bool FamousBeatsTalkBoxProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Mono or stereo in, mono or stereo out. The mic is usually mono;
    // the output is normally stereo (the vocoded signal is mono and
    // is written to both channels).
    const auto& in = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    if (in != juce::AudioChannelSet::stereo() && in != juce::AudioChannelSet::mono())
        return false;
    return true;
}

void FamousBeatsTalkBoxProcessor::resetVocoderState()
{
    for (auto& f : modFilters)
        if (f != nullptr)
            f->reset();
    for (auto& f : carrierFilters)
        if (f != nullptr)
            f->reset();
    for (int i = 0; i < kNumVocoderBands; ++i)
        bandEnvelopes[i] = 0.0f;
    modLevelSmooth = 0.0f;
    modGate = 0.0f;
    std::fill(dryDelayLine.begin(), dryDelayLine.end(), 0.0f);
    dryDelayPos = 0;
}

void FamousBeatsTalkBoxProcessor::resetFormantState()
{
    // Start the formant bank on the current vowel so there is no sweep
    // on the first note.
    const float v = juce::jlimit(0.0f, 4.0f, vowelParam->load());
    const int idx = v >= 4.0f ? 3 : (int) std::floor(v);
    const float frac = v >= 4.0f ? 1.0f : v - (float) idx;
    for (int i = 0; i < kNumFormants; ++i)
    {
        formantFreqs[i] = formantFreqTargets[i] =
            kVowelFormants[idx][i] + frac * (kVowelFormants[idx + 1][i] - kVowelFormants[idx][i]);
        formantGains[i] = formantGainTargets[i] =
            kVowelGains[idx][i] + frac * (kVowelGains[idx + 1][i] - kVowelGains[idx][i]);
        formantFilters[i]->reset();
        formantFilters[i]->coefficients =
            juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRateHz, formantFreqs[i], kFormantQ[i]);
    }
    currentVowel.store(v);
}

//==============================================================================
float FamousBeatsTalkBoxProcessor::polyBlep(float phase, float dt)
{
    if (phase < dt)
    {
        const float t = phase / dt;
        return t + t - t * t - 1.0f;
    }
    if (phase > 1.0f - dt)
    {
        const float t = (phase - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

float FamousBeatsTalkBoxProcessor::noteToFreq(int note, int octaveIndex, float bendSemitones)
{
    const float semis = (float) (note - 69) + (float) (octaveIndex - 2) * 12.0f + bendSemitones;
    return 440.0f * std::pow(2.0f, semis / 12.0f);
}

void FamousBeatsTalkBoxProcessor::noteOn(int note, float velocity)
{
    const int octaveIndex = (int) std::lround(octaveParam->load());
    const float glideTime = glideParam->load() * glideParam->load() * 0.5f;
    const float freq = noteToFreq(note, octaveIndex, pitchBendSemitones);
    const bool mono = monoParam->load() > 0.5f;

    if (mono)
    {
        if (std::find(monoNotes.begin(), monoNotes.end(), note) == monoNotes.end())
            monoNotes.push_back(note);

        Voice& v = voices[0];
        if (!v.active)
        {
            v.active = true;
            v.releasing = false;
            v.note = note;
            v.targetFreq = freq;
            v.velocity = velocity;
            v.env = 0.0f;
            v.age = 0;
            if (glideTime <= 0.0005f)
                v.currentFreq = freq;
        }
        else
        {
            // Legato: keep the envelope running, just retarget the pitch.
            v.note = note;
            v.targetFreq = freq;
            v.velocity = velocity;
            v.releasing = false;
        }
        return;
    }

    // Poly: retrigger the same note if it is already sounding.
    for (auto& v : voices)
    {
        if (v.active && !v.releasing && v.note == note)
        {
            v.env = 0.0f;
            v.velocity = velocity;
            v.targetFreq = freq;
            if (glideTime <= 0.0005f)
                v.currentFreq = freq;
            v.age = 0;
            return;
        }
    }

    Voice* slot = nullptr;
    for (auto& v : voices)
    {
        if (!v.active)
        {
            slot = &v;
            break;
        }
    }
    if (slot == nullptr)
    {
        // Steal the quietest voice.
        slot = &voices[0];
        for (auto& v : voices)
            if (v.env < slot->env)
                slot = &v;
    }

    slot->active = true;
    slot->releasing = false;
    slot->note = note;
    slot->targetFreq = freq;
    slot->velocity = velocity;
    slot->env = 0.0f;
    slot->age = 0;
    if (glideTime <= 0.0005f)
        slot->currentFreq = freq;
    // Otherwise glide from whatever pitch this voice was last on.
}

void FamousBeatsTalkBoxProcessor::noteOff(int note)
{
    const bool mono = monoParam->load() > 0.5f;

    if (mono)
    {
        monoNotes.erase(std::remove(monoNotes.begin(), monoNotes.end(), note), monoNotes.end());
        Voice& v = voices[0];
        if (monoNotes.empty())
        {
            if (v.active)
                v.releasing = true;
        }
        else
        {
            // Fall back to the most recently held note (legato).
            const int back = monoNotes.back();
            v.note = back;
            v.targetFreq = noteToFreq(back, (int) std::lround(octaveParam->load()), pitchBendSemitones);
            v.releasing = false;
        }
        return;
    }

    for (auto& v : voices)
    {
        if (v.active && !v.releasing && v.note == note)
        {
            v.releasing = true;
            return;
        }
    }
}

void FamousBeatsTalkBoxProcessor::allNotesOff()
{
    monoNotes.clear();
    for (auto& v : voices)
        if (v.active)
            v.releasing = true;
}

float FamousBeatsTalkBoxProcessor::renderVoiceSample(Voice& voice, float blend, float subLevel,
                                                     float glideCoeff, float attackCoeff,
                                                     float releaseCoeff, float bendFactor)
{
    // Amplitude envelope: fast attack, ~150 ms release.
    const float target = voice.releasing ? 0.0f : 1.0f;
    const float coeff = voice.releasing ? releaseCoeff : attackCoeff;
    voice.env += (target - voice.env) * coeff;
    if (voice.releasing && voice.env < 0.0005f)
    {
        voice.active = false;
        voice.env = 0.0f;
        return 0.0f;
    }

    // Glide: exponential slew toward the target pitch.
    voice.currentFreq += (voice.targetFreq - voice.currentFreq) * glideCoeff;
    float freq = voice.currentFreq * bendFactor;
    freq = juce::jmin(freq, (float) sampleRateHz * 0.45f); // stay below Nyquist
    const float dt = freq / (float) sampleRateHz;

    // Saw (polyBLEP).
    voice.phaseSaw += dt;
    if (voice.phaseSaw >= 1.0f)
        voice.phaseSaw -= 1.0f;
    const float saw = 2.0f * voice.phaseSaw - 1.0f - polyBlep(voice.phaseSaw, dt);

    // Square (polyBLEP on both edges).
    voice.phaseSquare += dt;
    if (voice.phaseSquare >= 1.0f)
        voice.phaseSquare -= 1.0f;
    float square = (voice.phaseSquare < 0.5f) ? 1.0f : -1.0f;
    square += polyBlep(voice.phaseSquare, dt);
    float edgePhase = voice.phaseSquare + 0.5f;
    if (edgePhase >= 1.0f)
        edgePhase -= 1.0f;
    square -= polyBlep(edgePhase, dt);

    // Sub oscillator: sine one octave down.
    voice.phaseSub += dt * 0.5f;
    if (voice.phaseSub >= 1.0f)
        voice.phaseSub -= 1.0f;
    const float sub = std::sin(2.0f * juce::MathConstants<float>::pi * voice.phaseSub);

    const float osc = saw * (1.0f - blend) + square * blend;
    return (osc + sub * subLevel * 0.7f) * voice.env * voice.velocity * 0.42f;
}

//==============================================================================
void FamousBeatsTalkBoxProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midiMessages)
{
    const int numSamples = buffer.getNumSamples();
    const int numInputChannels = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const int bufferChannels = buffer.getNumChannels();

    // ---- Capture the modulator (mic) input BEFORE clearing the buffer.
    // Talk Box mode ignores the input; Vocoder mode needs it.
    const bool vocoderMode = isVocoderMode();
    if ((int) modScratch.size() < numSamples)
        modScratch.resize((size_t) numSamples);
    if (vocoderMode && numInputChannels > 0)
    {
        const float* inL = buffer.getReadPointer(0);
        const float* inR = numInputChannels > 1 && bufferChannels > 1
            ? buffer.getReadPointer(1) : nullptr;
        for (int i = 0; i < numSamples; ++i)
            modScratch[(size_t) i] = inR != nullptr ? (inL[i] + inR[i]) * 0.5f : inL[i];
    }
    else
    {
        std::fill(modScratch.begin(), modScratch.begin() + numSamples, 0.0f);
    }

    buffer.clear();

    // Host tempo for tempo-synced Auto-Talk.
    if (auto* playHead = getPlayHead())
    {
        if (auto pos = playHead->getPosition())
        {
            if (auto bpm = pos->getBpm())
                hostBpm.store(*bpm);
            else
                hostBpm.store(0.0);
        }
    }

    // ---- MIDI ----
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
            noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
        else if (msg.isNoteOff())
            noteOff(msg.getNoteNumber());
        else if (msg.isPitchWheel())
            pitchBendSemitones = (msg.getPitchWheelValue() - 8192) / 8192.0f * 2.0f;
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            allNotesOff();
    }

    if (bypassParam->load() > 0.5f)
        return;

    if ((int) carrierScratch.size() < numSamples)
        carrierScratch.resize((size_t) numSamples);

    // ---- Mode change: reset both engines' state and fade back in ----
    // over ~10 ms so switching modes mid-stream never clicks.
    const int mode = vocoderMode ? 1 : 0;
    if (mode != lastMode)
    {
        lastMode = mode;
        resetVocoderState();
        resetFormantState();
        modeFade = 0.0f;
        setLatencySamples(mode == 1 ? getVocoderLatencySamples() : 0);
    }
    const float fadeStep = 1.0f / (0.010f * (float) sampleRateHz);

    // ---- Per-block parameters ----
    const float blend = waveBlendParam->load();
    const float subLevel = subLevelParam->load();
    const float glideTime = glideParam->load() * glideParam->load() * 0.5f;
    const float glideCoeff = glideTime > 0.0005f
        ? 1.0f - std::exp(-1.0f / (glideTime * (float) sampleRateHz)) : 1.0f;
    const float attackCoeff = 1.0f - std::exp(-1.0f / (0.005f * (float) sampleRateHz));
    const float releaseCoeff = 1.0f - std::exp(-1.0f / (0.15f * (float) sampleRateHz));
    const float bendFactor = std::pow(2.0f, pitchBendSemitones / 12.0f);

    const float drive = driveParam->load();
    const float driveMix = driveMixParam->load() * 0.01f;
    const float driveK = 1.0f + drive * 0.04f; // 1..5
    const float driveNorm = 1.0f / std::tanh(driveK);
    const float mix = mixParam->load() * 0.01f;
    const float outGainLin = std::pow(10.0f, outputGainParam->load() * 0.05f);

    // ---- Auto-Talk LFO -> vowel position (Talk Box mode only) ----
    const bool autoTalkOn = autoTalkOnParam->load() > 0.5f;
    const float depth = autoTalkDepthParam->load();
    const int shape = (int) std::lround(autoTalkShapeParam->load());
    if (autoTalkOn && mode == 0)
    {
        lfoPhase += getAutoTalkRateHz() / sampleRateHz * (double) numSamples;
        lfoPhase = std::fmod(lfoPhase, 1.0);
    }
    float lfo = 0.0f;
    if (autoTalkOn && mode == 0)
    {
        switch (shape)
        {
            case 1: // Triangle
                lfo = 2.0f * std::abs(2.0f * (float) (lfoPhase - std::floor(lfoPhase + 0.5))) - 1.0f;
                break;
            case 2: // Steps (square)
                lfo = lfoPhase < 0.5 ? 1.0f : -1.0f;
                break;
            default: // Smooth (sine)
                lfo = std::sin(2.0f * juce::MathConstants<float>::pi * (float) lfoPhase);
                break;
        }
    }
    const float vowelPos = juce::jlimit(0.0f, 4.0f,
        vowelParam->load() + (autoTalkOn && mode == 0 ? depth * 2.0f * lfo : 0.0f));
    currentVowel.store(vowelPos);

    // Formant targets from the vowel tables (linear interp between
    // adjacent vowels), then smooth toward them (~30 ms) so the
    // coefficients never jump.
    const int vIdx = vowelPos >= 4.0f ? 3 : (int) std::floor(vowelPos);
    const float vFrac = vowelPos >= 4.0f ? 1.0f : vowelPos - (float) vIdx;
    for (int i = 0; i < kNumFormants; ++i)
    {
        formantFreqTargets[i] = kVowelFormants[vIdx][i]
            + vFrac * (kVowelFormants[vIdx + 1][i] - kVowelFormants[vIdx][i]);
        formantGainTargets[i] = kVowelGains[vIdx][i]
            + vFrac * (kVowelGains[vIdx + 1][i] - kVowelGains[vIdx][i]);
    }
    const float smooth = 1.0f - std::exp(-(float) numSamples / (0.03f * (float) sampleRateHz));
    for (int i = 0; i < kNumFormants; ++i)
    {
        formantFreqs[i] += (formantFreqTargets[i] - formantFreqs[i]) * smooth;
        formantGains[i] += (formantGainTargets[i] - formantGains[i]) * smooth;
        formantFilters[i]->coefficients =
            juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRateHz, formantFreqs[i], kFormantQ[i]);
    }

    // ---- 1. Carrier synth (mono sum of all voices) ----
    for (int i = 0; i < numSamples; ++i)
    {
        float carrier = 0.0f;
        for (auto& v : voices)
        {
            if (v.active)
            {
                carrier += renderVoiceSample(v, blend, subLevel, glideCoeff,
                                             attackCoeff, releaseCoeff, bendFactor);
                ++v.age;
            }
        }
        carrierScratch[(size_t) i] = carrier;
    }

    // ---- Vocoder per-block parameters ----
    const float modLevel = modLevelParam->load();
    const float carrierLevel = carrierLevelParam->load();
    const float vocoderMix = vocoderMixParam->load() * 0.01f;
    // Envelope follower: ~2 ms attack (tracks consonants), ~80 ms
    // release (no chatter).
    const float envAttackCoeff = 1.0f - std::exp(-1.0f / (0.002f * (float) sampleRateHz));
    const float envReleaseCoeff = 1.0f - std::exp(-1.0f / (0.080f * (float) sampleRateHz));
    // Mic noise gate: ~20 ms level follower, ~5 ms gate smoothing,
    // opens above about -54 dBFS so room noise never opens the bands.
    const float gateLevelCoeff = 1.0f - std::exp(-1.0f / (0.020f * (float) sampleRateHz));
    const float gateSmoothCoeff = 1.0f - std::exp(-1.0f / (0.005f * (float) sampleRateHz));
    // Gain compensation for the bank (each band passes a fraction of
    // the carrier; the envelopes are < 1 for typical mic levels).
    constexpr float vocoderGainComp = 2.0f;
    const int latencySamples = getVocoderLatencySamples();
    const int delaySize = (int) dryDelayLine.size();

    // ---- 2. Drive -> 3. Talk box formants / Vocoder bank -> Output ----
    float* outL = buffer.getWritePointer(0);
    float* outR = bufferChannels > 1 ? buffer.getWritePointer(1) : nullptr;
    const int numOut = juce::jmin(numOutputChannels, bufferChannels);
    for (int i = 0; i < numSamples; ++i)
    {
        const float raw = carrierScratch[(size_t) i];

        // Drive: tanh grit, normalised so full drive stays near unity.
        const float driven = std::tanh(raw * driveK) * driveNorm;
        const float drivenMix = raw * (1.0f - driveMix) + driven * driveMix;

        float out = 0.0f;
        if (mode == 0)
        {
            // ---- Talk Box: 4 bandpass formants summed with vowel gains ----
            float formantOut = 0.0f;
            for (int j = 0; j < kNumFormants; ++j)
                formantOut += formantFilters[j]->processSample(drivenMix) * formantGains[j];
            formantOut *= 0.65f; // global formant gain
            out = raw * (1.0f - mix) + formantOut * mix;
        }
        else
        {
            // ---- Vocoder: 16-band channel vocoder ----
            // Modulator (mic), mono, at the Mod Level.
            const float modMono = modScratch[(size_t) i] * modLevel;

            // Noise gate on the modulator.
            modLevelSmooth += (std::abs(modMono) - modLevelSmooth) * gateLevelCoeff;
            const float gateTarget = modLevelSmooth > 0.002f ? 1.0f : 0.0f;
            modGate += (gateTarget - modGate) * gateSmoothCoeff;

            // Carrier into the bank, at the Carrier Level.
            const float carrierIn = drivenMix * carrierLevel;

            float wet = 0.0f;
            for (int b = 0; b < kNumVocoderBands; ++b)
            {
                const float modBand = modFilters[b]->processSample(modMono);
                float env = bandEnvelopes[b];
                const float mag = std::abs(modBand);
                const float coeff = mag > env ? envAttackCoeff : envReleaseCoeff;
                env += (mag - env) * coeff;
                bandEnvelopes[b] = env;

                const float carrierBand = carrierFilters[b]->processSample(carrierIn);
                wet += carrierBand * env * modGate;
            }
            wet *= vocoderGainComp;

            // Dry path: the mic delayed by the vocoder latency so the
            // dry/wet mix stays time-aligned.
            float dry = 0.0f;
            if (delaySize > 0)
            {
                dryDelayLine[(size_t) dryDelayPos] = modMono;
                const int readPos = (dryDelayPos - latencySamples + delaySize * 2) % delaySize;
                dry = dryDelayLine[(size_t) readPos];
                dryDelayPos = (dryDelayPos + 1) % delaySize;
            }

            out = dry * (1.0f - vocoderMix) + wet * vocoderMix;
        }

        out *= outGainLin;
        out = juce::jlimit(-1.0f, 1.0f, softClip(out));

        // Short fade after a mode switch so it never clicks.
        if (modeFade < 1.0f)
        {
            modeFade = juce::jmin(1.0f, modeFade + fadeStep);
            out *= modeFade;
        }

        outL[i] = out;
        if (numOut > 1 && outR != nullptr)
            outR[i] = out;
    }
}

//==============================================================================
juce::AudioProcessorEditor* FamousBeatsTalkBoxProcessor::createEditor()
{
    return new PluginEditor(*this);
}

void FamousBeatsTalkBoxProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream(destData, true);
    parameters.state.writeToStream(stream);
}

void FamousBeatsTalkBoxProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
        parameters.replaceState(tree);
}

//==============================================================================
double FamousBeatsTalkBoxProcessor::divisionToBeats(int divisionIndex)
{
    switch (divisionIndex)
    {
        case 0: return 1.0;  // 1/4
        case 1: return 0.5;  // 1/8
        case 2: return 0.25; // 1/16
        default: return 0.5;
    }
}

double FamousBeatsTalkBoxProcessor::getAutoTalkRateHz() const
{
    if (autoTalkSyncParam->load() > 0.5f)
    {
        const double bpm = hostBpm.load() > 0.0 ? hostBpm.load() : (double) autoTalkBpmParam->load();
        return (bpm / 60.0) * divisionToBeats((int) std::lround(autoTalkDivisionParam->load()));
    }
    return (double) autoTalkRateParam->load();
}

void FamousBeatsTalkBoxProcessor::getFormantFreqs(float* freqs, int n) const
{
    for (int i = 0; i < n && i < kNumFormants; ++i)
        freqs[i] = formantFreqs[i];
}

void FamousBeatsTalkBoxProcessor::getFormantGains(float* gains, int n) const
{
    for (int i = 0; i < n && i < kNumFormants; ++i)
        gains[i] = formantGains[i];
}

void FamousBeatsTalkBoxProcessor::getVocoderBandLevels(float* levels, int n) const
{
    for (int i = 0; i < n && i < kNumVocoderBands; ++i)
        levels[i] = juce::jlimit(0.0f, 1.0f, bandEnvelopes[i] * 4.0f);
}

void FamousBeatsTalkBoxProcessor::getVocoderBandFreqs(float* freqs, int n) const
{
    for (int i = 0; i < n && i < kNumVocoderBands; ++i)
        freqs[i] = vocoderBandCentres[i];
}

//==============================================================================
void FamousBeatsTalkBoxProcessor::setParamValue(const char* paramId, float value)
{
    auto* param = parameters.getParameter(paramId);
    if (param != nullptr)
        param->setValueNotifyingHost(param->convertTo0to1(value));
}

const char* FamousBeatsTalkBoxProcessor::getPresetName(int presetIndex)
{
    switch (presetIndex)
    {
        case 0: return "Roger";
        case 1: return "G-Funk";
        case 2: return "Robot";
        case 3: return "Talk Wah";
        case 4: return "Clean Talk";
        case 5: return "Vocoder";
        default: return "";
    }
}

void FamousBeatsTalkBoxProcessor::applyPreset(int presetIndex)
{
    currentPreset = juce::jlimit(0, kNumPresets - 1, presetIndex);

    // Defaults shared by every preset.
    setParamValue(kOctaveParam, 2.0f);
    setParamValue(kAutoTalkBpmParam, 120.0f);
    setParamValue(kAutoTalkDivisionParam, 1.0f);
    setParamValue(kOutputGainParam, 0.0f);
    setParamValue(kModeParam, 0.0f); // Talk Box, unless the preset says otherwise

    switch (currentPreset)
    {
        case 0: // Roger — classic Zapp: saw carrier, vowel morphing, moderate drive.
            setParamValue(kWaveBlendParam, 0.15f);
            setParamValue(kSubLevelParam, 0.25f);
            setParamValue(kGlideParam, 0.05f);
            setParamValue(kMonoParam, 0.0f);
            setParamValue(kVowelParam, 1.5f);
            setParamValue(kAutoTalkOnParam, 1.0f);
            setParamValue(kAutoTalkRateParam, 0.8f);
            setParamValue(kAutoTalkDepthParam, 0.55f);
            setParamValue(kAutoTalkSyncParam, 0.0f);
            setParamValue(kAutoTalkShapeParam, 0.0f);
            setParamValue(kDriveParam, 30.0f);
            setParamValue(kDriveMixParam, 80.0f);
            setParamValue(kMixParam, 75.0f);
            break;

        case 1: // G-Funk — slower auto-talk, deeper drive, sub up.
            setParamValue(kWaveBlendParam, 0.10f);
            setParamValue(kSubLevelParam, 0.60f);
            setParamValue(kGlideParam, 0.12f);
            setParamValue(kMonoParam, 0.0f);
            setParamValue(kVowelParam, 2.0f);
            setParamValue(kAutoTalkOnParam, 1.0f);
            setParamValue(kAutoTalkRateParam, 0.4f);
            setParamValue(kAutoTalkDepthParam, 0.70f);
            setParamValue(kAutoTalkSyncParam, 0.0f);
            setParamValue(kAutoTalkShapeParam, 0.0f);
            setParamValue(kDriveParam, 55.0f);
            setParamValue(kDriveMixParam, 90.0f);
            setParamValue(kMixParam, 85.0f);
            break;

        case 2: // Robot — square carrier, fast stepped auto-talk, mono.
            setParamValue(kWaveBlendParam, 0.90f);
            setParamValue(kSubLevelParam, 0.30f);
            setParamValue(kGlideParam, 0.0f);
            setParamValue(kMonoParam, 1.0f);
            setParamValue(kVowelParam, 1.0f);
            setParamValue(kAutoTalkOnParam, 1.0f);
            setParamValue(kAutoTalkRateParam, 4.0f);
            setParamValue(kAutoTalkDepthParam, 1.0f);
            setParamValue(kAutoTalkSyncParam, 0.0f);
            setParamValue(kAutoTalkShapeParam, 2.0f);
            setParamValue(kDriveParam, 45.0f);
            setParamValue(kDriveMixParam, 85.0f);
            setParamValue(kMixParam, 90.0f);
            break;

        case 3: // Talk Wah — manual vowel, wah-like morph, no auto-talk.
            setParamValue(kWaveBlendParam, 0.30f);
            setParamValue(kSubLevelParam, 0.20f);
            setParamValue(kGlideParam, 0.08f);
            setParamValue(kMonoParam, 0.0f);
            setParamValue(kVowelParam, 2.0f);
            setParamValue(kAutoTalkOnParam, 0.0f);
            setParamValue(kAutoTalkRateParam, 1.0f);
            setParamValue(kAutoTalkDepthParam, 0.50f);
            setParamValue(kAutoTalkSyncParam, 0.0f);
            setParamValue(kAutoTalkShapeParam, 0.0f);
            setParamValue(kDriveParam, 25.0f);
            setParamValue(kDriveMixParam, 80.0f);
            setParamValue(kMixParam, 80.0f);
            break;

        case 4: // Clean Talk — gentle, mostly vowels, light drive.
            setParamValue(kWaveBlendParam, 0.20f);
            setParamValue(kSubLevelParam, 0.15f);
            setParamValue(kGlideParam, 0.03f);
            setParamValue(kMonoParam, 0.0f);
            setParamValue(kVowelParam, 1.5f);
            setParamValue(kAutoTalkOnParam, 1.0f);
            setParamValue(kAutoTalkRateParam, 0.5f);
            setParamValue(kAutoTalkDepthParam, 0.35f);
            setParamValue(kAutoTalkSyncParam, 0.0f);
            setParamValue(kAutoTalkShapeParam, 0.0f);
            setParamValue(kDriveParam, 12.0f);
            setParamValue(kDriveMixParam, 70.0f);
            setParamValue(kMixParam, 60.0f);
            break;

        case 5: // Vocoder — sing into the mic, play the keys.
        default:
            setParamValue(kModeParam, 1.0f);
            setParamValue(kWaveBlendParam, 0.10f);
            setParamValue(kSubLevelParam, 0.30f);
            setParamValue(kGlideParam, 0.05f);
            setParamValue(kMonoParam, 0.0f);
            setParamValue(kDriveParam, 20.0f);
            setParamValue(kDriveMixParam, 50.0f);
            setParamValue(kMixParam, 75.0f); // unused in vocoder mode
            setParamValue(kModLevelParam, 1.0f);
            setParamValue(kCarrierLevelParam, 0.80f);
            setParamValue(kVocoderMixParam, 100.0f);
            break;
    }
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FamousBeatsTalkBoxProcessor();
}
