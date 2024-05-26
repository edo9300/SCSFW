#include <nds.h>
#include <fat.h>

#include <stdio.h>

#include "font.h"
#include "tonccpy.h"

#define SC_MODE_REG 			*(vu16*)0x09FFFFFE
#define SC_MODE_MAGIC			(u16)0xA55A
#define SC_MODE_FLASH_RW		(u16)0x4
#define SC_MODE_FLASH_RW_LITE	(u16)0x1510


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

extern u8 scfw_bin[];
extern u8 scfw_binEnd[];

static u8* scfw_buffer;

static PrintConsole tpConsole;
static PrintConsole btConsole;

static int bg;
static int bgSub;

const char* textBuffer = "X------------------------------X\nX------------------------------X";

volatile u32 cachedFlashID;
volatile u32 statData = 0x00000000;
volatile bool UpdateProgressText = false;
volatile bool PrintWithStat = true;
volatile bool ClearOnUpdate = true;
volatile bool SCLiteMode = false;

void vblankHandler (void) {
	if (UpdateProgressText) {
		if (!ClearOnUpdate) { ClearOnUpdate = true; } else { consoleClear(); }
		printf(textBuffer);
		if (!PrintWithStat) { PrintWithStat = true; } else { iprintf("%lx \n", statData); }
		UpdateProgressText = false;
	}
}

void Block_Erase(u32 blockAdd) {
	vu16 v1,v2;  
	u32 Address;
	u32 loop;
	u32 off = 0;
	
	Address = blockAdd;
	*((vu16 *)(FlashBase+0x555*2)) = 0xF0;
	*((vu16 *)(FlashBase+0x1555*2)) = 0xF0;
		
	if((blockAdd == 0) || (blockAdd == 0x1FC0000) || (blockAdd == 0xFC0000) || (blockAdd == 0x1000000)) {
		for(loop = 0; loop < 0x40000; loop += 0x8000) {
			*((vu16 *)(FlashBase+off+0x555*2)) = 0xAA;
			*((vu16 *)(FlashBase+off+0x2AA*2)) = 0x55;
			*((vu16 *)(FlashBase+off+0x555*2)) = 0x80;
			*((vu16 *)(FlashBase+off+0x555*2)) = 0xAA;
			*((vu16 *)(FlashBase+off+0x2AA*2)) = 0x55;
			*((vu16 *)(FlashBase+Address+loop)) = 0x30;
			
			*((vu16 *)(FlashBase+off+0x1555*2)) = 0xAA;
			*((vu16 *)(FlashBase+off+0x12AA*2)) = 0x55;
			*((vu16 *)(FlashBase+off+0x1555*2)) = 0x80;
			*((vu16 *)(FlashBase+off+0x1555*2)) = 0xAA;
			*((vu16 *)(FlashBase+off+0x12AA*2)) = 0x55;
			*((vu16 *)(FlashBase+Address+loop+0x2000)) = 0x30;
			
			*((vu16 *)(FlashBase+off+0x2555*2)) = 0xAA;
			*((vu16 *)(FlashBase+off+0x22AA*2)) = 0x55;
			*((vu16 *)(FlashBase+off+0x2555*2)) = 0x80;
			*((vu16 *)(FlashBase+off+0x2555*2)) = 0xAA;
			*((vu16 *)(FlashBase+off+0x22AA*2)) = 0x55;
			*((vu16 *)(FlashBase+Address+loop+0x4000)) = 0x30;
			
			*((vu16 *)(FlashBase+off+0x3555*2)) = 0xAA;
			*((vu16 *)(FlashBase+off+0x32AA*2)) = 0x55; 
			*((vu16 *)(FlashBase+off+0x3555*2)) = 0x80;
			*((vu16 *)(FlashBase+off+0x3555*2)) = 0xAA;
			*((vu16 *)(FlashBase+off+0x32AA*2)) = 0x55;
			*((vu16 *)(FlashBase+Address+loop+0x6000)) = 0x30;
			do {
				v1 = *((vu16 *)(FlashBase+Address+loop));
				v2 = *((vu16 *)(FlashBase+Address+loop));
			} while(v1!=v2);
			do {
				v1 = *((vu16 *)(FlashBase+Address+loop+0x2000));
				v2 = *((vu16 *)(FlashBase+Address+loop+0x2000));
			} while(v1!=v2);
			do {
				v1 = *((vu16 *)(FlashBase+Address+loop+0x4000));
				v2 = *((vu16 *)(FlashBase+Address+loop+0x4000));
			} while(v1!=v2);
			do {
				v1 = *((vu16 *)(FlashBase+Address+loop+0x6000));
				v2 = *((vu16 *)(FlashBase+Address+loop+0x6000));
			} while(v1!=v2);
		}	
	} else {
		*((vu16 *)(FlashBase+off+0x555*2)) = 0xAA;
		*((vu16 *)(FlashBase+off+0x2AA*2)) = 0x55;
		*((vu16 *)(FlashBase+off+0x555*2)) = 0x80;
		*((vu16 *)(FlashBase+off+0x555*2)) = 0xAA;
		*((vu16 *)(FlashBase+off+0x2AA*2)) = 0x55;
		*((vu16 *)(FlashBase+Address)) = 0x30;
		
		*((vu16 *)(FlashBase+off+0x1555*2)) = 0xAA;
		*((vu16 *)(FlashBase+off+0x12AA*2)) = 0x55;
		*((vu16 *)(FlashBase+off+0x1555*2)) = 0x80;
		*((vu16 *)(FlashBase+off+0x1555*2)) = 0xAA;
		*((vu16 *)(FlashBase+off+0x12AA*2)) = 0x55;
		*((vu16 *)(FlashBase+Address+0x2000)) = 0x30;
		
		do {
			v1 = *((vu16 *)(FlashBase+Address));
			v2 = *((vu16 *)(FlashBase+Address));
		} while(v1!=v2);
		do {
			v1 = *((vu16 *)(FlashBase+Address+0x2000));
			v2 = *((vu16 *)(FlashBase+Address+0x2000));
		} while(v1!=v2);
		
		*((vu16 *)(FlashBase+off+0x555*2)) = 0xAA;
		*((vu16 *)(FlashBase+off+0x2AA*2)) = 0x55;
		*((vu16 *)(FlashBase+off+0x555*2)) = 0x80;
		*((vu16 *)(FlashBase+off+0x555*2)) = 0xAA;
		*((vu16 *)(FlashBase+off+0x2AA*2)) = 0x55;
		*((vu16 *)(FlashBase+Address+0x20000)) = 0x30;
		
		*((vu16 *)(FlashBase+off+0x1555*2)) = 0xAA;
		*((vu16 *)(FlashBase+off+0x12AA*2)) = 0x55;
		*((vu16 *)(FlashBase+off+0x1555*2)) = 0x80;
		*((vu16 *)(FlashBase+off+0x1555*2)) = 0xAA;
		*((vu16 *)(FlashBase+off+0x12AA*2)) = 0x55;
		*((vu16 *)(FlashBase+Address+0x2000+0x20000)) = 0x30;
	
		do {
			v1 = *((vu16 *)(FlashBase+Address+0x20000));
			v2 = *((vu16 *)(FlashBase+Address+0x20000));
		} while(v1!=v2);
		do {
			v1 = *((vu16 *)(FlashBase+Address+0x2000+0x20000));
			v2 = *((vu16 *)(FlashBase+Address+0x2000+0x20000));
		} while(v1!=v2);	
	}
}

// Modified version of WriteNorFlash from EZFlash 3in1 card lib
void WriteNorFlash_SCLite(u32 address, u8 *buffer, u32 size) {
	vu16 v1,v2;
	u32 loopwrite;
	vu16* buf = (vu16*)buffer;
	u32 mapaddress;
	v1 = 0;
	v2 = 1;
	u32 off = 0; // Original offset code for alternate 3in1 revisions removed. They are not used for SC Lite.
	
	mapaddress = address;
	
	for(loopwrite = 0; loopwrite < (size >> 2); loopwrite++) {
		*((vu16*)(FlashBase+off+0x555*2)) = 0xAA;
		*((vu16*)(FlashBase+off+0x2AA*2)) = 0x55;
		*((vu16*)(FlashBase+off+0x555*2)) = 0xA0;
		*((vu16*)(FlashBase+mapaddress+loopwrite*2)) = buf[loopwrite];
		*((vu16*)(FlashBase+off+0x1555*2)) = 0xAA;
		*((vu16*)(FlashBase+off+0x12AA*2)) = 0x55;
		*((vu16*)(FlashBase+off+0x1555*2)) = 0xA0;			
		*((vu16*)(FlashBase+mapaddress+0x2000+loopwrite*2)) = buf[0x1000+loopwrite];
		do {
			v1 = *((vu16*)(FlashBase+mapaddress+loopwrite*2));
			v2 = *((vu16*)(FlashBase+mapaddress+loopwrite*2));
		} while(v1 != v2);
		do {
			v1 = *((vu16*)(FlashBase+mapaddress+0x2000+loopwrite*2));
			v2 = *((vu16*)(FlashBase+mapaddress+0x2000+loopwrite*2));
		} while(v1 != v2);
		if (!UpdateProgressText) {
			textBuffer = "\n\n\n\n\n\n\n\n\n\n\n      Programmed ";
			statData = (0x08000000 + loopwrite);
			UpdateProgressText = true;
		}
	}
}

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
	bool buf = REG_IME;
	REG_IME = 0;
	SC_MODE_REG = SC_MODE_MAGIC;
	SC_MODE_REG = SC_MODE_MAGIC;
	SC_MODE_REG = SC_MODE_FLASH_RW_LITE;
	SC_MODE_REG = SC_MODE_FLASH_RW_LITE;
	REG_IME = buf;
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
	
	while (*GBA_BUS != *GBA_BUS)swiWaitForVBlank();
	*GBA_BUS = SC_FLASH_IDLE;
	REG_IME = buf;
}

void sc_flash_erase_block(vu16 *addr) {
	bool buf = REG_IME;
	REG_IME = 0;
	SC_FLASH_MAGIC_ADDR_1 = SC_FLASH_MAGIC_1;
	SC_FLASH_MAGIC_ADDR_2 = SC_FLASH_MAGIC_2;
	SC_FLASH_MAGIC_ADDR_1 = SC_FLASH_ERASE;
	SC_FLASH_MAGIC_ADDR_1 = SC_FLASH_MAGIC_1;
	SC_FLASH_MAGIC_ADDR_2 = SC_FLASH_MAGIC_2;
	*addr = SC_FLASH_ERASE_BLOCK;
	
	while (*GBA_BUS != *GBA_BUS)swiWaitForVBlank();
	*GBA_BUS = SC_FLASH_IDLE;
	REG_IME = buf;
}

void sc_flash_program(vu16 *addr, u16 val) {
	bool buf = REG_IME;
	REG_IME = 0;
	SC_FLASH_MAGIC_ADDR_1 = SC_FLASH_MAGIC_1;
	SC_FLASH_MAGIC_ADDR_2 = SC_FLASH_MAGIC_2;
	SC_FLASH_MAGIC_ADDR_1 = SC_FLASH_PROGRAM;
	*addr = val;
	
	while (*GBA_BUS != *GBA_BUS)swiWaitForVBlank();
	
	*GBA_BUS = SC_FLASH_IDLE;
	REG_IME = buf;
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

bool DoFlash() {
	sc_flash_rw_enable();
	printf("\n      Death 2 supercard :3\n");
	printf("      Erasing whole chip\n");
	sc_flash_erase_chip();
	printf("      Erased whole chip\n");
	for (int i = 0; i < 60; i++)swiWaitForVBlank();
	for (u32 off = 0; off < (u32)(scfw_binEnd - scfw_bin); off += 2) {
		u16 val = 0;
		val |= scfw_bin[off];
		val |= (scfw_bin[off+1] << 8);
		sc_flash_program((vu16*)(0x08000000 + off), val);
		if (!UpdateProgressText && !(off & 0x00ff)) {
			textBuffer = "\n\n\n\n\n\n\n\n\n\n\n      Programmed ";
			statData = (0x08000000 + off);
			UpdateProgressText = true;
		}
	}
	while(UpdateProgressText)swiWaitForVBlank();
	printf("\n\n\n\n\n      Ded!\n");
	return false;
}

bool DoFlash_Lite() {
	sc_flash_rw_enable_lite();
	for(u32 offset = 0; offset < 0x80000; offset += 0x40000) {
		if (!UpdateProgressText) {
			textBuffer = "\n    Death 2 supercard lite :3\n\n\n      Erasing whole chip\n\n\n        Erased ";
			statData = (0x08000000 + offset);
			UpdateProgressText = true;
		}
		Block_Erase(offset);
	}
	printf("\n\n\n      Erased whole chip\n");
	
	for (int i = 0; i < 60; i++)swiWaitForVBlank();
		
	WriteNorFlash_SCLite(0, scfw_buffer, 0x80000);
	
	while(UpdateProgressText)swiWaitForVBlank();
	
	for (int i = 0; i < 60; i++)swiWaitForVBlank();
	
	printf("\n\n\n\n\n      Ded!\n");
	
	return true;
}

int main(void) {
	CustomConsoleInit();
	irqSet(IRQ_VBLANK, vblankHandler);
	sysSetCartOwner(true);
	cachedFlashID = sc_flash_id();
	textBuffer = "\n\n\n\n\n\n\n\n\n\n\n        Flash ID ";
	statData = cachedFlashID;
	UpdateProgressText = true;
	while(UpdateProgressText)swiWaitForVBlank();
	statData = 0;
	consoleSelect(&btConsole);
	
	scfw_buffer = (u8*)malloc(0x80000);
	toncset(scfw_buffer, 0xFF, 0x80000);
	tonccpy(scfw_buffer, scfw_bin, (scfw_binEnd - scfw_bin));
	
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

