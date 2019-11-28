#ifndef FILE_BROWSE_H
#define FILE_BROWSE_H

#include <string>
#include <vector>

struct DirEntry {
	std::string name;
	bool isDirectory;
};

std::vector<std::string> watchedListGet(void);
void watchedListAdd(std::vector<std::string> &watchedList, std::string item);
void watchedListRemove(std::vector<std::string> &watchedList, std::string item);
void watchedListSave(const std::vector<std::string> &watchedList);

std::string browseForFile(const std::vector<std::string>& extensionList);

#endif //FILE_BROWSE_H
