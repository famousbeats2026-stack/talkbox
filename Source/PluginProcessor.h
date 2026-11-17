#pragma once

// Explicit module includes (rather than the generated JuceHeader.h) so
// the test harness can build the processor without the plugin client's
// generated headers.
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>
#include <atomic>
#include <cmath>

//==============================================================================
// Famous Beats Talk Box — a playable MIDI talk box in a single plugin:
//
//   MIDI -> Carrier synth (saw/square + sub, poly or mono/legato)
//        -> Drive (tanh grit) -> Formant talk box (4 bandpass formants
//           per vowel, A/E/I/O/U with morph + Auto-Talk LFO)
//        -> Mix (dry carrier / talkboxed) -> Output
//
// This is a formant-filter emulation of the classic talk box sound,
// NOT a physical tube talk box: no real mouth or tube is involved.
// The vowel shaping is an approximation of the human vocal tract, and
// it will not sound identical to a hardware talk box. It is also NOT
// a clone of any other talk box product — it is built from standard
// published vowel formant frequencies and a from-scratch carrier.
class FamousBeatsTalkBoxProcessor : public juce::AudioProcessor
{
public:
    FamousBeatsTalkBoxProcessor();
    ~FamousBeatsTalkBoxProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Famous Beats Talk Box"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.3; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    juce::AudioProcessorValueTreeState& getParametersState() { return parameters; }

    // One-click presets. Sets every section's parameters.
    // 0 = Roger, 1 = G-Funk, 2 = Robot, 3 = Talk Wah, 4 = Clean Talk
    void applyPreset(int presetIndex);
    static const char* getPresetName(int presetIndex);
    int getCurrentPreset() const { return currentPreset; }

    // Live readouts (for the editor / tests).
    float getCurrentVowel() const { return currentVowel.load(); }
    void getFormantFreqs(float* freqs, int n) const;
    void getFormantGains(float* gains, int n) const;
    // Effective Auto-Talk rate in Hz (tempo-synced divisions when Sync
    // is on, otherwise the Hz control). Uses the host BPM when the
    // playhead provides one, else the manual BPM parameter.
    double getAutoTalkRateHz() const;
    // Beats per Auto-Talk division index (0 = 1/4, 1 = 1/8, 2 = 1/16).
    static double divisionToBeats(int divisionIndex);
    // Current host tempo in BPM, or 0 when the host provides none.
    double getHostBpm() const { return hostBpm.load(); }

    // ---- Vowel formant tables ----
    // Standard published vowel formant frequencies (Hz), F1..F4.
    // A ~730/1090/2440, E ~530/1840/2480, I ~270/2290/3010,
    // O ~570/840/2410, U ~300/870/2240. F4 is set to 3500 Hz for all
    // vowels (a typical high-formant region for a male voice).
    static constexpr int kNumVowels = 5;
    static constexpr int kNumFormants = 4;
    static constexpr float kVowelFormants[kNumVowels][kNumFormants] = {
        {  730.0f, 1090.0f, 2440.0f, 3500.0f }, // A
        {  530.0f, 1840.0f, 2480.0f, 3500.0f }, // E
        {  270.0f, 2290.0f, 3010.0f, 3500.0f }, // I
        {  570.0f,  840.0f, 2410.0f, 3500.0f }, // O
        {  300.0f,  870.0f, 2240.0f, 3500.0f }, // U
    };
    // Relative gain per formant (F1 loudest, falling off above).
    static constexpr float kVowelGains[kNumVowels][kNumFormants] = {
        { 1.00f, 0.75f, 0.50f, 0.35f }, // A
        { 1.00f, 0.70f, 0.50f, 0.35f }, // E
        { 1.00f, 0.65f, 0.50f, 0.35f }, // I
        { 1.00f, 0.80f, 0.50f, 0.35f }, // O
        { 1.00f, 0.75f, 0.50f, 0.35f }, // U
    };
    // Bandpass Q per formant (narrower higher up, like a vocal tract).
    static constexpr float kFormantQ[kNumFormants] = { 5.0f, 8.0f, 9.0f, 9.0f };
    static constexpr const char* kVowelNames[kNumVowels] = { "A", "E", "I", "O", "U" };

    // ---- Parameter IDs ----
    static constexpr const char* kBypassParam     = "bypass";
    static constexpr const char* kOutputGainParam = "outputGain"; // dB

    // Carrier synth.
    static constexpr const char* kWaveBlendParam = "waveBlend"; // 0..1 (0 = saw, 1 = square)
    static constexpr const char* kSubLevelParam  = "subLevel";  // 0..1
    static constexpr const char* kGlideParam     = "glide";     // 0..1 (0..0.5 s)
    static constexpr const char* kOctaveParam    = "octave";    // 0..4 (-2..+2)
    static constexpr const char* kMonoParam      = "monoMode";  // bool

    // Formant talk box.
    static constexpr const char* kVowelParam = "vowel"; // 0..4 continuous (A..U)

    // Auto-Talk LFO (moves the vowel morph automatically).
    static constexpr const char* kAutoTalkOnParam       = "autoTalkOn";       // bool
    static constexpr const char* kAutoTalkRateParam     = "autoTalkRate";     // Hz, 0.05..8
    static constexpr const char* kAutoTalkDepthParam    = "autoTalkDepth";    // 0..1
    static constexpr const char* kAutoTalkSyncParam     = "autoTalkSync";     // bool
    static constexpr const char* kAutoTalkDivisionParam = "autoTalkDivision"; // 0..2 (1/4, 1/8, 1/16)
    static constexpr const char* kAutoTalkBpmParam      = "autoTalkBpm";      // manual BPM, 40..240
    static constexpr const char* kAutoTalkShapeParam    = "autoTalkShape";    // 0..2 (Smooth/Triangle/Steps)

    // Drive + mix.
    static constexpr const char* kDriveParam    = "drive";    // 0..100 %
    static constexpr const char* kDriveMixParam = "driveMix"; // 0..100 %
    static constexpr const char* kMixParam      = "mix";      // 0..100 % (dry carrier -> talkboxed)

    static constexpr int kNumPresets = 5;
    static constexpr int kMaxVoices = 8;
    static constexpr const char* kAutoTalkDivisionNames[3] = { "1/4", "1/8", "1/16" };
    static constexpr const char* kAutoTalkShapeNames[3] = { "Smooth", "Triangle", "Steps" };
    static constexpr const char* kOctaveNames[5] = { "-2", "-1", "0", "+1", "+2" };

private:
    // ---- Carrier voices ----
    struct Voice
    {
        bool active = false;
        bool releasing = false;
        int note = -1;
        float targetFreq = 440.0f;
        float currentFreq = 440.0f;
        float phaseSaw = 0.0f;
        float phaseSquare = 0.0f;
        float phaseSub = 0.0f;
        float env = 0.0f;      // 0..1 amplitude envelope
        float velocity = 0.0f; // 0..1
        int age = 0;           // blocks since started (voice stealing)
    };

    void noteOn(int note, float velocity);
    void noteOff(int note);
    void allNotesOff();
    static float noteToFreq(int note, int octaveIndex, float bendSemitones);
    // PolyBLEP anti-aliased saw / square.
    static float polyBlep(float phase, float dt);
    float renderVoiceSample(Voice& voice, float blend, float subLevel,
                            float glideCoeff, float attackCoeff, float releaseCoeff,
                            float bendFactor);

    void setParamValue(const char* paramId, float value);

    juce::AudioProcessorValueTreeState parameters;
    int currentPreset = 0; // starts on Roger

    // Cached parameter pointers.
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* outputGainParam = nullptr;
    std::atomic<float>* waveBlendParam = nullptr;
    std::atomic<float>* subLevelParam = nullptr;
    std::atomic<float>* glideParam = nullptr;
    std::atomic<float>* octaveParam = nullptr;
    std::atomic<float>* monoParam = nullptr;
    std::atomic<float>* vowelParam = nullptr;
    std::atomic<float>* autoTalkOnParam = nullptr;
    std::atomic<float>* autoTalkRateParam = nullptr;
    std::atomic<float>* autoTalkDepthParam = nullptr;
    std::atomic<float>* autoTalkSyncParam = nullptr;
    std::atomic<float>* autoTalkDivisionParam = nullptr;
    std::atomic<float>* autoTalkBpmParam = nullptr;
    std::atomic<float>* autoTalkShapeParam = nullptr;
    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* driveMixParam = nullptr;
    std::atomic<float>* mixParam = nullptr;

    // Host tempo (BPM) from the playhead, 0 when the host gives none.
    std::atomic<double> hostBpm { 0.0 };
    std::atomic<float> currentVowel { 1.0f };

    double sampleRateHz = 44100.0;
    std::vector<float> carrierScratch; // mono carrier sum, one block

    // Voices + mono/legato note stack.
    std::array<Voice, kMaxVoices> voices;
    std::vector<int> monoNotes; // held notes in mono mode (stack)
    float pitchBendSemitones = 0.0f; // -2..+2

    // Auto-Talk LFO phase (0..1 cycles).
    double lfoPhase = 0.0;

    // Formant bank: 4 bandpass filters on the summed mono carrier.
    // IIR::Filter is neither copyable nor movable, so held by unique_ptr.
    std::unique_ptr<juce::dsp::IIR::Filter<float>> formantFilters[kNumFormants];
    float formantFreqs[kNumFormants] = { 730.0f, 1090.0f, 2440.0f, 3500.0f };
    float formantGains[kNumFormants] = { 1.0f, 0.75f, 0.5f, 0.35f };
    float formantFreqTargets[kNumFormants] = { 730.0f, 1090.0f, 2440.0f, 3500.0f };
    float formantGainTargets[kNumFormants] = { 1.0f, 0.75f, 0.5f, 0.35f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FamousBeatsTalkBoxProcessor)
};
