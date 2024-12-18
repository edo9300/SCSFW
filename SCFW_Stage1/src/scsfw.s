.arm
.syntax unified
	
cart_base:
.org 0x00
	b entrypoint
.org 0x04

# sc rumble specific code
# when assembled for the rumble,
# this file is padded by 0x40000

# all the data from here until 0xa0 will be replaced by
# the nintendo logo needed for the gba header for the
# non rumble variant of the firmware

# the "read flash" mode for the sc rumble
# is mode "0", this would work on the sc lite
# but doesn't work on the scsd, where it requires
# mode "4", that in turn doesn't work on the sc rumble
sc_mode_flash_ro_rumble:
	mov r0, # 0x0a000000
	sub r0, r0, #0x02
	mov r1, # 0x005a
	add r1, # 0xa500
	strh r1, [r0]
	strh r1, [r0]
	mov r1, # 0
	strh r1, [r0]
	strh r1, [r0]
	ldr pc, real_address
real_address:
	.word   0x08040000 + rumble_entrypoint

sc_rumble_overwrite_target:
	b sc_mode_flash_ro_rumble

# magic value checked by the SuperCard rumble's firmware
# if the value at 0x4006c is 0x00005A5A
# it will read the value at 0x40070 as a destination address
# to which copy the contents of the flash starting off
# 0x40000.
# We want to make the function
# copying the data overwrite itself with our actual entrypoint
# above so that it gets executed and it will
# jump back to flash
.org 0x6c
	.word 0x00005A5A
.org 0x70
	.word (0x2380000 + 0x2c4) - sc_rumble_overwrite_target

.org 0xa0
	.ascii "DsBooterSCFW"
.org 0xac
	.ascii "PASSsc"
.org 0xb2
	.byte 0x96
.org 0xb4
	.fill 8, 0, 0
.org 0xbc
	.byte 0
.org 0xbd
	.fill 3, 0, 0

.org 0xc0
entrypoint:
	b real_entrypoint
	.word 0x57464353 @ SCFW spelled backwards
	.word miniboot_arm7
	.word (miniboot_arm7_end - miniboot_arm7)
	.word miniboot_arm9
	.word (miniboot_arm9_end - miniboot_arm9)
	.word nds_rom
	.word (nds_rom_end - nds_rom)
	.word sc_lite_dldi
	.word (sc_lite_dldi_end - sc_lite_dldi)
	.word scsd_dldi
	.word (scsd_dldi_end - scsd_dldi)

.set supercard_switch_mode_offset, 0x9000
real_entrypoint:
	# copy and run this fn from RAM
	mov r0, #0x02000000
	add r0, r0, $supercard_switch_mode_offset
	adr r1, sc_mode_flash_rw
	adr r2, sc_mode_flash_rw_end
sc_mode_flash_rw_loop:
	ldr r3, [r1], # 4
	str r3, [r0], # 4
	cmp r1, r2
	blt sc_mode_flash_rw_loop
# jumps to sc_mode_flash_rw copied to address 0x02009000
	mov r0, #0x02000000
	add r0, r0, $supercard_switch_mode_offset
	mov lr, pc
	mov pc, r0

rumble_entrypoint:
	
	# detect GBA/NDS using mirroring
	mov r0, # 0x02000000
	add r0, r0, $supercard_switch_mode_offset
	mov r2, # 0
	str r2, [r0]
	add r1, r0, #0x00040000
	mov r2, # 1
	str r2, [r1]
	ldr r2, [r0]
	cmp r2, # 0
	beq nds_code


# load & execute multiboot gba rom
load_gba:
	adrl r0, gba_rom
	adrl r1, gba_rom_end
	mov r2, # 0x02000000
	add lr, r2, # 0x000000c0
load_gba_loop:
	ldr r3, [r0], # 4
	str r3, [r2], # 4
	cmp r0, r1
	blt load_gba_loop
	bx lr

# load & execute nds rom
nds_code:
	# sets the power flag to the one preferred by the firmware and
	# clears the flag that sets the maximum brightness when plugged in
	bl read_backlight_from_firmware

#the ds firmware stores the decrypted secure area of games at either 0x02000000 or 0x02004000
#don't touch that memory region
	mov r2, # 0x02000000
	add r2, r2, $supercard_switch_mode_offset
	mov r4, r2
	adrl r0, miniboot_arm9
	adrl r1, miniboot_arm9_end
load_miniboot_arm9_loop:
	ldr r3, [r0], # 4
	str r3, [r2], # 4
	cmp r0, r1
	blt load_miniboot_arm9_loop
	
	mov r2, # 0x03800000
	adrl r0, miniboot_arm7
	adrl r1, miniboot_arm7_end
load_miniboot_arm7_loop:
	ldr r3, [r0], # 4
	str r3, [r2], # 4
	cmp r0, r1
	blt load_miniboot_arm7_loop
	
	# reset arm9
	mov r1, # 0x02800000
	sub r1, r1, # 0x200
	str r4, [r1, # 0x24]
	mov lr, # 0x03800000
	bx lr

sc_mode_flash_rw:
	mov r0, # 0x0a000000
	sub r0, r0, #0x02
	mov r1, # 0x005a
	add r1, # 0xa500
	strh r1, [r0]
	strh r1, [r0]
	mov r1, # 4
	strh r1, [r0]
	strh r1, [r0]
	bx lr
sc_mode_flash_rw_end:

.balign 4, 0xff
read_backlight_from_firmware:
.incbin "../build/set_backlight.bin"

.balign 4, 0xff
gba_rom:
.incbin "../../SCFW_Stage2_GBA/SCFW_Stage2_GBA_mb.gba"
gba_rom_end:

.balign 4, 0xff
miniboot_arm7:
.incbin "miniboot/build/arm7.bin"
miniboot_arm7_end:

.balign 4, 0xff
miniboot_arm9:
.incbin "miniboot/build/arm9.bin"
miniboot_arm9_end:

.balign 4, 0xff
nds_rom:
.incbin "../../SCFW_Stage2_NDS/SCFW_Stage2_NDS.nds"
nds_rom_end:

.balign 4, 0xff
sc_lite_dldi:
.incbin "../../dldi/sc-lite.dldi"
sc_lite_dldi_end:

.balign 4, 0xff
scsd_dldi:
.incbin "../../dldi/scsd.dldi"
scsd_dldi_end:

