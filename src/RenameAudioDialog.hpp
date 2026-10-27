#pragma once
#include <windows.h>
#include <string>
#include "Language.h"

class RenameAudioDialog {
public:
    static bool Show(HWND hParent, const std::wstring& currentName, std::wstring& outNewName) {
        RenameAudioDialog dlg(hParent, currentName);
        if (dlg.DoModal()) {
            outNewName = dlg.m_newName;
            return true;
        }
        return false;
    }

private:
    HWND m_hParent = NULL;
    HWND m_hwndDlg = NULL;
    HWND m_hEditName = NULL;
    std::wstring m_oldName;
    std::wstring m_newName;
    bool m_confirmed = false;
    bool m_isRunning = false;

    HFONT m_hFontNormal = NULL;
    HFONT m_hFontBold = NULL;

    enum {
        IDC_EDIT_RENAME = 4201,
        IDC_BTN_RENAME_OK = IDOK,
        IDC_BTN_RENAME_CANCEL = IDCANCEL
    };

    explicit RenameAudioDialog(HWND hParent, const std::wstring& currentName)
        : m_hParent(hParent), m_oldName(currentName) {}

    ~RenameAudioDialog() {
        if (m_hFontNormal) DeleteObject(m_hFontNormal);
        if (m_hFontBold) DeleteObject(m_hFontBold);
    }

    bool DoModal() {
        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
        if (!GetClassInfoExW(GetModuleHandle(NULL), L"ATP_RenameDlgClass", &wc)) {
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.lpfnWndProc = DialogProcThunk;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = L"ATP_RenameDlgClass";
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
            RegisterClassExW(&wc);
        }

        m_hFontNormal = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        m_hFontBold = CreateFontW(-12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        int dlgW = 420;
        int dlgH = 175;

        RECT parentRect;
        if (m_hParent && IsWindow(m_hParent)) {
            GetWindowRect(m_hParent, &parentRect);
        } else {
            parentRect.left = 100; parentRect.top = 100;
            parentRect.right = 100 + dlgW; parentRect.bottom = 100 + dlgH;
        }

        int x = parentRect.left + (parentRect.right - parentRect.left - dlgW) / 2;
        int y = parentRect.top + (parentRect.bottom - parentRect.top - dlgH) / 2;

        std::wstring title = (Lang::current == Language::Thai) ? L"เปลี่ยนชื่อไฟล์เสียง" : L"Rename Audio File";

        m_hwndDlg = CreateWindowExW(
            WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
            L"ATP_RenameDlgClass",
            title.c_str(),
            WS_POPUP | WS_CAPTION | WS_SYSMENU,
            x, y, dlgW, dlgH,
            m_hParent, NULL, GetModuleHandle(NULL), this
        );

        if (!m_hwndDlg) return false;

        if (m_hParent && IsWindow(m_hParent)) {
            EnableWindow(m_hParent, FALSE);
        }

        ShowWindow(m_hwndDlg, SW_SHOW);
        UpdateWindow(m_hwndDlg);
        SetFocus(m_hEditName);

        // Select filename without extension
        size_t dotPos = m_oldName.rfind(L'.');
        int selEnd = (dotPos != std::wstring::npos) ? (int)dotPos : (int)m_oldName.size();
        SendMessage(m_hEditName, EM_SETSEL, 0, selEnd);

        m_isRunning = true;
        MSG msg;
        while (m_isRunning && GetMessageW(&msg, NULL, 0, 0)) {
            if (msg.message == WM_KEYDOWN) {
                if (msg.wParam == VK_ESCAPE) {
                    m_isRunning = false;
                    break;
                } else if (msg.wParam == VK_RETURN) {
                    OnOk();
                    break;
                }
            }

            if (!m_hwndDlg || !IsWindow(m_hwndDlg)) break;

            if (!IsDialogMessageW(m_hwndDlg, &msg)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }

        if (m_hwndDlg && IsWindow(m_hwndDlg)) {
            DestroyWindow(m_hwndDlg);
        }

        if (m_hParent && IsWindow(m_hParent)) {
            EnableWindow(m_hParent, TRUE);
            SetForegroundWindow(m_hParent);
        }

        return m_confirmed;
    }

    static LRESULT CALLBACK DialogProcThunk(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        RenameAudioDialog* pThis = NULL;
        if (uMsg == WM_NCCREATE) {
            CREATESTRUCTW* pCS = (CREATESTRUCTW*)lParam;
            pThis = (RenameAudioDialog*)pCS->lpCreateParams;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        } else {
            pThis = (RenameAudioDialog*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        }

        if (pThis) {
            return pThis->DialogProc(hwnd, uMsg, wParam, lParam);
        }
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    LRESULT DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
            case WM_CREATE: {
                bool isThai = (Lang::current == Language::Thai);

                HWND hLblPrompt = CreateWindowW(
                    L"STATIC",
                    isThai ? L"ระบุชื่อไฟล์เสียงใหม่:" : L"Enter new audio filename:",
                    WS_CHILD | WS_VISIBLE,
                    20, 16, 360, 20,
                    hwnd, NULL, GetModuleHandle(NULL), NULL
                );
                SendMessage(hLblPrompt, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                m_hEditName = CreateWindowExW(
                    WS_EX_CLIENTEDGE,
                    L"EDIT",
                    m_oldName.c_str(),
                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                    20, 42, 360, 26,
                    hwnd, (HMENU)IDC_EDIT_RENAME, GetModuleHandle(NULL), NULL
                );
                SendMessage(m_hEditName, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                HWND hBtnOk = CreateWindowW(
                    L"BUTTON",
                    isThai ? L"ตกลง (OK)" : L"OK",
                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                    190, 84, 90, 28,
                    hwnd, (HMENU)IDC_BTN_RENAME_OK, GetModuleHandle(NULL), NULL
                );
                SendMessage(hBtnOk, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

                HWND hBtnCancel = CreateWindowW(
                    L"BUTTON",
                    isThai ? L"ยกเลิก" : L"Cancel",
                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                    290, 84, 90, 28,
                    hwnd, (HMENU)IDC_BTN_RENAME_CANCEL, GetModuleHandle(NULL), NULL
                );
                SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                return 0;
            }

            case WM_COMMAND: {
                int id = LOWORD(wParam);
                if (id == IDC_BTN_RENAME_OK) {
                    OnOk();
                    return 0;
                } else if (id == IDC_BTN_RENAME_CANCEL) {
                    m_confirmed = false;
                    m_isRunning = false;
                    return 0;
                }
                break;
            }

            case WM_CLOSE:
                m_confirmed = false;
                m_isRunning = false;
                return 0;
        }
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    void OnOk() {
        wchar_t buf[MAX_PATH] = { 0 };
        GetWindowTextW(m_hEditName, buf, MAX_PATH);
        std::wstring name = buf;

        // Trim leading and trailing spaces
        size_t first = name.find_first_not_of(L" \t\r\n");
        if (first == std::wstring::npos) {
            MessageBoxW(m_hwndDlg,
                (Lang::current == Language::Thai) ? L"กรุณาระบุชื่อไฟล์เสียง" : L"Filename cannot be empty.",
                L"Warning", MB_OK | MB_ICONWARNING);
            return;
        }
        size_t last = name.find_last_not_of(L" \t\r\n");
        name = name.substr(first, (last - first + 1));

        // Check for illegal filename characters: \ / : * ? " < > |
        if (name.find_first_of(L"\\/:*?\"<>|") != std::wstring::npos) {
            MessageBoxW(m_hwndDlg,
                (Lang::current == Language::Thai) ?
                L"ชื่อไฟล์ไม่สามารถมีอักขระพิเศษต่อไปนี้ได้: \\ / : * ? \" < > |" :
                L"Filename cannot contain any of the following characters: \\ / : * ? \" < > |",
                L"Invalid Filename", MB_OK | MB_ICONERROR);
            return;
        }

        // If user omitted file extension, append original extension
        size_t oldDot = m_oldName.rfind(L'.');
        size_t newDot = name.rfind(L'.');
        if (newDot == std::wstring::npos && oldDot != std::wstring::npos) {
            name += m_oldName.substr(oldDot);
        }

        m_newName = name;
        m_confirmed = true;
        m_isRunning = false;
    }
};
