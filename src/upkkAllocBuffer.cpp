#include "../main.h"

/** EXPORT upkkAllocBuffer() :: UNPACK EXPORT FUNCTION!
 *
 * Virtual alloc buffer
 */
EXPORT byte* upkkAllocBuffer(DWORD bufferSize) {
	byte	*buffer;

	buffer = (byte*)VirtualAlloc(NULL, bufferSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	if (buffer == NULL) {
		log("ERROR@upkkAllocBuffer(S:%08X;E:%08X)", bufferSize, GetLastError());
	}

	return buffer;
}
