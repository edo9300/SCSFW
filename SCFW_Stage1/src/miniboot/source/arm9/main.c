// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2024 Adrian "asie" Siekierka

#include "common.h"
#include "bios.h"
#include "dka.h"
#include "bootstub.h"
#include "dldi_patch.h"
#include "console.h"
#include "scsd/sc_commands.h"

// #define DEBUG

#define DLDI_BACKUP   ((DLDI_INTERFACE*) 0x6820000)
#define checkErrorFatFs(...) do {(void)0; } while(0)

/* === Main logic === */

#define IPC_ARM7_NONE  0x000
#define IPC_ARM7_COPY  0x100
#define IPC_ARM7_RESET 0x200
#define IPC_ARM7_SYNC  0xF00

void ipc_arm7_cmd(uint32_t cmd) {
    uint32_t next_sync = REG_IPCSYNC & 0xF;
    uint32_t last_sync = next_sync;
    REG_IPCSYNC = cmd;
    while (last_sync == next_sync) next_sync = REG_IPCSYNC & 0xF;
}

const char *executable_path = "/BOOT.NDS";

#define ARM7_OWNS_ROM  (1 << 7)
#define GBA_BUS       ((volatile unsigned short*)(0x08000000))
#define GBA_BUS_U8    ((volatile unsigned char*)(0x08000000))

static inline
/*! \fn void sysSetCartOwner(bool arm9)
    \brief Sets the owner of the GBA cart.  Both CPUs cannot have access to the gba cart (slot 2).
    \param arm9 if true the arm9 is the owner, otherwise the arm7
*/
void sysSetCartOwner(bool arm9) {
  REG_EXMEMCNT = (REG_EXMEMCNT & ~ARM7_OWNS_ROM) | (arm9 ? 0 :  ARM7_OWNS_ROM);
}

#define SCSFW_MAGIC 0x57464353

typedef struct SCSFW_PARAMETERS {
	unsigned int scsfw_magic;
	unsigned int miniboot_arm7;
	unsigned int miniboot_arm7_size;
	unsigned int miniboot_arm9;
	unsigned int miniboot_arm9_size;
	unsigned int nds_rom;
	unsigned int nds_rom_size;
	unsigned int sc_lite_dldi;
	unsigned int sc_lite_dldi_size;
	unsigned int scsd_dldi;
	unsigned int scsd_dldi_size;
	unsigned int sccf_dldi;
	unsigned int sccf_dldi_size;
} SCSFW_PARAMETERS;

SCSFW_PARAMETERS parameters;

void readNds(void* dest, unsigned int offset, unsigned int size) {
	__aeabi_memcpy(dest, (void*)&GBA_BUS_U8[parameters.nds_rom + offset], size);
}

void findSCSFWParameters(SCSFW_PARAMETERS* params) {
	__aeabi_memcpy4(params, (void*)&GBA_BUS_U8[0xc0 + 4], sizeof(SCSFW_PARAMETERS));
	if(params->scsfw_magic == 0x57464353) {
		dprintf("sclite magic found\n");
		return;
	}
	// supercard rumble
	__aeabi_memcpy4(params, (void*)&GBA_BUS_U8[0xc0 + 4 + 0x40000], sizeof(SCSFW_PARAMETERS));
	// account for the values being offsetted
	params->miniboot_arm7 += 0x40000;
	params->miniboot_arm9 += 0x40000;
	params->nds_rom += 0x40000;
	params->sc_lite_dldi += 0x40000;
	params->scsd_dldi += 0x40000;
	params->sccf_dldi += 0x40000;
}

#define SECURE_AREA_FROM_FW (*((volatile uint32_t*)0x2000000))
#define SECURE_AREA_FROM_FW_2 (*((volatile uint32_t*)0x2004000))
#define CHIPID (*((volatile uint32_t*)0x027FF800))

typedef struct SUPERCARD_RAM_DATA {
	uint8_t magicString[8];
	uint8_t header[0x200];
	uint8_t secure_area[0x4000];
	uint32_t chipid;
} SUPERCARD_RAM_DATA;

static void backupDecryptedHeaderAndSecureAreaAndHeaderToSCRam(void) {
	uint8_t magicString[8] = {'S', 'C', 'S', 'F', 'W', 0, 0, 0};
	uint8_t magicString2[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	volatile uint32_t* secure_area_ptr = 0;
	uint8_t* magicStringPtr = magicString;
	SUPERCARD_RAM_DATA* ram_data = ((SUPERCARD_RAM_DATA*)GBA_BUS);
	
	if(SECURE_AREA_FROM_FW != 0) {
		secure_area_ptr = &SECURE_AREA_FROM_FW;
	} else if(SECURE_AREA_FROM_FW_2 != 0) {
		secure_area_ptr = &SECURE_AREA_FROM_FW_2;
	}
	if(secure_area_ptr == 0) {
		// this juggling is needed because otherwise either the supercard itself
		// or gcc simply made a single byte write not work
		dprintf("No secure area found\n");
		secure_area_ptr = &SECURE_AREA_FROM_FW;
		magicStringPtr = magicString2;
	} else {
		dprintf("Fround secure area at address: 0x%X\n", (intptr_t)secure_area_ptr);
		char str_buff[13];
		__aeabi_memcpy(str_buff, NDS_HEADER->game_title, 12);
		str_buff[13] = 0;
		dprintf("Loaded game is: %s\n", str_buff);
		__aeabi_memcpy(str_buff, NDS_HEADER->game_code, 4);
		str_buff[5] = 0;
		dprintf("Loaded game code is: %s\n", str_buff);
	}
	sc_change_mode(0x5);
	__aeabi_memcpy(ram_data->magicString, magicStringPtr, 8);
	__aeabi_memcpy4(ram_data->header, NDS_HEADER, 0x200);
	__aeabi_memcpy4(ram_data->secure_area, secure_area_ptr, 0x4000);
	ram_data->chipid = CHIPID;
	sc_change_mode(en_write);
}

int main(void) {
    int result;

    // Initialize VRAM (128KB to main engine, rest to CPU, 32KB WRAM to ARM7).
    REG_VRAMCNT_ABCD = VRAMCNT_ABCD(0x81, 0x80, 0x82, 0x8A);
    REG_VRAMCNT_EFGW = VRAMCNT_EFGW(0x80, 0x80, 0x80, 0x03);
    REG_VRAMCNT_HI = VRAMCNT_HI(0x80, 0x80);

    REG_POWCNT = POWCNT_LCD | POWCNT_2D_MAIN | POWCNT_DISPLAY_SWAP;
    // Ensure ARM9 has control over the cartridge slots.
    REG_EXMEMCNT = 0x6000; // ARM9 memory priority, ARM9 slot access, "slow" GBA timings

    // Reset display.
    displayReset();

    // If holding START while booting, or DEBUG is defined, enable 
    // debug output.
#ifndef NDEBUG
#ifdef DEBUG
    debugEnabled = true;
#else
    debugEnabled = ((~REG_KEYINPUT) & (KEY_L | KEY_R)) == (KEY_L | KEY_R);
#endif
#endif
	sysSetCartOwner(true);
	findSCSFWParameters(&parameters);
    dprintf("parameters.miniboot_arm9_size: 0x%X\n", parameters.miniboot_arm9_size);
    dprintf("parameters.miniboot_arm9: 0x%X\n", parameters.miniboot_arm9);
    dprintf("parameters.miniboot_arm7_size: 0x%X\n", parameters.miniboot_arm7_size);
    dprintf("parameters.miniboot_arm7: 0x%X\n", parameters.miniboot_arm7);
    dprintf("parameters.nds_rom_size: 0x%X\n", parameters.nds_rom_size);
    dprintf("parameters.nds_rom: 0x%X\n", parameters.nds_rom);
    dprintf("parameters.scsd_dldi_size: 0x%X\n", parameters.scsd_dldi_size);
    dprintf("parameters.scsd_dldi: 0x%X\n", parameters.scsd_dldi);
    dprintf("parameters.sc_lite_dldi_size: 0x%X\n", parameters.sc_lite_dldi_size);
    dprintf("parameters.sc_lite_dldi: 0x%X\n", parameters.sc_lite_dldi);
    dprintf("parameters.sccf_dldi_size: 0x%X\n", parameters.sccf_dldi_size);
    dprintf("parameters.sccf_dldi: 0x%X\n", parameters.sccf_dldi);
    dprintf("ARM7 sync");
    for (int i = 1; i <= 16; i++) {
        dprintf(".");
        ipc_arm7_cmd((i << 8) & 0xF00);
    }
    dprintf(" OK\n");

	backupDecryptedHeaderAndSecureAreaAndHeaderToSCRam();

    // Create a bootstub in memory, if one doesn't already exist.
    if (DKA_BOOTSTUB->magic != DKA_BOOTSTUB_MAGIC) {
        uint8_t *bootstub_loc = ((uint8_t*) DKA_BOOTSTUB) + sizeof(dka_bootstub_t);
        uint8_t *arm9_bin_loc = bootstub_loc + bootstub_size;
        uint8_t *arm7_bin_loc = arm9_bin_loc + parameters.miniboot_arm9_size;
        
        bootstub.arm9_target_entry = arm9_bin_loc;
        bootstub.arm7_target_entry = arm7_bin_loc;

        __aeabi_memcpy(bootstub_loc, &bootstub, bootstub_size);
        __aeabi_memcpy(arm9_bin_loc, (void*)&GBA_BUS_U8[parameters.miniboot_arm9], parameters.miniboot_arm9_size);
        __aeabi_memcpy(arm7_bin_loc, (void*)&GBA_BUS_U8[parameters.miniboot_arm7], parameters.miniboot_arm7_size);

        DKA_BOOTSTUB->magic = DKA_BOOTSTUB_MAGIC;
        DKA_BOOTSTUB->arm9_entry = bootstub_loc;
        DKA_BOOTSTUB->arm7_entry = bootstub_loc + 4;
        DKA_BOOTSTUB->loader_size = 0;
    }

    // Create a copy of the DLDI driver in VRAM before initializing it.
    // We'll make use of this copy for patching the ARM9 binary later.
    dprintf("reading dldi... ");
	SUPERCARD_TYPE type = detect_supercard_type();
	if(type & SC_LITE) {
		dprintf("for sclite...\n");
		__aeabi_memcpy4(DLDI_BACKUP, (void*)&GBA_BUS_U8[parameters.sc_lite_dldi], parameters.sc_lite_dldi_size);
	} else if (type == SC_CF) {
		dprintf("for sccf...\n");
		__aeabi_memcpy4(DLDI_BACKUP, (void*)&GBA_BUS_U8[parameters.sccf_dldi], parameters.sccf_dldi_size);
	} else {
		dprintf("for scsd...\n");
		__aeabi_memcpy4(DLDI_BACKUP, (void*)&GBA_BUS_U8[parameters.scsd_dldi], parameters.scsd_dldi_size);
	}

    // Mount the filesystem. Try to open BOOT.NDS.
    dprintf("Mounting FAT filesystem... ");
    checkErrorFatFs("Could not mount FAT filesystem", f_mount(&fs, "", 1));
    dprintf("OK\n");
    checkErrorFatFs("Could not find BOOT.NDS", f_open(&fp, executable_path, FA_READ));
    dprintf("BOOT.NDS found.\n"	);

    // Read the .nds file header.
    checkErrorFatFs("Could not read BOOT.NDS", f_read(&fp, NDS_HEADER, sizeof(nds_header_t), &bytes_read));
	readNds(NDS_HEADER, 0, sizeof(nds_header_t));

    bool waiting_arm7 = false;
    // Load the ARM7 binary.
    {
        dprintf("ARM7: %d bytes @ %X\n", NDS_HEADER->arm7_size, NDS_HEADER->arm7_start);
        bool in_main_ram = IN_RANGE_EX(NDS_HEADER->arm7_start, 0x2000000, 0x23BFE00);
        bool in_arm7_ram = IN_RANGE_EX(NDS_HEADER->arm7_start, 0x37F8000, 0x380FE00);
        if (!NDS_HEADER->arm7_size
            || !IN_RANGE_EX(NDS_HEADER->arm7_entry - NDS_HEADER->arm7_start, 0, NDS_HEADER->arm7_size)
            || (!in_main_ram && !in_arm7_ram)
            || (in_main_ram && !IN_RANGE_EX(NDS_HEADER->arm7_start + NDS_HEADER->arm7_size, 0x2000001, 0x23BFE01))
            || (in_arm7_ram && !IN_RANGE_EX(NDS_HEADER->arm7_start + NDS_HEADER->arm7_size, 0x37F8001, 0x380FE01))) {
            eprintf("Invalid ARM7 binary location."); while(1);
        }

        checkErrorFatFs("Could not read BOOT.NDS", f_lseek(&fp, NDS_HEADER->arm7_offset));
        checkErrorFatFs("Could not read BOOT.NDS", f_read(&fp, (void*) (in_arm7_ram ? 0x2000000 : NDS_HEADER->arm7_start), NDS_HEADER->arm7_size, &bytes_read));
		readNds((void*) (in_arm7_ram ? 0x2000000 : NDS_HEADER->arm7_start), NDS_HEADER->arm7_offset, NDS_HEADER->arm7_size);

        // If the ARM7 binary has to be relocated to ARM7 RAM, the ARM7 CPU
        // has to relocate it from main memory.
        if (in_arm7_ram) {
            REG_IPCFIFOSEND = NDS_HEADER->arm7_size;
            REG_IPCFIFOSEND = 0x2000000;
            REG_IPCFIFOSEND = NDS_HEADER->arm7_start;
            ipc_arm7_cmd(IPC_ARM7_COPY);
            waiting_arm7 = true;
        }
    }

    // Load the ARM9 binary.
    {
        dprintf("ARM9: %d bytes @ %X\n", NDS_HEADER->arm9_size, NDS_HEADER->arm9_start);
        bool in_main_ram = IN_RANGE_EX(NDS_HEADER->arm9_start, 0x2000000, 0x23BFE00);
        if (!NDS_HEADER->arm9_size
            || !IN_RANGE_EX(NDS_HEADER->arm9_entry - NDS_HEADER->arm9_start, 0, NDS_HEADER->arm9_size)
            || !in_main_ram
            || !IN_RANGE_EX(NDS_HEADER->arm9_start + NDS_HEADER->arm9_size, 0x2000001, 0x23BFE01)) {
            eprintf("Invalid ARM9 binary location."); while(1);
        }

        checkErrorFatFs("Could not read BOOT.NDS", f_lseek(&fp, NDS_HEADER->arm9_offset));
        if (waiting_arm7) {
            ipc_arm7_cmd(IPC_ARM7_NONE);
            waiting_arm7 = false;
        }
        checkErrorFatFs("Could not read BOOT.NDS", f_read(&fp, (void*) NDS_HEADER->arm9_start, NDS_HEADER->arm9_size, &bytes_read));
		readNds((void*)NDS_HEADER->arm9_start, NDS_HEADER->arm9_offset, NDS_HEADER->arm9_size);

        // Try to apply the DLDI driver patch.
        result = dldi_patch_relocate((void*) NDS_HEADER->arm9_start, NDS_HEADER->arm9_size, DLDI_BACKUP);
        if (result) {
            eprintf("Failed to apply DLDI patch.\n");
            switch (result) {
                case DLPR_NOT_ENOUGH_SPACE: eprintf("Not enough space."); break;
            }
            while(1);
        }
    }

    // Set up argv.
    DKA_ARGV->cmdline = (char*) 0x2FFFEB0;
    DKA_ARGV->cmdline_size = strlen(executable_path) + 1;
    __aeabi_memcpy(DKA_ARGV->cmdline, executable_path, DKA_ARGV->cmdline_size);
    DKA_ARGV->magic = DKA_ARGV_MAGIC;

    dprintf("Launching");

    // If debug enabled, wait for user to stop holding START
    if (debugEnabled) while (((~REG_KEYINPUT) & (KEY_L | KEY_R)) == (KEY_L | KEY_R));

    // Restore/clear system state.
    displayReset();
    REG_EXMEMCNT = 0xE880;

    // Start the ARM7 binary.
    ipc_arm7_cmd(IPC_ARM7_RESET);

    // Start the ARM9 binary.
    swiSoftReset();
}
