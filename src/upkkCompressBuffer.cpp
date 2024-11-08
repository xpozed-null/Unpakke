#include "../main.h"

/** EXPORT upkkCompressBuffer() :: COMPRESS EXPORT FUNCTION!
 *
 * compress buffer
 *
 * compAlgo			= (COMPRESS_DEFLATE|COMPRESS_LZF)
 * compLevel		= depending on the compression algorythm!
 * inputBuffer		= Buffer you like to be compressed
 * inputBufferSize	= Size of the buffer you going to compress
 * outputBufferSize	= Size of the output buffer
 */
EXPORT byte* upkkCompressBuffer(DWORD compAlgo, int compLevel, byte *inputBuffer, DWORD inputBufferSize, DWORD *outputBufferSize) {
	byte *buffCompress = NULL;

	switch(compAlgo) {
		case COMPRESS_ZLIB: {
			if ((zlibCompress2 != NULL) && (zlibCompressBound != NULL)) {
				*outputBufferSize = zlibCompressBound(inputBufferSize);
				buffCompress = upkkAllocBuffer(*(DWORD*)outputBufferSize);
				zlibCompress2(buffCompress, outputBufferSize, inputBuffer, inputBufferSize, compLevel);
			}
		} break;
		case COMPRESS_LZF: {
			if (lzfCompress != NULL) {
				buffCompress = upkkAllocBuffer(*(DWORD*)outputBufferSize);
				*outputBufferSize = lzfCompress(inputBuffer, (long unsigned int*)inputBufferSize, buffCompress, (unsigned int)outputBufferSize, compLevel);
			}
		} break;
		default: {
			/// Don't do anything, OOOOOR, return the same buffer maybe?
		} break;
	}
	return buffCompress;
}
