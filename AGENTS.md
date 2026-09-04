# AGENTS.md - Bass_amp

## Project

Bass_amp is a Windows 11 standalone real-time electric-bass amplifier simulator.

The primary priorities are, in order:

1. Stable audio
2. Low latency
3. No dangerous bursts, NaN/Inf, or avoidable drop-outs
4. Good bass tone
5. Simple UX
6. CPU efficiency
7. Visual polish
8. Additional features

Read `docs/SPECIFICATION.md` before changing architecture or DSP behaviour.

## Toolchain

- C++20
- CMake 3.22+
- JUCE 9.0.1
- Windows 11 / Visual Studio 2022
- Standalone app only for v1
- License: AGPL-3.0-only

JUCE is fetched by CMake FetchContent and pinned to 9.0.1.

## Audio backends

- Enable JUCE ASIO with `JUCE_ASIO=1`.
- Use JUCE's bundled ASIO headers unless the project explicitly changes policy.
- WASAPI must remain available as fallback.
- ASIO4ALL is not bundled. It is treated as an installed ASIO driver.
- Do not add vendor-specific driver code without a concrete need.

## Real-time audio rules

Inside the real-time audio callback / DSP processing path, do not:

- allocate heap memory
- read or write files
- parse JSON
- update GUI widgets
- write directly to disk logs
- wait on mutexes
- load IR files
- perform network operations

Parameter values crossing from GUI to DSP should use atomics or another real-time-safe mechanism. Continuous audible parameters should be smoothed where practical.

## Architecture

Keep these responsibilities separated:

- `MainComponent`: GUI and JUCE AudioAppComponent integration
- `dsp/BassAmpProcessor`: reusable bass DSP core
- future `tuner/`: pitch analysis outside the audio callback
- future `preset/`: serialisation and migrations
- future `diagnostics/`: callback timing / device status

DSP code must not depend on concrete GUI classes.

## DSP policy

Default signal chain:

Input -> Gate -> Compressor -> Amp -> Low Clean recombination -> 4-band EQ -> Cabinet -> Limiter -> Master

Amp voicings share one engine and differ by parameter/profile behaviour. Do not copy the whole DSP chain into separate Vintage/Modern/Aggressive implementations.

Protect the output from invalid floating-point values. Extreme valid parameter settings must not crash or generate NaN/Inf.

## Build verification

After changes:

1. Configure with CMake on Windows.
2. Build Release.
3. If DSP changed, test silence and strong input mentally or with automated tests when available.
4. Confirm ASIO and WASAPI support were not accidentally disabled.
5. Check GitHub Actions.

Never report runtime audio hardware testing as completed unless it was actually performed on hardware.

## Scope control

Do not add these to v1 without explicit approval:

- VST3/AU/AAX
- neural amp capture/model inference
- DAW or multitrack features
- cloud accounts
- auto updater
- macOS/Linux support

Prefer a complete, reliable bass amp over a wide but unfinished effect suite.
