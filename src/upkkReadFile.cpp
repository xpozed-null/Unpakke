#include "../main.h"

/** EXPORT upkkReadFile() :: EXPORT FUNCTION!
 *
 * Read an entire file
 *
 *  filePath		= Filepath of the file you like to read
 *  dataSize		= File size is returned in here
 */
EXPORT byte* upkkReadFile(char *filePath, DWORD *dataSize) {
	HANDLE	hFile;
	DWORD	fileSize;
	byte	*result;

	/// Zero the initial data size
	*dataSize = 0;

	hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		log("ERROR at upkkReadFile(%s): 0x%08X", filePath, GetLastError());
		return NULL;
	}

	fileSize = GetFileSize(hFile, NULL);
	result = upkkAllocBuffer(fileSize);
	ReadFile(hFile, result, fileSize, &*dataSize, NULL);
	CloseHandle(hFile);
	if (fileSize != *dataSize) {
		log("ERROR at upkkReadFile(%s): 0x%08X", filePath, GetLastError());
		log("file size: %d bytes, but %d bytes read instead", fileSize, *dataSize);
		upkkReleaseBuffer(result);
		*dataSize = 0;
		return NULL;
	}
	return result;
}
