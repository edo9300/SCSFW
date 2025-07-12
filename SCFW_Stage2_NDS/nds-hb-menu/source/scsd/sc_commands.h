#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define en_firmware 0
#define en_sdram 1
#define en_sdcard 2
#define en_write 4

void sc_change_mode(uint16_t mode);

typedef enum SC_FLASH_COMMAND {
	ERASE		= 0x80,
	ERASE_BLOCK	= 0x30,
	ERASE_CHIP	= 0x10,
	PROGRAM		= 0xA0,
	IDENTIFY	= 0x90,
} SC_FLASH_COMMAND;

typedef enum SUPERCARD_TYPE {
    SC_SD = 0x00,
    SC_LITE = 0x01,
    SC_CF = 0x02,
    SC_RUMBLE = (0x10 | SC_LITE),
    UNK = (uint8_t)~SC_RUMBLE
} SUPERCARD_TYPE;

void sc_flash_rw_enable(SUPERCARD_TYPE supercardType);
void sc_flash_erase_sector(volatile uint16_t* addr, SUPERCARD_TYPE supercardType);
void sc_flash_write(const uint16_t* src, volatile uint16_t* dest, size_t size, SUPERCARD_TYPE supercardType);

SUPERCARD_TYPE get_supercard_type(void);

#ifdef __cplusplus
}
#endif