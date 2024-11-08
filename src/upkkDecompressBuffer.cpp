#include "../main.h"

/** EXPORT upkkDecompressBuffer() :: DECOMPRESS EXPORT FUNCTION!
 *
 * decompress buffer
 *
 * compAlgo			= (COMPRESS_DEFLATE|COMPRESS_LZF)
 * inputBuffer		= Buffer you like to be decompressed
 * inputBufferSize	= Size of the buffer you going to decompress
 * outputBufferSize	= Size of the output buffer
 */
EXPORT byte* upkkDecompressBuffer(DWORD compAlgo, byte *inputBuffer, DWORD inputBufferSize, DWORD *outputBufferSize) {
	byte *buffCompress = NULL;

	buffCompress = upkkAllocBuffer(*(DWORD*)outputBufferSize);

	switch(compAlgo) {
		case COMPRESS_ZLIB: {
			if (zlibUncompress != NULL) {
				zlibUncompress(buffCompress, outputBufferSize, inputBuffer, inputBufferSize);
			}
		} break;
		case COMPRESS_LZF: {
			if (lzfUncompress != NULL) {
				*outputBufferSize = lzfUncompress(inputBuffer, (long unsigned int*)inputBufferSize, buffCompress, (unsigned int)outputBufferSize);
			}
		} break;
		default: {
			/// Don't do anything, OOOOOR, return the same buffer maybe?
		} break;
	}
	return buffCompress;
}
