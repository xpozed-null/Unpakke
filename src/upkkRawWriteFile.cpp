#include "../main.h"

/** EXPORT upkkRawWriteFile() :: EXPORT FUNCTION!
 *
 * Writes a buffer to a file and returns number of written bytes
 *
 *	hFile	= HANDLE to previously opened file
 */
EXPORT DWORD upkkRawWriteFile(HANDLE hFile, byte *data, DWORD dataSize) {
	DWORD	lpNumberOfBytesWritten;

	if (hFile == NULL) {
		return -1;
	}

	if (data == NULL) {
		return -1;
	}

	if (dataSize == 0) {
		return 0;
	}

	lpNumberOfBytesWritten = 0;
	WriteFile(hFile, data, dataSize, &lpNumberOfBytesWritten, NULL);
	return lpNumberOfBytesWritten;
}
