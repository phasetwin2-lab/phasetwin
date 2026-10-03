# PhaseLock 2.4 and PhaseTwin 1.3

Reviewed 2026-10-03 against Fluctor's official product page and its author listing on Max for Live. PhaseTwin is an independent implementation; no proprietary device code, UI assets or scoring formula were copied.

| Feature | PhaseLock 2.4 (published) | PhaseTwin 1.3 |
|---|---|---|
| Delivery | Max for Live for Live 12 / Max 9+ | JUCE VST3 source project; target-system build required |
| Auto-Align | Two-second capture | Two-second multi-window learn and hold |
| Selective correction | Timing, polarity, optional rotation | Timing and polarity individually allowed; measure-only when both off |
| Timing range | −100 to +150 ms | ±20 ms; narrower range retained |
| Score / confidence | Separate readouts | Separate coherence score and evidence/stability heuristic; not the same proprietary metric |
| Rotation | Optional all-pass phase rotation | Not implemented |
| Stereo tools | Stereo, Mono, Bass Mono | Linked stereo correction and mono bus support; no Bass Mono processing |
| Ducking | Beat/kick triggering and drawn curve | Not implemented |
| Scope | Live, tempo-based one-beat display | Before/after, tempo/manual 2–4000 ms views, one-beat default |
| Reset / audition | Neutral alignment reset | Reset plus latency-matched neutral A/B |

PhaseTwin adds a full-spectrum same-source profile and a low-end measurement profile. Its filters are for measurement only; a low-pass analysis limit is not a Bass Mono crossover. Confidence guards consistency across windows; a high score alone does not authorize a change. Independent bass notes and kick hits may not have one meaningful global lag.

The scope and learn workflow were prioritized in this update. Rotation, widening the timing range and ducking need separate designs and target-host testing. A wide negative shift needs additional lookahead/latency; a shaper needs reliable transient/retrigger and host-timing behavior. The current plugin must not be represented as feature-equivalent to PhaseLock or a validated replacement for it.

Sources:

- https://fluctoraudio.com/products/phaselock/
- https://maxforlive.com/library/device/16260/phaselock-2

The published feature summary is based on those pages, not hands-on testing of the commercial device.
