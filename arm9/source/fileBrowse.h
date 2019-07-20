#ifndef FILE_BROWSE_H
#define FILE_BROWSE_H

#include <string>

struct DirEntry {
	std::string name;
	bool isDirectory;
};

std::string browseForFile(void);

#endif //FILE_BROWSE_H
