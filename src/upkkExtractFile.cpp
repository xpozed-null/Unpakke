#include "../main.h"

/** EXPORT upkkExtractFile() :: UNPACK EXPORT FUNCTION!
 *
 * Extract a data from the archive and drop it like a file. Useful for simple package types
 *
 * !!!IMPORTANT!!!
 * Changing this function means you need to change upkkGetData too!
 *
 *	filePath		= Output filename
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
EXPORT BOOL upkkExtractFile(char *filePath, DWORD dataSize, DWORD64 archiveOffset, DWORD dwMoveMethod, HANDLE containerHandle) {
	HANDLE			hFile;
//	DWORD64			currentOffset;
	DWORD			lpNumberOfBytesRW,
					i;
//	char			*filePathFull;
	char			*filePathTemp;
	byte 			*fileData;
	bool			result = true;
	HANDLE			baseHandle;
	LARGE_INTEGER	fileOffset;
	LARGE_INTEGER	fileOffsetResult;

	/// Base handle is used, when we are working with multipart archives.
	baseHandle = containerHandle;
	if (baseHandle == NULL) {
		baseHandle = dArchive.archiveHandle;
	}

//	filePathFull = (char*)upkkAllocBuffer(lstrlen(dArchive.outputPath)+10+lstrlen(filePath));
//	wsprintf(filePathFull, "%s\\%s", dArchive.outputPath, filePath);
//	recursiveCreateDirectory(filePathFull);

	filePathTemp = StrRChr(filePath, NULL, '\\');
	if (filePathTemp != NULL) {
		*(byte*)&filePathTemp[0] = 0x00;
		recursiveCreateDirectory(filePath);
		*(byte*)&filePathTemp[0] = '\\';
	}

	/// Open the requested file
	hFile = CreateFile(filePath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		log("Cannot dump %s\n", filePath);
		result = false;
	} else {
		/// If the data is 0 bytes long, just return TRUE. Optimization, bitches!
		if (dataSize > 0) {

			/// If the data size is FILE_READ_FULL, read the entire file
			if (dataSize == FILE_READ_FULL) {
				dataSize = GetFileSize(baseHandle, NULL);
			}

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

			/// Allocate space for the chunk
			fileData = upkkAllocBuffer(FILE_CHUNK_SIZE);

			/// Read-Write the chunks to the destination
			for(i = 0; i < (dataSize/FILE_CHUNK_SIZE); i++) {
				ReadFile(baseHandle, fileData, FILE_CHUNK_SIZE, &lpNumberOfBytesRW, NULL);
				WriteFile(hFile, fileData, FILE_CHUNK_SIZE, &lpNumberOfBytesRW, NULL);
			}

			/// Read-Write the remaining part to the destination
			if ((dataSize%FILE_CHUNK_SIZE) > 0) {
				ReadFile(baseHandle, fileData, (dataSize%FILE_CHUNK_SIZE), &lpNumberOfBytesRW, NULL);
				WriteFile(hFile, fileData, (dataSize%FILE_CHUNK_SIZE), &lpNumberOfBytesRW, NULL);
			}

			/// Free the chunk space
			upkkReleaseBuffer(fileData);

			/// Revert the file offset to where it was
			if ((dwMoveMethod&FILE_RESTORE) == FILE_RESTORE) {
				fileOffset.QuadPart = fileOffsetResult.QuadPart;
				SetFilePointerEx(baseHandle, fileOffset, NULL, FILE_BEGIN);
			}
		}
		CloseHandle(hFile);
	}
//	upkkReleaseBuffer((byte*)filePathFull);	/// Free the filepath buffer, because we dont need it anymore

	return result;
}
