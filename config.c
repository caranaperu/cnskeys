// loader (File: config.cpp)
//
// Copyright (c) 2023 Carlos Arana & Copyright (c) 2023 Justin Stenning
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// Please visit https://cnskeys.github.io for more information
// about the project and latest updates.

#include <stdint.h>

#include "config.h"
#include "stringutils.h"

#define CFGERROR_FILEOPENERROR      1
#define CFGERROR_FILESIZEERROR      2
#define CFGERROR_READERROR          3
#define CFGERROR_NOTENOUGHMEM		4
#define CFGERROR_NOUTF8ENCODED		5
#define CFGERROR_INVALIDDATA		6
#define CFGERROR_PARSEERROR			7
#define CFGERROR_WRITEERROR			8
#define CFGERROR_NOTDEFAULTCONFIG	9


static BOOL ProcessConfigLine(LPWSTR lpszParseLine);
static BOOL ParseKeyStatus(LPWSTR lpszParseLine);
static BOOL ParseVariableConfigLine(LPWSTR lpszParseLine);
static BOOL ParseLanguageConfigLine(LPWSTR lpszParseLine);
static BOOL _WriteFileAsUtf8(HANDLE hFile, LPCWSTR lpszToWrite);
static BOOL CompareConfig(const CONFIG_STAT* newConfig, const CONFIG_STAT* oldConfig);


/* Definición única de la variable global */
CONFIG_STAT g_configStat =
{
	{
		{ L"caps",   TRUE},
		{ L"num",    TRUE},
		{ L"scroll", TRUE}
	},
	TRUE,
	L"en"
};

CONFIG_STAT g_configStatLoaded = { 0 }; // To keep track of loaded config

INT LoadConfig(LPWSTR lpszPath) {
	int iErrno = 0;
	LPSTR lpszBuffer = NULL; // Buffer to store read data
	DWORD dwBytesRead; // Number of bytes read

	// If its null , use the default config file
	if (lpszPath == NULL) {
		lpszPath = GetDefaultConfigPath();
	}

	if (lpszPath == NULL) {
		return CFGERROR_NOTDEFAULTCONFIG;
	}

	// Open the existing file
	HANDLE hFile = CreateFile(
		lpszPath,                   // File name
		GENERIC_READ,               // Access mode (read-only)
		FILE_SHARE_READ,            // Share mode
		NULL,                       // Security attributes
		OPEN_EXISTING,              // Open only if exists
		FILE_ATTRIBUTE_NORMAL,      // File attributes
		NULL                        // Template file (not used for existing files)
	);

	// freee memory
	free(lpszPath);
	lpszPath = NULL;


	if (hFile == INVALID_HANDLE_VALUE) {
		return CFGERROR_FILEOPENERROR;
	}

	// Get the file size
	LARGE_INTEGER iFileSize;

	if (GetFileSizeEx(hFile, &iFileSize) > 0) { // Read file contents
		// Create a buffer
		DWORD dwBufferSize = (DWORD)(iFileSize.QuadPart + 1);
		lpszBuffer = (LPSTR)calloc(1, dwBufferSize);


		if (lpszBuffer != NULL) {
			BOOL bResult = ReadFile(
				hFile,                  // File handle
				lpszBuffer,                 // Buffer to store data
				dwBufferSize - 1,        // Number of bytes to read (-1 to leave space for null-terminator)
				&dwBytesRead,           // Number of bytes read
				NULL                    // Overlapped (not used here)
			);

			if (bResult) {
				if (dwBytesRead > 0) {
					lpszBuffer[dwBytesRead - 1] = '\0'; // Null-terminate the buffer

					// is utf-8 encoded? (required)
					if ((uint8_t)lpszBuffer[0] == (uint8_t)0xEF && (uint8_t)lpszBuffer[1] == (uint8_t)0xBB && (uint8_t)lpszBuffer[2] == (unsigned char)0xBF) {
						// Convert UTF-8 to UTF-16 (wide character)
						int wcharCount = MultiByteToWideChar(CP_UTF8, 0, lpszBuffer, dwBytesRead, NULL, 0);
						// get wide char buffer
						LPWSTR lpszWideBuffer = (LPWSTR)calloc(1, wcharCount * sizeof(WCHAR));

						if (lpszWideBuffer) {
							// skip bom header ( +3)
							MultiByteToWideChar(CP_UTF8, 0, lpszBuffer + 3, dwBytesRead, lpszWideBuffer, wcharCount);

							free(lpszBuffer);
							lpszBuffer = NULL;

							// Split the buffer into lines and store them
							WCHAR* lpszNextToken = NULL;
							LPWSTR lpszToken = wcstok_s(lpszWideBuffer, L"\n", &lpszNextToken);
							while (lpszToken != NULL && iErrno == 0) {
								// IF begin with -- , skip
								if (!startsWith(lpszToken, L"--")) {
									if (!ProcessConfigLine(lpszToken)) {
										iErrno = CFGERROR_PARSEERROR;
									}

								}

								lpszToken = wcstok_s(NULL, L"\n", &lpszNextToken);
							}

							free(lpszWideBuffer);
							lpszWideBuffer = NULL;
						}
						else {
							iErrno = CFGERROR_NOTENOUGHMEM;
						}
					}
					else {
						iErrno = CFGERROR_NOUTF8ENCODED;
					}
				}
				else {
					iErrno = CFGERROR_INVALIDDATA;
				}
			}
			else {
				iErrno = CFGERROR_READERROR;
			}
		}
		else {
			iErrno = CFGERROR_NOTENOUGHMEM;
		}
	}
	else {
		iErrno = CFGERROR_FILESIZEERROR;
	}

	// Clean stuff
	if (lpszBuffer != NULL) {
		free(lpszBuffer);
	}

	// Close the file handle
	if (hFile) {
		CloseHandle(hFile);
	}

	g_configStatLoaded = g_configStat; // to preserve loaded values

	return iErrno;
}




static BOOL ProcessConfigLine(LPWSTR lpszParseLine) {
	if (startsWith(lpszParseLine, KEYSTAT_PREFIX)) {
		if (!ParseKeyStatus(lpszParseLine)) {
			return FALSE;
		}
	}
	else 	if (startsWith(lpszParseLine, VAR_PREFIX)) {
		if (!ParseVariableConfigLine(lpszParseLine)) {
			return FALSE;
		}
	}
	else 	if (startsWith(lpszParseLine, LANG_PREFIX)) {
		if (!ParseLanguageConfigLine(lpszParseLine)) {
			return FALSE;
		}
	}

	return TRUE;
}

static BOOL ParseKeyStatus(LPWSTR lpszParseLine) {
	WCHAR* lpszNextToken = NULL;
	LPWSTR lpszToken = NULL;
	int iPos = 0;
	errno_t errCopy = 0;

	WCHAR szKeyStatDescriptor[KEYSTAT_LENGTH + 1] = { 0 };
	BOOL  isStatusOn = FALSE;

	lpszToken = wcstok_s(lpszParseLine, L".=\n", &lpszNextToken);
	while (lpszToken != NULL && errCopy == 0) {
		if (iPos == 1) {
			// key descriptor line caps,num,scroll
			removeSpaces(lpszToken);
			errCopy = wcscpy_s(szKeyStatDescriptor, KEYSTAT_LENGTH + 1, lpszToken);
		}
		else if (iPos == 2) {
			// staus 1 on , 0 off
			removeSpaces(lpszToken);

			isStatusOn = FALSE;
			if (wcscmp(lpszToken, L"1") == 0) {
				isStatusOn = TRUE;
			}
		}

		if (!errCopy) {
			if (iPos == 2 && wcslen(szKeyStatDescriptor) != 0 &&
				(lstrcmp(szKeyStatDescriptor, L"caps") == 0 ||
					lstrcmp(szKeyStatDescriptor, L"num") == 0 ||
					lstrcmp(szKeyStatDescriptor, L"scroll") == 0))
			{
				if (!SetKeyStatusConfig(szKeyStatDescriptor, isStatusOn)) {
					return FALSE;
				}

			}
			else {
				// parse error
			}
		}
		else {
			// invalid position 
			return FALSE;
		}

		iPos++;
		lpszToken = wcstok_s(NULL, L".=\n", &lpszNextToken);
	}
	return TRUE;
}

static BOOL ParseVariableConfigLine(LPWSTR lpszParseLine) {
	WCHAR* lpszNextToken = NULL;
	LPWSTR lpszToken = NULL;
	int iPos = 0;
	errno_t errCopy = 0;
	INT iEntryType = -1;

	lpszToken = wcstok_s(lpszParseLine, L".=\n", &lpszNextToken);
	while (lpszToken != NULL && errCopy == 0) {
		removeSpaces(lpszToken);


		if (iPos == 1) {
			if (!lstrcmp(lpszToken, L"lang_default")) {
				iEntryType = 1;
			}
			else if (!lstrcmp(lpszToken, L"sound_on")) {
				// future variable types
				iEntryType = 2;
			}
			else {
				errCopy = 1; // err detected
			}
		}
		else if (iPos == 2) {
			if (iEntryType == 1) {
				SetConfigLanguage(lpszToken);
			}
			else if (iEntryType == 2) {
				SetSoundActiveConfig(!lstrcmp(lpszToken, L"1") ? TRUE : FALSE);
			}
			else {
				errCopy = 1; // err detected
			}

		}
		iPos++;

		lpszToken = wcstok_s(NULL, L".=\n", &lpszNextToken);
	}

	if (errCopy == 0) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}


static BOOL ParseLanguageConfigLine(LPWSTR lpszParseLine) {
	WCHAR* lpszNextToken = NULL;
	LPWSTR lpszToken = NULL;
	int iPos = 0;
	errno_t errCopy = 0;

	WCHAR szLangDescriptor[LANG_MAXDESC_LENGTH + 1] = { 0 };
	WCHAR szLangName[LANG_MAXNAME_LENGTH + 1] = { 0 };
	WCHAR szLangMessageId[LANG_MAXID_LENGTH + 1] = { 0 };
	WCHAR szLangMessage[LANG_MAXMSG_LENGTH + 1] = { 0 };

	BOOL isNextLangName = FALSE;

	lpszToken = wcstok_s(lpszParseLine, L".=\n", &lpszNextToken);
	while (lpszToken != NULL && errCopy == 0) {


		if (iPos == 1) {
			// lang descriptor, line es,en
			removeSpaces(lpszToken);
			errCopy = wcscpy_s(szLangDescriptor, LANG_MAXDESC_LENGTH, lpszToken);
		}
		else if (iPos == 2) {
			// lang msg id
			removeSpaces(lpszToken);
			if (wcscmp(lpszToken, L"langname") == 0) {
				isNextLangName = TRUE;
			}
			else {

				errCopy = wcscpy_s(szLangMessageId, LANG_MAXID_LENGTH, lpszToken);
			}

		}
		else if (iPos == 3) {
			rtrim(ltrim(lpszToken));

			if (isNextLangName) {
				errCopy = wcscpy_s(szLangName, LANG_MAXNAME_LENGTH, lpszToken);
			}
			else {
				// lang msg text
				errCopy = wcscpy_s(szLangMessage, LANG_MAXMSG_LENGTH, lpszToken);

			}
		}
		iPos++;

		lpszToken = wcstok_s(NULL, L".=\n", &lpszNextToken);
	}

	// Check error , if error return 
	if (errCopy != 0 && (wcslen(szLangDescriptor) == 0 || wcslen(szLangMessageId) == 0 ||
		wcslen(szLangMessage) == 0 || (isNextLangName && wcslen(szLangName) == 0))) {
		return FALSE;
	}

	if (addLanguage(szLangDescriptor, szLangName) == TRUE) {
		if (!isNextLangName) {
			addLanguageMessage(szLangDescriptor, szLangMessageId, szLangMessage);
		}
	}

	// Add the line elements to the language data
	return TRUE;
}



INT SaveConfig(LPWSTR lpszPath) {
	BOOL bWriteError = FALSE;
	DWORD dwBytesWritten;
	WCHAR szWorkBuffer[1024] = L"\0";

	if (CompareConfig(&g_configStat, &g_configStatLoaded)) {
		// No changes , no need to save
		return 0;
	}

	// If its null , use the default config file
	if (lpszPath == NULL) {
		lpszPath = GetDefaultConfigPath();
	}

	if (lpszPath == NULL) {
		return CFGERROR_NOTDEFAULTCONFIG;
	}

	// Open the existing file
	HANDLE hFile = CreateFile(
		lpszPath,                   // File name
		GENERIC_WRITE,               // Access mode (write-only)
		0,							 // Not shared
		NULL,                       // Security attributes
		OPEN_EXISTING,              // Open only if exists
		FILE_ATTRIBUTE_NORMAL,      // File attributes
		NULL
	);

	// freee memory
	free(lpszPath);
	lpszPath = NULL;

	if (hFile == INVALID_HANDLE_VALUE) {
		return CFGERROR_FILEOPENERROR;
	}

	// 0) Write BOM chars
	// 
	BYTE utf8BOM[] = { 0xEF, 0xBB, 0xBF };
	bWriteError = WriteFile(hFile, utf8BOM, sizeof(utf8BOM), &dwBytesWritten, NULL);

	// 1) Write defaults
	// 

	if (bWriteError) bWriteError = _WriteFileAsUtf8(hFile, L"-- key stats definitions\n");

	wsprintf(szWorkBuffer, L"keystat.caps = %d\n", GetKeyStatusConfig(L"caps"));
	if (bWriteError) bWriteError = _WriteFileAsUtf8(hFile, szWorkBuffer);

	wsprintf(szWorkBuffer, L"keystat.num = %d\n", GetKeyStatusConfig(L"num"));
	if (bWriteError) bWriteError = _WriteFileAsUtf8(hFile, szWorkBuffer);

	wsprintf(szWorkBuffer, L"keystat.scroll = %d\n", GetKeyStatusConfig(L"scroll"));
	if (bWriteError) bWriteError = _WriteFileAsUtf8(hFile, szWorkBuffer);


	if (bWriteError) bWriteError = _WriteFileAsUtf8(hFile, L"--var definitions\n");

	// 2) Variables
	// var.sound_on = 1 o 0
	wsprintf(szWorkBuffer, L"var.sound_on = %s\n\n", IsSoundActiveConfig() ? L"1" : L"0");
	if (bWriteError) bWriteError = _WriteFileAsUtf8(hFile, szWorkBuffer);

	// var.lang_default = xx
	CONFIG_LANGDESC* pLangDesc = getDefaultLanguage();
	wsprintf(szWorkBuffer, L"var.lang_default = %s\n\n", pLangDesc->szLangDescriptor);
	if (bWriteError) bWriteError = _WriteFileAsUtf8(hFile, szWorkBuffer);


	// 3) Write Language stuff
	// 
	if (bWriteError) bWriteError = _WriteFileAsUtf8(hFile, L"---- Language definiton\n");

	CONFIG_LANGS* pLangs = getLanguages();

	for (int i = 0; i < pLangs->iNumElems && bWriteError; i++) {
		if (bWriteError) bWriteError = _WriteFileAsUtf8(hFile, L"--\n");

		wsprintf(szWorkBuffer, L"lang.%s.langname = %s\n", pLangs->pLangDescriptors[i]->szLangDescriptor, pLangs->pLangDescriptors[i]->szLangName);
		if (bWriteError) bWriteError = _WriteFileAsUtf8(hFile, szWorkBuffer);

		for (int j = 0; j < pLangs->pLangDescriptors[i]->iNumEntries && bWriteError; j++) {
			wsprintf(szWorkBuffer, L"lang.%s.%s = %s\n", pLangs->pLangDescriptors[i]->szLangDescriptor, pLangs->pLangDescriptors[i]->pLangEntries[j]->szID, pLangs->pLangDescriptors[i]->pLangEntries[j]->szMsg);
			if (bWriteError) bWriteError = _WriteFileAsUtf8(hFile, szWorkBuffer);
		}
		if (bWriteError) bWriteError = _WriteFileAsUtf8(hFile, L"\n");

	}

	// Close the file handle
	if (hFile) {
		CloseHandle(hFile);
	}

	if (!bWriteError) {
		return CFGERROR_WRITEERROR;
	}

	return 0;
}

static BOOL _WriteFileAsUtf8(HANDLE hFile, LPCWSTR lpszToWrite) {
	DWORD dwBytesWritten = 0;
	BOOL bOk = TRUE;

	// Convert wide char string to UTF-8
	int utf8Size = WideCharToMultiByte(
		CP_UTF8,               // Code page (UTF-8)
		0,                     // Flags (0 for default behavior)
		lpszToWrite,            // Input wide char string
		-1,                    // Input string length (-1 for null-terminated)
		NULL,                  // Output buffer (get required size)
		0,                     // Output buffer size
		NULL, NULL             // Default replacement character, used when a character cannot be represented in the target character set.
	);

	if (utf8Size == 0) {
		return FALSE;
	}

	// Get the result buffer to be written
	char* utf8Buffer = (char*)calloc(1, utf8Size);

	if (WideCharToMultiByte(
		CP_UTF8,               // Code page (UTF-8)
		0,                     // Flags (0 for default behavior)
		lpszToWrite,            // Input wide char string
		-1,                    // Input string length (-1 for null-terminated)
		utf8Buffer,            // Output buffer
		utf8Size,              // Output buffer size
		NULL, NULL             // Default replacement character
	) == 0) {
		bOk = FALSE;
	}

	if (bOk) {
		WriteFile(hFile, utf8Buffer, utf8Size - 1, &dwBytesWritten, NULL);
	}

	if (utf8Buffer) {
		free(utf8Buffer);
		utf8Buffer = NULL;
	}

	return bOk;
}


LPWSTR GetDefaultConfigPath() {
	LPWSTR lpszPath = (LPWSTR)calloc(1, MAX_PATH * sizeof(WCHAR));

	if (lpszPath != NULL) {
		DWORD length = GetModuleFileName(NULL, lpszPath, MAX_PATH);

		if (length == 0) {
			return NULL;
		}

		// Remove the executable name to get the directory
		LPWSTR lpszLastBackslash = wcsrchr(lpszPath, L'\\');
		if (lpszLastBackslash != NULL) {
			*lpszLastBackslash = L'\0';
		}

		// Now add the config file part
		LPCWSTR lpszConfigFilePart = L"\\config.cfg";

		if (wcscat_s(lpszPath, MAX_PATH, lpszConfigFilePart) != 0) {
			return NULL;
		}


	}
	// Free memory outside is not null!!!!!!!!
	return lpszPath;
}


BOOL SetKeyStatusConfig(LPCWSTR lpszKeyStatDescriptor, BOOL isStatusOn) {
	BOOL bRet = TRUE;

	if (lstrcmp(lpszKeyStatDescriptor, L"caps") == 0) {
		g_configStat.keyStatus[KEYSTAT_CAPS].isKeyStatOn = isStatusOn;
	}
	else 	if (lstrcmp(lpszKeyStatDescriptor, L"num") == 0) {
		g_configStat.keyStatus[KEYSTAT_NUM].isKeyStatOn = isStatusOn;
	}
	else 	if (lstrcmp(lpszKeyStatDescriptor, L"scroll") == 0) {
		g_configStat.keyStatus[KEYSTAT_SCROLL].isKeyStatOn = isStatusOn;
	}
	else {
		bRet = FALSE;
	}

	return bRet;
}

BOOL GetKeyStatusConfig(LPCWSTR lpszKeyStatDescriptor) {
	if (lstrcmp(lpszKeyStatDescriptor, L"caps") == 0) {
		return g_configStat.keyStatus[KEYSTAT_CAPS].isKeyStatOn;
	}
	else 	if (lstrcmp(lpszKeyStatDescriptor, L"num") == 0) {
		return g_configStat.keyStatus[KEYSTAT_NUM].isKeyStatOn;
	}
	else 	if (lstrcmp(lpszKeyStatDescriptor, L"scroll") == 0) {
		return g_configStat.keyStatus[KEYSTAT_SCROLL].isKeyStatOn;
	}
	return -1;
}


BOOL IsSoundActiveConfig() {
	return g_configStat.soundOn;
}

VOID SetSoundActiveConfig(BOOL bSoundOn) {
	g_configStat.soundOn = bSoundOn;
}

VOID SetConfigLanguage(LPWSTR lpszLangDescriptor) {
	wcscpy_s(g_configStat.szDefaultLang, LANG_MAXDESC_LENGTH, lpszLangDescriptor);
	setDefaultLanguage(lpszLangDescriptor);
}

BOOL CompareConfig(const CONFIG_STAT* newConfig, const CONFIG_STAT* oldConfig) {
	if (newConfig->soundOn == oldConfig->soundOn &&
		!lstrcmp(newConfig->szDefaultLang, oldConfig->szDefaultLang) &&
			newConfig->keyStatus[KEYSTAT_CAPS].isKeyStatOn == oldConfig->keyStatus[KEYSTAT_CAPS].isKeyStatOn &&
			newConfig->keyStatus[KEYSTAT_NUM].isKeyStatOn == oldConfig->keyStatus[KEYSTAT_NUM].isKeyStatOn &&
			newConfig->keyStatus[KEYSTAT_SCROLL].isKeyStatOn == oldConfig->keyStatus[KEYSTAT_SCROLL].isKeyStatOn) {

			// The same
			return TRUE;
	}
	return FALSE;
}