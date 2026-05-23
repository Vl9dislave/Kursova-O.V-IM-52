#pragma comment(linker, "/SUBSYSTEM:windows")

#include <windows.h>
#include <vector>
#include <thread>

#define BTN_START_REC 1
#define BTN_STOP_REC 2
#define BTN_SET_HOTKEY 3
#define BTN_TOGGLE_AIM 4
#define RAD_CROSS 5
#define RAD_DOT 6
#define RAD_CIRCLE 7
#define BTN_SIZE_MINUS 8
#define BTN_SIZE_PLUS 9
#define RAD_T_CROSS 10

struct UserKeyAction {
    WORD vKey;
    DWORD keyFlags;
    DWORD waitTimeMs;
};

HWND hPrimaryWnd = NULL;
HWND hOverlayWnd = NULL;
HHOOK hGlobalKeyHook = NULL;
HBRUSH hDarkThemeBrush = NULL;

std::vector<UserKeyAction> savedActions;
bool bIsCapturing = false;
bool bAwaitingHotkey = false;
DWORD activationKey = 0;
ULONGLONG previousTick = 0;

int crossType = 0;
int crossSize = 30;

void ExecuteMacro() {
    if (savedActions.empty()) return;

    for (const auto& action : savedActions) {
        Sleep(action.waitTimeMs);

        INPUT simInput = { 0 };
        simInput.type = INPUT_KEYBOARD;
        simInput.ki.wVk = action.vKey;
        simInput.ki.dwFlags = action.keyFlags;

        SendInput(1, &simInput, sizeof(INPUT));
    }
}

LRESULT CALLBACK LowLevelKeyHandler(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* pKeyData = (KBDLLHOOKSTRUCT*)lParam;

        if (bAwaitingHotkey && wParam == WM_KEYDOWN) {
            activationKey = pKeyData->vkCode;
            bAwaitingHotkey = false;
            SetWindowTextW(GetDlgItem(hPrimaryWnd, BTN_SET_HOTKEY), L"Клавіша призначена!");
            return 1;
        }

        if (bIsCapturing) {
            ULONGLONG currentTick = GetTickCount64();
            DWORD delay = (previousTick == 0) ? 0 : (DWORD)(currentTick - previousTick);
            previousTick = currentTick;

            UserKeyAction newAction;
            newAction.vKey = (WORD)pKeyData->vkCode;
            newAction.keyFlags = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) ? KEYEVENTF_KEYUP : 0;
            newAction.waitTimeMs = delay;

            savedActions.push_back(newAction);
        }

        if (!bIsCapturing && !bAwaitingHotkey && pKeyData->vkCode == activationKey && wParam == WM_KEYDOWN) {
            std::thread(ExecuteMacro).detach();
        }
    }
    return CallNextHookEx(hGlobalKeyHook, nCode, wParam, lParam);
}

LRESULT CALLBACK OverlayWindowHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
        HBRUSH hBrush = CreateSolidBrush(RGB(0, 255, 0));

        HGDIOBJ hOldPen = SelectObject(hdc, hPen);
        HGDIOBJ hOldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));

        int cx = 100;
        int cy = 100;
        int r = crossSize / 2;
        int gap = max(2, r / 3);

        if (crossType == 0 || crossType == 3) {
            MoveToEx(hdc, cx - r, cy, NULL); LineTo(hdc, cx - gap, cy);
            MoveToEx(hdc, cx + gap, cy, NULL); LineTo(hdc, cx + r, cy);
            MoveToEx(hdc, cx, cy + gap, NULL); LineTo(hdc, cx, cy + r);
            if (crossType == 0) {
                MoveToEx(hdc, cx, cy - r, NULL); LineTo(hdc, cx, cy - gap);
            }
        }
        else if (crossType == 1) {
            SelectObject(hdc, hBrush);
            int dotR = max(2, r / 2);
            Ellipse(hdc, cx - dotR, cy - dotR, cx + dotR, cy + dotR);
        }
        else if (crossType == 2) {
            Ellipse(hdc, cx - r, cy - r, cx + r, cy + r);
        }

        SelectObject(hdc, hOldBrush);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        DeleteObject(hBrush);

        EndPaint(hwnd, &ps);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void SwitchCrosshairState() {
    if (hOverlayWnd) {
        DestroyWindow(hOverlayWnd);
        hOverlayWnd = NULL;
    }
    else {
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);

        hOverlayWnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
            L"OverlayClass", L"", WS_POPUP | WS_VISIBLE,
            (screenW - 200) / 2, (screenH - 200) / 2, 200, 200,
            NULL, NULL, GetModuleHandle(NULL), NULL
        );
        SetLayeredWindowAttributes(hOverlayWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    }
}

LRESULT CALLBACK MainInterfaceHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        CreateWindowW(L"STATIC", L"--- МАКРОСИ ---", WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 10, 280, 20, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"BUTTON", L"🔴 Почати запис", WS_VISIBLE | WS_CHILD, 60, 40, 180, 30, hwnd, (HMENU)BTN_START_REC, NULL, NULL);
        CreateWindowW(L"BUTTON", L"⬛ Зупинити запис", WS_VISIBLE | WS_CHILD, 60, 80, 180, 30, hwnd, (HMENU)BTN_STOP_REC, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Призначити клавішу (Бінд)", WS_VISIBLE | WS_CHILD, 50, 120, 200, 30, hwnd, (HMENU)BTN_SET_HOTKEY, NULL, NULL);

        CreateWindowW(L"STATIC", L"--- ПЕРЕХРЕСТЯ ---", WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 170, 280, 20, hwnd, NULL, NULL, NULL);

        HWND hRad1 = CreateWindowW(L"BUTTON", L"Хрест (центр)", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP, 25, 195, 120, 20, hwnd, (HMENU)RAD_CROSS, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Без верху", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON, 155, 195, 120, 20, hwnd, (HMENU)RAD_T_CROSS, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Крапка", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON, 25, 220, 120, 20, hwnd, (HMENU)RAD_DOT, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Коло", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON, 155, 220, 120, 20, hwnd, (HMENU)RAD_CIRCLE, NULL, NULL);

        SendMessage(hRad1, BM_SETCHECK, BST_CHECKED, 0);

        CreateWindowW(L"BUTTON", L"- Зменшити", WS_VISIBLE | WS_CHILD, 40, 255, 100, 30, hwnd, (HMENU)BTN_SIZE_MINUS, NULL, NULL);
        CreateWindowW(L"BUTTON", L"+ Збільшити", WS_VISIBLE | WS_CHILD, 160, 255, 100, 30, hwnd, (HMENU)BTN_SIZE_PLUS, NULL, NULL);

        CreateWindowW(L"BUTTON", L"👁 Показати/Сховати приціл", WS_VISIBLE | WS_CHILD, 50, 300, 200, 30, hwnd, (HMENU)BTN_TOGGLE_AIM, NULL, NULL);
        break;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, RGB(0, 200, 255));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LRESULT)hDarkThemeBrush;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case BTN_START_REC:
            savedActions.clear();
            previousTick = 0;
            bIsCapturing = true;
            SetWindowTextW(hwnd, L"Інструменти Гравця [ЙДЕ ЗАПИС...]");
            break;
        case BTN_STOP_REC:
            bIsCapturing = false;
            SetWindowTextW(hwnd, L"Інструменти Гравця [Записано]");
            break;
        case BTN_SET_HOTKEY:
            bAwaitingHotkey = true;
            SetWindowTextW(GetDlgItem(hwnd, BTN_SET_HOTKEY), L"Натисніть клавішу...");
            break;
        case BTN_TOGGLE_AIM:
            SwitchCrosshairState();
            break;
        case RAD_CROSS:
            crossType = 0;
            if (hOverlayWnd) InvalidateRect(hOverlayWnd, NULL, TRUE);
            break;
        case RAD_DOT:
            crossType = 1;
            if (hOverlayWnd) InvalidateRect(hOverlayWnd, NULL, TRUE);
            break;
        case RAD_CIRCLE:
            crossType = 2;
            if (hOverlayWnd) InvalidateRect(hOverlayWnd, NULL, TRUE);
            break;
        case RAD_T_CROSS:
            crossType = 3;
            if (hOverlayWnd) InvalidateRect(hOverlayWnd, NULL, TRUE);
            break;
        case BTN_SIZE_MINUS:
            if (crossSize > 10) crossSize -= 10;
            if (hOverlayWnd) InvalidateRect(hOverlayWnd, NULL, TRUE);
            break;
        case BTN_SIZE_PLUS:
            if (crossSize < 180) crossSize += 10;
            if (hOverlayWnd) InvalidateRect(hOverlayWnd, NULL, TRUE);
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

    WNDCLASSW overlayClass = { 0 };
    overlayClass.lpfnWndProc = OverlayWindowHandler;
    overlayClass.hInstance = hInstance;
    overlayClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    overlayClass.lpszClassName = L"OverlayClass";
    RegisterClassW(&overlayClass);

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

    hGlobalKeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyHandler, hInstance, 0);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hGlobalKeyHook);
    DeleteObject(hDarkThemeBrush);

    return 0;
}