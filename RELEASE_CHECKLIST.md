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
- [ ] Cancel during capture and worker analysis rejects stale results; Undo restores prior learned correction, preserves manual overrides and survives save/reopen. Reset clears undo history.
- [ ] Resizing, 100/125/150/200% DPI, keyboard focus, numeric editing and help dialog work in each supported host.

- [ ] Stacked/Summed changes both panels; exact opposite-polarity inputs show a zero sum and aligned inputs constructive peaks. Output mix/gain and trace visibility must not alter the unity-sum view. Verify saved-state recall, new kick/filled defaults and legacy stacked states.

- [ ] Automatic correction selector offers exactly three mutually exclusive modes, automates/recalls correctly and preserves disabled correction plus manual controls. Kick preset preserves mode. Verify legacy migration, in-flight mode changes, and Reset then Preserve timing on programmed kick/bass patterns.
- [ ] Scope button switches both panels and recalls Stacked/Summed across session reload at minimum and high-DPI editor sizes.
- [ ] Ducking defaults off; Amount/Harshness automate smoothly, stereo A remains linked and mixed B stays unducked. Verify reduction meter, reference-level sensitivity, missing/silent sidechain, bypass and legacy presets.
- [ ] Listen to soft/hard ducking on off-grid kicks at different levels, sample rates and block sizes; confirm analysis/scope remain pre-ducking.

- [ ] Lines/Filled and Reference trigger/Rolling buttons toggle both waveform views, show the current mode and restore correctly. Ducking sliders support dragging, numeric percent entry, double-click reset and host automation.
