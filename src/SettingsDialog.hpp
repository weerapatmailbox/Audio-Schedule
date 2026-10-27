#pragma once
#include <windows.h>
#include <shellapi.h>
#include <string>
#include "Language.h"
#include "AppSettings.hpp"
#include "PresetManager.hpp"
#include "Version.h"

class SettingsDialog {
public:
    static bool Show(HWND hParent) {
        SettingsDialog dlg(hParent);
        return dlg.DoModal();
    }

private:
    HWND m_hParent = NULL;
    HWND m_hwndDlg = NULL;
    bool m_isRunning = false;
    bool m_resultOk = false;

    HWND m_hGrpGeneral = NULL;
    HWND m_hChkAutoStart = NULL;
    HWND m_hLblAutoStartDesc = NULL;

    HWND m_hGrpLanguage = NULL;
    HWND m_hLblLangPrompt = NULL;
    HWND m_hRadioLangEn = NULL;
    HWND m_hRadioLangTh = NULL;

    HWND m_hGrpOverlap = NULL;
    HWND m_hRadioOverlapQueue = NULL;
    HWND m_hRadioOverlapSimul = NULL;

    HWND m_hGrpAppInfo = NULL;
    HWND m_hLblVersion = NULL;
    HWND m_hLblPublisher = NULL;
    HWND m_hLblStorage = NULL;
    HWND m_hBtnOpenAppFolder = NULL;
    HWND m_hBtnOpenPresetsFolder = NULL;

    HWND m_hBtnSave = NULL;
    HWND m_hBtnCancel = NULL;

    HFONT m_hFontBold = NULL;
    HFONT m_hFontNormal = NULL;
    HFONT m_hFontSmall = NULL;

    enum {
        IDC_CHK_AUTOSTART = 5001,
        IDC_RADIO_LANG_EN = 5002,
        IDC_RADIO_LANG_TH = 5003,
        IDC_RADIO_OVERLAP_QUEUE = 5006,
        IDC_RADIO_OVERLAP_SIMUL = 5007,
        IDC_BTN_OPEN_APP = 5004,
        IDC_BTN_OPEN_PRESETS = 5005,
        IDC_BTN_SAVE = IDOK,
        IDC_BTN_CANCEL = IDCANCEL
    };

    explicit SettingsDialog(HWND hParent) : m_hParent(hParent) {}

    bool DoModal() {
        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
        if (!GetClassInfoExW(GetModuleHandle(NULL), L"ATP_SettingsDlgClass", &wc)) {
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.lpfnWndProc = DialogProcThunk;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = L"ATP_SettingsDlgClass";
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
            RegisterClassExW(&wc);
        }

        int dlgW = 560;
        int dlgH = 600;

        RECT parentRect;
        if (m_hParent && IsWindow(m_hParent)) {
            GetWindowRect(m_hParent, &parentRect);
        } else {
            parentRect.left = 100;
            parentRect.right = 700;
            parentRect.top = 100;
            parentRect.bottom = 600;
        }

        int x = parentRect.left + (parentRect.right - parentRect.left - dlgW) / 2;
        int y = parentRect.top + (parentRect.bottom - parentRect.top - dlgH) / 2;
        if (x < 10) x = 10;
        if (y < 10) y = 10;

        m_hwndDlg = CreateWindowExW(
            WS_EX_DLGMODALFRAME,
            L"ATP_SettingsDlgClass",
            Lang::SettingsDialogTitle(),
            WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
            x, y, dlgW, dlgH,
            m_hParent, NULL, GetModuleHandle(NULL), this
        );

        if (!m_hwndDlg) return false;

        if (m_hParent && IsWindow(m_hParent)) {
            EnableWindow(m_hParent, FALSE);
        }

        m_isRunning = true;
        MSG msg;
        while (m_isRunning) {
            BOOL bRet = GetMessageW(&msg, NULL, 0, 0);
            if (bRet == 0) {
                PostQuitMessage((int)msg.wParam);
                break;
            } else if (bRet == -1) {
                break;
            }

            if (!IsDialogMessageW(m_hwndDlg, &msg)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }

        if (m_hParent && IsWindow(m_hParent)) {
            EnableWindow(m_hParent, TRUE);
            SetForegroundWindow(m_hParent);
        }

        return m_resultOk;
    }

    static LRESULT CALLBACK DialogProcThunk(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        SettingsDialog* pThis = NULL;
        if (uMsg == WM_NCCREATE) {
            CREATESTRUCT* pCS = (CREATESTRUCT*)lParam;
            pThis = (SettingsDialog*)pCS->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
            pThis->m_hwndDlg = hwnd;
        } else {
            pThis = (SettingsDialog*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }

        if (pThis) {
            return pThis->DialogProc(hwnd, uMsg, wParam, lParam);
        }
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    LRESULT DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
            case WM_CREATE: {
                m_hFontBold = CreateFontW(15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                         CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                m_hFontNormal = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                           CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                m_hFontSmall = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                          CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

                HINSTANCE hInst = GetModuleHandle(NULL);
                int pad = 16;
                int grpW = 512;

                // Group 1: General (Auto-start)
                int g1Y = 14;
                int g1H = 88;
                m_hGrpGeneral = CreateWindowW(L"BUTTON", Lang::SettingsGeneralGroup(), WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                              pad, g1Y, grpW, g1H, hwnd, NULL, hInst, NULL);
                SendMessage(m_hGrpGeneral, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

                m_hChkAutoStart = CreateWindowW(L"BUTTON", Lang::SettingsAutoStartChk(),
                                                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                                                pad + 16, g1Y + 24, grpW - 32, 24, hwnd, (HMENU)IDC_CHK_AUTOSTART, hInst, NULL);
                SendMessage(m_hChkAutoStart, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);
                SendMessage(m_hChkAutoStart, BM_SETCHECK, AppSettings::IsAutoStartEnabled() ? BST_CHECKED : BST_UNCHECKED, 0);

                m_hLblAutoStartDesc = CreateWindowW(L"STATIC", Lang::SettingsAutoStartDesc(),
                                                    WS_CHILD | WS_VISIBLE,
                                                    pad + 36, g1Y + 50, grpW - 52, 28, hwnd, NULL, hInst, NULL);
                SendMessage(m_hLblAutoStartDesc, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);

                // Group 2: Display Language
                int g2Y = g1Y + g1H + 12;
                int g2H = 92;
                m_hGrpLanguage = CreateWindowW(L"BUTTON", Lang::SettingsLanguageGroup(), WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                               pad, g2Y, grpW, g2H, hwnd, NULL, hInst, NULL);
                SendMessage(m_hGrpLanguage, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

                m_hLblLangPrompt = CreateWindowW(L"STATIC", Lang::SettingsLangPrompt(),
                                                 WS_CHILD | WS_VISIBLE,
                                                 pad + 16, g2Y + 24, grpW - 32, 20, hwnd, NULL, hInst, NULL);
                SendMessage(m_hLblLangPrompt, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                m_hRadioLangEn = CreateWindowW(L"BUTTON", Lang::SettingsRadioEn(),
                                               WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_TABSTOP,
                                               pad + 20, g2Y + 50, 200, 24, hwnd, (HMENU)IDC_RADIO_LANG_EN, hInst, NULL);
                SendMessage(m_hRadioLangEn, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                m_hRadioLangTh = CreateWindowW(L"BUTTON", Lang::SettingsRadioTh(),
                                               WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_TABSTOP,
                                               pad + 240, g2Y + 50, 200, 24, hwnd, (HMENU)IDC_RADIO_LANG_TH, hInst, NULL);
                SendMessage(m_hRadioLangTh, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                SendMessage(m_hRadioLangEn, BM_SETCHECK, (Lang::current == Language::English) ? BST_CHECKED : BST_UNCHECKED, 0);
                SendMessage(m_hRadioLangTh, BM_SETCHECK, (Lang::current == Language::Thai) ? BST_CHECKED : BST_UNCHECKED, 0);

                // Group 3: Schedule Conflict / Overlap Mode
                int g3Y = g2Y + g2H + 12;
                int g3H = 92;
                m_hGrpOverlap = CreateWindowW(L"BUTTON", Lang::SettingsOverlapGroup(), WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                              pad, g3Y, grpW, g3H, hwnd, NULL, hInst, NULL);
                SendMessage(m_hGrpOverlap, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

                m_hRadioOverlapQueue = CreateWindowW(L"BUTTON", Lang::SettingsOverlapQueue(),
                                                     WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_TABSTOP,
                                                     pad + 20, g3Y + 26, grpW - 40, 24, hwnd, (HMENU)IDC_RADIO_OVERLAP_QUEUE, hInst, NULL);
                SendMessage(m_hRadioOverlapQueue, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                m_hRadioOverlapSimul = CreateWindowW(L"BUTTON", Lang::SettingsOverlapSimultaneous(),
                                                     WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_TABSTOP,
                                                     pad + 20, g3Y + 54, grpW - 40, 24, hwnd, (HMENU)IDC_RADIO_OVERLAP_SIMUL, hInst, NULL);
                SendMessage(m_hRadioOverlapSimul, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                SendMessage(m_hRadioOverlapQueue, BM_SETCHECK, (AppSettings::overlapMode == OverlapMode::Queue) ? BST_CHECKED : BST_UNCHECKED, 0);
                SendMessage(m_hRadioOverlapSimul, BM_SETCHECK, (AppSettings::overlapMode == OverlapMode::Simultaneous) ? BST_CHECKED : BST_UNCHECKED, 0);

                // Group 4: Application Storage & Info
                int g4Y = g3Y + g3H + 12;
                int g4H = 160;
                m_hGrpAppInfo = CreateWindowW(L"BUTTON", Lang::SettingsAppInfoGroup(), WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                              pad, g4Y, grpW, g4H, hwnd, NULL, hInst, NULL);
                SendMessage(m_hGrpAppInfo, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

                std::wstring verStr = std::wstring(Lang::SettingsVersionLabel()) + L" Audio Schedule v1.0.0 ("
#ifdef _WIN64
                    + L"64-bit x64"
#else
                    + L"32-bit x86"
#endif
                    + L")";
                m_hLblVersion = CreateWindowW(L"STATIC", verStr.c_str(), WS_CHILD | WS_VISIBLE,
                                              pad + 16, g4Y + 26, grpW - 32, 20, hwnd, NULL, hInst, NULL);
                SendMessage(m_hLblVersion, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                std::wstring pubStr = std::wstring(Lang::SettingsPublisherLabel()) + L" " APP_TEAM_W;
                m_hLblPublisher = CreateWindowW(L"STATIC", pubStr.c_str(), WS_CHILD | WS_VISIBLE,
                                                pad + 16, g4Y + 50, grpW - 32, 20, hwnd, NULL, hInst, NULL);
                SendMessage(m_hLblPublisher, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                std::wstring stStr = std::wstring(Lang::SettingsStorageLabel()) + L"\r\n" + PresetManager::GetPresetsRootDirectory();
                m_hLblStorage = CreateWindowW(L"STATIC", stStr.c_str(), WS_CHILD | WS_VISIBLE,
                                              pad + 16, g4Y + 74, grpW - 32, 38, hwnd, NULL, hInst, NULL);
                SendMessage(m_hLblStorage, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);

                m_hBtnOpenAppFolder = CreateWindowW(L"BUTTON", Lang::BtnOpenAppFolder(),
                                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                                    pad + 16, g4Y + 118, 190, 28, hwnd, (HMENU)IDC_BTN_OPEN_APP, hInst, NULL);
                SendMessage(m_hBtnOpenAppFolder, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                m_hBtnOpenPresetsFolder = CreateWindowW(L"BUTTON", Lang::BtnOpenPresetsFolder(),
                                                        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                                        pad + 216, g4Y + 118, 190, 28, hwnd, (HMENU)IDC_BTN_OPEN_PRESETS, hInst, NULL);
                SendMessage(m_hBtnOpenPresetsFolder, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                // Bottom Buttons: Save & Cancel
                int btnY = g4Y + g4H + 16;
                m_hBtnSave = CreateWindowW(L"BUTTON", Lang::BtnSave(),
                                           WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
                                           pad + grpW - 205, btnY, 100, 30, hwnd, (HMENU)IDC_BTN_SAVE, hInst, NULL);
                SendMessage(m_hBtnSave, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

                m_hBtnCancel = CreateWindowW(L"BUTTON", Lang::BtnCancel(),
                                             WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                                             pad + grpW - 95, btnY, 95, 30, hwnd, (HMENU)IDC_BTN_CANCEL, hInst, NULL);
                SendMessage(m_hBtnCancel, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                return 0;
            }

            case WM_CTLCOLORSTATIC: {
                HDC hdc = (HDC)wParam;
                SetBkMode(hdc, TRANSPARENT);
                return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
            }

            case WM_COMMAND: {
                int id = LOWORD(wParam);
                switch (id) {
                    case IDC_BTN_OPEN_APP: {
                        ShellExecuteW(NULL, L"open", PresetManager::GetAppDirectory().c_str(), NULL, NULL, SW_SHOWNORMAL);
                        break;
                    }
                    case IDC_BTN_OPEN_PRESETS: {
                        ShellExecuteW(NULL, L"open", PresetManager::GetPresetsRootDirectory().c_str(), NULL, NULL, SW_SHOWNORMAL);
                        break;
                    }
                    case IDC_BTN_SAVE: {
                        bool autoStart = (SendMessage(m_hChkAutoStart, BM_GETCHECK, 0, 0) == BST_CHECKED);
                        AppSettings::SetAutoStart(autoStart);

                        if (SendMessage(m_hRadioLangTh, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                            Lang::current = Language::Thai;
                        } else {
                            Lang::current = Language::English;
                        }

                        if (SendMessage(m_hRadioOverlapSimul, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                            AppSettings::overlapMode = OverlapMode::Simultaneous;
                        } else {
                            AppSettings::overlapMode = OverlapMode::Queue;
                        }

                        AppSettings::Save();
                        m_resultOk = true;
                        m_isRunning = false;
                        DestroyWindow(hwnd);
                        break;
                    }
                    case IDC_BTN_CANCEL: {
                        m_resultOk = false;
                        m_isRunning = false;
                        DestroyWindow(hwnd);
                        break;
                    }
                }
                return 0;
            }

            case WM_CLOSE: {
                m_resultOk = false;
                m_isRunning = false;
                DestroyWindow(hwnd);
                return 0;
            }

            case WM_DESTROY: {
                if (m_hFontBold) DeleteObject(m_hFontBold);
                if (m_hFontNormal) DeleteObject(m_hFontNormal);
                if (m_hFontSmall) DeleteObject(m_hFontSmall);
                m_isRunning = false;
                return 0;
            }
        }
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
};
