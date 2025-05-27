#include <stdio.h>
#include <nds.h>
#include <fat.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#include "fileSelector.h"

#define MAX(a, b)   ((a) < (b) ? (b) : (a))
#define MIN(a, b)   ((a) > (b) ? (b) : (a))
#define MAX_DISPLAY_WIDTH 29   // Max characters displayed per file entry line
#define PATH_DISPLAY_WIDTH 26  // Max characters for path scrolling (excluding "Path: ")
#define SCROLL_DELAY 15        // Frames delay before scrolling text
#define END_PAUSE 30           // Frames to pause at end of scrolling
#define MAX_FILE_COUNT 512     // Maximum number of directory entries
#define MAX_FILENAME_LEN 256
#define MAX_PATH_LEN 512

extern PrintConsole* currentConsole;

static char currentPath[MAX_PATH_LEN] = "/";
static char fullPath[MAX_PATH_LEN];

// Struct to store file/folder entry info
typedef struct {
    char name[MAX_FILENAME_LEN];
    u8 isDir;
} Entry;

static Entry entries[MAX_FILE_COUNT];

/**
 * Displays a file/folder selector UI and lets the user pick a firmware file.
 * Supports scrolling long filenames and paths, B key to go up a directory,
 * and proper sorting with directories first.
 * 
 * Returns a pointer to the full selected file path, or NULL on error.
 */
char* selectFirmware(void) {
    int fileCount = 0;
    int selection = 0;
    int displayStart = 0;
    const int DISPLAY_LIMIT = 20;  // Number of lines to display for files

    int scrollOffset = 0;          // Scroll offset for long selected filename
    int scrollTimer = 0;           // Timer to control scrolling speed
    bool atTextEnd = false;        // Flag for scrolling at end of filename

    int pathScrollOffset = 0;      // Scroll offset for long path
    int pathScrollTimer = 0;       // Timer to control path scrolling
    bool pathAtEnd = false;        // Flag for scrolling at end of path

    int holdDelay = 0;
    const int INITIAL_DELAY = 15;  // Initial delay before repeating key input
    const int REPEAT_DELAY = 4;    // Delay between repeated key input events
    bool isHolding = false;
    int lastKey = 0;

    while (1) {
        DIR* pdir = opendir(currentPath);
        fileCount = 0;
        memset(entries, 0, sizeof(entries));

        if (pdir) {
            // Add ".." entry to go up directory unless at root
            if (strcmp(currentPath, "/") != 0) {
                strcpy(entries[fileCount].name, "..");
                entries[fileCount].isDir = 1;
                fileCount++;
            }

            struct dirent* pent;
            while ((pent = readdir(pdir)) != NULL && fileCount < MAX_FILE_COUNT) {
                // Skip "." and ".."
                if (strcmp(pent->d_name, ".") == 0 || 
                    strcmp(pent->d_name, "..") == 0)
                    continue;
                // Skipping too long filename
                if (strlen(pent->d_name) >= MAX_FILENAME_LEN)
                    continue;
                    
                int isDir = (pent->d_type == DT_DIR);
                // If it's a file, check extension
                if (!isDir) {
                    const char* ext = strrchr(pent->d_name, '.');
                    if (!ext || 
                        (strcasecmp(ext, ".frm") != 0 && 
                         strcasecmp(ext, ".bin") != 0 && 
                         strcasecmp(ext, ".fw") != 0 && 
                         strcasecmp(ext, ".gba") != 0 && 
                         strcasecmp(ext, ".nds") != 0))
                        continue;  // Skip files without valid extension
                }
                
                memset(entries[fileCount].name, 0, sizeof(entries[fileCount].name));
                strncpy(entries[fileCount].name, pent->d_name, MAX_FILENAME_LEN - 1);
                entries[fileCount].name[MAX_FILENAME_LEN - 1] = '\0';
                entries[fileCount].isDir = isDir;
                fileCount++;
            }
            closedir(pdir);

            // Sort entries: directories first, then files, both alphabetically
            for (int i = 0; i < fileCount - 1; i++) {
                for (int j = 0; j < fileCount - i - 1; j++) {
                    if ((entries[j].isDir < entries[j + 1].isDir) ||
                        (entries[j].isDir == entries[j + 1].isDir &&
                         strcasecmp(entries[j].name, entries[j + 1].name) > 0)) {
                        Entry temp = entries[j];
                        entries[j] = entries[j + 1];
                        entries[j + 1] = temp;
                    }
                }
            }
        } else {
            consoleClear();
            printf("Directory not found!\n");
            return NULL;
        }

        while (1) {
            scanKeys();

            // Scroll path text only (exclude "Path: " label)
            int pathLen = strlen(currentPath);
            if (pathLen > PATH_DISPLAY_WIDTH) {
                pathScrollTimer++;
                pathAtEnd = (pathScrollOffset >= pathLen - PATH_DISPLAY_WIDTH);
                if (pathAtEnd) {
                    if (pathScrollTimer > END_PAUSE) {
                        pathScrollOffset = 0;
                        pathScrollTimer = 0;
                        pathAtEnd = false;
                    }
                } else if (pathScrollTimer > SCROLL_DELAY) {
                    pathScrollOffset++;
                    pathScrollTimer = 0;
                }
            } else {
                pathScrollOffset = 0;
                pathScrollTimer = 0;
                pathAtEnd = false;
            }

            // Scroll filename if longer than display width
            if (strlen(entries[selection].name) > MAX_DISPLAY_WIDTH) {
                scrollTimer++;
                atTextEnd = (scrollOffset >= strlen(entries[selection].name) - MAX_DISPLAY_WIDTH);
                if (atTextEnd) {
                    if (scrollTimer > END_PAUSE) {
                        scrollOffset = 0;
                        scrollTimer = 0;
                        atTextEnd = false;
                    }
                } else if (scrollTimer > SCROLL_DELAY) {
                    scrollOffset++;
                    scrollTimer = 0;
                }
            } else {
                scrollOffset = 0;
                scrollTimer = 0;
                atTextEnd = false;
            }

            u32 keys = keysDown();
            u32 heldKeys = keysHeld();

            // Handle key hold and repeat logic for navigation keys
            if (keys & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT)) {
                isHolding = true;
                holdDelay = INITIAL_DELAY;
                lastKey = keys & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT);
            } else if (!(heldKeys & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT))) {
                isHolding = false;
                holdDelay = 0;
            }
            if (isHolding) {
                holdDelay--;
                if (holdDelay <= 0) {
                    holdDelay = REPEAT_DELAY;
                    keys |= lastKey;
                }
            }

            // Navigate right: jump forward by DISPLAY_LIMIT entries
            if (keys & KEY_RIGHT) {
                if (selection + DISPLAY_LIMIT < fileCount) {
                    selection += DISPLAY_LIMIT;
                    displayStart = selection;
                } else {
                    selection = fileCount - 1;
                    displayStart = MAX(0, fileCount - DISPLAY_LIMIT);
                }
                scrollOffset = scrollTimer = 0;
                continue;
            }

            // Navigate left: jump backward by DISPLAY_LIMIT entries
            if (keys & KEY_LEFT) {
                if (selection - DISPLAY_LIMIT >= 0) {
                    selection -= DISPLAY_LIMIT;
                    displayStart = selection;
                } else {
                    selection = 0;
                    displayStart = 0;
                }
                scrollOffset = scrollTimer = 0;
                continue;
            }

            // Navigate down: move selection down
            if (keys & KEY_DOWN && selection < fileCount - 1) {
                int step = (isHolding && holdDelay == REPEAT_DELAY) ? 3 : 1;
                selection = MIN(selection + step, fileCount - 1);
                if (selection >= displayStart + DISPLAY_LIMIT) {
                    displayStart = selection - DISPLAY_LIMIT + 1;
                }
                scrollOffset = scrollTimer = 0;
            }

            // Navigate up: move selection up
            if (keys & KEY_UP && selection > 0) {
                int step = (isHolding && holdDelay == REPEAT_DELAY) ? 3 : 1;
                selection = MAX(selection - step, 0);
                if (selection < displayStart) {
                    displayStart = selection;
                }
                scrollOffset = scrollTimer = 0;
            }

            // B key goes up one directory level, same as selecting ".."
            if (keys & KEY_B) {
                if (strcmp(currentPath, "/") != 0) {
                    char* lastSlash = strrchr(currentPath, '/');
                    if (lastSlash) {
                        if (lastSlash == currentPath)
                            lastSlash[1] = '\0'; // Keep root "/"
                        else
                            *lastSlash = '\0';    // Truncate path to parent
                    }
                    selection = displayStart = scrollOffset = scrollTimer = 0;
                    break;  // Refresh directory listing
                }
            }

            // Clamp displayStart to valid range
            displayStart = MAX(0, MIN(displayStart, fileCount - DISPLAY_LIMIT));

            // Clear screen at the start of frame
            consoleClear();

            // Print fixed title line (line 0)
            currentConsole->cursorY = 0;
            currentConsole->cursorX = 0;
            printf("Select firmware image to flash:");

            // Print "Path: " label and current path on line 1 with scrolling
            currentConsole->cursorY = 1;
            currentConsole->cursorX = 0;
            char pathDisplayBuf[PATH_DISPLAY_WIDTH + 6 + 1];
            char pathScrollBuf[PATH_DISPLAY_WIDTH + 1];
            pathScrollOffset = (pathLen <= PATH_DISPLAY_WIDTH) ? 0 : pathScrollOffset;
            strncpy(pathScrollBuf, &currentPath[pathScrollOffset], PATH_DISPLAY_WIDTH);
            pathScrollBuf[PATH_DISPLAY_WIDTH] = '\0';
            snprintf(pathDisplayBuf, sizeof(pathDisplayBuf), "Path: %s", pathScrollBuf);
            printf("%s\n", pathDisplayBuf);

            // Print file/folder list starting at line 2
            for (int i = 0; i < DISPLAY_LIMIT && displayStart + i < fileCount; i++) {
                int index = displayStart + i;
                currentConsole->cursorY = 3 + i;
                currentConsole->cursorX = 0;

                // Mark selected entry with '*'
                if (index == selection)
                    printf("*");
                else
                    printf(" ");

                // Prepare filename display with scrolling if selected
                char displayBuf[MAX_DISPLAY_WIDTH + 1];
                if (index == selection && strlen(entries[index].name) > MAX_DISPLAY_WIDTH) {
                    int len = MIN(MAX_DISPLAY_WIDTH, strlen(entries[index].name) - scrollOffset);
                    strncpy(displayBuf, &entries[index].name[scrollOffset], len);
                    displayBuf[len] = '\0';
                } else if (strlen(entries[index].name) > MAX_DISPLAY_WIDTH) {
                    strncpy(displayBuf, entries[index].name, MAX_DISPLAY_WIDTH - 3);
                    strcpy(displayBuf + MAX_DISPLAY_WIDTH - 3, "...");
                    displayBuf[MAX_DISPLAY_WIDTH] = '\0';
                } else {
                    snprintf(displayBuf, MAX_DISPLAY_WIDTH + 1, "%s", entries[index].name);
                }

                // Prefix directories with '>', files with space
                printf("%s%s\n", entries[index].isDir ? ">" : " ", displayBuf);
            }

            // If user pressed A key (select)
            if (keys & KEY_A) {
                if (entries[selection].isDir) {
                    // Change directory
                    if (strcmp(entries[selection].name, "..") == 0) {
                        // Go up one directory
                        if (strcmp(currentPath, "/") != 0) {
                            char* lastSlash = strrchr(currentPath, '/');
                            if (lastSlash) {
                                if (lastSlash == currentPath)
                                    lastSlash[1] = '\0';
                                else
                                    *lastSlash = '\0';
                            }
                        }
                    } else {
                        // Append selected folder to current path
                        if (strcmp(currentPath, "/") != 0) {
                            if (strlen(currentPath) + 1 + strlen(entries[selection].name) + 1 > sizeof(currentPath)) {
                                printf("Error: Path too long!\n");
                                return NULL;
                            }
                            strcat(currentPath, "/");
                        }
                        if (1 + strlen(entries[selection].name) + 1 > sizeof(currentPath)) {
                            printf("Error: Path too long!\n");
                            return NULL;
                        }
                        strcat(currentPath, entries[selection].name);
                    }
                    selection = displayStart = scrollOffset = scrollTimer = 0;
                    break; // Refresh directory listing
                } else {
                    // File selected, build full path and return it
                    if (strcmp(currentPath, "/") == 0) {
                        if (1 + strlen(entries[selection].name) + 1 > sizeof(currentPath)) {
                            printf("Error: Path too long!\n");
                            return NULL;
                        }
                        strcpy(fullPath, "/");
                        strcat(fullPath, entries[selection].name);
                    } else {
                        if (strlen(currentPath) + 1 + strlen(entries[selection].name) + 1 > sizeof(currentPath)) {
                            printf("Error: Path too long!\n");
                            return NULL;
                        }
                        strcpy(fullPath, currentPath);
                        strcat(fullPath, "/");
                        strcat(fullPath, entries[selection].name);
                    }

                    return fullPath;
                }
            }

            swiWaitForVBlank();
        }
    }
}
