#include "../main.h"

/** EXPORT upkkRawCreateFile() :: UNPACK EXPORT FUNCTION!
 *
 * Creates a raw file, and recursively creates the filepath to it
 *
 *	filePath				= Output filename
 *	dwCreationDisposition	= An action to take on a file or device that exists or does not exist. (refer to MSDN:CreateFile)
 */
EXPORT HANDLE upkkRawCreateFile(char *filePath, DWORD dwCreationDisposition) {
	HANDLE			hFile;
	char			*strFilePath;

	/// If the dwCreationDisposition is CREATE_ALWAYS, we should also create the filepath (if any)
	if (dwCreationDisposition == CREATE_ALWAYS) {
		strFilePath = StrRChr(filePath, NULL, '\\');
		if (strFilePath != NULL) {
			*(char*)&strFilePath[0] = 0x00;
			recursiveCreateDirectory(filePath);
			*(char*)&strFilePath[0] = '\\';
		}
	}

	/// Open the requested file
	hFile = CreateFile(filePath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		log("Cannot create %s\n", filePath);
		hFile = NULL;
	}

	return hFile;
}
