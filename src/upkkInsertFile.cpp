#include "../main.h"

/** EXPORT upkkInsertFile() :: PACK EXPORT FUNCTION!
 *
 * Read file and write it to the archive
 *
 *  filePath		= Filepath of the file you like to be inserted into the archive
 *  archiveOffset	= Offset inside the archive (if any). By default, leave that to 0
 *	dwMoveMethod	= Move method as in SetFilePointerEx. Might be one of these values:
 *					0x0000 = FILE_BEGIN;	- Set archiveOffset from the beginning of the file
 *					0x0001 = FILE_CURRENT;	- Set archiveOffset from the current position of the file
 *					0x0002 = FILE_END;		- Set archiveOffset from the end of the file
 *					0x00FF = FILE_IGNORE;	- Ignore archiveOffset
 *					with or without this:
 *					0x0100 = FILE_RESTORE;	- Restore file pointer
 *	containerHandle	= Handle of the storage file. For single archive files, this value should be set to NULL
 */
EXPORT BOOL upkkInsertFile(char *filePath, DWORD64 archiveOffset, DWORD dwMoveMethod, HANDLE containerHandle) {
	HANDLE			hFile;
	DWORD			lpNumberOfBytesRW,
					fileSize;
	byte 			*fileData;
	char			*filePathFull;
	HANDLE			baseHandle;
	LARGE_INTEGER	fileOffset;
	LARGE_INTEGER	fileOffsetResult;

	/// Base handle is used, when we are working with multipart archives.
	baseHandle = containerHandle;
	if (baseHandle == NULL) {
		baseHandle = dArchive.archiveHandle;
	}

	filePathFull = (char*)upkkAllocBuffer(lstrlen(dArchive.inputPath)+10+lstrlen(filePath));
	wsprintf(filePathFull, "%s\\%s", dArchive.inputPath, filePath);

	/// Open the requested file
	hFile = CreateFile(filePathFull, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	upkkReleaseBuffer((byte*)filePathFull);	/// Free the filepath buffer, because we dont need it anymore
	if (hFile == INVALID_HANDLE_VALUE) {
		log("Cannot read %s\n", filePath);
		return FALSE;
	}

	/// If the file is 0 bytes long, just return TRUE. Optimization, bitches!
	fileSize = GetFileSize(hFile, NULL);
	if (fileSize > 0) {
		/// Read the whole file into a buffer
		fileData = upkkAllocBuffer(fileSize);
		ReadFile(hFile, fileData, fileSize, &lpNumberOfBytesRW, NULL);

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
		WriteFile(baseHandle, fileData, fileSize, &lpNumberOfBytesRW, NULL);

		upkkReleaseBuffer(fileData);

		/// Revert the file offset to where it was
		if ((dwMoveMethod&FILE_RESTORE) == FILE_RESTORE) {
			fileOffset.QuadPart = fileOffsetResult.QuadPart;
			SetFilePointerEx(baseHandle, fileOffset, NULL, FILE_BEGIN);
		}

	}
	CloseHandle(hFile);
	return TRUE;
}
