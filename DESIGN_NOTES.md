# Workflow review — 3 October 2026

Primary sources reviewed:
- https://www.soundradix.com/products/auto-align/
- https://www.soundradix.com/products/auto-align/release-notes/
- https://fluctoraudio.com/products/phaselock/

Auto-Align 2 emphasizes a fast capture/alignment workflow, comparison presets and microphone time/spectral correction. Sound Radix also documents direct numeric delay entry. PhaseLock 2.4 emphasizes kick-sidechain routing, selective delay/polarity/rotation permissions, measure-only mode and separate score/confidence. Its optional rotation changes waveform shape; its shaper handles level overlap as a separate operation.

Applied to PhaseTwin: explicit kick/bass routing, learn/apply/hold, selectable timing/polarity permission, measure-only labeling, matched-latency unaligned audition, editable delay in milliseconds, visible learned/trim/applied timing, polarity and compensation latency, documented confidence semantics and stable visual captures. These are workflow choices inspired by public documentation, not copies of either proprietary algorithm or scoring system.

Release essentials: reproducible native build, validator runs, reliable state recall, automation/bypass/PDC tests, rejection of unreliable learning, no audio-thread visualization work, and listening across changing bass notes. These remain acceptance requirements, not assumed successes.

Scope boundaries: PhaseTwin does not provide Auto-Align's multi-instance grouping, ARA or spectral optimization, nor PhaseLock's phase rotation, bass-mono modes or ducking shaper. The current ±20 ms timing range is explicit. Adding those features would need separate DSP and host validation; UI polish does not imply algorithmic parity.
