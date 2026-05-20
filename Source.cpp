#pragma comment(linker, "/SUBSYSTEM:windows")
#include <windows.h>
#include <vector>
#include <thread>

HWND hPrimaryWnd = NULL;
HBRUSH hDarkThemeBrush = NULL;

LRESULT CALLBACK MainInterfaceHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
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