#include "../main.h"

/** EXPORT upkkXORBuffer() :: EXPORT FUNCTION!
 *
 */
EXPORT void upkkXORBuffer(byte *dataBuffer, DWORD dataLength, byte *keyBuffer, DWORD keyLength) {
	DWORD i;
	for(i = 0; i < dataLength; i++) {
		dataBuffer[i] = *(byte*)&dataBuffer[i] ^ *(byte*)&keyBuffer[i%keyLength];
	}
}
