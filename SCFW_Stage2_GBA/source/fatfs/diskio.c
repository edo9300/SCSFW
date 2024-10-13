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
#include <stdbool.h>
#include "../my_io_scsd.h"

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
	if(!_my_io_scsd.isInserted())
		return STA_NODISK;
	drive_present = true;
	if(!_my_io_scsd.startup())
		return STA_NOINIT;
	initialized = true;
	return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
	return _my_io_scsd.readSectors(sector, count, buff) ? RES_OK : RES_ERROR;
}
