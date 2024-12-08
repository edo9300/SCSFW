#include <utility>
#include "sc_commands.h"

#define SC_FLASH_IDLE			((uint16_t) 0xF0)

static std::pair<volatile uint16_t*, volatile uint16_t*> get_magic_addrs(bool lite) {
	auto* const SCLITE_FLASH_MAGIC_ADDR_1 = (volatile uint16_t*)0x08000AAA;
	auto* const SCLITE_FLASH_MAGIC_ADDR_2 = (volatile uint16_t*)0x08000554;
	auto* const SC_FLASH_MAGIC_ADDR_1	 = (volatile uint16_t*)0x08000b92;
	auto* const SC_FLASH_MAGIC_ADDR_2	 = (volatile uint16_t*)0x0800046c;
	if(lite)
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

void sc_flash_rw_enable(bool lite) {
	constexpr uint16_t SC_MODE_FLASH_RW		= 0x0004;
	constexpr uint16_t SC_MODE_FLASH_RW_LITE	= 0x1510;
	sc_change_mode(lite ? SC_MODE_FLASH_RW_LITE : SC_MODE_FLASH_RW);
}

void send_command(SC_FLASH_COMMAND command, bool lite) {
	constexpr uint16_t SC_FLASH_MAGIC_1 = 0x00aa;
	constexpr uint16_t SC_FLASH_MAGIC_2 = 0x0055;
	auto [magic_addr_1, magic_addr_2] = get_magic_addrs(lite);
	*magic_addr_1 = SC_FLASH_MAGIC_1;
	*magic_addr_2 = SC_FLASH_MAGIC_2;
	*magic_addr_1 = static_cast<uint16_t>(command);
}

bool try_guess_lite() {
	auto get_flash_id = [](bool lite) -> uint32_t {
		static constexpr uint16_t COMMAND_ERROR = 0x002e;
		sc_flash_rw_enable(lite);
		auto [magic_addr_1, magic_addr_2] = get_magic_addrs(lite);
		send_command(SC_FLASH_COMMAND::IDENTIFY, lite);
		auto* gba_bus = ((volatile uint16_t*)(0x08000000));
		auto upper_half = *gba_bus;
		auto magic = *magic_addr_1;
		*gba_bus = SC_FLASH_IDLE;
		if(upper_half == COMMAND_ERROR)
			return 0;
		return (upper_half << 16) | magic;
	};
	auto res = get_flash_id(true) != 0;
	sc_flash_rw_enable(res);
	
	return res;
}
