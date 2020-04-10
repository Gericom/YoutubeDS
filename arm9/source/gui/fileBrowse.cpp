#include "fileBrowse.h"
#include <algorithm>
#include <dirent.h>
#include <nds.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "graphics.h"

#include "fileBrowseBg.h"

#define SCREEN_COLS 22
#define ENTRIES_PER_SCREEN 11
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
		printText("Unable to open the directory.", 1, 1, 0, 0, 0);
	} else {
		while(true) {
			DirEntry dirEntry;

			struct dirent* pent = readdir(pdir);
			if(pent == NULL)	break;

			stat(pent->d_name, &st);
			dirEntry.name = pent->d_name;
			dirEntry.isDirectory = (st.st_mode & S_IFDIR) ? true : false;

			if(dirEntry.name.compare(".") != 0 && (dirEntry.isDirectory || nameEndsWith(dirEntry.name, extensionList))) {
				dirContents.push_back(dirEntry);
			}
		}
		closedir(pdir);
	}
	sort(dirContents.begin(), dirContents.end(), dirEntryPredicate);
}

std::vector<std::string> watchedListGet(void) {
	FILE* list = fopen(".MPEG4DS.watched", "rb");
	std::vector<std::string> watchedList;

	if(list) {
		char* line = NULL;
		size_t len = 0;

		while(__getline(&line, &len, list) != -1) {
			line[strlen(line)-1] = '\0'; // Remove newline
			watchedList.push_back(line);
		}
	}

	fclose(list);
	return watchedList;
}

void watchedListAdd(std::vector<std::string> &watchedList, std::string item) {
	for(unsigned int i=0;i<watchedList.size();i++) {
		if(watchedList[i] == item) {
			return;
		}
	}

	watchedList.push_back(item);
}

void watchedListRemove(std::vector<std::string> &watchedList, std::string item) {
	for(unsigned int i=0;i<watchedList.size();i++) {
		if(watchedList[i] == item) {
			watchedList.erase(watchedList.begin()+i);
			break;
		}
	}
}

void watchedListSave(const std::vector<std::string> &watchedList) {
	FILE* list = fopen(".MPEG4DS.watched", "wb");

	if(list) {
		for(unsigned int i=0;i<watchedList.size();i++) {
			fwrite((watchedList[i] + "\n").c_str(), 1, watchedList[i].size()+1 , list);
		}
	}

	fclose(list);
}

void showDirectoryContents(const std::vector<DirEntry>& dirContents, const std::vector<std::string> &watchedList, int selection, int startRow) {
	getcwd(path, PATH_MAX);

	// Print path
	drawImageSection(0, 0, 256, 17, fileBrowseBgBitmap, 256, 0, 0, 16);
	printTextMaxW(path, 250, 1, 0, 3, 1);

	// Print directory listing
	for(int i=0;i < ENTRIES_PER_SCREEN; i++) {
		// Clear row
		drawImageSection(0, i*16+16, 256, 16, fileBrowseBgBitmap, 256, 0, i*16+16, 16);

		if(i < ((int)dirContents.size() - startRow)) {
			const DirEntry *entry = &dirContents.at(startRow+i);
			std::u16string name = UTF8toUTF16(entry->name);

			// Trim to fit on screen
			bool addEllipsis = false;
			while(getTextWidth(name) > 240) {
				name = name.substr(0, name.length()-1);
				addEllipsis = true;
			}
			if(addEllipsis)	name += UTF8toUTF16("...");

			bool watched = false;
			if(!entry->isDirectory) {
				for(unsigned int j=0;j<watchedList.size();j++) {
					if(watchedList[j] == entry->name) {
						watched = true;
						break;
					}
				}
			}

			printText(name, 1, 1, !(startRow+i == selection) + (watched*2), 3, i * 16 + 17);
		}
	}
}

std::string browseForFile(const std::vector<std::string>& extensionList) {
	vramSetBankC(VRAM_C_SUB_BG);

	int pressed = 0, held = 0, screenOffset = 0, fileOffset = 0;
	touchPosition touch;
	std::vector<DirEntry> dirContents;
	std::vector<std::string> watchedList = watchedListGet();

	drawImage(0, 0, 256, 192, fileBrowseBgBitmap, 16);

	getDirectoryContents(dirContents, extensionList);
	showDirectoryContents(dirContents, watchedList, fileOffset, screenOffset);

	while(1) {
		// Power saving loop. Only poll the keys once per frame and sleep the CPU if there is nothing else to do
		do {
			swiWaitForVBlank();
			scanKeys();
			pressed = keysDown();
			held = keysDownRepeat();
		} while(!((held & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT) || (pressed & (KEY_A | KEY_B | KEY_Y | KEY_TOUCH)))));

		if(held & KEY_UP) {
			fileOffset -= 1;
			if(fileOffset < 0)	fileOffset = dirContents.size()-1;
		} else if(held & KEY_DOWN) {
			fileOffset += 1;
			if(fileOffset > (int)dirContents.size()-1)	fileOffset = 0;
		} else if(held & KEY_LEFT) {
			fileOffset -= ENTRY_PAGE_LENGTH;
			if(fileOffset < 0)	fileOffset = 0;
		} else if(held & KEY_RIGHT) {
			fileOffset += ENTRY_PAGE_LENGTH;
			if(fileOffset > (int)dirContents.size()-1)	fileOffset = dirContents.size()-1;
		} else if(pressed & KEY_A) {
			selection:
			DirEntry* entry = &dirContents.at(fileOffset);
			if(entry->isDirectory) {
				// Enter selected directory
				chdir(entry->name.c_str());
				getDirectoryContents(dirContents, extensionList);
				screenOffset = 0;
				fileOffset = 0;
				watchedList = watchedListGet();
			} else if(!entry->isDirectory) {
				// Sound::play(Sound::click);
				// Return the chosen file
				return entry->name;
			}
		} else if(pressed & KEY_B) {
			// Go up a directory
			if(!((strcmp (path, "sd:/") == 0) || (strcmp (path, "fat:/") == 0))) {
				chdir("..");
				getDirectoryContents(dirContents, extensionList);
				screenOffset = 0;
				fileOffset = 0;
				watchedList = watchedListGet();
			}
		} else if(pressed & KEY_Y) {
			DirEntry* entry = &dirContents.at(fileOffset);
			if(!entry->isDirectory) {
				bool onWatchList = false;
				for(unsigned int i=0;i<watchedList.size();i++) {
					if(watchedList[i] == entry->name) {
						onWatchList = true;
						break;
					}
				}
				if(onWatchList)	watchedListRemove(watchedList, entry->name);
				else	watchedListAdd(watchedList, entry->name);
				watchedListSave(watchedList);
			}
		} else if(pressed & KEY_TOUCH) {
			touchRead(&touch);
			for(int i=0;i<std::min(ENTRIES_PER_SCREEN, (int)dirContents.size());i++) {
				if(touch.py > (i+1)*16 && touch.py < (i+2)*16) {
					fileOffset = screenOffset + i;
					goto selection;
				}
			}
		}

		// Scroll screen if needed
		if(fileOffset < screenOffset) {
			screenOffset = fileOffset;
		} else if(fileOffset > screenOffset + ENTRIES_PER_SCREEN - 1) {
			screenOffset = fileOffset - ENTRIES_PER_SCREEN + 1;
		}

		showDirectoryContents(dirContents, watchedList, fileOffset, screenOffset);
	}
}
