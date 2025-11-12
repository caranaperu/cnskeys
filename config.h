#pragma once
#include  <Windows.h>
#include "lang.h"

#define KEYSTAT_PREFIX L"keystat."
#define KEYSTAT_LENGTH 6
#define LANG_PREFIX L"lang."
#define VAR_PREFIX L"var."

typedef struct {
	WCHAR szID[KEYSTAT_LENGTH + 1];
	BOOL isKeyStatOn;
} CONFIG_KEYSTAT;

typedef struct {
	CONFIG_KEYSTAT keyStatus[3];
	BOOL  soundOn;
	WCHAR szDefaultLang[LANG_MAXDESC_LENGTH + 1];
} CONFIG_STAT;

INT LoadConfig(LPWSTR lpszPath);
INT SaveConfig(LPWSTR lpszPath);
LPWSTR GetDefaultConfigPath();

enum keyStatusEnum {
	KEYSTAT_CAPS = 0,
	KEYSTAT_NUM = 1,
	KEYSTAT_SCROLL = 2
};

BOOL GetKeyStatusConfig(LPCWSTR lpszKeyStatDescriptor);
BOOL SetKeyStatusConfig(LPCWSTR lpszKeyStatDescriptor, BOOL isStatusOn);
BOOL IsSoundActiveConfig();
VOID SetConfigLanguage(LPWSTR lpszLangDescriptor);
VOID SetSoundActiveConfig(BOOL bSoundOn);

//extern CONFIG_KEYSTAT g_configStat.keyStatus[3];
//extern CONFIG_STAT g_configStat;
