#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#ifdef NDS
 #include <nds/ndstypes.h>
#else
 #include "gba_types.h"
#endif

#define sd_comadd 0x9800000
#define sd_dataadd 0x9000000  
#define sd_dataradd 0x9100000
#define sd_reset 0x9440000

#define en_fireware 0
#define en_sdram 1
#define en_sdcard 2
#define en_write 4
#define en_rumble 8
#define en_rumble_user_flash 1
// extern void sc_InitSCMode (void);
bool MemoryCard_IsInserted (void);
// extern void sc_sdcard_reset(void);

extern void sd_crc16_s(u16* buff,u16 num,u16* crc16buff);

extern void sd_data_read_s(u16 *buff);


bool init_sd();
void send_clk(u32 num);
void sc_mode(u16 mode);
void sc_sdcard_reset(void);
void SDCommand(u8 command,u32 sector);
void sd_data_write(u16 *buff,u8* crc16buff);
void WriteSector(u8 *buff,u32 sector,u32 writenum);
void ReadSector(u8 *buff,u32 sector,u32 readnum);

#ifdef __cplusplus
}
#endif