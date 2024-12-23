#include <gba.h>

#ifndef EWRAM_BSS
#include <stdint.h>
#include <string.h>
typedef enum RESTART_FLAG {
	ROM_RESTART,	/*!< Restart from RAM entry point. */
	RAM_RESTART		/*!< restart from ROM entry point */
} RESTART_FLAG;

#define EWRAM_BSS	__attribute__((section(".bss.iwram")))
void SoftReset(RESTART_FLAG RestartFlag);
#if	defined	( __thumb__ )
#define	SystemCall(Number)	 __asm ("SWI	  "#Number"\n" :::  "r0", "r1", "r2", "r3")
#else
#define	SystemCall(Number)	 __asm ("SWI	  "#Number"	<< 16\n" :::"r0", "r1", "r2", "r3")
#endif
static inline void VBlankIntrWait() { SystemCall(5); }

#define	REG_BASE	0x04000000
#define BG_COLORS		((volatile uint16_t *)0x05000000)	// Background color table
#define RGB5(r,g,b)	((r)|((g)<<5)|((b)<<10))
#define RGB8(r,g,b)	( (((b)>>3)<<10) | (((g)>>3)<<5) | ((r)>>3) )
#define	REG_DISPCNT		*((volatile uint16_t *)(REG_BASE + 0x00))
#define BIT(number) (1<<(number))
//---------------------------------------------------------------------------------
typedef enum LCDC_BITS {
//---------------------------------------------------------------------------------
	MODE_0	=	0,	/*!< BG Mode 0 */
	MODE_1	=	1,	/*!< BG Mode 1 */
	MODE_2	=	2,	/*!< BG Mode 2 */
	MODE_3	=	3,	/*!< BG Mode 3 */
	MODE_4	=	4,	/*!< BG Mode 4 */
	MODE_5	=	5,	/*!< BG Mode 5 */

	BACKBUFFER	=	BIT(4),		/*!< buffer display select			*/
	OBJ_1D_MAP	=	BIT(6),		/*!< sprite 1 dimensional mapping	*/
	LCDC_OFF	=	BIT(7),		/*!< LCDC OFF						*/
	BG0_ON		=	BIT(8),		/*!< enable background 0			*/
	BG1_ON		=	BIT(9),		/*!< enable background 1			*/
	BG2_ON		=	BIT(10),	/*!< enable background 2			*/
	BG3_ON		=	BIT(11),	/*!< enable background 3			*/
	OBJ_ON		=	BIT(12),	/*!< enable sprites					*/
	WIN0_ON		=	BIT(13),	/*!< enable window 0				*/
	WIN1_ON		=	BIT(14),	/*!< enable window 1				*/
	OBJ_WIN_ON	=	BIT(15),	/*!< enable obj window				*/

	BG0_ENABLE		=	BG0_ON,		/*!< enable background 0	*/
	BG1_ENABLE		=	BG1_ON, 	/*!< enable background 1	*/
	BG2_ENABLE		=	BG2_ON, 	/*!< enable background 2	*/
	BG3_ENABLE		=	BG3_ON,		/*!< enable background 3	*/
	OBJ_ENABLE		=	OBJ_ON, 	/*!< enable sprites			*/
	WIN0_ENABLE		=	WIN0_ON,	/*!< enable window 0		*/
	WIN1_ENABLE		=	WIN1_ON,	/*!< enable window 1		*/
	OBJ_WIN_ENABLE	=	OBJ_WIN_ON, /*!< enable obj window		*/

	BG_ALL_ON		=	BG0_ON | BG1_ON | BG2_ON | BG3_ON, 	    /*!< All Backgrounds on.		*/
	BG_ALL_ENABLE	=	BG0_ON | BG1_ON | BG2_ON | BG3_ON	    /*!< All Backgrounds enabled.	*/

} LCDC_BITS;
static inline void SetMode(int mode)	{REG_DISPCNT = mode;}
#endif