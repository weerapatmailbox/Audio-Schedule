#pragma once
#include <windows.h>
#include <string>
#include "PresetManager.hpp"

class SavePresetDialog {
public:
    static bool Show(HWND hParent, std::wstring& inOutPresetName, bool& outCopyAudio) {
        SavePresetDialog dlg(hParent, inOutPresetName);
        if (dlg.DoModal()) {
            inOutPresetName = dlg.m_presetName;
            outCopyAudio = dlg.m_copyAudio;
            return true;
        }
        return false;
    }

private:
    HWND m_hParent;
    std::wstring m_presetName;
    bool m_copyAudio = true;
    bool m_resultOk = false;
    bool m_isRunning = false;

    HWND m_hwndDlg = NULL;
    HWND m_hEditName = NULL;
    HWND m_hChkCopy = NULL;
    HWND m_hLblPreview = NULL;
    HFONT m_hFont = NULL;
    HFONT m_hFontBold = NULL;
    HFONT m_hFontSmall = NULL;

    SavePresetDialog(HWND hParent, const std::wstring& defaultName)
        : m_hParent(hParent), m_presetName(defaultName) {
        if (m_presetName.empty()) {
            m_presetName = L"My_Schedule";
        }
    }

    bool DoModal() {
        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
        if (!GetClassInfoExW(GetModuleHandle(NULL), L"ATP_SavePresetDialogClass", &wc)) {
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.lpfnWndProc = DialogProcThunk;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = L"ATP_SavePresetDialogClass";
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
            RegisterClassExW(&wc);
        }

        int dlgW = 540;
        int dlgH = 310;

        RECT parentRect;
        GetWindowRect(m_hParent, &parentRect);
        int x = parentRect.left + (parentRect.right - parentRect.left - dlgW) / 2;
        int y = parentRect.top + (parentRect.bottom - parentRect.top - dlgH) / 2;

        m_hwndDlg = CreateWindowExW(
            WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
            L"ATP_SavePresetDialogClass",
            L"บันทึกชุดการเล่นเสียง (Save Preset Package)",
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
        SavePresetDialog* pThis = NULL;
        if (uMsg == WM_NCCREATE) {
            CREATESTRUCT* pCS = (CREATESTRUCT*)lParam;
            pThis = (SavePresetDialog*)pCS->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
            pThis->m_hwndDlg = hwnd;
        } else {
            pThis = (SavePresetDialog*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
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
                    m_presetName = buf;
                    if (m_presetName.empty()) {
                        MessageBoxW(hwnd, L"กรุณาระบุชื่อชุดการเล่นก่อนบันทึก", L"แจ้งเตือน", MB_OK | MB_ICONWARNING);
                        return 0;
                    }
                    m_copyAudio = true;
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

        m_hFontSmall = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        auto SetFont = [this](HWND h) { SendMessage(h, WM_SETFONT, (WPARAM)m_hFont, TRUE); };
        auto SetFontBold = [this](HWND h) { SendMessage(h, WM_SETFONT, (WPARAM)m_hFontBold, TRUE); };
        auto SetFontSmall = [this](HWND h) { SendMessage(h, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE); };

        int curY = 20;

        // Label: Name
        HWND lblTitle = CreateWindowW(L"STATIC", L"ชื่อชุดการเล่น (Preset Name):", WS_CHILD | WS_VISIBLE, 25, curY, 250, 20, m_hwndDlg, NULL, NULL, NULL);
        SetFontBold(lblTitle);

        curY += 24;

        m_hEditName = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", m_presetName.c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                     25, curY, 470, 26, m_hwndDlg, (HMENU)1001, NULL, NULL);
        SetFont(m_hEditName);

        curY += 38;

        // Note: Automatic audio file copying
        m_hChkCopy = CreateWindowW(L"STATIC", L"📦 ระบบจะคัดลอกไฟล์เสียงทั้งหมดมาไว้ในโฟลเดอร์ของชุดการเล่นให้อัตโนมัติ",
                                   WS_CHILD | WS_VISIBLE,
                                   25, curY, 470, 22, m_hwndDlg, NULL, NULL, NULL);
        SetFont(m_hChkCopy);

        curY += 34;

        // Folder Preview Box
        HWND lblBox = CreateWindowW(L"STATIC", L"โฟลเดอร์ปลายทางที่จะสร้างในโปรแกรม:", WS_CHILD | WS_VISIBLE, 25, curY, 470, 18, m_hwndDlg, NULL, NULL, NULL);
        SetFontSmall(lblBox);

        curY += 20;

        m_hLblPreview = CreateWindowExW(WS_EX_STATICEDGE, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                        25, curY, 470, 48, m_hwndDlg, NULL, NULL, NULL);
        SetFontSmall(m_hLblPreview);

        curY += 62;

        // Buttons
        HWND btnOk = CreateWindowW(L"BUTTON", L"💾 บันทึกชุดการเล่น", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                   260, curY, 130, 32, m_hwndDlg, (HMENU)IDOK, NULL, NULL);
        SetFontBold(btnOk);

        HWND btnCancel = CreateWindowW(L"BUTTON", L"ยกเลิก", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                       400, curY, 95, 32, m_hwndDlg, (HMENU)IDCANCEL, NULL, NULL);
        SetFont(btnCancel);

        UpdateFolderPreview();
    }

    void UpdateFolderPreview() {
        wchar_t buf[256] = { 0 };
        GetWindowTextW(m_hEditName, buf, 256);
        std::wstring name = buf;
        if (name.empty()) name = L"ชื่อชุด";

        std::wstring path = PresetManager::GetPresetsRootDirectory() + L"\\" + name + L"\\\n └── audio\\ (ไฟล์เสียงทั้งหมด)";
        SetWindowTextW(m_hLblPreview, path.c_str());
    }
};
