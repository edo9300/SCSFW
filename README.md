#  SCFW: Bleeding-edge modular kernel branch

This branch is primarily for those who'd like to test out/play with new revisions of the kernel that have yet to be pushed to the main branch.

## Installation
1 - Download the zip file and extract the contents to the root directory of the sdcard.  
2 - This should replace the existing kernel.gba along with adding multiple emulator support.  
>[!TIP]
> You can also select the `firmware.frm` file from within the kernel to flash SCFW to the Supercard's firmware. Because the firmware is minimal and the kernel is loaded from the SD card, updates to the firmware should be rare. You can enjoy kernel updates without updating the firmware.  

## Prerequisites for emulator use
For Game Boy / Game Boy Color, you need to download your preferred Goomba fork/binary and rename it to:
- `gbc.gba`
    - For Game Boy Color emulation
- `gb.gba`
	- For Game Boy emulation

For NES/Famicom, you need to download your preferred PocketNES fork/binary and rename it to:
- `nes.gba`

For Sega Master System, Game Gear, and Sega Game 1000 (Sega 1000), you need to download your preferred SMSAdvance fork/binary and rename it to:
- `smsa.gba`

For NEC PC-Engine/TurboGrafx-16, you need to download your preferred PCEAdvance fork/binary and rename it to:
- `pcea.gba`

For Watara/Quickshot Supervision, you need to download your preferred WasabiGBA fork/binary and rename it to:
- `wsv.gba`

For Neo Geo Pocket / Color, you need to download your preferred NGPGBA fork/binary and rename it to:
- `ngp.gba`

For Home Video Computer Advance, builds require additional work - but after doing so you can rename the binary to:
- `hvca.gba`
    * The base.bin file of hvca requires gbafix and an addiontal step:
        * _"Like retail games, this emulator tries to increase the ROM speed which supercard is not compatible with, I just disabled that"_ - Metroid Maniac

For SwanGBA, you need to download OR build your preferred SwanGBA fork/binary along with the needed dependencies and rename it to:
- `bwsc.gba`  

For CoG, I bundled in a modified version for you to use thus making it modular. Already on the last version (CoG_Beta_0.9.7). The build is named:
- `cog.gba`
> [!TIP]
> Unlike Cologne, TheHiVE has implemented his own version of the Colecovision BIOS and embedded it in CoG. No external BIOS is needed.

For Cologne, you need to download OR build your preferred Cologne fork/binary along with the needed dependencies and rename it to:
- `cologne.gba`
> [!IMPORTANT]
> Cologne **REQUIRES** a Colecovision BIOS before you can use it. It will NOT work without it. Obtain any Colecovision BIOS and rename it to **[BIOS].col** then place it in the /scfw/ folder
  
>[!NOTE]
> I also build forks so you can also find the latest [SwanGBA binaries here](https://github.com/OmDRetro/SwanGBA-SCFW/releases)

For DrSMS, builds require additional work - but after doing so you can rename the binary to:
- `drsms.gba`

## Other applications

For Music Player Advance 2 by NEiM0D, there's only one known version I found and it's found on [archive.org](https://web.archive.org/web/20181020204131/http://www.cellularmobilephones.com/gba_net/MusicPlayer_Advance_2.zip). Simply rename `MPLAYERA.GBA` to `mpa.gba` and gbafix it before placing it inside the scfw folder.  
>[!NOTE]
>You can find a copy of the [MPAC ROM compiling tool here](https://github.com/OmDRetro/MPA2-Compilation-Tools). That repository should have the instructions necessary for you to play some low bitrate beats on your GBA

For Ebook Advance by Daniel Cotter(txt file reader), the latest version is found on [archive.org](https://web.archive.org/web/20070512224717/http://members.optushome.com.au:80/dancotter/ebook.htm). There are two versions here namely:  
- Ebook Advance - Variable width font **Vertical Version** (ebook03s.zip)
    - Simply extract `ebook.gba` found in the archive, gbafix it, then rename it to `txt_s.gba` then place it in the scfw folder.
- Ebook Advance - Variable width font **Horizontal Version** (ebook03.zip)
    - Simply extract `ebook.gba` found in the archive, gbafix it, then rename it to `txt.gba` then place it in the scfw folder.

Once you have those files, transfer these to the scfw folder.
You should find the ff. within the scfw folder:
- bwsc.gba
- cog.gba (Modified by me)
- cologne.gba
- drsms.gba (Custom built version by metroid-maniac)
- gb.gba
- gbc.gba
- hvca.gba (Custom built version by metroid-maniac)
- kernel.gba
- mpa.gba
- nes.gba
- ngp.gba
- pcea.gba
- smsa.gba
- txt_s.gba
- txt.gba
- wsv.gba
- ./hvca/ (folder)
- ./hvca/mapr/ (folder)

## Differences between this and the main kernel:
- Goomba support ✅
    - Loads Game Boy Color games (*.gbc)
    - Loads Game Boy games (*.gb)
- PCEAdvance support ✅
	- Loads PC-Engine/TurboGrafx-16 games (*.pce)
- PocketNES support ✅
    - Loads NES / Famicom games (*.nes)
    - Automatic ROM region detection (PAL / NTSC timing)
- SMSAdvance support ✅
	- Loads Game Gear games (*.gg)
	- Loads Sega Game 1000 / Sega 1000 games (*.sg)
    - Loads Sega Master System games (*.sms)
	- Custom BIOS loading support. Can be toggled within kernel settings
- WasabiGBA support ✅
    - Loads Watara/Quickshot Supervision games (*.sv)
	- Custom BIOS loading support. Can be toggled within kernel settings
- NGPGBA support ✅
    - Loads Neo Geo Pocket games (*.ngp)
	- Loads Neo Geo Pocket Color games (*.ngc)
	- Custom BIOS loading support. Can be toggled within kernel settings
- HVCA support ✅
    - Loads Famicom Disk System games (*.fds)
	- Plays Nintendo Sound Files (*.nsf)
- SwanGBA support ✅
    - Loads Benesse Pocket Challenge V2 games (*.pc2)
	- Loads WonderSwan games (*.ws)
	- Loads WonderSwan Color games (*.wsc)
- DrSMS support ✅
	- Loads Game Gear games (*.gg)
    - Loads Sega Master System games (*.sms)
	    - Supports FM Audio (Sega Master System Mark III)
	- Can use kernel settings to switch between SMSAdvance and DrSMS for SMS/GG ROM loading.
- E-Book Advance support ✅
	- Loads TXT files (*.txt)
- Music Player Advance 2 support ✅
    - Loads Music Player Advance files (*.mpa)
	- Loads Music Player Advance Compilation files (*.mpac)
- CoG support ✅
    - Loads Colecovision games (*.col)
	- Supports a larger library of Colecovision games
	- Can use kernel settings to switch between CoG and Cologne for Colecovision ROM loading
- Cologne support ✅
    - Loads Colecovision games (*.col)
	- Has customizable controls while in-game
	 
## Emu loading observation
- ✅ Stable on:
    - Original hardware:
        - GBA
        - NDS / NDSL ~ Game Boy mode
- ❌ Inconsistent:
    - Clone consoles:
        - EXEQ Game Box (clone console)
	    - DIGI RETROBOY / REVO K101 Plus
	
## Observed emulator quirks
System | Emulator | Quit to firmware | Soft reset | Modular
:-:|:-:|:-:|:-:|:-:
Game Boy | Goomba / Super Goomba / Goomba Color | ✔ | ✔ | ✔
Game Boy Color | Jagoomba Color / Goomba Color | ✔ | ✔ | ✔
Nintendo Entertainment System / Family Computer | PocketNES | ⚠ | ✔ | ✔
Sega Master System | SMSAdvance / DrSMS | ❌ | ✔ | ✔ / ‼
Sega Game Gear | SMSAdvance / DrSMS | ❌ | ✔ | ✔ / ‼
Sega Game 1000 / Sega 1000 | SMSAdvance | ❌ | ✔ | ✔
NEC PC-Engine / TurboGrafx-16 | PCEAdvance | ❌ | ❌ | ✔
Watara/Quickshot Supervision | WasabiGBA | ⚠ | ✔ | ✔
Neo Geo Pocket / Color | NGPGBA | ⚠ | ✔ | ✔
Famicom Disk System / NSF Player | HVCA | ❌ | ❌ | ‼
Bandai WonderSwan/WonderSwan Color / Benesse Pocket Challenge V2 | SwanGBA | ❌ | ✔ | ✔
Colecovision | CoG / Cologne | ❌ / ❌ | ✔ / ❌ | ‼ / ✔
> **_LEGEND:_**
> > * ‼ ~ Requires some technical know-how to get working / Modular to an extent, but requires additional work.
> > * ⚠ ~ Varies per fork / version OR works with some caveats(buggy). Use with caution
> > * ❌ ~ Unsupported / Not functioning as intended
> > * ✔ ~ Supported / Works as intended

>[!NOTE]
> Emulator binaries can be improved upon and can be made compatible with the kernel just like Goomba.
> * IF QUIT TO FIRMWARE DOESN'T WORK, USE THE SOFT RESET METHOD TO QUIT TO FIRMWARE.
>     * Soft reset key combination: **START** + **SELECT** + **A** + **B**
	
##NOTES
> [!WARNING]
> Some GBAOAC devices such as the EXEQ Game Box SP don't play nice with flash carts as it doesn't have the same wait time. Thus, ROMs boot faster and the flash cart does not have enough time to prepare. Try to toggle "Boot games through BIOS" each time you exit an emu ROM or game.
> - Alternative method for GBAOC devices: Create a ROM compilation and sideload the resulting gba file. This process is tedious, but it works best for clones like these.

> [!CAUTION]
> The cart **appears** to not have enough time to properly load both emulator and ROM if you skip the BIOS. It's better to leave that kernel option "Boot games through BIOS" as 1 (on).

## Links / Binaries
[GBATemp Bleeding-edge kernel thread](https://gbatemp.net/threads/scfw-bleeding-edge-modular-kernel-branch.656629/)

## Credits
[metroid maniac](https://github.com/metroid-maniac) - Main developer  
[Archeychen](https://github.com/ArcheyChen) - Early development into another loader, SDHC support  
[OmDRetro](https://github.com/OmDRetro) - Kernel enhancements, significantly more supported filetypes  
[RocketRobz](https://github.com/RocketRobz) - Twilightmenu++ "gbapatcher" code for patching Supercard ROMs  
[SiliconExarch](https://github.com/SiliconExarch) - Finding an old DevkitARM release with a functioning Supercard SD drive  
