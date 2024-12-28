#include "gba_include.h"

#include "tonccpy.h"

#include "dldi_patch.h"
#include "fatfs/ff.h"
#include "fatfs/dldi.h"
#include "scsd/sc_commands.h"

#define GBA_ROM ((volatile uint32_t*) 0x08000000)
#define GBA_ROM_U8    ((volatile uint8_t*)(0x08000000))

void tryAgain() {
	for (;;) {
		VBlankIntrWait();
	}
}

EWRAM_BSS uint8_t filebuf[0x20000];
EWRAM_BSS FATFS gFatFs;
#define SCSFW_MAGIC 0x57464353

typedef struct SCSFW_PARAMETERS {
	unsigned int scsfw_magic;
	unsigned int miniboot_arm7;
	unsigned int miniboot_arm7_size;
	unsigned int miniboot_arm9;
	unsigned int miniboot_arm9_size;
	unsigned int nds_rom;
	unsigned int nds_rom_size;
	unsigned int sc_lite_dldi;
	unsigned int sc_lite_dldi_size;
	unsigned int scsd_dldi;
	unsigned int scsd_dldi_size;
	unsigned int sccf_dldi;
	unsigned int sccf_dldi_size;
} SCSFW_PARAMETERS;

SCSFW_PARAMETERS parameters;

bool findSCSFWParameters(SCSFW_PARAMETERS* params) {
	tonccpy(params, (void*)&GBA_ROM_U8[0xc0 + 4], sizeof(SCSFW_PARAMETERS));
	if(params->scsfw_magic == 0x57464353) {
		return true;
	}
	// supercard rumble
	tonccpy(params, (void*)&GBA_ROM_U8[0xc0 + 4 + 0x40000], sizeof(SCSFW_PARAMETERS));
	// account for the values being offsetted
	params->miniboot_arm7 += 0x40000;
	params->miniboot_arm9 += 0x40000;
	params->nds_rom += 0x40000;
	params->sc_lite_dldi += 0x40000;
	params->scsd_dldi += 0x40000;
	params->sccf_dldi += 0x40000;
	return params->scsfw_magic == 0x57464353;
}

bool loadDldi(void) {
	if(!findSCSFWParameters(&parameters)) {
		return false;
	}
	SUPERCARD_TYPE type = detect_supercard_type();
	volatile void* targetDldi = NULL;
	if(type & SC_LITE) {
		targetDldi = (volatile void*)&GBA_ROM_U8[parameters.sc_lite_dldi];
	} else if (type == SC_CF) {
		targetDldi = (volatile void*)&GBA_ROM_U8[parameters.sccf_dldi];
	} else {
		targetDldi = (volatile void*)&GBA_ROM_U8[parameters.scsd_dldi];
	}
	dldi_patch_relocate((void*) &_io_dldi_stub, 4, (DLDI_INTERFACE*)targetDldi);
	return true;
}

static inline void setErrorScreenColor(uint32_t color) {
	// set the screen colors, color 0 is the background color
	BG_COLORS[0] = color;
	SetMode(MODE_0 | BG0_ON);
}

int main() {
	FIL kernel;	
	FRESULT err;
	
	if(!loadDldi()) {
		tryAgain();
	}

    if((err = f_mount(&gFatFs, "fat:", 1)) != FR_OK) {
		setErrorScreenColor(RGB8(0, 255, 0));
		tryAgain();
	}

	if ((err = f_open(&kernel, "fat:/scfw/kernel.gba", FA_READ)) != FR_OK
		&& (err = f_open(&kernel, "fat:/scsfw/kernel.gba", FA_READ)) != FR_OK
		&& (err = f_open(&kernel, "fat:/kernel.gba", FA_READ)) != FR_OK
		) {
		setErrorScreenColor(RGB8(0, 255, 255));
		tryAgain();
	}

	uint32_t kernel_size = f_size(&kernel);
	if (kernel_size > 0x02000000) {
		setErrorScreenColor(RGB8(255, 0, 255));
		tryAgain();
	}
	uint32_t total_bytes = 0;
	UINT bytes = 0;
	while(1) {
		if(f_read(&kernel, filebuf, (UINT)sizeof(filebuf), &bytes) != FR_OK) {
			setErrorScreenColor(RGB8(255, 255, 0));
			tryAgain();
		}
		if(bytes == 0)
			break;
		sc_change_mode(en_sdram + en_write);
		tonccpy((void*)&GBA_ROM[total_bytes >> 2], filebuf, bytes);
		if (GBA_ROM[(0 + total_bytes) >> 2] != *(volatile uint32_t*) &filebuf[0]) {
			setErrorScreenColor(RGB8(125, 125, 125));
			tryAgain();
		}
		sc_change_mode(en_sdram + en_sdcard);
		total_bytes += bytes;
	}
	
	if(kernel_size != total_bytes) {
		setErrorScreenColor(RGB8(0, 0, 255));
		tryAgain();
	}

	if (f_error(&kernel)) {
		setErrorScreenColor(RGB8(255, 0, 0));
		tryAgain();
	}

	sc_change_mode(en_sdram);

	if ((*GBA_ROM & 0xff000000) != 0xea000000) {
		setErrorScreenColor(RGB8(125, 125, 0));
		tryAgain();
	}

	SoftReset(ROM_RESTART);
	tryAgain();
}