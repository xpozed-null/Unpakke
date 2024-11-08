#include "../main.h"

/** EXPORT upkkCountFiles() :: EXPORT FUNCTION!
 *
 */
EXPORT DWORD upkkCountFiles(char *baseDir, BOOL recursive) {
	WIN32_FIND_DATA		wfd;
	HANDLE				hFind;
	char				*searchDir;
	DWORD				fileCount;
	HTREEITEM			temp = NULL;

	searchDir = (char*)upkkAllocBuffer(lstrlen(baseDir)+10+MAX_PATH);
	wsprintf(searchDir, "%s\\*", baseDir);

	fileCount = 0;

	hFind = FindFirstFile(searchDir, &wfd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if ((lstrcmp(wfd.cFileName, ".") != 0) && (lstrcmp(wfd.cFileName, "..") != 0)) {
				/// GUI ONLY
				if (outputToConsole == FALSE) {
					tvIns.item.pszText = (char*)wfd.cFileName;
					tvIns.item.iImage = tvIns.item.iSelectedImage = getIconIndex(wfd.cFileName, wfd.dwFileAttributes);
					temp = tvIns.hParent;
					tvIns.hParent = (HTREEITEM)SendMessage(hwndTree, TVM_INSERTITEM, 0, (LPARAM)&tvIns);
				}
				if ((wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
					if (recursive == TRUE) {
						wsprintf(searchDir, "%s\\%s", baseDir, wfd.cFileName);
						fileCount += upkkCountFiles(searchDir, recursive);
					}
				} else {
					fileCount++;
				}
				/// GUI ONLY
				if (outputToConsole == FALSE) {
					tvIns.hParent = temp;
				}
			}
		} while (FindNextFile(hFind, &wfd) != 0);
		FindClose(hFind);
	}
	upkkReleaseBuffer((byte*)searchDir);
	return fileCount;
}
