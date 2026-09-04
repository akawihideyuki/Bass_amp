# Bass_amp

Windows 11向けのリアルタイム・ベースアンプシミュレーターです。

エレキベースをUSBオーディオインターフェースへ接続し、ASIOまたはWASAPI経由で低遅延に演奏する用途を想定しています。

## v1.0 features

- Windows 11 standalone application
- ASIO / WASAPI audio I/O
- ASIO4ALL support as a normal installed ASIO driver
- Input/output channel selection
- Vintage / Modern / Aggressive voicings
- Noise gate
- Compressor
- Drive and Character controls
- Bass-specific Low Clean blend
- 4-band EQ
- DI / 1x15 / 4x10 / 8x10 cabinet profiles
- User WAV/AIFF cabinet IR loading
- OFF / 2x / 4x amp-stage oversampling
- Output limiter and clip indicators
- Bass tuner with adjustable A4 reference (430-450 Hz)
- Factory and user JSON presets
- A/B sound comparison
- Audio device settings persistence
- Audio diagnostics and callback deadline-miss counter
- DSP / tuner / preset automated tests
- Windows CI build artifact

The complete design specification is in [`docs/SPECIFICATION.md`](docs/SPECIFICATION.md).

## Recommended connection

```text
Electric Bass
    ↓
Audio Interface Hi-Z / Instrument Input
    ↓
Bass_amp
    ↓
Audio Interface Headphone / Line Output
```

For an audio interface with a native ASIO driver, use that driver first.

If no native ASIO driver is available, ASIO4ALL or WASAPI can be selected from **AUDIO SETUP**.

## Recommended starting audio settings

- Sample rate: 48 kHz
- Buffer size: 128 samples
- Input: mono instrument channel
- Output: stereo

64 samples may reduce latency further on sufficiently stable hardware. If clicks/dropouts occur, try 256 samples.

## Build requirements

- Windows 11 64-bit
- Visual Studio with **Desktop development with C++**
- CMake 3.22 or newer
- Git
- Internet connection during the first configure step (JUCE 9.0.1 is fetched automatically)

## Build

From a Developer PowerShell or terminal with the MSVC toolchain available:

```powershell
git clone https://github.com/akawihideyuki/Bass_amp.git
cd Bass_amp

cmake -S . -B build
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

The executable is generated under the CMake build artefacts directory, typically:

```text
build/BassAmp_artefacts/Release/Bass_amp.exe
```

GitHub Actions also uploads a **Bass_amp-v1.0-windows** artifact after a successful Windows build.

## First launch

1. Open **AUDIO SETUP**.
2. Select ASIO or WASAPI.
3. Select the bass input channel and output channels.
4. Start with 48 kHz / 128 samples where supported.
5. Confirm input level using the **IN** meter.
6. Choose a voicing and cabinet.
7. Adjust **MASTER** from a safe level.

The last successful audio-device configuration is saved under the user's application-data folder and restored on the next launch when possible.

## Controls

### Main controls

- **INPUT**: input trim
- **GATE**: noise-gate threshold
- **COMP**: simplified compressor amount
- **DRIVE**: amp saturation amount
- **CHARACTER**: harmonic / midrange character
- **LOW CLEAN**: restores clean bass fundamentals under the distorted path
- **BASS / LOW MID / HIGH MID / TREBLE**: 4-band EQ
- **MASTER**: final volume

### Voicing

- **VINTAGE**: rounder, softer saturation
- **MODERN**: tight and detailed
- **AGGRESSIVE**: stronger asymmetric harmonics and upper-mid attack

### Cabinet

- DI / OFF
- Compact 1x15
- Punch 4x10
- Massive 8x10
- USER IR

### Advanced

- Oversampling OFF / 2x / 4x
- Tuner A4 reference
- Limiter threshold
- User IR loading
- Audio diagnostics

## Presets

Factory presets:

- Clean Studio
- Warm Vintage
- Modern Punch
- Rock Grind
- Aggressive Pick

User presets are JSON files with a `presetVersion` field and can be saved, loaded, and deleted from the app.

## A/B comparison

The **A** and **B** buttons store two independent parameter states. Switching sides stores the current side before restoring the other one.

## User IR

Use **ADVANCED → LOAD USER IR** to select a WAV or AIFF cabinet impulse response.

The loaded IR is processed with JUCE partitioned convolution. Bass_amp itself does not bundle third-party cabinet IR files.

## Diagnostics

The Advanced panel displays:

- Driver and device
- Sample rate
- Buffer size
- Input/output latency reported by the driver
- DSP oversampling latency
- CPU usage
- Peak audio-callback time
- Callback deadline misses
- Loaded IR

A deadline miss means the audio callback exceeded the time available for the current buffer and is a useful indicator when tracking clicks/dropouts.

## Testing

The automated test executable checks:

- 44.1 / 48 / 96 kHz
- 64 / 128 / 256 / 512 sample blocks
- OFF / 2x / 4x oversampling
- finite DSP output (NaN/Infinity guard)
- extreme-gain safety
- bypass behaviour
- low bass pitch detection
- JSON preset round-trip and broken JSON rejection

Real ASIO/WASAPI latency and sound quality still require testing with actual audio hardware.

## Architecture

```text
Audio Device (ASIO / WASAPI)
          ↓
MainComponent / Audio callback
          ↓
BassAmpProcessor
  Gate
  Compressor
  120 Hz Low Clean split
  Amp saturation + oversampling
  4-band EQ
  Cabinet / User IR
  Master
  Limiter
          ↓
Audio Output

Raw input ──→ TunerEngine
UI state ──→ PresetManager
Callback timing ──→ Diagnostics
```

The DSP layer does not depend on the UI so that future plugin or alternate-UI versions can reuse the core.

## Scope after v1.0

Not included in v1.0:

- VST3
- macOS/Linux
- neural amp capture/modeling
- chorus/octaver/fuzz/envelope-filter effects
- looper
- multitrack recording
- cloud presets

These are intentionally kept out of the first stable standalone version.

## License

Bass_amp is released under **GNU Affero General Public License v3.0 only (AGPL-3.0-only)**.

JUCE and ASIO-related components retain their respective upstream licenses.
