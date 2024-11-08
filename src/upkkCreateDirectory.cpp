#include "../main.h"

/** EXPORT upkkCreateDirectory() :: EXPORT FUNCTION!
 *
 * Recursively cteates directory
 *
 *	szDirName		= Directory to be created
 */
EXPORT BOOL upkkCreateDirectory(char *szDirName) {
	char	szDriveLetter[0x10];
	char	*szCurDir;

	/// Empty directories are just discarded
	if (lstrlen(szDirName) == 0) {
		return TRUE;
	}

	/// If directory name starts with drive letter
	/// move there and create it
	if (*(char*)&szDirName[1] == ':') {
		/// get the drive letter first
		lstrcpyn(szDriveLetter, szDirName, 4);

		/// Preserve the current working directory, so we can restore it later
		szCurDir = (char*)upkkAllocBuffer(strlen(szDirName)+1);
		GetCurrentDirectory(strlen(szDirName), szCurDir);

		SetCurrentDirectory(szDriveLetter);
		recursiveCreateDirectory(szDirName);
		SetCurrentDirectory(szCurDir);

		/// Free the current directory preservation buffer
		upkkReleaseBuffer((byte*)szCurDir);

		return TRUE;
	}
	return recursiveCreateDirectory(szDirName);
}
