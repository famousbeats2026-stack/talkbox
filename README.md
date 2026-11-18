# Famous Beats Talk Box — playable MIDI talk box + vocoder for Mac

Two ways to talk in one plugin. **Talk Box mode** is the original:
keys and talk box in one plugin — MIDI notes drive a built-in synth
carrier through a formant (vowel) shaping engine, Roger Troutman /
Zapp style, G-funk talkbox sound. No audio input needed.

**Vocoder mode** is new: sing into your mic while you play the keys.
The mic is the modulator and the same MIDI carrier synth is the
carrier, through a 16-band channel vocoder. Pitch comes from the keys,
words and consonants come from the mic — it only sounds like talking
when both are going at once.

**What you get here is the full plugin project — not the plugin file
itself.** A Mac plugin file (.vst3 / .component) can only be built on a
Mac (or on GitHub's Mac computers). Pick one of the two ways below to
make the installable file. The easy one is Option A — it needs no Xcode
on your Mac.

---

## Option A — Let GitHub build it for you (easiest, no Xcode needed)

You need a free GitHub account. If you don't have one yet, make one at
https://github.com/signup — it takes about 5 minutes, just an email
and a password.

1. On GitHub, press the **+** at the top right, then **New repository**.
   Name it `talkbox`, leave it Public, and press **Create repository**.
2. On the next page, press **"uploading an existing file"**.
3. Drag this whole folder (everything in it) into the upload area, then
   press **Commit changes**.
4. Click the **Actions** tab at the top. You'll see a run called
   "Build macOS plugin" — wait for the green checkmark (a few minutes).
   If it hasn't started, press **Run workflow** on the right.
5. When it's done, click the finished run, scroll to the bottom, and
   download **FamousBeatsTalkBox-macOS**. Unzip it — inside are:
   - `Famous Beats Talk Box.vst3`
   - `Famous Beats Talk Box.component`

## Option B — Build it on your own Mac (needs Xcode)

1. Install Xcode free from the App Store (it's a big download, about
   12 GB — let it finish). Then open Terminal once and run:
   `xcode-select --install`
2. In Terminal, get this project (replace the link with your own if you
   put it on GitHub, or just copy this folder to your Mac):
   ```
   git clone https://github.com/YOURNAME/talkbox.git
   cd talkbox
   ```
3. Build it:
   ```
   cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15
   cmake --build build --config Release
   ```
4. The finished files are in:
   - `build/FamousBeatsTalkBox_artefacts/Release/VST3/Famous Beats Talk Box.vst3`
   - `build/FamousBeatsTalkBox_artefacts/Release/AU/Famous Beats Talk Box.component`

---

## Installing the plugin

1. Copy the files into your plugin folders:
   - `.vst3` → `~/Library/Audio/Plug-Ins/VST3/`
   - `.component` → `~/Library/Audio/Plug-Ins/Components/`

   (To get there in Finder: press Cmd+Shift+G and paste the path.)
2. This plugin isn't signed with an Apple developer certificate, so
   macOS may block it the first time. If your DAW says it can't load
   it, run this in Terminal (it just clears the block flag):
   ```
   xattr -cr ~/Library/Audio/Plug-Ins/VST3/"Famous Beats Talk Box.vst3"
   xattr -cr ~/Library/Audio/Plug-Ins/Components/"Famous Beats Talk Box.component"
   ```
   Then restart your DAW and rescan plugins.
3. Works on macOS 10.15 (Catalina) and newer, Intel and Apple Silicon
   (universal binary — both chips in one file).

## How to use it — Ableton Live 11

The plugin is an audio effect that also takes MIDI. That is what
makes the vocoder work: the audio track carries your voice, and a
MIDI track plays the carrier.

### Vocoder mode — sing through it (mic + keys)

1. **Audio track (your voice):** make an audio track, set its input
   to your mic, and arm it (or set Monitor to In) so you can hear
   yourself. Drop **Famous Beats Talk Box** on it as an audio effect.
2. **MIDI track (the keys):** make a separate MIDI track. Set its
   **MIDI To** to the audio track from step 1 — a second dropdown
   appears underneath; pick **Famous Beats Talk Box** as the
   destination. Arm the MIDI track too (or set its Monitor to In) so
   your keys reach the plugin.
3. **Switch the plugin to VOCODER mode** — press the VOCODER button
   at the top of the plugin, or hit the **Vocoder** preset.
4. **Sing into the mic and play the keys at the same time.** Hold a
   chord or a note and talk or sing over it — the keys set the pitch,
   your mouth sets the words.

**Say it plain:** pitch comes from the keys, the words and
consonants come from the mic. It only sounds like talking when both
are going at once. Play keys with no singing and you get a quiet
robot hum; sing with no keys and you get nothing at all.

**Vocoder knobs:**
- **Mic Level** — how hard your voice drives the bands. Turn it up
  if the robot sound is faint; turn it down if it distorts.
- **Carrier** — how loud the synth is going into the vocoder.
- **Vocoder Mix** — all the way up is the full robot voice; pull it
  back to blend your natural voice in underneath.
- The **16-band display** shows the mic's shape live — sing and the
  gold bars dance. If the bars barely move, your mic is too quiet
  (check the audio track's input level).
- A built-in noise gate keeps room noise from opening the bands, so
  the carrier stays quiet when you're not singing.

### Talk Box mode — keys only (no mic)

1. Put the plugin on an audio track (same routing as above, but with
   no mic needed), then route a MIDI track to it: **MIDI To** → the
   audio track → **Famous Beats Talk Box**. Arm the MIDI track and
   play.
2. You can also just drop it straight onto a MIDI track and play —
   in Talk Box mode it ignores the audio input, so it works either
   way.
3. **Pick a preset first** — Roger, G-Funk, Robot, Talk Wah, or Clean
   Talk. That sets the whole chain at once. Then turn any knob to taste.

**Talk Box controls:**
- **Vowel display** — the big gold letter shows the vowel the talk
  box is on right now, with the four live formant peaks as gold bars
  underneath. Click a vowel button (A / E / I / O / U) or turn the
  **Morph** knob to move between vowels yourself.
- **Auto-Talk** — turn it on and an LFO moves the vowel for you, so
  the talk box talks on its own while you hold notes. Rate sets how
  fast, Depth sets how far it travels, Shape picks Smooth (sine),
  Triangle, or Steps (hard vowel jumps — the Robot sound). Turn Sync
  on and the rate follows your song's tempo (1/4, 1/8, or 1/16 notes
  from the DAW, or the BPM knob when the DAW gives no tempo).
- **Carrier** — Wave Blend moves between saw (buzzy, classic) and
  square (hollow, robotic). Sub adds an octave-down sine for weight.
  Glide slides the pitch between notes. Octave shifts the whole
  keyboard up or down. Mono switches to one voice with legato — the
  way a real talkbox player slides between notes without retriggering.
- **Drive** — tanh grit before the formants. More drive = more throat.
  Drive Mix blends it with the clean carrier.
- **Mix** — dry carrier vs talkboxed signal. All the way up is the
  full talk box; pull it back to let the raw synth through.
- **Pitch bend** works (2 semitones), and the sustain pedal holds
  notes like a normal synth.

**Mode switch:** the TALK BOX / VOCODER buttons at the top switch
modes. In Vocoder mode the vowel buttons, Morph knob, and Auto-Talk
controls are hidden — they don't apply there. Switching modes
mid-song is safe: the plugin resets the filters and fades back in
over ~10 ms so it never clicks.

**Latency:** Talk Box mode adds no latency. Vocoder mode adds about
4.4 ms (194 samples at 44.1 kHz) from the filter bank — the plugin
reports this to your DAW so it stays in time with your other tracks,
and the dry mic path inside the plugin is delayed by the same amount
so the dry/wet mix stays lined up.

## The chain (in order)

**Talk Box mode:**
1. **Carrier synth** — polyphonic (up to 8 voices) or mono/legato.
   Anti-aliased saw + square oscillators with a blend, sub oscillator
   one octave down, glide/portamento, octave selector.
2. **Drive** — tanh saturation for throat grit, with a mix control.
3. **Formant talk box** — four bandpass formants per vowel (A / E /
   I / O / U), from standard published vowel formant frequencies,
   crossfaded by the Morph control or moved automatically by Auto-Talk.
4. **Mix** — dry carrier vs talkboxed signal.
5. **Output gain** — plus a soft clipper so it can never clip the
   output, even with all 8 voices at full tilt.

**Vocoder mode:**
1. **Modulator (mic)** — summed to mono, Mic Level, through a
   16-band filter bank (100 Hz – 8 kHz, log-spaced, Q 3). An
   envelope follower per band (~2 ms attack, ~80 ms release) tracks
   how loud the voice is in each band. A noise gate keeps room
   noise from opening the bands.
2. **Carrier (MIDI synth)** — the same carrier synth and Drive as
   Talk Box mode, through the same 16-band bank.
3. **Vocoder** — each carrier band's gain follows its modulator
   band's envelope, so the synth takes on the shape of your voice.
4. **Vocoder Mix** — dry mic (delayed to match the vocoder latency)
   vs vocoded signal.
5. **Output gain** — plus the same soft clipper, so it can never clip.

## The presets

- **Roger** — the classic Zapp sound. Saw carrier, vowel morphing on
  a slow Auto-Talk, moderate drive.
- **G-Funk** — slower Auto-Talk, deeper drive, sub turned up. Low and
  wide.
- **Robot** — square carrier, mono/legato, fast stepped Auto-Talk.
  Hard vowel jumps.
- **Talk Wah** — Auto-Talk off. Play the vowel yourself with the
  buttons or the Morph knob, like working a wah pedal.
- **Clean Talk** — gentle and mostly vowels, light drive. The
  politest talk box.
- **Vocoder** — switches to Vocoder mode with sensible levels: saw
  carrier, light drive, Mic Level full, Carrier 80%, Vocoder Mix
  100%. Sing and play.

## Please read — what this is and isn't

This plugin is a **formant-filter emulation** of the talk box sound
and a **classic channel vocoder**. It shapes a synth carrier with the
same formant frequencies as human vowels (talk box) or with the
spectral envelope of your voice (vocoder), which is why it talks. It
is **not a physical tube talk box**: there is no tube, no mouth, and
no real vocal tract involved, so it will not sound identical to a
hardware talk box — a real one has a mouth shaping the sound in ways
a filter bank can only approximate. It is also **not a clone of any
other talk box or vocoder product** — it is built from scratch from
standard vowel formant data and a standard channel-vocoder design.
Think of it as a talk box / vocoder *style* plugin in the classic
tradition, not a copy of anything on the market.

## What it does, technically

- Carrier: polyBLEP anti-aliased saw + square, sub sine one octave
  down, 8-voice polyphony with quietest-voice stealing, mono/legato
  mode with a held-note stack, exponential glide, pitch bend.
- Formants: 4 RBJ bandpass filters (Q 5 / 8 / 9 / 9) per vowel, from
  published formant tables — A 730/1090/2440, E 530/1840/2480,
  I 270/2290/3010, O 570/840/2410, U 300/870/2240 Hz (F4 3500 Hz for
  all vowels). Formant frequencies and gains are interpolated
  continuously between adjacent vowels and smoothed over ~30 ms so
  morphing never clicks.
- Auto-Talk: sine / triangle / square LFO, 0.05–8 Hz in Hz mode,
  tempo-synced 1/4 / 1/8 / 1/16 divisions from the host BPM with a
  manual BPM fallback. Depth moves the vowel up to ±2 vowels.
- Vocoder: 16 log-spaced RBJ bandpass filters (Q 3), 100 Hz – 8 kHz,
  one bank for the modulator and one for the carrier; per-band
  envelope followers (2 ms attack, 80 ms release) so consonants come
  through without chatter; mic noise gate (opens above about
  −54 dBFS); measured end-to-end latency 4.4 ms (194 samples at
  44.1 kHz, 189 samples for a noise burst) reported to the host with
  the dry path delay-matched.
- Tested on Linux before shipping (all 10 tests pass): Talk Box mode
  is unchanged — an A3 note at velocity 100 renders at −25.1 dBFS RMS
  with peak 0.17; the five vowels produce measurably distinct spectra
  (max pairwise cosine similarity 0.88, and 0.91 against the raw
  carrier); morphing A → U flips the formant emphasis (660 Hz /
  220 Hz ratio 2.79 for A, 0.09 for U); Auto-Talk moves the vowel
  from 1.86 to 0.00 over a quarter of its cycle and changes the window
  RMS by 24%; silence stays silent; all five talk box presets render
  with peaks at or below 0.29 and no NaN; glide + mono/legato holds
  sound through the note change and releases cleanly. Vocoder mode:
  mic + A3 note renders at RMS 0.095 with peak 0.28; the output's
  pitch follows the MIDI note (220 Hz is 80× stronger than 330 Hz for
  A3, and 261.6 Hz is 125× stronger than 220 Hz for C4); the output
  spectrum follows the modulator's bands (cosine similarity 0.73
  against modulator × carrier bands); a silent mic with a held note
  gives exactly zero output (no carrier bleed); a mic with no MIDI
  gives exactly zero output; a hot mic at full velocity peaks at 1.00
  with no NaN (the soft clipper catches it); switching modes
  mid-stream sounds clean on both sides with no click spike; the
  Vocoder preset renders at RMS 0.092 with peak 0.28. Real results
  depend on the notes, the preset, and your mic level.

## Files in this project

- `CMakeLists.txt` — the build file (JUCE is downloaded automatically)
- `Source/PluginProcessor.h` / `.cpp` — carrier synth + formant
  engine + 16-band vocoder
- `Source/PluginEditor.h` / `.cpp` — the black-and-gold interface
  (mode switch, vowel display, vocoder band display)
- `test/talkbox_test.cpp` — a test program that plays MIDI notes
  through both modes and reports measured results
- `.github/workflows/build-macos.yml` — the cloud build

## Credits

- Built with JUCE 8 (GPLv3 / JUCE licence — see JUCE's licence terms)
- Vowel formant data: standard published formant frequencies for
  English vowels (Peterson & Barney and later sources)
- Famous Beats Studio
