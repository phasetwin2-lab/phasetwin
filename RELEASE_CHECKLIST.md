# Release acceptance — not yet completed

- [ ] BUILD_AND_VALIDATE.ps1 succeeds: Release x64, ten CTest suites, pluginval strictness 5 and 10.
- [ ] DAW and OS/version recorded; external sidechain works with mono/stereo main and reference combinations.
- [ ] At 44.1, 48, 96 and 192 kHz, and buffer sizes 32–2048: learn → apply → hold → neutral A/B → reset works without glitches.
- [ ] Kick/bass: representative real kick hits and changing bass notes; rejected candidates hold existing settings. Compare the complete phrase by ear.
- [ ] Timing-only, polarity-only and measure-only preserve the disabled corrections; manual trim/polarity survive learning.
- [ ] Repeated learn, profile/range/focus changes while learning, freeze, disabled/silent sidechain and transport stop/resume cause no stale application or stuck progress.
- [ ] Save/reopen preserves correction; sample-rate change preserves milliseconds. Old sessions migrate correctly.
- [ ] Host bypass has correct compensated latency; neutral A/B has matched gain; scope quarter note is 500 ms at 120 BPM and 1000 ms at 60 BPM.
- [ ] Offline export matches held correction; real-time CPU, memory and editor open/close stress measured. Learning is done during real-time playback.
- [ ] Host sidechain delay compensation checked. Add reference B = 0 outputs bass only; internal summing does not double the kick's master feed.

Portable DSP test passes are recorded in TEST_RESULTS.md. Native wrapper/GUI/pluginval and DAW acceptance have not been run in this environment. No completed release certification is implied by this checklist.

- [ ] Stable scope holds through silence/transport stop, refreshes only with a complete kick capture, and Hold/Rolling work. Check 1/1 = 2000 ms at 120 BPM and new divisions retain correct labels.
- [ ] At normal/high display DPI, millisecond readouts, numeric entry, direction/polarity labels and hover explanations are legible.

- [ ] Confirm the new 32-sample interpolation guard in reported PDC/tail, neutral audition, host bypass, stopped/resumed playback and rendered audio at every supported sample rate.
- [ ] Collect native CPU/allocation and interpolation-quality measurements; assess musical recordings with microphone coloration and reflections rather than relying on synthetic delay tests.

- [ ] Lines/filled actual pixels preserve positive/negative waveform lobes and transient peaks; both scales match. Show A/B affects only visuals, and settings recall correctly.
- [ ] Cancel during capture and worker analysis rejects stale results; Undo/Redo restores complete audio settings and held correction through multiple steps. Reset is reversible. Session/preset reload starts a fresh baseline; audio/view state remains recalled.
- [ ] Resizing, 100/125/150/200% DPI, keyboard focus, numeric editing and help dialog work in each supported host.

- [ ] Stacked/Summed changes both panels; exact opposite-polarity inputs show a zero sum and aligned inputs constructive peaks. Output mix/gain and trace visibility must not alter the unity-sum view. Verify saved-state recall, new kick/filled defaults and legacy stacked states.

- [ ] Automatic correction selector offers exactly three mutually exclusive modes, automates/recalls correctly and preserves disabled correction plus manual controls. Kick preset preserves mode. Verify legacy migration, in-flight mode changes, and Reset then Preserve timing on programmed kick/bass patterns.
- [ ] Scope button switches both panels and recalls Stacked/Summed across session reload at minimum and high-DPI editor sizes.
- [ ] Ducking defaults off; Amount/Harshness automate smoothly, stereo A remains linked and mixed B stays unducked. Verify reduction meter, reference-level sensitivity, missing/silent sidechain, bypass and legacy presets.
- [ ] Listen to soft/hard ducking on off-grid kicks at different levels, sample rates and block sizes; confirm analysis/scope remain pre-ducking.

- [ ] Lines/Filled and Reference trigger/Rolling buttons toggle both waveform views, show the current mode and restore correctly. Ducking sliders support dragging, numeric percent entry, double-click reset and host automation.

- [ ] Build with LANGUAGES C CXX; inspect the new theme at minimum/default/maximum size and high DPI, including keyboard focus, disabled sliders, popup menus and multiple editor open/close cycles.

- [ ] Verify Pre/Post-duck After display and reduction history at 900×860 and high DPI. Before/analysis stay unchanged, gain reduction shares capture/Hold, pre/post toggle keeps scale, summed B remains unducked and state restores pre/post choice. Measure visualization CPU and queue drops with the larger packets.

- [ ] Verify groove search on real 180–190 BPM kick + three-sixteenth bass patterns, changing pitches, bass silence, off-grid/omitted kicks and both shift directions. Confirm default 2 ms cap and guard do not guarantee continuity or clear older shifts.
- [ ] Preview leaves held audio unchanged; Apply creates Undo; new analysis/manual settings/routing/state cancel stale proposals; stopped playback processes Apply only when audio resumes.
- [ ] Test independent timing/polarity audition and Hear unaligned precedence, including trim, manual flip, bypass, ducking, saved selection and crossfade settling.
- [ ] Verify No useful correction reasons, candidate estimates versus actual A/B, minimum 900×950 layout, and native callback CPU for predicted correlation.

- [ ] Output sample peak meters reflect final mix/gain and mono/stereo layouts; latch at full scale, clear on click/Space/Enter, relatch during ongoing overload and remain visible on bright/high-DPI displays. No true-peak or limiting claim.
- [ ] Advanced attack/release sliders enable only in Advanced mode; Harshness changes knee there. Sensitivity changes detection without boosting B. Verify state/automation, all sample rates and block sizes, envelope graph and click-free control changes.

- [ ] Verify all-pass default off, centre/Q automation, saved state, unity steady-state magnitude, transition level changes, tail reporting and rotation settling. Listen to attacks/tails and changing bass notes; groove guard does not protect all-pass group delay.
- [ ] Verify raw Before / processed pre-duck After spectral phase on the auto analysis channel; silence clears evidence. Brightness is band phase concentration, not temporal coherence. Validate spectrum/waveform switching and CPU at all sample rates.

- [ ] Verify all 32 section expansion combinations at 940×900, default and high DPI; use scrolling/keyboard focus to reach all controls. Collapsing must not change parameters or stop an active effect; headers show current automation state.
- [ ] Save/reload section expansion on an existing open editor; legacy state loads collapsed. Mouse wheel scrolls the viewport without automating sliders; resizing retains access to Apply/Undo/meter and the waveform/spectrum.

- [ ] Verify spectral curves, wrap breaks, unsupported gaps, isolated markers, energy shading and hover readouts with broadband pairs and sparse tones. Check Before/After visibility and Hold snapshots, FFT resolution labels and atomic snapshot update behavior.

- [ ] Hear proposal auditions full proposed timing/polarity without changing held state or Undo; Off restores comparison choices. Apply ends audition and commits; Cancel/Reset/new Analyze/changed settings/bypass/missing sidechain invalidate safely. Confirm transitions, automation, open/closed editor and stopped playback.
- [ ] After waveforms/spectrum/meters represent auditioned audio. Hold retains the old snapshot until released; triggered captures refresh after completion. Verify proposal label, disabled unaligned toggle and unchanged gain/ducking/rotation settings.

- [ ] v1.13.0 history: drag/numeric entry/toggles each form the intended step; preset and Reset are atomic history actions; new edit discards Redo. Verify all audio controls and applied alignment, fractional offsets across sample-rate changes, stopped multiple Undo/Redo clicks, edit during pending restore, Cancel/Apply/Reset/state recall races, open/closed editor and continuous tracking.
- [ ] Display preferences and temporary Hear proposal never create steps or change during Undo/Redo. Preview workflow preference stays as selected. Verify 128-step eviction and button counts at the minimum editor size.
- [ ] Ungestured host automation is grouped after 250 ms quiet; plugin Undo/Redo notifies the host but does not alter automation lanes. Confirm the DAW may reassert automated values.

- [ ] v1.14.0: Verify after Apply waits for settled, normal corrected audition; actual phase rotation is included, while ducking/mix/gain cannot bias the measure. Improved/worsened/unchanged/unsupported states are correct on fresh audio. Pause/resume, optional Off, changed controls, routing, state recall, worker cancellation, mono/stereo, different sample rates/block sizes and offline rendering behave consistently.
- [ ] Multi-section collection captures 2–4 passages in both profiles without changing held audio; common candidates improve across retained material, conflicting/unreliable sections are rejected, groove rejection holds on every kick section. Per-section gains, Preview/Hear/Apply, immediate Apply, Clear, Cancel, Undo/Redo, new settings and full-bank UI behavior work. Collected audio/results never persist; workflow toggles persist outside audio history.
