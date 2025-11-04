// cnskeys.cpp : Define el punto de entrada de la aplicación.
//

#include "framework.h"
#include <shellapi.h>
#include <stdbool.h>
#include <mmsystem.h>
#include "config.h"
#include "cnskeys.h"

#pragma comment(lib, "winmm.lib")


#define MAX_LOADSTRING 100
#define WM_TRAYICON (WM_USER + 1)

// Variables globales:
HINSTANCE hInst;                                // instancia actual
WCHAR szTitle[MAX_LOADSTRING];                  // Texto de la barra de título
WCHAR szWindowClass[MAX_LOADSTRING];            // nombre de clase de la ventana principal
HHOOK hHook;
NOTIFYICONDATAW nidCaps;
NOTIFYICONDATAW nidNum;

static VOID invalidConfigMessageBox();


HICON hIconCapsOn;
HICON hIconCapsOff;
HICON hIconNumOn;
HICON hIconNumOff;
HICON hIconScrollOn;
HICON hIconScrollOff;

enum {
    TRAY_ID_CAPS = 1,
    TRAY_ID_NUM = 2,
    TRAY_ID_SCROLL = 3
};

NOTIFYICONDATAW nidScroll;
bool capsLock = false, numLock = false, scrollLock = false;

void ShowTrayMenu(HWND hwnd, int trayId) {
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, 3001, L"Configurar teclas visibles...");
    AppendMenu(hMenu, MF_STRING, 3002, L"Salir");

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
}


INT_PTR CALLBACK ConfigDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        CheckDlgButton(hDlg, IDC_SHOW_CAPS, getKeyStatus(L"caps") ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_SHOW_NUM, getKeyStatus(L"num") ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_SHOW_SCROLL, getKeyStatus(L"scroll") ? BST_CHECKED : BST_UNCHECKED);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
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


void loadIcons() {
    hIconCapsOn = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_CLICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    hIconCapsOff = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_NCLICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    hIconNumOn = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_NLICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    hIconNumOff = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_NNLICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    hIconScrollOn = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_SLICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    hIconScrollOff = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_NSLICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

}

void unloadIcons() {
    if (hIconCapsOn)    DestroyIcon(hIconCapsOn);
    if (hIconCapsOff)   DestroyIcon(hIconCapsOff);
    if (hIconNumOn)     DestroyIcon(hIconNumOn);
    if (hIconNumOff)     DestroyIcon(hIconNumOff);
    if (hIconScrollOn)  DestroyIcon(hIconScrollOn);
    if (hIconScrollOff)  DestroyIcon(hIconScrollOff);
}

void updateIcon(NOTIFYICONDATAW* nid, LPCWSTR szKeyDescriptor,BOOL bCurrentStatus, HICON hIconKeyOn,HICON hIconKeyOff) {
    BOOL isAdd = FALSE;

    if (!getKeyStatus(szKeyDescriptor)) {
        Shell_NotifyIcon(NIM_DELETE, nid);
        nid->hIcon = NULL;
    }
    else {
        if (nid->hIcon == NULL) isAdd = TRUE;

        nid->hIcon = (bCurrentStatus ? hIconKeyOn : hIconKeyOff);
        Shell_NotifyIcon(isAdd ? NIM_ADD : NIM_MODIFY, nid);
        PlaySound(capsLock ? MAKEINTRESOURCE(IDR_KEYON) : MAKEINTRESOURCE(IDR_KEYOFF), hInst, SND_ASYNC | SND_RESOURCE);
    }
}

void UpdateTrayIcons() {
  	updateIcon(&nidCaps, L"caps",capsLock, hIconCapsOn, hIconCapsOff);
	updateIcon(&nidNum, L"num",numLock, hIconNumOn, hIconNumOff);
	updateIcon(&nidScroll, L"scroll",scrollLock, hIconScrollOn, hIconScrollOff);

}

// Hook de teclado para capturar teclas globales
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        if (wParam == WM_KEYDOWN || wParam == WM_KEYUP) {
            if (p->vkCode == VK_CAPITAL || p->vkCode == VK_NUMLOCK || p->vkCode == VK_SCROLL) {
                capsLock = (GetKeyState(VK_CAPITAL) & 0x0001);
                numLock = (GetKeyState(VK_NUMLOCK) & 0x0001);
                scrollLock = (GetKeyState(VK_SCROLL) & 0x0001);
                UpdateTrayIcons();
            }
        }
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

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

    // Load config file
    if (LoadConfig(NULL) != 0) {
        invalidConfigMessageBox();
        exit(-3);
    }

    // Inicializar cadenas globales
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CNSKEYS, szWindowClass, MAX_LOADSTRING);

    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CNSKEYS));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_CNSKEYS);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    RegisterClassExW(&wcex);
    
    // Realizar la inicialización de la aplicación:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CNSKEYS));

    MSG msg;

    // Bucle principal de mensajes:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
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

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

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

   capsLock = (GetKeyState(VK_CAPITAL) & 0x0001);
   numLock = (GetKeyState(VK_NUMLOCK) & 0x0001);
   scrollLock = (GetKeyState(VK_SCROLL) & 0x0001);

   UpdateTrayIcons();

   ShowWindow(hWnd, nCmdShow);
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
            // Analizar las selecciones de menú:
            switch (wmId)
            {
            case 3001:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_CONFIG_DIALOG), hWnd, ConfigDialogProc);
                UpdateTrayIcons(); // función que agrega/elimina íconos según showCaps/showNum/showScroll
                break;
            case 3002:
                PostQuitMessage(0);
                break;          
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
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
        unloadIcons();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Controlador de mensajes del cuadro Acerca de.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
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

static VOID invalidConfigMessageBox() {
    MessageBox(
        NULL,
        L"Cant load the config file",
        L"Config Error",
        MB_OK
    );
}