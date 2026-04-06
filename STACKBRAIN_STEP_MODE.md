## StackBrain STEP Mode

This document defines the MVP `STEP` mode for `StackBrain` in enough detail to guide implementation.

Related:
- [STACKBRAIN_SPEC.md](/Users/bseverns/Documents/GitHub/kastle2/STACKBRAIN_SPEC.md)

## Goal

`STEP` should be the first implemented mode because it proves the core identity of `StackBrain`:

- it coordinates other Kastles through clock, CV, and gate outputs
- it remains a meaningful processor in the audio chain
- the patch outputs and the audible behavior come from the same internal step logic

## One-Sentence Identity

`STEP` is a clocked control processor that emits a stepped main CV lane, a slewed companion lane, and a gate stream while rhythmically slicing and tone-shaping the input audio with the same sequence.

## Chosen First Audio Process

The first audio process for `STEP` should be:

### Clocked Contour Slicer

Definition:
- incoming audio stays audible through a dry path
- each step opens or suppresses a rhythmic VCA-like contour
- each step also moves a filter target
- accent steps hit harder and/or brighter

Why this is the right first process:
- simpler than full freeze/replay logic
- immediately audible and musically legible
- strongly tied to the step outputs
- easy to hear whether the sequence is doing anything useful
- can later grow into more advanced rhythmic repeater behavior

This is better for MVP than a micro-looper because it gives the app a distinct voice without requiring more fragile buffer choreography on day one.

## Output Semantics

`STEP` owns:

### `CV_OUT`

Primary stepped lane.

Behavior:
- outputs the current step value
- value range and quantization depend on pot settings
- intended first patch targets are pitch- and selection-like destinations on legacy Kastles

### `ENV_OUT`

Secondary companion lane.

Behavior:
- a slewed, offset, or inverted derivative of the main step lane
- designed for slower or more contour-like modulation destinations

### `GATE_OUT`

Event lane.

Behavior:
- high on active steps
- optionally accent-qualified or probability-thinned

### `SYNC_OUT`

Clock lane.

Behavior:
- emits the master pulse
- can later support divisions or multiplications, but MVP should start with a straightforward pulse-per-step output

## Inputs

### External Clock

Source:
- `TRIG_IN` as digital input

Behavior:
- when external clocking is enabled, each rising edge advances the step engine
- internal clock pauses or is ignored

### Reset

Source:
- `RESET_IN`

Behavior:
- resets step index to zero
- resets gate phase and contour phase
- optionally re-arms the sequence accent cycle

### Audio Input

Source:
- stereo input buffer

Behavior:
- summed or dual-mono processed for MVP
- dry path preserved
- the step contour modulates level and filter state

### Modulation CV Inputs

Initial intent:
- one free CV for rate influence
- one free/step CV for shape or spread influence
- one parameter CV for density or gating influence

Precise routing can be finalized in implementation, but MVP should keep the number of live external mod destinations modest.

## Internal State

`STEP` should expose a small number of shared state buses.

### `phase_bus`

Contains:
- current clock phase
- clock source mode
- step duration / period
- pulse position within current step

Used by:
- `SYNC_OUT`
- gate contour timing
- step advance logic

### `step_bus`

Contains:
- current step index
- sequence length
- current step value
- current accent/active flag

Used by:
- `CV_OUT`
- `GATE_OUT`
- audio contour shape
- filter target

### `companion_bus`

Contains:
- slewed or transformed derivative of current step value

Used by:
- `ENV_OUT`
- optional audio secondary modulation such as stereo width or extra tone movement

### `event_bus`

Contains:
- step trigger edge
- accent edge
- reset edge

Used by:
- contour re-trigger
- output pulse generation
- LED feedback

## Step Engine

The step engine should be explicit and deterministic.

### Sequence Length

MVP default:
- `8` or `16` steps

Recommendation:
- start with fixed `8` internal voltage states mapped through the existing sequencer-style logic
- allow effective pattern length from `4` to `16` later

### Step Values

MVP should avoid a full editable sequencer at first.

Recommended value source:
- use generated or procedural steps rather than user-programmed per-step values

Good MVP model:
- one procedural voltage pattern
- one trigger/accent mask
- controls morph the pattern family rather than exposing direct editing

That keeps the app aligned with Kastle behavior rather than turning it into a tiny workstation.

### Advance Event

On each clock edge:
1. increment or wrap step index
2. compute current step value
3. compute active/accent status
4. update `CV_OUT` target
5. update `ENV_OUT` target
6. emit `GATE_OUT` pulse state
7. reset the audio contour for the new step
8. compute new filter target for audio

That event is the central heartbeat of the mode.

## Procedural Pattern Model

MVP should not store a large pattern table yet.

Recommended pattern model:
- main lane from sequencer-like stepped values
- active/accent mask from a generated rhythmic pattern
- one morph control that shifts between straighter and stranger shapes

Simple candidate sources:
- `Sequencer`
- `KastleRungler`
- generated trigger mask with density control

Recommended split:
- main CV lane: `Sequencer`-style stepped voltage
- weirdness layer: optional `KastleRungler` contribution controlled by `shape`

This gives a useful middle ground between “too straight” and “too random.”

## Audio Signal Flow

MVP `STEP` audio path:

1. input sum or stereo pair arrives
2. optional light pre-filtering / cleanup
3. apply step contour gain
4. apply step-controlled filter target
5. mix dry and processed signal
6. output

Expressed simply:

`input -> contour VCA -> tone filter -> dry/wet mix -> output`

### Contour

The contour is the audible expression of `GATE_OUT`.

It should:
- retrigger every active step
- optionally hit harder on accent steps
- have short-to-medium decay controlled by pots

This does not have to be a classic envelope object if a lighter contour generator is simpler, but an envelope-like shape is the goal.

### Filter

The filter is the audible expression of `CV_OUT` and/or `ENV_OUT`.

Recommended MVP filter behavior:
- use a single lowpass or tilt-like movement first
- map step value to cutoff target
- smooth target changes slightly to avoid ugly clicks unless those clicks are musically useful

Good candidate:
- `Svf` or `DjFilter`

### Dry/Wet

MVP should preserve source identity.

Recommendation:
- default to a mixed dry/wet output, not full wet
- allow stronger processing with `shape` or a shift-layer parameter

This prevents the app from dominating too early.

## Pot Mapping For MVP STEP

### Normal Layer

| Control | Function |
|---|---|
| `POT_5` | clock rate |
| `POT_6` | CV range / span |
| `POT_4` | contour + audio response |
| `POT_1` | rate mod amount |
| `POT_2` | shape mod amount |
| `POT_3` | companion-lane slew |
| `POT_7` | density / active-step probability |

Interpretation:

- `clock rate`
  Internal tempo when not externally clocked

- `CV range`
  Scales the main step voltage from narrow to wide excursion

- `contour + audio response`
  Macro control for how sharply the audio is sliced and how strongly step values affect the filter

- `rate mod amount`
  How much external CV can bend the clock/rate behavior

- `shape mod amount`
  How much external CV can push the procedural pattern away from its base state

- `companion-lane slew`
  Smoothness of `ENV_OUT`

- `density`
  How many steps are active and how often `GATE_OUT` fires

### Shift Layer

| Control | Function |
|---|---|
| `POT_5` | sync pulse width / division |
| `POT_6` | companion lane mode |
| `POT_4` | dry/wet |
| `POT_1` | external clock behavior |
| `POT_2` | gate mode |
| `POT_3` | output polarity / inversion |
| `POT_7` | accent intensity |

Recommended companion lane modes:
- slewed main lane
- inverted main lane
- offset lane
- accent-follow lane

Recommended gate modes:
- all active steps
- accents only
- thinned by probability

### Mode Layer

| Control | Function |
|---|---|
| `POT_5` | quantization mode |
| `POT_6` | root / offset |
| `POT_4` | internal vs external clock preference |
| `POT_1` | pattern family |
| `POT_2` | reset behavior |
| `POT_3` | pattern length |
| `POT_7` | global intensity |

MVP may simplify this, but quantization, clock source, and pattern family are worth keeping in the design.

## Quantization

Quantization in `STEP` should affect:
- `CV_OUT`
- optionally the filter movement range if desired later

Recommendation for MVP:
- raw stepped voltages by default
- optional quantization in mode layer

Reason:
- raw control is more immediately useful for legacy Kastles, especially Drum destinations

## Clock Source Behavior

MVP should support:
- internal clock
- external clock from `TRIG_IN`

Recommended behavior:
- external clock wins when detected and enabled
- timeout returns to internal clock if external clock disappears

This avoids dead patches.

## State Machine

### Clock Source State

States:
- `INTERNAL`
- `EXTERNAL`

Transitions:
- internal -> external when valid external triggers arrive and external mode is enabled
- external -> internal when timeout occurs or external mode is disabled

### Step Activity State

Per step:
- `inactive`
- `active`
- `accent`

These govern:
- `GATE_OUT`
- contour amount
- filter emphasis

### Audio Contour State

States:
- `IDLE`
- `ATTACK`
- `DECAY`

Behavior:
- re-enter `ATTACK` on each active step
- accent steps start with higher contour amplitude
- inactive steps may still pass dry audio, but processed contour stays low

## LED Behavior

Recommended MVP:
- `LED_2`: mode color for `STEP`
- `LED_1`: clock pulse
- `LED_3`: current step activity or accent brightness

This makes the control logic visible without overdesign.

## Implementation Notes

### UI Loop Responsibilities

`UiLoop()` should:
- read pots and mode/layer state
- manage clock source detection decisions
- update high-level parameter targets
- handle memory persistence conditions

### Audio Loop Responsibilities

`AudioLoop()` should:
- advance internal phase if using internal clock
- react to step edge flags
- generate contour and gate timing
- apply audio contour and filter
- write outputs

### Suggested Helpers

Likely useful internal helpers:
- `AdvanceStep()`
- `UpdateCurrentStepTargets()`
- `TriggerContour()`
- `UpdateClockSource()`
- `RenderOutputs()`
- `ProcessAudioFrame()`

## Success Criteria For STEP MVP

`STEP` is successful if:
- it can clock and modulate a `Kastle Drum` meaningfully
- it can emit a main stepped lane and a distinct companion lane
- its audio processing is clearly tied to the same step logic
- it is musically useful before `FOLLOW` or `CHAOS` exist

## Best Test Patches

First validation patches:

1. `CV_OUT -> Kastle Drum pitch`, `ENV_OUT -> Kastle Drum decay`
2. `SYNC_OUT -> Kastle 1.5 reset/LFO reset`
3. `Kastle Drum audio -> StackBrain audio in`
4. `StackBrain audio out -> mixer or recorder`

If you can hear the step structure in the audio and also feel it in the legacy patch behavior, the mode is working.

