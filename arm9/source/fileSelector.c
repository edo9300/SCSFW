#include <stdio.h>
#include <nds.h>
#include <fat.h>
#include <dirent.h>
#include <string.h>

#include "fileSelector.h"

#define MAX(a, b)   ((a) < (b) ? (b) : (a))
#define MIN(a, b)   ((a) > (b) ? (b) : (a))
#define MAX_DISPLAY_WIDTH 30  // Maximum number of characters displayed per line on screen
#define SCROLL_DELAY 15       // Delay frames during normal scrolling
#define END_PAUSE 30          // Pause frames at the end of scrolling
#define MAX_FILE_COUNT 255        // Maximum supported number of files
#define MAX_FILENAME_LEN 256      // Maximum filename length
#define MAX_PATH_LEN 512          // Maximum path length

extern PrintConsole* currentConsole;
static char filenames[MAX_FILE_COUNT][MAX_FILENAME_LEN];
static char fullPath[MAX_PATH_LEN];

char *selectFirmware(void) {
	DIR *pdir;
	struct dirent *pent;
	
	int fileCount = 0;
	int selection = 0;
    int displayStart = 0;  // Track the start position of the display window
    const int DISPLAY_LIMIT = 22;  // Maximum number of files displayed per screen
    int scrollOffset = 0;  // Horizontal scroll offset
    int scrollTimer = 0;   // Scroll timer
    bool atTextEnd = false;

    // variables related to long press acceleration
    int holdDelay = 0;          // Key hold time counter
    const int INITIAL_DELAY = 15; // Initial delay frames
    const int REPEAT_DELAY = 4;   // Repeat delay frames
    bool isHolding = false;     // Whether it is in long press state
    int lastKey = 0;            // Last pressed direction key

	pdir = opendir("/firmwares");

	memset(filenames, 0, sizeof(filenames));
	memset(fullPath, 0, sizeof(fullPath));

	if(pdir) {
		while((pent = readdir(pdir)) != NULL && fileCount < MAX_FILE_COUNT) {
			if(strcmp(".", pent->d_name) == 0 || strcmp("..", pent->d_name) == 0) continue;
			if(pent->d_type == DT_DIR) continue;

			const char *name = pent->d_name;
			int len = strlen(name);
			if(len < 4) continue; // Extension length must be at least 4 characters
			const char *ext = name + len - 4;
			if(strcasecmp(ext, ".frm") != 0 && strcasecmp(ext, ".bin") != 0) continue;

            strncpy(filenames[fileCount], pent->d_name, MAX_FILENAME_LEN-1);
            filenames[fileCount][MAX_FILENAME_LEN-1] = '\0';
            fileCount++;
		}
		
		closedir(pdir);
		// Bubble sort
		for(int i = 0; i < fileCount - 1; i++) {
			for(int j = 0; j < fileCount - 1 - i; j++) {
				if(strcasecmp(filenames[j], filenames[j + 1]) > 0) {
					char temp[MAX_FILENAME_LEN];
					strcpy(temp, filenames[j]);
					strcpy(filenames[j], filenames[j + 1]);
					strcpy(filenames[j + 1], temp);
				}
			}
		}
		
		while(1) {
			scanKeys();

            if(strlen(filenames[selection]) > MAX_DISPLAY_WIDTH) {
                scrollTimer++;
                atTextEnd = (scrollOffset >= strlen(filenames[selection]) - MAX_DISPLAY_WIDTH);
                if(atTextEnd) {
                    // wait for long name file
                    if(scrollTimer > END_PAUSE) {
                        scrollOffset = 0;
                        scrollTimer = 0;
                        atTextEnd = false;
                    }
                } else {
                    // normal
                    if(scrollTimer > SCROLL_DELAY) {
                        scrollOffset++;
                        scrollTimer = 0;
                    }
                }
            } else {
                scrollOffset = 0;
                scrollTimer = 0;
                atTextEnd = false;
            }

            u32 keys = keysDown();
            u32 heldKeys = keysHeld();
            if(keys & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT)) {
                // New button pressed
                isHolding = true;
                holdDelay = INITIAL_DELAY;
                lastKey = keys & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT);
            } 
            else if(!(heldKeys & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT))) {
                // No button hold
                isHolding = false;
                holdDelay = 0;
            }
            if(isHolding) {
                holdDelay--;
                if(holdDelay <= 0) {
                    holdDelay = REPEAT_DELAY;
                    keys |= lastKey;
                }
            }

            // right and left to jump page
            if(keys & KEY_RIGHT) {
                if(selection + DISPLAY_LIMIT < fileCount) {
                    selection += DISPLAY_LIMIT;
                    displayStart = selection;
                } else {
                    selection = fileCount - 1;
                    displayStart = MAX(0, fileCount - DISPLAY_LIMIT);
                }
                scrollOffset = 0;
                scrollTimer = 0;
                atTextEnd = false;
                continue;
            }
            
            if(keys & KEY_LEFT) {
                if(selection - DISPLAY_LIMIT >= 0) {
                    selection -= DISPLAY_LIMIT;
                    displayStart = selection;
                } else {
                    selection = 0;
                    displayStart = 0;
                }
                scrollOffset = 0;
                scrollTimer = 0;
                atTextEnd = false;
                continue;
            }
            
  
            // move up and down (hold to faster move)
            if(keys & KEY_DOWN && selection < fileCount - 1) {
                int moveStep = 1;
                if(isHolding && holdDelay == REPEAT_DELAY) {
                    moveStep = 3;
                }
                
                selection += moveStep;
                if(selection >= fileCount) selection = fileCount - 1;
                
                if(selection >= displayStart + DISPLAY_LIMIT) {
                    displayStart = selection - DISPLAY_LIMIT + 1;
                }
                scrollOffset = 0;
                scrollTimer = 0;
                atTextEnd = false;
            }
            
            if(keys & KEY_UP && selection > 0) {
                int moveStep = 1;
                if(isHolding && holdDelay == REPEAT_DELAY) {
                    moveStep = 3;
                }
                
                selection -= moveStep;
                if(selection < 0) selection = 0;
                
                if(selection < displayStart) {
                    displayStart = selection;
                }
                scrollOffset = 0;
                scrollTimer = 0;
                atTextEnd = false;
            }
            
            displayStart = MAX(0, MIN(displayStart, fileCount - DISPLAY_LIMIT));

			// show file lists
			consoleClear();
			printf("Select firmware image to flash:\n\n");
            for(int i = 0; i < DISPLAY_LIMIT && displayStart + i < fileCount; i++) {
                int fileIndex = displayStart + i;
				currentConsole->cursorY = 2 + i;
                currentConsole->cursorX = 0;
                
                if(fileIndex == selection) {
                    printf("*");
                    
                    if(strlen(filenames[fileIndex]) > MAX_DISPLAY_WIDTH) {
                        char displayBuf[MAX_DISPLAY_WIDTH + 1];
                        int len = MIN(MAX_DISPLAY_WIDTH, strlen(filenames[fileIndex]) - scrollOffset);
                        strncpy(displayBuf, &filenames[fileIndex][scrollOffset], len);
                        displayBuf[len] = '\0';
                        printf("%s", displayBuf);
                    } else {
                        printf("%-*s", MAX_DISPLAY_WIDTH, filenames[fileIndex]);
                    }
                } else {
                    printf(" ");
                    
                    if(strlen(filenames[fileIndex]) > MAX_DISPLAY_WIDTH) {
                        char displayBuf[MAX_DISPLAY_WIDTH + 1];
                        strncpy(displayBuf, filenames[fileIndex], MAX_DISPLAY_WIDTH - 3);
                        displayBuf[MAX_DISPLAY_WIDTH - 3] = '\0';
                        strcat(displayBuf, "...");
                        printf("%s", displayBuf);
                    } else {
                        printf("%-*s", MAX_DISPLAY_WIDTH, filenames[fileIndex]);
                    }
                }
            }
            
            if(keysDown() & KEY_A) {
                sprintf(fullPath, "/firmwares/%s", filenames[selection]);
                return fullPath;
            }
            
            swiWaitForVBlank();
        }
    }
	else {
        consoleClear();
		printf("directory not found!\n");
		return NULL;
	}
}