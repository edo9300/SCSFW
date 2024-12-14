
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <string.h> // basename
#include "configurator.h"
#include "../file_browse.h"
#include "../args.h"
#include "../iconTitle.h"
#include <nds.h>

void configMenu(struct SCSFW_CONFIGS* configs) {
	bool changed = false;
	int fileOffset = 0;
	while(true) {
		// Clear the screen
		iprintf("\x1b[2J");

		// Print the path
		iprintf("CONFIGURATION");

		// Move to 2nd row
		iprintf("\x1b[1;0H");
		// Print line of dashes
		iprintf("--------------------------------");
		int totalEntries = 0;
		auto printEntry = [&](const char* prefix, auto& entry) mutable {
			iprintf("\x1b[%d;0H", totalEntries + 2);
			++totalEntries;
			if constexpr(requires{entry.path;}) {
				char entryName[SCREEN_COLS + 1];
				std::string copy = entry.path;
				snprintf(entryName, SCREEN_COLS, " %s (%s)", prefix, basename(copy.data()));
				iprintf(entryName);
			} else {
				iprintf(" %s", prefix);
			}
		};

		printEntry("NO BUTTON", configs->hk_none);
		printEntry("BUTTON A", configs->hk_a);
		printEntry("BUTTON B", configs->hk_b);
		printEntry("BUTTON X", configs->hk_x);
		printEntry("BUTTON Y", configs->hk_y);
		
		auto getEntryPath = [&](int off) -> char* {
			switch(off) {
				case 0:
					return configs->hk_none.path;
				case 1:
					return configs->hk_a.path;
				case 2:
					return configs->hk_b.path;
				case 3:
					return configs->hk_x.path;
				case 4:
					return configs->hk_y.path;
				default:
					__builtin_unreachable();
			}
		};
		
		auto cancelOffset = totalEntries;
		printEntry("CANCEL", cancelOffset);
		auto saveAndExitOffset = totalEntries;
		printEntry("SAVE & EXIT", cancelOffset);
		
		auto printCurEntry = [&]{
			// move to row totalEntries + 2
			iprintf("\x1b[%d;0H", totalEntries + 2);
			// clears the screen starting from this row
			iprintf("\x1b[J");
			// move to row totalEntries + 4
			iprintf("\x1b[%d;0H", totalEntries + 4);
			
			clearIconTitle();
			
			if(fileOffset != cancelOffset && fileOffset != saveAndExitOffset) {
				auto* filename = getEntryPath(fileOffset);
				iprintf(filename);
				if(*filename != 0) {
					iconTitleUpdate(false, filename);
					iprintf("\x1b[%d;0H", totalEntries + 14);
					iprintf("Press L or R to clear");
				}
			}
		};
		
		printCurEntry();
		while(true) {
			scanKeys();
			if((keysHeld() & (KEY_A | KEY_B)) == 0)
				break;
			swiWaitForVBlank();
		}

		while (true) {
			chdir("fat:/");
			// Clear old cursors
			for (int i = 2; i < totalEntries + 2; i++) {
				iprintf("\x1b[%d;0H ", i);
			}
			// Show cursor
			iprintf("\x1b[%d;0H*", fileOffset + 2);


			// Power saving loop. Only poll the keys once per frame and sleep the CPU if there is nothing else to do
			int pressed;
			do {
				scanKeys();
				pressed = keysDownRepeat();
				swiWaitForVBlank();
			} while (!pressed);

			if (pressed & KEY_UP) 		fileOffset -= 1;
			if (pressed & KEY_DOWN) 	fileOffset += 1;

			if (fileOffset < 0) 	fileOffset = totalEntries - 1;		// Wrap around to bottom of list
			if (fileOffset >= totalEntries)		fileOffset = 0;		// Wrap around to top of list
			
			printCurEntry();

			if (pressed & KEY_A) {
				if(fileOffset == cancelOffset) {
					return;
				} else if(fileOffset == saveAndExitOffset) {
					goto end;
				}
				auto entry = browseForFile({".nds", ".srldr"});
				std::string absPath;
				toAbsPath(entry, nullptr, absPath);
				if(absPath.size() >= sizeof(ENTRY::path)) {
					iprintf("\x1b[2J");
					iprintf("File path too long, press\n a to continue");
					do {
						scanKeys();
						pressed = keysDownRepeat();
						swiWaitForVBlank();
					} while ((pressed & KEY_A) == 0);
				} else {
					changed = true;
					strcpy(getEntryPath(fileOffset), absPath.data());
				}
				break;
			} else if(pressed & (KEY_L | KEY_R)) {
				if(fileOffset != cancelOffset && fileOffset != saveAndExitOffset) {
					changed = true;
					getEntryPath(fileOffset)[0] = 0;
					break;
				}
			} else if (pressed & KEY_B) {
				return;
			}
		}
	}
	end:
	
	if(changed) {
		iprintf("Saving configs\n");
		save_configs(configs);
		iprintf("Saved\n");
	}
	
	return;
}
