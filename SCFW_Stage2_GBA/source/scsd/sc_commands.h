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

typedef enum SUPERCARD_TYPE {
    SC_SD = 0x00,
    SC_LITE = 0x01,
    SC_CF = 0x02,
    SC_RUMBLE = (0x10 | SC_LITE),
    UNK = ~SC_RUMBLE,
} SUPERCARD_TYPE;

SUPERCARD_TYPE detect_supercard_type(void);

#ifdef __cplusplus
}
#endif