/*
 * File : stringUtils.c
 * --------------------
 * Utilities string manipulation.
 *
 * @author   Carlos Arana Reategui
 * @version 1.00-19072023
 */

#include <wchar.h>

 /**
 * find the first caracter position on a string.
 *
 * @param wchar_t* str the origin string to search in.
 * @param wchart_t ch the character to search.
 *
 * @return int >0 found position < 0 not found.
 */
int GetFirstCharacterPosition(const wchar_t* str, wchar_t ch)
{
	const wchar_t* found = wcschr(str, ch);
	if (found != NULL)
	{
		// Calculate the position by subtracting the start pointer
		return (int)(found - str);
	}
	else
	{
		return -1;  // Character not found
	}
}


/**
* Finds if a source string ends with the specified string target.
*
* @param wchar_t* source the origin string to search in.
* @param wchar_t* target the string to search at the end
*
* @return int 1 ends with 0 not end with.
*/
int endsWith(const wchar_t* source, const wchar_t* target)
{
	size_t sourceLength = wcslen(source);
	size_t targetLength = wcslen(target);

	if (targetLength > sourceLength)
		return 0;  // Target string is longer than the source string

	const wchar_t* endOfSource = source + (sourceLength - targetLength);

	return wcscmp(endOfSource, target) == 0 ? 1 : 0;
}

/**
*Finds if a source string begins with the specified string prefix.
*
* @param wchar_t* source the origin string to search in.
* @param wchar_t* prefix the string to search at the end
*
* @return int 1 ends with 0 not end with.
*/
int startsWith(const wchar_t* source, const wchar_t* prefix) {
	size_t prefixLength = wcslen(prefix);
	return wcsncmp(source, prefix, prefixLength) == 0;
}


/**
* Check if a wide char is an hexadecimal digit
* 0-9
* A,B,C,D,E,F
* a,b,c,d,e,f
*
* @param wchar_t* wc the wide char to check
*
* @return int 1 if it is , 0 otherwise.
*/
int isWideXDigit(wchar_t wc) {
	if ((wc >= L'0' && wc <= L'9') ||
		(wc >= L'A' && wc <= L'F') ||
		(wc >= L'a' && wc <= L'f')) {
		return 1; // Wide character is a hexadecimal digit
	}
	return 0; // Wide character is not a hexadecimal digit
}

/**
* Check if a wide string had all hexadecimal digits
*
* @param wchar_t* str the wide string to check
*
* @return int 1 if it is , 0 otherwise.
*/
int areAllWideHex(const wchar_t* str) {
	while (*str != L'\0') {
		if (!isWideXDigit(*str)) {
			return 0; // Not a hexadecimal digit, return false
		}
		str++; // Move to the next wide character
	}
	return 1; // All characters are hexadecimal digits
}


/**
* Remove all white spaces from a string
*
* @param wchar_t* str the string to process
*/
void removeSpaces(wchar_t* str) {
	wchar_t* dest = str; // Destination pointer for non-space characters

	while (*str != L'\0') {
		if (!iswspace(*str)) {
			*dest++ = *str; // Copy non-space characters
		}
		str++; // Move to the next character in the original string
	}

	*dest = L'\0'; // Null-terminate the string at the new end
}

/**
* Remove all white spaces on the left of a string
*
* @param wchar_t* str the string to process
*
* @return wchar_t* the result string.
*/
wchar_t* ltrim(wchar_t* str) {
	wchar_t* pos = str;

	// Find the first non-whitespace character
	while (*pos && iswspace(*pos)) {
		pos++;
	}

	// Shift the remaining characters to the beginning of the string
	wchar_t* dest = pos;
	pos = str;
	while (*dest) {
		*pos++ = *dest++;
	}

	// Null-terminate the trimmed string
	*pos = '\0';
	return str;
}

/**
* Remove all white spaces on the rigth of a string
*
* @param wchar_t* str the string to process
*
* @return wchar_t* the result string.
*/
wchar_t* rtrim(wchar_t* str) {
	// Find the last non-whitespace character
	wchar_t* end = str;
	while (*end) {
		end++;
	}
	while (end > str && iswspace(*(end - 1))) {
		end--;
	}

	// Null-terminate the trimmed string
	*end = '\0';
	return str;
}



/**
* Uppercase all chars on string
*
* @param wchar_t* str the string to process
*/
void stringToUpper(wchar_t* str) {
	while (*str != L'\0') {
		*str = (wchar_t)towupper((wint_t)*str); // Convert wide character to uppercase
		str++; // Move to the next wide character
	}
}

/**
* Lowercase all chars on string
*
* @param wchar_t* str the string to process
*/
void stringToLower(wchar_t* str) {
	while (*str != L'\0') {
		*str = (wchar_t)towlower((wint_t)*str); // Convert wide character to uppercase
		str++; // Move to the next wide character
	}
}