/*-----------------------------------------------------------------
 Copyright (C) 2005 - 2013
	Michael "Chishm" Chisholm
	Dave "WinterMute" Murphy
	Claudio "sverx"

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

------------------------------------------------------------------*/
#include <nds.h>
#include <cstdio>
#include <fat.h>

#include "args.h"
#include "file_browse.h"
#include "hbmenu_banner.h"
#include "iconTitle.h"
#include "nds_loader_arm9.h"
#include "config/scsfw_config.h"
#include "config/configurator.h"
#include "scsd/sc_commands.h"


[[noreturn]] void stop() {
	while(1) {
		swiWaitForVBlank();
	}
}

bool console_initialized = false;
bool top_screen_initialized = false;

void initConsole() {
	if(console_initialized) return;
	console_initialized = true;

	// Subscreen as a console
	videoSetModeSub(MODE_0_2D);
	vramSetBankH(VRAM_H_SUB_BG);
	consoleInit(nullptr, 0, BgType_Text4bpp, BgSize_T_256x256, 15, 0, false, true);
}

void initTopScreen() {
	if(top_screen_initialized) return;
	top_screen_initialized = true;
	iconTitleInit();
}

static std::string_view getExecPath(const SCSFW_CONFIGS& confs) {
	static char executable_path[128];

	scanKeys();

	switch(keysHeld() & (KEY_A | KEY_B | KEY_X | KEY_Y | KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT)) {
		case KEY_A | KEY_B:
			memcpy(executable_path, "CONFIG", sizeof("CONFIG"));
			break;
		case KEY_A:
		case KEY_RIGHT:
			memcpy(executable_path, confs.hk_a.path, sizeof(confs.hk_a.path));
			break;
		case KEY_B:
		case KEY_DOWN:
			memcpy(executable_path, confs.hk_b.path, sizeof(confs.hk_b.path));
			break;
		case KEY_X:
		case KEY_UP:
			memcpy(executable_path, confs.hk_x.path, sizeof(confs.hk_x.path));
			break;
		case KEY_Y:
		case KEY_LEFT:
			memcpy(executable_path, confs.hk_y.path, sizeof(confs.hk_y.path));
			break;
		default:
			memcpy(executable_path, confs.hk_none.path, sizeof(confs.hk_none.path));
			break;
	}

	executable_path[127] = 0;
	return executable_path;
}

int main(int argc, char** argv) {
	// overwrite reboot stub identifier
	// so tapping power on DSi returns to DSi menu
	extern char* fake_heap_end;
	*fake_heap_end = 0;

	if(!fatInitDefault()) {
		initTopScreen();
		initConsole();
		iprintf("fatinitDefault failed!\n");
		stop();
	}

	SCSFW_CONFIGS confs;
	read_configs(&confs);
	
	const auto executable_path = getExecPath(confs);

	if(executable_path.starts_with("fat:/")) {
		std::vector<std::string> argarray;
		int err = 10;
		if(argsFillArray(executable_path, argarray)) {
			err = runNdsFile(argarray);
		}
		initConsole();
		initTopScreen();
		iprintf("Failed to start\n%s.\n"
				"Error %i.\n"
				"Press START to continue boot.\n"
				"You can change the autoboot\n"
				"settings by holding A+B on\n"
				"launch", executable_path.data(), err);
		while(1) {
			swiWaitForVBlank();
			scanKeys();
			if((keysHeld() & KEY_START)) break;
		}
	}

	initConsole();
	initTopScreen();

	keysSetRepeat(25, 5);

	if(executable_path == "CONFIG") {
		configMenu(&confs);
	}

	chdir("fat:/");

	const auto extensionList = argsGetExtensionList();

	while(1) {

		auto filename = browseForFile(extensionList);

		// Construct a command line
		std::vector<std::string> argarray;
		if(!argsFillArray(filename, argarray)) {
			iprintf("Invalid NDS or arg file selected\n");
		} else {
			iprintf("Running %s with %d parameters\n", argarray[0].c_str(), argarray.size());
			auto err = runNdsFile(argarray);
			iprintf("Start failed. Error %i\n", err);
		}

		while(1) {
			swiWaitForVBlank();
			scanKeys();
			if(!(keysHeld() & KEY_A)) break;
		}
	}

	return 0;
}
