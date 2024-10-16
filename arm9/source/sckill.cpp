#include <nds.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <fat.h>
#include <stdio.h>
#include <nds/arm9/console.h>

#include "font.h"
#include "tonccpy.h"

#define SC_MODE_REG 			*(vu16*)0x09FFFFFE
#define SC_MODE_MAGIC			(u16)0xA55A
#define SC_MODE_FLASH_RW		(u16)0x4
#define SC_MODE_FLASH_RW_LITE	(u16)0x1510


#define SCLITE_FLASH_MAGIC_ADDR_1	(*(vu16*) 0x08000AAA)
#define SCLITE_FLASH_MAGIC_ADDR_2	(*(vu16*) 0x08000554)
#define SC_FLASH_MAGIC_ADDR_1	(*(vu16*) 0x08000b92)
#define SC_FLASH_MAGIC_ADDR_2	(*(vu16*) 0x0800046c)
#define SC_FLASH_MAGIC_1		((u16) 0xaa)
#define SC_FLASH_MAGIC_2		((u16) 0x55)
#define SC_FLASH_ERASE			((u16) 0x80)
#define SC_FLASH_ERASE_BLOCK	((u16) 0x30)
#define SC_FLASH_ERASE_CHIP		((u16) 0x10)
#define SC_FLASH_PROGRAM		((u16) 0xA0)
#define SC_FLASH_IDLE			((u16) 0xF0)
#define SC_FLASH_IDENTIFY		((u16) 0x90)

#define FlashBase				0x08000000
#define MaxFirmSize				0x80000

extern u8 scfw_bin[];
extern u8 scfw_binEnd[];

u8* scfw_buffer;

static PrintConsole tpConsole;
static PrintConsole btConsole;

extern PrintConsole* currentConsole;


static int bg;
static int bgSub;

const char* textBuffer = "X------------------------------X\nX------------------------------X";

volatile u32 cachedFlashID;
volatile u32 statData = 0x00000000;
volatile u32 firmSize = 0x80000;
volatile bool UpdateProgressText = false;
volatile bool PrintWithStat = true;
volatile bool ClearOnUpdate = true;
volatile bool SCLiteMode = false;
volatile bool FileSuccess = false;


u32 sc_flash_id() {
	SC_FLASH_MAGIC_ADDR_1 = SC_FLASH_MAGIC_1;
	SC_FLASH_MAGIC_ADDR_2 = SC_FLASH_MAGIC_2;
	SC_FLASH_MAGIC_ADDR_1 = SC_FLASH_IDENTIFY;
	
	// should equal 0x000422b9
	u32 res = SC_FLASH_MAGIC_ADDR_1;
	res |= *GBA_BUS << 16;
	
	*GBA_BUS = SC_FLASH_IDLE;
	
	return res;
}

void sc_flash_rw_enable() {
	bool buf = REG_IME;
	REG_IME = 0;
	SC_MODE_REG = SC_MODE_MAGIC;
	SC_MODE_REG = SC_MODE_MAGIC;
	SC_MODE_REG = SC_MODE_FLASH_RW;
	SC_MODE_REG = SC_MODE_FLASH_RW;
	REG_IME = buf;
}

void sc_flash_rw_enable_lite() {
	SC_MODE_REG = SC_MODE_MAGIC;
	SC_MODE_REG = SC_MODE_MAGIC;
	SC_MODE_REG = SC_MODE_FLASH_RW_LITE;
	SC_MODE_REG = SC_MODE_FLASH_RW_LITE;
}

void sc_flash_erase_chip() {
	bool buf = REG_IME;
	REG_IME = 0;
	SC_FLASH_MAGIC_ADDR_1 = SC_FLASH_MAGIC_1;
	SC_FLASH_MAGIC_ADDR_2 = SC_FLASH_MAGIC_2;
	SC_FLASH_MAGIC_ADDR_1 = SC_FLASH_ERASE;
	SC_FLASH_MAGIC_ADDR_1 = SC_FLASH_MAGIC_1;
	SC_FLASH_MAGIC_ADDR_2 = SC_FLASH_MAGIC_2;
	SC_FLASH_MAGIC_ADDR_1 = SC_FLASH_ERASE_CHIP;
	
	while (*GBA_BUS != *GBA_BUS);
	*GBA_BUS = SC_FLASH_IDLE;
	REG_IME = buf;
}

void sclite_flash_erase_chip() {
	SCLITE_FLASH_MAGIC_ADDR_1 = SC_FLASH_MAGIC_1;
	SCLITE_FLASH_MAGIC_ADDR_2 = SC_FLASH_MAGIC_2;
	SCLITE_FLASH_MAGIC_ADDR_1 = SC_FLASH_ERASE;
	SCLITE_FLASH_MAGIC_ADDR_1 = SC_FLASH_MAGIC_1;
	SCLITE_FLASH_MAGIC_ADDR_2 = SC_FLASH_MAGIC_2;
	SCLITE_FLASH_MAGIC_ADDR_1 = SC_FLASH_ERASE_CHIP;
	
	while (*GBA_BUS != *GBA_BUS);
	*GBA_BUS = SC_FLASH_IDLE;
}

/*void sc_flash_erase_block(vu16 *addr) {
	bool buf = REG_IME;
	REG_IME = 0;
	SC_FLASH_MAGIC_ADDR_1 = SC_FLASH_MAGIC_1;
	SC_FLASH_MAGIC_ADDR_2 = SC_FLASH_MAGIC_2;
	SC_FLASH_MAGIC_ADDR_1 = SC_FLASH_ERASE;
	SC_FLASH_MAGIC_ADDR_1 = SC_FLASH_MAGIC_1;
	SC_FLASH_MAGIC_ADDR_2 = SC_FLASH_MAGIC_2;
	*addr = SC_FLASH_ERASE_BLOCK;
	
	// while (*GBA_BUS != *GBA_BUS)swiWaitForVBlank();
	while (*GBA_BUS != *GBA_BUS);
	*GBA_BUS = SC_FLASH_IDLE;
	REG_IME = buf;
}

void sclite_flash_erase_block(vu16 *addr) {
	SCLITE_FLASH_MAGIC_ADDR_1 = SC_FLASH_MAGIC_1;
	SCLITE_FLASH_MAGIC_ADDR_2 = SC_FLASH_MAGIC_2;
	SCLITE_FLASH_MAGIC_ADDR_1 = SC_FLASH_ERASE;
	SCLITE_FLASH_MAGIC_ADDR_1 = SC_FLASH_MAGIC_1;
	SCLITE_FLASH_MAGIC_ADDR_2 = SC_FLASH_MAGIC_2;
	*addr = SC_FLASH_ERASE_BLOCK;
	
	while (*GBA_BUS != *GBA_BUS);
	*GBA_BUS = SC_FLASH_IDLE;
}*/

void sc_flash_program(vu16 *addr, u16 val) {
	bool buf = REG_IME;
	REG_IME = 0;
	SC_FLASH_MAGIC_ADDR_1 = SC_FLASH_MAGIC_1;
	SC_FLASH_MAGIC_ADDR_2 = SC_FLASH_MAGIC_2;
	SC_FLASH_MAGIC_ADDR_1 = SC_FLASH_PROGRAM;
	*addr = val;
	while (*GBA_BUS != *GBA_BUS);
	*GBA_BUS = SC_FLASH_IDLE;
	REG_IME = buf;
}

void sclite_flash_program(vu16 *addr, u16 val) {
	SCLITE_FLASH_MAGIC_ADDR_1 = SC_FLASH_MAGIC_1;
	SCLITE_FLASH_MAGIC_ADDR_2 = SC_FLASH_MAGIC_2;
	SCLITE_FLASH_MAGIC_ADDR_1 = SC_FLASH_PROGRAM;
	*addr = val;
	while (*GBA_BUS != *GBA_BUS);
	*GBA_BUS = SC_FLASH_IDLE;
}


bool DoFlash() {
	sc_flash_rw_enable();
	printf("\n      Death 2 supercard :3\n");
	printf("      Erasing whole chip\n");
	sc_flash_erase_chip();
	printf("      Erased whole chip\n");
	for (int i = 0; i < 60; i++)swiWaitForVBlank();
	for (u32 off = 0; off < firmSize; off += 2) {
		u16 val = 0;
		val |= scfw_buffer[off];
		val |= (scfw_buffer[off+1] << 8);
		sc_flash_program((vu16*)(FlashBase+off), val);
		if (!UpdateProgressText && !(off & 0x00ff)) {
			textBuffer = "\n\n\n\n\n\n\n\n\n\n\n      Programmed ";
			statData = (FlashBase+off);
			UpdateProgressText = true;
		}
	}
	while(UpdateProgressText)swiWaitForVBlank();
	printf("\n\n\n\n\n      Ded!\n");
	return false;
}

bool DoFlash_Lite() {
	sc_flash_rw_enable_lite();
	sc_flash_rw_enable_lite();
	printf("\n    Death 2 supercard lite :3\n");
	printf("      Erasing whole chip\n");
	sclite_flash_erase_chip();
	/*for (u32 addr = 0; addr < 0x80000; addr += 0x2000) {
		if (!UpdateProgressText) {
			textBuffer = "\n\n\n\n\n\n\n\n\n\n\n        Erased ";
			statData = (FlashBase+addr);
			UpdateProgressText = true;
		}
		sclite_flash_erase_block((vu16*)(FlashBase+addr));
	}*/
	printf("      Erased whole chip\n");
	for (int i = 0; i < 60; i++)swiWaitForVBlank();
	for (u32 off = 0; off < firmSize; off += 2) {
		u16 val = 0;
		val |= scfw_buffer[off];
		val |= (scfw_buffer[off+1] << 8);
		sclite_flash_program((vu16*)(FlashBase+off), val);
		if (!UpdateProgressText && !(off & 0x00ff)) {
			textBuffer = "\n\n\n\n\n\n\n\n\n\n\n      Programmed ";
			statData = (FlashBase+off);
			UpdateProgressText = true;
		}
	}
	while(UpdateProgressText)swiWaitForVBlank();
	printf("\n\n\n\n\n      Ded!\n");
	return false;
}

void CustomConsoleInit() {
	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);
	vramSetBankA (VRAM_A_MAIN_BG);
	vramSetBankC (VRAM_C_SUB_BG);
	
	bg = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 1, 0);
	bgSub = bgInitSub(3, BgType_Bmp8, BgSize_B8_256x256, 1, 0);
		
	consoleInit(&btConsole, 3, BgType_Text4bpp, BgSize_T_256x256, 20, 0, false, false);
	consoleInit(&tpConsole, 3, BgType_Text4bpp, BgSize_T_256x256, 20, 0, true, false);
		
	ConsoleFont font;
	font.gfx = (u16*)fontTiles;
	font.pal = (u16*)fontPal;
	font.numChars = 95;
	font.numColors =  fontPalLen / 2;
	font.bpp = 4;
	font.asciiOffset = 32;
	font.convertSingleColor = true;
	consoleSetFont(&btConsole, &font);
	consoleSetFont(&tpConsole, &font);

	consoleSelect(&tpConsole);
}

bool Prompt() {
	while(1) {
		swiWaitForVBlank();
		scanKeys();
		if ((keysDown() & KEY_UP) || (keysDown() & KEY_DOWN) || (keysDown() & KEY_LEFT) || (keysDown() & KEY_RIGHT)) {
			if (SCLiteMode) { SCLiteMode = false; } else { SCLiteMode = true; }
			consoleSelect(&tpConsole);
			if (SCLiteMode) {
				textBuffer = "\n\n         [SCLITE MODE]\n\n\n\n\n\n\n\n\n        Flash ID ";
			} else {
				textBuffer = "\n\n\n\n\n\n\n\n\n\n\n        Flash ID ";
			}
			statData = cachedFlashID;
			UpdateProgressText = true;
			while(UpdateProgressText)swiWaitForVBlank();
			statData = 0;
			consoleSelect(&btConsole);
		} else {
			switch (keysDown()) {
				case KEY_A: return true;
				case KEY_B: return false;
			}
		}
	}
}


void vBlankHandler (void) {
	if (UpdateProgressText) {
		if (!ClearOnUpdate) { ClearOnUpdate = true; } else { consoleClear(); }
		printf(textBuffer);
		if (!PrintWithStat) { 
			PrintWithStat = true; 
		} else { 
			if (FileSuccess && (currentConsole != &btConsole)) {
				iprintf("%lx \n\n\n\n\n\n\n\n\n     [FOUND FIRMWARE.FRM]", statData); 
			} else {
				iprintf("%lx \n", statData);
			}
		}
		UpdateProgressText = false;
	}
}

int main(void) {
	defaultExceptionHandler();
	CustomConsoleInit();
	irqSet(IRQ_VBLANK, vBlankHandler);
	sysSetCartOwner(true);
	fifoWaitValue32(FIFO_USER_02);
	if (isDSiMode() || fifoCheckValue32(FIFO_USER_01)) {
		textBuffer = "\n\n\n\n\n\n\n\n\n\n   Trying to kill on DSi/3DS?\n\n       Have you gone mad?";
		PrintWithStat = false;
		UpdateProgressText = true;
		while(UpdateProgressText)swiWaitForVBlank();
		consoleSelect(&btConsole);
		printf("\n Press [A] or [B] to exit.\n");
		while(1) {
			swiWaitForVBlank();
			scanKeys();
			switch (keysDown()) {
				case KEY_A: return 0;
				case KEY_B: return 0;
			}
		}
		return 0;
	}
	cachedFlashID = sc_flash_id();
	textBuffer = "\n\n\n\n\n\n\n\n\n\n\n        Flash ID ";
	statData = cachedFlashID;
	UpdateProgressText = true;
	while(UpdateProgressText)swiWaitForVBlank();
	statData = 0;
	
	firmSize = MaxFirmSize;
	scfw_buffer = (u8*)malloc(firmSize);
	toncset(scfw_buffer, 0xFF, firmSize);
	
	if (fatInitDefault()) {
		FILE *src = NULL;
		if (access("/firmware.frm", F_OK) == 0) {
			src = fopen("/firmware.frm", "rb");
		} else if (access("/scfw/firmware.frm", F_OK) == 0) {
			src = fopen("/scfw/firmware.frm", "rb");
		}
		if (src) {
			fseek(src, 0, SEEK_END);
			firmSize = ftell(src);
			fseek(src, 0, SEEK_SET);
			if (firmSize <= MaxFirmSize) {
				printf("\n\n\n\n\n\n\n\n     [FOUND FIRMWARE.FRM]");
				consoleSelect(&btConsole);
				printf("\n Reading FIRMWARE.FRM\n\n Please Wait...");
				fread((u8*)scfw_buffer, 1, firmSize, src);
				FileSuccess = true;
				fclose(src);
				consoleClear();
			} else {
				FileSuccess = false;
			}
		} else {
			FileSuccess = false;
		}
	} else {
		FileSuccess = false;
	}
	
	if (!FileSuccess) {
		consoleSelect(&btConsole);
		toncset(scfw_buffer, 0xFF, MaxFirmSize);
		tonccpy(scfw_buffer, scfw_bin, (scfw_binEnd - scfw_bin));
		firmSize = (scfw_binEnd - scfw_bin);
	}
	
	printf("\n Press [A] to kill supercard.\n");
	printf(" Press [B] to spare supercard.\n");
	
	if (!Prompt())return 0;
	
	consoleClear();
	
	if (SCLiteMode) { DoFlash_Lite(); } else { DoFlash(); }	
	 
	while(1) {
		swiWaitForVBlank();
		scanKeys();
		switch (keysDown()) {
			default: swiWaitForVBlank(); break;
			case KEY_A: return 0;
			case KEY_B: return 0;
			case KEY_START: return 0;
		}
	}
	return 0;
}

