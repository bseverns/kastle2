## StackBrain MIDI Ideas

These are the next MIDI directions that make sense for `StackBrain`.

The goal is not “generic MIDI support.”
The goal is:
- make MIDI behave like another patch source
- let MIDI reinforce the stack logic
- keep the firmware centered on clocks, lanes, and mutation

## Good Next MIDI Directions

### 1. MIDI Mode Addressing

Use MIDI notes or program changes to:
- select `STEP`, `FOLLOW`, or `CHAOS`
- recall routing personalities
- recall clock-policy personalities

This is higher value than adding lots of isolated CCs.

### 2. MIDI Scene Recall

This now exists in a first useful form:
- `CC 67` selects the scene slot
- `CC 68` recalls the selected scene
- `CC 69` stores the current scene

The next step is not “add scenes.” The next step is to make scenes more expressive:
- note-driven scene address selection
- scene-morph time or curve control
- routing-personality scenes distinct from raw pot snapshots

### 3. MIDI Clock As Structural Input

Current firmware already follows MIDI clock.

The next level would be:
- multipliers and dividers beyond the current single divider
- swing or pulse-skipping tied to `CHAOS`
- different clock treatment by mode

Examples:
- `STEP`: clean divided clock
- `FOLLOW`: recovered pulse can push or ignore MIDI clock depending on policy
- `CHAOS`: MIDI clock as stable base while internal mutation creates off-grid accents

### 4. MIDI Notes As Lane Addressing

Instead of only using note pitch as a bias:
- use note ranges to address lane personalities
- low notes choose output relationship
- middle notes choose event density family
- high notes fire accents, bursts, or resets

This fits `StackBrain` better than trying to make it a normal monosynth.

### 5. MIDI CC Output Telemetry

The firmware could also report its own state:
- current mode
- main lane intensity
- companion lane intensity
- gate density
- clock division
- chaos burst or reseed events

This would make it easier to pair with external sequencers or software without losing the patch-first identity.

### 6. MIDI Learn Or User Mapping

Only worth doing if it stays compact.

Good target:
- remap the 7 normal controls
- remap the 7 shift controls
- keep the engine semantics fixed

Bad target:
- arbitrary per-mode MIDI matrix with no clear panel logic

### 7. Bidirectional Hybrid Use

Most interesting long-term direction:
- MIDI clock drives the internal pulse
- MIDI notes bias or address lanes
- analog patch inputs still mutate the live behavior
- outputs still drive the legacy Kastles

That keeps `StackBrain` in its intended role:
- not a normal MIDI effect
- not a normal CV utility
- a translator between structured digital control and unstable analog patch behavior

## Practical Recommendation

If we keep building from the current firmware, the best next MIDI implementation steps are:
1. MIDI mode selection beyond the current pot/CC scene model
2. MIDI CC output telemetry
3. scene-address notes plus morph-time or morph-curve control

Those are more valuable than adding dozens of extra inbound CC assignments.
