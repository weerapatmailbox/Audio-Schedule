#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "PresetManager.hpp"

class DeletePresetDialog {
public:
    static bool Show(HWND hParent, const PresetManager::PresetDeleteInfo& info) {
        DeletePresetDialog dlg(hParent, info);
        return dlg.DoModal();
    }

private:
    HWND m_hParent;
    PresetManager::PresetDeleteInfo m_info;
    bool m_confirmed = false;
    bool m_isRunning = false;

    HWND m_hwndDlg = NULL;
    HWND m_hEditDetails = NULL;
    HWND m_hBtnConfirm = NULL;
    HWND m_hBtnCancel = NULL;

    HFONT m_hFontNormal = NULL;
    HFONT m_hFontBold = NULL;
    HFONT m_hFontSmall = NULL;

    DeletePresetDialog(HWND hParent, const PresetManager::PresetDeleteInfo& info)
        : m_hParent(hParent), m_info(info) {}

    bool DoModal() {
        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
        if (!GetClassInfoExW(GetModuleHandle(NULL), L"ATP_DeletePresetDlgClass", &wc)) {
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.lpfnWndProc = DialogProcThunk;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = L"ATP_DeletePresetDlgClass";
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
            RegisterClassExW(&wc);
        }

        int dlgW = 600;
        int dlgH = 460;

        RECT parentRect;
        GetWindowRect(m_hParent, &parentRect);
        int x = parentRect.left + (parentRect.right - parentRect.left - dlgW) / 2;
        int y = parentRect.top + (parentRect.bottom - parentRect.top - dlgH) / 2;

        m_hwndDlg = CreateWindowExW(
            WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
            L"ATP_DeletePresetDlgClass",
            L"ยืนยันการลบชุดการเล่น (Delete Preset)",
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

        return m_confirmed;
    }

    static LRESULT CALLBACK DialogProcThunk(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        DeletePresetDialog* pThis = NULL;
        if (uMsg == WM_NCCREATE) {
            CREATESTRUCTW* pCS = (CREATESTRUCTW*)lParam;
            pThis = (DeletePresetDialog*)pCS->lpCreateParams;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        } else {
            pThis = (DeletePresetDialog*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        }

        if (pThis) {
            return pThis->DialogProc(hwnd, uMsg, wParam, lParam);
        }
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    LRESULT DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
            case WM_CREATE: {
                m_hwndDlg = hwnd;

                // Create fonts
                m_hFontNormal = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                           CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                m_hFontBold = CreateFontW(-15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                         CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                m_hFontSmall = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                          CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

                // Warning / Title Header
                std::wstring headerText = L"⚠️ คุณต้องการลบชุดการเล่น: [" + m_info.presetName + L"] หรือไม่?";
                HWND hTitle = CreateWindowW(L"STATIC", headerText.c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT,
                                           20, 16, 545, 24, hwnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hTitle, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

                // Folder Location Label
                std::wstring folderText = L"📁 โฟลเดอร์: " + m_info.presetFolder;
                HWND hFolder = CreateWindowW(L"STATIC", folderText.c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT,
                                            20, 42, 545, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hFolder, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);

                // Build detailed text description
                std::wstring details;
                details += L"==========================================================\r\n";
                details += L"🗑️ ไฟล์เสียงใน Preset (จะถูกลบออกจากเครื่องด้วย - รวม " + std::to_wstring(m_info.filesInPreset.size()) + L" ไฟล์):\r\n";
                details += L"==========================================================\r\n";
                if (m_info.filesInPreset.empty()) {
                    details += L"  (ไม่มีไฟล์เสียงที่คัดลอกไว้ในโฟลเดอร์ชุดนี้)\r\n";
                } else {
                    for (size_t i = 0; i < m_info.filesInPreset.size(); ++i) {
                        const wchar_t* p = wcsrchr(m_info.filesInPreset[i].c_str(), L'\\');
                        std::wstring fn = p ? (p + 1) : m_info.filesInPreset[i];
                        details += L"  " + std::to_wstring(i + 1) + L". " + fn + L"\r\n";
                    }
                }

                details += L"\r\n==========================================================\r\n";
                details += L"🔗 ไฟล์เสียงที่ LINK มาจากภายนอก (ปลอดภัย 100% ไม่ถูกลบ - รวม " + std::to_wstring(m_info.filesLinkedSafe.size()) + L" ไฟล์):\r\n";
                details += L"==========================================================\r\n";
                if (m_info.filesLinkedSafe.empty()) {
                    details += L"  (ไม่มีไฟล์เสียงภายนอกที่เชื่อมโยง)\r\n";
                } else {
                    for (size_t i = 0; i < m_info.filesLinkedSafe.size(); ++i) {
                        details += L"  " + std::to_wstring(i + 1) + L". " + m_info.filesLinkedSafe[i] + L"\r\n";
                    }
                }

                details += L"\r\n----------------------------------------------------------\r\n";
                details += L"คำเตือน: หากกดยืนยัน โฟลเดอร์ชุด Preset และไฟล์เสียงที่อยู่ในโฟลเดอร์นี้จะถูกลบทันที\r\n";
                details += L"ไฟล์ที่อยู่นอกโฟลเดอร์ (เช่น C:\\..., D:\\...) จะยังคงปลอดภัยและไม่ได้รับผลกระทบใดๆ";

                // Multi-line read-only details text box
                m_hEditDetails = CreateWindowExW(
                    WS_EX_CLIENTEDGE,
                    L"EDIT",
                    details.c_str(),
                    WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                    20, 68, 545, 280,
                    hwnd, NULL, GetModuleHandle(NULL), NULL
                );
                SendMessage(m_hEditDetails, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                // Bottom Buttons
                m_hBtnConfirm = CreateWindowW(
                    L"BUTTON", L"🗑️ ยืนยันการลบชุด Preset",
                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                    200, 368, 210, 34, hwnd, (HMENU)IDOK, GetModuleHandle(NULL), NULL
                );
                SendMessage(m_hBtnConfirm, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

                m_hBtnCancel = CreateWindowW(
                    L"BUTTON", L"ยกเลิก",
                    WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                    420, 368, 145, 34, hwnd, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL
                );
                SendMessage(m_hBtnCancel, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                return 0;
            }

            case WM_COMMAND: {
                int id = LOWORD(wParam);
                if (id == IDOK) {
                    m_confirmed = true;
                    m_isRunning = false;
                    return 0;
                } else if (id == IDCANCEL) {
                    m_confirmed = false;
                    m_isRunning = false;
                    return 0;
                }
                return 0;
            }

            case WM_CLOSE:
                m_confirmed = false;
                m_isRunning = false;
                return 0;

            case WM_DESTROY:
                if (m_hFontNormal) DeleteObject(m_hFontNormal);
                if (m_hFontBold) DeleteObject(m_hFontBold);
                if (m_hFontSmall) DeleteObject(m_hFontSmall);
                return 0;
        }
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
};
