OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(read_backlight_from_firmware)
SECTIONS
{
    .text :
    {
        *(.text.read_backlight_from_firmware);
        *(.text*);
    }
}