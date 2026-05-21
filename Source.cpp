#pragma comment(linker, "/SUBSYSTEM:windows")

#include <windows.h>
#include <vector>
#include <thread>

#define BTN_START_REC 1
#define BTN_STOP_REC 2
#define BTN_SET_HOTKEY 3

HWND hPrimaryWnd = NULL;
HBRUSH hDarkThemeBrush = NULL;

LRESULT CALLBACK MainInterfaceHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        CreateWindowW(L"STATIC", L"--- МАКРОСИ ---", WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 10, 280, 20, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"BUTTON", L"🔴 Почати запис", WS_VISIBLE | WS_CHILD, 60, 40, 180, 30, hwnd, (HMENU)BTN_START_REC, NULL, NULL);
        CreateWindowW(L"BUTTON", L"⬛ Зупинити запис", WS_VISIBLE | WS_CHILD, 60, 80, 180, 30, hwnd, (HMENU)BTN_STOP_REC, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Призначити клавішу (Бінд)", WS_VISIBLE | WS_CHILD, 50, 120, 200, 30, hwnd, (HMENU)BTN_SET_HOTKEY, NULL, NULL);
        break;

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, RGB(0, 200, 255));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LRESULT)hDarkThemeBrush;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case BTN_START_REC:
            SetWindowTextW(hwnd, L"Інструменти Гравця [ЙДЕ ЗАПИС...]");
            break;
        case BTN_STOP_REC:
            SetWindowTextW(hwnd, L"Інструменти Гравця [Записано]");
            break;
        case BTN_SET_HOTKEY:
            SetWindowTextW(GetDlgItem(hwnd, BTN_SET_HOTKEY), L"Натисніть клавішу...");
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    hDarkThemeBrush = CreateSolidBrush(RGB(15, 15, 30));

    WNDCLASSW mainWndClass = { 0 };
    mainWndClass.lpfnWndProc = MainInterfaceHandler;
    mainWndClass.hInstance = hInstance;
    mainWndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    mainWndClass.hbrBackground = hDarkThemeBrush;
    mainWndClass.lpszClassName = L"MainAppClass";
    RegisterClassW(&mainWndClass);

    hPrimaryWnd = CreateWindowW(L"MainAppClass", L"Інструменти Гравця",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        100, 100, 320, 390, NULL, NULL, hInstance, NULL);

    ShowWindow(hPrimaryWnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DeleteObject(hDarkThemeBrush);
    return 0;
}