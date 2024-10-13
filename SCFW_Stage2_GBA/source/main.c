#include <gba.h>
#include "fatfs/ff.h"

#include "new_scsdio.h"

#define GBA_ROM ((vu32*) 0x08000000)

enum
{
	SC_RAM_RO = 0x1,
	SC_MEDIA = 0x3,
	SC_RAM_RW = 0x5,
};

void tryAgain() {
	// iprintf("Critical failure.\nPress A to restart.");
	for (;;) {
		scanKeys();
		if (keysDown() & KEY_A)
			for (;;) {
				scanKeys();
				if (keysUp() & KEY_A)
					((void(*)()) 0x02000000)();
			}
		VBlankIntrWait();
	}
}

EWRAM_BSS u8 filebuf[0x20000];
EWRAM_BSS FATFS gFatFs;

int main() {
	irqInit();
	irqEnable(IRQ_VBLANK);

	// consoleDemoInit();

	// iprintf("SCSFW GBA-mode\n\n");
	
	FIL kernel;
	
	FRESULT err;

    if((err = f_mount(&gFatFs, "fat:", 1)) != FR_OK) {
		// iprintf("Fat mount failed! error: %d\n", (int)err);
		tryAgain();
	}
	if ((err = f_open(&kernel, "fat:/scfw/kernel.gba", FA_READ)) != FR_OK) {
		// iprintf("Kernel file open failed! error: %d\n", (int)err);
		tryAgain();
	}

	u32 kernel_size = f_size(&kernel);
	if (kernel_size > 0x40000) {
		// iprintf("Kernel too large to load!\n");
		tryAgain();
	}
	// iprintf("Loading kernel\n\n");
	// f_rewind(&kernel);

	u32 total_bytes = 0;
	UINT bytes = 0;
	do {
		f_read(&kernel, filebuf, (UINT)sizeof(filebuf), &bytes);
		sc_mode(SC_RAM_RW);
		for (u32 i = 0; i < bytes; i += 4) {
			GBA_ROM[(i + total_bytes) >> 2] = *(vu32*) &filebuf[i];
			if (GBA_ROM[(i + total_bytes) >> 2] != *(vu32*) &filebuf[i]) {
				// iprintf("\x1b[1A\x1b[KSDRAM write failed at\n0x%x\n\n", (int)(i + total_bytes));
			}
		}
		sc_mode(SC_MEDIA);
		total_bytes += bytes;
		// iprintf("\x1b[1A\x1b[K0x%x/0x%x\n", (int)total_bytes, (int)kernel_size);
	} while (bytes);

	if (f_error(&kernel)) {
		// iprintf("Error reading kernel.\n");
		tryAgain();
	}

	sc_mode(SC_RAM_RO);

	if ((*GBA_ROM & 0xff000000) != 0xea000000) {
		// iprintf("Unexpected ROM entrypont, kernel not GBA ROM?");
		tryAgain();
	}

	// iprintf("Kernel loaded successfully.\n");
	// iprintf("Let's go.\n");
	SoftReset(ROM_RESTART);
	// iprintf("Unreachable, panic!\n");
	tryAgain();
}