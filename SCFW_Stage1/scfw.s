.arm
.syntax unified
	
cart_base:
.org 0x00
	b entrypoint
.org 0x04
	.fill 0x9c, 0, 0
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
.set supercard_switch_mode_offset, 0x9000
entrypoint:
	b real_entrypoint
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
#libnds crt clears those important values from the header, back them up to an unused
#area of the header to then be retrieved manually later
	mov r0, # 0x02800000
	sub r0, r0, # 0x200
	add r1, r0, #0x80
	add r2, r0, #0x88
	add r3, r0, #0x94
backup_header_contents_loop:
	ldr r4, [r1], # 4
	str r4, [r3], # 4
	cmp r1, r2
	blt backup_header_contents_loop

	#check if the secure area was loaded at 0x02000000
	mov r0, # 0x02000000
	ldr r1, [r0]
	cmp r1, # 0
	#something nonzero was found here, assume it's the arm9 binary
	bne load_nds_binaries

	#secure area contents not found, could be at 0x02004000
	add r1, r0, # 0x4000
	ldr r2, [r1]
	cmp r2, # 0
	#no secure area binaries, just load target rom
	beq load_nds_binaries
	mov r2, # 0x02000000
	add r3, r1, # 0x4000
move_secure_area_loop:
	ldr r4, [r1], # 4
	str r4, [r2], # 4
	cmp r1, r3
	blt move_secure_area_loop

load_nds_binaries:
	mov r2, # 0x02000000
	add r2, r2, # 0x8000
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

