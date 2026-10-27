#pragma once
#include <windows.h>
#include <string>
#include <shlwapi.h>
#include "PresetManager.hpp"

class NewPresetDialog {
public:
    static bool Show(HWND hParent, std::wstring& outPresetName, bool& outCopyCurrentGroups) {
        NewPresetDialog dlg(hParent);
        if (dlg.DoModal()) {
            outPresetName = dlg.m_presetName;
            outCopyCurrentGroups = dlg.m_copyCurrentGroups;
            return true;
        }
        return false;
    }

private:
    HWND m_hParent;
    std::wstring m_presetName = L"ชุดเสียงใหม่";
    bool m_copyCurrentGroups = false;
    bool m_resultOk = false;
    bool m_isRunning = false;

    HWND m_hwndDlg = NULL;
    HWND m_hEditName = NULL;
    HWND m_hChkCopy = NULL;
    HWND m_hLblPreview = NULL;
    HFONT m_hFont = NULL;
    HFONT m_hFontBold = NULL;
    HFONT m_hFontSmall = NULL;

    NewPresetDialog(HWND hParent) : m_hParent(hParent) {}

    bool DoModal() {
        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
        if (!GetClassInfoExW(GetModuleHandle(NULL), L"ATP_NewPresetDialogClass", &wc)) {
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.lpfnWndProc = DialogProcThunk;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = L"ATP_NewPresetDialogClass";
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
            RegisterClassExW(&wc);
        }

        int dlgW = 500;
        int dlgH = 290;

        RECT parentRect;
        GetWindowRect(m_hParent, &parentRect);
        int x = parentRect.left + (parentRect.right - parentRect.left - dlgW) / 2;
        int y = parentRect.top + (parentRect.bottom - parentRect.top - dlgH) / 2;

        m_hwndDlg = CreateWindowExW(
            WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
            L"ATP_NewPresetDialogClass",
            L"สร้างชุดการเล่นใหม่ (Create New Preset)",
            WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
            x, y, dlgW, dlgH,
            m_hParent, NULL, GetModuleHandle(NULL), this
        );

        if (!m_hwndDlg) return false;

        EnableWindow(m_hParent, FALSE);

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

            if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
                m_isRunning = false;
                break;
            }

            if (!m_hwndDlg || !IsWindow(m_hwndDlg)) {
                break;
            }

            if (!IsDialogMessageW(m_hwndDlg, &msg)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }

        if (m_hwndDlg && IsWindow(m_hwndDlg)) {
            DestroyWindow(m_hwndDlg);
        }

        EnableWindow(m_hParent, TRUE);
        SetForegroundWindow(m_hParent);

        return m_resultOk;
    }

    static LRESULT CALLBACK DialogProcThunk(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        NewPresetDialog* pThis = NULL;
        if (uMsg == WM_NCCREATE) {
            CREATESTRUCT* pCS = (CREATESTRUCT*)lParam;
            pThis = (NewPresetDialog*)pCS->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
            pThis->m_hwndDlg = hwnd;
        } else {
            pThis = (NewPresetDialog*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }

        if (pThis) {
            return pThis->DialogProc(hwnd, uMsg, wParam, lParam);
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    LRESULT DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
            case WM_CREATE:
                CreateControls();
                return 0;

            case WM_COMMAND: {
                int id = LOWORD(wParam);
                int code = HIWORD(wParam);

                if (id == 1001 && code == EN_CHANGE) {
                    UpdateFolderPreview();
                } else if (id == IDOK) {
                    wchar_t buf[256] = { 0 };
                    GetWindowTextW(m_hEditName, buf, 256);
                    std::wstring name = buf;

                    // Trim whitespace
                    size_t s = name.find_first_not_of(L" \t\r\n");
                    size_t e = name.find_last_not_of(L" \t\r\n");
                    if (s == std::wstring::npos) {
                        MessageBoxW(hwnd, L"กรุณาระบุชื่อชุดการเล่นใหม่", L"แจ้งเตือน", MB_OK | MB_ICONWARNING);
                        SetFocus(m_hEditName);
                        return 0;
                    }
                    name = name.substr(s, e - s + 1);

                    // Check if folder already exists
                    std::wstring folder = PresetManager::GetPresetsRootDirectory() + L"\\" + name;
                    if (PathFileExistsW(folder.c_str())) {
                        MessageBoxW(hwnd, L"มีชุดการเล่นชื่อนี้อยู่แล้ว กรุณาใช้ชื่ออื่น", L"แจ้งเตือน", MB_OK | MB_ICONWARNING);
                        SetFocus(m_hEditName);
                        return 0;
                    }

                    m_presetName = name;
                    m_copyCurrentGroups = (SendMessage(m_hChkCopy, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    m_resultOk = true;
                    m_isRunning = false;
                    DestroyWindow(hwnd);
                } else if (id == IDCANCEL) {
                    m_isRunning = false;
                    DestroyWindow(hwnd);
                }
                return 0;
            }

            case WM_CLOSE:
                m_isRunning = false;
                DestroyWindow(hwnd);
                return 0;

            case WM_DESTROY:
                if (m_hFont) DeleteObject(m_hFont);
                if (m_hFontBold) DeleteObject(m_hFontBold);
                if (m_hFontSmall) DeleteObject(m_hFontSmall);
                m_hwndDlg = NULL;
                m_isRunning = false;
                return 0;
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    void CreateControls() {
        m_hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        m_hFontBold = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        m_hFontSmall = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        HWND hTitle = CreateWindowW(L"STATIC", L"📁 ตั้งชื่อชุดการเล่นใหม่:",
                                   WS_CHILD | WS_VISIBLE, 20, 18, 440, 24,
                                   m_hwndDlg, NULL, GetModuleHandle(NULL), NULL);
        SendMessage(hTitle, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

        m_hEditName = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", m_presetName.c_str(),
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                     20, 46, 440, 28, m_hwndDlg, (HMENU)1001, GetModuleHandle(NULL), NULL);
        SendMessage(m_hEditName, WM_SETFONT, (WPARAM)m_hFont, TRUE);
        SendMessage(m_hEditName, EM_SETSEL, 0, -1);

        m_hLblPreview = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE,
                                     20, 80, 440, 20, m_hwndDlg, NULL, GetModuleHandle(NULL), NULL);
        SendMessage(m_hLblPreview, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);

        m_hChkCopy = CreateWindowW(L"BUTTON", L"คัดลอกรายการกลุ่มจากชุดปัจจุบันมาใส่ในชุดใหม่นี้ด้วย",
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                   20, 110, 440, 24, m_hwndDlg, (HMENU)1002, GetModuleHandle(NULL), NULL);
        SendMessage(m_hChkCopy, WM_SETFONT, (WPARAM)m_hFont, TRUE);

        HWND hNote = CreateWindowW(L"STATIC",
                                  L"💡 เมื่อกดสร้าง โปรแกรมจะสร้างโฟลเดอร์สำหรับชุดการเล่นนี้ในโฟลเดอร์ presets อัตโนมัติ",
                                  WS_CHILD | WS_VISIBLE, 20, 145, 440, 40,
                                  m_hwndDlg, NULL, GetModuleHandle(NULL), NULL);
        SendMessage(hNote, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);

        HWND hBtnOk = CreateWindowW(L"BUTTON", L"สร้างชุดใหม่ (&Create)",
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                   220, 200, 130, 34, m_hwndDlg, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
        SendMessage(hBtnOk, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

        HWND hBtnCancel = CreateWindowW(L"BUTTON", L"ยกเลิก (&Cancel)",
                                       WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                       360, 200, 100, 34, m_hwndDlg, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);
        SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)m_hFont, TRUE);

        UpdateFolderPreview();
        SetFocus(m_hEditName);
    }

    void UpdateFolderPreview() {
        wchar_t buf[256] = { 0 };
        GetWindowTextW(m_hEditName, buf, 256);
        std::wstring name = buf;
        if (name.empty()) name = L"...";
        std::wstring preview = L"📂 โฟลเดอร์ที่จะสร้าง: presets\\" + name;
        SetWindowTextW(m_hLblPreview, preview.c_str());
    }
};
