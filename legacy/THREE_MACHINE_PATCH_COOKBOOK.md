## Three-Machine Patch Cookbook

This document collects patch ideas for a three-machine Bastl setup:

- `Kastle 1.5`
- `Kastle Drum`
- `Kastle 2`

Default role split:
- `Kastle 1.5`: CV brain / unstable modulation source
- `Kastle Drum`: transient / percussion engine
- `Kastle 2`: transformer, processor, or resampling target

Assumptions:
- Function names refer to the panel labels, not physical positions.
- For `Kastle 2`, this starts from `FX Wizard` because it is the most direct “end-of-chain weirdness” role.

General rules:
1. Start with one clock master.
2. Add one modulation lane, then audio routing, then at most one return lane.
3. If a patch collapses into mush, remove the return lane first.
4. Kastle-to-Kastle patching is normal use. For external modular gear, use the protected I/O jack path rather than raw mini-patch points.

### 1. Industrial Driver

Patch:
- `Kastle 1.5 stepped/rungler -> Kastle Drum pitch`
- `Kastle 1.5 square -> Kastle Drum decay`
- `Kastle Drum audio -> Kastle 2 audio in`

Use:
- `Kastle 2 FX Wizard` as delay/filter/smear stage

Why it works:
- The Drum stays intelligible.
- The `Kastle 1.5` scrambles motion without fully erasing groove.
- The `Kastle 2` turns it into a finished soundstage.

### 2. Drum Select Mutator

Patch:
- `Kastle 1.5 triangle -> Kastle Drum drum` selector CV
- `Kastle 1.5 stepped/rungler -> Kastle Drum pitch`
- `Kastle Drum audio -> Kastle 2 audio in`

Use:
- Keep the triangle LFO slow

Why it works:
- Drum algorithm changes feel intentional rather than random blur.

### 3. Broken Sequencer

Patch:
- `Kastle 1.5 square -> Kastle Drum clk in`
- `Kastle 1.5 stepped/rungler -> Kastle Drum pitch`
- `Kastle 1.5 triangle -> Kastle Drum decay`
- `Kastle Drum audio -> Kastle 2 audio in`

Why it works:
- The `Kastle 1.5` becomes the composer.
- The Drum becomes the voice under algorithmic control.

### 4. Clock-Locked Chaos

Patch:
- `Kastle Drum clk out -> Kastle 1.5 LFO reset`
- `Kastle 1.5 stepped/rungler -> Kastle Drum pitch`
- `Kastle Drum audio -> Kastle 2 audio in`

Why it works:
- Reset relationships give structured weirdness instead of free drift.

### 5. Cross-Mangled Legacy Pair

Patch:
- `Kastle 1.5 main out -> Kastle Drum pitch` or another CV destination you like
- `Kastle Drum noises -> Kastle 1.5 timbre` or `waveshape`
- Audio from either or both machines -> `Kastle 2 audio in`

Why it works:
- Both legacy units start acting like unstable control oscillators.

### 6. Noisy Snare Fog

Patch:
- `Kastle Drum noises -> Kastle 2 audio in`
- `Kastle 1.5 stepped/rungler -> Kastle Drum decay`
- `Kastle 1.5 square -> Kastle Drum clk in`

Use:
- Short delay or smear on `Kastle 2`, not huge wash

Why it works:
- The `noises` output often gives a more distinctive result than the main Drum output.

### 7. Drone Collapse

Patch:
- `Kastle 1.5 main out -> Kastle 2 audio in`
- `Kastle Drum noises -> Kastle 1.5 timbre`
- `Kastle Drum clk out -> Kastle 1.5 LFO reset`

Use:
- Keep Drum audio low or muted

Why it works:
- The Drum behaves more like a destabilizer than a voice.

### 8. Call And Response

Patch:
- `Kastle Drum clk out -> Kastle 1.5 reset/LFO reset`
- Optional: `Kastle 1.5 square -> Kastle Drum clk in`
- `Kastle 1.5 audio -> mixer side A`
- `Kastle Drum -> Kastle 2 -> mixer side B`

Why it works:
- You get two related but distinct voices instead of one merged patch.

### 9. Pseudo Sidechain

Patch:
- `Kastle Drum clk out -> Kastle 2` sync/tempo-related modulation input if the chosen mode responds musically
- `Kastle 1.5 triangle -> Kastle 2 amount/time mod`
- `Kastle Drum audio -> Kastle 2 audio in`

Why it works:
- The FX stage breathes in time with the legacy pair instead of acting like a static insert.

### 10. Wave Bard Prep Patch

If `Kastle 2` is later flashed to `Wave Bard`:

Patch:
- `Kastle 1.5 stepped/rungler -> Kastle Drum pitch`
- `Kastle 1.5 square -> Kastle Drum clk in`
- `Kastle Drum noises -> Kastle 1.5 timbre`

Workflow:
- Record short hits, loops, and drones from the legacy pair
- Load them into `Wave Bard`

Why it works:
- One machine makes chaos.
- One machine freezes chaos into playable material.
- One machine destabilizes playback.

### Practical Tuning Notes

- If a patch is too random, slow the triangle source first.
- If a patch is too polite, move one modulation lane from decay to pitch.
- If a patch loses punch, reduce FX depth before changing the legacy pair.
- If you want groove, emphasize reset/clock relationships.
- If you want destruction, emphasize audio-rate cross-mod.

### Best First Three

1. `Industrial Driver`
2. `Broken Sequencer`
3. `Drone Collapse`

