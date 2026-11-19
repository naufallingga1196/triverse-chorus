# TriVerse Chorus

A three-voice stereo chorus plugin (VST3 / AU / Standalone) inspired by 1980s
three-phase rack chorus units, known for the wide, "swirling" electric piano sound.
Built with JUCE 8 / C++17.

- Three modulated delay lines running in parallel.
- Three LFOs with a fixed 120° phase offset, so the modulation is never in sync.
- Each voice sits at a different stereo position (left, centre, right), creating spatial
  movement rather than mere widening.
- Analog character from emulating a BBD (bucket-brigade delay) chip and its surrounding
  filters and compander.

The goal is not to copy any single unit exactly, but to capture the **behaviour** and
**character** of this kind of circuit under the product's own identity.

## Download

Installers for the latest release:

| Platform | Installer | Contents |
|---|---|---|
| macOS — any Mac (recommended) | [TriVerse-Chorus-macOS-universal.pkg](https://github.com/naufallingga1196/triverse-chorus/releases/latest/download/TriVerse-Chorus-macOS-universal.pkg) | VST3, AU, Standalone · macOS 10.13+ |
| macOS — Apple Silicon (M1 and later) | [TriVerse-Chorus-macOS-arm64.pkg](https://github.com/naufallingga1196/triverse-chorus/releases/latest/download/TriVerse-Chorus-macOS-arm64.pkg) | VST3, AU, Standalone · macOS 11+ |
| macOS — Intel | [TriVerse-Chorus-macOS-x86_64.pkg](https://github.com/naufallingga1196/triverse-chorus/releases/latest/download/TriVerse-Chorus-macOS-x86_64.pkg) | VST3, AU, Standalone · macOS 10.13+ |
| Windows 10/11 (64-bit) | [TriVerse-Chorus-Windows-Setup.exe](https://github.com/naufallingga1196/triverse-chorus/releases/latest/download/TriVerse-Chorus-Windows-Setup.exe) | VST3, Standalone |

All versions and release notes: [Releases](https://github.com/naufallingga1196/triverse-chorus/releases).

### Installing

The installers are not code-signed yet, so the operating system will warn you the first time:

- **macOS:** if the installer is blocked ("cannot be opened because it is from an
  unidentified developer"), open **System Settings → Privacy & Security** and click
  **Open Anyway**. The plugins go to `/Library/Audio/Plug-Ins/{VST3,Components}` and the
  app to `/Applications`.
- **Windows:** if SmartScreen shows "Windows protected your PC", click **More info → Run
  anyway**. The plugin goes to `C:\Program Files\Common Files\VST3`.

Afterwards, rescan plugins in your DAW. Look for **TriVerse Chorus** under modulation effects.

## Prerequisites

### macOS

1. **Xcode** (from the App Store), then run once:
   ```sh
   sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
   sudo xcodebuild -license accept
   ```
2. **CMake ≥ 3.22**: `brew install cmake`, or the official binary from cmake.org.

### Windows

1. **Visual Studio 2022** with the "Desktop development with C++" workload.
2. **CMake ≥ 3.22** and **Git**.

JUCE is downloaded automatically by CMake (tag 8.0.15). To use a local copy instead,
pass `-DJUCE_DIR=/path/to/JUCE`.

## Build

macOS:

```sh
cmake -B build -G Xcode
cmake --build build --config Release
ctest --test-dir build -C Release        # DSP unit tests
```

Or without the Xcode IDE: `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j`.

Windows:

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 -DTRICHORUS_COPY_PLUGIN=OFF
cmake --build build --config Release
```

On macOS, build outputs are copied automatically to `~/Library/Audio/Plug-Ins/{VST3,Components}`
(disable with `-DTRICHORUS_COPY_PLUGIN=OFF`). The Standalone app is in
`build/TriVerseChorus_artefacts/Release/Standalone/`.

For a universal binary (Intel + Apple Silicon), add
`-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`.

## Validation

```sh
auval -v aufx Tvc3 Tvrs                  # Audio Unit validation
pluginval --strictness-level 10 --validate ~/Library/Audio/Plug-Ins/VST3/TriVerse\ Chorus.vst3
```

## Automated releases (GitHub Actions)

The workflow in [.github/workflows/build.yml](.github/workflows/build.yml) builds the plugin
on macOS and Windows, runs the unit tests, and produces installers:

- macOS (VST3 + AU + Standalone), via [tools/macos/make-pkg.sh](tools/macos/make-pkg.sh):
  - `TriVerse-Chorus-<version>-macOS-universal.pkg`: any Mac, macOS 10.13+
  - `TriVerse-Chorus-<version>-macOS-arm64.pkg`: Apple Silicon (M1 and later), macOS 11+
  - `TriVerse-Chorus-<version>-macOS-x86_64.pkg`: Intel Macs, macOS 10.13+
- Windows (VST3 + Standalone, x64), via [tools/windows/installer.iss](tools/windows/installer.iss):
  - `TriVerse-Chorus-<version>-Windows-Setup.exe`

Every run uploads versioned installers under **Actions → run → Artifacts** (GitHub login
required, kept for 90 days). To publish a public GitHub Release, push a version tag. The
release job strips the version from the file names (e.g. `TriVerse-Chorus-macOS-arm64.pkg`)
so the links in [Download](#download) always point to the latest release:

```sh
git tag v0.1.0 && git push origin v0.1.0
```

The version is read from `project (... VERSION x.y.z)` in `CMakeLists.txt`. The installers
are not code-signed yet, so macOS shows a Gatekeeper warning and Windows shows a
SmartScreen warning.

To build a `.pkg` locally: `tools/macos/make-pkg.sh 0.1.0 build x86_64` (output goes to `dist/`).

## GUI snapshot

Render the panel to a PNG without a DAW (useful for reviewing the look):

```sh
cmake -B build -DTRICHORUS_BUILD_TOOLS=ON
cmake --build build --target trichorus_snapshot
./build/trichorus_snapshot_artefacts/Release/trichorus_snapshot panel.png 2
```

## Project structure

```
Source/
├── PluginProcessor.*      AudioProcessor, APVTS, bypass crossfade, peak meter
├── PluginEditor.*         1U rack panel (1320×200 canvas, resizable)
├── gui/                   LookAndFeel for knobs, buttons, LEDs, meter
├── params/                Parameter definitions
└── dsp/                   Pure DSP engine (no JUCE dependency)
    ├── TriChorusEngine.*  wires all modules together
    ├── Compander.h        compressor & expander
    ├── BbdDelayLine.h     buffer + Hermite interpolation
    ├── BbdFilter.h        AA & recon low-pass
    ├── LfoBank.h          3 LFOs + drift
    ├── Saturation.h
    └── PanMatrix.h
tests/DspUnitTests.cpp     Numerical DSP tests
tools/                     Installer scripts and GUI snapshot tool
```

DSP classes do not depend on JUCE, so they can be tested on their own.

---

# Technical design

## Goals & constraints

### Goals
1. A wide, smooth, musical three-phase chorus on electric piano, clean guitar, pads, and vocals.
2. Convincing analog character: slightly dark, slightly noisy, with "alive" modulation.
3. Zero latency (no lookahead).
4. Low CPU load: target < 1% per instance on a modern desktop CPU at 48 kHz.
5. No audible aliasing at extreme settings.

### Non-goals (v1)
- Do not model every circuit component (this is not a real-time SPICE simulation).
- No flanger/vibrato mode in v1 (possible roadmap item).
- No LFO tempo sync in v1 (roadmap).

## Signal architecture

```
                     ┌──────────────────────────────────────────────────────────┐
 In L ──┐            │   WET PATH (repeated 3×, one per voice)                  │
        ├─► Input ───┼─► Compressor ─► AA LPF ─► BBD Delay ─► Recon LPF ─► Expander ─┼─► Pan Matrix ─┐
 In R ──┘   Stage    │   (NE570-ish)    (≈8–10k)   (modulated    (≈8–10k)   (NE570-ish)│               │
            (mono    │                             by LFO n)                          │               │
             sum /   └──────────────────────────────────────────────────────────┘               │
             stereo)                                                                             ▼
                                                                                   ┌────────────────────┐
 Dry L/R ───────────────────────────────────────────────────────────────────────►  │  Dry/Wet Mix +     │─► Out L
                                                                                   │  Width + Output    │─► Out R
                                                                                   └────────────────────┘

 LFO Bank:  LFO1 (φ = 0°)   LFO2 (φ = 120°)   LFO3 (φ = 240°)
            + slow random drift per LFO (optional)
```

Design notes:
- **Is the compander per voice or shared?** In many hardware designs the compander sits
  before and after the whole BBD block. For efficiency, v1 uses **one shared compressor**
  at the front and **one expander per voice** at the back (since each voice has a
  different delay). To be validated against the reference.
- **Mono vs stereo input.** The input is *summed* to mono before the wet path (typical
  rack chorus behaviour), while the dry path stays stereo.

## DSP modules

### Input stage
- Input gain (−12 dB to +12 dB).
- Light soft clipper (tanh or cubic polynomial) to mimic op-amp headroom. Nearly
  transparent by default; only noticeable when the input is pushed.
- DC blocker (one-pole HPF at ~10 Hz).

### Compressor (pre-BBD)
Mimics the 2:1 compander commonly used to suppress BBD noise.

- **Detector:** average rectifier (not true RMS) with a one-pole filter. Effective
  attack/release is set by a single time constant (initially ~10–20 ms, tuned against
  the reference).
- **Gain:** `g = 1 / sqrt(env / ref)`, limited so it does not blow up on very small signals.
- Desired side effect: slight "pumping" and softened transients on the wet signal.

### Anti-aliasing filter (AA LPF)
- 2- to 4-pole low-pass (two Butterworth biquads or a Sallen-Key model), cutoff ~8–10 kHz.
- In hardware this filter exists because a BBD is a discrete-time sampled system. In the
  plugin it is actually a key part of the character (the "dark" sound).

### BBD delay model
This is the core of the character. Two implementation levels are planned.

#### Level A — modulated delay + filter (v1, current)
- Circular delay line with **fractional interpolation** (4-point Hermite or 3rd-order
  Lagrange; linear is too "dull" and causes amplitude modulation).
- Delay time follows the BBD formula:

  ```
  t_delay = N_stages / (2 × f_clock)
  ```
  Example: 1024 stages, 100 kHz clock → 5.12 ms.

  Modulation is applied to the **clock**, not directly to the delay time. Because the
  relationship is inverse, symmetric clock modulation produces slightly asymmetric delay
  modulation, which contributes to the character.

- Extra character:
  - Light in-path saturation (small asymmetry for even harmonics).
  - BBD noise floor (filtered white noise at −80 to −70 dBFS), passed through the
    compander so that it "breathes" too.
  - Small per-stage signal loss (extra HF damping that depends on delay length).

#### Level B — clock-based BBD emulation (v2 / "Vintage" mode)
- Simulate the sample-and-hold process at the real BBD clock rate, including aliasing and
  imaging filtered by models of the input/output filters.
- Reference approach: Holters & Parker (DAFx-18), which combines the BBD and its
  input/output filters into a single efficient model.
- More accurate, but more CPU-intensive and more complex. Scheduled after Level A matures.

### LFO bank
- 3 LFOs sharing one master phase, offset by 0°, 120°, and 240°.
- Waveforms: sine (default) and triangle. The "analog" option is a slightly rounded
  triangle, like an op-amp integrator LFO.
- **Rate:** 0.05–10 Hz (logarithmic scale).
- **Depth:** 0–100%, mapped to a clock-deviation range.
- **Drift:** very slow random noise (< 0.2 Hz) per LFO at small amplitude, so the three
  LFOs do not sound "digitally perfect".
- **Phase spread:** hidden/advanced parameter (0–180°) for experimentation; default 120°.

### Reconstruction filter (recon LPF)
- Similar to the AA LPF (cutoff ~8–10 kHz). In hardware this filter attenuates clock
  components and images.
- Level A: a biquad is enough. Level B: part of the combined model.

### Expander (post-BBD)
- The inverse of the compressor: 1:2 ratio, `g = env / ref`, with a detector of the same kind.
- Small mismatches between compressor and expander (e.g. different time constants) are
  intentional, as they produce the characteristic "breathing" artefacts.

### Clock bleed (optional)
- A very subtle tone at the clock frequency aliased into the audible range. **Off** or very
  quiet by default; could be part of the "Vintage" mode.

### Pan matrix, width & mix
- Default voice positions: V1 = hard left, V2 = centre, V3 = hard right.
- **Width (0–100%):** interpolates the pan positions from all-centre (mono) to the default positions.
- Pan law: constant-power (−3 dB at centre).
- **Mix:** equal-power dry/wet.
- **Output gain:** −12 dB to +12 dB.

## Parameters (v1)

| ID | Name | Range | Default | Scale | Notes |
|---|---|---|---|---|---|
| `input` | Input | −12 … +12 dB | 0 dB | linear dB | drives the soft clip |
| `rate` | Rate | 0.05 … 10 Hz | 0.6 Hz | log | LFO rate |
| `int1`–`int3` | Intensity I / II / III | 0 … 100 % | 45 % | linear | clock deviation per voice (100 % = ±30 % clock) |
| `delay` | Delay | 2 … 20 ms | 7 ms | log | centre delay time |
| `shape` | Shape | Sine / Tri / Analog | Sine | choice | LFO waveform |
| `width` | Width | 0 … 100 % | 100 % | linear | stereo spread |
| `tone` | Tone | 4 … 14 kHz | 9 kHz | log | AA/recon cutoff |
| `char` | Character | 0 … 100 % | 30 % | linear | saturation, noise, and drift together |
| `mix` | Mix | 0 … 100 % | 50 % | linear | dry/wet |
| `output` | Output | −12 … +12 dB | 0 dB | linear dB | |
| `bypass` | Bypass | on/off | off | — | click-free bypass |

All continuous parameters use **smoothing** (~20 ms) to prevent zipper noise.

## Core pseudocode (Level A)

```cpp
// per block
for (int n = 0; n < numSamples; ++n)
{
    float x   = monoSum(inL[n], inR[n]) * inputGain;
    x         = softClip(dcBlock(x));
    float xc  = compressor.process(x);          // shared
    xc        = aaFilter.process(xc);
    delayLine.write(xc + bbdNoise.next());

    float outL = 0.f, outR = 0.f;
    for (int v = 0; v < 3; ++v)
    {
        float lfo    = lfoBank.value(v);               // −1 … +1, drift included
        float clock  = baseClock * (1.f + depth * lfo);
        float tDelay = N_STAGES / (2.f * clock);       // seconds
        float y      = delayLine.readHermite(tDelay * sampleRate);
        y            = bbdSaturate(y, character);
        y            = reconFilter[v].process(y);
        y            = expander[v].process(y);
        outL += y * panL[v];
        outR += y * panR[v];
    }
    lfoBank.advance();

    outBufL[n] = mixEqualPower(inL[n], outL, mix) * outputGain;
    outBufR[n] = mixEqualPower(inR[n], outR, mix) * outputGain;
}
```

`baseClock` is derived from the `delay` parameter (by inverting the BBD formula), so the
user sets milliseconds while the modulation still works in the clock domain. The actual
implementation is in [Source/dsp/TriChorusEngine.cpp](Source/dsp/TriChorusEngine.cpp).

## Quality & performance

- **Oversampling:** Level A generally does not need it, since its nonlinearities are mild.
  A 2× option for saturation could be added when `char` is high. Level B will likely need
  an internal multirate approach.
- **Denormals:** `juce::ScopedNoDenormals` is enabled in the audio callback.
- **Sample rate:** all filter coefficients and time constants are recomputed in
  `prepareToPlay` for 44.1–192 kHz.
- **Delay buffer size:** allocated for maximum delay + maximum deviation + interpolation
  margin.
- **Determinism:** noise/drift generators use a fixed seed per instance so offline renders
  are consistent.
- **Mono compatibility:** test the L+R sum; a three-phase chorus can cause partial
  cancellation. Consider slightly different filters per voice to reduce sharp combing in mono.

## Validation plan

### With access to reference hardware
1. **Sine sweep** dry vs wet → wet-path frequency response (filter cutoff and slope).
2. **Steady 1 kHz sine** → extract the output's phase/frequency modulation to measure the
   LFO shape, rate, and depth, and the delay range.
3. **Bursts and stepped-level tones** → compander behaviour (time constants, gain curve).
4. **Silent input** → noise spectrum and clock bleed.
5. **High-level input** → saturation curve and harmonics.

### Without hardware
- Use legitimate reference recordings (e.g. samples from libraries you own) and compare
  by ear and by spectrogram.
- Tune parameter by parameter against "target" presets.

### Technical tests
- Unit tests: filter response, delay accuracy, LFO continuity, compander gain.
- Stress tests: fast automation of all parameters, buffer size changes, sample rate changes.
- Compatibility: pluginval (high strictness), Reaper, Ableton Live, Logic Pro, FL Studio,
  Cubase, Pro Tools (if AAX).

## Roadmap

| Stage | Content | Deliverable |
|---|---|---|
| M0 | Python prototype: 3-voice chorus + filter + compander | A/B audio files (skipped) |
| M1 | JUCE port, basic parameters, GUI | working plugin (VST3/AU) ✅ |
| M2 | Character: saturation, noise, drift, filter tuning | "beta sound" build |
| M3 | Custom GUI polish, presets | feature-complete build |
| M4 | Validation (pluginval, cross-DAW), CPU optimisation | release candidate |
| M5 (optional) | Vintage mode (Level B BBD), tempo sync, vibrato mode | v1.1 / v2 |

## Product identity & legal

- Uses its **own name, logo, and GUI design**. Hardware brand names and other plugin
  brand names are not used in the product.
- Marketing copy should stay generic, e.g. "inspired by classic 80s three-phase rack chorus".
- JUCE is used under AGPLv3; a commercial JUCE licence is needed for closed-source distribution.
- VST3 requires agreeing to Steinberg's licence; AAX requires registering with Avid's
  developer programme.
- This is not legal advice; consult an expert if the product will be sold.

## Technical references

- M. Holters & J. D. Parker, *A Combined Model for a Bucket Brigade Device and its Input
  and Output Filters*, DAFx-18.
- C. Raffel & J. O. Smith, *Practical Modeling of Bucket-Brigade Device Circuits*, DAFx-10.
- J. O. Smith, *Physical Audio Signal Processing* — sections on delay lines and fractional
  interpolation (CCRMA, Stanford).
- JUCE documentation: `juce::dsp`, `AudioProcessorValueTreeState`.
- BBD chip datasheets (MN3xxx family) and NE570/NE571 compander datasheets for stage
  counts, clock ranges, and compression curves.

## Open questions

1. Is there access to a hardware unit or reference recordings for measurement?
2. Should AAX be added in a later version?

---

## License

AGPLv3. See [LICENSE](LICENSE). VST is a trademark of Steinberg Media Technologies GmbH.
