#pragma once
#include <stdint.h>

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

void sc_send_command(SC_FLASH_COMMAND command);
bool try_guess_lite(void);

#ifdef __cplusplus
}
#endif