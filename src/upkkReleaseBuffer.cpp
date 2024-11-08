#include "../main.h"

/** EXPORT upkkReleaseBuffer() :: UNPACK EXPORT FUNCTION!
 *
 * Release a Virtually allocated buffer
 */
EXPORT BOOL upkkReleaseBuffer(byte *dataBuffer) {
	if (dataBuffer != NULL) {
		VirtualFree(dataBuffer, 0, MEM_RELEASE);
	}
	return TRUE;
}
