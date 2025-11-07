#pragma once
#include <Windows.h>

// Max lengths definitions 
#define LANG_MAXMSG_LENGTH 100
#define LANG_MAXID_LENGTH 50
#define LANG_MAXDESC_LENGTH 10
#define LANG_MAXNAME_LENGTH 30

typedef struct {
	WCHAR szID[LANG_MAXID_LENGTH + 1];
	WCHAR szMsg[LANG_MAXMSG_LENGTH + 1];
} CONFIG_LANGENTRY;

typedef struct {
	INT iId;
	WCHAR szLangDescriptor[LANG_MAXDESC_LENGTH + 1];
	WCHAR szLangName[LANG_MAXNAME_LENGTH + 1];
	INT iNumEntries;
	CONFIG_LANGENTRY** pLangEntries;
} CONFIG_LANGDESC;

typedef struct {
	INT iNumElems;
	WCHAR szDefaultLang[LANG_MAXDESC_LENGTH + 1];
	CONFIG_LANGDESC** pLangDescriptors;
} CONFIG_LANGS;




CONFIG_LANGS* getLanguages();
VOID setDefaultLanguage(LPWSTR lpszLanguage);
CONFIG_LANGDESC* getDefaultLanguage();
BOOL addLanguage(LPCWSTR lpszLangDescriptor, LPCWSTR lpszLangName);
BOOL addLanguageMessage(LPCWSTR lpszLangDescriptor, LPCWSTR lpszMessageId, LPCWSTR lpszMessage);
LPCWSTR getLanguageMessage(LPCWSTR lpszLangDesc, LPCWSTR lpszMessageId);
CONFIG_LANGDESC* searchLanguageByName(LPCWSTR lpszLangName);
CONFIG_LANGDESC* searchLanguageById(INT iId);
CONFIG_LANGDESC* searchLanguage(LPCWSTR lpszLangDescriptor);
VOID freeLangsConfig();
