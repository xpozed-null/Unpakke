#include "../main.h"

/** EXPORT upkkOpenStorage() :: Open additional file
 *
 * Basically, it's an open file implementation
 *
 * lpFileName	= File name of the Storage file
 */
EXPORT HANDLE upkkOpenStorage(char *lpFileName, DWORD dwCreationDisposition) {
	HANDLE	hFile;

	/// Open the requested file
	hFile = CreateFile(lpFileName, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		log("Cannot open container file %s\n", lpFileName);
		return NULL;
	}
	return hFile;
}
