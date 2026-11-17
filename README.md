# Famous Beats Talk Box — playable MIDI talk box for Mac

Keys and talk box in one plugin. Load it on a MIDI / instrument
track, play your keyboard, and it talks — Roger Troutman / Zapp
style, G-funk talkbox sound. No audio input, no tube, no mic:
MIDI notes drive a built-in synth carrier through a formant (vowel)
shaping engine.

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

## How to use it

- Load it as an **instrument** on a MIDI track — not as an effect on
  an audio track. It makes its own sound; it needs no audio input.
- **Play your MIDI keyboard.** The notes you play drive the carrier
  synth, and the formant engine makes it talk.
- **Pick a preset first** — Roger, G-Funk, Robot, Talk Wah, or Clean
  Talk. That sets the whole chain at once. Then turn any knob to taste.
- **Vowel display** — the big gold letter shows the vowel the talk box
  is on right now, with the four live formant peaks as gold bars
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
- It adds no latency, and it is light on CPU — it is a small synth
  plus four bandpass filters.

## The chain (in order)

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

## Please read — what this is and isn't

This plugin is a **formant-filter emulation** of the talk box sound.
It shapes a synth carrier with the same formant frequencies as human
vowels, which is why it talks. It is **not a physical tube talk box**:
there is no tube, no mouth, and no real vocal tract involved, so it
will not sound identical to a hardware talk box — a real one has a
mouth shaping the sound in ways a filter bank can only approximate.
It is also **not a clone of any other talk box product** — it is built
from scratch from standard vowel formant data. Think of it as a talk
box *style* instrument in the classic tradition, not a copy of
anything on the market.

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
- Tested on Linux before shipping (all tests pass): an A3 note at
  velocity 100 renders at −25.1 dBFS RMS with peak 0.17; the five
  vowels produce measurably distinct spectra (max pairwise cosine
  similarity 0.88, and 0.91 against the raw carrier — the formants
  genuinely reshape the sound); morphing A → U flips the formant
  emphasis (660 Hz / 220 Hz ratio 2.79 for A, 0.09 for U); Auto-Talk
  moves the vowel from 1.86 to 0.00 over a quarter of its cycle and
  changes the window RMS by 24%; silence stays silent; all five
  presets render with peaks at or below 0.29 and no NaN; glide +
  mono/legato holds sound through the note change and releases cleanly.
  Real results depend on the notes and preset.

## Files in this project

- `CMakeLists.txt` — the build file (JUCE is downloaded automatically)
- `Source/PluginProcessor.h` / `.cpp` — carrier synth + formant engine
- `Source/PluginEditor.h` / `.cpp` — the black-and-gold interface
- `test/talkbox_test.cpp` — a test program that plays MIDI notes
  through the chain and reports measured results
- `.github/workflows/build-macos.yml` — the cloud build

## Credits

- Built with JUCE 8 (GPLv3 / JUCE licence — see JUCE's licence terms)
- Vowel formant data: standard published formant frequencies for
  English vowels (Peterson & Barney and later sources)
- Famous Beats Studio
