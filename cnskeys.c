// cnskeys.c punto de entrada y principal de la aplicacion
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
#define WM_MENUSOUND 		    (WM_USER+3251)

#define ISCAPSLOCKON  (GetKeyState(VK_CAPITAL) & 0x0001)
#define ISNUMLOCKON   (GetKeyState(VK_NUMLOCK) & 0x0001)
#define ISSCROLLLOCKON (GetKeyState(VK_SCROLL) & 0x0001)

// Variables globales:
HINSTANCE hInst;                                // instancia actual
WCHAR szTitle[MAX_LOADSTRING];                  // Texto de la barra de título
WCHAR szWindowClass[MAX_LOADSTRING];            // nombre de clase de la ventana principal
HHOOK hHook;

typedef struct {
	NOTIFYICONDATAW nid;
	HICON hIconOn;
	HICON hIconOff;
	BOOL isVisible;
	UINT keyId;
	BOOL keyStatePrev;
	BOOL added;
} TRAYICON;

static TRAYICON trayCaps, trayNum, trayScroll;


// Declaraciones de funciones adelantadas incluidas en este módulo de código:
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

static VOID invalidConfigMessageBox();
static void EndApp();
INT_PTR CALLBACK ConfigDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static void loadTrayIcons();
static void unloadTrayIcons();
static void TrayIconInit(TRAYICON* t, HWND hWnd, UINT id, UINT keyId);
static void ShowTrayIcons();
static void UpdateTrayIcons();
static void ShowTrayMenu(HWND hwnd, int trayId);
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);



enum {
	TRAY_ID_CAPS = 1,
	TRAY_ID_NUM = 2,
	TRAY_ID_SCROLL = 3
};

static BOOL g_configDialogOpen = FALSE;


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
	if (LoadConfig() != 0) {
		invalidConfigMessageBox();
		exit(-3);
	}


	WNDCLASSEXW wcex = { 0 };

	wcex.cbSize = sizeof(WNDCLASSEXW);

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


	//  comprobar CreateWindowEx
	HWND hWnd = CreateWindowEx(WS_EX_TOOLWINDOW, szWindowClass, szTitle,
		WS_POPUP, -100, -100, 1, 1, NULL, NULL, hInstance, NULL);
	if (!hWnd) {
		DWORD err = GetLastError();
		wchar_t buf[128];
		swprintf_s(buf, _countof(buf), L"CreateWindowEx fallo: %u", err);
		MessageBoxW(NULL, buf, L"Init error", MB_OK | MB_ICONERROR);
			return FALSE;
	}

	// Inicializo variables
	hInst = hInstance; // Almacenar identificador de instancia en una variable global
	loadTrayIcons();

	TrayIconInit(&trayCaps, hWnd, TRAY_ID_CAPS, VK_CAPITAL);
	TrayIconInit(&trayNum, hWnd, TRAY_ID_NUM, VK_NUMLOCK);
	TrayIconInit(&trayScroll, hWnd, TRAY_ID_SCROLL, VK_SCROLL);

	ShowTrayIcons();
	UpdateTrayIcons();

	ShowWindow(hWnd, SW_SHOWNOACTIVATE);
	UpdateWindow(hWnd);


	// Comprobar SetWindowsHookEx
	hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
	if (!hHook) {
		DWORD err = GetLastError();
		OutputDebugStringW(L"Warning: SetWindowsHookEx failed\n");
		// No abortar: la app puede seguir funcionando sin hook; decide según tu política.
	}

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

				POINT pt = { 0 };
				RECT rc;

				// Crear identificador para el icono de la bandeja
				NOTIFYICONIDENTIFIER nii = { 0 };
				nii.cbSize = sizeof(NOTIFYICONIDENTIFIER);
				// Tomo el dato x,y del primer visible
				if (trayCaps.isVisible) {
					nii.hWnd = trayCaps.nid.hWnd;
					nii.uID = trayCaps.nid.uID;
				}
				else if (trayNum.isVisible) {
					nii.hWnd = trayNum.nid.hWnd;
					nii.uID = trayNum.nid.uID;;
				}
				else {
					nii.hWnd = trayScroll.nid.hWnd;
					nii.uID = trayScroll.nid.uID;
				}

				if (SUCCEEDED(Shell_NotifyIconGetRect(&nii, &rc))) {
					pt.x = rc.left;
					pt.y = rc.top;
				}
				else {
					// fallback si no disponible
					GetCursorPos(&pt);
				}

				// Pasamos la dirección de dlgPos en lParam; es válido porque DialogBoxParam es modal
				// no permitimos abrir más de uno a la vez
				g_configDialogOpen = TRUE;
				DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_CONFIG_DIALOG), hWnd, ConfigDialogProc, (LPARAM)&pt);
				g_configDialogOpen = FALSE;

				ShowTrayIcons(); // función que agrega/elimina íconos según showCaps/showNum/showScroll
			}
		}
		else if (wmId == WM_MENUSOUND) {
			// toggle sonido
			BOOL isSoundOn = IsSoundActiveConfig();
			SetSoundActiveConfig(!isSoundOn);
		}
		else if (wmId == WM_MENUEXIT) {
			EndApp();
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
					SetConfigLanguage(pLangs->pLangDescriptors[i]->szLangDescriptor);
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
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		// salida al apagarse el equipo  , cerrar sesion, desde el adm de tareas,etc
		EndApp();
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

	// Eliminar íconos del area de notificación ANTES de destruir HICON
	if (trayCaps.added) Shell_NotifyIconW(NIM_DELETE, &trayCaps.nid);
	if (trayNum.added)  Shell_NotifyIconW(NIM_DELETE, &trayNum.nid);
	if (trayScroll.added) Shell_NotifyIconW(NIM_DELETE, &trayScroll.nid);


	unloadTrayIcons();

	// Eliminar hook si existe
	if (hHook) {
		UnhookWindowsHookEx(hHook);
		hHook = NULL;
	}

	freeLangsConfig();
}

static void EndApp() {
	// actualizamos las globales del config previo a grabar
	SetKeyStatusConfig(L"caps", trayCaps.isVisible);
	SetKeyStatusConfig(L"num", trayNum.isVisible);
	SetKeyStatusConfig(L"scroll", trayScroll.isVisible);

	if (SaveConfig()) {
		// Config failed , nada que hacer quedara como esta
		OutputDebugStringW(L"Warning: SaveConfig failed\n");
	}

	DestroyApp();
	PostQuitMessage(0);
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
	
	// agregamos el toggle de sonidos
	UINT soundFlags = MF_STRING;
	if (IsSoundActiveConfig()) {
		soundFlags |= MF_CHECKED;
	}
	else {
		soundFlags |= MF_UNCHECKED;
	}
	AppendMenu(hMenu, soundFlags, WM_MENUSOUND,getLanguageMessage(pLang->szLangDescriptor, L"traymenu_sound"));

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
static void loadTrayIcons() {
	trayCaps.hIconOn= (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_CLICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	trayCaps.hIconOff = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_NCLICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	trayNum.hIconOn = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_NLICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	trayNum.hIconOff = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_NNLICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	trayScroll.hIconOn = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_SLICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	trayScroll.hIconOff = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_NSLICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

}

static void unloadTrayIcons() {
	if (trayCaps.hIconOn)    DestroyIcon(trayCaps.hIconOn);
	if (trayCaps.hIconOff)   DestroyIcon(trayCaps.hIconOff);
	if (trayNum.hIconOn)     DestroyIcon(trayNum.hIconOn);
	if (trayNum.hIconOff)     DestroyIcon(trayNum.hIconOff);
	if (trayScroll.hIconOn)  DestroyIcon(trayScroll.hIconOn);
	if (trayScroll.hIconOff)  DestroyIcon(trayScroll.hIconOff);
}

static void TrayIconInit(TRAYICON* t, HWND hWnd, UINT id,  UINT keyId) {
	if (!t) return;
	//ZeroMemory(t, sizeof(*t));
	t->nid.cbSize = sizeof(NOTIFYICONDATAW);
	t->nid.hWnd = hWnd;
	t->nid.uID = id;
	t->nid.uCallbackMessage = WM_TRAYICON;
	t->nid.uFlags = NIF_ICON | NIF_MESSAGE;
	t->isVisible = GetKeyStatusConfig(id == TRAY_ID_CAPS ? L"caps" : id == TRAY_ID_NUM ? L"num" : L"scroll");
	t->keyId = keyId;
	t->keyStatePrev = FALSE;
	t->added = FALSE;
}

static void ShowTrayIconStatus(TRAYICON* ti) {
	if (ti->isVisible) {
		BOOL bCurrentStatus = (GetKeyState(ti->keyId) & 0x0001);
		
		ti->nid.hIcon = bCurrentStatus ? ti->hIconOn: ti->hIconOff;

		if (!ti->added) {
			if (Shell_NotifyIconW(NIM_ADD, &ti->nid))
				ti->added = TRUE;
			else {
				DWORD err = GetLastError();
				OutputDebugStringW(L"Shell_NotifyIcon NIM_ADD falló\n");
			}
		}
		else {
			Shell_NotifyIconW(NIM_MODIFY, &ti->nid);
		}
	}
	else {
		if (ti->added) {
			Shell_NotifyIconW(NIM_DELETE, &ti->nid);
			ti->added = FALSE;
		}
		ti->nid.hIcon = NULL;
	}
}

static void ShowTrayIcons() {
	ShowTrayIconStatus(&trayCaps);
	ShowTrayIconStatus(&trayNum);	
	ShowTrayIconStatus(&trayScroll);
}

static void UpdateTrayIcon(TRAYICON* ti) {
	if (ti->isVisible) {

		BOOL bCurrentStatus = (GetKeyState(ti->keyId) & 0x0001);
		ti->nid.hIcon = (bCurrentStatus ? ti->hIconOn : ti->hIconOff);
		Shell_NotifyIcon(NIM_MODIFY, &ti->nid);
		if (IsSoundActiveConfig()) {
			PlaySound(bCurrentStatus ? MAKEINTRESOURCE(IDR_KEYON) : MAKEINTRESOURCE(IDR_KEYOFF), hInst, SND_ASYNC | SND_RESOURCE);
		}
	}
}


static void UpdateTrayIcons() {
	if (ISCAPSLOCKON != trayCaps.keyStatePrev) {
		UpdateTrayIcon(&trayCaps);
		trayCaps.keyStatePrev  = ISCAPSLOCKON;
	}
	if (ISNUMLOCKON != trayNum.keyStatePrev) {
		UpdateTrayIcon(&trayNum);
		trayNum.keyStatePrev = ISNUMLOCKON;
	}
	if (ISSCROLLLOCKON != trayScroll.keyStatePrev) {
		UpdateTrayIcon(&trayScroll);
		trayScroll.keyStatePrev = ISSCROLLLOCKON;
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


		CheckDlgButton(hDlg, IDC_SHOW_CAPS, trayCaps.isVisible ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_SHOW_NUM, trayNum.isVisible ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_SHOW_SCROLL, trayScroll.isVisible ? BST_CHECKED : BST_UNCHECKED);
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

			trayCaps.isVisible = IsDlgButtonChecked(hDlg, IDC_SHOW_CAPS);
			trayCaps.keyStatePrev = ISCAPSLOCKON;

			trayNum.isVisible = IsDlgButtonChecked(hDlg, IDC_SHOW_NUM);
			trayNum.keyStatePrev = ISNUMLOCKON; // actualizar estado previo para forzar repintado

			trayScroll.isVisible = IsDlgButtonChecked(hDlg, IDC_SHOW_SCROLL);
			trayScroll.keyStatePrev = ISSCROLLLOCKON; // actualizar estado previo para forzar repintado

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
