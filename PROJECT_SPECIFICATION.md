# PhaseTwin product specification

Accepted project direction: 3 October 2026. This document records the user's development brief and governs prioritization. Current source revision: 1.9.1 release candidate.

## Product goal

A professional time and phase alignment tool for related recordings: microphones, DI/amp pairs and other multi-track captures. Select a reference, analyze a target, inspect the recommendation, apply it and retain manual control. Improve constructive interaction and reduce cancellation without hiding the processing. Kick/bass remains a distinct musical interaction profile; its objective must not be conflated with identifying delay between related microphone signals.

No claim of equivalence to another product's algorithm, score or performance. Production release depends on native build, host validation, listening and measured quality.

## Stage 1 — reliable two-signal release

Pipeline: target/reference capture → signal diagnostics → normalized cross-correlation and coarse signed lag → fractional correlation refinement → polarity evaluation → evidence/confidence → selective application → independent output verification and matched-latency audition.

Required behavior:
- Main target A, external-sidechain reference B; explicit mono/stereo routing and signal diagnostics.
- Sample/fractional-sample timing, automatic polarity, manual trim/polarity, selectable permissions and measure-only analysis.
- Coherent learning sessions that hold previous correction when evidence is weak, periodic or contradictory. Confidence is evidence quality, not a guarantee of musical improvement.
- No allocation, waits, locks or analysis FFT in the audio callback. Background results belong to an immutable capture/settings revision and cannot overwrite newer settings.
- A high-quality causal fractional delay; predictable advance through reported latency; stereo-linked corrections; 20 ms fixed-tap crossfades with documented settling/queued-request behavior. During a blend the displayed offset is an interpolated diagnostic, not a single physical delay.
- Visible learned/trim/applied timing in samples and milliseconds, polarity, before/after correlation, confidence and latency. Triggered/rolling/held waveform views must represent the real captured data.
- Matched-gain, matched-latency original/corrected audition; manual and automatic corrections remain editable and recallable.
- Versioned state, deterministic held-correction rendering, correct latency/tail reporting and host bypass behavior.

Current implementation: these DSP primitives and pairwise source/UI workflows are present in source; portable tests pass. JUCE wrapper/editor, native state/callback integration, pluginval and real DAW acceptance are still unexecuted in this workspace. That distinction is a release blocker.

Current limits: ±20 ms search/total correction, 44.1–192 kHz target support, 8192-sample correlation frames and a two-second real-time learning session. Long microphone offsets need preliminary DAW alignment. Only one channel is selected for evidence and one common stereo correction applied. Reflections and microphone coloration may produce several meaningful optima; a simple delay cannot equalize a frequency-dependent phase response. Automatic adaptation is optional; learn/hold is the default.

Remaining Stage 1 gates:
1. Build the native VST3 and integration test, validate with pluginval, and complete RELEASE_CHECKLIST.md.
2. Verify microphone/DI/amp recordings across levels, SNR, transients, reflections, coloration, periodic material and stereo configurations. Compare independent held-out passages and listen to the actual sum.
3. Measure interpolation passband/error, settled null, transition behavior, denormals, CPU, allocations and callback deadlines on supported native systems.
4. Verify sidechain PDC, host bypass, automation, state restore, rate/block-size changes, transport loops, editor stress and offline export.
5. Tune capture duration/search limits only against measured performance. Wider ±100 ms-class support requires larger/adaptive analysis windows and a matching latency/ring redesign; a UI range change alone is insufficient.

Acceptance thresholds are scenario-specific. Synthetic known-delay tests must recover signed timing/polarity, reject ambiguous/unrelated inputs and independently verify corrected output. Real recordings need useful, repeatable correction and no misleading confidence or correlation claim. Stage 1 is not complete until the native and listening gates pass.

## Stage 2 — spectral analysis, then optional optimization

First add a diagnostic view: ensemble cross-spectrum, auto-spectra, magnitude-squared coherence and phase difference after removing the estimated linear delay/polarity. Show frequency regions, usable evidence and before/after behavior. Do not infer a trustworthy transfer phase from a single low-coherence FFT.

Then prototype bounded all-pass correction. Fit only supported frequency regions, regularize complexity, constrain stability/group delay, and test on held-out passages. Add phase bypass, manual limits and explicit display of each correction region. Keep optimization optional and off by default. A flat-magnitude filter can still smear transients or worsen another microphone relationship; coherence and waveform shape must both be evaluated.

Gates: unity-magnitude tolerance, stability during coefficient changes, impulse/group-delay response, independently measured spectral improvement, transient listening and predictable latency/state. Automatic filter fitting and spectral controls remain planned, not implemented in 1.6.

## Stage 3 — multi-instance groups

Use stable instance/group IDs, explicit membership and one timing reference. Preserve raw capture timestamps and define whether an analysis uses uncorrected or corrected audio; never allow accidental correction chains or feedback. Link each result to capture, reference, membership and settings revisions. Losing a reference must hold correction and explain the state.

Design host/process communication before implementing discovery: plugin instances may run in separate processes. A shared in-process registry alone is insufficient. Audio callbacks must not wait on discovery or transfers. A coordinator should work outside real-time processing, with bounded data exchange and a documented fallback for unsupported hosts.

Start with a star group anchored to one reference. Add graph/group optimization only after pairwise evidence, inconsistent relationships, reference changes and cumulative delay constraints are demonstrably handled. A unified view should expose track labels, reference choice, per-track evidence/correction, selective application and group audition.

Gates: lifecycle/reload/reorder/isolation tests, sample-timeline/PDC agreement, stale-result rejection, recovery after missing tracks, deterministic session recall, native CPU/memory scaling and full drum-session listening. Multi-instance discovery, communication and group optimization are not implemented in 1.6.

## Release discipline

BUILD_AND_VALIDATE.ps1 builds the source, runs all ten CTest suites (nine portable plus one native integration suite) and pluginval levels 5/10. Its successful execution is necessary but does not replace DAW/listening acceptance. TEST_RESULTS.md identifies what has actually run. Features must be labeled implemented, experimentally evaluated or planned; source presence must never be reported as host validation.

## Current workflow preference

Version 1.8 defaults new instances to Kick + bass and filled waveforms at the user’s request. Same-source/microphone alignment remains available. This default choice does not change the staged microphone/spectral/group development goals. Stacked and actual unity-summed Before/After views are available as display-only preferences.

## Optional sidechain ducking

Envelope ducking is separate from alignment: latency-matched B drives a stereo-linked gain applied to A only. Enable defaults off; Amount sets up to 24 dB depth, Harshness controls attack/release/knee. Analysis and waveform diagnostics remain pre-ducking. The detector is level-dependent (fixed −36 to −12 dBFS region). No drawn-curve shaper or additional latency is introduced. Automatic correction uses a three-mode selector: both corrections (default), Preserve polarity or Preserve timing. Preserve means keeping the current value; Reset restores original timing. Manual controls remain independent.
