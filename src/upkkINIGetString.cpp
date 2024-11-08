#include "../main.h"

/** EXPORT upkkINIGetString() :: Get string value from INI file
 *
 * Get string from unpakke.ini
 *
 * lpKeyName		= Key name
 * lpDefault		= Default value
 * lpReturnedString	= Return value
 * nSize			= Maximum size of return value
 */
EXPORT DWORD upkkINIGetString(char *lpKeyName, char *lpDefault, char *lpReturnedString, DWORD nSize) {
	DWORD	result;
	char	*szFilepathINI;

	szFilepathINI = (char*)upkkAllocBuffer((MAX_PATH*2)+1);
	GetModuleFileName(NULL, szFilepathINI, MAX_PATH);
	wsprintf(StrRChr(szFilepathINI, NULL, '\\'), "\\%s", szINIFilename);

	result = GetPrivateProfileString(mi.codename, lpKeyName, lpDefault, (LPSTR)lpReturnedString, nSize, szFilepathINI);

	upkkReleaseBuffer((byte*)szFilepathINI);

	return result;
}
