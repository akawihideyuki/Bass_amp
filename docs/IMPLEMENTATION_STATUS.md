# Bass_amp v1.0 implementation status

This document maps the v1.0 implementation to the agreed specification.

## Complete in v1.0

- [x] Windows standalone JUCE application
- [x] C++20 / CMake
- [x] JUCE 9.0.1 pinned by CMake FetchContent
- [x] ASIO support
- [x] WASAPI support
- [x] ASIO4ALL usable as an installed ASIO driver
- [x] Input/output device and channel selection
- [x] 48 kHz / 128 sample recommended operating point
- [x] Input gain and meters
- [x] Noise gate
- [x] Compressor
- [x] Vintage / Modern / Aggressive voicings
- [x] Drive
- [x] Character
- [x] ~120 Hz Low Clean split/recombine
- [x] 4-band EQ
- [x] DI / 1x15 / 4x10 / 8x10 cabinet profiles
- [x] User WAV/AIFF IR loading
- [x] JUCE partitioned convolution
- [x] OFF / 2x / 4x nonlinear-stage oversampling
- [x] Final limiter
- [x] Startup/device restart fade-in
- [x] Input/output clip hold indication
- [x] Bass tuner
- [x] Adjustable A4 reference
- [x] Factory presets
- [x] User JSON preset save/load/delete
- [x] `presetVersion`
- [x] A/B state comparison
- [x] Audio settings persistence
- [x] Advanced panel
- [x] Diagnostics panel
- [x] Callback deadline-miss counter
- [x] DSP latency display
- [x] Automated DSP tests
- [x] Pitch detector tests
- [x] Preset serialization tests
- [x] GitHub Actions Windows build/test
- [x] Windows executable uploaded as CI artifact
- [x] README build/use documentation
- [x] AGENTS.md
- [x] AGPL-3.0-only license notice

## Physical hardware verification still required

Automated CI cannot verify the analogue/audio-device path. The following are v1.0 acceptance checks to perform on a real Windows PC:

- native audio-interface ASIO driver opens correctly
- ASIO4ALL opens correctly when installed
- WASAPI fallback opens correctly
- instrument input channel mapping is correct
- 48 kHz / 128 samples is stable on the target interface
- 64 samples is evaluated as an optional low-latency setting
- no audible click occurs during normal parameter changes
- device switching fades in safely
- cabinet/voicing tonal balance is adjusted by ear
- tuner response is checked with a real bass, especially B0/E1
- user IR loading is auditioned with real cabinet IR files

Hardware-dependent tonal and latency tuning may be refined in v1.0.x without changing the architecture.
