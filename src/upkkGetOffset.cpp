#include "../main.h"

/** EXPORT upkkGetOffset() :: UNPACK EXPORT FUNCTION!
 *
 * Get current offset from the archive
 *
 *	containerHandle	= Handle of the storage file. For single archive files, this value should be set to NULL
 */
EXPORT DWORD64 upkkGetOffset(HANDLE containerHandle) {
	HANDLE			baseHandle;
	LARGE_INTEGER	fileOffset;
	LARGE_INTEGER	fileOffsetResult;

	/// Base handle is used, when we are working with multipart archives.
	baseHandle = containerHandle;
	if (baseHandle == NULL) {
		baseHandle = dArchive.archiveHandle;
	}

	fileOffset.QuadPart = 0;
	SetFilePointerEx(baseHandle, fileOffset, &fileOffsetResult, FILE_CURRENT);

	return fileOffsetResult.QuadPart;
}
