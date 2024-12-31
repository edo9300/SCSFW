#include "sc_commands.h"

void sc_change_mode(uint16_t mode) {
	auto* const SC_MODE_REG = (volatile uint16_t*)0x09FFFFFE;
	const uint16_t SC_MODE_MAGIC			= 0xA55A;
	*SC_MODE_REG = SC_MODE_MAGIC;
	*SC_MODE_REG = SC_MODE_MAGIC;
	*SC_MODE_REG = mode;
	*SC_MODE_REG = mode;
}

void sc_flash_rw_enable(SUPERCARD_TYPE supercardType) {
	constexpr uint16_t SC_MODE_FLASH_RW		= 0x0004;
	constexpr uint16_t SC_MODE_FLASH_RW_LITE	= 0x1510;
	sc_change_mode((supercardType & SC_LITE) ? SC_MODE_FLASH_RW_LITE : SC_MODE_FLASH_RW);
}

SUPERCARD_TYPE detect_supercard_type() {
	auto type = []{
		constexpr uint16_t SC_MODE_SDCARD = 0x0002;
		sc_change_mode(SC_MODE_SDCARD);
		auto val = *(volatile uint16_t*)0x09800000;
		switch(val & 0xe300) {
			case 0xa000:
				return SUPERCARD_TYPE::SC_LITE;
			case 0xc000:
				return SUPERCARD_TYPE::SC_RUMBLE;
			case 0xe000:
				return SUPERCARD_TYPE::SC_SD;
			default: {
				auto* cfstatns = (volatile unsigned short*)0x99c0000;
				*cfstatns = 0x50;
				// originally this was a single nop for timing purposes very likely
				// but the code was designed for the arm7 of the gba, let's pump it up
				// a bit
				__asm__ volatile (
					"nop\n"
					"nop\n"
					"nop\n"
					"nop\n"
					"nop\n"
					"nop\n"
					"nop\n"
					"nop\n"
				);
				if(*cfstatns == 0x50)
					return SUPERCARD_TYPE::SC_CF;
				return SUPERCARD_TYPE::UNK;
			}
		}
	}();
	sc_flash_rw_enable(type);
	return type;
}