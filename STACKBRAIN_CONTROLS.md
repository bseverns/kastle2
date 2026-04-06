## StackBrain Controls

This is the working control and I/O reference for the current `StackBrain` firmware.

It covers:
- global pot names
- per-mode pot behavior
- shift-layer engine controls
- mode-layer routing and personality controls
- active patch inputs
- active outputs
- buttons and LEDs
- MIDI behavior
- currently unassigned hardware I/O

## Global Pot Names

These names stay stable across the whole app. The normal layer is performance-first. The shift layer is engine-depth.

| Pot | Global name | Main job |
|---|---|---|
| `POT_1` | `Rate Mod` | External CV amount for time/speed/rate-style control |
| `POT_2` | `Shape Mod` | External CV amount for response/shape/mutation behavior |
| `POT_3` | `Slew` | Companion-lane smoothing / lag |
| `POT_4` | `Response` | Main audio/control response character |
| `POT_5` | `Rate` | Master rate / follower speed / mutation speed |
| `POT_6` | `Range` | Output span / lane spread |
| `POT_7` | `Density` | Density / threshold / burst pressure |

## Per-Mode Pot Map

### `STEP`

| Pot | Panel name in this mode | Behavior |
|---|---|---|
| `POT_1` | `Rate Mod` | Amount of `PARAM_1` CV applied to clock rate |
| `POT_2` | `Shape Mod` | Amount of `PARAM_2` CV applied to pattern shape |
| `POT_3` | `Slew` | Slew amount for the companion `ENV_OUT` lane |
| `POT_4` | `Response` | Trigger family, sequence feel, contour depth, and filter intensity |
| `POT_5` | `Rate` | Internal clock rate when not externally clocked |
| `POT_6` | `Range` | CV span for `CV_OUT` and companion output behavior |
| `POT_7` | `Density` | Trigger density / probability |

Extra `STEP` control:
- `MODE` CV input biases pattern pressure and shape movement.
- `SYNC_IN` acts as an accent / contour retrigger input.

### `FOLLOW`

| Pot | Panel name in this mode | Behavior |
|---|---|---|
| `POT_1` | `Speed Mod` | Amount of `PARAM_1` CV applied to follower speed |
| `POT_2` | `Response Mod` | Amount of `PARAM_2` CV applied to transient/audio response |
| `POT_3` | `Slew` | Slew amount for the companion transient lane |
| `POT_4` | `Response` | Audio aggression, transient weight, and delay/ducking intensity |
| `POT_5` | `Speed` | Envelope/transient follower speed |
| `POT_6` | `Range` | Output span for `CV_OUT` and `ENV_OUT` |
| `POT_7` | `Threshold` | Event threshold and gate-density bias |

Extra `FOLLOW` control:
- `MODE` CV input biases the threshold up or down. Negative movement makes it easier to fire events; positive movement makes the follower more selective.

### `CHAOS`

| Pot | Panel name in this mode | Behavior |
|---|---|---|
| `POT_1` | `Rate Mod` | Amount of `PARAM_1` CV applied to mutation clock rate |
| `POT_2` | `Mutation Mod` | Amount of `PARAM_2` CV applied to rungler behavior |
| `POT_3` | `Slew` | Slew amount for the companion chaos lane |
| `POT_4` | `Response` | Bit behavior, reseed pressure, and audio aggression |
| `POT_5` | `Rate` | Mutation clock rate |
| `POT_6` | `Range` | Output span for chaos lanes |
| `POT_7` | `Burst Pressure` | Active-step pressure, gate density, and burst tendency |

Extra `CHAOS` controls:
- `MODE` CV input biases burst and reseed pressure.
- `SYNC_IN` injects bursts and forced accents.

## Shift Layer

Hold `SHIFT` to access the engine-depth layer. This is where the extra hardware I/O and MIDI behavior get shaped.

| Pot | Shift name | Behavior |
|---|---|---|
| `POT_1` | `Pitch1 Depth` | Amount of `PITCH_1` CV applied to the main lane |
| `POT_2` | `Pitch2 Depth` | Amount of `PITCH_2` CV applied to the companion lane |
| `POT_3` | `Link` | Pulls `CV_OUT` and `ENV_OUT` toward each other |
| `POT_4` | `Event Bias` | Global gate-length / accent emphasis bias |
| `POT_5` | `MIDI Note Depth` | Amount of MIDI note / pitch-bend bias applied to the main lane |
| `POT_6` | `MIDI Velocity Depth` | Amount of MIDI velocity bias applied to the companion lane |
| `POT_7` | `MIDI Clock Div` | Divider for incoming MIDI clock pulses |

Current design intent:
- normal layer should stay playable without menu-diving
- shift layer should expose the “hidden engine” and cross-patching depth

## Mode Layer

Hold `MODE` to access the routing/personality layer. A quick tap still cycles app modes. A hold exposes these deeper policies instead.

| Pot | Mode-layer name | Behavior |
|---|---|---|
| `POT_1` | `Mode Select` | Directly selects `STEP`, `FOLLOW`, or `CHAOS` |
| `POT_2` | `Clock Default` | Default clock policy when `FEED_1` is unpatched |
| `POT_3` | `Link Default` | Default companion-lane policy when `FEED_2` is unpatched |
| `POT_4` | `Route Default` | Default output routing when `FEED_3` is unpatched |
| `POT_5` | `MIDI Role` | Whether MIDI acts as bias, events, both, or not at all |
| `POT_6` | `Bias Route` | How MIDI and high-resolution bias signals are distributed across the output lanes |
| `POT_7` | `Personality` | Overall engine intensity / character |

Mode-layer policy values:

`Clock Default`
- `TRIGGER`
- `AUTO`
- `MIDI`

`Link Default`
- `INVERT`
- `FREE`
- `MERGE`

`Route Default`
- `SWAP`
- `NORMAL`
- `BLEND`

`MIDI Role`
- `OFF`
- `BIAS`
- `EVENTS`
- `HYBRID`

`Bias Route`
- `SPLIT`
- `CROSS`
- `MAIN`
- `BOTH`

`Personality`
- `TAME`
- `BALANCED`
- `PUSHED`
- `WILD`

## Active Hardware I/O

### Audio

| I/O | Current role |
|---|---|
| `Audio In` | Source audio for all three modes |
| `Audio Out` | Full processed stereo output from the current mode |

### Patch Inputs

| Input | Current role |
|---|---|
| `PARAM_1` | Rate/speed CV in all modes |
| `PARAM_2` | Shape/response/mutation CV in all modes |
| `PARAM_3` | Density/threshold/burst CV in all modes |
| `MODE` | Pattern-pressure CV in `STEP`; threshold-bias CV in `FOLLOW`; burst/reseed pressure CV in `CHAOS` |
| `PITCH_1` | High-resolution bias source for the main lane, scaled by `SHIFT + POT_1` |
| `PITCH_2` | High-resolution bias source for the companion lane, scaled by `SHIFT + POT_2` |
| `TRIG_IN` | External clock in `STEP` and `CHAOS`; manual force-event input in `FOLLOW` |
| `RESET_IN` | Hard reset of the active mode state in all modes |
| `SYNC_IN` | Accent retrigger input in `STEP`; external pulse assist / event source in `FOLLOW`; burst/accent injection in `CHAOS` |
| `FEED_1` | Clock policy selector |
| `FEED_2` | Companion-lane link policy selector |
| `FEED_3` | Output routing policy selector |

Feed semantics are tri-state:
- `LOW`
- `UNCONNECTED`
- `HIGH`

### Feed Policies

`FEED_1` clock policy:
- `LOW`: external trigger favored, MIDI clock ignored
- `UNCONNECTED`: use the `MODE + POT_2` default
- `HIGH`: MIDI clock favored, then trigger input, then internal fallback

`FEED_2` companion policy:
- `LOW`: invert the companion lane
- `UNCONNECTED`: use the `MODE + POT_3` default
- `HIGH`: pull the companion lane toward the main lane

`FEED_3` output routing policy:
- `LOW`: swap `CV_OUT` and `ENV_OUT`
- `UNCONNECTED`: use the `MODE + POT_4` default
- `HIGH`: blend the companion output toward the main lane with accent bias

### Outputs

| Output | `STEP` | `FOLLOW` | `CHAOS` |
|---|---|---|---|
| `CV_OUT` | Main stepped sequence lane | Envelope/intensity lane | Primary rungler lane |
| `ENV_OUT` | Slewed companion lane | Transient-weighted companion lane | Shadow chaos lane with spread bias |
| `GATE_OUT` | Step gate | Recovered transient/beat/manual gate | Irregular gate with bursts |
| `SYNC_OUT` | Clock pulse | Recovered event pulse | Clock/mutation pulse |

## Buttons

| Button | Current role |
|---|---|
| `MODE` | Quick tap cycles `STEP -> FOLLOW -> CHAOS`; hold exposes the mode layer |
| `SHIFT` | Exposes the engine-depth pot layer; quick tap recalls the next stored scene |

Scene gesture:
- hold `MODE`, then quick tap `SHIFT` to store the current state into the active scene slot
- quick `SHIFT` tap recalls the next scene with a short morph instead of a hard jump

## LEDs

| LED | Current role |
|---|---|
| `LED_1` | Mode identity and current energy/CV intensity |
| `LED_2` | Event/accent display |
| `LED_3` | Clock/transient/reseed context depending on mode |

Mode-specific notes:
- `STEP`: `LED_3` distinguishes external-vs-internal clock feel.
- `FOLLOW`: `LED_2` goes white on recovered beat pulses and orange on transient-hot behavior.
- `FOLLOW`: `LED_3` flashes recovered event activity.
- `CHAOS`: `LED_2` highlights burst steps.
- `CHAOS`: `LED_3` highlights reseed moments.

## Current Patch Summary

If you want the shortest useful mental model:
- `STEP` is a sequenced control engine.
- `FOLLOW` is a recovered-pulse translator.
- `CHAOS` is a paired-lane mutation engine.

The four main patch outputs should be thought of like this:
- `CV_OUT`: the main statement
- `ENV_OUT`: the shadow or companion statement
- `GATE_OUT`: event emphasis
- `SYNC_OUT`: pulse relationship

## MIDI

`StackBrain` now has active MIDI behavior.

Current MIDI engine:
- MIDI CC can control both the normal layer and shift-layer `FancyPot` controls
- MIDI CC can also control the mode-layer policy pots
- MIDI clock can drive `STEP` and `CHAOS`
- MIDI note on can create a manual event, especially useful in `FOLLOW`
- MIDI velocity can bias the companion lane
- MIDI pitch bend can bias the main lane

Current MIDI CC map:

| CC | Function |
|---|---|
| `20` | `Rate Mod` |
| `21` | `Shape Mod` |
| `22` | `Slew` |
| `23` | `Response` |
| `24` | `Rate` |
| `25` | `Range` |
| `26` | `Density` |
| `52` | `Pitch1 Depth` |
| `53` | `Pitch2 Depth` |
| `54` | `Link` |
| `55` | `Event Bias` |
| `56` | `MIDI Note Depth` |
| `57` | `MIDI Velocity Depth` |
| `58` | `MIDI Clock Div` |
| `60` | `Mode Select` |
| `61` | `Clock Default` |
| `62` | `Link Default` |
| `63` | `Route Default` |
| `64` | `MIDI Role` |
| `65` | `Bias Route` |
| `66` | `Personality` |
| `67` | `Scene Select` |
| `68` | `Scene Recall` |
| `69` | `Scene Store` |

Current MIDI note behavior:
- note on maps pitch into a centered bias for the main lane
- velocity maps to a positive bias for the companion lane
- note on also queues a `FOLLOW` event
- note off clears the velocity bias
- pitch bend becomes a centered pitch-bias control
- `MODE + POT_5` decides whether those note/velocity messages act as bias, events, both, or neither

Current MIDI clock behavior:
- `Start` / `Continue` arms MIDI clock following
- `Stop` halts MIDI clock following
- `SHIFT + POT_7` sets the pulse divider used by incoming MIDI clock
- `FEED_1` decides how strongly MIDI clock participates relative to trigger input and internal clock

Current scene behavior:
- there are `4` stored scene slots
- quick `SHIFT` tap cycles and recalls the next scene slot with a short morph
- recalled values stay latched until the corresponding pots are brought near the stored target
- `CC 67` selects the active scene slot
- `CC 68` recalls the selected scene when the value is above midscale
- `CC 69` stores the current state into the selected scene when the value is above midscale

## Currently Unassigned Hardware

These are not currently mapped inside `StackBrain`:
- explicit MIDI CC output reporting
- user control over scene morph time / curve

That means the app is now using all three main control layers in a coherent way, plus the formerly-idle pitch/feed inputs and the previously spare `MODE`/`SYNC_IN` roles. The main remaining growth area is user-shaped morph behavior and richer MIDI reporting.

## Good Next Expansion Targets

If you want to push the app toward truly full-panel usage, the highest-value next additions are:
1. Add user control over scene morph time and morph curve.
2. Let MIDI notes or CCs select routing personalities, not just bias values.
3. Add MIDI CC output for reporting current mode, active scene slot, lane intensity, and selected clock division.
