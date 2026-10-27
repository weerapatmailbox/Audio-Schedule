#include <windows.h>
#include <commctrl.h>
#include "Security.hpp"
#include "Language.h"
#include "AppSettings.hpp"
#include "MainWindow.hpp"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    // ------------------------------------------------------------------------
    // LEVEL 3: Anti-Debugging & Anti-Tampering Startup Enforcement
    // ------------------------------------------------------------------------
    Security::AntiDebug::Enforce();

    // ------------------------------------------------------------------------
    // Single Instance Guard: Allow only ONE running instance of Audio Schedule
    // If user launches a second instance, bring the existing window to the front
    // (even if hidden in the System Tray) and terminate the duplicate process.
    // ------------------------------------------------------------------------
    HANDLE hMutex = CreateMutexW(NULL, TRUE, CRYPT_W(APP_SINGLE_INSTANCE_MUTEX_W).c_str());
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        UINT uMsg = RegisterWindowMessageW(CRYPT_W(APP_RESTORE_MSG_W).c_str());
        HWND hExistingWnd = FindWindowW(CRYPT_W(APP_WINDOW_CLASS_W).c_str(), NULL);
        if (hExistingWnd) {
            DWORD dwProcessId = 0;
            GetWindowThreadProcessId(hExistingWnd, &dwProcessId);
            if (dwProcessId != 0) {
                AllowSetForegroundWindow(dwProcessId);
            }

            // Notify existing instance to unhide/restore from tray and take focus
            SendMessageW(hExistingWnd, uMsg, 0, 0);

            // Also directly unhide/restore window
            ShowWindow(hExistingWnd, SW_SHOW);
            if (IsIconic(hExistingWnd)) {
                ShowWindow(hExistingWnd, SW_RESTORE);
            }
            HWND hTarget = GetLastActivePopup(hExistingWnd);
            if (!hTarget || !IsWindow(hTarget)) hTarget = hExistingWnd;
            SetForegroundWindow(hTarget);
            BringWindowToTop(hTarget);
        } else {
            // Fallback: broadcast restore message
            PostMessageW(HWND_BROADCAST, uMsg, 0, 0);
        }

        if (hMutex) {
            CloseHandle(hMutex);
        }
        return 0; // Exit duplicate instance immediately
    }

    // Load application settings (language, etc.)
    AppSettings::Load();

    // Initialize Modern Windows Common Controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES | ICC_UPDOWN_CLASS | ICC_BAR_CLASSES | ICC_TAB_CLASSES;
    InitCommonControlsEx(&icex);

    MainWindow app;
    if (!app.Create(hInstance, nCmdShow)) {
        MessageBoxW(NULL, L"ไม่สามารถเริ่มต้นหน้าต่างโปรแกรมได้", L"ข้อผิดพลาด", MB_ICONERROR | MB_OK);
        if (hMutex) CloseHandle(hMutex);
        return 1;
    }

    HACCEL hAccel = app.GetAcceleratorTable();
    HWND hWnd = app.GetHwnd();

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (!hAccel || !TranslateAcceleratorW(hWnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (hMutex) {
        CloseHandle(hMutex);
    }

    return (int)msg.wParam;
}
