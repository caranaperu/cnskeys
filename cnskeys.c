// cnskeys.cpp : Define el punto de entrada de la aplicación.
//

#include "framework.h"
#include <shellapi.h>
#include <stdbool.h>
#include <mmsystem.h>
#include "config.h"
#include "lang.h"
#include "cnskeys.h"

#pragma comment(lib, "winmm.lib")


#define MAX_LOADSTRING 100
#define WM_TRAYICON (WM_USER + 1)

#define WM_MENUCONFIGUREKEYS 	(WM_USER+3000)
#define WM_MENUABOUT		    (WM_USER+3001)
#define WM_MENUEXIT		        (WM_USER+3002)
#define WM_MENULANGFIRST		(WM_USER+3003)
#define WM_MENULANGLAST			(WM_USER+3250) // mas que suficiente

// Variables globales:
HINSTANCE hInst;                                // instancia actual
WCHAR szTitle[MAX_LOADSTRING];                  // Texto de la barra de título
WCHAR szWindowClass[MAX_LOADSTRING];            // nombre de clase de la ventana principal
HHOOK hHook;
NOTIFYICONDATAW nidCaps;
NOTIFYICONDATAW nidNum;
NOTIFYICONDATAW nidScroll;

HICON hIconCapsOn;
HICON hIconCapsOff;
HICON hIconNumOn;
HICON hIconNumOff;
HICON hIconScrollOn;
HICON hIconScrollOff;

static VOID invalidConfigMessageBox();
static void DestroyApp();
INT_PTR CALLBACK ConfigDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static void loadIcons();
static void unloadIcons();
static void ShowTrayIcons();
static void UpdateTrayIcons();
static void ShowTrayMenu(HWND hwnd, int trayId);
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);



enum {
	TRAY_ID_CAPS = 1,
	TRAY_ID_NUM = 2,
	TRAY_ID_SCROLL = 3
};

bool capsLock = false, numLock = false, scrollLock = false;
bool capsLockPrev = false, numLockPrev = false, scrollLockPrev = false;

static BOOL g_configDialogOpen = FALSE;

// Declaraciones de funciones adelantadas incluidas en este módulo de código:
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// Inicializar cadenas globales
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_CNSKEYS, szWindowClass, MAX_LOADSTRING);

	if (FindWindowW(szWindowClass, NULL)) {
		// Ya hay una instancia corriendo
		return FALSE;
	}


	// Load config file
	if (LoadConfig(NULL) != 0) {
		invalidConfigMessageBox();
		exit(-3);
	}


	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CNSKEYS));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_CNSKEYS);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	RegisterClassExW(&wcex);

	// Realizar la inicialización de la aplicación:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CNSKEYS));

	MSG msg;

	// Bucle principal de mensajes:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}


//
//   FUNCIÓN: InitInstance(HINSTANCE, int)
//
//   PROPÓSITO: Guarda el identificador de instancia y crea la ventana principal
//
//   COMENTARIOS:
//
//        En esta función, se guarda el identificador de instancia en una variable común y
//        se crea y muestra la ventana principal del programa.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{


	hInst = hInstance; // Almacenar identificador de instancia en una variable global

	HWND hWnd = CreateWindowEx( WS_EX_TOOLWINDOW, szWindowClass, szTitle,
		WS_POPUP,
		-100, -100, 1, 1, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}


	nidCaps.cbSize = sizeof(NOTIFYICONDATA);
	nidCaps.hWnd = hWnd;
	nidCaps.uCallbackMessage = WM_TRAYICON;
	nidCaps.uID = TRAY_ID_CAPS;
	nidCaps.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;

	nidNum.cbSize = sizeof(NOTIFYICONDATA);
	nidNum.hWnd = hWnd;
	nidNum.uCallbackMessage = WM_TRAYICON;
	nidNum.uID = TRAY_ID_NUM;
	nidNum.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;


	nidScroll.cbSize = sizeof(NOTIFYICONDATA);
	nidScroll.hWnd = hWnd;
	nidScroll.uCallbackMessage = WM_TRAYICON;
	nidScroll.uID = TRAY_ID_SCROLL;
	nidScroll.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;

	loadIcons();

	ShowTrayIcons(TRUE);
	UpdateTrayIcons();

	ShowWindow(hWnd, SW_SHOWNOACTIVATE);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  FUNCIÓN: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PROPÓSITO: Procesa mensajes de la ventana principal.
//
//  WM_COMMAND  - procesar el menú de aplicaciones
//  WM_PAINT    - Pintar la ventana principal
//  WM_DESTROY  - publicar un mensaje de salida y volver
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_TRAYICON:
		switch (LOWORD(wParam)) {
		case TRAY_ID_CAPS:
		case TRAY_ID_NUM:
		case TRAY_ID_SCROLL:
			if (lParam == WM_RBUTTONUP)
				ShowTrayMenu(hWnd, LOWORD(wParam));
			break;
		}
		break;

	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Analizar las selecciones de menú

		if (wmId == WM_MENUCONFIGUREKEYS) {
			if (!g_configDialogOpen) {

				POINT pt;
				RECT rc;

				// Crear identificador para el icono de la bandeja
				NOTIFYICONIDENTIFIER nii = { 0 };
				nii.cbSize = sizeof(NOTIFYICONIDENTIFIER);
				// Tomo el dato x,y del primer visible
				if (getKeyStatusConfig(L"caps")) {
					nii.hWnd = nidCaps.hWnd;
					nii.uID = nidCaps.uID;
				}
				else if (getKeyStatusConfig(L"num")) {
					nii.hWnd = nidNum.hWnd;
					nii.uID = nidNum.uID;
				}
				else {
					nii.hWnd = nidScroll.hWnd;
					nii.uID = nidScroll.uID;
				}

				Shell_NotifyIconGetRect(&nii, &rc);
				pt.x = rc.left;
				pt.y = rc.top;

				// Pasamos la dirección de dlgPos en lParam; es válido porque DialogBoxParam es modal
				// no permitimos abrir más de uno a la vez
				g_configDialogOpen = TRUE;
				DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_CONFIG_DIALOG), hWnd, ConfigDialogProc, (LPARAM)&pt);
				g_configDialogOpen = FALSE;

				ShowTrayIcons(FALSE); // función que agrega/elimina íconos según showCaps/showNum/showScroll
			}
		}
		else if (wmId == WM_MENUEXIT) {
			DestroyApp();
			PostQuitMessage(0);
		}
		else if (wmId == IDM_ABOUT) {
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
		}
		else if (wmId >= WM_MENULANGFIRST && wmId <= WM_MENULANGLAST) {
			// Cambio de lenguaje
			int langId = wmId - WM_MENULANGFIRST;
			CONFIG_LANGS* pLangs = getLanguages();
			for (int i = 0; i < pLangs->iNumElems; i++) {
				if (pLangs->pLangDescriptors[i]->iId == langId) {
					setDefaultLanguage(pLangs->pLangDescriptors[i]->szLangDescriptor);
					break;
				}
			}
		}
		else {
			return DefWindowProc(hWnd, message, wParam, lParam);
		}

	}
	break;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: Agregar cualquier código de dibujo que use hDC aquí...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		DestroyApp();
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

/*
*
* Helpers 
*
*/
// Hook de teclado para capturar teclas globales
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION) {
		KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
		if (wParam == WM_KEYDOWN || wParam == WM_KEYUP) {
			if (p->vkCode == VK_CAPITAL || p->vkCode == VK_NUMLOCK || p->vkCode == VK_SCROLL) {
				//capsLock = (GetKeyState(VK_CAPITAL) & 0x0001);
				//numLock = (GetKeyState(VK_NUMLOCK) & 0x0001);
				//scrollLock = (GetKeyState(VK_SCROLL) & 0x0001);
				UpdateTrayIcons();
			}
		}
	}
	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

static VOID invalidConfigMessageBox() {
	MessageBox(
		NULL,
		L"Cant load the config file",
		L"Config Error",
		MB_OK
	);
}

static void DestroyApp() {
	unloadIcons();
	freeLangsConfig();
}

/*
*
* Tray popup menu
*
*/
static void ShowTrayMenu(HWND hwnd, int trayId) {
	POINT pt;
	GetCursorPos(&pt);

	HMENU hMenu = CreatePopupMenu();

	CONFIG_LANGDESC* pLang = getDefaultLanguage();

	AppendMenu(hMenu, MF_STRING, WM_MENUCONFIGUREKEYS, getLanguageMessage(pLang->szLangDescriptor, L"traymenu_configure"));
	HMENU hSubMenuLanguage = CreatePopupMenu();
	CONFIG_LANGS* pLangs = getLanguages();
	for (int i = 0; i < pLangs->iNumElems; i++) {
		AppendMenu(hSubMenuLanguage, MF_STRING, WM_MENULANGFIRST + (UINT_PTR)pLangs->pLangDescriptors[i]->iId, pLangs->pLangDescriptors[i]->szLangName);
	}
	AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSubMenuLanguage, getLanguageMessage(pLang->szLangDescriptor, L"traymenu_language"));
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING, IDM_ABOUT, getLanguageMessage(pLang->szLangDescriptor, L"traymenu_about"));
	AppendMenu(hMenu, MF_STRING, WM_MENUEXIT, getLanguageMessage(pLang->szLangDescriptor, L"traymenu_exit"));



	SetForegroundWindow(hwnd);
	TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
	DestroyMenu(hMenu);
}

/*
*
* Icon managing
*
*/
static void loadIcons() {
	hIconCapsOn = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_CLICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	hIconCapsOff = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_NCLICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	hIconNumOn = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_NLICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	hIconNumOff = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_NNLICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	hIconScrollOn = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_SLICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	hIconScrollOff = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_NSLICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

}

static void unloadIcons() {
	if (hIconCapsOn)    DestroyIcon(hIconCapsOn);
	if (hIconCapsOff)   DestroyIcon(hIconCapsOff);
	if (hIconNumOn)     DestroyIcon(hIconNumOn);
	if (hIconNumOff)     DestroyIcon(hIconNumOff);
	if (hIconScrollOn)  DestroyIcon(hIconScrollOn);
	if (hIconScrollOff)  DestroyIcon(hIconScrollOff);
}

static void ShowIconStatus(NOTIFYICONDATAW* nid, BOOL bShow, HICON hIcon) {
	if (bShow) {
		nid->hIcon = hIcon;
		Shell_NotifyIcon(NIM_ADD, nid);
	}
	else {
		Shell_NotifyIcon(NIM_DELETE, nid);
		nid->hIcon = NULL;
	}
}

static void ShowTrayIcons(BOOL isInit) {
	capsLock = (GetKeyState(VK_CAPITAL) & 0x0001);
	numLock = (GetKeyState(VK_NUMLOCK) & 0x0001);
	scrollLock = (GetKeyState(VK_SCROLL) & 0x0001);

	ShowIconStatus(&nidCaps, getKeyStatusConfig(L"caps"), capsLock ? hIconCapsOn : hIconCapsOff);
	ShowIconStatus(&nidNum, getKeyStatusConfig(L"num"), numLock ? hIconNumOn : hIconNumOff);
	ShowIconStatus(&nidScroll, getKeyStatusConfig(L"scroll"), scrollLock ? hIconScrollOn : hIconScrollOff);
}


static void updateIcon(NOTIFYICONDATAW* nid, LPCWSTR szKeyDescriptor, BOOL bCurrentStatus, HICON hIconKeyOn, HICON hIconKeyOff) {
	nid->hIcon = (bCurrentStatus ? hIconKeyOn : hIconKeyOff);
	Shell_NotifyIcon(NIM_MODIFY, nid);
	PlaySound(bCurrentStatus ? MAKEINTRESOURCE(IDR_KEYON) : MAKEINTRESOURCE(IDR_KEYOFF), hInst, SND_ASYNC | SND_RESOURCE);
}


static void UpdateTrayIcons() {
	capsLock = (GetKeyState(VK_CAPITAL) & 0x0001);
	numLock = (GetKeyState(VK_NUMLOCK) & 0x0001);
	scrollLock = (GetKeyState(VK_SCROLL) & 0x0001);

	if (capsLock != capsLockPrev) {
		if(getKeyStatusConfig(L"caps"))
			updateIcon(&nidCaps, L"caps", capsLock, hIconCapsOn, hIconCapsOff);
		capsLockPrev = capsLock;
	}
	if (numLock != numLockPrev) {
		if(getKeyStatusConfig(L"num"))
			updateIcon(&nidNum, L"num", numLock, hIconNumOn, hIconNumOff);
		numLockPrev = numLock;
	}
	if (scrollLock != scrollLockPrev) {
		if (getKeyStatusConfig(L"scroll"))
			updateIcon(&nidScroll, L"scroll", scrollLock, hIconScrollOn, hIconScrollOff);
		scrollLockPrev = scrollLock;
	}
}

/*
*
* Config dialog 
*
*/
static void ApplyConfigToDialog(HWND hDlg) {
	CONFIG_LANGDESC* pLang = getDefaultLanguage();

	SetWindowTextW(hDlg, getLanguageMessage(pLang->szLangDescriptor, L"dialog_title"));

	SetDlgItemTextW(hDlg, IDC_SHOW_CAPS, getLanguageMessage(pLang->szLangDescriptor, L"dialog_showcaps"));
	SetDlgItemTextW(hDlg, IDC_SHOW_NUM, getLanguageMessage(pLang->szLangDescriptor, L"dialog_shownum"));
	SetDlgItemTextW(hDlg, IDC_SHOW_SCROLL, getLanguageMessage(pLang->szLangDescriptor, L"dialog_showscroll"));

	SetDlgItemTextW(hDlg, IDOK, getLanguageMessage(pLang->szLangDescriptor, L"dialog_btnok"));
	SetDlgItemTextW(hDlg, IDCANCEL, getLanguageMessage(pLang->szLangDescriptor, L"dialog_btncancel"));
}


INT_PTR CALLBACK ConfigDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_INITDIALOG:
		// Seteamos los textos acordes al lenguaje
		ApplyConfigToDialog(hDlg);

		// Si el llamador pasó un POINT (coordenadas en pantalla) en lParam, reposicionar el diálogo
		if (lParam) {
			POINT* pPos = (POINT*)lParam;
			RECT rcDlg;
			// leo el tamano actual del dialog box
			if (GetWindowRect(hDlg, &rcDlg)) {
				int width = rcDlg.right - rcDlg.left; // ancho
				int height = rcDlg.bottom - rcDlg.top; // alto

				// Area de trabajo del monitor donde se encuentra el cursor
				// el punto es enviado por el llamador
				HMONITOR hm = MonitorFromPoint(*pPos, MONITOR_DEFAULTTONEAREST);
				MONITORINFO mi = { 0 };
				mi.cbSize = sizeof(mi);
				RECT work = { 0 };
				if (GetMonitorInfoW(hm, &mi)) work = mi.rcWork;
				else SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0); // fallback

				// Intentar obtener rect de la taskbar (APPBAR). Si falla, usar FindWindow fallback.
				RECT taskRect = { 0 };
				UINT edge = ABE_BOTTOM;
				APPBARDATA abd = { sizeof(abd) };
				if (SHAppBarMessage(ABM_GETTASKBARPOS, &abd)) {
					taskRect = abd.rc;
					edge = abd.uEdge;
				}
				else {
					HWND hTray = FindWindowW(L"Shell_TrayWnd", NULL);
					if (hTray && GetWindowRect(hTray, &taskRect)) {
						// aproximar edge según posición relativa al work area
						if (taskRect.left <= work.left && taskRect.right >= work.right) {
							edge = (taskRect.top <= work.top) ? ABE_TOP : ABE_BOTTOM;
						}
						else {
							edge = (taskRect.left <= work.left) ? ABE_LEFT : ABE_RIGHT;
						}
					}
					else {
						// no taskbar info -> usar work area como referencia (no hay ajuste adicional)
						taskRect = work;
					}
				}

				// Posición propuesta (viene del llamador)
				int x = pPos->x;
				int y = pPos->y;

				// Ajustes según edge de la taskbar para evitar solapamiento
				// basicamente es para windows 10 ya que en win 11 no se puede cambiar 
				// la posicion de la barra de tareas en forma sencilla ya que se requieren
				// cambios en el registro o utilitarios de terceros para moverla a los lados o arriba.
				// Aun asi se toman las precauciones por si el usuario tiene win10 o versiones anteriores
				// o usa algun utilitario para mover la barra de tareas.
				if (edge == ABE_BOTTOM) {
					if (y + height > taskRect.top) {
						// intentar colocar encima del icono
						y = pPos->y - height;
					}
				}
				else if (edge == ABE_TOP) {
					if (y < taskRect.bottom) {
						// colocar debajo de la taskbar
						y = taskRect.bottom;
					}
				}
				else if (edge == ABE_LEFT) {
					if (x < taskRect.right) {
						x = taskRect.right;
					}
				}
				else if (edge == ABE_RIGHT) {
					if (x + width > taskRect.left) {
						x = taskRect.left - width;
					}
				}

				// Clamp dentro del work area (para que no quede fuera de pantalla)
				if (x + width > work.right) x = work.right - width;
				if (x < work.left) x = work.left;
				if (y + height > work.bottom) y = work.bottom - height;
				if (y < work.top) y = work.top;

				SetWindowPos(hDlg, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
			}
		}


		CheckDlgButton(hDlg, IDC_SHOW_CAPS, getKeyStatusConfig(L"caps") ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_SHOW_NUM, getKeyStatusConfig(L"num") ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_SHOW_SCROLL, getKeyStatusConfig(L"scroll") ? BST_CHECKED : BST_UNCHECKED);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			if (IsDlgButtonChecked(hDlg, IDC_SHOW_CAPS) == BST_UNCHECKED &&
				IsDlgButtonChecked(hDlg, IDC_SHOW_NUM) == BST_UNCHECKED &&
				IsDlgButtonChecked(hDlg, IDC_SHOW_SCROLL) == BST_UNCHECKED) {
				// Al menos uno debe estar seleccionado
				CONFIG_LANGDESC* pLang = getDefaultLanguage();
				MessageBoxW(hDlg, getLanguageMessage(pLang->szLangDescriptor, L"dialog_err_atleastone"), getLanguageMessage(pLang->szLangDescriptor, L"dialog_err_title"), MB_OK | MB_ICONERROR);
				return TRUE;
			}

			setKeyStatus(L"caps", IsDlgButtonChecked(hDlg, IDC_SHOW_CAPS));
			setKeyStatus(L"num", IsDlgButtonChecked(hDlg, IDC_SHOW_NUM));
			setKeyStatus(L"scroll", IsDlgButtonChecked(hDlg, IDC_SHOW_SCROLL));
			EndDialog(hDlg, IDOK);
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		break;
	}
	return FALSE;
}


/*
* 
* About dialog 
* 
*/
static void ApplyConfigToAboutDialog(HWND hDlg) {
	CONFIG_LANGDESC* pLang = getDefaultLanguage();

	SetWindowTextW(hDlg, getLanguageMessage(pLang->szLangDescriptor, L"about_title"));

	SetDlgItemTextW(hDlg, IDC_STRVERSION, getLanguageMessage(pLang->szLangDescriptor, L"about_version"));
	SetDlgItemTextW(hDlg, IDC_STRCOPYRIGHT, getLanguageMessage(pLang->szLangDescriptor, L"about_copyright"));

	SetDlgItemTextW(hDlg, IDOK, getLanguageMessage(pLang->szLangDescriptor, L"dialog_btnok"));
}

// Controlador de mensajes del cuadro Acerca de.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		ApplyConfigToAboutDialog(hDlg);

		// Reposicionamos el dialogo en el centro del monitor donde esta el cursor
		POINT pos;
		GetCursorPos(&pos);

		RECT rcDlg;
		// leo el tamano actual del dialog box
		if (GetWindowRect(hDlg, &rcDlg)) {
			int width = rcDlg.right - rcDlg.left; // ancho
			int height = rcDlg.bottom - rcDlg.top; // alto

			// Area de trabajo del monitor donde se encuentra el cursor
			// el punto es enviado por el llamador
			HMONITOR hm = MonitorFromPoint(pos, MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi = { 0 };
			mi.cbSize = sizeof(mi);
			RECT work = { 0 };
			if (GetMonitorInfoW(hm, &mi)) work = mi.rcWork;
			else SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0); // fallback

			// lo centramos ahora
			int x = ((work.right - work.left) - width) / 2;
			int y = ((work.bottom - work.top) - height) / 2;

			SetWindowPos(hDlg, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

		}
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
