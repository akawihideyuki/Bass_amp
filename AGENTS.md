# Bass_amp Agent Guide

This file is persistent development guidance for agents working in this repository.

## Product

Bass_amp is a Windows 11 standalone real-time bass amplifier simulator.

The v1.0 product specification is `docs/SPECIFICATION.md`.

## Stack

- C++20
- JUCE 9.0.1
- CMake 3.22+
- Windows standalone app
- ASIO enabled with JUCE
- WASAPI supported by JUCE
- AGPL-3.0-only project license

Do not introduce another application framework or audio framework without a concrete requirement.

## Architecture

Keep these responsibilities separate:

- `src/dsp/`: realtime amp, dynamics, EQ, cabinet and IR processing
- `src/tuner/`: pitch analysis
- `src/preset/`: persistent sound-state serialization
- `src/audio/`: audio-device persistence/helpers
- `src/diagnostics/`: realtime-safe diagnostic counters
- `src/MainComponent.*`: application orchestration and GUI

Do not move filesystem, JSON, dialogs, or UI logic into `BassAmpProcessor`.

## Realtime audio rules

The audio callback must not:

- perform heap allocation
- read/write files
- parse JSON/XML
- display dialogs
- call normal logging sinks
- block on mutexes
- wait for another thread

Preallocate DSP working memory in `prepare()`.

Atomic values or wait-free/single-producer-single-consumer structures are preferred for realtime communication.

If a lock is unavoidable around a JUCE object that requires serialization, the audio callback must use a non-blocking try-lock and safely skip that optional processing for the block when the lock is unavailable.

## DSP safety

All external parameter values must be clamped.

All processed samples must be protected from NaN/Infinity propagation.

The final limiter must remain in the normal processed path.

Device/startup changes must fade in rather than emit an abrupt buffer.

Do not hard-code calculations to 48 kHz. 48 kHz / 128 samples is the recommended operating point, not a code assumption.

## Bass-specific behaviour

Preserve the Low Clean architecture around ~120 Hz.

Do not turn the amp into a guitar-oriented model by removing clean low-frequency support.

Vintage / Modern / Aggressive use one shared engine with profiles/behavioural differences rather than three copy-pasted processors.

## ASIO

ASIO and WASAPI are both supported.

ASIO4ALL requires no dedicated backend; it appears as an installed ASIO driver.

Do not bundle ASIO4ALL.

## User IR

Do not add third-party IR binary files unless their redistribution terms have been explicitly verified.

User-selected WAV/AIFF IRs are supported through JUCE convolution.

Do not perform filesystem IR loading directly inside ordinary per-sample DSP code.

## Presets

Preset files are JSON and must contain `presetVersion`.

New parameters must have safe defaults so older presets continue loading.

Broken presets must fail gracefully without crashing.

## Tests

Before considering a change complete, run:

```powershell
cmake -S . -B build
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

Relevant changes should preserve tests for:

- 44.1 / 48 / 96 kHz
- 64 / 128 / 256 / 512 blocks
- NaN/Infinity protection
- extreme gain settings
- bypass
- pitch detection
- preset serialization

GitHub Actions Windows Build must be green before merging significant changes.

## Change policy

Prefer small cohesive changes.

Do not perform unrelated mass formatting.

Do not add dependencies when JUCE or a small local implementation is sufficient.

If realtime behaviour changes, explain expected latency/CPU consequences in the PR.

## Done

A code change is not done merely because it was committed.

For significant changes:

1. inspect affected code
2. build Release
3. run automated tests
4. inspect CI
5. state what still requires physical audio-hardware testing
