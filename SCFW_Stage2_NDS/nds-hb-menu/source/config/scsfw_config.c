#include "scsfw_config.h"
#include "../scsd/sc_commands.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#define CONFIG_MAX_SIZE (0x7C000 - 0x70000)
static const uint16_t SLOT_MAGIC = 0xABCD;

static volatile struct SCSFW_CONFIGS_BLOCK* get_conf_block(SUPERCARD_TYPE supercardType) {
	intptr_t start = 0x08070000;
	if(supercardType == SC_RUMBLE)
		start += 0x40000;
	return (volatile struct SCSFW_CONFIGS_BLOCK*)start;
}

#define MAX_CONFS 64

struct SCSFW_CONFIGS_BLOCK {
	char magic[8];
	uint16_t used_slots[MAX_CONFS];
	struct SCSFW_CONFIGS confs[MAX_CONFS];
};

static struct SCSFW_CONFIGS default_configs = {
	.hk_a = {{0}},
	.hk_b = {{0}},
	.hk_x = {{0}},
	.hk_y = {{0}},
	.hk_none = {{0}},
};

static char conf_magic_str[8] = {"SCSFWCON"};

static bool check_magic(volatile const char* magic) {
	for(int i = 0; i < 8; ++i) {
		if(magic[i] != conf_magic_str[i])
			return false;
	}
	return true;
}

static volatile struct SCSFW_CONFIGS* find_active_block(volatile struct SCSFW_CONFIGS_BLOCK* conf) {
	uint32_t i = 0;
	for(; i < MAX_CONFS; ++i) {
		if(conf->used_slots[i] != SLOT_MAGIC)
			break;
	}
	if(i == 0)
		return NULL;
	return &conf->confs[i - 1];
}

static uint32_t find_available_conf_block_idx(volatile struct SCSFW_CONFIGS_BLOCK* conf) {
	uint32_t i = 0;
	for(; i < MAX_CONFS; ++i) {
		if(conf->used_slots[i] != SLOT_MAGIC)
			break;
	}
	if(i == MAX_CONFS) {
		return 0xFFFFFFFF;
	}
	return i;
}

void read_configs(struct SCSFW_CONFIGS* configs) {
	SUPERCARD_TYPE supercardType = get_supercard_type();
	sc_flash_rw_enable(supercardType);
	volatile struct SCSFW_CONFIGS_BLOCK* conf = get_conf_block(supercardType);
	if(!check_magic(conf->magic)) {
		memcpy(configs, (void*)&default_configs, sizeof(struct SCSFW_CONFIGS));
		return;
	}
	
	volatile struct SCSFW_CONFIGS* block = find_active_block(conf);
	if(block == NULL) {
		memcpy(configs, (void*)&default_configs, sizeof(struct SCSFW_CONFIGS));
		return;
	}
	
	memcpy(configs, (void*)block, sizeof(struct SCSFW_CONFIGS));
}

static void write_conf_magic(SUPERCARD_TYPE supercardType) {
	volatile struct SCSFW_CONFIGS_BLOCK* conf = get_conf_block(supercardType);
	sc_flash_write((uint16_t*)&conf_magic_str, (volatile uint16_t*)conf->magic, sizeof(conf_magic_str), supercardType);
}

void save_configs(struct SCSFW_CONFIGS* configs) {
	SUPERCARD_TYPE supercardType = get_supercard_type();
	sc_flash_rw_enable(supercardType);
	volatile struct SCSFW_CONFIGS_BLOCK* conf = get_conf_block(supercardType);
	uint32_t idx = 0xFFFFFFFF;
	if(!check_magic(conf->magic)) {
		sc_flash_erase_sector((volatile uint16_t*)conf, supercardType);
		write_conf_magic(supercardType);
		idx = 0;
	}
	
	if(idx == 0xFFFFFFFF)
		idx = find_available_conf_block_idx(conf);
	if(idx == 0xFFFFFFFF) {
		sc_flash_erase_sector((volatile uint16_t*)conf, supercardType);
		write_conf_magic(supercardType);
		idx = 0;
	}
	sc_flash_write((uint16_t*)&SLOT_MAGIC, (volatile uint16_t*)&conf->used_slots[idx], sizeof(SLOT_MAGIC), supercardType);
	sc_flash_write((uint16_t*)configs, (volatile uint16_t*)&conf->confs[idx], sizeof(struct SCSFW_CONFIGS), supercardType);
}