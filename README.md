# SCSFW Simple firmware for the Supercard

Simple firmware replacement for the Supercard based off [metroid maniac's](https://github.com/metroid-maniac/SCFW) [SCFW](https://github.com/metroid-maniac/SCFW).
The gba kernel part got stripped out from the SCFW repo, and only the firmware related code is left.
The gba side was left unchanged in behaviour from SCFW, but the used dldi was updated to the latest version from https://github.com/ArcheyChen/SuperCard-SDHC-DLDI.
The passme mode instead now is set up to boot devkitpro's [nds-hbmenu](https://github.com/devkitPro/nds-hb-menu)

## Installation
Set up [SCFW](https://github.com/metroid-maniac/SCFW) on your supercard, and launch the `firmware.frm` from it to flash it.

## Usage
To use in GBA mode, a setup with SCFW is expected
When used in passme mode, nds hbmenu will be loaded, and from there other homebrews can be loaded from the SD

## Building
With devkitpro set up for gba development, with libgba and libfat-gba installed, run `make` in the `SCFW_Stage2_GBA` folder.

If you want to use a different ds homebrew from hbmenu, replace the `SCFW_Stage2_NDS.NDS` file in the `SCFW_Stage2_NDS`, if dldi patching is required, patch it with `scsd-sdhc-dldi.dldi`

Assuming you have the devkitpro tools in your path, run from the `SCFW_Stage1` folder
```
arm-none-eabi-as scfw.s && arm-none-eabi-objcopy -O binary a.out firmware.frm && gbafix firmware.frm
```

Keep in mind that the final `firmware.frm` file must not be bigger than `0x80000` bytes

## Credits
[metroid maniac](https://github.com/metroid-maniac) - SCFW and the gba loader code  
[Archeychen](https://github.com/ArcheyChen) - SDHC support  
[SiliconExarch](https://github.com/SiliconExarch) - Finding an old DevkitARM release with a functioning Supercard SD drive
