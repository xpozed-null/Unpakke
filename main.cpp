#include "main.h"

/// Global functions
moduleInfo			mi;
dataArchive			dArchive;
archiveRow			aRow;
HINSTANCE			hInst;
HWND				hwndModuleList;						// Main controls HWNDs
HWND				hwndLogBox;							//
HWND				hwndTree;							//
HWND				hwndTab; 							// Tabs HWNDs
HWND				hwndTabPack; 						//
HWND				hwndTabUnpack;						//
HWND				hwndTabINI; 						//
HWND				hwndTabAbout;						//
HWND				hwndDlg;							// Main dialog handle
char				baseAppDirectory[MAX_PATH+1];		// Base dir of Unpakke.exe
HMODULE				hwndModule = NULL;
TV_INSERTSTRUCT		tvIns;
BOOL				outputToConsole;
moduleProc			module = {NULL, NULL, NULL};
const char			*szINIFilename = "unpakke.ini";
HIMAGELIST			phimlSmall;							// Image list for the Tree view
FLOAT				dpiX, dpiY;

/// Global functions related to Zlib
HMODULE				hCompZLIB;
hZlibCompress2		zlibCompress2;
hZlibCompressBound	zlibCompressBound;
hZlibUncompress		zlibUncompress;

/// Global functions related to Lzflib
HMODULE				hCompLZF;
hLzfCompress		lzfCompress;
hLzfUncompress		lzfUncompress;

/// Local-global functions, if such thing ever existed...
HWND				hwndProgressBar;
HANDLE				hwndAboutIcon;
MainThreadParams	p;
LVITEM				lv;
const char			*szClassName = "UNPAKKE_MAIN";
const char			*szClassTabButtons = "UNPAKKE_TABS";
char				dirModules[] = "modules";										// Installed modules directory
char				dirLibraries[] = "libs";										// Additional libraries directory (zlib, gz, bz2, encryption, etc.)
char				urlVersionCheck[] = "http://nullsecurity.org/unpakke/version";	// URL to unpakke's version checking page

/** progressSet()
 *
 */
void progressSet(int state) {
	if (outputToConsole == FALSE) {
		int	ctrlID[] = {IDC_TABS, IDC_MODULE_LIST, IDB_MODULE_LIST};
		int	i;

		/// Setup the progress bar
		SetWindowPos(hwndLogBox, NULL, 0, 0, 770, ((state==SW_SHOW)?100:115), SWP_NOMOVE);
		SendMessage(hwndProgressBar, PBM_SETMARQUEE, (BOOL)state, 10);
		ShowWindow(hwndProgressBar, state);

		/// Enable/Disable tabs and buttons, to the user cannot click on them while the un/packing is working
		for(i = 0; i < (int)(sizeof(ctrlID)/sizeof(int)); i++) {
			EnableWindow(GetDlgItem(hwndDlg, ctrlID[i]), ((state==SW_SHOW)?FALSE:TRUE));
		}
	}
}

/** reloadModulesList()
 *
 *  List the installed modules in the correct combo box
 */
void reloadModulesList() {
	WIN32_FIND_DATA		ffd;
	HANDLE				hFind;
	char				*searchDir;
	int					modulesCount = 0;

	/** Remove all entries from the combobox */
	SendMessage(hwndModuleList, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);

	searchDir = (char*)upkkAllocBuffer(lstrlen(baseAppDirectory)+lstrlen(dirModules)+20);
	wsprintf(searchDir, "%s\\%s\\*.umod", baseAppDirectory, dirModules);
	if ((hFind = FindFirstFile(searchDir, &ffd)) != INVALID_HANDLE_VALUE) {
		do {
			SendMessage(hwndModuleList, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)ffd.cFileName);
			modulesCount++;
		} while (FindNextFile(hFind, &ffd) != 0);
		FindClose(hFind);
	}
	/** Select the first element */
	SendMessage(hwndModuleList, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

	/// Release the temporary path buffer
	upkkReleaseBuffer((byte*)searchDir);

	log("Reloading modules, %d modules found", modulesCount);
}

/** moduleLoad()
 *  moduleName	= Name of the library to load
 *
 *  Load the module specified by the user
 */
BOOL moduleLoad(LPCSTR modulePath) {
	/** Extract only the module file name, for better messages */

	/** Unload any module that is already loaded - just in case... */
	moduleUnload();

	if ((hwndModule = LoadLibrary(modulePath)) != NULL) {
		module.init = (execInit)GetProcAddress(hwndModule, "init");
		module.pack = (execPack)GetProcAddress(hwndModule, "pack");
		module.unpack = (execUnpack)GetProcAddress(hwndModule, "unpack");
		if (module.init != NULL && module.pack != NULL && module.unpack != NULL) {
			/** Move the module info struct to the local one */
			mi = module.init();
			/** Verify the version */
			if (HIWORD(UNPAKKE_VERSION) < mi.min_version) {
				/** Seems the supported version of this module is above your Unpakke version, soohwyyy. */
				log("This version is for Unpakke v%d.%d, and yours seems to be older: v%d.%d(build %d) %08X",
					HIBYTE(mi.min_version), LOBYTE(mi.min_version),
					HIBYTE(HIWORD(UNPAKKE_VERSION)), LOBYTE(HIWORD(UNPAKKE_VERSION)),
					LOWORD(UNPAKKE_VERSION),
					mi.min_version
				);
			} else {
				/** Module seems to be 100% legit, lets continue */
				log("Module loaded successfully: %s", modulePath);
				log("Author: %s, %s", mi.author, mi.date);
				log("Module v%s; Minimum Unpakke v%d.%d", mi.version, HIBYTE(mi.min_version), LOBYTE(mi.min_version));
				log("Supported actions - Packing: %s; Unpacking: %s", (mi.packing?"YES":"NO"), (mi.unpacking?"YES":"NO"));

				if (lstrlen(mi.description) > 0) {
					log("%s", mi.description);
				}
				return TRUE;
			}
		} else {
			/** This module doesnt contain the mandatory export funtions*/
			log("This module is not supported: %s", modulePath);
		}
	} else {
		/** Module cannot be loaded for some reason. Bad path?*/
		log("Cannot load module: %s", modulePath);
	}
	moduleUnload();
	return FALSE;
}

/** moduleUnload()
 *
 *  Free the current used module library (if any!)
 */
void moduleUnload() {
	/// Check if there's a loaded lib already
	if (hwndModule != NULL) {
		module = (moduleProc){NULL, NULL, NULL};
		FreeLibrary(hwndModule);
	}
}

/** log()
 *  fmt	= format string
 *  ... = format arguments
 *
 *  Prints a line in the log control
 */
void log(const char *fmt, ...) {
	char		line[MAX_PATH*2];
    va_list		args;
	SYSTEMTIME	st;
	char		szTimestamp[10];

    va_start(args, fmt);
	wvsprintf(line, fmt, args);
    va_end(args);

	/// CMD Unpakke uses the console
	if (outputToConsole) {
		printf("%s\n", line);
	/// GUI Unpakke uses the log windows
	} else {
		/// Insert item
		ListView_InsertItem(hwndLogBox, &lv);

		/// set time
		GetSystemTime(&st);
		wsprintf(szTimestamp, "%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
		ListView_SetItemText(hwndLogBox, lv.iItem, 0, szTimestamp);

		/// set message
		ListView_SetItemText(hwndLogBox, lv.iItem, 1, (char*)line);

		/// scroll to bottom
		ListView_EnsureVisible(hwndLogBox, lv.iItem, TRUE);

		/// Iterate for the next item
		lv.iItem++;
	}
	/// Output Debug String
	/// OutputDebugString(line);
}

/** logClear()
 *
 * Clears the entire log data
 */
void logClear() {
	ListView_DeleteAllItems(hwndLogBox);
	lv.iItem = 0;
}

/** logCopy()
 * all = copy whole log or just selected lines
 *
 * Copy selected parts or the entire logged data
 */
void logCopy(BOOL all) {
	LV_ITEM	LvItem;
	char	*tText;
	char	*clipboardData;
	int		i, clipboardSize, filter;

	if (all == TRUE) {
		filter = LVNI_ALL;
	} else {
		filter = LVNI_SELECTED;
	}

	tText = (char*)upkkAllocBuffer(2000+1);
	clipboardSize = 0;
	if ((i = ListView_GetNextItem(hwndLogBox, -1, filter)) != - 1) {
		do {
			memset(&LvItem, 0, sizeof(LV_ITEM));
			LvItem.mask=LVIF_TEXT;
			LvItem.iSubItem=1;
			LvItem.pszText=tText;
			LvItem.cchTextMax=2000;
			LvItem.iItem=i;

			/// Calculates clipboard size like so:
			/// length of the text + length of timestamp + 4 bytes for '[', ']', ' ' and string terminator
			clipboardSize += SendMessage(hwndLogBox, LVM_GETITEMTEXT, i, (LPARAM)&LvItem) + 20;
		} while((i = ListView_GetNextItem(hwndLogBox, i, filter)) != -1);
	}

	if (clipboardSize > 0) {
		clipboardData = (char*)upkkAllocBuffer(clipboardSize);
		lstrcpy(clipboardData, "");
		if ((i = ListView_GetNextItem(hwndLogBox, -1, filter)) != - 1) {
			do {
				ListView_GetItemText(hwndLogBox, i, 0, tText, 2000);
				lstrcat(clipboardData, "[");
				lstrcat(clipboardData, tText);
				lstrcat(clipboardData, "] ");
				ListView_GetItemText(hwndLogBox, i, 1, tText, 2000);
				lstrcat(clipboardData, tText);
				lstrcat(clipboardData, "\r\n");
			} while((i = ListView_GetNextItem(hwndLogBox, i, filter)) != -1);
		}

		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, clipboardSize+1);
		memcpy(GlobalLock(hMem), clipboardData, clipboardSize);
		GlobalUnlock(hMem);
		OpenClipboard(NULL);
		EmptyClipboard();
		SetClipboardData(CF_TEXT, hMem);
		CloseClipboard();
		upkkReleaseBuffer((byte*)clipboardData);
	}
	upkkReleaseBuffer((byte*)tText);
}

/** dpi()
 *
 * Recalculate size according to the screen's DPI
 */
int dpi(float xy, int v) {
	return static_cast<INT>(xy * (float)v / 96.f);
}

/** addControl()
 *
 * Just a CreateWindowEx wrapper
 */
HWND addControl(LPCTSTR lpClassName, LPCTSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu) {
	return (HWND)CreateWindowEx(0, lpClassName, lpWindowName, dwStyle,
								dpi(dpiX, x),
								dpi(dpiY, y),
								dpi(dpiX, nWidth),
								dpi(dpiY, nHeight),
								hWndParent, hMenu, hInst, NULL);
}

/** getIconIndex()
 *
 * Get icon index for TreeView usage
 */
int getIconIndex(char *strPath, DWORD fileAttrib) {
    SHFILEINFO sfi;
    memset(&sfi, 0, sizeof(sfi));
    SHGetFileInfo(strPath, fileAttrib, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES);
    return sfi.iIcon;
}

/** reloadTreeView()
 *
 * Resets the file three list
 */
void reloadTreeView() {
	/// GUI only
	if (outputToConsole == FALSE) {
		SendMessage(hwndTree, TVM_DELETEITEM, 0, 0);
		tvIns.hInsertAfter = TVI_ROOT;
		tvIns.hParent = NULL;
		tvIns.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIS_STATEIMAGEMASK;
		tvIns.item.cchTextMax = MAX_PATH;
	}
}

/** loadINI()
 *
 *  Load INI file
 */
void loadINI() {
	HWND	hINIEditorEdit;
	HANDLE	hINIFile;
	int		textLength;
	char	*textData;
	DWORD	lpNumberOfBytesRW;
	char	*buffINIFilepath;

	/// Allocate space for the ini filepath and build it.
	/// There was a nasty bug-case when packing/unpacking routine messes up the current working directory.
	/// So, packing/unpacking and modifying the ini afterwards, reallocates it to the output directory.
	buffINIFilepath = (char*)upkkAllocBuffer((MAX_PATH*2)+1);
	GetModuleFileName(NULL, buffINIFilepath, (MAX_PATH*2));
	strrchr(buffINIFilepath, '\\')[1] = 0x00;
	lstrcat(buffINIFilepath, szINIFilename);

	/// Write INI file to disk
	hINIFile = CreateFile(buffINIFilepath, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hINIFile != INVALID_HANDLE_VALUE) {
		textLength = GetFileSize(hINIFile, NULL);
		if (textLength > 0) {
			textData = (char*)upkkAllocBuffer(textLength);
			ReadFile(hINIFile, textData, textLength, &lpNumberOfBytesRW, NULL);

			hINIEditorEdit = GetDlgItem(hwndTabINI, IDC_INI_TEXT);

			SetWindowText(hINIEditorEdit, textData);

			upkkReleaseBuffer((byte*)textData);
		}
		CloseHandle(hINIFile);
	}

	upkkReleaseBuffer((byte*)buffINIFilepath);
}

/** saveINI()
 *
 *  Save INI file
 */
void saveINI() {
	HWND	hINIEditorEdit;
	HANDLE	hINIFile;
	int		textLength;
	char	*textData;
	DWORD	lpNumberOfBytesRW;
	char	*buffINIFilepath;

	hINIEditorEdit = GetDlgItem(hwndTabINI, IDC_INI_TEXT);

	textLength = GetWindowTextLength(hINIEditorEdit);

	/// bugfix, when someone clears the INI
	/// Originally, upkkAllocBuffer() throwed error, because of the zero length of the buffer
	textData = NULL;
	if (textLength > 0) {
		textData = (char*)upkkAllocBuffer(textLength);
		GetWindowText(hINIEditorEdit, textData, textLength+1); /// +1, to include the term. zero
	}

	/// Allocate space for the ini filepath and build it.
	/// There was a nasty bug-case when packing/unpacking routine messes up the current working directory.
	/// So, packing/unpacking and modifying the ini afterwards, reallocates it to the output directory.
	buffINIFilepath = (char*)upkkAllocBuffer((MAX_PATH*2)+1);
	GetModuleFileName(NULL, buffINIFilepath, (MAX_PATH*2));
	strrchr(buffINIFilepath, '\\')[1] = 0x00;
	lstrcat(buffINIFilepath, szINIFilename);

	/// Write INI file to disk
	hINIFile = CreateFile(buffINIFilepath, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hINIFile != INVALID_HANDLE_VALUE) {
		WriteFile(hINIFile, textData, textLength, &lpNumberOfBytesRW, NULL);
		CloseHandle(hINIFile);

		log("%s saved!", szINIFilename);
	}

	upkkReleaseBuffer((byte*)buffINIFilepath);
	upkkReleaseBuffer((byte*)textData);
}

/** unpakkePack()
 *
 *  Packing procedure
 */
BOOL unpakkePack() {
	DWORD	dwAttrib;
	BOOL	moduleLoadStatus;

	dArchive.buffer = NULL;
	dArchive.numEntries = 0;
	dArchive.archiveHandle = NULL;
	dArchive.inputPath = p.pathInput;
	dArchive.outputPath = p.pathOutput;

	/// Reset the three view control (GUI ONLY)
	reloadTreeView();

	/** First, lets init the module */
	if ((moduleLoadStatus = moduleLoad(p.pathModule)) == FALSE) {
		/** Seems like the module is bad (old, unsupported, broken, penis?, etc.) sooo, yeah. */
		return FALSE;
	} else if (moduleLoadStatus == TRUE && module.pack == NULL) {
		/** In this case, the module is legit, but it doesnt support packing */
		log("This module doesnt support packing, sorry!");
		moduleUnload();
		return FALSE;
	}

	/** Let's verify the input directory */
	dwAttrib = GetFileAttributes(dArchive.inputPath);
	if (dwAttrib == INVALID_FILE_ATTRIBUTES || !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
		log("Invalid input directory: %s", dArchive.inputPath);
		moduleUnload(); // Unload module on exit!
		return FALSE;
	}
	/** The input dir seems to be valid, so let's continue */

	/// Set the working directory as the Input directory, so we can access files easier
	SetCurrentDirectory(dArchive.inputPath);

	/** Let's see how many files we got... */
	dArchive.numEntries = upkkCountFiles(dArchive.inputPath, TRUE);
	if (dArchive.numEntries == 0) {
		log("Nothing to do, directory is empty: %s", dArchive.inputPath);
		moduleUnload(); // Unload module on exit!
		return FALSE;
	}

	/** Create the new archive */
	dArchive.archiveHandle = CreateFile(p.pathOutput, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (dArchive.archiveHandle == INVALID_HANDLE_VALUE) {
		log("Cannot create output file: %s", p.pathOutput);
		moduleUnload(); // Unload module on exit!
		return FALSE;
	}

	/// Progress bar init (if GUI);
	progressSet(SW_SHOW);

	/** Allocate buffer for all of the files and enumerate the directory */
	dArchive.buffer = upkkAllocBuffer(sizeof(archiveRow)*dArchive.numEntries);
	enumDirectory(dArchive.inputPath, 0);

	log("Total files: %d", dArchive.numEntries);

	/** BOF: AT THIS POINT, WE HAVE THE FILE STRUCTURE IN A BUFFER THAT CAN BE USED BY THE MODULE */
	/** */
	if (module.pack(&dArchive)) {
		log("Packing done!");
	} else {
		log("Packing failed! No idea why.");
	}
	/** */
	/** EOF */

	/** Free the enumerated dirs buffer */
	upkkReleaseBuffer(dArchive.buffer);

	CloseHandle(dArchive.archiveHandle);

	/** On exit unload the module */
	moduleUnload();

	/// Progress bar de-init (if GUI);
	progressSet(SW_HIDE);

	return TRUE;
}

/** unpakkeUnpack() : this should be threaded! And now it is, so shut up already
 *
 *  Unpacking procedure wrapper, used both by GIU and CMD versions
 */
BOOL unpakkeUnpack() {
	DWORD	dwAttrib;
	BOOL	moduleLoadStatus;

	dArchive.buffer = NULL;
	dArchive.archiveSize = 0;
	dArchive.archiveHandle = NULL;
	dArchive.inputPath = p.pathInput;
	dArchive.outputPath = p.pathOutput;

	/// Reset the three view control (GUI ONLY)
	reloadTreeView();

	/// First, lets init the module
	if ((moduleLoadStatus = moduleLoad(p.pathModule)) == FALSE) {
		/// Seems like the module is bad (old, unsupported, broken, penis?, etc.) sooo, yeah.
		return FALSE;
	} else if (moduleLoadStatus == TRUE && module.unpack == NULL) {
		/// In this case, the module is legit, but it doesnt support unpacking
		log("This module doesnt support unpacking, sorry!");
		moduleUnload();
		return FALSE;
	}
	/// At this point, the module seems to be legit, so enough harrasing it.

	/// Let's verify the output path
	dwAttrib = GetFileAttributes(dArchive.outputPath);
	if (dwAttrib == INVALID_FILE_ATTRIBUTES || !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
		log("Invalid output directory: %s", dArchive.outputPath);
		moduleUnload(); // Unload module on exit!
		return FALSE;
	}
	/// The output dir seems to be valid, so let's continue

	/// Set the working directory as the Output directory, so we can access files easier
	SetCurrentDirectory(dArchive.outputPath);

	/// Now, lets finally map the input file
	dArchive.archiveHandle = CreateFile(dArchive.inputPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (dArchive.archiveHandle == INVALID_HANDLE_VALUE) {
		log("Cannot open input file: %s", dArchive.inputPath);
		moduleUnload(); // Unload module on exit!
		return FALSE;
	}

	/// If the file size is 0, dont bother parsing it, right?
	dArchive.archiveSize = GetFileSize(dArchive.archiveHandle, NULL);
	if (dArchive.archiveSize == 0) {
		/// The file is empty, nothing to do!
		log("Nothing to do, file is empty: %s", dArchive.inputPath);
		CloseHandle(dArchive.archiveHandle);
		moduleUnload(); // Unload module on exit!
		return FALSE;
	}

	/// Progress bar init (if GUI);
	progressSet(SW_SHOW);

	/** BOF: AT THIS POINT WE HAVE THE WHOLE FILE DATA MAPPED, SO WE CAN PASS IT TO THE MODULE! */
	/** */
	if (module.unpack(&dArchive)) {
		log("Unpacking done!");
	} else {
		log("Unpacking failed! No idea why.");
	}
	/** */
	/** EOF */

	/// Populate the file list
	upkkCountFiles(dArchive.outputPath, TRUE);

	CloseHandle(dArchive.archiveHandle);
	/// On exit unload the module
	moduleUnload();

	/// Progress bar de-init (if GUI);
	progressSet(SW_HIDE);

	return TRUE;
}

/** enumDirectory()
 *
 * Enumerate the files from directory and put them in the RAW structure buffer
 */
int enumDirectory(LPCTSTR baseDir, int i) {
	WIN32_FIND_DATA		wfd;
	HANDLE				hFind;
	char				*searchDir;
	int					ti,
						subtractBytes;
	archiveRow			aRow;

	/// On first recursion, set the file Offset and Size to 0
	if (i == 0) {
		aRow.fileOffset = 0;
		aRow.fileSizeRaw = 0;
	}

	subtractBytes = strlen(dArchive.inputPath);
	ti = i;

	searchDir = (char*)upkkAllocBuffer(lstrlen(baseDir)+10+MAX_PATH);
	wsprintf(searchDir, "%s\\*", baseDir);

	hFind = FindFirstFile(searchDir, &wfd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if ((lstrcmp(wfd.cFileName, ".") != 0) && (lstrcmp(wfd.cFileName, "..") != 0)) {
				if ((wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
					wsprintf(searchDir, "%s\\%s", baseDir, wfd.cFileName);
					ti = enumDirectory(searchDir, ti);
				} else {
					if (strlen(baseDir) == (unsigned int)subtractBytes) {
						lstrcpy(aRow.filePath, wfd.cFileName);
					} else {
						wsprintf(aRow.filePath, "%s\\%s", baseDir+subtractBytes+1, wfd.cFileName);
					}

					/// Set the file size
					aRow.fileSizeRaw = wfd.nFileSizeLow;
					/// Set the creation timestamp
					aRow.time = wfd.ftCreationTime;
					/// Move everything to the structure buffer
					//memcpy(dArchive.buffer+(ti*sizeof(archiveRow)), &aRow, sizeof(archiveRow));
					((archiveRow*)dArchive.buffer)[ti] = aRow;
					/// Update the offset for the next entry, to current offset+current file size
					aRow.fileOffset += aRow.fileSizeRaw;

					ti++;
				}
			}
		} while (FindNextFile(hFind, &wfd) != 0);
		FindClose(hFind);
	}
	upkkReleaseBuffer((byte*)searchDir);
	return ti;
}

/** libsLoad() :: Initalize the additional libraries used for compression, encryption, hashing, etc.
 *
 */
void libsLoad() {

	/// Free the libraries
	libsUnload();

	if ((hCompZLIB = LoadLibrary("zlib1.dll")) == NULL) {
		if ((hCompZLIB = LoadLibrary("libs/zlib1.dll")) == NULL) {
			log("WARNING: zlib1.dll not found! All of the related functionalities will be unavailable!");
		}
	}

	if (hCompZLIB != NULL) {
		zlibCompress2 = (hZlibCompress2)GetProcAddress(hCompZLIB, "compress2");
		zlibUncompress = (hZlibUncompress)GetProcAddress(hCompZLIB, "uncompress");
		zlibCompressBound = (hZlibCompressBound)GetProcAddress(hCompZLIB, "compressBound");
		if (zlibCompress2 == NULL || zlibUncompress == NULL || zlibCompressBound == NULL) {
			log("ZLIB Error: Cannot init one (or more) of the zlib functions.");
		}
	}

	if ((hCompLZF = LoadLibrary("lzflib.dll")) == NULL) {
		if ((hCompLZF = LoadLibrary("libs/lzflib.dll")) == NULL) {
			log("WARNING: lzflib.dll not found! All of the related functionalities will be unavailable!");
		}
	}

	if (hCompLZF != NULL) {
		lzfCompress = (hLzfCompress)GetProcAddress(hCompLZF, "lzf_compress");
		lzfUncompress = (hLzfUncompress)GetProcAddress(hCompLZF, "lzf_decompress");
		if (lzfCompress == NULL || lzfUncompress == NULL) {
			log("LZFLIB Error: Cannot init one (or more) of the lzf functions.");
		}
	}

/*
	if ((hCompCryptoPP = LoadLibrary("cryptopp.dll")) == NULL) {
		if ((hCompCryptoPP = LoadLibrary("libs/cryptopp.dll")) == NULL) {
			log("WARNING: cryptopp.dll not found! All of the related functionalities will be unavailable!");
		}
	}
*/

}

/** libsUnload() :: Free the previously inited additional libraries
 *
 */
void libsUnload() {
	if (hCompZLIB != NULL) {
		FreeLibrary(hCompZLIB);
	}
	if (hCompLZF != NULL) {
		FreeLibrary(hCompLZF);
	}
	hCompZLIB = NULL;
	hCompLZF = NULL;
}

/** recursiveCreateDirectory() :: Creates directory recursively, from fullpath
 *
 */
BOOL recursiveCreateDirectory(LPCTSTR inputFilepath) {
	DWORD	i;
	char*	filePath;

	/// Empty directory
	if (lstrlen(inputFilepath) == 0) {
		return TRUE;
	}

	/// Copy the filepath provided by the user
	filePath = (char*)upkkAllocBuffer(strlen(inputFilepath)+1);
	lstrcpy(filePath, inputFilepath);

	/// Walk through the string and create directory every time a \ character is spotted
	for(i = 0; i < strlen(filePath); i++) {
		if (*(byte*)&filePath[i] == '\\') {
			*(byte*)&filePath[i] = 0x00;	/// Terminate string
			if (lstrlen(filePath) > 0) {
				CreateDirectory(filePath, NULL);
			}
			*(byte*)&filePath[i] = '\\';	/// Restore string
		}
	}
	/// Finally create the last subdir.
	/// This also fixes the cases when only one level of recursion is present
	CreateDirectory(filePath, NULL);

	/// Release the buffer
	upkkReleaseBuffer((byte*)filePath);
	return true;
}

/** TabProcedure()
 *
 * Message callback for the damn tabs
 */
LRESULT CALLBACK TabProcedure(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
    switch(Message) {
        case WM_COMMAND: {
			char szFile[MAX_PATH];

			switch(LOWORD(wParam)) {
				/**
				 * PACK, Output file
				 * UNPACK, Input file
				 */
				case IDB_PACK_OUTPUT_FILE:
				case IDB_UNPACK_INPUT_FILE: {
					OPENFILENAME	ofn;

					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hwnd;
					ofn.lpstrFile = szFile;
					ofn.lpstrFile[0] = '\0';
					ofn.nMaxFile = sizeof(szFile);
					ofn.lpstrFilter = "All\0*.*\0";
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = NULL ;
					ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;

					if (LOWORD(wParam) == IDB_PACK_OUTPUT_FILE) {
						if (GetSaveFileName(&ofn)) {
							SetDlgItemText(hwnd, IDC_PACK_OUTPUT_FILE, ofn.lpstrFile);
						}
					} else {
						if (GetOpenFileName(&ofn)) {
							SetDlgItemText(hwnd, IDC_UNPACK_INPUT_FILE, ofn.lpstrFile);
						}
					}
					break;
				}
				/**
				 * PACK, Input directory
				 * UNPACK, Output directory
				 */
				case IDB_PACK_INPUT_DIR:
				case IDB_UNPACK_OUTPUT_DIR: {
					BROWSEINFO	bi;
					int			id;

					ZeroMemory(&bi, sizeof(bi));
					bi.hwndOwner = hwnd;
					bi.ulFlags = BIF_USENEWUI;

					if (LOWORD(wParam) == IDB_PACK_INPUT_DIR) {
						bi.lpszTitle = "Choose the input folder";
						id = IDC_PACK_INPUT_DIR;
					} else {
						bi.lpszTitle = "Choose the output folder";
						id = IDC_UNPACK_OUTPUT_DIR;
					}

					LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
					if (pidl != 0) {
						SHGetPathFromIDList(pidl, szFile);
						SetDlgItemText(hwnd, id, szFile);
					}
					break;
				}
				/** UNPACK & UNPACK, Execute **/
				case IDB_PACK:
				case IDB_UNPACK: {
					int		idInput, idOutput;

					if (LOWORD(wParam) == IDB_PACK) {
						idInput = IDC_PACK_INPUT_DIR;
						idOutput = IDC_PACK_OUTPUT_FILE;
						lstrcpy(p.method, "pack");
					} else {
						idInput = IDC_UNPACK_INPUT_FILE;
						idOutput = IDC_UNPACK_OUTPUT_DIR;
						lstrcpy(p.method, "unpack");
					}

					wsprintf(p.pathModule, "%s\\", dirModules);
					GetDlgItemText(hwndDlg, IDC_MODULE_LIST, p.pathModule+lstrlen(p.pathModule), MAX_PATH);

					GetDlgItemText(hwnd, idInput, p.pathInput, MAX_PATH);
					GetDlgItemText(hwnd, idOutput, p.pathOutput, MAX_PATH);

					log("Unpakke command line:");
					log("unpakke.exe \"%s\" %s \"%s\" \"%s\"", p.pathModule, p.method, p.pathInput, p.pathOutput);

					if (LOWORD(wParam) == IDB_PACK) {
						CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&unpakkePack, NULL, 0, NULL);
					} else {
						CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&unpakkeUnpack, NULL, 0, NULL);
					}

					break;
				}
				/** INI Editor save changes **/
				case IDC_INI_SAVE: {
					saveINI();
					SetDlgItemText(hwndTabINI, IDC_INI_SAVE, "save");
					break;
				}
				/** INI Editor change indicator **/
				case IDC_INI_TEXT: {
					if(HIWORD(wParam) == EN_CHANGE) {
						SetDlgItemText(hwndTabINI, IDC_INI_SAVE, "* save");
					}
					break;
				}
				/** INI Editor search **/
				case IDC_INI_FIND: {
					char	searchPhrase[MAX_PATH];
					HWND	hINIEditorEdit;
					char	*textData;
					int		textLength, textOccurr;

					GetDlgItemText(hwndTabINI, IDC_INI_SEARCH, searchPhrase, MAX_PATH);
					if (lstrlen(searchPhrase) >= 3) {
						hINIEditorEdit = GetDlgItem(hwndTabINI, IDC_INI_TEXT);

						textLength = GetWindowTextLength(hINIEditorEdit);
						textData = (char*)upkkAllocBuffer(textLength);
						GetWindowText(hINIEditorEdit, textData, textLength+1);
						textOccurr = textLength-lstrlen(strstr(textData, searchPhrase));
						upkkReleaseBuffer((byte*)textData);

						if (textOccurr != textLength) {
							SetFocus(hINIEditorEdit);
							SendMessage(hINIEditorEdit, EM_SETSEL, (WPARAM)textOccurr, (LPARAM)textOccurr+lstrlen(searchPhrase));
							SendMessage(hINIEditorEdit, EM_SCROLLCARET, (WPARAM)textOccurr, (LPARAM)0);
						}

					}
					break;
				}
				/** About tab -> check for update **/
				case IDC_ABOUT_UPDATE: {
					HINTERNET	hSession, hRequest;
					char		buffVersion[20];
					DWORD		valVersion;
					DWORD		bytesRead;
					char		errorMsg[] = "Cannot retrieve latest version. Use the site instead.";

					memset(buffVersion, 0x00, sizeof(buffVersion));

					hSession = InternetOpen("Mozilla/4.0 (compatible) Poison", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
					if (hSession == NULL) {
						log("%s [InternetOpen:0x%08X]", errorMsg, GetLastError());
						break;
					}

					hRequest = InternetOpenUrl(hSession, urlVersionCheck, NULL, 0, 0, 0);
					if (hRequest == NULL) {
						log("%s [InternetOpenUrl:0x%08X]", errorMsg, GetLastError());
						InternetCloseHandle(hSession);
						log(errorMsg);
						break;
					}

					if (InternetReadFile(hRequest, buffVersion, 10, &bytesRead)) {
						if (*(WORD*)&buffVersion[0] == *(WORD*)&"0x") {
							valVersion = strtol(buffVersion, NULL, 16);
							if (valVersion > UNPAKKE_VERSION) {
								log("New version available: v%d.%d (build:%d)", HIBYTE(HIWORD(valVersion)), LOBYTE(HIWORD(valVersion)), LOWORD(valVersion));
							} else {
								log("You are using the latest version.");
							}
						} else {
							log("%s", errorMsg);
						}
					} else {
						log("%s [InternetReadFile:0x%08X]", errorMsg, GetLastError());
					}
					InternetCloseHandle(hRequest);
					InternetCloseHandle(hSession);

					break;
				}
			}
			break;
        }
        default:
			return DefWindowProc(hwnd, Message, wParam, lParam);
    }
    return 0;
}

/** WindowProcedure()
 *
 * Main procedure
 */
LRESULT CALLBACK WindowProcedure(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_CREATE: {
			INITCOMMONCONTROLSEX	icc;
			TCITEM					tie;
			WNDCLASSEX				wc;
			LVCOLUMN				lvc;

			/// Init common controls
			icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
			icc.dwICC  = ICC_PROGRESS_CLASS;
			InitCommonControlsEx(&icc);

			/// Get the current app directory
			GetModuleFileName(NULL, baseAppDirectory, MAX_PATH);
			PathRemoveFileSpec(baseAppDirectory);

			/// Module chooser goes first
			hwndModuleList = addControl("COMBOBOX", 0, WS_CHILD|WS_BORDER|WS_VISIBLE|CBS_HASSTRINGS|CBS_DROPDOWNLIST|WS_VSCROLL, 10, 10, 330, 200, hwndDlg, (HMENU)IDC_MODULE_LIST);
			addControl("BUTTON", "Reload modules", WS_CHILD|WS_VISIBLE|BS_FLAT, 350, 10, 120, 25, hwndDlg, (HMENU)IDB_MODULE_LIST);

			/// Create tabs control
			hwndTab = addControl("SysTabControl32", "", WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE, 10, 40, 460, 200, hwndDlg, (HMENU)IDC_TABS);

			/// Populate the tabs
			tie.mask = TCIF_TEXT | TCIF_IMAGE;
			tie.iImage = -1;
			tie.pszText = (LPSTR)" Pack ";
			TabCtrl_InsertItem(hwndTab, 0, &tie);
			tie.pszText = (LPSTR)" Unpack ";
			TabCtrl_InsertItem(hwndTab, 1, &tie);
			tie.pszText = (LPSTR)" INI editor ";
			TabCtrl_InsertItem(hwndTab, 2, &tie);
			tie.pszText = (LPSTR)" About ";
			TabCtrl_InsertItem(hwndTab, 3, &tie);

			/// Populate Tab panels
			wc.hInstance		= hInst;
			wc.lpszClassName	= szClassTabButtons;
			wc.lpfnWndProc		= TabProcedure;
			wc.style			= CS_DBLCLKS;
			wc.cbSize			= sizeof(WNDCLASSEX);
			wc.hIcon			= LoadIcon(NULL,IDC_ARROW);
			wc.hIconSm			= LoadIcon(NULL,IDC_ARROW);
			wc.hCursor			= LoadCursor(NULL,IDC_ARROW);
			wc.lpszMenuName		= NULL;
			wc.cbClsExtra		= 0;
			wc.cbWndExtra		= 0;
			wc.hbrBackground	= (HBRUSH)COLOR_BACKGROUND;
			RegisterClassEx(&wc);

			/// PACK
			hwndTabPack = CreateWindowEx(0, szClassTabButtons, "", WS_CHILD, dpi(dpiX, 5), dpi(dpiY, 25), dpi(dpiX, 450), dpi(dpiY, 170), hwndTab, NULL, hInst, (HMENU)IDD_TAB_PACK);
			/// Create PACK Tab controls
			addControl("BUTTON", "input directory", WS_CHILDWINDOW|WS_VISIBLE|BS_GROUPBOX, 5, 10, 440, 55, hwndTabPack, NULL);
			addControl("EDIT", "", WS_CHILD|WS_VISIBLE|ES_READONLY|WS_BORDER|ES_AUTOHSCROLL, 15, 30, 330, 25, hwndTabPack, (HMENU)IDC_PACK_INPUT_DIR);
			addControl("BUTTON", "open", WS_CHILD|WS_VISIBLE|BS_FLAT, 355, 30, 80, 25, hwndTabPack, (HMENU)IDB_PACK_INPUT_DIR);
            addControl("BUTTON", "output file", WS_CHILD|WS_VISIBLE|BS_GROUPBOX, 5, 70, 440, 55, hwndTabPack, NULL);
			addControl("EDIT", "", WS_CHILD|WS_VISIBLE|ES_READONLY|WS_BORDER|ES_AUTOHSCROLL, 15, 90, 330, 25, hwndTabPack, (HMENU)IDC_PACK_OUTPUT_FILE);
			addControl("BUTTON", "choose", WS_CHILD|WS_VISIBLE|BS_FLAT, 355, 90, 80, 25, hwndTabPack, (HMENU)IDB_PACK_OUTPUT_FILE);
			addControl("BUTTON", "start", WS_CHILD|WS_VISIBLE|BS_FLAT, 355, 135, 80, 25, hwndTabPack, (HMENU)IDB_PACK);

			/// UNPACK
			hwndTabUnpack = CreateWindowEx(0, szClassTabButtons, "", WS_CHILD, dpi(dpiX, 5), dpi(dpiY, 25), dpi(dpiX, 450), dpi(dpiY, 170), hwndTab, NULL, hInst, (HMENU)IDD_TAB_UNPACK);
			/// Create UNPACK Tab controls
			addControl("BUTTON", "input file", WS_CHILDWINDOW|WS_VISIBLE|BS_GROUPBOX, 5, 10, 440, 55, hwndTabUnpack, NULL);
			addControl("EDIT", "", WS_CHILD|WS_VISIBLE|ES_READONLY|WS_BORDER|ES_AUTOHSCROLL, 15, 30, 330, 25, hwndTabUnpack, (HMENU)IDC_UNPACK_INPUT_FILE);
			addControl("BUTTON", "open", WS_CHILD|WS_VISIBLE|BS_FLAT, 355, 30, 80, 25, hwndTabUnpack, (HMENU)IDB_UNPACK_INPUT_FILE);
            addControl("BUTTON", "output directory", WS_CHILD|WS_VISIBLE|BS_GROUPBOX, 5, 70, 440, 55, hwndTabUnpack, NULL);
			addControl("EDIT", "", WS_CHILD|WS_VISIBLE|ES_READONLY|WS_BORDER|ES_AUTOHSCROLL, 15, 90, 330, 25, hwndTabUnpack, (HMENU)IDC_UNPACK_OUTPUT_DIR);
			addControl("BUTTON", "choose", WS_CHILD|WS_VISIBLE|BS_FLAT, 355, 90, 80, 25, hwndTabUnpack, (HMENU)IDB_UNPACK_OUTPUT_DIR);
			addControl("BUTTON", "start", WS_CHILD|WS_VISIBLE|BS_FLAT, 355, 135, 80, 25, hwndTabUnpack, (HMENU)IDB_UNPACK);

			/// INI EDITOR
			hwndTabINI = CreateWindowEx(0, szClassTabButtons, "", WS_CHILD, dpi(dpiX, 5), dpi(dpiY, 25), dpi(dpiX, 450), dpi(dpiY, 170), hwndTab, NULL, hInst, (HMENU)IDD_TAB_INI);
			/// Create INI Editor Tab controls
			addControl("EDIT", "", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOVSCROLL, 5, 10, 300, 25, hwndTabINI, (HMENU)IDC_INI_SEARCH);
			addControl("BUTTON", "find", WS_CHILD|WS_VISIBLE|BS_FLAT, 315, 10, 60, 25, hwndTabINI, (HMENU)IDC_INI_FIND);
			addControl("BUTTON", "save", WS_CHILD|WS_VISIBLE|BS_FLAT, 385, 10, 60, 25, hwndTabINI, (HMENU)IDC_INI_SAVE);
			addControl("EDIT", "", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_MULTILINE|ES_AUTOVSCROLL|WS_VSCROLL, 5, 40, 440, 125, hwndTabINI, (HMENU)IDC_INI_TEXT);
			loadINI();

			/// ABOUT
			char aboutText[50];
			hwndAboutIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN));
			wsprintf(aboutText, "Unpakke v%d.%d (build:%d)", HIBYTE(HIWORD(UNPAKKE_VERSION)), LOBYTE(HIWORD(UNPAKKE_VERSION)), LOWORD(UNPAKKE_VERSION));

			hwndTabAbout = CreateWindowEx(0, szClassTabButtons, "", WS_CHILD, dpi(dpiX, 5), dpi(dpiY, 25), dpi(dpiX, 450), dpi(dpiY, 170), hwndTab, NULL, hInst, (HMENU)IDD_TAB_ABOUT);
			/// Create ABOUT Tab controls
			addControl("STATIC", "", WS_CHILD|WS_VISIBLE|SS_ICON|SS_CENTERIMAGE, 5, 10, 32, 32, hwndTabAbout, (HMENU)IDC_ABOUT_ICON);
			addControl("STATIC", aboutText, WS_CHILDWINDOW|WS_VISIBLE, 45, 18, 440, 55, hwndTabAbout, NULL);
			addControl("STATIC", "Universal game resource unpacker\nXpoZed, xpozed@nullsecurity.org\n\nvisit http://nullsecurity.org/unpakke for more information", WS_CHILDWINDOW|WS_VISIBLE, 5, 50, 440, 400, hwndTabAbout, NULL);
			addControl("BUTTON", "version check", WS_CHILD|WS_VISIBLE|BS_FLAT, 5, 138, 110, 25, hwndTabAbout, (HMENU)IDC_ABOUT_UPDATE);
			SendDlgItemMessage(hwndTabAbout, IDC_ABOUT_ICON, STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hwndAboutIcon);

			/// by default, show the first tab only!
			ShowWindow(hwndTabPack, SW_SHOW);

			/// Directory structure tree view
			hwndTree = addControl("SysTreeView32", "", WS_CHILD|WS_VISIBLE|WS_BORDER|TVS_HASBUTTONS|TVS_HASLINES|TVS_LINESATROOT, 480, 10, 300, 230, hwndDlg, (HMENU)IDC_FILETREE);
			/// Set default icon for the file tree view
			Shell_GetImageLists(NULL, &phimlSmall);
			SendMessage(hwndTree, TVM_SETIMAGELIST, (WPARAM)TVSIL_NORMAL, (LPARAM)phimlSmall);

			/// Log box goes here
			hwndLogBox = addControl(WC_LISTVIEW, NULL, WS_CHILD|WS_BORDER|WS_VISIBLE|LVS_REPORT, 10, 250, 770, 115, hwndDlg, (HMENU)IDC_LOGBOX);
			ListView_SetExtendedListViewStyle(hwndLogBox, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);

			/// Log columns
			lvc.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH  | LVCF_FMT;
			lvc.fmt  = LVCFMT_LEFT;

			lvc.iSubItem = 0;
			lvc.cx       = dpi(dpiX, 50);
			lvc.pszText  = (LPSTR)"Time";
			ListView_InsertColumn(hwndLogBox, lvc.iSubItem, &lvc);

			lvc.iSubItem = 1;
			lvc.cx       = dpi(dpiX, 700);
			lvc.pszText  = (LPSTR)"Message";
			ListView_InsertColumn(hwndLogBox, lvc.iSubItem, &lvc);

			/// Zero the lv.iItem
			lv.iItem = 0;

			/// Progress bar
			hwndProgressBar = addControl(PROGRESS_CLASS, "", WS_CHILD|PBS_MARQUEE, 10, 358, 770, 10, hwndDlg, NULL);

			/// Set fonts
			HFONT hFont = CreateFont(dpi(dpiX, 16), 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
			SendMessage(hwndModuleList, WM_SETFONT, (WPARAM)hFont, TRUE);
			SendMessage(hwndTab, WM_SETFONT, (WPARAM)hFont, TRUE);

			hFont = CreateFont(dpi(dpiX, 14), 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
			SendMessage(hwndLogBox, WM_SETFONT, (WPARAM)hFont, TRUE);
			SendMessage(hwndTree, WM_SETFONT, (WPARAM)hFont, TRUE);

			/// Load modules
			reloadModulesList();

			/// Load libraries for GUI version
			libsLoad();

			break;
		}
		case WM_NOTIFY: {
			switch(((LPNMHDR)lParam)->code) {
				case TCN_SELCHANGING:
					switch(TabCtrl_GetCurFocus(hwndTab)) {
						case 0: ShowWindow(hwndTabPack, SW_HIDE); break;
                        case 1: ShowWindow(hwndTabUnpack, SW_HIDE); break;
                        case 2: ShowWindow(hwndTabINI, SW_HIDE); break;
                        case 3: ShowWindow(hwndTabAbout, SW_HIDE); break;
                    }
				break;
				case TCN_SELCHANGE:
					switch(TabCtrl_GetCurFocus(hwndTab)) {
						case 0: ShowWindow(hwndTabPack, SW_SHOW); break;
                        case 1: ShowWindow(hwndTabUnpack, SW_SHOW); break;
                        case 2: ShowWindow(hwndTabINI, SW_SHOW); break;
                        case 3: ShowWindow(hwndTabAbout, SW_SHOW); break;
                    }
				break;
			}

			/// When right button clicked on mouse
			if ((((LPNMHDR)lParam)->hwndFrom) == hwndLogBox) {
				switch (((LPNMHDR)lParam)->code) {
					case NM_RCLICK: {
						POINT cursor; /// Getting the cursor position
						GetCursorPos(&cursor);
						/// Creating the po-up menu list
						TrackPopupMenu((HMENU)GetSubMenu(LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT)), 0), TPM_LEFTALIGN|TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, hwndDlg, NULL);
					}
					break;
				}
			}
			break;
		}
		case WM_COMMAND: {
			if(HIWORD(wParam) == CBN_SELCHANGE) {
				TCHAR	moduleName[MAX_PATH];
				int moduleID = SendMessage((HWND)lParam, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				if (moduleID > 0) { // Skip the first item
					SendMessage((HWND)lParam, (UINT)CB_GETLBTEXT, (WPARAM)moduleID, (LPARAM)moduleName);
				}
			} else if (LOWORD(wParam) == IDB_MODULE_LIST) {
				reloadModulesList();
			} else if (LOWORD(wParam) == ID_COPYLINES) {
				logCopy(FALSE);
			} else if (LOWORD(wParam) == ID_COPYLOG) {
				logCopy(TRUE);
			} else if (LOWORD(wParam) == ID_CLEARLOG) {
				logClear();
			}
			break;
		}
		case WM_DESTROY: {
			ShowWindow(GetConsoleWindow(), SW_SHOW);
			moduleUnload();
			libsUnload();
			DestroyIcon((HICON)hwndAboutIcon);
			PostQuitMessage(0);
			break;
		}
		default:
			return DefWindowProc(hwndDlg,uMsg,wParam,lParam);
    }
	return 0;
}

/** WinMain()
 *
 * Windows main procedure
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nFunsterStil) {

	LPWSTR *szArglist;
	int nArgs;
	outputToConsole = FALSE;

	szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	if (szArglist == NULL) {
		MessageBox(NULL, "Unknown error:CommandLineToArgvW", "ERROR", MB_ICONERROR|MB_OK);
		return 0;
	}

	/// Command line arguments are present, do it console way
	if(nArgs >= 2) {
		outputToConsole = TRUE;

		/// Console usage
		if (nArgs == 5) {
			WideCharToMultiByte(CP_UTF8, 0, szArglist[1], -1, p.pathModule, MAX_PATH, NULL, NULL);
			WideCharToMultiByte(CP_UTF8, 0, szArglist[2], -1, p.method, MAX_PATH, NULL, NULL);
			WideCharToMultiByte(CP_UTF8, 0, szArglist[3], -1, p.pathInput, MAX_PATH, NULL, NULL);
			WideCharToMultiByte(CP_UTF8, 0, szArglist[4], -1, p.pathOutput, MAX_PATH, NULL, NULL);

			/// Load libraries for console version
			libsLoad();

			if (lstrcmp(p.method, "pack") == 0) {
				unpakkePack();
			} else if (lstrcmp(p.method, "unpack") == 0) {
				unpakkeUnpack();
			} else {
				log("ERROR: Unsupported method - %s", p.method);
			}

			/// Release libraries
			libsUnload();

		/// Print the console help
		} else {
			char info[] = "\n"\
			" \xDB\x20\x20\xDB\n"\
			" \xDB\x20\x20\xDB \x20\x20\x20 \x20\x20\x20 \x20\x20\x20 \xDB\x20\x20 \xDB\n"\
			" \xDB\x20\x20\xDB \xDB\xDB\x20 \xDB\xDB\xDB \xDB\xDB\xDB \xDB\x20\xDB \xDB\x20\xDB \xDB\xDB\xDB  XpoZed's tiny game resource unpacker\n"\
			" \xDB\x20\x20\xDB \xDB\x20\xDB \xDB\x20\xDB \xDB\x20\xDB \xDB\x20\xDB \xDB\x20\xDB \xDB\x20\x20\n"\
			" \xDB\x20\x20\xDB \xDB\x20\xDB \xDB\x20\xDB \xDB\xDB\xDB \xDB\xDB\x20 \xDB\xDB\x20 \xDB\xDB\x20  Learn more:   http://nullsecurity.org/unpakke\n"\
			" \xDB\x20\xDB\xDB \xDB\x20\xDB \xDB\x20\xDB \xDB\x20\xDB \xDB\x20\xDB \xDB\x20\xDB \xDB\x20\x20  Your comment: xpozed.dgt@gmail.com\n"\
			" \xDB\xDB\xDB\xDB \xDB\x20\xDB \xDB\xDB\xDB \xDB\x20\xDB \xDB\x20\xDB \xDB\x20\xDB \xDB\xDB\xDB\n"\
			" \x20\x20\x20\x20 \x20\x20\x20 \xDB\x20ver:%d.%d (build:%d)\n"\
			" \x20\x20\x20\x20 \x20\x20\x20 \xDB\n\n"\
			"Usage:\n"\
			"   Pack: unpakke.exe <module> pack <input directory> <output file>\n"\
			" Unpack: unpakke.exe <module> unpack <input file> <output directory>\n\n";
			log(info, HIBYTE(HIWORD(UNPAKKE_VERSION)), LOBYTE(HIWORD(UNPAKKE_VERSION)), LOWORD(UNPAKKE_VERSION));
		}

		LocalFree(szArglist);
		return 0;

	/// Show the gui, and forget about the command line params
	} else {

		LocalFree(szArglist);

		ShowWindow(GetConsoleWindow(), SW_HIDE);

		MSG				message;
		WNDCLASSEX		wincl;
		char			mainCaption[MAX_PATH];

		hInstance = GetModuleHandle(NULL);
		hInst = hInstance;
		wincl.hInstance		= hInstance;
		wincl.lpszClassName	= szClassName;
		wincl.lpfnWndProc	= WindowProcedure;
		wincl.style			= CS_DBLCLKS;
		wincl.cbSize		= sizeof(WNDCLASSEX);
		wincl.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
		wincl.hIconSm		= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
		wincl.hCursor		= LoadCursor(NULL,IDC_ARROW);
		wincl.lpszMenuName	= NULL;
		wincl.cbClsExtra	= 0;
		wincl.cbWndExtra	= 0;
		wincl.hbrBackground	= (HBRUSH)COLOR_BACKGROUND;

		if (!RegisterClassEx(&wincl)) {
			return 0;
		}

		/// get screen DPI for scaling
		HDC screen = GetDC(0);
		dpiX = static_cast<FLOAT>(GetDeviceCaps(screen, LOGPIXELSX));
		dpiY = static_cast<FLOAT>(GetDeviceCaps(screen, LOGPIXELSY));
		ReleaseDC(0, screen);

		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);

		wsprintf(mainCaption, "Unpakke v%d.%d (build:%d)", HIBYTE(HIWORD(UNPAKKE_VERSION)), LOBYTE(HIWORD(UNPAKKE_VERSION)), LOWORD(UNPAKKE_VERSION));
		hwndDlg = CreateWindowEx(0, szClassName, mainCaption, WS_CAPTION|WS_POPUP|WS_SYSMENU|DS_MODALFRAME,
							((screenWidth-796)/2),
							((screenHeight-400)/2),
							dpi(dpiX, 796),
							dpi(dpiY, 400),
							HWND_DESKTOP, NULL, hInstance, NULL);

		ShowWindow(hwndDlg, nFunsterStil);

		while (GetMessage(&message, NULL, 0, 0)) {
			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		return message.wParam;
	}

}
