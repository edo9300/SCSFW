#include "new_scsdio.h"
#ifdef NDS
#include <nds.h>
#else
#include <gba.h>
#endif

static bool isSDHC;

uint8_t _SD_CRC7(uint8_t* pBuf, int len);
bool SCSD_readData(void* buffer);

// uint64_t sdio_crc16_4bit_checksum(uint32_t* data, uint32_t num_words);
inline uint16_t setFastCNT(uint16_t originData) {
	/*  2-3   32-pin GBA Slot ROM 1st Access Time (0-3 = 10, 8, 6, 18 cycles)
		4     32-pin GBA Slot ROM 2nd Access Time (0-1 = 6, 4 cycles)*/
	const uint16_t mask = ~(7 << 2);//~ 000011100, clear bit 2-3 + 4
	const uint16_t setVal = ((2) << 2) | (1 << 4);
	return (originData & mask) | setVal;
}
vu16* const EXMEMCNT_ADDR = (vu16*)0x4000204;

void get_resp_drop(int dropBytes = 6);

void ReadSector(uint8_t* buff, uint32_t sector, uint32_t readnum) {
	u16 originMemStat = *EXMEMCNT_ADDR;
	*EXMEMCNT_ADDR = setFastCNT(originMemStat);
	auto param = isSDHC ? sector : (sector << 9);
	if(readnum == 1) {
		SDCommand(17, param);
		SCSD_readData(buff);
	} else {
		SDCommand(0x12, param); // R0 = 0x12, R1 = 0, R2 as calculated above

		for(auto buffer_end = buff + readnum * (512); buff < buffer_end; buff += 512) {
			SCSD_readData(buff); // Add R6, left shifted by 9, to R4 before casting
		}
		SDCommand(0xC, 0); // Command to presumably stop reading
		get_resp_drop();           // Get response from SD card
	}

	send_clk(0x10);       // Send clock signal
	*EXMEMCNT_ADDR = originMemStat;
}

#define BUSY_WAIT_TIMEOUT 500000
#define SCSD_STS_BUSY 0x100
void sc_mode(u16 mode) {
	u32 ime = REG_IME;
	REG_IME = 0;
	*(vu16*)0x9FFFFFE = 0xA55A;
	*(vu16*)0x9FFFFFE = 0xA55A;
	*(vu16*)0x9FFFFFE = mode;
	*(vu16*)0x9FFFFFE = mode;
	REG_IME = ime;
}

inline void sc_sdcard_reset(void) {
	vu16* reset_addr = (vu16*)sd_reset;
	*reset_addr = 0xFFFF;
}
inline void sc_sdcard_enable_lite(void)//what was that?
{
	vu16* reset_addr = (vu16*)sd_reset;
	*reset_addr = 0;
}

void send_clk(u32 num) {
	vu16* cmd_addr = (vu16*)sd_comadd;
	while(num--) {
		*cmd_addr;
	}
}

#define REG_SCSD_CMD (*(vu16 *)(0x09800000))
void SDCommand(u8 command, u32 argument) {
	u8 databuff[6];
	u8* tempDataPtr = databuff;

	*tempDataPtr++ = command | 0x40;
	*tempDataPtr++ = argument >> 24;
	*tempDataPtr++ = argument >> 16;
	*tempDataPtr++ = argument >> 8;
	*tempDataPtr++ = argument;
	*tempDataPtr = _SD_CRC7(databuff, 5);

	while(((REG_SCSD_CMD & 0x01) == 0)) {}

	REG_SCSD_CMD;

	tempDataPtr = databuff;
	volatile uint32_t* send_command_addr = (volatile uint32_t*)(0x09800000); // 假设sd_comadd也是数据写入地址

#define SEND_ONE_COMMAND_BYTE \
        "ldrb r0, [%0], #1 \n"         /* uint32_t data = *tempDataPtr++; */ \
        "orr  r0, r0, r0, lsl #17 \n"  /* data = data | (data << 17); */ \
        "mov  r1, r0, lsl #2 \n"       \
        "mov  r2, r0, lsl #4 \n"       \
        "mov  r3, r0, lsl #6 \n"       \
        "stmia %1, {r0-r3}\n"
	asm volatile(//发送6次
				 "1:\n"
				 SEND_ONE_COMMAND_BYTE
				 SEND_ONE_COMMAND_BYTE
				 "cmp %0, %2\n"
				 "blt 1b"
				 : // 没有输出
	: "r"((u32)databuff), "r"((u32)send_command_addr), "r"(((u32)databuff) + 6)
		: "r0", "r1", "r2", "r3"// 破坏列表
		);
	// int length = 6;
	// while (length--)
	// {
	//     uint32_t data = *tempDataPtr++;
	//     data = data | (data << 17);
	//     *send_command_addr = data;
	//     *send_command_addr = data << 2;
	//     *send_command_addr = data << 4;
	//     *send_command_addr = data << 6;
	//     // sd_dataadd[0] ~ [3]至少目前证明都是镜像的，可以随便用，可以用stmia来加速
	//     // 本质上是将U16的写合并成U32的写
	// }
}
void inline get_resp_drop(int byteNum) {
	byteNum++; // +  8 clocks

	// Wait for the card to be non-busy
	vu16* const cmd_addr_u16 = (vu16*)(0x09800000);
	vu32* const cmd_addr_u32 = (vu32*)(0x09800000);
	while((((*cmd_addr_u16) & 0x01) != 0));

	// 实际上，当跳出这个循环的时候，已经读了一个bit了，后续会多读一个bit，但是这是抛弃的rsp,因此多读一个bit也就是多一个时钟周期罢了

	asm volatile(
		"2: \n\t"
		"ldmia %0, {r0-r3} \n\t"
		"subs %1, %1, #1 \n\t"   // byteNum--
		"bne 2b \n\t"            // 如果byteNum不为0，则继续循环
		: // 没有输出
	: "r" (cmd_addr_u32), "r" (byteNum) // 输入
		: "r0", "r1", "r2", "r3", "cc" // 破坏列表
		);
	// while (byteNum--)
	// {
	//     *cmd_addr_u32;
	//     *cmd_addr_u32;
	//     *cmd_addr_u32;
	//     *cmd_addr_u32;
	// }
}

static const uint8_t crc7_lut[] =
    {0x0, 0x12, 0x24, 0x36, 0x48, 0x5A, 0x6C, 0x7E, 0x90, 0x82, 0xB4, 0xA6, 0xD8, 0xCA, 0xFC, 0xEE,
     0x32, 0x20, 0x16, 0x4, 0x7A, 0x68, 0x5E, 0x4C, 0xA2, 0xB0, 0x86, 0x94, 0xEA, 0xF8, 0xCE, 0xDC,
     0x64, 0x76, 0x40, 0x52, 0x2C, 0x3E, 0x8, 0x1A, 0xF4, 0xE6, 0xD0, 0xC2, 0xBC, 0xAE, 0x98, 0x8A,
     0x56, 0x44, 0x72, 0x60, 0x1E, 0xC, 0x3A, 0x28, 0xC6, 0xD4, 0xE2, 0xF0, 0x8E, 0x9C, 0xAA, 0xB8,
     0xC8, 0xDA, 0xEC, 0xFE, 0x80, 0x92, 0xA4, 0xB6, 0x58, 0x4A, 0x7C, 0x6E, 0x10, 0x2, 0x34, 0x26,
     0xFA, 0xE8, 0xDE, 0xCC, 0xB2, 0xA0, 0x96, 0x84, 0x6A, 0x78, 0x4E, 0x5C, 0x22, 0x30, 0x6, 0x14,
     0xAC, 0xBE, 0x88, 0x9A, 0xE4, 0xF6, 0xC0, 0xD2, 0x3C, 0x2E, 0x18, 0xA, 0x74, 0x66, 0x50, 0x42,
     0x9E, 0x8C, 0xBA, 0xA8, 0xD6, 0xC4, 0xF2, 0xE0, 0xE, 0x1C, 0x2A, 0x38, 0x46, 0x54, 0x62, 0x70,
     0x82, 0x90, 0xA6, 0xB4, 0xCA, 0xD8, 0xEE, 0xFC, 0x12, 0x0, 0x36, 0x24, 0x5A, 0x48, 0x7E, 0x6C,
     0xB0, 0xA2, 0x94, 0x86, 0xF8, 0xEA, 0xDC, 0xCE, 0x20, 0x32, 0x4, 0x16, 0x68, 0x7A, 0x4C, 0x5E,
     0xE6, 0xF4, 0xC2, 0xD0, 0xAE, 0xBC, 0x8A, 0x98, 0x76, 0x64, 0x52, 0x40, 0x3E, 0x2C, 0x1A, 0x8,
     0xD4, 0xC6, 0xF0, 0xE2, 0x9C, 0x8E, 0xB8, 0xAA, 0x44, 0x56, 0x60, 0x72, 0xC, 0x1E, 0x28, 0x3A,
     0x4A, 0x58, 0x6E, 0x7C, 0x2, 0x10, 0x26, 0x34, 0xDA, 0xC8, 0xFE, 0xEC, 0x92, 0x80, 0xB6, 0xA4,
     0x78, 0x6A, 0x5C, 0x4E, 0x30, 0x22, 0x14, 0x6, 0xE8, 0xFA, 0xCC, 0xDE, 0xA0, 0xB2, 0x84, 0x96,
     0x2E, 0x3C, 0xA, 0x18, 0x66, 0x74, 0x42, 0x50, 0xBE, 0xAC, 0x9A, 0x88, 0xF6, 0xE4, 0xD2, 0xC0,
     0x1C, 0xE, 0x38, 0x2A, 0x54, 0x46, 0x70, 0x62, 0x8C, 0x9E, 0xA8, 0xBA, 0xC4, 0xD6, 0xE0, 0xF2
};

inline uint8_t CRC7_one(uint8_t crcIn, uint8_t data) {
	crcIn ^= data;
	return crc7_lut[crcIn];
}
inline uint8_t _SD_CRC7(uint8_t* pBuf, int len) {
	uint8_t crc = 0;
	while(len--)
		crc = CRC7_one(crc, *pBuf++);
	crc |= 1;
	return crc;
}

vu16* const REG_SCSD_DATAREAD_ADDR = ((vu16*)(0x09100000));
vu32* const REG_SCSD_DATAREAD_32_ADDR = ((vu32*)(0x09100000));
bool SCSD_readData(void* buffer) {
	u8* buff_u8 = (u8*)buffer;
	int i;

	i = BUSY_WAIT_TIMEOUT;
	while(((*REG_SCSD_DATAREAD_ADDR) & SCSD_STS_BUSY));
	if(i == 0) {
		return false;
	}

	const u32 maskHi = 0xFFFF0000;
#define LOAD_U32_ALIGNED_2WORDS \
            "ldmia  %2, {r0-r7} \n"   /*从REG_SCSD_DATAREAD_32_ADDR读取8个32位值到r0-r7*/  \
            "and    r3, r3, %3 \n"     /*r3 &= maskHi*/                                     \
            "and    r7, r7, %3 \n"     \
            "orr    r3, r3, r1, lsr #16 \n"     /*r3 |= (r1 >> 16)*/                        \
            "orr    r7, r7, r5, lsr #16 \n"     \
            "stmia  %0!, {r3,r7} \n"  /*将r3和r7的值存储到buff_u32指向的位置，并将buff_u32增加8字节*/

#define LOAD_U32_ALIGNED_1WORDS \
            "ldmia  %2, {r0-r3} \n"   /*从REG_SCSD_DATAREAD_32_ADDR读取8个32位值到r0-r7*/  \
            "and    r3, r3, %3 \n"     /*r3 &= maskHi*/                                     \
            "orr    r3, r3, r1, lsr #16 \n"     /*r3 |= (r1 >> 16)*/                        \
            "str    r3, [%0], #4 \n"  /*将r3和r7的值存储到buff_u32指向的位置，并将buff_u32增加8字节*/

#define LOAD_U32_ALIGNED_U16 \
            "ldmia  %2, {r0-r1} \n"   /*从REG_SCSD_DATAREAD_32_ADDR读取8个32位值到r0-r7*/  \
            "lsr r1, r1, #16\n"     \
            "strh r1, [%0], #2\n"       //u16


	if(((u32)buff_u8 & 0x03) == 0) {//u32 aligned
		asm volatile(
			"1: \n"
			LOAD_U32_ALIGNED_2WORDS
			LOAD_U32_ALIGNED_2WORDS
			"cmp    %0, %1 \n"
			"blt    1b \n"              // if buffer<bufferEnd continue;
			:
		: "r" (buffer), "r" ((u32)buffer + 512), "r" (REG_SCSD_DATAREAD_32_ADDR), "r" (maskHi)
			: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "memory", "cc"
			);
		// i=256;
		// u16* buff = (u16*)buffer;
		// while(i--) {

		// 	*(REG_SCSD_DATAREAD_32_ADDR);
		// 	*buff++ = (*(REG_SCSD_DATAREAD_32_ADDR)) >> 16; 
		// }
	} else if((((u32)buff_u8 & 0x01) == 0)) {//u16 aligned
		asm volatile(
			LOAD_U32_ALIGNED_U16
			LOAD_U32_ALIGNED_1WORDS
			LOAD_U32_ALIGNED_2WORDS
			"2: \n"
			LOAD_U32_ALIGNED_2WORDS
			LOAD_U32_ALIGNED_2WORDS
			"cmp    %0, %1 \n"
			"blt    2b \n"              // if buffer<bufferEnd continue;
			LOAD_U32_ALIGNED_U16
			:
		: "r" (buffer), "r" ((u32)buffer + 510/*512-2*/), "r" (REG_SCSD_DATAREAD_32_ADDR), "r" (maskHi)
			: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "memory", "cc"
			);
	} else {
		u32 temp;
		i = 256;
		while(i--) {
			*REG_SCSD_DATAREAD_32_ADDR;
			temp = (*REG_SCSD_DATAREAD_32_ADDR) >> 16;
			*buff_u8++ = (u8)temp;
			*buff_u8++ = (u8)(temp >> 8);
		}
	}

	asm volatile(
		"ldmia  %0, {r0-r7} \n"   // drop the crc16
		"ldrh   r1, [%0] \n"   // end
		:
	: "r" ((u32)REG_SCSD_DATAREAD_32_ADDR)
		: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7"
		);
	// for (i = 0; i < 8; i++) {
	// 	*REG_SCSD_DATAREAD_32_ADDR;
	// }
	// *REG_SCSD_DATAREAD_ADDR;

	return true;
}

bool MemoryCard_IsInserted(void) {
	uint16_t status = *(vu16*)sd_comadd; // 读取状态寄存器的值
	return (status & 0x300) == 0;
}

uint32_t loadBigEndU32_u8(u8*& dataBuf) {
	u32 data;
	data = (*dataBuf++) << 24;
	data |= (*dataBuf++) << 16;
	data |= (*dataBuf++) << 8;
	data |= (*dataBuf++);
	return data;
}

bool get_resp(u8* dest, u32 length) {
	u32 i;
	int numBits = length * 8;

	i = BUSY_WAIT_TIMEOUT;
	while(((REG_SCSD_CMD & 0x01) != 0) && (--i));
	if(i == 0) {
		return false;
	}

	// The first bit is always 0
	vu16* const cmd_addr_u16 = (vu16*)(0x09800000);
	vu32* const cmd_addr_u32 = (vu32*)(0x09800000);

	u32 partial_result = ((*cmd_addr_u16) & 0x01) << 16;
	numBits -= 2;
	// Read the remaining bits in the response.
	// It's always most significant bit first
	const u32 mask_2bit = 0x10001;
	while(numBits) {
		numBits -= 2;
		partial_result = (partial_result << 2) | ((*cmd_addr_u32) & mask_2bit);
		if((numBits & 0x7) == 0) {
			//_1_3_5_7 _0_2_4_6 
			*dest++ = ((partial_result >> 16) | (partial_result << 1));
			partial_result = 0;
		}
	}
	for(i = 0; i < 4; i++) {//8clock
		*cmd_addr_u32;
	}
	return true;
}
#define NUM_STARTUP_CLOCKS 30000
#define GO_IDLE_STATE 0
#define CMD8 8
#define APP_CMD 55
#define CMD58 58
#define SD_APP_OP_COND 41
#define MAX_STARTUP_TRIES 5000
#define SD_OCR_VALUE 0x00030000
//2.8V to 3.0V
#define ALL_SEND_CID 2
#define SEND_RELATIVE_ADDR 3
#define SD_STATE_STBY 3
#define SEND_CSD 9
#define SELECT_CARD 7
#define SET_BUS_WIDTH 6
#define SET_BLOCKLEN 16
#define RESPONSE_TIMEOUT 256
#define SEND_STATUS 13
#define SD_STATE_TRAN 4
#define READY_FOR_DATA 1
u32 relativeCardAddress = 0;
bool cmd_and_response(u32 respLenth, u8* responseBuffer, u8 command, u32 data) {
	SDCommand(command, data);
	return get_resp(responseBuffer, respLenth);
}
void cmd_and_response_drop(u32 respLenth, u8* responseBuffer, u8 command, u32 data) {
	SDCommand(command, data);
	get_resp_drop(respLenth);
}


bool init_sd() {
	sc_sdcard_reset();//do we really need that?
	send_clk(NUM_STARTUP_CLOCKS);
	SDCommand(GO_IDLE_STATE, 0);
	send_clk(NUM_STARTUP_CLOCKS);
	int i;

	u8 responseBuffer[17] = { 0 };
	isSDHC = false;
	bool cmd8Response = cmd_and_response(17, responseBuffer, CMD8, 0x1AA);
	if(cmd8Response && responseBuffer[0] == CMD8 && responseBuffer[1] == 0 && responseBuffer[2] == 0 && responseBuffer[3] == 0x1 && responseBuffer[4] == 0xAA) {
		isSDHC = true;//might be
	}
	for(i = 0; i < MAX_STARTUP_TRIES; i++) {
		cmd_and_response(6, responseBuffer, APP_CMD, 0);
		if(responseBuffer[0] != APP_CMD) {
			return false;
		}

		// u32 arg = SD_OCR_VALUE;
		u32 arg = 0;
		arg |= (1 << 28); //Max performance
		arg |= (1 << 20); //3.3v
		if(isSDHC)
			arg |= (1 << 30); // Set HCS bit,Supports SDHC

		if(cmd_and_response(6, responseBuffer, SD_APP_OP_COND, arg) &&//ACMD41
		   ((responseBuffer[1] & (1 << 7)) != 0)/*Busy:0b:initing 1b:init completed*/) {
			bool CCS = responseBuffer[1] & (1 << 6);//0b:SDSC  1b:SDHC/SDXC
			if(!CCS && isSDHC)
				isSDHC = false;
			break; // Card is ready
		}
		send_clk(NUM_STARTUP_CLOCKS);
	}

	if(i >= MAX_STARTUP_TRIES) {
		return false;
	}

	// The card's name, as assigned by the manufacturer
	cmd_and_response_drop(17, responseBuffer, ALL_SEND_CID, 0);
	// Get a new address
	for(i = 0; i < MAX_STARTUP_TRIES; i++) {
		cmd_and_response(6, responseBuffer, SEND_RELATIVE_ADDR, 0);
		relativeCardAddress = (responseBuffer[1] << 24) | (responseBuffer[2] << 16);
		if((responseBuffer[3] & 0x1e) != (SD_STATE_STBY << 1)) {
			break;
		}
	}
	if(i >= MAX_STARTUP_TRIES) {
		return false;
	}

	// Some cards won't go to higher speeds unless they think you checked their capabilities
	cmd_and_response_drop(17, responseBuffer, SEND_CSD, relativeCardAddress);

	// Only this card should respond to all future commands
	cmd_and_response_drop(6, responseBuffer, SELECT_CARD, relativeCardAddress);

	// Set a 4 bit data bus
	cmd_and_response_drop(6, responseBuffer, APP_CMD, relativeCardAddress);
	cmd_and_response_drop(6, responseBuffer, SET_BUS_WIDTH, 2); // 4-bit mode.

	return true;
}