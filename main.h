#ifndef __MAIN_H__
#define __MAIN_H__

#define _WIN32_IE 0x0700
#define _WIN32_WINNT 0x0601

#include <windows.h>
#include <stdio.h>
#include <shlobj.h>
#include <commctrl.h>
#include <commoncontrols.h>
#include <shlwapi.h>
#include <wininet.h>

#pragma pack(1)					/// Align structures to 1 byte

#define EXPORT __declspec(dllexport)

/// Patches and h4x
#ifndef PBS_MARQUEE
#define PBS_MARQUEE				0x08
#endif // PBS_MARQUEE

#ifndef PBM_SETMARQUEE
#define PBM_SETMARQUEE          (WM_USER+10)
#endif // PBM_SETMARQUEE

/** Few words about versioning: 0x12345678
 *    0x12 - Major version
 *    0x34 - Minor version
 *  0x5678 - Build
 */
#define UNPAKKE_VERSION			0x010203F0

#define IDI_MAIN				1
#define IDI_SMALL				2
#define IDB_BITMAP				3
#define IDR_CONTEXT				4
#define ID_COPYLINES			5
#define ID_COPYLOG				6
#define ID_CLEARLOG				7

// GUI Tabs
#define IDD_TAB_PACK			1000	// Pack
#define IDD_TAB_UNPACK			2000	// Unpack
#define IDD_TAB_INI				3000	// INI editor
#define IDD_TAB_ABOUT			4000	// About

#define IDC_MODULE_LIST			100		// Module list dropdown
#define IDB_MODULE_LIST			110		// Module list reload button
#define IDC_TABS				120		// Tabs
#define IDC_LOGBOX				200		// Log window
#define IDC_FILETREE			300		// The file tree control

// Pack controls
#define IDC_PACK_INPUT_DIR		1100	// Input dir textbox
#define IDB_PACK_INPUT_DIR		1200	// Input dir button
#define IDC_PACK_OUTPUT_FILE	1300	// Output file textbox
#define IDB_PACK_OUTPUT_FILE	1400	// Output file button
#define IDB_PACK				1500	// RUN PACK Button

// Unpack controls
#define IDC_UNPACK_INPUT_FILE	2100	// Input file textbox
#define IDB_UNPACK_INPUT_FILE	2200	// Input file button
#define IDC_UNPACK_OUTPUT_DIR	2300	// Output dir textbox
#define IDB_UNPACK_OUTPUT_DIR	2400	// Output dir button
#define IDB_UNPACK				2500	// RUN UNPACK Button

// INI Editor controls
#define IDC_INI_TEXT			3100	// File contents
#define IDC_INI_LOAD			3200	// NOT USED!
#define IDC_INI_SAVE			3300	// Save button
#define IDC_INI_SEARCH			3400	// Search bar
#define IDC_INI_FIND			3500	// Search button

// About controls
#define IDC_ABOUT_UPDATE		4100	// Check for updates button
#define IDC_ABOUT_ICON			4200	// Check for updates button

// File pointer move methods
//#define FILE_BEGIN			0x00000000	// defined in windows.h
//#define FILE_CURRENT			0x00000001	// defined in windows.h
//#define FILE_END				0x00000002	// defined in windows.h
#define FILE_IGNORE				0x00000100
#define FILE_RESTORE			0x00001000

// File sizing options
#define FILE_READ_FULL			0xFFFFFFFF

// Big files should be extracted in parts.
// Allocating a gigabyte of memory is just retarded (not to mention impossible in most of the cases)
#define FILE_CHUNK_SIZE			0x00A00000		// 10mb chunk sizes

// Compression types
#define COMPRESS_ZLIB			10001000		// ZLIB compression algorithm
#define COMPRESS_DEFLATE		COMPRESS_ZLIB	// same as above
#define COMPRESS_LZF			10002000		// LZF Compression algorithm

/** Archive parameters
 *
 *  required by Unpakke
 */
typedef struct dataArchive {
	char	*inputPath;					/// Packing=Directory path; Unpacking=Archive filepath
	char	*outputPath;				/// Packing=Archive filepath; Unpacking=Directory path
	int		numEntries;					/// Number of entries
	DWORD64	archiveSize;				/// Size of the archive
	HANDLE	archiveHandle;				/// file HANDLE of the archive
	byte	*buffer;					/// Packing=raw structure; Unpacking=user's decision
} dataArchive;

/** Archive structure row
 *
 *  required by Unpakke
 */
typedef struct archiveRow {
	char		filePath[MAX_PATH+1];	/// Full, relative (eg, starts from input directory) filepath
	DWORD64		fileSizeRaw;			/// Raw size of the file
	DWORD64		fileSizeZip;			/// Compressed/Encrypted size of the file (if any compression/encryption is applied)
	DWORD64		fileOffset;				/// Relative file offset (first entry is 0, second equals the first entry's size, and so on...)
	DWORD		timestamp;				/// Unix timestamp
	FILETIME	time;					/// Windows FILETIME time
} archiveRow;

/** Module info structure
 *
 *  required by Unpakke
 */
typedef struct moduleInfo {
	char	codename[20];				/// Module name used as INI section name
	char	author[70];					/// Module author
	char	description[200];			/// Module description (if any)
	char	date[15];					/// Module release date in "dd Mon, YYYY" format!
	char	version[4];					/// Module version, not used, but it's good to have this one
	WORD	min_version;				/// Minimum Unpakke supported version in 0xAABB format (AA = Major version; BB Minor version)
	BOOL	packing;					/// Packing capability (TRUE - packing available; FALSE - packing unavailable)
	BOOL	unpacking;					/// Unpacking capability (TRUE - unpacking available; FALSE - unpacking unavailable)
} moduleInfo;

/** PACK/UNPACK thread parameters
 *
 */
typedef struct MainThreadParams {
	char pathModule[MAX_PATH+1];
	char pathInput[MAX_PATH+1];
	char pathOutput[MAX_PATH+1];
	char method[MAX_PATH+1];
} MainThreadParams;

/** Export functions from the modules
 *
 * Init		- to get the module info structure
 * Pack		- to pack a resource
 * Unpack	- to unpack a resource
 */
typedef struct moduleInfo (*execInit)();
typedef BOOL (*execPack)(dataArchive *data);
typedef BOOL (*execUnpack)(dataArchive *data);
typedef struct moduleProc {
	execInit		init;
	execPack		pack;
	execUnpack		unpack;
} moduleProc;

/** Additional library related functions
 *
 */

/// ZLIB :: BEGIN
typedef DWORD (*hZlibCompress2)(byte *dest, unsigned long *destLen, byte *source, unsigned long sourceLen, int level);	/// ZLIB compress
typedef DWORD (*hZlibCompressBound)(DWORD);																				/// ZLIB compress bound (calculate uncompressed size)
typedef DWORD (*hZlibUncompress)(byte *dest, unsigned long *destLen, byte *source, unsigned long sourceLen);			/// ZLIB decompress

/// LZF :: BEGIN
typedef DWORD (*hLzfCompress)(byte *dest, unsigned long *destLen, byte *source, unsigned long sourceLen, int level);	/// LZFLIB compress
typedef DWORD (*hLzfUncompress)(byte *dest, unsigned long *destLen, byte *source, unsigned long sourceLen);				/// LZFLIB decompress

/// Internal functions declaration goes here
void progressSet(int state);
void reloadModulesList();
BOOL moduleLoad(LPCSTR moduleName);
void moduleUnload();
//void log(const char *fmt, ...); // moved to EXPORTS so the modules could use it
void logClear();
void logCopy(BOOL all);
HWND addControl(LPCTSTR lpClassName, LPCTSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu);
void loadINI();
void saveINI();
BOOL unpakkePack();
BOOL unpakkeUnpack();
int enumDirectory(LPCTSTR baseDir, int i);
void libsLoad();
void libsUnload();
BOOL recursiveCreateDirectory(LPCTSTR inputFilepath);
LRESULT CALLBACK TabProcedure(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProcedure(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
int getIconIndex(char *strPath, DWORD fileAttrib);
void reloadTreeView();

#ifdef __cplusplus
extern "C" {
#endif

	/** Shared functions
	 *
	 */
	EXPORT void log(const char *fmt, ...);
	EXPORT byte* upkkAllocBuffer(DWORD bufferSize);
	EXPORT BOOL upkkReleaseBuffer(byte *dataBuffer);
	EXPORT DWORD upkkCountFiles(char *baseDir, BOOL recursive);
	EXPORT void upkkXORBuffer(byte *dataBuffer, DWORD dataLength, byte *keyBuffer, DWORD keyLength);
	EXPORT DWORD upkkAdler32(byte *dataBuffer, DWORD bufferSize);
	EXPORT DWORD upkkCRC32(byte *dataBuffer, DWORD bufferSize);
	EXPORT DWORD upkkINIGetString(char *lpKeyName, char *lpDefault, char *lpReturnedString, DWORD nSize);
	EXPORT DWORD upkkINIGetInt(char *lpKeyName, DWORD nDefault);
	EXPORT void upkkRawCloseFile(HANDLE hFile);
	EXPORT HANDLE upkkRawCreateFile(char *filePath, DWORD dwCreationDisposition);
	EXPORT DWORD upkkRawReadFile(HANDLE hFile, byte *data, DWORD dataSize);
	EXPORT DWORD upkkRawWriteFile(HANDLE hFile, byte *data, DWORD dataSize);
	EXPORT BOOL upkkCreateDirectory(char *szDirName);

	/** Compress/Decompress
	 *
	 */
	EXPORT byte* upkkCompressBuffer(DWORD compAlgo, int compLevel, byte *inputBuffer, DWORD inputBufferSize, DWORD *outputBufferSize);
	EXPORT byte* upkkDecompressBuffer(DWORD compAlgo, byte *inputBuffer, DWORD inputBufferSize, DWORD *outputBufferSize);

	/** Pack functions
	 *
	 */
	EXPORT BOOL upkkInsertFile(char *filePath, DWORD64 archiveOffset, DWORD dwMoveMethod, HANDLE containerHandle);
	EXPORT BOOL upkkInsertData(byte *data, DWORD dataSize, DWORD64 archiveOffset, DWORD dwMoveMethod, HANDLE containerHandle);
	EXPORT byte* upkkReadFile(char *filePath, DWORD *dataSize);

	/** Unpack functions
	 *
	 */
	EXPORT BOOL upkkExtractData(char *filePath, byte *dataBuffer, DWORD dataLength);
	EXPORT BOOL upkkExtractFile(char *filePath, DWORD dataSize, DWORD64 archiveOffset, DWORD dwMoveMethod, HANDLE containerHandle);
	EXPORT byte* upkkGetData(DWORD dataSize, DWORD64 archiveOffset, DWORD dwMoveMethod, HANDLE containerHandle);
	EXPORT DWORD64 upkkGetOffset(HANDLE containerHandle);
	EXPORT HANDLE upkkOpenStorage(char *lpFileName, DWORD dwCreationDisposition);
	EXPORT BOOL upkkCloseStorage(HANDLE containerHandle);

#ifdef __cplusplus
}
#endif

/// Global functions
extern moduleInfo			mi;
extern dataArchive			dArchive;
extern archiveRow			aRow;
extern HINSTANCE			hInst;
extern HWND					hwndModuleList;					// Main controls HWNDs
extern HWND					hwndLogBox;						//
extern HWND					hwndTree;						//
extern HWND					hwndTab; 						// Tabs HWNDs
extern HWND					hwndTabPack; 					//
extern HWND					hwndTabUnpack;					//
extern HWND					hwndTabINI; 					//
extern HWND					hwndTabAbout;					//
extern HWND					hwndDlg;						// Main dialog handle
extern char					baseAppDirectory[MAX_PATH+1];	// Base dir of Unpakke.exe
extern HMODULE				hwndModule;
extern TV_INSERTSTRUCT		tvIns;
extern BOOL					outputToConsole;
extern moduleProc			module;
extern const char			*szINIFilename;
extern HIMAGELIST			phimlSmall;						// Image list for the Tree view

/// Global functions related to Zlib
extern HMODULE				hCompZLIB;
extern hZlibCompress2		zlibCompress2;
extern hZlibCompressBound	zlibCompressBound;
extern hZlibUncompress		zlibUncompress;

/// Global functions related to Zlib
extern HMODULE				hCompLZF;
extern hLzfCompress			lzfCompress;
extern hLzfUncompress		lzfUncompress;

#endif
