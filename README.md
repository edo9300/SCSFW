# SCSFW Simple firmware for the Supercard

"Simple" firmware replacement for the SuperCard (all variants).  
This firmware is tailored towards the use of the SuperCard as a NDS SLOT-2 flashcart
rather than as GBA flashcart.  

## Installation
Download the firmware.frm or firmware-rumble.frm (for the SuperCard Rumble) from the [release page](https://github.com/edo9300/SCSFW/releases)
and flash it with apache thunder's [SCKILL](https://github.com/ApacheThunder/SCKILL/releases/latest)  
If you want to use GBA mode (**NOTE: GBA mode doesn't work for the SuperCard Rumble.**),
you need a "custom firmware" setup on your sd, as the
firmware will attempt to load `kernel.gba` from root, the `scsfw` folder or the `scfw` folder.
You can use [SCFW](https://github.com/metroid-maniac/SCFW)
or [SuperFW](https://gbatemp.net/threads/superfw-a-very-much-wip-supercard-firmware.654847/)
as the GBA mode firmware.

## Usage
Boot in NDS mode to launch [nds-hbmenu](https://github.com/devkitPro/nds-hb-menu), and from there
load other homebrews (for example [TWiLightMenu](https://github.com/DS-Homebrew/TWiLightMenu/))  
For the non Rumble variants, the firmware will store extra information about the currently inserted
card allowing homebrews accounting for it (like https://github.com/edo9300/nitrohax-usercheat-supercard)
to boot the inserted card without hotswapping

Boot in GBA mode to launch the previously set up `kernel.gba` from the sd.  
**NOTE: GBA mode doesn't work for the SuperCard Rumble.**

## Technical Functioning
The GBA mode is straightforward, the embedded `SCFW_Stage2_GBA.gba` rom is loaded as multiboot rom, and booted,
the currently provided rom will attempt to chainload the `kernel.gba` file.  
The NDS mode is a multi stage process, for the initial entrypoint, a stripped down version
of [nds-miniboot](https://github.com/asiekierka/nds-miniboot) is launched, miniboot will then
take care of loading the embedded `SCFW_Stage2_NDS.NDS`  (currently defaulting to [nds-hbmenu](https://github.com/devkitPro/nds-hb-menu))
from the flash in a clean environment, also dldi patching it appropriately
depending on the type of SuperCard used, using either `scsd.dldi` or `sc-lite.dldi` from the dldi folder  
If the the SuperCard is launched in NDS mode with another game cart inserted in the slot1 (via flashme),
it will store this information at the beginning of its internal ram, so that other applications can use it:
```
typedef struct SUPERCARD_RAM_DATA {
	uint8_t magicString[8]{'S', 'C', 'S', 'F', 'W', 0, 0, 0};
	uint8_t header[0x200];
	uint8_t secure_area[0x4000];
	uint32_t chipid;
} SUPERCARD_RAM_DATA;
```

(note the header will have some fields altered by flashme, but they can be reconstructed, check
[nitrohax-usercheat-supercard](https://github.com/edo9300/nitrohax-usercheat-supercard/blob/6354f044f9b85b7e78d8d6591204149455d1e368/arm9/source/main.cpp#L393-L421)
for how to do it)

## Building
Blocksds is required with `wf-tools`, `target-gba`, and `thirdparty-blocksds-toolchain` installed

From a blocksds shell run
```
make -C SCFW_Stage2_GBA
```
to generate the GBA bootloader

If you want to use a different ds homebrew from hbmenu, replace the `SCFW_Stage2_NDS.NDS`
file in the `SCFW_Stage2_NDS`

From a blocksds shell run
```
make -C SCFW_Stage1
```
to generate the final firmware

Keep in mind that the final `firmware.frm` file must not be bigger than `0x80000` bytes
for the SuperCard SD/miniSD, `0x7C000` for the SuperCard Lite, and `0x200000` for the SuperCard
Rumble (in the case of the SuperCard rumble, the built firmware will always be padded with `0x40000`
at the beginning as that region is read only on the card)

## Credits
[nds-miniboot](https://github.com/asiekierka/nds-miniboot) - The bootloader used to launch the bundled homebrew  
[metroid maniac](https://github.com/metroid-maniac) - The gba loader code, and initial NDS/GBA loading code  
[Archeychen](https://github.com/ArcheyChen) - Initial SuperCard SD/MiniSD dldi with SDHC support  
[edo9300](https://github.com/edo9300) - SuperCard Lite dldi driver
