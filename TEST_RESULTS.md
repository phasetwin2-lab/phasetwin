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

## v1.9.0 — polarity permission, scope button and sidechain ducking

Added a two-position Auto polarity permission knob, a shared Stacked/Summed scope button and optional stereo-linked envelope ducking. Ducking defaults off, affects A only, and runs after alignment. Capture, correlation and scope remain pre-ducking. New parameters are appended to preserve existing parameter order; legacy states explicitly initialize ducking off.

The portable ducking suite passes C++17 with -O2 -Wall -Wextra -Wpedantic -Werror and UndefinedBehaviorSanitizer. It covers default unity, silence, depth at 44.1/48/96/192 kHz, linked stereo/anti-phase reference, harshness attack/release differences, disable settling and invalid data. Native regression coverage was added for A-only attenuation, unducked B summing, stereo output, the reduction meter, parameter recall, legacy-state defaults and host bypass. These native tests and the new GUI remain uncompiled/unexecuted here. All ten portable suites were rebuilt and passed with the same strict compiler flags; logs end in `_v19.txt`. Full CTest now contains ten portable suites and one native suite.

## v1.9.1 — mutually exclusive automatic correction modes

Replaced the independent editor permissions with one automatable selector: Timing + polarity (default), Preserve polarity, or Preserve timing. Both session learning and continuous tracking derive permissions from the selector. Mode changes invalidate older analysis jobs without clearing held correction or manual overrides; the Kick preset retains the selection. State schema 6 appends the selector while retaining old parameter IDs for migration. Legacy timing-only/polarity-only presets map directly; old measure-only presets map to Preserve timing and load locked (Analyze explicitly unlocks). Legacy permission automation no longer controls new analysis.

Kick regression tests verify useful polarity correction while preserving held timing and useful timing correction while preserving inversion. Native regression assertions cover default mode, held-correction preservation, selector recall and all four legacy permission combinations; native tests remain uncompiled/unexecuted in this environment. GUI layout and DAW automation still require local validation.

Kick, workflow and learning portable suites pass fresh C++17 strict-warning builds for this change; kick selective-correction tests also pass UndefinedBehaviorSanitizer. Logs end in `_v191.txt`. Remaining portable suites retain their v1.9.0 results because their DSP was unchanged.

## v1.9.2 — direct scope buttons and ducking sliders

Lines/Filled and Reference trigger/Rolling now use two-state buttons with current-mode labels. Restore explicitly updates labels and display state without triggering save callbacks; packed preference values and defaults are unchanged. Ducking Amount/Harshness use horizontal sliders with numeric percent entry and a 50% double-click reset; their parameter attachments and DSP are unchanged. This is a reversible editor-only change. Native JUCE compilation, mouse/keyboard interaction, layout at supported sizes/DPI and session recall require local validation; existing portable preference/DSP results do not validate the new GUI.

## v1.9.2 — editor theme and C language build fix

Enabled C and C++ in the top-level project declaration as required by JUCE dependencies. Added an editor-owned LookAndFeel with dark gradients, outlined panels, styled slider tracks/thumbs, active-button accents and keyboard-focus outlines. Editor destruction disconnects the LookAndFeel before member destruction; styling is local to the plugin editor. Audio processing, parameter values and saved-state layout are unchanged. Source inspection completed. This environment has no CMake/JUCE native build; native compilation, visual layout, DPI, focus/disabled states and popup appearance remain unverified. Prior portable DSP results remain applicable but do not validate styling or the CMake fix.

## v1.9.2 — post-duck scope and reduction history

Audio visualization packets now include actual post-duck stereo A and per-sample gain reduction in dB. Editor history derives the post-duck A+B sum before decimation. After: Pre/Post-duck is saved in an unused preference bit (legacy default pre-duck); the Before display and alignment scores remain unchanged. A reduction graph shares the same capture window and Hold. Pre/post scale includes both histories regardless of selected view. Minimum editor height is 860 to accommodate the added graph. Additional portable tests cover preference recall, attenuated target, unducked-reference sum cancellation and reduction peaks in raw/decimated histories. Native waveform/button rendering, processor capture integration and additional visualization CPU/memory require local JUCE/DAW checks.

Fresh strict-warning C++17 builds pass for view, triggered scope, workflow, end-to-end alignment regressions and ducking. The expanded view tests also pass UndefinedBehaviorSanitizer. Logs: `*_duckview.txt` and `view_duck_ubsan.txt`. Native tests now assert actual captured ducked A, unchanged B, reduction dB and saved post-duck preference; those native assertions remain unexecuted here.

## v1.10.0 — groove guard, recommendation workflow and independent audition

Added optional kick/bass groove search (default on, 2 ms new-shift cap) and a full inter-kick low-band joint-energy gap heuristic using 2 ms bins. Preview defaults on; session and continuous learning propose, and Apply commits on the audio thread with Undo. Proposals are invalidated by capture/settings changes and are not persisted. Kick candidate improvement is measured on the captured hit objective; same-source improvement is predicted on the last input frame with linear fractional sampling and is not live verification. No-benefit/unreliable results retain previous correction with specific messages. Independent timing/polarity audition is automatable, holds stored correction and uses the existing latency/crossfade. New parameters are appended, with schema 7 defaults on legacy recall. Native preview/apply and audition assertions were added but remain uncompiled/unexecuted here.

Portable kick tests cover shift caps, connected-pattern gap rejection for both advance and delay, unchanged-groove acceptance and predicted-correlation polarity discrimination. DSP, workflow and end-to-end regression suites also pass strict-warning C++17 builds. Musical groove acceptance, native preview invalidation/apply/state/automation, callback CPU (including prediction), high-DPI layout and listening remain required before release.

Expanded kick groove/prediction tests also pass UndefinedBehaviorSanitizer (`kick_guard_ubsan_v110.txt`). Preview Apply invalidates older worker generations before consuming further results, preventing pre-Apply captures from issuing a fresh stale proposal. Native callback/workflow verification remains pending.

All ten portable suites have fresh passing v1.10 results. Expanded groove/preview tests additionally pass UBSan. Native build, pluginval, real-session gap detection and editor/automation acceptance remain pending.

## v1.11.0 — output headroom and advanced duck envelope

Added final stereo sample peak meters (24 dB/sec display decay, 0 dBFS marker) and an audio-thread latched full-scale warning with click/keyboard reset. This is sample-peak monitoring, not true-peak limiting. Advanced duck mode defaults off; explicit 0.1–100 ms attack and 10–1000 ms release override Harshness timing, while Harshness still sets knee. Detector-only sensitivity −24..+24 dB works in either mode with 5 ms smoothing. Parameter IDs are appended and legacy-state defaults initialize the new controls; schema 8. The captured gain-reduction history is now labelled Duck envelope.

Portable duck tests pass strict-warning C++17 builds, including explicit attack/release response, detector sensitivity and unchanged simple timing when advanced is off. Native regression assertions cover final stereo peak readings, clipping latch through safe output and reset; native compilation, meter/UI behavior, state/automation and musical listening are unexecuted here. Prior portable alignment/view results remain applicable because their processing was unchanged.

Expanded advanced ducking tests also pass UndefinedBehaviorSanitizer (`advanced_duck_ubsan_v111.txt`). Native output-meter and envelope integration remain pending.

## v1.12.0 — spectral phase view and optional manual all-pass

Added worker-thread 8192-sample Hann FFT phase diagnostics over 32 log bands, with raw Before and actual processed/pre-duck After on the strongest analysis channel. Brightness is within-band cross-phase concentration with an energy floor, not temporal coherence or calibrated confidence. Manual stereo-linked second-order all-pass centre/Q controls default off; coefficients and enable mix use 20 ms time constants. Settled processing has unity magnitude; transitions may change level and frequency-dependent group delay affects attacks/tails. Analyze does not optimize rotation. Neutral/timing-only/neither audition bypass rotation. Added conservative 300 ms tail reporting and rotation-settling verification gating; state schema 9 appends/migrates rotation defaults.

Portable phase tests cover 44.1/48/96/192 kHz, 40/80/400/3000 Hz steady-state magnitude, stereo linking, disable settling, 180° centre relationship, impulse-energy conservation at extreme Q and spectral polarity correction/silence. Native compiler, wrapper/state/rotation-audition tests, spectral display/DPI, callback CPU and real musical listening remain pending. Spectral optimization is not implemented or claimed. Full CTest has eleven portable suites plus one native suite.

Expanded phase tests pass strict-warning C++17 and UndefinedBehaviorSanitizer (`phase_tools_v112.txt`, `phase_tools_ubsan_v112.txt`). Added native wrapper centre-phase and rotation recall assertions; they remain unexecuted here. Prior portable results remain applicable for unchanged alignment/ducking cores.

## v1.12.1 — collapsible controls and compact layout

Grouped nonessential controls into five individually collapsible sections; primary profile/mode, preview, manual trim/polarity, lock and mix/gain remain visible. All sections default collapsed, active effects retain header status and hidden settings remain attached/processing. An editor-owned Viewport scrolls controls without scrolling sliders into parameter changes. Layout reflows based on expansion with a 310 px visualization reservation; default 1080×960 and minimum 940×900. Expansion mask is nonautomatable state metadata (`editorSections`, bounded to five bits); schema 10, legacy default collapsed. Native recall assertion added. Source inspection only: this is an editor/state-metadata change; native compilation, expansion/scroll/focus behavior, DPI and restored-state layout remain untested here. Portable DSP results do not validate the GUI.

## v1.12.2 — useful spectral curves and inspection

Expanded to 64 bands, relative per-bin joint-energy floor and combined Before/After phase concentration. Supported adjacent bands draw curves; phase wraps and weak/empty bands break them, with isolated markers retained. Added Before/After visibility toggles, log-frequency grids, zero/±180° guides, relative-energy shading and hover readouts for band bounds, phase, support and relative joint energy. Hold view now freezes both waveform and spectral snapshots. An atomic epoch and GUI snapshot prevent mixed-frame band rendering; reads skip busy updates without waiting. FFT length/resolution remains 8192 samples; more display bands do not add physical resolution. Band support is still not temporal coherence.

Added portable connectivity tests for wrap/gap/low-support handling and native pixel-gap assertions. Native compiler/UI/hover/hold/high-DPI validation remains pending.

Expanded phase/spectrum tests pass strict-warning C++17 and UndefinedBehaviorSanitizer (`spectral_curves.txt`, `spectral_curves_ubsan.txt`). Native spectrum pixel-gap, hover, Hold and visibility tests remain unexecuted here.

## v1.12.3 — audible proposal preview

Hear proposal is a nonpersisted request handled on the audio callback. It substitutes proposed timing/polarity in the processing path, with current manual controls and effects, without writing held correction or Undo. It temporarily overrides component audition/unaligned comparison while preserving their parameters; turning off restores their behavior. Apply/new proposal/settings invalidation/bypass/missing reference end temporary audition. The actual heard samples feed After scope, spectrum and output meters. Existing delay/polarity/rotation smoothing remains in use. Added native start/stop, held-state/Undo preservation, comparison-choice preservation and Apply/end assertions (uncompiled/unexecuted here). Added portable impulse-path tests for temporary timing/polarity, unchanged reference/neutral paths and restoration at three sample rates.

The expanded end-to-end regression suite passes a strict-warning C++17 build with 131329 assertions (`proposal_audition.txt`). Native proposal-audition assertions are provided but remain uncompiled/unexecuted; GUI state, callback invalidation and musical comparison require DAW validation.

## v1.13.0 — audio-only multi-step history

Added a fixed-capacity 128-step message-thread history for normalized audio parameters and coherent held delay/polarity in milliseconds. GUI gestures, kick preset and Reset are grouped; new edits discard Redo. Continuous tracking replaces its current step; ungestured host automation is grouped after 250 ms quiet or explicit Undo. Parameter restoration uses host notifications on the message thread; learned correction restoration and stale-job invalidation use the audio callback. No history allocation or lock was added to the audio callback. Preview/Hear proposal and display preferences remain outside history. Loaded sessions retain their audio/view state but establish a new history baseline (schema 11).

All **12 portable suites** were rebuilt with C++17 `-O2 -Wall -Wextra -Wpedantic -Werror -pthread` and passed, including 81 new history checks and 131329 alignment regression assertions. The new history suite also passed UndefinedBehaviorSanitizer. Native history integration tests were expanded for gestures, grouped presets, display exclusion, applied correction Undo/Redo, reversible Reset, state recall, branching and stopped navigation. **JUCE compilation, native tests, editor rendering and DAW/pluginval checks remain unexecuted here.** CTest now has 13 suites: 12 portable and one native.

## v1.14.0 — fresh verification and optional section collection

Added two appended workflow parameters: Verify after apply (default On) and Collect multiple sections (default Off), schema 12 migration, both excluded from audio history. Post-Apply verification uses matched-latency unaligned/output capture, actual phase rotation, pre-duck processing, waiting/settling guards and independent capture epochs. Kick/bass compares supported reference-onset events over four seconds; Same source aggregates fresh supported correlation windows for at least one second with a two-second timeout. No automatic correction is made from this check.

The worker-owned section bank stores 2–4 complete kick captures or supported same-source frames (maximum 64 per section). A common per-section candidate is evaluated against every retained section; mean improvement must exceed 0.02, at least 75% improve, and no section can be harmed by more than 0.02. Kick groove cap/gap guards apply to every section. Rejected/contradictory collections retain held correction. Captures are ephemeral, with generation/epoch invalidation on settings changes.

All **13 portable suites** rebuilt and passed under C++17 strict warnings, including **33 session-analysis checks**, **82 audio-history checks** and **131329 alignment regression assertions**. Session analysis also passed UndefinedBehaviorSanitizer. Fresh-output tests additionally exercised the real AudioEngine neutral/corrected paths and filtered captures at 44.1, 48 and 96 kHz. New native integration assertions cover two captured kick sections, common proposal/Apply, fresh verification without changing correction, workflow-history exclusion and state recall. **Native JUCE compile/UI and this new wrapper workflow have not been executed in this environment.** The user reports successful DAW testing of prior builds; that does not validate this newly added workflow. CTest now has 14 suites: 13 portable and one native.
