#include "my_io_scsd.h"
// #include <cstdio>
/*
	iointerface.c template
	
 Copyright (c) 2006 Michael "Chishm" Chisholm
	
 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

  1. All derivative works must be clearly marked as such. Derivatives of this file 
	 must have the author of the derivative indicated within the source.  
  2. The name of the author may not be used to endorse or promote products derived
     from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


// When compiling for NDS, make sure NDS is defined
#ifndef NDS
 #if defined ARM9 || defined ARM7
  #define NDS
 #endif
#endif

#ifdef NDS
 #include <nds/ndstypes.h>
#else
 #include "gba_types.h"
#endif

#ifndef NULL
 #define NULL 0
#endif

#include "new_scsdio.h"

const char build_info[]=
"SuperCard-SDHC-DLDI Ver 1.0 The 'Moon Eclipse' "
"[https://github.com/ArcheyChen/SuperCard-SDHC-DLDI]"
"Author: Ausar Date:2024/03/03\n"
"Supports:1.SD init 2.SDHC 3.unaligned r/w 4.compatiable with nds-bootstrap"
"Please Keep this info";
const char* get_build_info(){
    return build_info;
}

static bool startup(void) {
    sc_mode(en_sdram + en_sdcard);
    return MemoryCard_IsInserted() && init_sd();
}

static bool isInserted (void) {
    return MemoryCard_IsInserted();
}

static bool clearStatus (void) {
    return true;
}
static bool readSectors (u32 sector, u32 numSectors, void* buffer) {
    sc_mode(en_sdram + en_sdcard);
    ReadSector(buffer,sector,numSectors);
    return true;
}


static bool writeSectors (u32 sector, u32 numSectors, void* buffer) {
    sc_mode(en_sdram + en_sdcard);
    WriteSector(buffer,sector,numSectors);
    return true;
}
static bool shutdown(void) {
    return true;
}

const DISC_INTERFACE _my_io_scsd = {
	DEVICE_TYPE_SCSD,
	FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE | FEATURE_SLOT_GBA,
	(FN_MEDIUM_STARTUP)&startup,
	(FN_MEDIUM_ISINSERTED)&isInserted,
	(FN_MEDIUM_READSECTORS)&readSectors,
	(FN_MEDIUM_WRITESECTORS)&writeSectors,
	(FN_MEDIUM_CLEARSTATUS)&clearStatus,
	(FN_MEDIUM_SHUTDOWN)&shutdown
} ;