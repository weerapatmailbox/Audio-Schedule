#pragma once
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <algorithm>
#include "ScheduleGroup.hpp"
#include "AudioEngine.hpp"

class GroupDialog {
public:
    static bool Show(HWND hParent, ScheduleGroup& group, bool isEditing = false) {
        GroupDialog dlg(hParent, group, isEditing);
        return dlg.DoModal();
    }

    int m_dragSourceIndex = -1;
    bool m_isDragging = false;
    POINT m_dragStartPoint = { 0, 0 };

    void OnDropItem(int srcIdx, int targetIdx) {
        if (srcIdx < 0 || srcIdx >= (int)m_audioFiles.size()) return;
        if (targetIdx < 0 || targetIdx >= (int)m_audioFiles.size()) return;
        if (srcIdx == targetIdx) return;

        std::wstring item = m_audioFiles[srcIdx];
        m_audioFiles.erase(m_audioFiles.begin() + srcIdx);
        m_audioFiles.insert(m_audioFiles.begin() + targetIdx, item);

        UpdateListBox();
        SendMessage(m_hListBox, LB_SETCURSEL, targetIdx, 0);
    }

private:
    HWND m_hParent;
    ScheduleGroup& m_group;
    bool m_isEditing;
    bool m_resultOk = false;
    bool m_isRunning = false;

    HWND m_hwndDlg = NULL;
    HWND m_hEditName = NULL;
    HWND m_hEditHour = NULL, m_hSpinHour = NULL;
    HWND m_hEditMin = NULL, m_hSpinMin = NULL;
    HWND m_hEditSec = NULL, m_hSpinSec = NULL;
    HWND m_hDays[7] = { NULL };
    HWND m_hListBox = NULL;
    HWND m_hChkEnabled = NULL;
    HFONT m_hFont = NULL;
    HFONT m_hFontBold = NULL;

    std::vector<std::wstring> m_audioFiles;

    GroupDialog(HWND hParent, ScheduleGroup& group, bool isEditing)
        : m_hParent(hParent), m_group(group), m_isEditing(isEditing) {
        m_audioFiles = group.audioFiles;
    }

    bool DoModal() {
        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
        if (!GetClassInfoExW(GetModuleHandle(NULL), L"ATP_GroupDialogClass", &wc)) {
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.lpfnWndProc = DialogProcThunk;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = L"ATP_GroupDialogClass";
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
            RegisterClassExW(&wc);
        }

        int dlgW = 620;
        int dlgH = 580;

        RECT parentRect;
        GetWindowRect(m_hParent, &parentRect);
        int x = parentRect.left + (parentRect.right - parentRect.left - dlgW) / 2;
        int y = parentRect.top + (parentRect.bottom - parentRect.top - dlgH) / 2;

        std::wstring title = m_isEditing ?
            ((Lang::current == Language::Thai) ? L"แก้ไขกลุ่มการเล่นเสียง" : L"Edit Schedule Group") :
            ((Lang::current == Language::Thai) ? L"สร้างกลุ่มการเล่นเสียงใหม่" : L"Create Schedule Group");

        m_hwndDlg = CreateWindowExW(
            WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
            L"ATP_GroupDialogClass",
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
                AudioEngine::Instance().Stop();
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
        GroupDialog* pThis = NULL;
        if (uMsg == WM_NCCREATE) {
            CREATESTRUCT* pCS = (CREATESTRUCT*)lParam;
            pThis = (GroupDialog*)pCS->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
            pThis->m_hwndDlg = hwnd;
        } else {
            pThis = (GroupDialog*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }

        if (pThis) {
            return pThis->DialogProc(hwnd, uMsg, wParam, lParam);
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    static LRESULT CALLBACK ListBoxSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        GroupDialog* pThis = (GroupDialog*)dwRefData;
        if (!pThis) return DefSubclassProc(hwnd, uMsg, wParam, lParam);

        switch (uMsg) {
            case WM_LBUTTONDOWN: {
                POINTS pts = MAKEPOINTS(lParam);
                DWORD res = (DWORD)SendMessage(hwnd, LB_ITEMFROMPOINT, 0, MAKELPARAM(pts.x, pts.y));
                if (HIWORD(res) == 0) {
                    int idx = LOWORD(res);
                    pThis->m_dragSourceIndex = idx;
                    pThis->m_isDragging = false;
                    pThis->m_dragStartPoint.x = pts.x;
                    pThis->m_dragStartPoint.y = pts.y;
                }
                break;
            }
            case WM_MOUSEMOVE: {
                if (pThis->m_dragSourceIndex >= 0 && (wParam & MK_LBUTTON)) {
                    POINTS pts = MAKEPOINTS(lParam);
                    int dx = pts.x - pThis->m_dragStartPoint.x;
                    int dy = pts.y - pThis->m_dragStartPoint.y;
                    if (!pThis->m_isDragging && (abs(dx) > 3 || abs(dy) > 3)) {
                        pThis->m_isDragging = true;
                        SetCapture(hwnd);
                        SetCursor(LoadCursor(NULL, IDC_SIZENS));
                    }
                    if (pThis->m_isDragging) {
                        SetCursor(LoadCursor(NULL, IDC_SIZENS));
                        DWORD res = (DWORD)SendMessage(hwnd, LB_ITEMFROMPOINT, 0, MAKELPARAM(pts.x, pts.y));
                        if (HIWORD(res) == 0) {
                            int hoverIdx = LOWORD(res);
                            SendMessage(hwnd, LB_SETCURSEL, hoverIdx, 0);
                        }
                        return 0;
                    }
                }
                break;
            }
            case WM_LBUTTONUP: {
                if (pThis->m_isDragging) {
                    ReleaseCapture();
                    SetCursor(LoadCursor(NULL, IDC_ARROW));
                    POINTS pts = MAKEPOINTS(lParam);
                    DWORD res = (DWORD)SendMessage(hwnd, LB_ITEMFROMPOINT, 0, MAKELPARAM(pts.x, pts.y));
                    int targetIdx = (HIWORD(res) == 0) ? LOWORD(res) : -1;
                    pThis->OnDropItem(pThis->m_dragSourceIndex, targetIdx);
                    pThis->m_isDragging = false;
                    pThis->m_dragSourceIndex = -1;
                    return 0;
                }
                pThis->m_dragSourceIndex = -1;
                break;
            }
        }
        return DefSubclassProc(hwnd, uMsg, wParam, lParam);
    }

    LRESULT DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
            case WM_CREATE:
                CreateControls();
                return 0;

            case WM_COMMAND: {
                int id = LOWORD(wParam);

                if (id == 1001) { // Add files
                    OnAddFiles();
                } else if (id == 1002) { // Remove file
                    OnRemoveFile();
                } else if (id == 1003) { // Move Up
                    MoveSelectedItem(-1);
                } else if (id == 1004) { // Move Down
                    MoveSelectedItem(1);
                } else if (id == 1005) { // Preview selected track
                    OnPreviewTrack();
                } else if (id == 1006) { // Preview entire group
                    OnPreviewGroup();
                } else if (id == 1007) { // Stop audio
                    AudioEngine::Instance().Stop();
                } else if (id == 1010) { // Select All Days
                    for (int i = 0; i < 7; ++i) SendMessage(m_hDays[i], BM_SETCHECK, BST_CHECKED, 0);
                } else if (id == 1011) { // Mon-Fri
                    for (int i = 0; i < 7; ++i) {
                        SendMessage(m_hDays[i], BM_SETCHECK, (i >= 1 && i <= 5) ? BST_CHECKED : BST_UNCHECKED, 0);
                    }
                } else if (id == IDOK) {
                    if (SaveDataFromControls()) {
                        m_resultOk = true;
                        AudioEngine::Instance().Stop();
                        m_isRunning = false;
                        DestroyWindow(hwnd);
                    }
                } else if (id == IDCANCEL) {
                    AudioEngine::Instance().Stop();
                    m_isRunning = false;
                    DestroyWindow(hwnd);
                }
                return 0;
            }

            case WM_CLOSE:
                AudioEngine::Instance().Stop();
                m_isRunning = false;
                DestroyWindow(hwnd);
                return 0;

            case WM_DESTROY:
                if (m_hFont) DeleteObject(m_hFont);
                if (m_hFontBold) DeleteObject(m_hFontBold);
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

        auto SetFont = [this](HWND h) {
            SendMessage(h, WM_SETFONT, (WPARAM)m_hFont, TRUE);
        };
        auto SetFontBold = [this](HWND h) {
            SendMessage(h, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);
        };

        int curY = 18;

        // Label: Group Name
        HWND lblName = CreateWindowW(L"STATIC", (Lang::current == Language::Thai) ? L"ชื่อกลุ่ม (Group Name):" : L"Group Name:", WS_CHILD | WS_VISIBLE, 25, curY, 150, 20, m_hwndDlg, NULL, NULL, NULL);
        SetFont(lblName);
        m_hEditName = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", m_group.name.c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                     180, curY - 2, 400, 24, m_hwndDlg, NULL, NULL, NULL);
        SetFont(m_hEditName);

        curY += 38;

        // Label: Start Time
        HWND lblTime = CreateWindowW(L"STATIC", (Lang::current == Language::Thai) ? L"เวลาเริ่มเล่น (Start Time):" : L"Start Time:", WS_CHILD | WS_VISIBLE, 25, curY, 150, 20, m_hwndDlg, NULL, NULL, NULL);
        SetFont(lblTime);

        // Hour Spin
        m_hEditHour = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER,
                                      180, curY - 2, 45, 24, m_hwndDlg, NULL, NULL, NULL);
        m_hSpinHour = CreateWindowW(UPDOWN_CLASSW, L"", WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS,
                                    0, 0, 0, 0, m_hwndDlg, NULL, NULL, NULL);
        SendMessage(m_hSpinHour, UDM_SETBUDDY, (WPARAM)m_hEditHour, 0);
        SendMessage(m_hSpinHour, UDM_SETRANGE32, 0, 23);
        SendMessage(m_hSpinHour, UDM_SETPOS32, 0, m_group.hour);
        SetFont(m_hEditHour);

        HWND lblSep1 = CreateWindowW(L"STATIC", L":", WS_CHILD | WS_VISIBLE, 230, curY, 10, 20, m_hwndDlg, NULL, NULL, NULL);
        SetFont(lblSep1);

        // Minute Spin
        m_hEditMin = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER,
                                     245, curY - 2, 45, 24, m_hwndDlg, NULL, NULL, NULL);
        m_hSpinMin = CreateWindowW(UPDOWN_CLASSW, L"", WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS,
                                   0, 0, 0, 0, m_hwndDlg, NULL, NULL, NULL);
        SendMessage(m_hSpinMin, UDM_SETBUDDY, (WPARAM)m_hEditMin, 0);
        SendMessage(m_hSpinMin, UDM_SETRANGE32, 0, 59);
        SendMessage(m_hSpinMin, UDM_SETPOS32, 0, m_group.minute);
        SetFont(m_hEditMin);

        HWND lblSep2 = CreateWindowW(L"STATIC", L":", WS_CHILD | WS_VISIBLE, 295, curY, 10, 20, m_hwndDlg, NULL, NULL, NULL);
        SetFont(lblSep2);

        // Second Spin
        m_hEditSec = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER,
                                     310, curY - 2, 45, 24, m_hwndDlg, NULL, NULL, NULL);
        m_hSpinSec = CreateWindowW(UPDOWN_CLASSW, L"", WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS,
                                   0, 0, 0, 0, m_hwndDlg, NULL, NULL, NULL);
        SendMessage(m_hSpinSec, UDM_SETBUDDY, (WPARAM)m_hEditSec, 0);
        SendMessage(m_hSpinSec, UDM_SETRANGE32, 0, 59);
        SendMessage(m_hSpinSec, UDM_SETPOS32, 0, m_group.second);
        SetFont(m_hEditSec);

        HWND lblFormat = CreateWindowW(L"STATIC", (Lang::current == Language::Thai) ? L"(ชม:นาที:วินาที)" : L"(HH:MM:SS)", WS_CHILD | WS_VISIBLE, 370, curY, 130, 20, m_hwndDlg, NULL, NULL, NULL);
        SetFont(lblFormat);

        curY += 38;

        // Label: Days
        HWND lblDays = CreateWindowW(L"STATIC", (Lang::current == Language::Thai) ? L"วันที่เล่น (Days):" : L"Active Days:", WS_CHILD | WS_VISIBLE, 25, curY, 150, 20, m_hwndDlg, NULL, NULL, NULL);
        SetFont(lblDays);

        const wchar_t* const dayLabels[] = {
            Lang::DaySun(), Lang::DayMon(), Lang::DayTue(), Lang::DayWed(),
            Lang::DayThu(), Lang::DayFri(), Lang::DaySat()
        };
        int chkX = 180;
        for (int i = 0; i < 7; ++i) {
            m_hDays[i] = CreateWindowW(L"BUTTON", dayLabels[i], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                       chkX, curY - 2, 45, 22, m_hwndDlg, NULL, NULL, NULL);
            SetFont(m_hDays[i]);
            SendMessage(m_hDays[i], BM_SETCHECK, m_group.IsDayActive(i) ? BST_CHECKED : BST_UNCHECKED, 0);
            chkX += 46;
        }

        curY += 28;
        HWND btnAll = CreateWindowW(L"BUTTON", (Lang::current == Language::Thai) ? L"เลือกทุกวัน" : L"All Days", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 180, curY, 85, 24, m_hwndDlg, (HMENU)1010, NULL, NULL);
        SetFont(btnAll);
        HWND btnWork = CreateWindowW(L"BUTTON", (Lang::current == Language::Thai) ? L"จันทร์-ศุกร์" : L"Mon - Fri", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 275, curY, 90, 24, m_hwndDlg, (HMENU)1011, NULL, NULL);
        SetFont(btnWork);

        curY += 38;

        // Group files section header
        HWND lblFilesHead = CreateWindowW(L"STATIC", (Lang::current == Language::Thai) ? L"ไฟล์เสียงในกลุ่ม (เล่นต่อกันตามลำดับ • ลากเพื่อเปลี่ยนลำดับ):" : L"Group Audio Tracks (Plays sequentially • Drag to reorder):",
                                         WS_CHILD | WS_VISIBLE, 25, curY, 550, 20, m_hwndDlg, NULL, NULL, NULL);
        SetFontBold(lblFilesHead);

        curY += 24;

        // Files ListBox
        m_hListBox = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"LISTBOX",
            L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
            25, curY, 440, 180,
            m_hwndDlg, (HMENU)2001, NULL, NULL
        );
        SetFont(m_hListBox);
        SetWindowSubclass(m_hListBox, ListBoxSubclassProc, 1, (DWORD_PTR)this);

        // Right side buttons for ListBox
        int btnX = 475;
        int sideY = curY;
        auto CreateSideBtn = [this, btnX](const wchar_t* text, int id, int y, int w = 115) {
            HWND h = CreateWindowW(L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                   btnX, y, w, 28, m_hwndDlg, (HMENU)(UINT_PTR)id, NULL, NULL);
            SendMessage(h, WM_SETFONT, (WPARAM)m_hFont, TRUE);
            return h;
        };

        CreateSideBtn((Lang::current == Language::Thai) ? L"➕ เพิ่มไฟล์..." : L"➕ Add Files...", 1001, sideY); sideY += 34;
        CreateSideBtn((Lang::current == Language::Thai) ? L"❌ ลบไฟล์" : L"❌ Remove", 1002, sideY); sideY += 34;
        CreateSideBtn((Lang::current == Language::Thai) ? L"⬆️ ย้ายขึ้น" : L"⬆️ Move Up", 1003, sideY); sideY += 34;
        CreateSideBtn((Lang::current == Language::Thai) ? L"⬇️ ย้ายลง" : L"⬇️ Move Down", 1004, sideY); sideY += 34;
        CreateSideBtn((Lang::current == Language::Thai) ? L"▶️ ฟังไฟล์นี้" : L"▶️ Preview Track", 1005, sideY); sideY += 34;

        curY += 190;

        // Group playback test buttons
        HWND btnGroupPlay = CreateWindowW(L"BUTTON", (Lang::current == Language::Thai) ? L"▶ ฟังทั้งกลุ่ม (Play Group)" : L"▶ Play Group", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                         25, curY, 180, 28, m_hwndDlg, (HMENU)1006, NULL, NULL);
        SetFont(btnGroupPlay);

        HWND btnStop = CreateWindowW(L"BUTTON", (Lang::current == Language::Thai) ? L"⏹ หยุดเสียง" : L"⏹ Stop", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                    215, curY, 95, 28, m_hwndDlg, (HMENU)1007, NULL, NULL);
        SetFont(btnStop);

        curY += 40;

        // Active Checkbox
        HWND chkEnabled = CreateWindowW(L"BUTTON", (Lang::current == Language::Thai) ? L"เปิดใช้งานกลุ่มนี้ (Active)" : L"Enable this group (Active)", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                      25, curY, 250, 22, m_hwndDlg, NULL, NULL, NULL);
        m_hChkEnabled = chkEnabled;
        SetFont(m_hChkEnabled);
        SendMessage(m_hChkEnabled, BM_SETCHECK, m_group.enabled ? BST_CHECKED : BST_UNCHECKED, 0);

        curY += 45;

        // OK / Cancel buttons
        HWND btnOk = CreateWindowW(L"BUTTON", (Lang::current == Language::Thai) ? L"ตกลง (OK)" : L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                   380, curY, 100, 32, m_hwndDlg, (HMENU)IDOK, NULL, NULL);
        SetFontBold(btnOk);

        HWND btnCancel = CreateWindowW(L"BUTTON", (Lang::current == Language::Thai) ? L"ยกเลิก (Cancel)" : L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                       490, curY, 100, 32, m_hwndDlg, (HMENU)IDCANCEL, NULL, NULL);
        SetFont(btnCancel);

        UpdateListBox();
    }

    void UpdateListBox() {
        SendMessage(m_hListBox, LB_RESETCONTENT, 0, 0);
        for (size_t i = 0; i < m_audioFiles.size(); ++i) {
            const wchar_t* fn = wcsrchr(m_audioFiles[i].c_str(), L'\\');
            std::wstring display = std::to_wstring(i + 1) + L". " + (fn ? (fn + 1) : m_audioFiles[i]);
            SendMessage(m_hListBox, LB_ADDSTRING, 0, (LPARAM)display.c_str());
        }
    }

    void OnAddFiles() {
        wchar_t szFiles[32768] = { 0 };
        OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
        ofn.hwndOwner = m_hwndDlg;
        ofn.lpstrFile = szFiles;
        ofn.nMaxFile = 32768;
        ofn.lpstrFilter = L"ไฟล์เสียง (*.mp3;*.wav;*.wma;*.m4a)\0*.mp3;*.wav;*.wma;*.m4a\0ไฟล์ทั้งหมด (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

        if (GetOpenFileNameW(&ofn)) {
            wchar_t* p = szFiles;
            std::wstring dir = p;
            p += dir.length() + 1;
            if (*p == L'\0') {
                // Single file selected
                m_audioFiles.push_back(dir);
            } else {
                // Multiple files selected
                while (*p != L'\0') {
                    std::wstring fileName = p;
                    std::wstring fullPath = dir;
                    if (!fullPath.empty() && fullPath.back() != L'\\') fullPath += L'\\';
                    fullPath += fileName;
                    m_audioFiles.push_back(fullPath);
                    p += fileName.length() + 1;
                }
            }
            UpdateListBox();
            SendMessage(m_hListBox, LB_SETCURSEL, (WPARAM)(m_audioFiles.size() - 1), 0);

            // If group name is empty, auto-fill with first file name
            wchar_t nameBuf[256] = { 0 };
            GetWindowTextW(m_hEditName, nameBuf, 256);
            if (wcslen(nameBuf) == 0 && !m_audioFiles.empty()) {
                const wchar_t* fn = wcsrchr(m_audioFiles[0].c_str(), L'\\');
                SetWindowTextW(m_hEditName, fn ? (fn + 1) : m_audioFiles[0].c_str());
            }
        }
    }

    void OnRemoveFile() {
        int cur = (int)SendMessage(m_hListBox, LB_GETCURSEL, 0, 0);
        if (cur < 0 || cur >= (int)m_audioFiles.size()) {
            MessageBoxW(m_hwndDlg, L"กรุณาเลือกไฟล์ที่ต้องการลบออกจากกลุ่ม", L"แจ้งเตือน", MB_OK | MB_ICONINFORMATION);
            return;
        }

        m_audioFiles.erase(m_audioFiles.begin() + cur);
        UpdateListBox();
        if (cur >= (int)m_audioFiles.size()) cur = (int)m_audioFiles.size() - 1;
        if (cur >= 0) SendMessage(m_hListBox, LB_SETCURSEL, cur, 0);
    }

    void MoveSelectedItem(int delta) {
        int cur = (int)SendMessage(m_hListBox, LB_GETCURSEL, 0, 0);
        if (cur < 0 || cur >= (int)m_audioFiles.size()) return;
        int target = cur + delta;
        if (target < 0 || target >= (int)m_audioFiles.size()) return;

        std::swap(m_audioFiles[cur], m_audioFiles[target]);
        UpdateListBox();
        SendMessage(m_hListBox, LB_SETCURSEL, target, 0);
    }

    void OnPreviewTrack() {
        int cur = (int)SendMessage(m_hListBox, LB_GETCURSEL, 0, 0);
        if (cur < 0 || cur >= (int)m_audioFiles.size()) {
            MessageBoxW(m_hwndDlg, L"กรุณาเลือกไฟล์ที่ต้องการฟัง", L"แจ้งเตือน", MB_OK | MB_ICONINFORMATION);
            return;
        }
        AudioEngine::Instance().PlaySingle(m_audioFiles[cur], m_hwndDlg);
    }

    void OnPreviewGroup() {
        if (m_audioFiles.empty()) {
            MessageBoxW(m_hwndDlg, L"กรุณาเพิ่มไฟล์เสียงเข้ากลุ่มก่อนทดสอบ", L"แจ้งเตือน", MB_OK | MB_ICONINFORMATION);
            return;
        }
        wchar_t nameBuf[256] = { 0 };
        GetWindowTextW(m_hEditName, nameBuf, 256);
        std::wstring gName = (wcslen(nameBuf) > 0) ? nameBuf : L"ทดสอบกลุ่ม";
        AudioEngine::Instance().PlayGroup(gName, m_audioFiles, m_hwndDlg);
    }

    bool SaveDataFromControls() {
        wchar_t nameBuf[256] = { 0 };
        GetWindowTextW(m_hEditName, nameBuf, 256);
        m_group.name = nameBuf;

        m_group.hour = (int)SendMessage(m_hSpinHour, UDM_GETPOS32, 0, 0);
        m_group.minute = (int)SendMessage(m_hSpinMin, UDM_GETPOS32, 0, 0);
        m_group.second = (int)SendMessage(m_hSpinSec, UDM_GETPOS32, 0, 0);

        uint8_t mask = 0;
        for (int i = 0; i < 7; ++i) {
            if (SendMessage(m_hDays[i], BM_GETCHECK, 0, 0) == BST_CHECKED) {
                mask |= (1 << i);
            }
        }
        m_group.daysMask = mask;

        m_group.enabled = (SendMessage(m_hChkEnabled, BM_GETCHECK, 0, 0) == BST_CHECKED);
        m_group.audioFiles = m_audioFiles;

        if (m_group.name.empty()) {
            m_group.name = L"กลุ่มการเล่นที่ " + std::to_wstring(m_group.id);
        }

        return true;
    }
};

// Typedef for compatibility
typedef GroupDialog ItemDialog;
