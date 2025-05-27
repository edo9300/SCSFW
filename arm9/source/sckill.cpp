#include <nds.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <fat.h>
#include <stdio.h>
#include <nds/arm9/console.h>
#include <utility>

#include "font.h"
#include "tonccpy.h"
#include "fileSelector.h"

enum class SC_FLASH_COMMAND : u16 {
	ERASE		= 0x80,
	ERASE_BLOCK	= 0x30,
	ERASE_CHIP	= 0x10,
	PROGRAM		= 0xA0,
	IDENTIFY	= 0x90,
};
enum SUPERCARD_TYPE : u8 {
    SC_SD = 0x00,
    SC_LITE = 0x01,
    SC_RUMBLE = (0x10 | SC_LITE),
    UNK = uint8_t(~SC_RUMBLE)
};

#define SC_FLASH_IDLE			((u16) 0xF0)

extern u8 scfw_bin[];
extern u8 scfw_binEnd[];

u16* scfw_buffer;

static PrintConsole tpConsole;
static PrintConsole btConsole;

extern PrintConsole* currentConsole;

char *firmwareFilename = NULL;

static int bg;
static int bgSub;

const char* textBuffer = "X------------------------------X\nX------------------------------X";

u32 cachedFlashID;
u32 statData = 0x00000000;
u32 firmSize = 0x200000;
bool UpdateProgressText = false;
bool PrintWithStat = true;
bool ClearOnUpdate = true;
SUPERCARD_TYPE SuperCardType = SUPERCARD_TYPE::UNK;
bool FileSuccess = false;

constexpr u16 SC_MODE_FLASH_RW				= 0x0004;
constexpr u16 SC_MODE_SDCARD				= 0x0002;
constexpr u16 SC_MODE_FLASH_RW_LITE_RUMBLE	= 0x1510;

static void change_mode(u16 mode) {
	auto* const SC_MODE_REG = (vu16*)0x09FFFFFE;
	const u16 SC_MODE_MAGIC			= 0xA55A;
	*SC_MODE_REG = SC_MODE_MAGIC;
	*SC_MODE_REG = SC_MODE_MAGIC;
	*SC_MODE_REG = mode;
	*SC_MODE_REG = mode;
}

SUPERCARD_TYPE detect_supercard_type() {
    change_mode(SC_MODE_SDCARD);
	auto val = *(volatile uint16_t*)0x09800000;
    switch(val & 0xe300) {
        case 0xa000:
            return SUPERCARD_TYPE::SC_LITE;
        case 0xc000:
            return SUPERCARD_TYPE::SC_RUMBLE;
        case 0xe000:
            return SUPERCARD_TYPE::SC_SD;
        default:
            return SUPERCARD_TYPE::UNK;
    }
}

static std::pair<vu16*, vu16*> get_magic_addrs(SUPERCARD_TYPE scType = SuperCardType) {
	auto* const SCLITE_FLASH_MAGIC_ADDR_1 = (vu16*)0x08000AAA;
	auto* const SCLITE_FLASH_MAGIC_ADDR_2 = (vu16*)0x08000554;
	auto* const SC_FLASH_MAGIC_ADDR_1	 = (vu16*)0x08000b92;
	auto* const SC_FLASH_MAGIC_ADDR_2	 = (vu16*)0x0800046c;
	if(scType == SUPERCARD_TYPE::SC_SD)
		return { SC_FLASH_MAGIC_ADDR_1, SC_FLASH_MAGIC_ADDR_2 };
	return { SCLITE_FLASH_MAGIC_ADDR_1, SCLITE_FLASH_MAGIC_ADDR_2 };
}

static u32 get_max_firm_size(SUPERCARD_TYPE scType = SuperCardType) {
	switch(scType){
        case SUPERCARD_TYPE::SC_SD:
			return 0x80000;
        case SUPERCARD_TYPE::SC_LITE:
			return 0x7C000;
        case SUPERCARD_TYPE::SC_RUMBLE:
        case SUPERCARD_TYPE::UNK:
			return 0x200000;
		default:
			__builtin_unreachable();
	}
}

static void send_command(SC_FLASH_COMMAND command, SUPERCARD_TYPE scType = SuperCardType) {
	constexpr u16 SC_FLASH_MAGIC_1 = 0x00aa;
	constexpr u16 SC_FLASH_MAGIC_2 = 0x0055;
	auto [magic_addr_1, magic_addr_2] = get_magic_addrs(scType);
	*magic_addr_1 = SC_FLASH_MAGIC_1;
	*magic_addr_2 = SC_FLASH_MAGIC_2;
	*magic_addr_1 = static_cast<u16>(command);
}


void sc_flash_rw_enable(SUPERCARD_TYPE scType = SuperCardType) {
	change_mode((scType == SUPERCARD_TYPE::SC_SD) ? SC_MODE_FLASH_RW : SC_MODE_FLASH_RW_LITE_RUMBLE);
}

u32 get_flash_id(SUPERCARD_TYPE scType = SuperCardType) {
	static constexpr u16 COMMAND_ERROR = 0x002e;
	sc_flash_rw_enable(scType);
	auto [magic_addr_1, magic_addr_2] = get_magic_addrs(scType);
	send_command(SC_FLASH_COMMAND::IDENTIFY, scType);
	auto upper_half = *GBA_BUS;
	auto magic = *magic_addr_1;
	*GBA_BUS = SC_FLASH_IDLE;
	if(upper_half == COMMAND_ERROR)
		return 0;
	return (upper_half << 16) | magic;
}

void sc_flash_erase_chip() {
	send_command(SC_FLASH_COMMAND::ERASE);
	send_command(SC_FLASH_COMMAND::ERASE_CHIP);

	while (*GBA_BUS != *GBA_BUS);
	*GBA_BUS = SC_FLASH_IDLE;
}

bool sc_flash_erase_block_rumble(vu16 *addr) {
	vu16* addr1 = (vu16*)((0xfff8000 & ((intptr_t)addr)) + 0xaaa);
	vu16* addr2 = (vu16*)((0xfff8000 & ((intptr_t)addr)) + 0x554);


	*addr1 = 0xaa;
	*addr2 = 0x55;
	*addr1 = 0x80;

	*addr1 = 0xaa;
	*addr2 = 0x55;
	*addr = 0x30;

	for(int i = 0x3d0000; i >= 0; --i) {
		if(*addr == 0xffff)
			return true;
	}
	return false;
}

void sc_flash_erase_block(vu16 *addr, SUPERCARD_TYPE scType = SuperCardType)
{
	constexpr u16 SC_FLASH_MAGIC_1 = 0x00aa;
	constexpr u16 SC_FLASH_MAGIC_2 = 0x0055;
	auto [magic_addr_1, magic_addr_2] = get_magic_addrs(scType);
	send_command(SC_FLASH_COMMAND::ERASE);

	*magic_addr_1 = SC_FLASH_MAGIC_1;
	*magic_addr_2 = SC_FLASH_MAGIC_2;
	*addr = (u16)SC_FLASH_COMMAND::ERASE_BLOCK;

	while (*GBA_BUS != *GBA_BUS) ;
	*GBA_BUS = SC_FLASH_IDLE;
}

void sc_flash_program_rumble(vu16 *addr, u16 val)  {
	vu16* addr1 = (vu16*)((0xfff8000 & ((intptr_t)addr)) + 0xaaa);
	vu16* addr2 = (vu16*)((0xfff8000 & ((intptr_t)addr)) + 0x554);

	*addr1 = 0xaa;
	*addr2 = 0x55;
	*addr1 = 0xa0;

	*addr = val;

	for(int i = 0x100; i >= 0; --i) {
		if(*addr == val)
			break;
	}
	*GBA_BUS = SC_FLASH_IDLE;
}

void sc_flash_program(vu16 *addr, u16 val) {
	send_command(SC_FLASH_COMMAND::PROGRAM);
	*addr = val;
	while (*GBA_BUS != *GBA_BUS);
	*GBA_BUS = SC_FLASH_IDLE;
}


bool DoFlash() {
	auto* const FLASH_BASE = (vu16*)0x08000000;
	sc_flash_rw_enable();
	printf("\n      Death 2 supercard :3\n");
	printf("      Erasing whole chip\n");
	sc_flash_erase_chip();
	printf("      Erased whole chip\n");
	for (int i = 0; i < 60; i++)swiWaitForVBlank();
	auto total = (firmSize + 1) / 2; // account for odd sizes
	u32 off = 0;
	auto* flash_function = &sc_flash_program;
	if(SuperCardType == SUPERCARD_TYPE::SC_RUMBLE) { // first 0x40000 bytes of a supercard rumble are read only
		off = (0x40000 / 2);
		flash_function = &sc_flash_program_rumble;
	}
	for (; off < (total); ++off) {
		(*flash_function)(FLASH_BASE+off, scfw_buffer[off]);
		if (!UpdateProgressText && !(off & 0x007f)) {
			textBuffer = "\n\n\n\n\n\n\n\n\n\n\n      Programmed ";
			statData = (uintptr_t)(FLASH_BASE+off);
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

void printHeader() {
	consoleSelect(&tpConsole);
	switch(SuperCardType) {
		case SUPERCARD_TYPE::SC_SD:
			textBuffer = "\n\n\n\n\n\n\n\n\n\n\n        Flash ID ";
			break;
		case SUPERCARD_TYPE::SC_LITE:
			textBuffer = "\n\n         [SCLITE MODE]\n\n\n\n\n\n\n\n\n        Flash ID ";
			break;
		case SUPERCARD_TYPE::SC_RUMBLE:
			textBuffer = "\n\n         [RUMBLE MODE]\n\n\n\n\n\n\n\n\n        Flash ID ";
			break;
		case SUPERCARD_TYPE::UNK:
			textBuffer = "\n\n           [UNKNOWN]\n\n\n\n\n\n\n\n\n        Flash ID ";
			break;
	}
	statData = cachedFlashID;
	UpdateProgressText = true;
	while(UpdateProgressText)swiWaitForVBlank();
	statData = 0;
}

bool Prompt() {
	while(1) {
		swiWaitForVBlank();
		scanKeys();
		if ((keysDown() & KEY_UP) || (keysDown() & KEY_DOWN) || (keysDown() & KEY_LEFT) || (keysDown() & KEY_RIGHT)) {
			consoleSelect(&tpConsole);
			auto get_next = [](SUPERCARD_TYPE cur_mode){
				switch(cur_mode) {
					case SUPERCARD_TYPE::SC_SD:
						return SUPERCARD_TYPE::SC_LITE;
					case SUPERCARD_TYPE::SC_LITE:
						return SUPERCARD_TYPE::SC_RUMBLE;
					case SUPERCARD_TYPE::SC_RUMBLE:
						return SUPERCARD_TYPE::SC_SD;
					case SUPERCARD_TYPE::UNK:
						return SUPERCARD_TYPE::SC_SD;
					default:
						__builtin_unreachable();
				}
			};
			SuperCardType = get_next(SuperCardType);
			printHeader();
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
				iprintf("%lx \n\n\n\n\n\n\n\n\n[FOUND %s]", statData, firmwareFilename);
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
	SuperCardType = detect_supercard_type();
	cachedFlashID = get_flash_id();
	if(cachedFlashID == 0 || SuperCardType == SUPERCARD_TYPE::UNK) {
		textBuffer = "\n\n\n\n\n\n\n\n\n\nThe cart has not been recognized\n\n"
					 "If you're sure you got a\n"
					 "supercard\n"
					 "You'll have to manually\n"
					 "pick if it's a supercard lite,\n"
					 "rumble or normal";
		PrintWithStat = false;
		UpdateProgressText = true;
		while(UpdateProgressText)swiWaitForVBlank();
		consoleSelect(&btConsole);
		printf("\n Press [A] or [B] to continue.\n");
		[] {
			while(1) {
				swiWaitForVBlank();
				scanKeys();
				switch (keysDown()) {
					case KEY_A: return;
					case KEY_B: return;
				}
			}
		}();
	}
	printHeader();

	firmSize = get_max_firm_size();
	scfw_buffer = (u16*)malloc(firmSize);
	toncset(scfw_buffer, 0xFF, firmSize);

	if (fatInitDefault()) {
		FILE *src = NULL;
		consoleSelect(&btConsole);
		consoleClear();
		firmwareFilename = selectFirmware();
		consoleSelect(&tpConsole);
		if(!firmwareFilename) {
    		FileSuccess = false;
		} else {
		src = fopen(firmwareFilename, "rb");
		if (src) {
			fseek(src, 0, SEEK_END);
			firmSize = ftell(src);
			fseek(src, 0, SEEK_SET);
			if (firmSize <= get_max_firm_size()) {
				iprintf("\n\n\n\n\n\n\n\n[FOUND %s]",firmwareFilename);
				consoleSelect(&btConsole);
				iprintf("\n Reading %s\n\n Please Wait...",firmwareFilename);
				FileSuccess = fread((u8*)scfw_buffer, firmSize, 1, src) == 1;
				fclose(src);
				consoleClear();
			} else {
				FileSuccess = false;
			}
		} else {
			FileSuccess = false;
		}
		}
	} else {
		FileSuccess = false;
	}

	if (!FileSuccess) {
		printf("\n\n\n\n\n\n\n\n          [File error!]\n   [USING SCFW's firmware.frm]");
		consoleSelect(&btConsole);
		consoleClear();
		tonccpy(scfw_buffer, scfw_bin, (scfw_binEnd - scfw_bin));
		firmSize = (scfw_binEnd - scfw_bin);
	}

	printf("\n Press [A] to kill supercard.\n");
	printf(" Press [B] to spare supercard.\n");

	if (!Prompt())return 0;

	consoleClear();

	DoFlash();

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

