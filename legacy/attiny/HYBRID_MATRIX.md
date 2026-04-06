## Legacy Hybrid Matrix

This document maps the vendored official Bastl legacy `ATtiny85` sketches to practical chip-swap combinations for a dual-chip legacy Kastle setup.

Scope:
- `Kastle 1.5`
- `Kastle Drum`
- legacy Bastl dual-`ATtiny85` hardware only

Important:
- Treat `sound` and `mod` as the two chip roles in the legacy platform.
- Keep one known-good stock machine available while testing.
- Change one chip at a time whenever possible.
- Some compatibility notes below are inferred from the sketch structure and intended role, not guaranteed by Bastl.

Source sketches live in [sketches](/Users/bseverns/Documents/GitHub/kastle2/legacy/attiny/bastl-official/sketches).

### Baselines

| Role | Stock sketch |
|---|---|
| Synth sound | `kastleSynthe_VCO` |
| Synth mod | `kastleSynth_LFO` |
| Drum sound | `kastleDrum` |
| Drum mod | `kastleDRUM_CLK_LFO` |

### Sketch Roles

#### Safer sound-chip sketches

| Sketch | Intended flavor | Risk |
|---|---|---|
| `kastleSynthe_VCO` | stock synth | low |
| `kastleDrum` | stock drum/percussion | low |
| `kastleArp` | melodic/chordal | low-medium |
| `kastleKarplus` | plucked/string-like resonator | low-medium |
| `kastleSampler` | sample-based voice | medium |
| `kastleSamplerNoise` | noisy sample-based voice | medium |
| `kastleSynthe_BIT` | harsh digital / bytebeat-ish | medium |
| `kastleSynthe_FOLD` | folded / distorted synth | medium |
| `kastleSynthe_VCO_2` | later synth variant | medium |

#### Safer mod-chip sketches

| Sketch | Intended flavor | Risk |
|---|---|---|
| `kastleSynth_LFO` | stock synth modulation / rungler | low |
| `kastleDRUM_CLK_LFO` | clock-centric rhythmic modulation | low-medium |
| `kastleSynth_DUAL_STEP` | stepped sequence / alternate rungler behavior | medium |

#### Experimental / paired-only candidates

| Sketch | Why caution |
|---|---|
| `snowTrap` | appears to use custom chip-to-chip messaging and should not be treated as a generic drop-in |
| `kastleNEW_CORe` | appears experimental and not clearly distinct enough to recommend as a first-pass hybrid |

### Recommended First-Pass Hybrids

These are the most useful combinations to try first.

| ID | Sound chip | Mod chip | Best host | Expected result | Risk |
|---|---|---|---|---|---|
| H1 | `kastleArp` | `kastleSynth_LFO` | `Kastle 1.5` | cleanest alternate instrument, still playable | low |
| H2 | `kastleKarplus` | `kastleSynth_LFO` | `Kastle 1.5` | plucked, unstable, resonant voice | low-medium |
| H3 | `kastleSamplerNoise` | `kastleSynth_LFO` | `Kastle 1.5` | broken sample-noise machine | medium |
| H4 | `kastleSynthe_BIT` | `kastleSynth_LFO` | `Kastle 1.5` | nasty digital logic voice | medium |
| H5 | `kastleSynthe_FOLD` | `kastleSynth_LFO` | `Kastle 1.5` | aggressive folded distortion synth | medium |
| H6 | `kastleDrum` | `kastleSynth_DUAL_STEP` | `Kastle Drum` | drum voice with less predictable modulation | medium |
| H7 | `kastleDrum` | `kastleSynth_LFO` | `Kastle Drum` | drum sound under more familiar synth-style CV behavior | medium |
| H8 | `kastleSampler` | `kastleDRUM_CLK_LFO` | either | crude rhythmic sample voice | medium |
| H9 | `kastleSynthe_VCO_2` | `kastleDRUM_CLK_LFO` | either | synth voice pushed into more clocked/rhythmic behavior | medium |

### Conservative Test Order

If you want to learn the platform without losing the plot, use this order:

1. Keep `Kastle Drum` stock.
2. Reflash only the `Kastle 1.5` sound chip.
3. Try `kastleArp`.
4. Try `kastleKarplus`.
5. Try `kastleSamplerNoise`.
6. Restore whichever sound sketch you like most.
7. Only then begin mod-chip experiments on the `Kastle 1.5`.
8. After that, start hybridizing the `Kastle Drum`.

### Machine-Specific Recommendations

#### Best use of the `Kastle 1.5`

The `Kastle 1.5` is the best first experiment box because losing stock synth behavior is less costly than losing your dedicated drum baseline.

Recommended combinations:
- `kastleArp` + `kastleSynth_LFO`
- `kastleKarplus` + `kastleSynth_LFO`
- `kastleSamplerNoise` + `kastleSynth_LFO`
- `kastleSynthe_BIT` + `kastleSynth_DUAL_STEP`

#### Best use of the `Kastle Drum`

Use the `Kastle Drum` as the second-phase hybrid box after you have one favorite `Kastle 1.5` experiment.

Recommended combinations:
- `kastleDrum` + `kastleDRUM_CLK_LFO` for stock baseline
- `kastleDrum` + `kastleSynth_DUAL_STEP` for a stronger rhythm mutation
- `kastleDrum` + `kastleSynth_LFO` for a synth-flavored drum hybrid

### High-Reward Combinations

These are more likely to produce strange or unstable results worth exploring:

| Sound chip | Mod chip | Why it may be good |
|---|---|---|
| `kastleSynthe_BIT` | `kastleDRUM_CLK_LFO` | digital voice with more percussive/clocked behavior |
| `kastleSamplerNoise` | `kastleSynth_DUAL_STEP` | chaotic sample stepping and noisy transients |
| `kastleKarplus` | `kastleDRUM_CLK_LFO` | physically-modeled-ish voice under rhythmic forcing |
| `kastleDrum` | `kastleSynth_DUAL_STEP` | strongest candidate for a custom industrial hybrid |

### Combinations To Avoid First

Avoid these until you already have one reliable hybrid and one stock unit:

- `snowTrap` with anything
- `kastleNEW_CORe` with anything
- changing both chips in both machines at once
- changing both chips in one machine before testing each role independently

### Suggested Test Log

For every flash attempt, log:
- machine
- sound sketch
- mod sketch
- clock / fuse setup
- boots?
- audio output?
- controls sensible?
- patch points still useful?
- keeper or discard

### Suggested Starting Pair For Your Stack

If the goal is a three-machine rig with distinct personalities:

- `Kastle 1.5`: `kastleKarplus` or `kastleArp` hybrid
- `Kastle Drum`: mostly stock at first, then `kastleSynth_DUAL_STEP` mod experiment
- `Kastle 2`: separate modern personality via `Wave Bard`, `Alchemist`, or custom RP2040 firmware

