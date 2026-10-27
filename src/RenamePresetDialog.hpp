#pragma once
#include <windows.h>
#include <string>
#include <shlwapi.h>
#include "Language.h"
#include "PresetManager.hpp"

class RenamePresetDialog {
public:
    static bool Show(HWND hParent, const std::wstring& oldName, std::wstring& outNewName) {
        RenamePresetDialog dlg(hParent, oldName);
        if (dlg.DoModal()) {
            outNewName = dlg.m_newName;
            return true;
        }
        return false;
    }

private:
    HWND m_hParent;
    std::wstring m_oldName;
    std::wstring m_newName;
    bool m_resultOk = false;
    bool m_isRunning = false;

    HWND m_hwndDlg = NULL;
    HWND m_hEditName = NULL;
    HWND m_hLblPreview = NULL;
    HFONT m_hFont = NULL;
    HFONT m_hFontBold = NULL;
    HFONT m_hFontSmall = NULL;

    RenamePresetDialog(HWND hParent, const std::wstring& oldName)
        : m_hParent(hParent), m_oldName(oldName), m_newName(oldName) {}

    bool DoModal() {
        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
        if (!GetClassInfoExW(GetModuleHandle(NULL), L"ATP_RenamePresetDialogClass", &wc)) {
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.lpfnWndProc = DialogProcThunk;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = L"ATP_RenamePresetDialogClass";
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
            RegisterClassExW(&wc);
        }

        int dlgW = 490;
        int dlgH = 250;

        RECT parentRect;
        GetWindowRect(m_hParent, &parentRect);
        int x = parentRect.left + (parentRect.right - parentRect.left - dlgW) / 2;
        int y = parentRect.top + (parentRect.bottom - parentRect.top - dlgH) / 2;

        std::wstring title = (Lang::current == Language::Thai)
            ? L"เปลี่ยนชื่อชุดการเล่น (Rename Preset)"
            : L"Rename Preset Package";

        m_hwndDlg = CreateWindowExW(
            WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
            L"ATP_RenamePresetDialogClass",
            title.c_str(),
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
        RenamePresetDialog* pThis = NULL;
        if (uMsg == WM_NCCREATE) {
            CREATESTRUCT* pCS = (CREATESTRUCT*)lParam;
            pThis = (RenamePresetDialog*)pCS->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
            pThis->m_hwndDlg = hwnd;
        } else {
            pThis = (RenamePresetDialog*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
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
                        MessageBoxW(hwnd,
                            (Lang::current == Language::Thai ? L"กรุณาระบุชื่อชุดการเล่นใหม่" : L"Please enter a new preset name"),
                            (Lang::current == Language::Thai ? L"แจ้งเตือน" : L"Warning"),
                            MB_OK | MB_ICONWARNING);
                        SetFocus(m_hEditName);
                        return 0;
                    }
                    name = name.substr(s, e - s + 1);

                    // Check for invalid directory characters
                    if (name.find_first_of(L"\\/:*?\"<>|") != std::wstring::npos) {
                        MessageBoxW(hwnd,
                            (Lang::current == Language::Thai
                                ? L"ชื่อชุดการเล่นต้องไม่มีอักขระพิเศษ \\ / : * ? \" < > |"
                                : L"Preset name cannot contain special characters \\ / : * ? \" < > |"),
                            (Lang::current == Language::Thai ? L"แจ้งเตือน" : L"Warning"),
                            MB_OK | MB_ICONWARNING);
                        SetFocus(m_hEditName);
                        return 0;
                    }

                    // Check if unchanged
                    if (name == m_oldName) {
                        m_isRunning = false;
                        DestroyWindow(hwnd);
                        return 0;
                    }

                    // Check if target folder already exists
                    std::wstring newFolder = PresetManager::GetPresetsRootDirectory() + L"\\" + name;
                    if (_wcsicmp(name.c_str(), m_oldName.c_str()) != 0 && PathFileExistsW(newFolder.c_str())) {
                        MessageBoxW(hwnd,
                            (Lang::current == Language::Thai
                                ? L"มีชุดการเล่นชื่อนี้อยู่แล้ว กรุณาใช้ชื่ออื่น"
                                : L"A preset with this name already exists. Please choose another name."),
                            (Lang::current == Language::Thai ? L"แจ้งเตือน" : L"Warning"),
                            MB_OK | MB_ICONWARNING);
                        SetFocus(m_hEditName);
                        return 0;
                    }

                    m_newName = name;
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

        std::wstring headerText = (Lang::current == Language::Thai)
            ? (L"✏️ เปลี่ยนชื่อชุดการเล่นเดิม: \"" + m_oldName + L"\"")
            : (L"✏️ Rename Preset Package: \"" + m_oldName + L"\"");
        HWND hTitle = CreateWindowW(L"STATIC", headerText.c_str(),
                                   WS_CHILD | WS_VISIBLE, 20, 18, 440, 24,
                                   m_hwndDlg, NULL, GetModuleHandle(NULL), NULL);
        SendMessage(hTitle, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

        m_hEditName = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", m_oldName.c_str(),
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                     20, 50, 440, 28, m_hwndDlg, (HMENU)1001, GetModuleHandle(NULL), NULL);
        SendMessage(m_hEditName, WM_SETFONT, (WPARAM)m_hFont, TRUE);
        SendMessage(m_hEditName, EM_SETSEL, 0, -1);

        m_hLblPreview = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE,
                                     20, 86, 440, 20, m_hwndDlg, NULL, GetModuleHandle(NULL), NULL);
        SendMessage(m_hLblPreview, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);

        HWND hNote = CreateWindowW(L"STATIC",
                                  (Lang::current == Language::Thai
                                      ? L"💡 โฟลเดอร์ของชุดการเล่นและไฟล์ใน presets จะถูกเปลี่ยนชื่อตามชื่อใหม่นี้โดยอัตโนมัติ"
                                      : L"💡 The preset directory and all contained files will be renamed automatically."),
                                  WS_CHILD | WS_VISIBLE, 20, 114, 440, 36,
                                  m_hwndDlg, NULL, GetModuleHandle(NULL), NULL);
        SendMessage(hNote, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);

        std::wstring okText = (Lang::current == Language::Thai) ? L"💾 บันทึกชื่อใหม่ (&Save)" : L"💾 &Save Name";
        HWND hBtnOk = CreateWindowW(L"BUTTON", okText.c_str(),
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                   200, 160, 140, 34, m_hwndDlg, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
        SendMessage(hBtnOk, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

        std::wstring cancelText = (Lang::current == Language::Thai) ? L"ยกเลิก (&Cancel)" : L"&Cancel";
        HWND hBtnCancel = CreateWindowW(L"BUTTON", cancelText.c_str(),
                                       WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                       350, 160, 110, 34, m_hwndDlg, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);
        SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)m_hFont, TRUE);

        UpdateFolderPreview();
        SetFocus(m_hEditName);
    }

    void UpdateFolderPreview() {
        wchar_t buf[256] = { 0 };
        GetWindowTextW(m_hEditName, buf, 256);
        std::wstring name = buf;
        if (name.empty()) name = L"...";
        std::wstring preview = (Lang::current == Language::Thai)
            ? (L"📂 โฟลเดอร์ใหม่: presets\\" + name)
            : (L"📂 New folder path: presets\\" + name);
        SetWindowTextW(m_hLblPreview, preview.c_str());
    }
};
