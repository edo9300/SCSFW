// not actually compiled code
typedef unsigned char u8;
typedef volatile u8 vu8;
typedef unsigned short u16;
typedef volatile u16 vu16;
typedef unsigned int u32;
typedef volatile u32 vu32;
#define BIT(n) (1 << (n))
typedef enum {
	POWER_SOUND = BIT(0),			//!<	Controls the power for the sound controller.

	PM_CONTROL_REG		= 0,		//!<	Selects the PM control register
	PM_BATTERY_REG		= 1,		//!<	Selects the PM battery register
	PM_AMPLIFIER_REG	= 2,		//!<	Selects the PM amplifier register
	PM_READ_REGISTER	= (1<<7),	//!<	Selects the PM read register
	PM_AMP_OFFSET		= 2,		//!<	Selects the PM amp register
	PM_GAIN_OFFSET		= 3,		//!<	Selects the PM gain register
	PM_BACKLIGHT_LEVEL	= 4, 		//!<	Selects the DS Lite backlight register
	PM_GAIN_20			= 0,		//!<	Sets the mic gain to 20db
	PM_GAIN_40			= 1,		//!<	Sets the mic gain to 40db
	PM_GAIN_80			= 2,		//!<	Sets the mic gain to 80db
	PM_GAIN_160			= 3,		//!<	Sets the mic gain to 160db
	PM_AMP_ON			= 1,		//!<	Turns the sound amp on
	PM_AMP_OFF			= 0			//!<	Turns the sound amp off
} ARM7_power;

#define REG_SPICNT      (*(vu16*)0x040001C0)
#define REG_SPIDATA     (*(vu16*)0x040001C2)

#define SPI_ENABLE  BIT(15)
#define SPI_IRQ     BIT(14)
#define SPI_BUSY    BIT(7)

#define SPI_BAUD_4MHz    0
#define SPI_BAUD_2MHz    1
#define SPI_BAUD_1MHz    2
#define SPI_BAUD_512KHz  3

#define SPI_BYTE_MODE   (0<<10)
#define SPI_HWORD_MODE  (1<<10)

#define SPI_CONTINUOUS       BIT(11)

#define SPI_DEVICE_POWER      (0 << 8)

#define REG_IME	(*(vu32*)0x04000208)
#define FIRMWARE_SETTINGS (*(vu8*)0x027FFC80)


//---------------------------------------------------------------------------------
static int writePowerManagement(int reg, int command) {
//---------------------------------------------------------------------------------
	// Write the register / access mode (bit 7 sets access mode)
	while (REG_SPICNT & SPI_BUSY);
	REG_SPICNT = SPI_ENABLE | SPI_BAUD_1MHz | SPI_BYTE_MODE | SPI_CONTINUOUS | SPI_DEVICE_POWER;
	REG_SPIDATA = reg;

	// Write the command / start a read
	while (REG_SPICNT & SPI_BUSY);
	REG_SPICNT = SPI_ENABLE | SPI_BAUD_1MHz | SPI_BYTE_MODE | SPI_DEVICE_POWER;
	REG_SPIDATA = command;

	// Read the result
	while (REG_SPICNT & SPI_BUSY);

	return REG_SPIDATA & 0xFF;
}

static int readPowerManagement(int reg) {
	return writePowerManagement((reg)|PM_READ_REGISTER, 0);
}

typedef struct LanguageAndFlags {
	u8 language : 3;
	u8 gbaPosition : 1;
	u8 backlightLevel : 2;
	u8 bootmenuDisable : 1;
	u8 unknown : 2;
    u8 SettingsLost : 1; //(1=Prompt for User Info, and Language, and Calibration)
    u8 SettingsOkay1 : 1; //(0=Prompt for User Info)
    u8 SettingsOkay2 : 1; //(0=Prompt for User Info) (Same as Bit10)
    u8 Nofunction    : 1;
    u8 SettingsOkay3 : 1; //(0=Prompt for User Info, and Language)
    u8 SettingsOkay4 : 1; //(0=Prompt for User Info) (Same as Bit10)
    u8 SettingsOkay5 : 1; //(0=Prompt for User Info) (Same as Bit10)
} __attribute__ ((packed)) LanguageAndFlags;

static_assert(sizeof(LanguageAndFlags) == 2);

extern void removePowerFlag(void) {
	unsigned char pmBacklight = readPowerManagement(PM_BACKLIGHT_LEVEL);
	volatile LanguageAndFlags* language_and_flags = ((volatile LanguageAndFlags*)(0x027FFC80 + 0x064));
	writePowerManagement(PM_BACKLIGHT_LEVEL, ((pmBacklight & (~(BIT(2) | BIT(1) | BIT(0)))) | language_and_flags->backlightLevel));
}
