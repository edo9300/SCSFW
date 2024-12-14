#ifndef __SCSFW_CONFIG_H__
#define __SCSFW_CONFIG_H__

#include "../scsd/sc_commands.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ENTRY {
	char path[128];
} ENTRY;

typedef struct SCSFW_CONFIGS {
	ENTRY hk_a;
	ENTRY hk_b;
	ENTRY hk_x;
	ENTRY hk_y;
	ENTRY hk_none;
} SCSFW_CONFIGS;

void read_configs(struct SCSFW_CONFIGS* configs);
void save_configs(struct SCSFW_CONFIGS* configs);

#ifdef __cplusplus
}
#endif

#endif //__SCSFW_CONFIG_H__