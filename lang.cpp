#include "lang.h"
#include "stringUtils.h"

#define LANG_BATCH_GROUPSIZE 5
#define LANG_MSG_BATCH_GROUPSIZE 10


static CONFIG_LANGENTRY* searchLanguageMessage(LPCWSTR lpszLangDesc, LPCWSTR lpszMessageId);

static CONFIG_LANGS stConfigLangs = { 0,L"en",NULL };

CONFIG_LANGS* getLanguages() {
	return &stConfigLangs;
}

VOID setDefaultLanguage(LPWSTR lpszLanguage) {
	if (lpszLanguage != NULL && wcslen(lpszLanguage) > 1) {
		stringToLower(lpszLanguage);
		wcscpy_s(stConfigLangs.szDefaultLang, LANG_MAXDESC_LENGTH, lpszLanguage);
	}
}

CONFIG_LANGDESC* getDefaultLanguage() {
	CONFIG_LANGDESC* pLang = searchLanguage(stConfigLangs.szDefaultLang);
	if (pLang == NULL) {
		pLang = searchLanguage(L"es");
	}

	return pLang;
}


BOOL addLanguage(LPCWSTR lpszLangDescriptor, LPCWSTR lpszLangName) {
	BOOL bResult = FALSE;

	// if a languge descriptor is defined , proceed
	if (lpszLangDescriptor != NULL && wcslen(lpszLangDescriptor) > 1) {
		// check if already exist
		if (searchLanguage(lpszLangDescriptor) != NULL) {
			return TRUE;
		}
		else {
			// The new language entry need a name
			if (lpszLangName == NULL || wcslen(lpszLangName) <= 3) {
				return FALSE;
			}
		}

		// Add first group 
		CONFIG_LANGDESC** pLangDescriptors = stConfigLangs.pLangDescriptors;
		if (stConfigLangs.iNumElems == 0) {
			pLangDescriptors = (CONFIG_LANGDESC**)calloc(LANG_BATCH_GROUPSIZE, sizeof(CONFIG_LANGDESC*));
		}
		else {
			if (stConfigLangs.iNumElems % LANG_BATCH_GROUPSIZE == 0) {
				// add new batch to support more languages
				CONFIG_LANGDESC** temp = pLangDescriptors = (CONFIG_LANGDESC**)realloc(stConfigLangs.pLangDescriptors, (size_t)sizeof(CONFIG_LANGS*) * (size_t)(LANG_BATCH_GROUPSIZE + stConfigLangs.iNumElems));
				if (!temp) {
					return FALSE;
				}
				pLangDescriptors = temp;
			}
		}

		// add entry
		if (pLangDescriptors != NULL) {
			stConfigLangs.pLangDescriptors = pLangDescriptors;

			CONFIG_LANGDESC* pLangDescEntry = (CONFIG_LANGDESC*)calloc(1, sizeof(CONFIG_LANGDESC));
			if (pLangDescEntry != NULL) {
				// ok add the language
				wcscpy_s(pLangDescEntry->szLangDescriptor, LANG_MAXDESC_LENGTH, lpszLangDescriptor);
				wcscpy_s(pLangDescEntry->szLangName, LANG_MAXNAME_LENGTH, lpszLangName);
				pLangDescEntry->pLangEntries = NULL;

				stConfigLangs.pLangDescriptors[stConfigLangs.iNumElems] = pLangDescEntry;
				stConfigLangs.pLangDescriptors[stConfigLangs.iNumElems]->iId = stConfigLangs.iNumElems;
				stConfigLangs.iNumElems++;

				bResult = TRUE;
			}
		}
	}
	return bResult;

}

BOOL addLanguageMessage(LPCWSTR lpszLangDescriptor, LPCWSTR lpszMessageId, LPCWSTR lpszMessage) {
	BOOL bResult = FALSE;

	// Nothing to do if not exist the language
	CONFIG_LANGDESC* pLangDescriptors = searchLanguage(lpszLangDescriptor);
	if (pLangDescriptors == NULL) {
		return FALSE;
	}

	CONFIG_LANGENTRY* pLangEntry = searchLanguageMessage(lpszLangDescriptor, lpszMessage);
	// if exist , override the message
	if (pLangEntry != NULL) {
		wcscpy_s(pLangEntry->szMsg, LANG_MAXMSG_LENGTH, lpszMessage);
	}
	else {
		CONFIG_LANGENTRY** pLangEntries = pLangDescriptors->pLangEntries;

		// add a new entry
		if (pLangDescriptors->iNumEntries == 0) {
			pLangEntries = (CONFIG_LANGENTRY**)calloc(LANG_MSG_BATCH_GROUPSIZE, sizeof(CONFIG_LANGENTRY));
		}
		else {
			if (pLangDescriptors->iNumEntries % LANG_MSG_BATCH_GROUPSIZE == 0) {
				// add new batch to support more languages
				CONFIG_LANGENTRY** temp = (CONFIG_LANGENTRY**)realloc(pLangDescriptors->pLangEntries, (size_t)sizeof(CONFIG_LANGENTRY*) * (size_t)(LANG_MSG_BATCH_GROUPSIZE + pLangDescriptors->iNumEntries));
				if (!temp) {
					return FALSE;
				}
				pLangEntries = temp;
			}
		}

		// add entry
		if (pLangEntries != NULL) {
			pLangDescriptors->pLangEntries = pLangEntries;

			// ok add the language
			CONFIG_LANGENTRY* pLangEntry = (CONFIG_LANGENTRY*)calloc(1, sizeof(CONFIG_LANGENTRY));

			if (pLangEntry != NULL) {
				wcscpy_s(pLangEntry->szID, LANG_MAXID_LENGTH, lpszMessageId);
				wcscpy_s(pLangEntry->szMsg, LANG_MAXMSG_LENGTH, lpszMessage);

				pLangDescriptors->pLangEntries[pLangDescriptors->iNumEntries] = pLangEntry;
				pLangDescriptors->iNumEntries++;

				bResult = TRUE;
			}
		}
	}

	return bResult;
}


CONFIG_LANGDESC* searchLanguageById(INT iId) {
	CONFIG_LANGDESC* pLangDescriptor = NULL;

	if (iId >= 0) {
		// linear search is sufficient here
		for (int i = 0; i < stConfigLangs.iNumElems; i++) {
			if (stConfigLangs.pLangDescriptors[i]->iId == iId) {
				pLangDescriptor = stConfigLangs.pLangDescriptors[i];
				break;
			}
		}
	}

	return pLangDescriptor;
}

CONFIG_LANGDESC* searchLanguageByName(LPCWSTR lpszLangName) {
	CONFIG_LANGDESC* pLangDescriptor = NULL;

	if (lpszLangName != NULL && wcslen(lpszLangName) > 3) {
		// linear search is sufficient here
		for (int i = 0; i < stConfigLangs.iNumElems; i++) {
			if (!lstrcmp(stConfigLangs.pLangDescriptors[i]->szLangName, lpszLangName)) {
				pLangDescriptor = stConfigLangs.pLangDescriptors[i];
				break;
			}
		}
	}

	return pLangDescriptor;
}

CONFIG_LANGDESC* searchLanguage(LPCWSTR lpszLangDescriptor) {
	CONFIG_LANGDESC* pLangDescriptor = NULL;

	if (lpszLangDescriptor != NULL && wcslen(lpszLangDescriptor) > 1) {
		// linear search is sufficient here
		for (int i = 0; i < stConfigLangs.iNumElems; i++) {
			if (!lstrcmp(stConfigLangs.pLangDescriptors[i]->szLangDescriptor, lpszLangDescriptor)) {
				pLangDescriptor = stConfigLangs.pLangDescriptors[i];
				break;
			}
		}
	}

	return pLangDescriptor;
}

static CONFIG_LANGENTRY* searchLanguageMessage(LPCWSTR lpszLangDesc, LPCWSTR lpszMessageId) {
	CONFIG_LANGENTRY* pLangEntry = NULL;
	CONFIG_LANGDESC* pLangDescriptors = NULL;

	// if none indicated take the default language
	if (lpszLangDesc == NULL) {
		pLangDescriptors = searchLanguage(stConfigLangs.szDefaultLang);
	}
	else {
		if (wcslen(lpszLangDesc) > 1) {
			pLangDescriptors = searchLanguage(lpszLangDesc);
		}
	}
	// if the language is found , search the message
	if (pLangDescriptors != NULL) {
		// Search if the message id exist
		// linear search is sufficient her
		for (int i = 0; i < pLangDescriptors->iNumEntries; i++) {

			if (!lstrcmp(pLangDescriptors->pLangEntries[i]->szID, lpszMessageId)) {
				pLangEntry = pLangDescriptors->pLangEntries[i];
				break;
			}
		}
	}
	return pLangEntry;
}

LPCWSTR getLanguageMessage(LPCWSTR lpszLangDesc, LPCWSTR lpszMessageId) {

	CONFIG_LANGENTRY* pLangEntry = searchLanguageMessage(lpszLangDesc, lpszMessageId);
	if (pLangEntry != NULL) {
		return pLangEntry->szMsg;
	}
	return L"";
}


VOID freeLangsConfig() {
	// Free al the config lang descriptors memory
	if (stConfigLangs.iNumElems > 0) {
		for (int i = 0; i < stConfigLangs.iNumElems; i++) {
			for (int j = 0; j < stConfigLangs.pLangDescriptors[i]->iNumEntries; j++) {
				// free each lang entry
				if (stConfigLangs.pLangDescriptors[i]->pLangEntries[j] != NULL) {
					free(stConfigLangs.pLangDescriptors[i]->pLangEntries[j]);
					stConfigLangs.pLangDescriptors[i]->pLangEntries[j] = NULL;

				}
			}

			// Free the list of each lang entry
			if (stConfigLangs.pLangDescriptors[i]->pLangEntries != NULL) {
				free(stConfigLangs.pLangDescriptors[i]->pLangEntries);
				stConfigLangs.pLangDescriptors[i]->pLangEntries = NULL;

			}

			// free each lang descriptor
			if (stConfigLangs.pLangDescriptors[i]) {
				free(stConfigLangs.pLangDescriptors[i]);
				stConfigLangs.pLangDescriptors[i] = NULL;
			}
		}

		if (stConfigLangs.pLangDescriptors) {
			free(stConfigLangs.pLangDescriptors);
			stConfigLangs.pLangDescriptors = NULL;
		}

		stConfigLangs.iNumElems = 0;
	}
}