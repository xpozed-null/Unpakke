#include "../main.h"

/** EXPORT upkkAdler32() :: Adler-32 CRC
 *
 * Calculates the Adler-32 checksum of a given buffer
 *
 * dataBuffer		= Buffer you like to calculate
 * bufferSize		= Size of the buffer
 */
EXPORT DWORD upkkAdler32(byte *dataBuffer, DWORD bufferSize) {
	unsigned int a, b, i;
	a = 1;
	b = 0;
	for (i = 0; i < bufferSize; ++i) {
		a = (a + dataBuffer[i]) % 65521;
		b = (b + a) % 65521;
    }
	return (b << 16) | a;
}
