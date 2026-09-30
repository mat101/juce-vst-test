# MySynth

A cross-platform VST3 / AU / Standalone synthesizer plugin, built with
[JUCE](https://juce.com), ported from a Web Audio API prototype.

## Features

- 4 waveforms: Sine, Square, Sawtooth, Triangle
- ADSR-style envelope (Attack / Release controls)
- Shared low-pass filter (cutoff + resonance) applied to the mixed output —
  **paraphonic**, matching the original web prototype: many voices, one filter
- 8-voice polyphony (adjustable via `numVoices` in `PluginProcessor.h`)
- On-screen piano keyboard (also responds to MIDI input / your DAW's
  virtual keyboard)
- All parameters exposed via `AudioProcessorValueTreeState` so the host DAW
  can automate them and save/recall presets

## Requirements

- CMake 3.22+
- A C++17 compiler (Xcode / MSVC / GCC or Clang)
- [Ninja](https://ninja-build.org/) (recommended, used by the CI workflow)
- Internet access on first configure (JUCE is fetched automatically via
  `FetchContent` — no manual submodule setup needed)

### Linux only

Install the JUCE system dependencies first:

```bash
sudo apt-get install libasound2-dev libjack-jackd2-dev ladspa-sdk \
    libcurl4-openssl-dev libfreetype6-dev libx11-dev libxcomposite-dev \
    libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev \
    libxrender-dev libwebkit2gtk-4.1-dev libglu1-mesa-dev mesa-common-dev
```

## Building locally

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

Or with Ninja explicitly (faster, same on every OS):

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Built plugins are copied automatically to your system's standard plugin
folder (`COPY_PLUGIN_AFTER_BUILD TRUE` in `CMakeLists.txt`):

| OS      | VST3 location                                  |
|---------|-------------------------------------------------|
| macOS   | `~/Library/Audio/Plug-Ins/VST3/`                |
| Windows | `C:\Program Files\Common Files\VST3\`           |
| Linux   | `~/.vst3/`                                       |

The **Standalone** app (no DAW needed) is also built — useful for quick
testing — and can be found under `build/MySynth_artefacts/Release/Standalone/`.

## Building on GitHub (CI)

Push this repo to GitHub as-is. `.github/workflows/build.yml` runs a matrix
build on `macos-latest`, `windows-latest`, and `ubuntu-latest`, and uploads
the built plugin for each platform as a downloadable workflow artifact —
no local toolchain required.

## Project layout

```
MySynth/
├── CMakeLists.txt              # JUCE plugin target + FetchContent setup
├── Source/
│   ├── PluginProcessor.h/.cpp  # DSP: Synthesiser, voices, shared filter
│   └── PluginEditor.h/.cpp     # UI: knobs, waveform selector, keyboard
└── .github/workflows/build.yml # Cross-platform CI matrix build
```

## Architecture notes

- `SynthVoice` (in `PluginProcessor.h`) is the per-note unit: one
  `juce::dsp::Oscillator` + one `juce::ADSR`, created/reused per active
  MIDI note by `juce::Synthesiser` — this is the polyphonic part.
- `MySynthAudioProcessor::processBlock()` renders all active voices into
  one mixed buffer, then runs that buffer through a **single**
  `juce::dsp::StateVariableTPTFilter` (low-pass) before output — this is
  the paraphonic part, deliberately mirroring the original prototype's
  "one filter, many oscillators" signal chain.
- Want fully independent per-voice filtering instead? Move the
  `StateVariableTPTFilter` member from the processor into `SynthVoice`
  and apply it inside `renderNextBlock()` per voice.

## License

JUCE itself is licensed under the JUCE license (free for personal/small
commercial use under their tiers, GPL, or commercial license — see
https://juce.com/get-juce for details). This project's own code is yours
to license as you see fit.
