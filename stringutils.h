#pragma once
#include <Windows.h>

#define WCHAR_BYTES_SIZE(length)  ((length) * sizeof(wchar_t))

int GetFirstCharacterPosition(const wchar_t* str, wchar_t ch);
int endsWith(const wchar_t* source, const wchar_t* target);
BOOL startsWith(const wchar_t* source, const wchar_t* prefix);
int isWideXDigit(wchar_t wc);
int areAllWideHex(const wchar_t* str);
void removeSpaces(wchar_t* str);
wchar_t* ltrim(wchar_t* str);
wchar_t* rtrim(wchar_t* str);
void stringToUpper(wchar_t* str);
void stringToLower(wchar_t* str);