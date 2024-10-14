/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "../new_scsdio.h"

static bool initialized = false;
static bool drive_present = false;

DSTATUS disk_status (BYTE pdrv)
{
	if(!drive_present)
		return STA_NODISK;
	if(!initialized)
		return STA_NOINIT;
	return 0;
}

DSTATUS disk_initialize (BYTE pdrv)
{
    sc_mode(en_sdram + en_sdcard);
	if(!MemoryCard_IsInserted())
		return STA_NODISK;
	drive_present = true;
	if(!init_sd())
		return STA_NOINIT;
	initialized = true;
	return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    sc_mode(en_sdram + en_sdcard);
    ReadSector(buff,sector,count);
	return RES_OK;
}
