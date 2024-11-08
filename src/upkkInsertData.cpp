#include "../main.h"

/** EXPORT upkkInsertData() :: PACK EXPORT FUNCTION!
 *
 * Basically, the same as upkkInsertFile. This one uses a buffer instead of a file
 *
 *  data			= Data you like to be inserted into the archive
 *  dataSize		= Size of the data for insertion
 *  archiveOffset	= Offset inside the archive (if any).
 *	dwMoveMethod	= Move method as in SetFilePointerEx. Might be one of these values:
 *					0x0000 = FILE_BEGIN;	- Set archiveOffset from the beginning of the file
 *					0x0001 = FILE_CURRENT;	- Set archiveOffset from the current position of the file
 *					0x0002 = FILE_END;		- Set archiveOffset from the end of the file
 *					0x00FF = FILE_IGNORE;	- Ignore archiveOffset
 *					with or without this:
 *					0x0100 = FILE_RESTORE;	- Restore file pointer
 *	containerHandle	= Handle of the storage file. For single archive files, this value should be set to NULL
 */
EXPORT BOOL upkkInsertData(byte *data, DWORD dataSize, DWORD64 archiveOffset, DWORD dwMoveMethod, HANDLE containerHandle) {
	DWORD 			lpNumberOfBytesRW;
	HANDLE			baseHandle;
	LARGE_INTEGER	fileOffset;
	LARGE_INTEGER	fileOffsetResult;

	/// Base handle is used, when we are working with multipart archives.
	baseHandle = containerHandle;
	if (baseHandle == NULL) {
		baseHandle = dArchive.archiveHandle;
	}

	/// If the data size is 0, ignore the rest of the code to optimize the execution a bit
	if (dataSize > 0) {

		/// Get the current offset, so we can revert it after WriteFile
		if ((dwMoveMethod&FILE_RESTORE) == FILE_RESTORE) {
			fileOffset.QuadPart = 0;
			SetFilePointerEx(baseHandle, fileOffset, &fileOffsetResult, FILE_CURRENT);
		}

		/// If the dwMoveMethod is FILE_IGNORE, ignore the offset parameter as well
		if ((dwMoveMethod&FILE_IGNORE) != FILE_IGNORE) {
			fileOffset.QuadPart = archiveOffset;
			SetFilePointerEx(baseHandle, fileOffset, NULL, (dwMoveMethod&0xFF));
		}

		/// Write data to archive
		WriteFile(baseHandle, data, dataSize, &lpNumberOfBytesRW, NULL);

		/// Revert the file offset to where it was
		if ((dwMoveMethod&FILE_RESTORE) == FILE_RESTORE) {
			fileOffset.QuadPart = fileOffsetResult.QuadPart;
			SetFilePointerEx(baseHandle, fileOffset, NULL, FILE_BEGIN);
		}

	}
	return TRUE;
}
