#include "../main.h"

/** EXPORT upkkGetData() :: UNPACK EXPORT FUNCTION!
 *
 * Get data buffer from the archive
 *
 * !!!IMPORTANT!!!
 * Changing this function means you need to change upkkExtractFile too!
 *
 *	dataSize		= Size of the requested data. This parameter can be set to FILE_READ_FULL, to read the entire file.
 *	archiveOffset	= Offset of the requested data. If dwMoveMethod is FILE_RESTORE, this parameter is ignored and should be set to 0.
 *	dwMoveMethod	= Move method as in SetFilePointerEx. Might be one of these values:
 *					0x0000 = FILE_BEGIN;	- Set archiveOffset from the beginning of the file
 *					0x0001 = FILE_CURRENT;	- Set archiveOffset from the current position of the file
 *					0x0002 = FILE_END;		- Set archiveOffset from the end of the file
 *					0x00FF = FILE_IGNORE;	- Ignore archiveOffset
 *					with or without this:
 *					0x0100 = FILE_RESTORE;	- Restore file pointer
 *	containerHandle	= Handle of the storage file. For single archive files, this value should be set to NULL
 */
EXPORT byte* upkkGetData(DWORD dataSize, DWORD64 archiveOffset, DWORD dwMoveMethod, HANDLE containerHandle) {
	byte			*dataBuffer;
//	DWORD64			currentOffset;
	DWORD			lpNumberOfBytesRW;
	HANDLE			baseHandle;
	LARGE_INTEGER	fileOffset;
	LARGE_INTEGER	fileOffsetResult;

	/// Base handle is used, when we are working with multipart archives.
	baseHandle = containerHandle;
	if (baseHandle == NULL) {
		baseHandle = dArchive.archiveHandle;
	}

	/// If the data size is 0, return NULL
	if (dataSize == 0)  {
		return NULL;
	}

	/// If the data size is FILE_READ_FULL, read the entire file
	if (dataSize == FILE_READ_FULL) {
		dataSize = GetFileSize(baseHandle, NULL);
	}

	dataBuffer = upkkAllocBuffer(dataSize);

	/// Get the current offset, so we can revert it after ReadFile
	if ((dwMoveMethod&FILE_RESTORE) == FILE_RESTORE) {
		fileOffset.QuadPart = 0;
		SetFilePointerEx(baseHandle, fileOffset, &fileOffsetResult, FILE_CURRENT);
	}

	/// If the dwMoveMethod is FILE_IGNORE, ignore the offset parameter as well
	if ((dwMoveMethod&FILE_IGNORE) != FILE_IGNORE) {
		fileOffset.QuadPart = archiveOffset;
		SetFilePointerEx(baseHandle, fileOffset, NULL, (dwMoveMethod&0xFF));
	}

	/// Read the buffer
	ReadFile(baseHandle, dataBuffer, dataSize, &lpNumberOfBytesRW, NULL);

	/// Revert the file offset to where it was
	if ((dwMoveMethod&FILE_RESTORE) == FILE_RESTORE) {
		fileOffset.QuadPart = fileOffsetResult.QuadPart;
		SetFilePointerEx(baseHandle, fileOffset, NULL, FILE_BEGIN);
	}

	return dataBuffer;
}
