#pragma once
#include  <Windows.h>

#define KEYSTAT_PREFIX L"keystat."
#define KEYSTAT_LENGTH 6
#define LANG_PREFIX L"lang."
#define VAR_PREFIX L"var."

typedef struct {
	WCHAR szID[KEYSTAT_LENGTH + 1];
	BOOL isKeyStatOn;
} CONFIG_KEYSTAT;

INT LoadConfig(LPWSTR lpszPath);
INT SaveConfig(LPWSTR lpszPath);
LPWSTR GetDefaultConfigPath();

enum keyStatusEnum {
	KEYSTAT_CAPS = 0,
	KEYSTAT_NUM = 1,
	KEYSTAT_SCROLL = 2
};

BOOL getKeyStatusConfig(LPCWSTR lpszKeyStatDescriptor);
BOOL setKeyStatus(LPCWSTR lpszKeyStatDescriptor, BOOL isStatusOn);

extern CONFIG_KEYSTAT g_keyStatusConfig[3];
