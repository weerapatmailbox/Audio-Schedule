#pragma once
#include <windows.h>
#include <shellapi.h>
#include <string>
#include "Version.h"

class AboutDialog {
public:
    static void Show(HWND hParent) {
        AboutDialog dlg(hParent);
        dlg.DoModal();
    }

private:
    HWND m_hParent;
    bool m_isRunning = false;

    HWND m_hwndDlg = NULL;
    HWND m_hEditLicense = NULL;
    HWND m_hBtnCopy = NULL;
    HWND m_hBtnWebsite = NULL;
    HWND m_hBtnOk = NULL;
    HWND m_hLinkWebsite = NULL;
    HWND m_hLinkEmail = NULL;
    HICON m_hIcon = NULL;

    HFONT m_hFontTitle = NULL;
    HFONT m_hFontSub = NULL;
    HFONT m_hFontBold = NULL;
    HFONT m_hFontNormal = NULL;
    HFONT m_hFontLink = NULL;
    HFONT m_hFontMono = NULL;

    enum {
        IDC_BTN_COPY_INFO = 3101,
        IDC_BTN_WEBSITE = 3102,
        IDC_LINK_WEBSITE = 3103,
        IDC_LINK_EMAIL = 3104,
        IDC_BTN_OK = IDOK
    };

    explicit AboutDialog(HWND hParent) : m_hParent(hParent) {}

    bool DoModal() {
        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
        if (!GetClassInfoExW(GetModuleHandle(NULL), L"ATP_AboutDlgClass", &wc)) {
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.lpfnWndProc = DialogProcThunk;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = L"ATP_AboutDlgClass";
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
            RegisterClassExW(&wc);
        }

        int dlgW = 660;
        int dlgH = 580;

        RECT parentRect;
        if (m_hParent && IsWindow(m_hParent)) {
            GetWindowRect(m_hParent, &parentRect);
        } else {
            parentRect.left = 100;
            parentRect.right = 700;
            parentRect.top = 100;
            parentRect.bottom = 640;
        }

        int x = parentRect.left + (parentRect.right - parentRect.left - dlgW) / 2;
        int y = parentRect.top + (parentRect.bottom - parentRect.top - dlgH) / 2;
        if (x < 10) x = 10;
        if (y < 10) y = 10;

        std::wstring dlgTitle = L"เกี่ยวกับโปรแกรม " APP_NAME_W L" (About)";
        m_hwndDlg = CreateWindowExW(
            WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
            L"ATP_AboutDlgClass",
            dlgTitle.c_str(),
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

            if (msg.message == WM_KEYDOWN) {
                if (msg.wParam == VK_ESCAPE || msg.wParam == VK_RETURN) {
                    m_isRunning = false;
                    break;
                }
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

        if (m_hParent && IsWindow(m_hParent)) {
            EnableWindow(m_hParent, TRUE);
            SetForegroundWindow(m_hParent);
        }

        return true;
    }

    static LRESULT CALLBACK DialogProcThunk(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        AboutDialog* pThis = NULL;
        if (uMsg == WM_NCCREATE) {
            CREATESTRUCTW* pCS = (CREATESTRUCTW*)lParam;
            pThis = (AboutDialog*)pCS->lpCreateParams;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        } else {
            pThis = (AboutDialog*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        }

        if (pThis) {
            return pThis->DialogProc(hwnd, uMsg, wParam, lParam);
        }
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    void CopyInfoToClipboard() {
#ifdef _WIN64
        std::wstring archStr = L" (64-bit x64)";
#else
        std::wstring archStr = L" (32-bit x86)";
#endif
        std::wstring info =
            APP_NAME_W L" v" APP_VERSION_WSTRING + archStr + L"\r\n"
            APP_DESC_W L"\r\n\r\n"
            L"รุ่นสถาปัตยกรรม (Architecture): "
#ifdef _WIN64
            + std::wstring(L"64-bit (x64 Native Build)\r\n")
#else
            + std::wstring(L"32-bit (x86 Compatible Build)\r\n")
#endif
            + L"ผู้พัฒนา (Author): " APP_AUTHOR_W L"\r\n"
            L"ทีม (Team): " APP_TEAM_W L"\r\n"
            L"เว็บไซต์ (Website): " APP_WEBSITE_URL_W L"\r\n"
            L"อีเมล (Email): " APP_EMAIL_W L"\r\n"
            L"สัญญาอนุญาต (License): MIT License\r\n"
            L"ลิขสิทธิ์ (Copyright): " APP_COPYRIGHT_W L"\r\n\r\n"
            L"--- MIT LICENSE ---\r\n"
            L"Copyright (c) 2026 Mr.Weerapat Nusui <weerapat.mailbox@gmail.com> & Softlunar Foundation\r\n\r\n"
            L"Permission is hereby granted, free of charge, to any person obtaining a copy\r\n"
            L"of this software and associated documentation files (the \"Software\"), to deal\r\n"
            L"in the Software without restriction, including without limitation the rights\r\n"
            L"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\r\n"
            L"copies of the Software, and to permit persons to whom the Software is\r\n"
            L"furnished to do so, subject to the following conditions:\r\n\r\n"
            L"The above copyright notice and this permission notice shall be included in all\r\n"
            L"copies or substantial portions of the Software.\r\n\r\n"
            L"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\r\n"
            L"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\r\n"
            L"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\r\n"
            L"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\r\n"
            L"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\r\n"
            L"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\r\n"
            L"SOFTWARE.\r\n";

        if (OpenClipboard(m_hwndDlg)) {
            EmptyClipboard();
            size_t bytes = (info.size() + 1) * sizeof(wchar_t);
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
            if (hMem) {
                void* pMem = GlobalLock(hMem);
                if (pMem) {
                    memcpy(pMem, info.c_str(), bytes);
                    GlobalUnlock(hMem);
                    SetClipboardData(CF_UNICODETEXT, hMem);
                }
            }
            CloseClipboard();
            if (m_hBtnCopy) {
                SetWindowTextW(m_hBtnCopy, L"✅ คัดลอกแล้ว!");
            }
        }
    }

    LRESULT DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
            case WM_CREATE: {
                m_hwndDlg = hwnd;

                // Create fonts
                m_hFontTitle = CreateFontW(-20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                          CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                m_hFontSub = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                m_hFontBold = CreateFontW(-14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                         CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                m_hFontNormal = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                           CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                m_hFontLink = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, TRUE, FALSE,
                                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                         CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                m_hFontMono = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                         CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");

                // Application Icon
                m_hIcon = (HICON)LoadImageW(GetModuleHandle(NULL), MAKEINTRESOURCEW(1), IMAGE_ICON, 48, 48, LR_DEFAULTCOLOR);
                if (!m_hIcon) {
                    m_hIcon = LoadIconW(NULL, IDI_APPLICATION);
                }
                HWND hStaticIcon = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ICON,
                                                24, 18, 48, 48, hwnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hStaticIcon, STM_SETICON, (WPARAM)m_hIcon, 0);

                // Title Label
#ifdef _WIN64
                std::wstring archText = L" (64-bit x64)";
#else
                std::wstring archText = L" (32-bit x86)";
#endif
                std::wstring titleText = APP_NAME_W L"  v" APP_VERSION_WSTRING + archText;
                HWND hTitle = CreateWindowW(L"STATIC", titleText.c_str(), WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_LEFT,
                                            82, 16, 480, 24, hwnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hTitle, WM_SETFONT, (WPARAM)m_hFontTitle, TRUE);

                // Subtitle Label
                HWND hSub = CreateWindowW(L"STATIC", APP_DESC_W,
                                          WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_LEFT,
                                          82, 42, 480, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hSub, WM_SETFONT, (WPARAM)m_hFontSub, TRUE);

                // Separator
                CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
                              24, 70, 536, 2, hwnd, NULL, GetModuleHandle(NULL), NULL);

                // ==============================================================
                // Two-Column Layout for Metadata (Labels Left, Values Right)
                // ย่อหน้าและคอลัมน์ตรงกันอย่างสมบูรณ์ตามตำแหน่งในภาพ
                // ==============================================================

                // --- Column 1: Labels (Left: x = 24, w = 170) ---
                HWND hLblAuthor = CreateWindowW(L"STATIC", L"👤 ผู้พัฒนา (Author):",
                    WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_LEFT, 24, 78, 170, 18, hwnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hLblAuthor, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                HWND hLblTeam = CreateWindowW(L"STATIC", L"👥 ทีม (Team):",
                    WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_LEFT, 24, 100, 170, 18, hwnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hLblTeam, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                HWND hLblWeb = CreateWindowW(L"STATIC", L"🌐 เว็บไซต์ (Website):",
                    WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_LEFT, 24, 122, 170, 18, hwnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hLblWeb, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                HWND hLblEmail = CreateWindowW(L"STATIC", L"📧 อีเมล (Email):",
                    WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_LEFT, 24, 144, 170, 18, hwnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hLblEmail, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                HWND hLblLicense = CreateWindowW(L"STATIC", L"📜 สัญญาอนุญาต (License):",
                    WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_LEFT, 24, 166, 170, 18, hwnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hLblLicense, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                HWND hLblCopy = CreateWindowW(L"STATIC", L"⚖️ ลิขสิทธิ์ (Copyright):",
                    WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_LEFT, 24, 188, 170, 18, hwnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hLblCopy, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                // --- Column 2: Values (Right: x = 196, w = 364) ---
                HWND hValAuthor = CreateWindowW(L"STATIC", APP_AUTHOR_W,
                    WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_LEFT, 196, 78, 364, 18, hwnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hValAuthor, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                HWND hValTeam = CreateWindowW(L"STATIC", APP_TEAM_W,
                    WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_LEFT, 196, 100, 364, 18, hwnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hValTeam, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                // Clickable Website Link
                m_hLinkWebsite = CreateWindowW(L"STATIC", APP_WEBSITE_W,
                    WS_CHILD | WS_VISIBLE | SS_NOTIFY | SS_NOPREFIX | SS_LEFT,
                    196, 122, 364, 18, hwnd, (HMENU)IDC_LINK_WEBSITE, GetModuleHandle(NULL), NULL);
                SendMessage(m_hLinkWebsite, WM_SETFONT, (WPARAM)m_hFontLink, TRUE);

                // Clickable Email Link
                m_hLinkEmail = CreateWindowW(L"STATIC", APP_EMAIL_W,
                    WS_CHILD | WS_VISIBLE | SS_NOTIFY | SS_NOPREFIX | SS_LEFT,
                    196, 144, 364, 18, hwnd, (HMENU)IDC_LINK_EMAIL, GetModuleHandle(NULL), NULL);
                SendMessage(m_hLinkEmail, WM_SETFONT, (WPARAM)m_hFontLink, TRUE);

                // License Value with SS_NOPREFIX so '&' is not hidden
                HWND hValLicense = CreateWindowW(L"STATIC", L"MIT License (Free & Open Source Software)",
                    WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_LEFT, 196, 166, 364, 18, hwnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hValLicense, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                // Copyright Value (2 lines to ensure full display without cut-off)
                HWND hValCopy = CreateWindowW(L"STATIC", APP_COPYRIGHT_W,
                    WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_LEFT, 196, 188, 430, 36, hwnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hValCopy, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                // License Section Header
                HWND hSecHeader = CreateWindowW(L"STATIC", L"ข้อกำหนดสัญญาอนุญาตและข้อมูลโปรแกรม (License & Specifications):",
                                               WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_LEFT,
                                               24, 230, 600, 18, hwnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hSecHeader, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

                // License & Spec text content
                std::wstring textContent;
                textContent += L"MIT License\r\n\r\n";
                textContent += L"Copyright (c) 2026 Mr.Weerapat Nusui <weerapat.mailbox@gmail.com> & Softlunar Foundation\r\n\r\n";
                textContent += L"Permission is hereby granted, free of charge, to any person obtaining a copy\r\n";
                textContent += L"of this software and associated documentation files (the \"Software\"), to deal\r\n";
                textContent += L"in the Software without restriction, including without limitation the rights\r\n";
                textContent += L"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\r\n";
                textContent += L"copies of the Software, and to permit persons to whom the Software is\r\n";
                textContent += L"furnished to do so, subject to the following conditions:\r\n\r\n";
                textContent += L"The above copyright notice and this permission notice shall be included in all\r\n";
                textContent += L"copies or substantial portions of the Software.\r\n\r\n";
                textContent += L"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\r\n";
                textContent += L"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\r\n";
                textContent += L"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\r\n";
                textContent += L"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\r\n";
                textContent += L"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\r\n";
                textContent += L"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\r\n";
                textContent += L"SOFTWARE.\r\n\r\n";
                textContent += L"============================================================\r\n";
                textContent += L"ข้อมูลทางเทคนิคและสถาปัตยกรรม (Technical Specifications)\r\n";
                textContent += L"============================================================\r\n";
                textContent += L"• รุ่นของโปรแกรมที่กำลังทำงาน (Active Architecture): "
#ifdef _WIN64
                    + std::wstring(L"64-bit (x64 Native Build)\r\n")
#else
                    + std::wstring(L"32-bit (x86 Compatible Build)\r\n")
#endif
                    ;
                textContent += L"• ผู้พัฒนา (Author): " APP_AUTHOR_W L"\r\n";
                textContent += L"• ทีม / องค์กร (Team): " APP_TEAM_W L"\r\n";
                textContent += L"• เว็บไซต์ทางการ (Website): " APP_WEBSITE_URL_W L"\r\n";
                textContent += L"• ภาษาและมาตรฐาน (Language): Modern C++ (C++17 ISO Standard)\r\n";
                textContent += L"• สถาปัตยกรรม (Framework): Native Win32 API & Common Controls 6.0\r\n";
                textContent += L"• คอมไพเลอร์ (Toolchain): MinGW-w64 GCC/G++ 16.2.0 (Static Linking)\r\n";
                textContent += L"• แพลตฟอร์มที่รองรับ (Platform): Windows XP / Vista / 7 / 8 / 8.1 / 10 / 11 (32-bit & 64-bit)\r\n";
                textContent += L"• รูปแบบโปรแกรม: Portable ZIP และ Setup Installer (User Mode & Admin Mode)\r\n";
                textContent += L"• การรองรับหลายภาษา (Multilingual): English (Default) และ ภาษาไทย (สลับภาษาได้ทันที)\r\n";
                textContent += L"• การเริ่มต้นระบบ (Startup): เปิดโปรแกรมอัตโนมัติเมื่อเปิดเครื่อง (Windows Startup Registry)\r\n";
                textContent += L"• ระบบเสียง (Audio Engine): Windows MCI & Direct Media Subsystem (Non-blocking Threading)\r\n";
                textContent += L"• รูปแบบเสียงที่รองรับ: MP3 (.mp3), WAV (.wav), WMA (.wma), M4A (.m4a), MID (.mid)\r\n";
                textContent += L"• ระบบจัดเก็บข้อมูล: Portable Preset Packages (.asch), ZIP Backup/Restore และ Local Presets\r\n";
                textContent += L"• การพึ่งพาภายนอก (Dependencies): Zero External Runtime Dependencies (100% Standalone)\r\n";

                // Read-only multi-line Edit control
                m_hEditLicense = CreateWindowExW(
                    WS_EX_CLIENTEDGE,
                    L"EDIT",
                    textContent.c_str(),
                    WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                    24, 252, 600, 196,
                    hwnd, NULL, GetModuleHandle(NULL), NULL
                );
                SendMessage(m_hEditLicense, WM_SETFONT, (WPARAM)m_hFontMono, TRUE);

                // Bottom Buttons
                m_hBtnCopy = CreateWindowW(
                    L"BUTTON", L"📋 คัดลอกข้อมูล",
                    WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                    24, 464, 160, 34, hwnd, (HMENU)IDC_BTN_COPY_INFO, GetModuleHandle(NULL), NULL
                );
                SendMessage(m_hBtnCopy, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                m_hBtnWebsite = CreateWindowW(
                    L"BUTTON", L"🌐 softlunar.com",
                    WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                    194, 464, 170, 34, hwnd, (HMENU)IDC_BTN_WEBSITE, GetModuleHandle(NULL), NULL
                );
                SendMessage(m_hBtnWebsite, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                m_hBtnOk = CreateWindowW(
                    L"BUTTON", L"ปิด (Close)",
                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                    504, 464, 120, 34, hwnd, (HMENU)IDC_BTN_OK, GetModuleHandle(NULL), NULL
                );
                SendMessage(m_hBtnOk, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

                return 0;
            }

            case WM_SETCURSOR: {
                HWND hTarget = (HWND)wParam;
                if (hTarget == m_hLinkWebsite || hTarget == m_hLinkEmail) {
                    SetCursor(LoadCursor(NULL, IDC_HAND));
                    return TRUE;
                }
                break;
            }

            case WM_CTLCOLORSTATIC: {
                HDC hdc = (HDC)wParam;
                HWND hCtrl = (HWND)lParam;
                if (hCtrl == m_hEditLicense) {
                    SetBkColor(hdc, RGB(255, 255, 255));
                    return (LRESULT)GetStockObject(WHITE_BRUSH);
                }
                if (hCtrl == m_hLinkWebsite || hCtrl == m_hLinkEmail) {
                    SetTextColor(hdc, RGB(0, 102, 204));
                    SetBkMode(hdc, TRANSPARENT);
                    return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
                }
                SetBkMode(hdc, TRANSPARENT);
                return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
            }

            case WM_COMMAND: {
                int id = LOWORD(wParam);
                if (id == IDC_BTN_COPY_INFO) {
                    CopyInfoToClipboard();
                    return 0;
                } else if (id == IDC_BTN_WEBSITE || id == IDC_LINK_WEBSITE) {
                    ShellExecuteW(hwnd, L"open", APP_WEBSITE_URL_W, NULL, NULL, SW_SHOWNORMAL);
                    return 0;
                } else if (id == IDC_LINK_EMAIL) {
                    std::wstring mailto = L"mailto:" + std::wstring(APP_EMAIL_W);
                    ShellExecuteW(hwnd, L"open", mailto.c_str(), NULL, NULL, SW_SHOWNORMAL);
                    return 0;
                } else if (id == IDC_BTN_OK || id == IDCANCEL) {
                    m_isRunning = false;
                    return 0;
                }
                return 0;
            }

            case WM_CLOSE:
                m_isRunning = false;
                return 0;

            case WM_DESTROY:
                if (m_hFontTitle) DeleteObject(m_hFontTitle);
                if (m_hFontSub) DeleteObject(m_hFontSub);
                if (m_hFontBold) DeleteObject(m_hFontBold);
                if (m_hFontNormal) DeleteObject(m_hFontNormal);
                if (m_hFontLink) DeleteObject(m_hFontLink);
                if (m_hFontMono) DeleteObject(m_hFontMono);
                if (m_hIcon) DestroyIcon(m_hIcon);
                return 0;
        }
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
};
