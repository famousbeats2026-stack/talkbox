// Test harness: feed MIDI notes through the Famous Beats Talk Box
// chain and report measured results.
//
//   1. A MIDI note (A3, velocity 100) renders audible audio (RMS/peak)
//   2. The 5 vowels produce measurably distinct spectra (Goertzel
//      magnitudes at the carrier's harmonics, pairwise cosine
//      similarity) and differ from the raw carrier
//   3. Morph from A to U moves the formant emphasis (660 Hz vs 220 Hz)
//   4. Auto-Talk on moves the vowel over time; off holds it still
//      (plus the RMS difference between two windows)
//   5. Silence with no notes stays silent
//   6. All 5 presets render with peak <= 1.0, no NaN, no clipping
//   7. Glide + mono/legato mode: no crash, no NaN, sound continues
//      through the legato note change
//
// Usage: talkbox_test
// Exit code 0 = all tests passed, 1 = any test failed.

#include "../Source/PluginProcessor.h"
#include <cstdio>
#include <cmath>
#include <vector>
#include <functional>
#include <algorithm>

namespace
{
    using Processor = FamousBeatsTalkBoxProcessor;
    constexpr double kSr = 44100.0;
    constexpr int kBlock = 512;
    constexpr double kPi = 3.14159265358979323846;

    double db(double rms) { return 20.0 * std::log10(rms + 1e-12); }

    void setParam(Processor& p, const char* id, float value)
    {
        auto* param = p.getParametersState().getParameter(id);
        param->setValueNotifyingHost(param->convertTo0to1(value));
    }

    std::unique_ptr<Processor> makeProc()
    {
        auto proc = std::make_unique<Processor>();
        proc->prepareToPlay(kSr, kBlock);
        return proc;
    }

    struct MidiEvent
    {
        double timeSec;
        juce::MidiMessage msg;
    };

    // Render `seconds` of audio through the processor in 512-sample
    // blocks, delivering the scheduled MIDI events. `afterBlock(t)`
    // is called after each block with the time at the END of the block.
    // Returns the full stereo output.
    juce::AudioBuffer<float> render(Processor& proc, double seconds,
                                    const std::vector<MidiEvent>& events,
                                    const std::function<void(double)>& afterBlock = nullptr)
    {
        const int total = (int) (seconds * kSr);
        juce::AudioBuffer<float> out(2, total);
        juce::AudioBuffer<float> block(2, kBlock);
        int done = 0;
        while (done < total)
        {
            const int n = juce::jmin(kBlock, total - done);
            juce::MidiBuffer midi;
            for (const auto& e : events)
            {
                const int samplePos = (int) (e.timeSec * kSr) - done;
                if (samplePos >= 0 && samplePos < n)
                    midi.addEvent(e.msg, samplePos);
            }
            block.clear();
            block.setSize(2, n, false, false, false);
            proc.processBlock(block, midi);
            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom(ch, done, block, ch, 0, n);
            done += n;
            if (afterBlock != nullptr)
                afterBlock(done / kSr);
            block.setSize(2, kBlock, false, false, false);
        }
        return out;
    }

    double regionRms(const juce::AudioBuffer<float>& buf, double t0, double t1)
    {
        const int s0 = (int) (t0 * kSr), s1 = (int) (t1 * kSr);
        double sum = 0.0;
        int n = 0;
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        {
            const auto* data = buf.getReadPointer(ch);
            for (int i = s0; i < s1 && i < buf.getNumSamples(); ++i)
            {
                sum += (double) data[i] * data[i];
                ++n;
            }
        }
        return n ? std::sqrt(sum / n) : 0.0;
    }

    float regionPeak(const juce::AudioBuffer<float>& buf, double t0, double t1)
    {
        const int s0 = (int) (t0 * kSr), s1 = (int) (t1 * kSr);
        float peak = 0.0f;
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        {
            const auto* data = buf.getReadPointer(ch);
            for (int i = s0; i < s1 && i < buf.getNumSamples(); ++i)
                peak = juce::jmax(peak, std::abs(data[i]));
        }
        return peak;
    }

    int countNan(const juce::AudioBuffer<float>& buf)
    {
        int n = 0;
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        {
            const auto* data = buf.getReadPointer(ch);
            for (int i = 0; i < buf.getNumSamples(); ++i)
                if (std::isnan(data[i]) || std::isinf(data[i]))
                    ++n;
        }
        return n;
    }

    // Goertzel magnitude at one frequency over a window of samples.
    // The window is exactly 220 cycles of a 220 Hz carrier, so probing
    // at exact harmonics of 220 Hz is leakage-free.
    double goertzel(const float* data, int n, double freq)
    {
        const double w = 2.0 * kPi * freq / kSr;
        const double coeff = 2.0 * std::cos(w);
        double s1 = 0.0, s2 = 0.0;
        for (int i = 0; i < n; ++i)
        {
            const double s0 = data[i] + coeff * s1 - s2;
            s2 = s1;
            s1 = s0;
        }
        const double power = s1 * s1 + s2 * s2 - coeff * s1 * s2;
        return std::sqrt(std::max(0.0, power)) / n;
    }

    // Harmonic magnitude vector (harmonics 1..15 of 220 Hz) for a
    // 1-second window of channel 0, normalised to unit length.
    std::vector<double> harmonicVector(const juce::AudioBuffer<float>& buf,
                                       double t0, double t1)
    {
        const int s0 = (int) (t0 * kSr), s1 = (int) (t1 * kSr);
        const auto* data = buf.getReadPointer(0);
        std::vector<double> v;
        for (int h = 1; h <= 15; ++h)
            v.push_back(goertzel(data + s0, s1 - s0, 220.0 * h));
        double norm = 0.0;
        for (double x : v)
            norm += x * x;
        norm = std::sqrt(norm);
        if (norm > 0.0)
            for (double& x : v)
                x /= norm;
        return v;
    }

    double cosine(const std::vector<double>& a, const std::vector<double>& b)
    {
        double dot = 0.0;
        for (size_t i = 0; i < a.size() && i < b.size(); ++i)
            dot += a[i] * b[i];
        return dot; // both vectors are unit length
    }

    int failures = 0;
    void check(bool ok, const char* name)
    {
        std::printf("  [%s] %s\n", ok ? "PASS" : "FAIL", name);
        if (!ok)
            ++failures;
    }

    // Common setup for the vowel tests: manual vowel, talkboxed only.
    void setupVowelTest(Processor& p, float vowel)
    {
        setParam(p, Processor::kAutoTalkOnParam, 0.0f);
        setParam(p, Processor::kVowelParam, vowel);
        setParam(p, Processor::kMixParam, 100.0f);
        setParam(p, Processor::kDriveParam, 0.0f);
        setParam(p, Processor::kSubLevelParam, 0.0f);
        setParam(p, Processor::kWaveBlendParam, 0.0f); // pure saw carrier
        setParam(p, Processor::kOutputGainParam, 0.0f);
    }

    std::vector<MidiEvent> noteA3(double onTime = 0.1, double offTime = 1.4)
    {
        return {
            { onTime, juce::MidiMessage::noteOn(1, 57, (juce::uint8) 100) }, // A3 = 220 Hz
            { offTime, juce::MidiMessage::noteOff(1, 57) },
        };
    }

    // ---- Vocoder test helpers ----

    // Vowel-A-like modulator: harmonics of a 110 Hz "voice" shaped by
    // the A-vowel formant envelope (peaks at 730 / 1090 / 2440 /
    // 3500 Hz). This is what a sung "ahh" looks like spectrally.
    float vowelModSample(double t, float amplitude = 0.35f)
    {
        static const float formantF[4] = { 730.0f, 1090.0f, 2440.0f, 3500.0f };
        static const float formantG[4] = { 1.00f, 0.75f, 0.50f, 0.35f };
        double s = 0.0;
        for (int h = 1; h <= 40; ++h)
        {
            const double f = 110.0 * h;
            if (f > 5000.0)
                break;
            double w = 0.25; // floor so every band has some energy
            for (int i = 0; i < 4; ++i)
            {
                const double d = (f - formantF[i]) / (0.35 * formantF[i]);
                w += formantG[i] * std::exp(-0.5 * d * d);
            }
            s += (w / h) * std::sin(2.0 * kPi * f * t);
        }
        return (float) (s * amplitude);
    }

    // Fill a mono buffer with the vowel modulator over `seconds`.
    juce::AudioBuffer<float> makeVowelModBuffer(double seconds, float amplitude = 0.35f)
    {
        const int total = (int) (seconds * kSr);
        juce::AudioBuffer<float> buf(1, total);
        auto* data = buf.getWritePointer(0);
        for (int i = 0; i < total; ++i)
            data[i] = vowelModSample(i / kSr, amplitude);
        return buf;
    }

    // Render with a modulator signal fed into the audio input (both
    // input channels get the same mono modulator, like a mic on a
    // stereo track). `mod` is a mono buffer covering `seconds`.
    juce::AudioBuffer<float> renderWithMod(Processor& proc, double seconds,
                                           const std::vector<MidiEvent>& events,
                                           const juce::AudioBuffer<float>& mod,
                                           const std::function<void(double)>& afterBlock = nullptr)
    {
        const int total = (int) (seconds * kSr);
        juce::AudioBuffer<float> out(2, total);
        juce::AudioBuffer<float> block(2, kBlock);
        int done = 0;
        while (done < total)
        {
            const int n = juce::jmin(kBlock, total - done);
            juce::MidiBuffer midi;
            for (const auto& e : events)
            {
                const int samplePos = (int) (e.timeSec * kSr) - done;
                if (samplePos >= 0 && samplePos < n)
                    midi.addEvent(e.msg, samplePos);
            }
            block.clear();
            block.setSize(2, n, false, false, false);
            // Feed the modulator into the input channels.
            const auto* modData = mod.getReadPointer(0);
            for (int ch = 0; ch < 2; ++ch)
            {
                auto* dst = block.getWritePointer(ch);
                for (int i = 0; i < n; ++i)
                    dst[i] = modData[done + i];
            }
            proc.processBlock(block, midi);
            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom(ch, done, block, ch, 0, n);
            done += n;
            if (afterBlock != nullptr)
                afterBlock(done / kSr);
            block.setSize(2, kBlock, false, false, false);
        }
        return out;
    }

    // Goertzel magnitude at each vocoder band centre over a window.
    std::vector<double> bandEnergies(const juce::AudioBuffer<float>& buf,
                                     double t0, double t1)
    {
        float freqs[Processor::kNumVocoderBands] = {};
        // Centres are log-spaced 100 Hz – 8 kHz; recompute here so the
        // helper works even before prepareToPlay ran.
        const double ratio = std::pow(8000.0 / 100.0,
                                      1.0 / (Processor::kNumVocoderBands - 1));
        for (int i = 0; i < Processor::kNumVocoderBands; ++i)
            freqs[i] = (float) (100.0 * std::pow(ratio, i));

        const int s0 = (int) (t0 * kSr), s1 = (int) (t1 * kSr);
        const auto* data = buf.getReadPointer(0);
        std::vector<double> v;
        for (int b = 0; b < Processor::kNumVocoderBands; ++b)
            v.push_back(goertzel(data + s0, s1 - s0, freqs[b]));
        return v;
    }

    // Cosine similarity between two non-negative vectors (both
    // normalised to unit length first).
    double cosineSim(std::vector<double> a, std::vector<double> b)
    {
        auto normalise = [](std::vector<double>& v) {
            double norm = 0.0;
            for (double x : v)
                norm += x * x;
            norm = std::sqrt(norm);
            if (norm > 0.0)
                for (double& x : v)
                    x /= norm;
        };
        normalise(a);
        normalise(b);
        double dot = 0.0;
        for (size_t i = 0; i < a.size() && i < b.size(); ++i)
            dot += a[i] * b[i];
        return dot;
    }

    // Common setup for the vocoder tests: Vocoder preset, but with
    // drive off and a pure saw carrier so the measurements are clean.
    void setupVocoderTest(Processor& p)
    {
        p.applyPreset(5); // Vocoder
        setParam(p, Processor::kDriveParam, 0.0f);
        setParam(p, Processor::kWaveBlendParam, 0.0f); // pure saw
        setParam(p, Processor::kSubLevelParam, 0.0f);
        setParam(p, Processor::kVocoderMixParam, 100.0f); // fully wet
        setParam(p, Processor::kOutputGainParam, 0.0f);
    }
}

int main()
{
    std::printf("Famous Beats Talk Box — test harness\n\n");

    // ---- Test 1: a MIDI note renders audible audio ----
    {
        std::printf("Test 1: MIDI note A3 renders audio (Roger preset)\n");
        auto proc = makeProc();
        proc->applyPreset(0);
        auto out = render(*proc, 2.0, noteA3(0.1, 1.5));
        const double rms = regionRms(out, 0.5, 1.4);
        const float peak = regionPeak(out, 0.0, 2.0);
        std::printf("  RMS (0.5-1.4s): %.4f (%.1f dBFS), peak: %.4f, NaN: %d\n",
                    rms, db(rms), peak, countNan(out));
        check(rms > 0.05, "note is audible (RMS > 0.05)");
        check(peak <= 1.0f, "peak <= 1.0");
        check(countNan(out) == 0, "no NaN/Inf");
    }

    // ---- Test 2: the 5 vowels have distinct spectra ----
    {
        std::printf("Test 2: five vowels, distinct spectra\n");
        std::vector<std::vector<double>> vectors;
        for (int v = 0; v < 5; ++v)
        {
            auto proc = makeProc();
            setupVowelTest(*proc, (float) v);
            auto out = render(*proc, 1.5, noteA3());
            vectors.push_back(harmonicVector(out, 0.5, 1.4));
        }
        // Raw carrier for comparison (mix = 0).
        std::vector<double> carrierVec;
        {
            auto proc = makeProc();
            setupVowelTest(*proc, 0.0f);
            setParam(*proc, Processor::kMixParam, 0.0f);
            auto out = render(*proc, 1.5, noteA3());
            carrierVec = harmonicVector(out, 0.5, 1.4);
        }

        double maxPairwise = 0.0;
        for (int i = 0; i < 5; ++i)
        {
            for (int j = i + 1; j < 5; ++j)
                maxPairwise = std::max(maxPairwise, cosine(vectors[i], vectors[j]));
        }
        double maxVsCarrier = 0.0;
        for (int i = 0; i < 5; ++i)
            maxVsCarrier = std::max(maxVsCarrier, cosine(vectors[i], carrierVec));

        for (int v = 0; v < 5; ++v)
        {
            // Top 3 emphasised harmonics for this vowel.
            std::vector<int> idx(15);
            for (int i = 0; i < 15; ++i)
                idx[i] = i;
            std::sort(idx.begin(), idx.end(), [&](int a, int b) {
                return vectors[v][a] > vectors[v][b];
            });
            std::printf("  vowel %s: top harmonics %d Hz (%.2f), %d Hz (%.2f), %d Hz (%.2f)\n",
                        Processor::kVowelNames[v],
                        (idx[0] + 1) * 220, vectors[v][idx[0]],
                        (idx[1] + 1) * 220, vectors[v][idx[1]],
                        (idx[2] + 1) * 220, vectors[v][idx[2]]);
        }
        std::printf("  max pairwise cosine similarity between vowels: %.3f\n", maxPairwise);
        std::printf("  max cosine similarity vs raw carrier: %.3f\n", maxVsCarrier);
        check(maxPairwise < 0.97, "vowels are spectrally distinct (cosine < 0.97)");
        check(maxVsCarrier < 0.97, "formants shape the carrier (cosine vs raw < 0.97)");
    }

    // ---- Test 3: morph A -> U moves the formant emphasis ----
    {
        std::printf("Test 3: morph A -> U moves formant emphasis\n");
        double ratioA = 0.0, ratioU = 0.0;
        for (int pass = 0; pass < 2; ++pass)
        {
            auto proc = makeProc();
            setupVowelTest(*proc, pass == 0 ? 0.0f : 4.0f);
            auto out = render(*proc, 1.5, noteA3());
            const auto* data = out.getReadPointer(0);
            const int s0 = (int) (0.5 * kSr), n = (int) (0.9 * kSr);
            const double e660 = goertzel(data + s0, n, 660.0);
            const double e220 = goertzel(data + s0, n, 220.0);
            const double ratio = e660 / (e220 + 1e-12);
            std::printf("  vowel %s: 660 Hz mag %.5f, 220 Hz mag %.5f, ratio %.3f\n",
                        pass == 0 ? "A" : "U", e660, e220, ratio);
            if (pass == 0)
                ratioA = ratio;
            else
                ratioU = ratio;
        }
        check(ratioA > ratioU, "A emphasises 660 Hz more than U does");
        check(ratioA > 1.0, "A: 660 Hz stronger than 220 Hz (F1 ~730)");
        check(ratioU < 1.0, "U: 220 Hz stronger than 660 Hz (F1 ~300)");
    }

    // ---- Test 4: Auto-Talk moves the vowel over time ----
    {
        std::printf("Test 4: Auto-Talk on/off\n");
        float vAt05 = 0.0f, vAt075 = 0.0f;
        double rmsW1 = 0.0, rmsW2 = 0.0;
        {
            auto proc = makeProc();
            setParam(*proc, Processor::kAutoTalkOnParam, 1.0f);
            setParam(*proc, Processor::kAutoTalkSyncParam, 0.0f);
            setParam(*proc, Processor::kAutoTalkRateParam, 1.0f); // 1 Hz sine
            setParam(*proc, Processor::kAutoTalkDepthParam, 1.0f);
            setParam(*proc, Processor::kAutoTalkShapeParam, 0.0f); // Smooth
            setParam(*proc, Processor::kVowelParam, 2.0f);         // centre I
            setParam(*proc, Processor::kMixParam, 100.0f);
            auto out = render(*proc, 2.0, noteA3(0.1, 1.8), [&](double t) {
                if (std::abs(t - 0.5) < 0.012)
                    vAt05 = proc->getCurrentVowel();
                if (std::abs(t - 0.75) < 0.012)
                    vAt075 = proc->getCurrentVowel();
            });
            rmsW1 = regionRms(out, 0.5, 0.6);
            rmsW2 = regionRms(out, 0.75, 0.85);
            std::printf("  Auto-Talk ON: vowel at 0.50s = %.2f, at 0.75s = %.2f\n", vAt05, vAt075);
            std::printf("  RMS 0.50-0.60s = %.4f, RMS 0.75-0.85s = %.4f (rel. diff %.1f%%)\n",
                        rmsW1, rmsW2, 100.0 * std::abs(rmsW1 - rmsW2) / (rmsW1 + 1e-12));
            check(std::abs(vAt075 - vAt05) > 0.5f, "vowel moves with Auto-Talk on");
            check(std::abs(rmsW1 - rmsW2) / (rmsW1 + 1e-12) > 0.01,
                  "output changes over time (window RMS differs > 1%)");
        }
        {
            auto proc = makeProc();
            setParam(*proc, Processor::kAutoTalkOnParam, 0.0f);
            setParam(*proc, Processor::kVowelParam, 2.0f);
            setParam(*proc, Processor::kMixParam, 100.0f);
            float v1 = 0.0f, v2 = 0.0f;
            render(*proc, 2.0, noteA3(0.1, 1.8), [&](double t) {
                if (std::abs(t - 0.5) < 0.012)
                    v1 = proc->getCurrentVowel();
                if (std::abs(t - 0.75) < 0.012)
                    v2 = proc->getCurrentVowel();
            });
            std::printf("  Auto-Talk OFF: vowel at 0.50s = %.2f, at 0.75s = %.2f\n", v1, v2);
            check(std::abs(v2 - v1) < 0.001f, "vowel holds still with Auto-Talk off");
        }
    }

    // ---- Test 5: silence stays silent ----
    {
        std::printf("Test 5: silence with no notes\n");
        auto proc = makeProc();
        proc->applyPreset(0);
        auto out = render(*proc, 1.0, {});
        const float peak = regionPeak(out, 0.0, 1.0);
        std::printf("  peak: %.8f, NaN: %d\n", peak, countNan(out));
        check(peak < 1e-6f, "no output without notes");
        check(countNan(out) == 0, "no NaN/Inf");
    }

    // ---- Test 6: all 5 presets render clean ----
    {
        std::printf("Test 6: all 5 presets\n");
        for (int i = 0; i < 5; ++i)
        {
            auto proc = makeProc();
            proc->applyPreset(i);
            auto out = render(*proc, 1.5, noteA3(0.1, 1.2));
            const float peak = regionPeak(out, 0.0, 1.5);
            const double rms = regionRms(out, 0.5, 1.1);
            std::printf("  %-10s peak %.4f, RMS %.4f, NaN %d\n",
                        Processor::getPresetName(i), peak, rms, countNan(out));
            char name[64];
            std::snprintf(name, sizeof(name), "preset %s: peak <= 1.0, no NaN",
                          Processor::getPresetName(i));
            check(peak <= 1.0f && countNan(out) == 0, name);
            check(rms > 0.02, "preset is audible");
        }
    }

    // ---- Test 7: glide + mono/legato ----
    {
        std::printf("Test 7: glide + mono/legato\n");
        auto proc = makeProc();
        proc->applyPreset(3); // Talk Wah (manual vowel)
        setParam(*proc, Processor::kMonoParam, 1.0f);
        setParam(*proc, Processor::kGlideParam, 0.5f); // ~0.125 s glide
        std::vector<MidiEvent> events = {
            { 0.1, juce::MidiMessage::noteOn(1, 57, (juce::uint8) 100) },  // A3 (220 Hz)
            { 0.5, juce::MidiMessage::noteOn(1, 60, (juce::uint8) 100) },  // C4 (legato)
            { 1.0, juce::MidiMessage::noteOff(1, 60) },
            { 1.1, juce::MidiMessage::noteOff(1, 57) },
        };
        auto out = render(*proc, 2.0, events);
        const float peak = regionPeak(out, 0.0, 2.0);
        const double rmsLegato = regionRms(out, 0.6, 0.9);
        const double rmsAfterOff = regionRms(out, 1.6, 1.9);
        std::printf("  peak %.4f, RMS during legato (0.6-0.9s) %.4f, RMS after note-off (1.6-1.9s) %.4f, NaN %d\n",
                    peak, rmsLegato, rmsAfterOff, countNan(out));
        check(peak <= 1.0f, "peak <= 1.0");
        check(countNan(out) == 0, "no NaN/Inf");
        check(rmsLegato > 0.02, "sound continues through legato note change");
        check(rmsAfterOff < 0.001, "voice releases after note-off");
    }

    // ---- Test 8: vocoder mode ----
    {
        std::printf("Test 8: vocoder mode (mic in + MIDI carrier)\n");

        // (a) Spectrum follows the modulator, pitch follows the MIDI note.
        std::vector<double> outBands, predicted;
        double rmsVoc = 0.0, peakVoc = 0.0;
        {
            auto proc = makeProc();
            setupVocoderTest(*proc);
            auto mod = makeVowelModBuffer(1.5);
            auto out = renderWithMod(*proc, 1.5, noteA3(0.1, 1.4), mod);
            outBands = bandEnergies(out, 0.5, 1.4);
            rmsVoc = regionRms(out, 0.5, 1.4);
            peakVoc = regionPeak(out, 0.0, 1.5);
            std::printf("  vocoder out: RMS (0.5-1.4s) %.4f, peak %.4f, NaN %d\n",
                        rmsVoc, peakVoc, countNan(out));
            check(rmsVoc > 0.02, "(a) vocoder output is audible with mic + note");
            check(peakVoc <= 1.0f, "(a) peak <= 1.0");
            check(countNan(out) == 0, "(a) no NaN/Inf");

            // Pitch follows the MIDI note: A3 = 220 Hz.
            const auto* data = out.getReadPointer(0);
            const int s0 = (int) (0.5 * kSr), n = (int) (0.9 * kSr);
            const double e220 = goertzel(data + s0, n, 220.0);
            const double e330 = goertzel(data + s0, n, 330.0);
            std::printf("  pitch check: 220 Hz mag %.5f, 330 Hz mag %.5f, ratio %.2f\n",
                        e220, e330, e220 / (e330 + 1e-12));
            check(e220 > 3.0 * e330, "(a) pitch follows the MIDI note (220 >> 330)");

            // Spectrum follows the modulator: predicted band vector is
            // (modulator bands) x (carrier bands), measured the same way.
            std::vector<double> modBands = bandEnergies(mod, 0.5, 1.4);
            std::vector<double> carrierBands;
            {
                auto proc2 = makeProc();
                setupVowelTest(*proc2, 0.0f);
                setParam(*proc2, Processor::kMixParam, 0.0f); // raw carrier
                auto raw = render(*proc2, 1.5, noteA3(0.1, 1.4));
                carrierBands = bandEnergies(raw, 0.5, 1.4);
            }
            predicted.resize(Processor::kNumVocoderBands);
            for (int b = 0; b < Processor::kNumVocoderBands; ++b)
                predicted[b] = modBands[b] * carrierBands[b];
            const double sim = cosineSim(outBands, predicted);
            std::printf("  band spectrum: cosine similarity out vs (mod x carrier) = %.3f\n", sim);
            check(sim > 0.7, "(a) output spectrum follows the modulator bands");
        }

        // Same note, different key: pitch follows the note (C4 = 261.6 Hz).
        {
            auto proc = makeProc();
            setupVocoderTest(*proc);
            auto mod = makeVowelModBuffer(1.5);
            std::vector<MidiEvent> events = {
                { 0.1, juce::MidiMessage::noteOn(1, 60, (juce::uint8) 100) }, // C4
                { 1.4, juce::MidiMessage::noteOff(1, 60) },
            };
            auto out = renderWithMod(*proc, 1.5, events, mod);
            const auto* data = out.getReadPointer(0);
            const int s0 = (int) (0.5 * kSr), n = (int) (0.9 * kSr);
            const double e262 = goertzel(data + s0, n, 261.63);
            const double e220 = goertzel(data + s0, n, 220.0);
            std::printf("  C4 check: 261.6 Hz mag %.5f, 220 Hz mag %.5f, ratio %.2f\n",
                        e262, e220, e262 / (e220 + 1e-12));
            check(e262 > 2.0 * e220, "(a) pitch follows a different note (C4 261.6 >> 220)");
        }

        // (b) Silence on the mic + a held note = near-silence (no carrier bleed).
        {
            auto proc = makeProc();
            setupVocoderTest(*proc);
            juce::AudioBuffer<float> silence(1, (int) (1.5 * kSr));
            silence.clear();
            auto out = renderWithMod(*proc, 1.5, noteA3(0.1, 1.4), silence);
            const float peak = regionPeak(out, 0.0, 1.5);
            std::printf("  (b) silent mic + held note: peak %.8f, NaN %d\n", peak, countNan(out));
            check(peak < 1e-4f, "(b) no carrier bleed with a silent mic");
            check(countNan(out) == 0, "(b) no NaN/Inf");
        }

        // (c) Mic signal present but no MIDI = near-silence.
        {
            auto proc = makeProc();
            setupVocoderTest(*proc);
            auto mod = makeVowelModBuffer(1.5);
            auto out = renderWithMod(*proc, 1.5, {}, mod);
            const float peak = regionPeak(out, 0.0, 1.5);
            std::printf("  (c) mic present, no MIDI: peak %.8f, NaN %d\n", peak, countNan(out));
            check(peak < 1e-4f, "(c) silence with no MIDI note");
            check(countNan(out) == 0, "(c) no NaN/Inf");
        }

        // (d) Hot input + full velocity: still no clipping, no NaN.
        {
            auto proc = makeProc();
            setupVocoderTest(*proc);
            auto mod = makeVowelModBuffer(1.5, 1.0f); // loud mic
            std::vector<MidiEvent> events = {
                { 0.1, juce::MidiMessage::noteOn(1, 57, (juce::uint8) 127) },
                { 1.4, juce::MidiMessage::noteOff(1, 57) },
            };
            auto out = renderWithMod(*proc, 1.5, events, mod);
            const float peak = regionPeak(out, 0.0, 1.5);
            std::printf("  (d) hot mic + velocity 127: peak %.4f, NaN %d\n", peak, countNan(out));
            check(peak <= 1.0f, "(d) peak <= 1.0 at full tilt");
            check(countNan(out) == 0, "(d) no NaN/Inf");
        }

        // The Vocoder preset itself (with its own drive etc.) is audible.
        {
            auto proc = makeProc();
            proc->applyPreset(5);
            auto mod = makeVowelModBuffer(1.5);
            auto out = renderWithMod(*proc, 1.5, noteA3(0.1, 1.4), mod);
            const double rms = regionRms(out, 0.5, 1.4);
            const float peak = regionPeak(out, 0.0, 1.5);
            std::printf("  Vocoder preset: RMS %.4f, peak %.4f, NaN %d\n", rms, peak, countNan(out));
            check(rms > 0.02, "Vocoder preset is audible");
            check(peak <= 1.0f && countNan(out) == 0, "Vocoder preset clean");
        }
    }

    // ---- Test 9: mode switching mid-stream ----
    {
        std::printf("Test 9: mode switching mid-stream\n");
        auto proc = makeProc();
        proc->applyPreset(0); // Roger (Talk Box)
        auto mod = makeVowelModBuffer(2.0);
        bool switchedToVocoder = false, switchedBack = false;
        auto out = renderWithMod(*proc, 2.0, noteA3(0.1, 1.8), mod, [&](double t) {
            if (!switchedToVocoder && t >= 0.5)
            {
                setParam(*proc, Processor::kModeParam, 1.0f);
                switchedToVocoder = true;
            }
            if (!switchedBack && t >= 1.2)
            {
                setParam(*proc, Processor::kModeParam, 0.0f);
                switchedBack = true;
            }
        });
        const float peak = regionPeak(out, 0.0, 2.0);
        const float peakAfterSwitch1 = regionPeak(out, 0.5, 0.52);
        const float peakAfterSwitch2 = regionPeak(out, 1.2, 1.22);
        const double rmsVocoder = regionRms(out, 0.7, 1.1);
        const double rmsTalkBox = regionRms(out, 1.5, 1.8);
        std::printf("  peak overall %.4f, 20ms after switch 1: %.4f, after switch 2: %.4f\n",
                    peak, peakAfterSwitch1, peakAfterSwitch2);
        std::printf("  RMS vocoder segment (0.7-1.1s) %.4f, RMS talk box segment (1.5-1.8s) %.4f, NaN %d\n",
                    rmsVocoder, rmsTalkBox, countNan(out));
        check(switchedToVocoder && switchedBack, "both mode switches happened");
        check(peak <= 1.0f, "peak <= 1.0 across the switches");
        check(countNan(out) == 0, "no NaN/Inf");
        check(peakAfterSwitch1 <= 1.0f && peakAfterSwitch2 <= 1.0f,
              "no click spike at the switches");
        check(rmsVocoder > 0.01, "vocoder sounds after switching to it");
        check(rmsTalkBox > 0.01, "talk box sounds after switching back");
    }

    // ---- Test 10: vocoder latency measurement ----
    {
        std::printf("Test 10: vocoder latency (mic burst + held note)\n");
        auto measureOnset = [](bool noiseBurst) {
            auto proc = makeProc();
            setupVocoderTest(*proc);
            // Modulator: silence, then a 30 ms burst at 0.5 s.
            juce::AudioBuffer<float> mod(1, (int) (1.2 * kSr));
            mod.clear();
            auto* data = mod.getWritePointer(0);
            const int burstStart = (int) (0.5 * kSr);
            const int burstLen = (int) (0.03 * kSr);
            // Deterministic pseudo-noise for the noise-burst variant.
            unsigned seed = 12345;
            for (int i = 0; i < burstLen; ++i)
            {
                const double t = (burstStart + i) / kSr;
                if (noiseBurst)
                {
                    seed = seed * 1664525u + 1013904223u;
                    data[burstStart + i] = ((seed >> 8) / 16777216.0f - 0.5f) * 0.8f;
                }
                else
                {
                    data[burstStart + i] = vowelModSample(t, 0.4f);
                }
            }
            std::vector<MidiEvent> events = {
                { 0.05, juce::MidiMessage::noteOn(1, 57, (juce::uint8) 100) },
                { 0.95, juce::MidiMessage::noteOff(1, 57) },
            };
            auto out = renderWithMod(*proc, 1.2, events, mod);
            const auto* outData = out.getReadPointer(0);
            float peak = 0.0f;
            for (int i = burstStart; i < (int) (0.9 * kSr); ++i)
                peak = juce::jmax(peak, std::abs(outData[i]));
            int first = -1;
            for (int i = burstStart; i < (int) (0.9 * kSr); ++i)
            {
                if (std::abs(outData[i]) > 0.1f * peak)
                {
                    first = i;
                    break;
                }
            }
            const double latencySec = first >= 0 ? (first - burstStart) / kSr : -1.0;
            return std::make_pair(latencySec, (double) peak);
        };

        const auto [vowelLatency, vowelPeak] = measureOnset(false);
        const auto [noiseLatency, noisePeak] = measureOnset(true);
        std::printf("  vowel burst: onset %.2f ms (%d samples @ 44.1k), peak %.4f\n",
                    vowelLatency * 1000.0, (int) std::lround(vowelLatency * kSr), vowelPeak);
        std::printf("  noise burst: onset %.2f ms (%d samples @ 44.1k), peak %.4f\n",
                    noiseLatency * 1000.0, (int) std::lround(noiseLatency * kSr), noisePeak);
        int reportedSamples = 0;
        {
            auto proc = makeProc();
            setupVocoderTest(*proc);
            juce::AudioBuffer<float> mod(1, (int) (0.1 * kSr));
            mod.clear();
            renderWithMod(*proc, 0.1, noteA3(0.0, 0.09), mod);
            reportedSamples = proc->getLatencySamples();
        }
        std::printf("  reported latency: %d samples (%.2f ms)\n",
                    reportedSamples, 1000.0 * Processor::kVocoderLatencySeconds);
        check(vowelLatency > 0.0, "vocoder has a measurable onset delay");
        check(vowelLatency < 0.015, "latency under 15 ms");
        check(noiseLatency > 0.0 && noiseLatency < 0.015, "noise-burst latency sane");
        const double measuredSamples = vowelLatency * kSr;
        check(std::abs(reportedSamples - measuredSamples) <= 0.3 * measuredSamples,
              "reported latency matches the measured onset (within 30%)");
    }

    std::printf("\n%s: %d failure(s)\n", failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED", failures);
    return failures == 0 ? 0 : 1;
}
