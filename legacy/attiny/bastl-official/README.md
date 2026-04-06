## Bastl Legacy ATtiny Assets

This directory vendors legacy ATtiny-oriented headers from the official Bastl `kastle` repository so this repo can act as a single workspace for the old dual-ATtiny Kastles and the newer Kastle 2 platform.

Source:
- Repository: `https://github.com/bastl-instruments/kastle`
- Vendored from local clone on `2026-04-05`
- Canonical source paths:
  - `kastleDrum/bastlAttinySamples/`
  - `kastleDrum/TR_HH_AT.h`
  - `kastleSampler/SNARE2_AT.h`
  - `kastleSampler/TR_KICK_AT.h`

License and attribution:
- Bastl's legacy repo states the Kastle software and schematics are `CC BY-SA`.
- Original author attribution in the upstream repo credits Vaclav Pelousek / Bastl Instruments.
- Treat these files as vendored third-party legacy assets, not original work in this repo.

Notes:
- This subtree is intentionally separate from the RP2040 Kastle 2 firmware under `code/`.
- The `bastlAttinySamples/` folder contains the shared waveform and drum/sample headers used by multiple legacy ATtiny sketches.
- The standalone `TR_HH_AT.h`, `SNARE2_AT.h`, and `TR_KICK_AT.h` headers are included because some official legacy sketches reference them directly outside the shared sample folder.
- The vendored sketch directories live under `sketches/` and preserve the upstream folder layout closely enough to stay useful as a reference workspace.

Included sketch directories:
- `kastleArp`
- `kastleDRUM_CLK_LFO`
- `kastleDrum`
- `kastleKarplus`
- `kastleNEW_CORe`
- `kastleSampler`
- `kastleSamplerNoise`
- `kastleSynth_DUAL_STEP`
- `kastleSynth_LFO`
- `kastleSynthe_BIT`
- `kastleSynthe_FOLD`
- `kastleSynthe_VCO`
- `kastleSynthe_VCO_2`
- `snowTrap`

Not included:
- `SMT_Kastle` was not vendored because it does not appear to add a distinct ATtiny sketch entrypoint in the same way as the folders above.
