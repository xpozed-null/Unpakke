#include "../main.h"

/** EXPORT upkkExtractData() :: UNPACK EXPORT FUNCTION!
 *
 * Drop buffer as a file. Useful for simple package types
 */
EXPORT BOOL upkkExtractData(char *filePath, byte *dataBuffer, DWORD dataLength) {
	HANDLE	hFile;
	DWORD	lpNumberOfBytesWritten;
	char	*filePathTemp;
	bool	result;

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
		WriteFile(hFile, dataBuffer, dataLength, &lpNumberOfBytesWritten, NULL);
		CloseHandle(hFile);
		result = true;
	}
//	upkkReleaseBuffer((byte*)filePathFull);	/// Free the filepath buffer, because we dont need it anymore
	return result;
}
