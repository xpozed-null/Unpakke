#include "../main.h"

/** EXPORT upkkRawReadFile() :: EXPORT FUNCTION!
 *
 * Reads a buffer from file and returns the readen bytes
 *
 *	hFile	= HANDLE to previously opened file
 */
EXPORT DWORD upkkRawReadFile(HANDLE hFile, byte *data, DWORD dataSize) {
	DWORD	lpNumberOfBytesRead;

	if (hFile == NULL) {
		return -1;
	}

	if (data == NULL) {
		return -1;
	}

	if (dataSize == 0) {
		return 0;
	}

	lpNumberOfBytesRead = 0;
	ReadFile(hFile, data, dataSize, &lpNumberOfBytesRead, NULL);
	return lpNumberOfBytesRead;
}
