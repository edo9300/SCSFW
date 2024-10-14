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
	for (;;) {
		VBlankIntrWait();
	}
}

EWRAM_BSS u8 filebuf[0x20000];
EWRAM_BSS FATFS gFatFs;

int main() {
	FIL kernel;	
	FRESULT err;

    if((err = f_mount(&gFatFs, "fat:", 1)) != FR_OK) {
		tryAgain();
	}
	if ((err = f_open(&kernel, "fat:/scfw/kernel.gba", FA_READ)) != FR_OK) {
		tryAgain();
	}

	u32 kernel_size = f_size(&kernel);
	if (kernel_size > 0x40000) {
		tryAgain();
	}
	u32 total_bytes = 0;
	UINT bytes = 0;
	do {
		f_read(&kernel, filebuf, (UINT)sizeof(filebuf), &bytes);
		sc_mode(SC_RAM_RW);
		for (u32 i = 0; i < bytes; i += 4) {
			GBA_ROM[(i + total_bytes) >> 2] = *(vu32*) &filebuf[i];
			if (GBA_ROM[(i + total_bytes) >> 2] != *(vu32*) &filebuf[i]) {
			}
		}
		sc_mode(SC_MEDIA);
		total_bytes += bytes;
	} while (bytes);

	if (f_error(&kernel)) {
		tryAgain();
	}

	sc_mode(SC_RAM_RO);

	if ((*GBA_ROM & 0xff000000) != 0xea000000) {
		tryAgain();
	}

	SoftReset(ROM_RESTART);
	tryAgain();
}