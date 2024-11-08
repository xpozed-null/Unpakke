#include "../main.h"

/** EXPORT upkkINIGetInt() :: Get integer value from INI file
 *
 * Get integer from unpakke.ini
 *
 * lpKeyName	= Key name
 * nDefault		= Default value
 */
EXPORT DWORD upkkINIGetInt(char *lpKeyName, DWORD nDefault) {
	DWORD	result;
	char	*szFilepathINI;

	szFilepathINI = (char*)upkkAllocBuffer((MAX_PATH*2)+2);
	GetModuleFileName(NULL, szFilepathINI, MAX_PATH);
	wsprintf(StrRChr(szFilepathINI, NULL, '\\'), "\\%s", szINIFilename);

	result = GetPrivateProfileInt(mi.codename, lpKeyName, nDefault, szFilepathINI);

	upkkReleaseBuffer((byte*)szFilepathINI);

	return result;
}
