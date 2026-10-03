# PhaseTwin

**Automatic timing and polarity alignment, with built-in visual monitoring.**

PhaseTwin is a JUCE/C++ VST3 plugin that analyzes a target signal against an external-sidechain reference. It offers kick/bass and same-source microphone analysis, fractional-sample timing correction, automatic polarity evaluation, and manual control over the result.

Phase alignment helps signals combine more constructively by correcting timing and polarity differences that can cause cancellation. Before/after waveform displays, stacked and summed views, correlation measurements, confidence feedback, and matched-latency audition help you understand and evaluate the correction.

## Project status

**Current source version: 0.1.0 — initial development release.**

This repository contains source code, not a standalone application or an installer. Build the VST3 and load it in a compatible DAW.

Portable DSP, capture, scope, and preference tests have passing results recorded in [TEST_RESULTS.md](TEST_RESULTS.md). Native JUCE integration tests are supplied, but native compilation, pluginval, and DAW acceptance have not been completed in the development environment. The project is not yet certified as production-ready.

## Features

- **Two analysis profiles:** Kick + bass, and Same source / microphones.
- **Automatic timing and polarity:** signed sample-delay estimation, fractional refinement, and optional polarity reversal.
- **High-quality fractional delay:** 64-tap windowed-sinc interpolation, with causal guard samples.
- **Selective analysis:** allow timing changes, polarity changes, both, or measurement only.
- **Learn and hold:** reliable corrections are applied once and retained; unreliable analysis keeps the previous correction.
- **Manual adjustment:** timing trim and polarity override remain available after analysis.
- **Monitoring:** Before/After waveforms, Lines/Filled styles, Stacked/Summed views, L/R/mono selection, and shared amplitude scaling.
- **Stable scope:** reference-triggered captures, continuous Rolling mode, and Hold scope.
- **Tempo-aware duration:** note divisions from 1/64 through 1/1, or a manual millisecond window.
- **Audition and recovery:** matched-latency unaligned comparison, Cancel analysis, and one-level Undo.
- **Session recall:** learned correction, parameters, undo history, and scope preferences are saved with the plugin.

New instances default to **Kick + bass**, **Filled waveforms**, **Stacked view**, and **0.0 dB output gain**. Saved sessions retain their existing settings.

## Quick start

1. Insert PhaseTwin on the **target track A**—for example, bass, a second microphone, or an amplifier recording.
2. Route the **reference track B** into the plugin's external sidechain—for example, kick, the reference microphone, or a DI recording. Both input meters should show signal.
3. Choose the appropriate analysis profile.
4. Play a representative section and press **Learn** in kick/bass mode or **Analyze** in same-source mode.
5. Read the result and confidence, then use **Hear unaligned** to compare the corrected and neutral paths.
6. Adjust timing trim or manual polarity if needed. Analyze again when the material or routing changes.

### Kick + bass

Use the bass as A and the kick as B. The learner captures four seconds, detects kick events, and evaluates low-end interaction across those hits. It needs at least three detected kicks and checks agreement on hits excluded from fitting before accepting a new correction.

A bassline with changing notes may not have one useful global correction. Listen across the whole phrase rather than optimizing a single kick or chasing the highest score.

### Same source / microphones

Use this profile for related recordings such as two microphones on an instrument or a DI/amp pair. A two-second session collects complete analysis windows and estimates a consistent timing/polarity relationship through normalized cross-correlation and fractional peak refinement.

Weakly related signals, competing reflections, periodic ambiguity, or offsets at the search boundary can cause rejection. A rejected result leaves the previous correction intact.

## Controls

| Control | Purpose |
| --- | --- |
| Learn timing / Learn polarity | Choose which corrections analysis may change. Both off makes the action **Measure**. |
| Timing trim | Add a manual offset to the learned timing, in milliseconds. |
| Search limit | Set the timing search from ±1 to ±20 ms. Total applied correction is also limited to ±20 ms. |
| Manual polarity flip | Reverse the learned polarity result without overwriting it. |
| Analysis focus | Set the analysis-only low-pass cutoff from 50–500 Hz in kick/bass mode. Output remains full-band. |
| Input correlation gate | Same-source window threshold, initially **0.65**. It is disabled and unused in kick/bass learning. |
| Continuous tracking | Optional rolling adaptation in same-source mode. Off by default. |
| Lock correction | Prevent new automatic correction updates. Starting a new analysis explicitly unlocks it. |
| Add reference B | Add B to the output: 0 = target A only; 1 = A plus full-amplitude B. Analysis still receives B at zero. |
| Output gain | Apply the same gain to corrected and unaligned audition paths. Default and double-click reset: **0.0 dB**. |
| Hear unaligned | Audition neutral timing/polarity at matched latency and output gain. Learned settings remain intact. |
| Cancel | Stop the active analysis and hold the existing correction. |
| Undo last analyze | Restore one prior learned timing/polarity correction, preserving current manual controls. |
| Reset alignment | Clear learned correction and manual timing/polarity overrides; clear undo history. |

**Positive timing offsets advance A relative to B; negative offsets delay A further.** Relative advance is made causal through the plugin's reported latency.

The Kick + bass preset sets the profile, 180 Hz focus, and learning permissions. It turns continuous tracking off and unlocks updates. It preserves the same-source correlation gate, output gain, and held correction until another reliable result is learned.

Cancel and Undo requests are processed on the next audio callback. If playback is stopped, resume processing to complete the request.

## Waveform monitoring

Both panels use one shared automatic amplitude scale. They show the captured signals before output mixing and gain.

| View | Before | After |
| --- | --- | --- |
| **Stacked: A & B** | Individual input A and reference B traces | Corrected—or neutral-audition—A and latency-matched B traces |
| **Summed: A + B** | Unity sum of input A and reference B | Unity sum of corrected—or neutral-audition—A and latency-matched B |

The Before and After panels remain vertically separated in both modes. Mint represents A, amber represents B, and violet represents the sum.

**Summed is a visual unity-sum comparison.** It does not enable audio summing and is independent of Add reference B and Output gain. Sums are calculated sample by sample before envelope decimation, preserving actual cancellation in long views. For mono targets, the reference display uses the same channel fold-down as the analysis/output path.

- **Lines** displays waveform outlines and min/max peaks.
- **Filled** adds translucent signed waveform areas while retaining the outlines.
- **Show A / Show B** controls trace visibility in Stacked mode. These controls are disabled in Summed mode so both signals always contribute to the sum.
- **Reference trigger** holds the latest complete capture until another reference transient produces a complete replacement.
- **Rolling** scrolls continuously; **Hold scope** freezes the picture only.
- **DAW sync** uses the host tempo for window duration, with a labeled 120 BPM fallback if tempo is unavailable.
- **Note divisions** are 1/64, 1/32, 1/16, 1/8, 1/4, 1/2, and 1/1. At 120 BPM, 1/4 is 500 ms and 1/1 is 2000 ms.
- **Manual window** supports 2–4000 ms. All scope durations are capped at four seconds.

Tempo sync controls duration, not bar position or time-signature alignment. Stable captures remain visible through silence and transport stops. Hold is temporary; other view preferences are recalled when reopening the editor.

## Scores and confidence

**Same-source score** is positive signed zero-lag correlation multiplied by 100. The output verifier independently measures the processed signal after correction has settled; it does not verify the entire stereo field from one selected analysis channel.

**Kick candidate score** measures weighted low-end interaction around the detected kicks: 50 represents neutral interaction, lower values indicate destructive interaction, and higher values indicate constructive interaction. Before/candidate scores describe the learning capture, not continuous output verification.

**Confidence** is an evidence-quality heuristic, not a probability of success. High correlation or score alone does not prove the correction will improve the mix. Level balance, changing notes, reflections, stereo relationships, and musical timing still require listening.

## Build with VS Code on Windows

VS Code can be the editor and terminal; the compiler is supplied separately by the C++ build tools.

Requirements:

- Visual Studio 2022 Build Tools with **Desktop development with C++**, MSVC, and the Windows SDK.
- **CMake 3.22 or newer**.
- A **JUCE 8 source checkout** containing `CMakeLists.txt`.
- Optional VS Code C/C++ and CMake Tools extensions.

Open the project directory containing this README and `CMakeLists.txt` in a developer terminal. Configure and build:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DJUCE_DIR="C:/dev/JUCE"
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Expected plugin output:

```text
build/PhaseTwin_artefacts/Release/VST3/PhaseTwin.vst3
```

A `.vst3` is a plugin bundle, not a standalone `.exe`. Copy the complete bundle into your VST3 directory, then rescan plugins in the DAW. The project does not install it automatically.

### Build and validate

With a local `pluginval.exe`, the supplied script builds Release x64, runs all ten CTest suites, and validates the plugin at strictness levels 5 and 10:

```powershell
./BUILD_AND_VALIDATE.ps1 -JuceDir "C:/dev/JUCE" -PluginvalPath "C:/tools/pluginval.exe"
```

Logs are written to `build-release/logs`. The script stops on a failed step. Finish the host/listening checks in [RELEASE_CHECKLIST.md](RELEASE_CHECKLIST.md) before treating a build as production-ready.

### Portable tests without JUCE

The nine portable test suites can also be built independently:

```sh
cmake -S . -B build-dsp -DPHASETWIN_DSP_ONLY=ON
cmake --build build-dsp --config Release --parallel
ctest --test-dir build-dsp -C Release --output-on-failure
```

The full build additionally includes the native JUCE integration test. Recorded validation results and their limits are described in [TEST_RESULTS.md](TEST_RESULTS.md).

## Processing limits and routing

- Timing search and total correction are currently **±20 ms**. Larger track offsets need preliminary alignment in the DAW.
- Target sample-rate coverage is **44.1–192 kHz**; native host coverage remains to be validated.
- Reported latency is `ceil(0.020 × sample rate) + 32` samples: approximately **20.667 ms at 48 kHz**.
- Correction changes use a 20 ms crossfade between fixed delay taps. During transition, the displayed offset interpolates between taps; the blended audio does not have one physical delay.
- Learn during real-time playback before offline export. Held correction can render without depending on fresh worker analysis.
- Check host sidechain/track delay compensation, especially when B also feeds the master separately.
- If Add reference B is enabled, avoid duplicating B through its original master feed. Preserve its sidechain send when changing routing.
- The output has no limiter. Unity gain is the default; internal summing can require manual headroom.
- Timing/polarity correction does not remove arbitrary frequency-dependent phase differences. Both stereo channels currently receive one shared correction.

## Roadmap

| Stage | Focus | Status |
| --- | --- | --- |
| **1** | Reliable two-signal timing/polarity alignment, manual controls, monitoring, audition, and session recall | Implemented in source; native release acceptance pending |
| **2** | Spectral coherence/phase visualization and optional frequency-dependent phase optimization | Planned |
| **3** | Multi-instance communication, reference selection, and multi-track group alignment | Planned |

Spectral correction, phase-rotation controls, multi-instance grouping, bass-mono processing, and a ducking shaper are not currently implemented.

See [PROJECT_SPECIFICATION.md](PROJECT_SPECIFICATION.md) for the staged design, [RELEASE_GAPS.md](RELEASE_GAPS.md) for outstanding priorities, and [RELEASE_CHECKLIST.md](RELEASE_CHECKLIST.md) for release acceptance.

## Contributing

Bug reports are most useful with the plugin/source version, DAW and OS versions, sample rate, buffer size, main/sidechain channel layout, reproduction steps, and relevant validation logs. Small reproducible audio examples help distinguish routing, timing-estimation, and musical-material issues.

Keep changes focused and run the relevant tests. New automatic corrections should expose their behavior clearly and be evaluated on independent material, not only the capture used for fitting.

## License

Original PhaseTwin source is licensed under the **MIT License**; see [LICENSE](LICENSE). JUCE and its third-party dependencies have separate license terms and are not included in this repository's license grant.
