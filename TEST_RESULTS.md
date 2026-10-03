# PhaseTwin validation history — current version 1.8.1

## Passed automated checks

- **20 original DSP assertions**, including the added cubic-interpolation/ring-wrap regression. Compiled with g++ / C++17 and `-O2 -Wall -Wextra -Wpedantic -Werror`, with no warnings.
- **16 end-to-end configurations**: 44.1/48/96/192 kHz × positive/negative fractional lag × normal/inverted polarity. The test estimates lag/polarity from the input, applies that estimate through the exact `AudioEngine` used by the plugin, waits for smoothing, and independently measures the processed output. All passed.
- Across those 16 cases, maximum residual lag was **0.022337 samples**, minimum processed zero-lag correlation was **0.999475**, and maximum relative RMS null error was **3.591%**. This finite test signal has non-harmonic frequencies from roughly 180 Hz to 10.7 kHz. These are test results, not guarantees for arbitrary audio or frequencies near Nyquist.
- A deliberately wrong manual correction produces a 240-sample residual, and a newly requested correction reports that it is still settling.
- NaN/infinity estimator frames are rejected.
- Scope queue capacity, full/empty behavior and ordering pass. A real producer/consumer thread test transfers 3000 packets and checks all 8 traces for torn or reordered samples.
- The regression executable reports 131317 assertions; most are per-sample stereo consistency checks. This is not a count of independent test scenarios.
- A separate **UndefinedBehaviorSanitizer** regression build passed without reported undefined behavior. Its output is in `validation/regression_ubsan.txt`.

CSV measurements, the console logs and a plot from the actual tested audio engine are in `validation/`. The waveform plot is not a screenshot of the JUCE plugin.

## Code-review fixes

The editor no longer calls a raw peak estimate “Aligned” while the applied delay is moving. Verification now uses the actual processed paths and requires a settled window, an unambiguous residual below 0.5 sample, positive polarity and sufficient zero-lag correlation. It continues with frozen/manual correction.

Analysis results are committed as one ownership-controlled mailbox transaction on the audio thread. Job generation/config snapshots reject results from an earlier route, mode, trim, confidence/search setting or restored project state. Lag and polarity no longer arrive as independent worker writes. These wrapper changes were reviewed but could not be exercised in a JUCE host here.

Four-point fractional interpolation reduces the prior linear-interpolation high-frequency loss. Sample input sanitization prevents non-finite audio from contaminating delay history. The GUI is fed by a bounded SPSC queue, so the audio producer never waits for repainting.

## Not verified here

**No JUCE/VST3 build or DAW execution was possible**: JUCE and CMake are unavailable in this environment. The original package and this update remain source projects.

The GUI, JUCE API/build compatibility, VST3 validation/scanning, mono/stereo bus negotiation, host bypass, session restore, external-sidechain latency compensation, real-time CPU use and offline rendering still need tests on the target machine. The standalone tests cover the shared audio engine and scope transport, not the JUCE wrapper.

The earlier AddressSanitizer/LeakSanitizer run could not complete because process information under `/proc` was inaccessible. No successful ASan result is claimed.

## Target-system acceptance checks

1. Build `PhaseTwin_VST3`; build the two test targets and run `ctest`.
2. Run pluginval and load it in the DAW.
3. Use duplicate broadband/transient audio, add known signed delays and invert polarity. Confirm that the after traces overlap and the measured residual returns near zero after settling.
4. Choose a deliberately wrong manual lag; output verification must fail.
5. Check mono/stereo main and sidechain combinations, disabled/silent sidechains, small/large blocks, sample-rate changes and L/R selection.
6. Toggle Auto, Freeze, manual trim/polarity and bypass while playing. Save/reload the project and confirm the held correction.
7. Verify routing/host delay compensation; avoid hearing B twice when using the internal sum. Freeze before offline render.


## v1.2 update

The **17 new workflow tests** pass: tempo-to-ms conversion, missing/invalid BPM fallback, manual duration, 4-second cap, opposite-polarity stereo detection, right-only channel selection, explicit silent/unrelated/periodic diagnostics, a full 4000-ms history at 192 kHz, single-sample transient preservation, real mono cancellation, dropped-packet continuity and sample-rate changes. They also pass under UndefinedBehaviorSanitizer. The 20 DSP assertions and 16 end-to-end alignment configurations pass again after the estimator update.

The former Auto toggle started on, so clicking it could disable continuous correction; the new Align Now action removes that ambiguity. Input meters/status messages expose missing sidechain audio and rejected analysis. This addresses source-level problems but does **not** establish the cause of the reported DAW failure, which has not been reproduced here.

The live DAW BPM read, Align Now GUI callback, parameter notifications, RMS meters and stereo wrapper capture were reviewed in source but remain uncompiled/untested in JUCE. The portable tests cover the timing/history policies and estimator/channel selection, not a real host callback. Existing v1.1 test logs and waveform plot remain historical evidence for the shared delay engine.


## v1.3 — current implementation

**14 new learning tests pass** with g++ C++17, `-O2 -Wall -Wextra -Wpedantic -Werror`, and separately with UndefinedBehaviorSanitizer. They cover no-evidence/single-peak refusal, a stable fractional candidate, unstable lag rejection, conflicting polarity rejection, negative lag/polarity consensus, eviction of stale rolling evidence, separate score semantics, high-frequency/DC rejection in the analysis-only band, band-aware stereo channel selection, known low-end transient timing/polarity recovery, retained periodic-ambiguity rejection, and neutral A/B at matched baseline latency.

The learning test initially exposed the fixed 8-sample ambiguity radius rejecting valid broad low-frequency peaks. Low-end analysis now excludes a main-lobe neighborhood derived from sample rate and focus cutoff; a dedicated periodic-tone test confirms that separated equivalent peaks still reject alignment. The channel-selection test also checks that a louder high-frequency channel does not win a low-end measurement.

All 20 original DSP assertions, 17 workflow assertions and 16 end-to-end configurations pass again with the updated common estimator and engine. Existing CSV/PNG evidence still describes the shared engine's source-alignment tests, not the new GUI or a musical kick/bass validation. `learning_results.txt` and `learning_ubsan.txt` contain the new results.

The independent score is positive zero-lag correlation × 100. Learn confidence is a window-level evidence/uniqueness/stability heuristic. It is not a success probability and does not imply identical performance or scoring to PhaseLock. The low-end transient test uses related signals with a known ground truth; it does not establish that arbitrary kicks and changing bass notes always have a meaningful global correction.

The custom control panel, two-second session timer, selective application, MIDI/host parameter notifications, neutral comparison crossfade, freeze/reset behavior, old-state migration and DAW tempo read were reviewed in source but **not compiled or run with JUCE**. The portable tests exercise the common learning/filter/neutral-path primitives, not a real editor click or host session.

Before adopting this build, run the four CTest targets, build/validate VST3, and test the full real-time Learn → apply → hold → A/B → reset sequence in the target DAW. Specifically check selective timing-only/polarity-only, measure-only with poor confidence, disabled/silent sidechain, profile changes during learning, old/new session restore, continuous tracking, host bypass and offline bounce. No claim of complete bug freedom or production readiness is made.

## v1.4 — kick learning and release tooling

All five portable suites were rebuilt with C++17, -O2, -Wall -Wextra -Wpedantic -Werror and passed. Fresh logs end in `_v14.txt`. Kick learning additionally passes UndefinedBehaviorSanitizer (`kick_ubsan.txt`). New tests cover consistent known timing/polarity, interaction improvement, timing-only/polarity-only preservation, measure-only, silence, conflicting held-out hits, invalid input and exact four-second capture at 48 kHz. These are synthetic tests, not certification on real musical recordings.

Kick mode now fits correction on alternating kick hits in a four-second low-end capture and rejects candidates that fail per-hit consistency, including held-out hits. Same-source rolling tracking no longer drives kick correction. An explicit 1/4-note scope option replaces the equivalent but unclear '1 beat' label; existing quarter-note conversion test confirms 500 ms at 120 BPM.

The native JUCE integration test target and BUILD_AND_VALIDATE.ps1 are included but **not compiled or executed here**: this workspace lacks CMake and JUCE. The native test checks the real processor's kick capture/worker/application flow, finite output, saved correction and sample-rate scaling. The script builds all targets, runs six CTest suites and pluginval levels 5/10. GUI, validator, host automation, bypass, PDC and real DAW listening remain release gates in RELEASE_CHECKLIST.md. This revision is a source release candidate, not a production-certified binary.

## v1.5 — scope and UI polish

Six portable suites pass under C++17 with -O2 -Wall -Wextra -Wpedantic -Werror. New scope tests cover explicit quarter/whole-note durations, signed samples-to-ms conversion, unpublished partial captures, complete publication, an unmoving frame through silence, subsequent kick refresh, packet gaps, routing invalidation, exact slice boundaries and keeping the previous captured duration when requesting another division, and a full four-second stable capture at 192 kHz. Scope tests also pass UndefinedBehaviorSanitizer. Fresh logs end in `_v15.txt`.

All new triggering/history work runs on the editor thread. Audio processing/correction was not changed in this revision. JUCE UI compilation, actual text layout/tooltip rendering, native integration and DAW behavior remain untested here. The target script now runs seven CTest suites including the unexecuted native test. No production certification is implied.

## v1.6 — Stage 1 alignment foundation

Eight portable suites pass with C++17, -O2 -Wall -Wextra -Wpedantic -Werror. Fresh logs end in `_v16.txt`. New microphone tests cover 16 rate/delay-sign/polarity combinations, each with five frames containing level mismatch, DC, independent noise and a weaker reflection. They also reject equal competing reflection peaks and unrelated signals. These are synthetic microphone-like scenarios, not recordings from real microphone sessions.

The coarse NCC peak now receives Fourier-correlation derivative refinement. The original 16 independent end-to-end configurations still pass, with the input lag tolerance tightened to 0.02 sample. In this finite dataset maximum input lag error is **0.00653417 sample**, maximum independently remeasured output residual **0.00647934 sample**, and maximum normalized RMS null error **0.1215284%**. CSVs with higher precision are `alignment_metrics_v16.csv` and `alignment_waveforms_v16.csv`. Previous CSV/PNG evidence remains historical.

The new 64-tap, Blackman-windowed sinc interpolator passes 20 tone-frequency/fraction combinations through 0.45 times sample rate, with worst relative RMS interpolation error **0.000225688**, approximately **−72.93 dB**. This is a band-limited steady-tone error test, not broadband THD or a Nyquist-band guarantee. Twenty engine rate/limit configurations check guarded interpolation at both ±20 ms bounds across 44.1/48/96/192 kHz. Integer target delay and neutral/reference paths remain exact delay taps.

A transition test verifies that a delay update produces the known weighted mixture of two fixed taps and settles over 20 ms. This replaces the old continuous delay slew. Rapid automation may queue a following transition, and temporary interference during a blend still needs listening tests. The quality/transition suite also passes UndefinedBehaviorSanitizer (`delay_quality_ubsan.txt`).

The 32-sample interpolation guard raises reported latency to ceil(0.020 × sample rate) + 32 samples. Target ring headroom and correction clamp use separate limits; native latency/tail behavior needs host verification. The native JUCE test includes new latency/tail assertions but remains uncompiled/unexecuted here. BUILD_AND_VALIDATE.ps1 now covers nine CTest suites: eight portable plus one native. Pluginval, native CPU/allocation tests, editor/host automation, PDC, bypass and musical listening remain release blockers. Spectral phase optimization and multi-instance grouping are planned, not implemented.

## v1.7 — view policies and workflow controls

The new portable view suite passes C++17 with -O2 -Wall -Wextra -Wpedantic -Werror and separately UndefinedBehaviorSanitizer. It checks default preferences, 84 valid style/mode/channel/division roundtrips, invalid/nonfinite settings, bounded malformed state, hidden traces, and positive/negative/mixed filled-area bounds. Logs: `view_tests_v17.txt`, `view_ubsan.txt`. The eight unchanged DSP/capture suites retain their v1.6 passing results; this does not reclassify them as new GUI tests.

The JUCE native test now includes scope preference/undo recall, cancellation, selective undo preserving manual trim, and a real off-screen pixel comparison between line and filled waveform rendering. These native tests are **uncompiled/unexecuted here**, as are the editor resizing, dialog, controls and workflow/state additions. Portable fill-bound tests do not validate JUCE pixels or layout. The validation script covers ten CTest suites (nine portable, one native), followed by pluginval. Native compilation, DAW acceptance, CPU/headroom and display/keyboard checks remain release gates.

## v1.8 — actual sum history and new defaults

View tests pass with C++17/-O2/-Wall/-Wextra/-Wpedantic/-Werror and UndefinedBehaviorSanitizer. They now check filled defaults and 168 valid summed/style/mode/channel/division roundtrips. Sum-specific tests verify exact cancellation in raw views and long decimated histories, corrected constructive amplitude, mono sum peaks and stacked compatibility when the new saved-state bit is absent. Workflow and triggered-scope suites pass again after increasing history to six stereo/mono wave groups. Logs end in `_v18.txt`.

The native pixel comparison now covers summed rendering even when individual trace visibility is disabled. That test and native default/profile/session behavior are **uncompiled/unexecuted here**. Shared before/after scaling, labels, layout and the new default profile were reviewed in source; portable results do not prove JUCE rendering or DAW operation. Audio routing, gain and correction processing are unchanged by scope composition.

## v1.8.1 — default/preset correction

Source inspection confirms output gain and its double-click reset are 0.0 dB; the Kick + bass preset no longer writes the same-source-only correlation gate. The 0.65 parameter default and kick/bass DSP are unchanged. No new DSP tests were needed for these default/preset edits; previous results remain applicable. Native compilation/UI behavior for this patch have not been tested here.
