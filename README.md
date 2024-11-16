# SCSFW Simple firmware for the Supercard

Simple firmware replacement for the Supercard based off [metroid maniac's](https://github.com/metroid-maniac/SCFW)
[SCFW](https://github.com/metroid-maniac/SCFW).
The gba kernel part got stripped out from the SCFW repo, and only the firmware related code is left.
The gba side was left unchanged in behaviour from SCFW, but the used dldi was updated to the latest version from https://github.com/ArcheyChen/SuperCard-SDHC-DLDI.
The passme mode instead now is set up to boot devkitpro's [nds-hbmenu](https://github.com/devkitPro/nds-hb-menu)
This firmware ships with 2 dldis, one for the Supercard micro/mini/SD (also compatible but very slow with the Supercard Lite)
and one specifically for the Supercard Lite (the fastest Nintendo DS dldi driver ever).
They are in the dldi folder, called `scsd.dldi` and `sc-lite.dldi`
The drivers are from https://github.com/edo9300/SuperCard-SDHC-DLDI/ and https://github.com/edo9300/SuperCard-SDHC-DLDI/tree/supercard-lite respectively
Another quirk of this firmware, is that in ds mode, it copies the currently decrypted secure area for a game in slot1,
allowing homebrews to directly boot that cart without hotswapping, like https://github.com/edo9300/nitrohax-usercheat-supercard

## Installation
Download the prebuild firmware.frm, or build it yourself, and flash it with apache thunder's [SCKILL](https://github.com/ApacheThunder/SCKILL/releases/latest)

## Usage
To use in GBA mode, a setup with SCFW is expected
When used in passme mode, nds hbmenu will be loaded, and from there other homebrews can be loaded from the SD

## Building
Blocksds is required with `wf-tools`, `target-gba`, and `thirdparty-blocksds-toolchain` installed

From a blocksds shell run
```
make -C SCFW_Stage2_GBA
```
to generate the GBA bootloader

If you want to use a different ds homebrew from hbmenu, replace the `SCFW_Stage2_NDS.NDS` file in the `SCFW_Stage2_NDS`

From a blocksds shell run
```
make -C SCFW_Stage1
```
to generate the final firmware

Keep in mind that the final `firmware.frm` file must not be bigger than `0x80000` bytes

## Credits
[nds-miniboot](https://github.com/asiekierka/nds-miniboot) - The bootloader used to launch the bundled homebrew
[metroid maniac](https://github.com/metroid-maniac) - The gba loader code
[Archeychen](https://github.com/ArcheyChen) - SDHC support
