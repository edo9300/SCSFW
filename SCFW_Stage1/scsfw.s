.arm
.syntax unified

.org 0x40000
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
	.word (scsd_dldi_end - sc_lite_dldi)
.org 0x40060
	ldr pc, real_address
real_address:
        .word   0x08040000

.org 0x4006c
	.word 0x00005A5A
.org 0x40070
	.word (0x2380000 + 0x2c4) - 0x60

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
	bl removePowerFlag

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
	mov r1, # 0
	strh r1, [r0]
	strh r1, [r0]
	bx lr
sc_mode_flash_rw_end:


# generated asm from set_backlight.c

.set SPI_BUSY, 0x80
.set SPI_CNT_OFF, 0xC0
.set SPI_DATA_OFF, 0xC2
writePowerManagement:
	ldr	r3, .L8
.L2:
	ldrh	r2, [r3, $SPI_CNT_OFF]
	tst	r2, $SPI_BUSY
	bne	.L2
	ldr	r2, .L8+4
	strh	r2, [r3, $SPI_CNT_OFF]
	strh	r0, [r3, $SPI_DATA_OFF]
.L3:
	ldrh	r2, [r3, $SPI_CNT_OFF]
	tst	r2, $SPI_BUSY
	bne	.L3
	ldr	r2, .L8+8
	strh	r2, [r3, $SPI_CNT_OFF]
	strh	r1, [r3, $SPI_DATA_OFF]
.L4:
	ldrh	r2, [r3, $SPI_CNT_OFF]
	tst	r2, $SPI_BUSY
	bne	.L4
	ldrh	r0, [r3, $SPI_DATA_OFF]
	and	r0, r0, # 0xff
	bx	lr
.L8:
	.word	0x04000100
	# SPI_ENABLE | SPI_BAUD_1MHz | SPI_BYTE_MODE | SPI_CONTINUOUS | SPI_DEVICE_POWER
	.word	0x8802
	# SPI_ENABLE | SPI_BAUD_1MHz | SPI_BYTE_MODE | SPI_DEVICE_POWER
	.word	0x8002

removePowerFlag:
	mov	r1, # 0
	push	{r4, lr}
	mov	r0, # 0x84
	bl	writePowerManagement
	ldr	r3, .L12
	ldrb	r1, [r3, #-27]	@ zero_extendqisi2
	and	r0, r0, # 0xF8
	lsl	r1, r1, # 0x1A
	orr	r1, r0, r1, lsr # 30
	pop	{r4, lr}
	mov	r0, # 4
	b	writePowerManagement
.L12:
	.word	0x27FFCFF

.balign 4, 0xff
gba_rom:
.incbin "../SCFW_Stage2_GBA/SCFW_Stage2_GBA_mb.gba"
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
.incbin "../SCFW_Stage2_NDS/SCFW_Stage2_NDS.nds"
nds_rom_end:

.balign 4, 0xff
sc_lite_dldi:
.incbin "../dldi/sc-lite.dldi"
sc_lite_dldi_end:

.balign 4, 0xff
scsd_dldi:
.incbin "../dldi/scsd.dldi"
scsd_dldi_end:

