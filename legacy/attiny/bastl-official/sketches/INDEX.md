## Legacy Sketch Index

This index summarizes the vendored official Bastl legacy ATtiny sketch folders under this directory.

Lower-risk starting points:
- `kastleSynthe_VCO`: stock synth sound chip
- `kastleSynth_LFO`: stock synth modulation chip
- `kastleDrum`: stock drum sound chip
- `kastleDRUM_CLK_LFO`: stock drum modulation/clock chip
- `kastleArp`: melodic/chordal voice
- `kastleKarplus`: plucked/string-oriented voice

Good alternate sound sketches:
- `kastleSampler`: sample-driven sound voice
- `kastleSamplerNoise`: noisier sample-driven voice
- `kastleSynthe_BIT`: harsher digital/bytebeat-like sound voice
- `kastleSynthe_FOLD`: folded/distorted synth voice
- `kastleSynthe_VCO_2`: later synth variant with expanded behavior

Good alternate modulation sketch:
- `kastleSynth_DUAL_STEP`: stepped modulation / pattern-style behavior

More experimental:
- `kastleNEW_CORe`
- `snowTrap`

Notes:
- These folders are vendored from the official Bastl legacy repo and kept mostly as upstream reference material.
- Some directories include their own local headers or duplicate sample assets; this is intentional to preserve upstream structure.
- The shared canonical sample/header pool for this workspace also exists one level up in `../bastlAttinySamples/` plus the standalone `TR_HH_AT.h`, `SNARE2_AT.h`, and `TR_KICK_AT.h`.
