# SCSFW Simple firmware for the Supercard

Simple firmware replacement for the Supercard based off [metroid maniac's](https://github.com/metroid-maniac/SCFW) [SCFW](https://github.com/metroid-maniac/SCFW).
The gba kernel part got stripped out from the SCFW repo, and only the firmware related code is left.
The gba side was left unchanged in behaviour from SCFW, but the used dldi was updated to the latest version from https://github.com/ArcheyChen/SuperCard-SDHC-DLDI.
The passme mode instead now is set up to boot devkitpro's [nds-hbmenu](https://github.com/devkitPro/nds-hb-menu)
This firmware ships with 2 dldis, one for the Supercard micro/mini/SD (also compatible but very slow with the Supercard Lite)
and one specifically for the Supercard Lite (the fastest Nintendo DS dldi driver ever).
They are in the dldi folder, called `scsd.dldi` and `sc-lite.dldi`
The drivers are from https://github.com/edo9300/SuperCard-SDHC-DLDI/ and https://github.com/edo9300/SuperCard-SDHC-DLDI/tree/supercard-lite respectively

## Installation
Set up [SCFW](https://github.com/metroid-maniac/SCFW) on your supercard, and launch the `firmware.frm` from it to flash it.

## Usage
To use in GBA mode, a setup with SCFW is expected
When used in passme mode, nds hbmenu will be loaded, and from there other homebrews can be loaded from the SD

## Building
Both blocksds and devkitpro (if you want to build a different GBA booter) are required

If you want to build a different GBA booter:
With devkitpro set up for gba development, with libgba and libfat-gba installed, run `make` in the `SCFW_Stage2_GBA` folder.

If you want to use a different ds homebrew from hbmenu, replace the `SCFW_Stage2_NDS.NDS` file in the `SCFW_Stage2_NDS`

From a blocksds shell, run from the `SCFW_Stage1` folder
```
make
```

Keep in mind that the final `firmware.frm` file must not be bigger than `0x80000` bytes

## Credits
[nds-miniboot](https://github.com/asiekierka/nds-miniboot) - The bootloader used to launch the bundled homebrew  
[metroid maniac](https://github.com/metroid-maniac) - The gba loader code  
[Archeychen](https://github.com/ArcheyChen) - SDHC support  
