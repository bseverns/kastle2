## StackBrain Spec

`StackBrain` is a proposed Kastle 2 community firmware focused on making the modern Kastle 2 act as an audio-reactive control processor for a legacy stack.

Primary target setup:
- `Kastle 1.5`
- `Kastle Drum`
- `Kastle 2` as the central logic, CV, and mutation layer

Tentative app ID:
- `0x10`

This stays in the community range from [APP_LIST.md](/Users/bseverns/Documents/GitHub/kastle2/APP_LIST.md).

Detailed MVP design for the first implemented mode lives in:
- [STACKBRAIN_STEP_MODE.md](/Users/bseverns/Documents/GitHub/kastle2/STACKBRAIN_STEP_MODE.md)

## App Role

`StackBrain` should not try to replace `FX Wizard` or duplicate a normal synth.

Its job is:
- generate useful clocks and gates for the legacy pair
- generate two related but distinct CV lanes
- translate external audio into CV/gate structure
- process audio in a way that audibly reflects the same internal control logic
- stay patchable and performance-friendly

In short:
- one modern brain
- two legacy sound sources
- one firmware that helps them behave like a system while remaining a meaningful part of the sound

## MVP Summary

MVP should provide:
- one app with `3` modes
- internal or external clocking
- `CV_OUT` as a primary modulation lane
- `ENV_OUT` as a secondary modulation lane
- `GATE_OUT` as a derived trigger/gate lane
- `SYNC_OUT` as clock out
- audio input and audible processing in all modes
- audio input analysis in at least one mode
- memory-backed settings for core preferences

MVP should not require:
- sample capture
- second-core processing
- MIDI-first workflow
- custom file formats

## Core Principle

`StackBrain` should have:
- a control plane
- an audio plane

These are not separate personalities.

The same internal modulators should drive:
- patch outputs
- audio transformations

Examples:
- if a step sequence is moving `CV_OUT`, the audio should also pulse, slice, or shift with that sequence
- if `FOLLOW` emits a transient gate on `GATE_OUT`, the audio should also react to that transient
- if `CHAOS` produces drift or reseeding on `ENV_OUT`, the audio should also become unstable in sympathy

This is what gives the `Kastle 2` a special coordinating role without reducing it to a passive utility module.

## Core Modes

### 1. Step

Purpose:
- internal clocked control sequencer

Behavior:
- drives regular clock and stepped CV
- useful for `Kastle Drum` pitch, decay, and clock control
- useful for `Kastle 1.5` pitch, timbre, or reset modulation
- audio path is shaped by the same step pattern

Outputs:
- `CV_OUT`: stepped main sequence
- `ENV_OUT`: slewed or offset companion lane
- `GATE_OUT`: trigger at each step or probability-qualified step
- `SYNC_OUT`: main clock pulse

Audio role:
- rhythmic pulser / slicer
- clocked filter or repeater
- pattern-shaped dry/wet or stereo movement

Good use:
- `CV_OUT -> Kastle Drum pitch`
- `ENV_OUT -> Kastle Drum decay`
- `SYNC_OUT -> legacy clock/reset`

### 2. Follow

Purpose:
- audio-reactive translator

Behavior:
- listens to audio input
- tracks envelope and transients
- outputs patchable CV and derived gates

Outputs:
- `CV_OUT`: envelope follower or averaged intensity
- `ENV_OUT`: transient emphasis, decay contour, or spectral tilt proxy
- `GATE_OUT`: beat/transient gate
- `SYNC_OUT`: clock recovered or divided from input activity when possible

Audio role:
- transient-driven freeze or repeater behavior
- ducking or pumping
- input-driven filter or drive response

Good use:
- `Kastle Drum audio -> Kastle 2 audio in`
- `CV_OUT -> Kastle 1.5 timbre`
- `ENV_OUT -> Kastle Drum decay`

### 3. Chaos

Purpose:
- controlled instability / rungler-style mutation mode

Behavior:
- produces semi-repeatable CV with probability, drift, and skip behavior
- can run free or be reset by external clock
- should feel like “musical sabotage,” not white-noise randomness

Outputs:
- `CV_OUT`: stepped/rungler/random-walk lane
- `ENV_OUT`: slower drift, inverted lane, or smoothed partner lane
- `GATE_OUT`: accents, skips, bursts, or irregular trigger lane
- `SYNC_OUT`: master or follower clock

Audio role:
- stochastic microlooping
- unstable delay / crusher / pitch wobble
- chaos-linked panning or filter movement

Good use:
- `CV_OUT -> Kastle Drum drum select or pitch`
- `ENV_OUT -> Kastle 1.5 waveshape`
- `GATE_OUT -> reset legacy phrasing`

## Audio Behavior

`StackBrain` is primarily a control processor, not a neutral utility and not a generic FX box.

MVP audio behavior should:
- always expose the current mode’s internal control logic
- preserve enough dry path to keep the source legible
- add one clear mode-specific transformation layer

Shared expectations:
- dry path available in every mode
- one audible transformation per mode
- the transformation is driven by the same internal state that drives outputs

This keeps the app useful in the audio stream without letting it drift into “unrelated effect plus CV tool.”

## Output Ownership

MVP likely needs to take control of:
- `ENV_OUT`
- `SYNC_OUT`
- `GATE_OUT`
- `CV_OUT`

It should also manage its own audio path instead of relying on a stock synth/effect chain.

So the implementation will likely want to disable relevant base features, starting with:
- `AUDIO_CHAIN`
- `ENV_OUT`
- likely any base-owned clock/CV outputs that would conflict with explicit app control

Exact feature disables should be confirmed during implementation against the available `Base::Feature` set.

## Control Model

The app should follow the repo’s existing layered UI style rather than inventing a new control philosophy.

### Mode Selection

Use `FancyMode` on the `MODE` CV / mode control lane.

Modes:
- `STEP`
- `FOLLOW`
- `CHAOS`

### Normal Layer

Normal layer should expose the three most musical controls plus supporting modifiers.

| Control | MVP function |
|---|---|
| `POT_5` | clock/rate |
| `POT_6` | range / target spread |
| `POT_4` | audio/control response |
| `POT_1` | rate modulation attenuverter |
| `POT_2` | shape modulation attenuverter |
| `POT_3` | slew / smoothing |
| `POT_7` | density / probability / clock division |

Interpretation by mode:

#### Step
- `POT_5`: master clock rate
- `POT_6`: CV span
- `POT_4`: sequence shape and audio pattern response
- `POT_1`: external CV amount for rate
- `POT_2`: external CV amount for sequence shape
- `POT_3`: slew on companion lane
- `POT_7`: trigger density / skip probability

#### Follow
- `POT_5`: detector speed / responsiveness
- `POT_6`: CV output range
- `POT_4`: transient vs smooth emphasis in both outputs and audio
- `POT_1`: rate/response mod
- `POT_2`: analysis bias / threshold mod
- `POT_3`: smoothing
- `POT_7`: gate sensitivity / density

#### Chaos
- `POT_5`: mutation clock
- `POT_6`: CV excursion
- `POT_4`: chaos amount in both outputs and audio
- `POT_1`: clock modulation amount
- `POT_2`: chaos modulation amount
- `POT_3`: smoothing / drift
- `POT_7`: reset probability / burst density

### Shift Layer

Shift layer should handle routing and behavior choices.

| Control | MVP function |
|---|---|
| `POT_5` | `SYNC_OUT` division/multiplication |
| `POT_6` | `CV_OUT` lane source selection |
| `POT_4` | `ENV_OUT` lane source selection |
| `POT_1` | input clock/external reset behavior |
| `POT_2` | gate mode |
| `POT_3` | output polarity / inversion behavior |
| `POT_7` | audio monitor amount |

Examples:
- `CV_OUT`: main sequence, envelope, drift, random walk
- `ENV_OUT`: slewed main lane, transient lane, inverted lane, offset lane
- gate mode: every step, accents only, transient only, burst only

### Mode Layer

Mode layer should hold slower-to-change musical preferences.

| Control | MVP function |
|---|---|
| `POT_5` | scale / quantization mode |
| `POT_6` | root note / offset |
| `POT_4` | clock source preference |
| `POT_1` | memory preset or seed |
| `POT_2` | reset behavior |
| `POT_3` | fallback output behavior when no audio clock exists |
| `POT_7` | global intensity |

MVP can keep this lighter if needed, but quantization and reset policy are worth keeping.

## CV / Gate Definitions

MVP should define output semantics clearly:

### `CV_OUT`

Primary lane.

By mode:
- `STEP`: stepped sequence
- `FOLLOW`: envelope / intensity
- `CHAOS`: random walk / rungler lane

### `ENV_OUT`

Secondary lane.

By mode:
- `STEP`: slewed, inverted, or offset partner lane
- `FOLLOW`: transient contour or faster envelope lane
- `CHAOS`: drift lane or smoothed mutation lane

### `GATE_OUT`

Digital event lane.

By mode:
- `STEP`: step trigger / accent gate
- `FOLLOW`: transient gate
- `CHAOS`: irregular accent / burst gate

### `SYNC_OUT`

Clock lane.

By mode:
- `STEP`: internal master pulse
- `FOLLOW`: recovered or derived pulse
- `CHAOS`: master or externally-reset pulse

## Inputs

### Audio Input

Most important in `FOLLOW`.

Use cases:
- track Drum dynamics
- detect legacy transients
- derive envelope and gate behavior

### `TRIG_IN`

Use for:
- external clock
- step advance
- mode-specific reseed or trigger

### `RESET`

Use for:
- sequence reset
- chaos reseed
- envelope reset behavior

### CV Inputs

Primary external CV usage:
- free pitch lane as generic modulation source
- second pitch lane as quantized or step-related source
- parameter CVs for rate, shape, and mode behavior

## Internal State Buses

The cleanest design is to maintain a few shared internal buses and map both outputs and audio parameters from them.

Suggested buses:
- `phase_bus`
  Master clock phase, step position, pulse timing
- `energy_bus`
  Envelope or input intensity
- `event_bus`
  Trigger, transient, accent, or beat events
- `chaos_bus`
  Random walk, reseed, drift, mutation state

These should be treated as the internal glue between control and audio behavior.

## Internal Building Blocks

The repo already contains good building blocks for this app:

- `FancyMode`
- `FancyPot`
- `EnvelopeFollower`
- `BeatDetector`
- `KastleRungler`
- `Sequencer`
- `SlewGenerator` / `Slewer`
- `Quantizer`
- `EdgeDetector`
- `TriggerGenerator`

This means MVP should mostly be composition and control logic, not deep new DSP.

## LED Behavior

Keep it simple and legible.

Suggested mapping:
- `LED_2`: mode color
- `LED_1`: clock / trigger activity
- `LED_3`: output intensity or transient indication

Suggested colors:
- `STEP`: green
- `FOLLOW`: cyan or white
- `CHAOS`: orange or raspberry

## Memory

MVP should persist:
- current mode
- quantization/scale choice
- root/offset
- preferred clock source behavior
- lane routing choices
- gate mode

MVP does not need multi-preset storage yet.

## MIDI

MIDI is useful but should not block MVP.

MVP MIDI scope:
- optional mode select
- optional clock start/stop
- optional CC control over rate/range/shape

Safe choice:
- stub MIDI support for later
- do not let MIDI design complexity delay the first version

## Second Core

MVP should avoid needing `SecondCoreWorker()`.

Reason:
- control logic is lighter than full sample/effect engines
- simpler implementation and debugging

Second core becomes interesting later if:
- input analysis grows more advanced
- FFT-derived behavior becomes part of the app
- multiple output lanes require heavier audio-rate calculations

## File Plan

Suggested app directory:
- `code/src/apps/StackBrain/`

Suggested initial files:
- `main.cpp`
- `AppStackBrain.hpp`
- `AppStackBrain.cpp`
- `StackBrainParameterMaps.hpp`
- `README.md`
- optionally `cc.hpp` later if MIDI grows

## Implementation Shape

### MVP Phase 1

Build:
- mode switching
- pot framework
- manual output control
- `STEP` mode only
- one clear sequence-driven audio process

Success means:
- can clock one legacy machine
- can emit stepped CV on `CV_OUT`
- can emit companion lane on `ENV_OUT`
- can audibly slice, pulse, or filter incoming audio from the same step logic

### MVP Phase 2

Build:
- `FOLLOW` mode
- envelope follower
- transient gate extraction
- transient-driven audio reaction

Success means:
- Drum audio can drive useful CV and gate outputs
- Drum audio can also audibly drive the mode’s processing

### MVP Phase 3

Build:
- `CHAOS` mode
- probability / drift / reset behavior
- chaos-linked audio destabilization

Success means:
- the app becomes a real mutation brain rather than just a utility clock sequencer

## Best First Patch Targets

Once MVP exists, the first three patch targets should be:

1. `CV_OUT -> Kastle Drum pitch`, `ENV_OUT -> Kastle Drum decay`
2. `CV_OUT -> Kastle 1.5 timbre`, `GATE_OUT -> reset or trigger relationship`
3. `Kastle Drum audio -> Kastle 2 audio in` while `FOLLOW` extracts CV from the Drum itself

## Non-Goals

These are intentionally out of scope for MVP:
- full effect engine
- deep sample recorder
- full groovebox behavior
- trying to be `Wave Bard` plus `FX Wizard` plus a utility app at once

## Open Questions

These should be resolved before coding starts:

1. Should `StackBrain` always pass audio through, or should it be silent unless explicitly monitoring input?
2. Should `SYNC_OUT` always remain a clock, or may it become a second digital event lane in some modes?
3. Should `STEP` mode be quantized by default, or raw stepped voltages by default?
4. Should `CHAOS` mode favor repeatable seeded patterns or unstable drift?

## Recommendation

Start coding with a narrow MVP:
- `STEP` first
- no second core
- minimal MIDI
- clear output ownership
- useful patching on day one
- clearly audible sequence-driven processing on day one

That will tell you quickly whether `StackBrain` is the right permanent role for your `Kastle 2`, and it gives a clean base to grow `FOLLOW` and `CHAOS` later.
