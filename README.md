# PhaseTwin

**Automatic timing and polarity alignment, with built-in visual monitoring.**

PhaseTwin is a JUCE/C++ VST3 plugin that analyzes a target signal against an external-sidechain reference. It offers kick/bass and same-source microphone analysis, fractional-sample timing correction, automatic polarity evaluation, and manual control over the result.

Phase alignment helps signals combine more constructively by correcting timing and polarity differences that can cause cancellation. Before/after waveform displays, stacked and summed views, correlation measurements, confidence feedback, and matched-latency audition help you understand and evaluate the correction.

## Project status

**Current source version: 1.14.0 — release candidate.**

This repository contains source code, not a standalone application or an installer. Build the VST3 and load it in a compatible DAW.

Portable DSP, capture, scope, and preference tests have passing results recorded in [TEST_RESULTS.md](TEST_RESULTS.md). Native JUCE integration tests are supplied, but native compilation, pluginval, and DAW acceptance have not been completed in the development environment. The project is not yet certified as production-ready.

## Features

- **Two analysis profiles:** Kick + bass, and Same source / microphones.
- **Automatic timing and polarity:** signed sample-delay estimation, fractional refinement, and optional polarity reversal.
- **High-quality fractional delay:** 64-tap windowed-sinc interpolation, with causal guard samples.
- **Modern editor:** dark layered panels, mint accents, styled buttons/sliders and visible keyboard focus.
- **Automatic correction mode:** Timing + polarity (default), Preserve polarity, or Preserve timing.
- **Learn and hold:** reliable corrections are applied once and retained; unreliable analysis keeps the previous correction.
- **Manual adjustment:** timing trim and polarity override remain available after analysis.
- **Monitoring:** Before/After waveforms, Lines/Filled styles, Stacked/Summed views, L/R/mono selection, and shared amplitude scaling.
- **Stable scope:** reference-triggered captures, continuous Rolling mode, and Hold view.
- **Tempo-aware duration:** note divisions from 1/64 through 1/1, or a manual millisecond window.
- **Audition and recovery:** matched-latency unaligned comparison, Cancel analysis, and 128-step Undo/Redo.
- **Session recall:** learned correction, parameters, and scope preferences are saved with the plugin.

New instances default to **Kick + bass**, **Filled waveforms**, **Stacked view**, and **0.0 dB output gain**. Saved sessions retain their existing settings.

## Quick start

1. Insert PhaseTwin on the **target track A**—for example, bass, a second microphone, or an amplifier recording.
2. Route the **reference track B** into the plugin's external sidechain—for example, kick, the reference microphone, or a DI recording. Both input meters should show signal.
3. Choose the appropriate analysis profile.
4. Play a representative section and press **Learn** in kick/bass mode or **Analyze** in same-source mode.
5. Read the result and confidence, then use **Hear unaligned** to compare the corrected and neutral paths.
6. With Preview before apply enabled (default), inspect proposed timing, polarity and estimated improvement, then click Apply recommendation. The held correction stays unchanged until Apply.
7. Adjust timing trim or manual polarity if needed. Analyze again when the material or routing changes.

### Kick + bass

Use the bass as A and the kick as B. The learner captures four seconds, detects kick events, and evaluates low-end interaction across those hits. It needs at least three detected kicks and checks agreement on hits excluded from fitting before accepting a new correction.

A bassline with changing notes may not have one useful global correction. Listen across the whole phrase rather than optimizing a single kick or chasing the highest score.

### Same source / microphones

Use this profile for related recordings such as two microphones on an instrument or a DI/amp pair. A two-second session collects complete analysis windows and estimates a consistent timing/polarity relationship through normalized cross-correlation and fractional peak refinement.

Weakly related signals, competing reflections, periodic ambiguity, or offsets at the search boundary can cause rejection. A rejected result leaves the previous correction intact.

## Controls

| Control | Purpose |
| --- | --- |
| Automatic correction | **Timing + polarity** (default) allows both. **Preserve polarity** changes timing only. **Preserve timing** changes polarity only. Choices are mutually exclusive; held corrections and manual controls remain intact. |
| Timing trim | Add a manual offset to the learned timing, in milliseconds. |
| Search limit | Set the timing search from ±1 to ±20 ms. Total applied correction is also limited to ±20 ms. |
| Manual polarity flip | Reverse the learned polarity result without overwriting it. |
| Analysis focus | Set the analysis-only low-pass cutoff from 50–500 Hz in kick/bass mode. Output remains full-band. |
| Input correlation gate | Same-source window threshold, initially **0.65**. It is disabled and unused in kick/bass learning. |
| Continuous tracking | Optional rolling adaptation in same-source mode. Off by default. |
| Lock correction | Prevent new automatic correction updates. Starting a new analysis explicitly unlocks it. |
| Add reference B | Add B to the output: 0 = target A only; 1 = A plus full-amplitude B. Analysis still receives B at zero. |
| Sidechain ducking | Optional, off by default. Latency-matched reference B controls stereo-linked attenuation of target A after alignment. B remains unducked. |
| Ducking amount slider | 0–100%; maximum reduction is 0–24 dB. Default 50% (up to 12 dB). The reduction meter shows actual attenuation. |
| Ducking harshness slider | 0–100%; changes attack from 15 to 0.3 ms, release from 250 to 60 ms, and the detector knee. Default 50%. |
| Output gain | Apply the same gain to corrected and unaligned audition paths. Default and double-click reset: **0.0 dB**. |
| Hear unaligned | Audition neutral timing/polarity at matched latency and output gain. Learned settings remain intact. |
| Cancel | Stop the active analysis and hold the existing correction. |
| Undo / Redo | Move backward or forward through up to 128 audio edits, including applied corrections, manual controls, ducking and rotation. |
| Reset alignment | Clear learned correction and manual timing/polarity overrides; the reset itself is reversible. |

**Positive timing offsets advance A relative to B; negative offsets delay A further.** Relative advance is made causal through the plugin's reported latency.

The Kick + bass preset sets the profile and 180 Hz focus while retaining the selected automatic correction mode. It turns continuous tracking off and unlocks updates. It preserves the correction mode, the same-source correlation gate, output gain, and held correction until another reliable result is learned.

**Preserve timing keeps any previously learned advance/delay and manual trim.** To retain your original programmed kick/bass timing, first Reset alignment, then choose Preserve timing and Analyze. Switching modes does not reset held corrections. Preserve polarity similarly keeps the existing polarity. This is polarity inversion, not continuous phase rotation.

Older timing-only and polarity-only presets migrate to the matching mode. Legacy measure-only presets load in Preserve timing with Lock correction enabled; starting Analyze explicitly unlocks and uses the displayed mode. The old permission parameters remain as legacy state fields, but new automation should use Automatic correction.

Cancel and restoration of learned corrections run on the next audio callback. Undo/Redo updates parameter settings immediately; while stopped you can navigate several steps and the latest requested correction takes effect when processing resumes. A pending Reset finishes on the next callback before it can be undone.

Ducking uses a detector region from −36 to −12 dBFS; Detector sensitivity applies −24 to +24 dB of detector-only gain, so reference level affects the response. It does not boost reference audio. It is envelope-based, not a drawn volume-shaping curve. Controls and enable changes are smoothed, and ducking adds no latency. Analysis and scores remain pre-ducking; the After scope can show pre- or post-ducking. The reduction meter describes the separate volume effect. Hear unaligned keeps ducking active for a fair comparison of alignment. Host bypass returns to the neutral, unducked target.

## Compact controls

The main workflow stays visible: profile, correction mode, preview, manual timing/polarity, lock and output mix/gain. Expand **Analysis & groove**, **Ducking**, **Advanced ducking**, **Phase rotation** or **Audition** to access their settings. All sections start collapsed; their expansion state is saved with the session. Effect headers show their active status even when closed. Collapsing only hides controls—it never disables processing or resets parameters.

The controls area reflows and scrolls when needed, keeping the waveform/spectrum visible. Wheel scrolling does not change slider values; drag or use numeric entry to adjust them. Default editor size is 1080×960, with a 940×900 minimum. The output meter, Analyze/Apply, Undo and unaligned comparison remain accessible outside the sections.

## Spectral phase monitoring and manual rotation

Click **Spectrum** to replace the waveform view with 64 logarithmic bands across 20 Hz–20 kHz. Amber shows raw target/reference phase; mint shows processed A versus latency-matched B, including enabled phase rotation, before ducking. 0° indicates matching phase; ±180° indicates opposition. Values are wrapped phase differences, not an unwrapped transfer-function plot. Before/After curves connect supported adjacent measurements and break at phase wraps or gaps. Isolated supported measurements retain markers. Relative joint-energy shading, a clear zero line, log-frequency guides and mouse-hover band ranges/phase/support values make the evidence readable. Before/After toggles hide either curve; Hold view freezes the spectral snapshot. Coherent atomic snapshots prevent mixing bands from different updates. Empty/faint bands have little joint energy or poor within-band phase concentration. Brightness is **not temporal coherence, calibrated confidence or proof of cancellation**. Low frequencies have limited resolution at high sample rates (8192-sample Hann-windowed FFT). The strongest analysis channel is selected automatically; this is independent of the waveform channel selector. Spectral FFTs run on the worker thread.

**All-pass phase rotation** defaults off and affects A only. Centre 20–2000 Hz and Q 0.2–2 control one second-order all-pass section. At a fixed setting it has unity magnitude and approximately 180° phase shift at its centre; it is not a constant phase-angle control. Parameters and enable transitions are smoothed over a 20 ms time constant. Intermediate dry/wet transitions can temporarily change level. No additional fixed latency is reported, but frequency-dependent group delay can reshape attacks and note tails; this can affect the groove even with Preserve timing enabled. Conservative tail reporting adds 300 ms for filter decay.

Rotation is **manual**, not fitted by Analyze. Timing/polarity recommendations use the source relationship and do not optimize the all-pass response. Hear unaligned, Timing only and Neither audition bypass rotation; Polarity only includes rotation when enabled. Recheck the After spectrum and listen across changing bass notes. Automatic multi-band spectral optimization remains future work.

## Output headroom and advanced ducking

The stereo output meter shows sample peaks **after ducking, reference mixing and output gain**, with a 0 dBFS marker and a decaying peak display. The CLIP indicator latches when a sample reaches/exceeds full scale. Click the meter (or press Space/Enter when focused) to clear the latch on the next audio callback; ongoing over-full-scale output will relatch it. The meter does not measure intersample/true peaks or limit output.

**Advanced attack / release** defaults off. Simple mode retains the Harshness-controlled timing. Advanced mode enables separate **Attack 0.1–100 ms** and **Release 10–1000 ms** sliders; Harshness still controls the knee. Detector sensitivity works in either mode, defaults to 0 dB and is smoothed over 5 ms. Separate attack/release settings default to 3/125 ms and recall independently. Actual gain also has the existing 1 ms smoothing.

The **Duck envelope** graph displays actual gain reduction over time—not a hypothetical editable curve. It shares the waveform capture and Hold; the numerical reduction readout updates live. Higher downward reduction means stronger ducking. Post-duck After view shows its effect on A and the summed signal.

## Groove preservation, preview and audition

**Preserve kick/bass groove** defaults on in kick/bass mode. It limits each new timing move around the held offset (default 2 ms; adjustable 0.1–5 ms) and rejects candidates that turn previously active low-band sections into quiet 2 ms bins between detected kicks. This analyzes the joint source energy, not the cancellation in their sum. It is a conservative heuristic, not note detection or a guarantee that every groove is preserved. It does not undo an existing large shift; Reset restores original timing. Turning it off restores the wider search. Same-source analysis does not use this guard.

**Preview before apply** defaults on. Analyze produces a pending recommendation. **Hear proposal** temporarily processes it so you can listen and inspect actual After waveforms, spectrum and output peaks. Toggle it off to return to the held correction; Apply recommendation commits it and creates an Undo entry. Disabling Preview restores immediate reliable application. New analysis, Reset, Cancel, manual/analysis setting changes or state reload invalidate a pending proposal. Proposals and Hear proposal are ephemeral and are not restored with the session. Temporary audition preserves held correction and Undo history. It temporarily overrides the component-audition selector and Hear unaligned, without changing their saved settings. Manual trim/flip and enabled rotation, ducking, output mix and gain remain as configured. Applying, invalidating or replacing a proposal ends its audition. Transitions use the existing smoothing; held/triggered displays update according to their capture policy—release Hold to see new audio. Apply is handled on the audio callback; resume playback if processing has stopped.

The preview reports learned offset (manual trim remains additional), learned polarity and estimated interaction/correlation improvement. Kick/bass estimates use the captured hit objective. Same-source estimates use linear fractional sampling of the last captured input frame; they are predictions, not independent live validation or calibrated confidence. Weak evidence, little measured overlap, no meaningful gain or rejected groove candidates produce **No useful correction** and retain the previous correction. Continuous same-source tracking also proposes instead of committing while Preview is on.

**Audition correction** selects Timing + polarity, Timing only, Polarity only or Neither. It includes the corresponding manual trim/flip, retains latency, and never overwrites stored correction. Existing Hear unaligned overrides it with neutral output; turn Hear unaligned off to hear the selection. Ducking stays active across audition modes. Timing/polarity changes crossfade over the existing 20 ms transition; comparisons are not instantaneous sample switches.

## Waveform monitoring

Click **Waveforms: Filled / Lines**, **Reference trigger / Capture: Rolling**, or **Scope: Stacked / Summed** to switch the display modes directly. All choices are saved in the session.

Both panels use one shared automatic amplitude scale. Before remains the input view. The After: Pre-duck / Post-duck button switches aligned A before/after ducking; Summed adds unducked B. Both views remain before output mixing and gain. The button is available when ducking is enabled, and its choice is saved. A 0–24 dB reduction history appears below the After waveform while ducking is enabled. It follows the same captured time window, trigger/rolling mode and Hold. Pre/post views share a scale computed from both signals, preventing toggle-driven rescaling.

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
- **Rolling** scrolls continuously; **Hold view** freezes the picture only.
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

With a local `pluginval.exe`, the supplied script builds Release x64, runs all twelve CTest suites, and validates the plugin at strictness levels 5 and 10:

```powershell
./BUILD_AND_VALIDATE.ps1 -JuceDir "C:/dev/JUCE" -PluginvalPath "C:/tools/pluginval.exe"
```

Logs are written to `build-release/logs`. The script stops on a failed step. Finish the host/listening checks in [RELEASE_CHECKLIST.md](RELEASE_CHECKLIST.md) before treating a build as production-ready.

### Portable tests without JUCE

The eleven portable test suites can also be built independently:

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

Automatic spectral correction, multi-instance grouping, bass-mono processing, and a drawn-curve ducking shaper are not currently implemented. Manual all-pass phase rotation and spectral phase monitoring are available. Envelope sidechain ducking is available.

See [PROJECT_SPECIFICATION.md](PROJECT_SPECIFICATION.md) for the staged design, [RELEASE_GAPS.md](RELEASE_GAPS.md) for outstanding priorities, and [RELEASE_CHECKLIST.md](RELEASE_CHECKLIST.md) for release acceptance.

## Contributing

Bug reports are most useful with the plugin/source version, DAW and OS versions, sample rate, buffer size, main/sidechain channel layout, reproduction steps, and relevant validation logs. Small reproducible audio examples help distinguish routing, timing-estimation, and musical-material issues.

Keep changes focused and run the relevant tests. New automatic corrections should expose their behavior clearly and be evaluated on independent material, not only the capture used for fitting.

## License

Original PhaseTwin source is licensed under the **MIT License**; see [LICENSE](LICENSE). JUCE and its third-party dependencies have separate license terms and are not included in this repository's license grant.

## Audio Undo / Redo

Up to **128 steps** are kept per plugin instance, including audio parameters, applied learned timing/polarity and Reset. A slider gesture is one step; the kick preset and Reset group their parameter changes. New audio edits after Undo discard the redo branch. Continuous tracking is grouped into one evolving step; host automation without gestures is coalesced until 250 ms of quiet (or an explicit Undo/Redo). DAW automation can reassert its values during playback; plugin history does not rewrite the DAW automation lanes.

Display changes—scope composition/style, Hold, tempo/window, channel visibility, spectrum view and section expansion—are excluded. **Preview before apply** is a workflow preference and also excluded. **Hear proposal** remains temporary and creates no entry; applying a proposal creates an audio edit. Saved audio comparison/audition switches affect sound and are included.

The edit history is session-local: loading a preset/session establishes a fresh baseline while recalling the saved sound and display. It is not serialized, and legacy one-level Undo metadata is ignored. Counts on the Undo/Redo buttons show available steps. Native JUCE/DAW integration remains a release gate; the portable history tests do not certify host behavior.

## Optional post-Apply verification

**Analysis & groove → Verify after apply** defaults **On**. After applying a recommendation or immediately applying reliable learning, the plugin waits for normal corrected audition and settled processing, then measures **fresh audio** against unaligned A at matched latency. It never changes the correction. Kick/bass checks four seconds of supported low-band kick events; Same source accumulates valid nonoverlapping correlation windows for at least one second, with a two-second capture timeout. Continue playback to finish.

The always-visible footer reports **Improved**, **Worsened**, **No clear difference**, or **Not reliably measurable**, with signed before/after interaction/correlation and heuristic support where available. A 0.02 difference is the significance threshold; improvement/worsening needs at least 75% directional agreement. Silence, insufficient overlap/hits, invalid input or conflicting evidence do not produce a success claim. The result describes the measured passage; it is not continuous monitoring or a calibrated confidence probability.

The check includes the actual phase-rotation path, but runs before ducking, reference mix and output gain, which could otherwise make alignment appear better through attenuation. Unaligned/component/proposal audition pauses and restarts the pending capture. Timing/polarity/rotation/analysis changes, new Analyze, Reset, Undo/Redo and state reload invalidate it. Turning the check off disables pending measurement; turning it back on arms the next Apply.

## Optional multiple-section analysis

1. Expand **Analysis & groove** and enable **Collect multiple sections**.
2. Play a representative passage and click **Collect section**. Each capture is four seconds in Kick/bass or two seconds in Same source. The held correction stays unchanged.
3. Move playback to another representative passage or bass-note range and collect again. Collect **2–4 sections**. The plugin cannot detect whether you chose distinct song locations.
4. Click **Evaluate sections**. It evaluates fitted candidates from the sections against every retained passage, requiring meaningful mean improvement, at least 75% improved sections and no section harmed by more than 0.02. Kick/bass also checks the groove cap and gap heuristic on every section.
5. With Preview enabled, hear the common proposal and inspect per-section gains before Apply. Without Preview, a reliable result applies immediately. Inconsistent/unreliable sections or lack of a safe common candidate leave the previous correction intact.

Kick/bass retains each complete filtered capture. Same source retains up to 64 FFT input frames per section and evaluates all supported retained windows; it does not concatenate different song passages into one waveform. Candidates come from per-section fits, rather than an exhaustive global timing search or spectral optimizer. This conservative workflow can reject a compromise you prefer by ear.

**Clear sections** discards analysis captures while retaining the held sound and audio history. Collection mode/settings changes, Cancel, Reset, Undo/Redo, applying a correction, routing/state/sample-rate changes clear the collection. Captures, pending proposals and verification results are transient and are not saved in presets/sessions. The two workflow toggles are saved but excluded from audio Undo/Redo.
