
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <cstring> // basename
#include <sys/stat.h>

#include <nds.h>

#include "configurator.h"
#include "../args.h"
#include "../file_browse.h"
#include "../iconTitle.h"

char* getEntryPath(int off, struct SCSFW_CONFIGS* configs) {
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
}

bool readConfigsFromFile(struct SCSFW_CONFIGS* configs) {
	struct SCSFW_CONFIGS tmpConfs{};
	auto* file = fopen("fat:/scsfw/hotkeys.conf", "rb");
	if(!file) {
		iprintf("\x1b[2J");
		iprintf("fat:/scsfw/hotkeys.conf\nnot found\n");
		return false;
	}
	char line[sizeof(ENTRY::path) + 1]; // newline character would be included
	for(int i = 0; i < 5; ++i) {
		if(!fgets(line, sizeof(line), file)) {
			iprintf("\x1b[2J");
			iprintf("Invalid config file\n");
			fclose(file);
			return false;
		}
		auto len = strlen(line);
		if(len > sizeof(ENTRY::path)) {
			iprintf("\x1b[2J");
			iprintf("Invalid config file\n");
			fclose(file);
			return false;
		}
		if(line[len-1] == '\n') {
			line[len-1] = 0;
			--len;
		}
		strcpy(getEntryPath(i, &tmpConfs), line);
	}
	fclose(file);
	memcpy(configs, &tmpConfs, sizeof(struct SCSFW_CONFIGS));
	return true;
}

bool dumpConfigsToFile(struct SCSFW_CONFIGS* configs) {
	if(mkdir("fat:/scsfw", 0777) == -1 && errno != EEXIST) {
		iprintf("\x1b[2J");
		iprintf("failed to create fat:/scsfw folder\n");
		return false;
	}
	auto* file = fopen("fat:/scsfw/hotkeys.conf", "wb");
	if(!file) {
		iprintf("\x1b[2J");
		iprintf("failed to create fat:/scsfw/hotkeys.conf\n");
		return false;
	}
	for(int i = 0; i < 5; ++i) {
		auto* path = getEntryPath(i, configs);
		fwrite(path, 1, strlen(path), file);
		fwrite("\n", 1, 1, file);
	}
	fclose(file);
	return true;
}

void printErrorAndWaitForA(const char* errorMessage) {
	iprintf("%s\nPress A to continue", errorMessage);
	while(1) {
		scanKeys();
		if(keysDownRepeat() & KEY_A)
			return;
		swiWaitForVBlank();
	}
}

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
		printEntry("A/RIGHT", configs->hk_a);
		printEntry("B/DOWN ", configs->hk_b);
		printEntry("X/UP   ", configs->hk_x);
		printEntry("Y/LEFT ", configs->hk_y);
		
		auto restoreOffset = totalEntries;
		printEntry("LOAD FROM FILE", restoreOffset);
		auto dumpOffset = totalEntries;
		printEntry("SAVE TO FILE", restoreOffset);
		auto cancelOffset = totalEntries;
		printEntry("CANCEL", restoreOffset);
		auto saveAndExitOffset = totalEntries;
		printEntry("SAVE & EXIT", restoreOffset);
		
		auto printCurEntry = [&]{
			// move to row totalEntries + 2
			iprintf("\x1b[%d;0H", totalEntries + 2);
			// clears the screen starting from this row
			iprintf("\x1b[J");
			// move to row totalEntries + 4
			iprintf("\x1b[%d;0H", totalEntries + 4);
			
			clearIconTitle();
			
			if(fileOffset < restoreOffset) {
				auto* filename = getEntryPath(fileOffset, configs);
				iprintf(filename);
				if(*filename != 0) {
					iconTitleUpdate(false, filename);
					iprintf("\x1b[%d;0H", totalEntries + 12);
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
				} else if (fileOffset == restoreOffset) {
					if(!readConfigsFromFile(configs)) {
						printErrorAndWaitForA("Failed to read configs.");
					} else {
						changed = true;
					}
				} else if (fileOffset == dumpOffset) {
					if(!dumpConfigsToFile(configs)) {
						printErrorAndWaitForA("Failed to save configs.");
					}
				} else {
					auto entry = browseForFile({".nds", ".srldr"});
					std::string absPath;
					toAbsPath(entry, {}, absPath);
					if(absPath.size() >= sizeof(ENTRY::path)) {
						iprintf("\x1b[2J");
						printErrorAndWaitForA("File path too long.");
					} else {
						changed = true;
						strcpy(getEntryPath(fileOffset, configs), absPath.data());
					}
				}
				break;
			} else if(pressed & (KEY_L | KEY_R)) {
				if(fileOffset < restoreOffset) {
					changed = true;
					getEntryPath(fileOffset, configs)[0] = 0;
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
