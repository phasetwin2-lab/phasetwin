# Remaining priorities — 1.7

The source is a release candidate. Further controls are not a substitute for these gates.

1. Native VST3 build, native integration tests and pluginval. Current DSP/view policy passes do not certify JUCE or host behavior.
2. Real microphone/DI/amp and kick/bass recordings: independent passages, changing notes, reflections, stereo disagreement and long offsets. The search remains ±20 ms.
3. PDC/sidechain routing, original/corrected gain matching, automation, bypass, undo/cancel races, state migration, offline render and sample-rate/block-size changes.
4. CPU/allocation/deadline measurements and long session/editor stress. Profile the newer sinc interpolation and correlation refinement on native machines.
5. Output headroom feedback: a real output peak/over-0-dBFS meter is still a useful addition. The output has no limiter; summing two tracks can exceed conversion headroom.
6. Small-screen/high-DPI/keyboard accessibility and visual validation. Resizing is implemented in source, but the current minimum window is 900 × 740. Colour is supported by A/B text labels; a broader accessibility audit remains.
7. Optional audition snapshots/preset comparison and a longer/wider analysis strategy after measured validation. Current Undo/Redo keeps 128 session-local audio edits; native host automation/gesture/race validation remains outstanding.
8. Spectral coherence/phase view and optional correction (Stage 2), followed by multi-instance grouping (Stage 3), as specified in PROJECT_SPECIFICATION.md. These are planned features.

The v1.9.1 automatic correction selector, scope button and ducking processor/state integration require native compilation and host/UI acceptance. Portable ducking tests validate its envelope core, not JUCE integration or musical quality.

The v1.10.0 preview/Apply workflow, independent audition, groove guard controls and legacy parameter defaults need native integration and DAW acceptance. Energy-gap tests are synthetic and do not certify musical groove preservation.

The v1.11.0 final-output meter, clip reset, advanced envelope controls and new state defaults need native integration/DAW validation. The envelope graph is measured gain reduction; no drawn-curve shaper or true-peak limiter is implemented.

The v1.12.0 spectral phase view and manual all-pass have portable mathematical tests only. Native wrapper/state/audition behavior, FFT CPU, transition/group-delay audibility and editor layout require host acceptance. Auto spectral optimization is still future work.

The v1.12.3 Hear proposal overlay needs native workflow validation, including comparison precedence, settings invalidation and Apply during audition. Portable delay-path tests do not certify the wrapper/UI overlay.

The v1.14.0 post-Apply verification and section bank have portable algorithm tests, but the native capture/mailbox/UI/DAW integration is still unexecuted here. User validation covers earlier builds. Check wrapper races, changed settings mid-evaluation, supported material, plugin CPU and memory usage (especially 64 retained source frames per section) before promoting this new source iteration. Common-candidate selection uses section fits, not exhaustive multi-section optimization; verification measures one fresh passage rather than continuous confidence.
