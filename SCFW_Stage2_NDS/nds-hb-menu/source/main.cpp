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
#include <stdio.h>
#include <fat.h>
#include <sys/stat.h>
#include <limits.h>

#include <string.h>
#include <unistd.h>

#include "args.h"
#include "file_browse.h"
#include "hbmenu_banner.h"
#include "iconTitle.h"
#include "nds_loader_arm9.h"
#include "config/scsfw_config.h"
#include "config/configurator.h"
#include "scsd/sc_commands.h"


using namespace std;

//---------------------------------------------------------------------------------
void stop (void) {
//---------------------------------------------------------------------------------
	while (1) {
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
	consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 15, 0, false, true);
}

void initTopScreen() {
	if(top_screen_initialized) return;
	top_screen_initialized = true;
	iconTitleInit();
}

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

	// overwrite reboot stub identifier
	// so tapping power on DSi returns to DSi menu
	extern char *fake_heap_end;
	*fake_heap_end = 0;
	
	if (!fatInitDefault()) {
		initTopScreen();
		initConsole();
		iprintf ("fatinitDefault failed!\n");
		stop();
	}
	
	SCSFW_CONFIGS confs;
	char executable_path[128];
	
	read_configs(&confs);
	
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
	
	if (memcmp(executable_path, "fat:/", sizeof("fat:/") - 1) == 0) {
		std::vector<string> argarray;
		int err = 10;
		std::vector<const char*> c_args;
		if(argsFillArray(executable_path, argarray)) {
			for (const auto& arg: argarray) {
				c_args.push_back(arg.c_str());
			}
			err = runNdsFile(c_args[0], c_args.size(), &c_args[0]);
		}
		initConsole();
		initTopScreen();
		iprintf("Failed to start\n%s.\nError %i.\nPress START to continue boot.\nYou can change the autoboot\nsettings by holding A+B on\nlaunch", c_args[0], err);
		while (1) {
			swiWaitForVBlank();
			scanKeys();
			if ((keysHeld() & KEY_START)) break;
		}
	}
	
	initConsole();
	initTopScreen();	
	
	keysSetRepeat(25,5);

	if(memcmp(executable_path, "CONFIG", sizeof("CONFIG") - 1) == 0) {
		configMenu(&confs);
	}

	chdir("fat:/");

	vector<string> extensionList = argsGetExtensionList();

	while(1) {

		string filename = browseForFile(extensionList);

		// Construct a command line
		vector<string> argarray;
		if (!argsFillArray(filename, argarray)) {
			iprintf("Invalid NDS or arg file selected\n");
		} else {
			iprintf("Running %s with %d parameters\n", argarray[0].c_str(), argarray.size());

			// Make a copy of argarray using C strings, for the sake of runNdsFile
			vector<const char*> c_args;
			for (const auto& arg: argarray) {
				c_args.push_back(arg.c_str());
			}

			// Try to run the NDS file with the given arguments
			int err = runNdsFile(c_args[0], c_args.size(), &c_args[0]);
			iprintf("Start failed. Error %i\n", err);
		}

		argarray.clear();

		while (1) {
			swiWaitForVBlank();
			scanKeys();
			if (!(keysHeld() & KEY_A)) break;
		}

	}

	return 0;
}
