## Kastle 2 Expansion Sketch

This document frames possible expansion directions for the Kastle 2 platform in this repo, with an eye toward a mixed setup that includes:

- legacy dual-`ATtiny85` Kastles
- one modern `Kastle 2`
- a personal “tower” or stack of related Bastl voices

The goal is not just “more apps,” but a system where the `Kastle 2` becomes a strong complement to the legacy machines rather than duplicating them.

## What The Repo Already Supports

The current codebase already exposes the key ingredients for serious custom apps:

- whole-app firmware model with `Init()`, `AudioLoop()`, `UiLoop()`
- optional `SecondCoreWorker()` for dual-core work
- `MidiCallback()` support
- memory-backed settings via app IDs and `MemoryInitialization()`
- internal clock, sequencer, LFO, CV/gate outputs, and layer-aware UI
- audio input and output path control
- sample playback, oscillators, FM, filters, delays, clipping, quantizer, rungler-like utilities

Useful references:
- [APP_LIST.md](/Users/bseverns/Documents/GitHub/kastle2/APP_LIST.md)
- [GLOSSARY_EXAMPLES.md](/Users/bseverns/Documents/GitHub/kastle2/GLOSSARY_EXAMPLES.md)
- [AppTemplate.cpp](/Users/bseverns/Documents/GitHub/kastle2/code/src/apps/Template/AppTemplate.cpp)
- [AppExampleSynth.cpp](/Users/bseverns/Documents/GitHub/kastle2/code/src/apps/ExampleSynth/AppExampleSynth.cpp)
- [FxWizard README](/Users/bseverns/Documents/GitHub/kastle2/code/src/apps/FxWizard/README.md)
- [WaveBard README](/Users/bseverns/Documents/GitHub/kastle2/code/src/apps/WaveBard/README.md)

There are also a lot of reusable DSP building blocks already in-tree:
- oscillators and FM
- sample player
- filters
- bit crusher
- stereo delay
- envelope follower
- FFT
- quantizer
- rungler and sequencer utilities

## Expansion Lanes

For your setup, the strongest expansion lanes are:

### 1. Audio Transformer Apps

These treat `Kastle 2` as the “finishing brain” for the legacy pair.

Characteristics:
- audio input is central
- one or more CV outputs mirror or abstract the input
- patch-friendly modulation behavior
- best when paired with `Kastle 1.5` and `Kastle Drum`

This is the most practical lane if the `Kastle Drum` stays your raw sound source.

### 2. CV Brain / Companion Apps

These apps use the modern hardware to control the legacy pair.

Characteristics:
- `Kastle 2` is not mainly a synth voice
- it emits clocks, stepped CV, envelope-derived CV, gate logic
- audio may be secondary or absent

This is the best lane if you want the `Kastle 2` to feel like the “meta-Kastle” in the stack.

### 3. Hybrid Instrument Apps

These make `Kastle 2` a voice that still plays well with the older units.

Characteristics:
- synthesis and external audio coexist
- the app can self-oscillate or process other machines
- one firmware can move between solo use and stack use

This is the best lane if you want one signature personal firmware.

### 4. Resampling / Freeze / Recomposition Apps

These let the `Kastle 2` ingest chaos from the legacy pair and turn it into stable material.

Characteristics:
- sample capture or clip slicing
- loop mutation
- trigger-aware playback
- CV-addressable recall or resequencing

This is the highest-ceiling lane for creating a truly unique voice ecosystem.

## Best Concepts For Your Stack

These are the most promising concepts, ordered by payoff.

### A. Stack Brain

Role:
- modern controller and translator for the legacy pair

Core behavior:
- clock master / divider / multiplier
- stepped CV lanes for legacy pitch and decay mutation
- envelope follower from audio input, exposed on CV out
- gate logic derived from transients
- optional quantized note lane for pitching one legacy machine in a more musical way

Why it fits:
- your legacy boxes already do dirt and instability well
- they do not give you a clean programmable control center
- `Kastle 2` can provide that without sounding generic

Repo fit:
- uses `Base` sequencer, clock, LFO, analog outputs, gate out, and envelope-following primitives already present

This is the best “system expansion” app if you want the stack to become more coherent and more playable.

### B. Legacy Ingest

Role:
- audio reactor for `Kastle 1.5` and `Kastle Drum`

Core behavior:
- input beat or transient detection
- automatic micro-looping / freeze
- corruption based on trigger density
- CV outputs reflecting detected rhythm, intensity, or spectral tilt

Why it fits:
- the legacy pair is unpredictable in a good way
- `Kastle 2` can turn that unpredictability into controllable structure

Repo fit:
- input path, beat detector, envelope follower, delay, track-and-hold, filters, FFT support

This is probably the best audio-first custom app for your actual three-machine setup.

### C. Mini Citadel For The Stack

Role:
- a “performance shell” app rather than a single synthesis engine

Core behavior:
- one mode is synth/drone
- one mode is audio processor
- one mode is CV utility
- one mode is rhythmic mutation

Why it fits:
- if you only keep one custom firmware on the `Kastle 2`, it should be useful in multiple stack roles

Repo fit:
- `FancyMode`, `FancyPot`, memory-backed settings, LEDs, CV inputs, MIDI

This is the best “one firmware to keep installed most of the time” idea.

### D. Bastard Wave Bard

Role:
- re-sample and deform the legacy pair into new instrument banks

Core behavior:
- build or load banks derived from `Kastle 1.5` and `Kastle Drum`
- emphasize ugly transients, short loops, aliasing, and mismatched scales
- use CV to scan across a bank of self-made artifacts

Why it fits:
- it creates a personal voice fast
- it turns the stack into a recursive ecosystem

Repo fit:
- directly adjacent to `Wave Bard` and sample playback infrastructure

This is the best route if your goal is “I want my Kastle 2 to sound unlike anyone else’s.”

## Strong First App Candidates

If the goal is to actually build something rather than merely ideate, these are the best starting points.

### Candidate 1: Stack Brain

Best first custom app because:
- low sample-memory complexity
- little or no new file format work
- strong utility for the full stack
- can be implemented incrementally

Minimum viable version:
- internal clock
- one stepped CV lane
- one smooth CV lane
- one gate output
- one envelope-following output from audio input

### Candidate 2: Legacy Ingest

Best first audio-reactive app because:
- uses existing input-oriented infrastructure
- complements the `Kastle Drum` well
- can begin as a simple freeze/corrupt processor and grow from there

Minimum viable version:
- audio in
- beat detector / transient detector
- short buffer freeze
- one corruption control
- one CV output derived from input intensity

### Candidate 3: Bastard Wave Bard

Best first personal-voice app because:
- directly leverages the strongest weirdness source you already own: the legacy pair
- does not require inventing an entirely new control language

Minimum viable version:
- start with a custom bank workflow rather than a whole new app
- only become a new app if the stock Wave Bard interaction feels too polite

## Concrete Expansion Themes

These are recurring themes worth pursuing in custom Kastle 2 work:

- `Translator`
  Turn dirty analog events into cleaner CV/gate structure.

- `Destabilizer`
  Turn stable loops or inputs into controlled breakdown.

- `Memory`
  Capture short events and replay them with modulation.

- `Performer`
  Give one firmware multiple stack roles instead of a single fixed identity.

- `Bridge`
  Make the modern Kastle 2 talk musically to the legacy pair.

## What Not To Build First

Avoid these as the first custom app:

- an ordinary subtractive synth
  Reason: `ExampleSynth` already proves the platform can do that, but it does not really expand your stack.

- a generic multi-effect clone of `FX Wizard`
  Reason: you already own `FX Wizard`.

- an over-ambitious sampler with a brand-new format
  Reason: too much file and UX work too early.

- an app that tries to be a full groovebox immediately
  Reason: too many design decisions at once.

## Suggested Build Order

1. Keep using `FX Wizard` and `Wave Bard` as reference personalities.
2. Build a `Stack Brain` prototype first.
3. Use that to learn how you want the `Kastle 2` to talk to the legacy pair.
4. Then choose either:
   - `Legacy Ingest` if audio processing becomes the center
   - `Bastard Wave Bard` if resampling becomes the center
5. Only after that consider a more permanent all-in-one personal firmware.

## Best Immediate Direction

If you want the best next step for this repo, it is:

### Build a new community app tentatively called `StackBrain`

Reason:
- it complements your existing machines
- it does not duplicate `FX Wizard`
- it can later grow into a more complete performance shell
- it should be easier to prototype than a full new sample-based app

Suggested app family slot:
- community firmware range from [APP_LIST.md](/Users/bseverns/Documents/GitHub/kastle2/APP_LIST.md)
- something in the `0x1x` range would be reasonable

