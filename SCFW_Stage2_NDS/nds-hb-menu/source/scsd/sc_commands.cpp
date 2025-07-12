#include <utility>
#include <cstdio>
#include "sc_commands.h"

constexpr uint16_t SC_FLASH_IDLE				= 0x00F0;
constexpr uint16_t SC_MODE_SDCARD				= 0x0002;
constexpr uint16_t SC_MODE_FLASH_RW				= 0x0004;
constexpr uint16_t SC_MODE_FLASH_RW_LITE_RUMBLE	= 0x1510;
constexpr uint16_t SC_FLASH_MAGIC_1 			= 0x00AA;
constexpr uint16_t SC_FLASH_MAGIC_2 			= 0x0055;

#define GBA_BUS       ((volatile uint16_t*)(0x08000000))

static std::pair<volatile uint16_t*, volatile uint16_t*> get_magic_addrs(SUPERCARD_TYPE scType) {
	auto* const SCLITE_FLASH_MAGIC_ADDR_1 = (volatile uint16_t*)0x08000AAA;
	auto* const SCLITE_FLASH_MAGIC_ADDR_2 = (volatile uint16_t*)0x08000554;
	auto* const SC_FLASH_MAGIC_ADDR_1	 = (volatile uint16_t*)0x08000b92;
	auto* const SC_FLASH_MAGIC_ADDR_2	 = (volatile uint16_t*)0x0800046c;
	if(scType & SUPERCARD_TYPE::SC_LITE)
		return { SCLITE_FLASH_MAGIC_ADDR_1, SCLITE_FLASH_MAGIC_ADDR_2 };
	return { SC_FLASH_MAGIC_ADDR_1, SC_FLASH_MAGIC_ADDR_2 };
}

void sc_change_mode(uint16_t mode) {
	auto* const SC_MODE_REG = (volatile uint16_t*)0x09FFFFFE;
	const uint16_t SC_MODE_MAGIC			= 0xA55A;
	*SC_MODE_REG = SC_MODE_MAGIC;
	*SC_MODE_REG = SC_MODE_MAGIC;
	*SC_MODE_REG = mode;
	*SC_MODE_REG = mode;
}

void sc_flash_rw_enable(SUPERCARD_TYPE supercardType) {
	sc_change_mode((supercardType & SUPERCARD_TYPE::SC_LITE) ? SC_MODE_FLASH_RW_LITE_RUMBLE : SC_MODE_FLASH_RW);
}

void send_command(SC_FLASH_COMMAND command, SUPERCARD_TYPE supercardType) {
	auto [magic_addr_1, magic_addr_2] = get_magic_addrs(supercardType);
	*magic_addr_1 = SC_FLASH_MAGIC_1;
	*magic_addr_2 = SC_FLASH_MAGIC_2;
	*magic_addr_1 = static_cast<uint16_t>(command);
}

void sc_flash_erase_sector(volatile uint16_t* addr, SUPERCARD_TYPE supercardType)
{
	auto [magic_addr_1, magic_addr_2] = get_magic_addrs(supercardType);
	
	auto* base_addr = (volatile uint16_t*)((intptr_t)addr & ~0xFFFF);
	for(auto* addr = base_addr; addr != (base_addr + (0xE000 / 2)); addr += 0x1000) {
		send_command(SC_FLASH_COMMAND::ERASE, supercardType);

		*magic_addr_1 = SC_FLASH_MAGIC_1;
		*magic_addr_2 = SC_FLASH_MAGIC_2;
		*addr = (uint16_t)SC_FLASH_COMMAND::ERASE_BLOCK;

		for(int i = 0; i < 0x3D0900 && *addr != 0xFFFF; ++i) {}
		if(*addr != 0xFFFF) {
			iprintf("ersase failed, 0x%X\n", *addr);
		}
		*GBA_BUS = SC_FLASH_IDLE;
	}
}

void sc_flash_program(volatile uint16_t* addr, uint16_t val, SUPERCARD_TYPE supercardType) {
	send_command(SC_FLASH_COMMAND::PROGRAM, supercardType);
	*addr = val;
	while (*GBA_BUS != *GBA_BUS);
	*GBA_BUS = SC_FLASH_IDLE;
}

void sc_flash_write(const uint16_t* src, volatile uint16_t* dest, size_t size, SUPERCARD_TYPE supercardType) {
	for(size_t i = 0; i < (size / 2); ++i) {
		sc_flash_program(dest, *src, supercardType);
		if(*dest != *src) {
			printf("Write failed, expected: 0x%X, got 0x%X\n", *src, *dest);
		}
		++dest;
		++src;
	}
}

SUPERCARD_TYPE get_supercard_type() {
	static auto type = []{
		sc_change_mode(SC_MODE_SDCARD);
		auto type = [] {
				switch((*(volatile uint16_t*)0x09800000) & 0xe300) {
				case 0xa000:
					return SUPERCARD_TYPE::SC_LITE;
				case 0xc000:
					return SUPERCARD_TYPE::SC_RUMBLE;
				case 0xe000:
					return SUPERCARD_TYPE::SC_SD;
				default:
					return SUPERCARD_TYPE::SC_CF;
			}
		}();
		sc_flash_rw_enable(type);
		return type;
	}();
	return type;
}
