#include "../main.h"

/** EXPORT upkkCloseStorage() :: Close file
 *
 * Basically, it's an close file implementation
 *
 * lpFileName	= File name of the Storage file
 */
EXPORT BOOL upkkCloseStorage(HANDLE containerHandle) {
	if (containerHandle != NULL) {
		return CloseHandle(containerHandle);
	}
	return FALSE;
}
