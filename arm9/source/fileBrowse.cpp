#include "fileBrowse.h"
#include <algorithm>
#include <dirent.h>
#include <nds.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <vector>


#define SCREEN_COLS 22
#define ENTRIES_PER_SCREEN 23
#define ENTRIES_START_ROW 1
#define OPTIONS_ENTRIES_START_ROW 2
#define ENTRY_PAGE_LENGTH 10

static char path[PATH_MAX];

bool nameEndsWith(const std::string& name, const std::vector<std::string> extensionList) {
	if(name.substr(0, 2) == "._") return false;

	if(name.size() == 0) return false;

	if(extensionList.size() == 0) return true;

	for(int i = 0; i <(int)extensionList.size(); i++) {
		const std::string ext = extensionList.at(i);
		if(strcasecmp(name.c_str() + name.size() - ext.size(), ext.c_str()) == 0) return true;
	}
	return false;
}

bool dirEntryPredicate(const DirEntry& lhs, const DirEntry& rhs) {
	if(!lhs.isDirectory && rhs.isDirectory) {
		return false;
	}
	if(lhs.isDirectory && !rhs.isDirectory) {
		return true;
	}
	return strcasecmp(lhs.name.c_str(), rhs.name.c_str()) < 0;
}

void getDirectoryContents(std::vector<DirEntry>& dirContents, const std::vector<std::string> extensionList) {
	struct stat st;

	dirContents.clear();

	DIR *pdir = opendir(".");

	if(pdir == NULL) {
		printf("Unable to open the directory.\n");
	} else {
		while(true) {
			DirEntry dirEntry;

			struct dirent* pent = readdir(pdir);
			if(pent == NULL)	break;

			stat(pent->d_name, &st);
			dirEntry.name = pent->d_name;
			dirEntry.isDirectory =(st.st_mode & S_IFDIR) ? true : false;

			if(dirEntry.name.compare(".") != 0 &&(dirEntry.isDirectory || nameEndsWith(dirEntry.name, extensionList))) {
				dirContents.push_back(dirEntry);
			}
		}
		closedir(pdir);
	}
	sort(dirContents.begin(), dirContents.end(), dirEntryPredicate);
}

void showDirectoryContents(const std::vector<DirEntry>& dirContents, int fileOffset, int startRow) {
	getcwd(path, PATH_MAX);

	// Clear the screen
	printf("\x1b[2J");

	// Print the path
	printf("\x1B[42m");	// Print green color
	printf("________________________________");
	printf("\x1b[0;0H");
	if(strlen(path) < SCREEN_COLS) {
		printf("%s", path);
	} else {
		printf("%s", path + strlen(path) - SCREEN_COLS);
	}

	// Move to 2nd row
	printf("\x1b[1;0H");

	// Print directory listing
	for(int i = 0; i < (int)(dirContents.size() - startRow) && i < ENTRIES_PER_SCREEN; i++) {
		const DirEntry* entry = &dirContents.at(i + startRow);

		// Set row
		printf("\x1b[%d;0H", i + ENTRIES_START_ROW);
		if((fileOffset - startRow) == i) {
			printf("\x1B[47m");	// Print foreground white color
		} else {
			printf("\x1B[40m");	// Print foreground black color
		}

		if(entry->isDirectory) {
			printf("[%s]", entry->name.substr(0, SCREEN_COLS).c_str());
		} else {
			printf("%s", entry->name.substr(0, SCREEN_COLS).c_str());
		}
	}

	printf("\x1B[47m");	// Print foreground white color
}

std::string browseForFile(void) {
	int pressed = 0, held = 0, screenOffset = 0, fileOffset = 0;
	std::vector<DirEntry> dirContents;

	getDirectoryContents(dirContents, {"mp4"});

	while(true) {
		showDirectoryContents(dirContents, fileOffset, screenOffset);

		// Power saving loop. Only poll the keys once per frame and sleep the CPU if there is nothing else to do
		do {
			scanKeys();
			pressed = keysDown();
			held = keysDownRepeat();
			swiWaitForVBlank();
		} while(!((pressed & (KEY_A | KEY_B)) || (held & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT))));

		printf("\x1B[47m");		// Print foreground white color
		printf("\x1b[%d;0H", fileOffset - screenOffset + ENTRIES_START_ROW);

		if(held & KEY_UP) { 
			if(fileOffset > 0)	fileOffset--;
			else	fileOffset = dirContents.size()-1;
		} else if(held & KEY_DOWN) {
			if(fileOffset < (int)dirContents.size()-1)	fileOffset++;
			else	fileOffset = 0;
		} else if(held & KEY_LEFT) {
			fileOffset -= ENTRY_PAGE_LENGTH;
			if(fileOffset < 0)	fileOffset = 0;
		} else if(held & KEY_RIGHT) {
			fileOffset += ENTRY_PAGE_LENGTH;
			if(fileOffset > (int)dirContents.size()-1)	fileOffset = dirContents.size()-1;
		}

		// Scroll screen if needed
		if(fileOffset < screenOffset) {
			screenOffset = fileOffset;
			showDirectoryContents(dirContents, fileOffset, screenOffset);
		}
		if(fileOffset > screenOffset + ENTRIES_PER_SCREEN - 1) {
			screenOffset = fileOffset - ENTRIES_PER_SCREEN + 1;
			showDirectoryContents(dirContents, fileOffset, screenOffset);
		}

		if(pressed & KEY_A) {
			DirEntry* entry = &dirContents.at(fileOffset);
			if(entry->isDirectory) {
				printf("Entering directory\n");
				// Enter selected directory
				chdir(entry->name.c_str());
				getDirectoryContents(dirContents, {"mp4"});
				screenOffset = 0;
				fileOffset = 0;
			} else {
				return entry->name;
			}
		} else if(pressed & KEY_B) {
			getcwd(path, PATH_MAX);
			if(!((strcmp(path, "sd:/") == 0) || (strcmp(path, "fat:/") == 0) || (strcmp(path, "nitro:/") == 0))) {
				// Go up a directory
				chdir("..");
				getDirectoryContents(dirContents, {"mp4"});
				screenOffset = 0;
				fileOffset = 0;
			}
		}
	}
}
