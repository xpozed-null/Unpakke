#include "../main.h"

/** EXPORT upkkRawCloseFile() :: UNPACK EXPORT FUNCTION!
 *
 * Closes a file handle, previously opened by upkkCreateFile()
 *
 *	hFile	= HANDLE to previously opened file
 */
EXPORT void upkkRawCloseFile(HANDLE hFile) {
	if (hFile != NULL) {
		CloseHandle(hFile);
	}
}
