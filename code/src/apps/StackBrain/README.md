## StackBrain

`StackBrain` is a community Kastle 2 app that treats the machine as an audio-reactive control processor for a legacy stack.

Current implemented mode arms:
- `STEP`: clocked procedural sequencer with contour-sliced audio
- `FOLLOW`: recovered-pulse translator with envelope/transient/beat analysis and event-shaped delay
- `CHAOS`: burst-capable rungler mutation mode with paired CV lanes, reseeds, and clock-linked delay corruption

The app owns:
- `CV_OUT`
- `ENV_OUT`
- `GATE_OUT`
- `SYNC_OUT`
- the full audio path

Related notes:
- [STACKBRAIN_CONTROLS.md](/Users/bseverns/Documents/GitHub/kastle2/STACKBRAIN_CONTROLS.md)
- [STACKBRAIN_MIDI_IDEAS.md](/Users/bseverns/Documents/GitHub/kastle2/STACKBRAIN_MIDI_IDEAS.md)

## Local Build Note

This repo can now build `stackbrain` using local vendored tooling:
- Pico SDK at `code/pico-sdk`
- ARM toolchain at `tools/arm-gnu-toolchain/bin`
- picotool config at `tools/picotool`

If those directories exist, [configure.sh](/Users/bseverns/Documents/GitHub/kastle2/configure.sh) will use them automatically.

Typical local flow:

```bash
./configure.sh
cmake --build code/build --target stackbrain -j4
```

If you want to keep a separate build tree outside `code/build`, this also works:

```bash
cmake -S code -B build-local -G "Unix Makefiles" \
  -DPICO_SDK_PATH="$(pwd)/code/pico-sdk" \
  -DPICO_TOOLCHAIN_PATH="$(pwd)/tools/arm-gnu-toolchain/bin" \
  -Dpicotool_DIR="$(pwd)/tools/picotool" \
  -DPICOTOOL_FETCH_FROM_GIT_PATH="$(pwd)/tools"

cmake --build build-local --target stackbrain -j4
```

Current output artifact:
- `build-local/output/kastle2-stackbrain.uf2`

## Current Boundaries

What exists now:
- mode switching by the `MODE` button
- shared output ownership across all modes
- one coherent control/audio engine rather than separate utility and FX personalities
- EEPROM-backed persistence for mode and the main pot set
- a full normal / shift / mode-layer control architecture
- scene recall/store on top of the mode-layer routing and personality model
- short scene morphing on recall instead of hard-switching the pot state
- soft-takeover scene latching so recalled values stay active until the matching knobs are brought near them
- shift-layer engine controls for pitch CV, link, event bias, and MIDI depth
- active use of `PITCH_1`, `PITCH_2`, `MODE`, `SYNC_IN`, and `FEED_1/2/3`
- active MIDI clock / note / CC / scene handling

## Pot Semantics

The same seven pots stay physically consistent across all modes, but their meanings change by mode:

### `STEP`
- `POT_5`: internal clock rate
- `POT_6`: CV span
- `POT_4`: pattern shape, trigger family, and audio contour depth
- `POT_7`: trigger density
- `POT_3`: companion slew
- `POT_1`: external CV amount for rate
- `POT_2`: external CV amount for shape
- `MODE` CV input: pattern-density / shape-pressure bias
- `SYNC_IN`: accent and contour re-trigger input

### `FOLLOW`
- `POT_5`: follower speed
- `POT_6`: output span for `CV_OUT` and `ENV_OUT`
- `POT_4`: audio response and filter intensity
- `POT_7`: transient threshold / gate density bias
- `POT_3`: companion slew
- `POT_1`: external CV amount for follower speed
- `POT_2`: external CV amount for response shaping
- `MODE` CV input: threshold bias / selectivity

### `CHAOS`
- `POT_5`: mutation clock rate
- `POT_6`: chaos range
- `POT_4`: rungler bit behavior, reseed intensity, and audio aggression
- `POT_7`: probability of active steps and burst pressure
- `POT_3`: companion slew
- `POT_1`: external CV amount for chaos rate
- `POT_2`: external CV amount for mutation behavior
- `MODE` CV input: burst/reseed pressure bias
- `SYNC_IN`: burst injection and accent forcing

## Scene Gestures

- quick `SHIFT` tap: recall the next scene slot with a short morph
- hold `MODE`, then quick `SHIFT` tap: store the current state into the active scene slot
- MIDI `CC 67`: select scene slot
- MIDI `CC 68`: recall selected scene
- MIDI `CC 69`: store current state into the selected scene

## LED Semantics

- `LED_1`: mode identity with brightness tied to that mode's current energy or CV movement
- `LED_2`: event/accent indication
- `LED_3`: clock source or transient/gate indication, depending on mode

## Hardware Tuning Checklist

Real tuning still needs the actual machines. The shortest useful checklist is:
- `FOLLOW`: verify transient threshold against `Kastle Drum` main out and noise out
- `FOLLOW`: verify `ENV_OUT` smoothing feels distinct from `CV_OUT`, not just lagged
- `CHAOS`: verify burst generation stays exciting rather than constant once `POT_7` crosses the upper third
- `CHAOS`: verify reseed frequency in the upper half of `POT_4` feels intentional rather than annoying
- `CHAOS`: verify the new delay smear remains useful on short percussive input as well as drones
- `CHAOS`: verify accent asymmetry still survives mono collapse in a musically useful way
- `STEP`: verify external trigger takeover feels immediate and clean when clocked from the legacy pair
- all modes: verify output voltages feel useful on `Kastle 1.5` pitch/timbre and `Kastle Drum` pitch/decay/drum-select inputs

What still needs iteration:
- more explicit performance UI feedback for the active scene slot
- dedicated MIDI telemetry / CC output reporting
- deeper control over morph time or morph shape
- tighter tuning of `FOLLOW` thresholds and the new `CHAOS` burst/reseed thresholds on real hardware
