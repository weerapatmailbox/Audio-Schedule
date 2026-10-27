#pragma once
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include "Security.hpp"
#include <string>
#include <vector>
#include <algorithm>
#include "Version.h"
#include "Language.h"
#include "AppSettings.hpp"
#include "ScheduleGroup.hpp"
#include "AudioEngine.hpp"
#include "PresetManager.hpp"
#include "GroupDialog.hpp"
#include "SavePresetDialog.hpp"
#include "DeletePresetDialog.hpp"
#include "NewPresetDialog.hpp"
#include "AboutDialog.hpp"
#include "ManualDialog.hpp"
#include "SettingsDialog.hpp"
#include "RenameAudioDialog.hpp"
#include "RenamePresetDialog.hpp"

#define ID_TIMER_TICK           101
#define IDM_FILE_NEW            2001
#define IDM_FILE_SAVE           2003
#define IDM_FILE_SAVEAS         2007
#define IDM_FILE_EXIT           2004
#define IDM_HELP_ABOUT          2005
#define IDM_FILE_DELETE_PRESET  2006
#define IDM_HELP_MANUAL         2008
#define IDM_HELP_SETTINGS       2009

#define IDM_BACKUP_EXPORT_ASCH  2101
#define IDM_BACKUP_IMPORT       2103
#define IDM_BACKUP_EXPORT_ZIP   2104
#define IDM_BACKUP_RESTORE_ZIP  2105
#define IDM_BACKUP_OPEN_FOLDER  2106

#define IDM_LANG_ENGLISH        2201
#define IDM_LANG_THAI           2202

#define WM_TRAYICON             (WM_APP + 100)
#define IDM_TRAY_OPEN           2301
#define IDM_TRAY_STOP           2302
#define IDM_TRAY_EXIT           2303

#define IDC_COMBO_PRESETS       2010
#define IDC_BTN_MANAGE_PRESET   2013
#define IDM_PRESET_NEW          2014
#define IDM_PRESET_EDIT         2015
#define IDM_PRESET_DEL          2016
#define IDM_DELETE_ITEM         2017
#define IDC_BTN_NEW_PRESET      2012
#define IDC_BTN_DEL_PRESET      2011

// Left Panel IDs (Schedule groups)
#define IDC_LISTVIEW            3010
#define IDC_BTN_GRP_ADD         3001
#define IDC_BTN_GRP_EDIT        3002
#define IDC_BTN_GRP_DEL         3003
#define IDC_BTN_GRP_TOGGLE      3004

// Right Panel IDs (Audio files in selected group)
#define IDC_RIGHT_LISTVIEW      1100
#define IDC_BTN_FILE_ADD        1101
#define IDC_BTN_FILE_DEL        1102
#define IDC_BTN_FILE_UP         1103
#define IDC_BTN_FILE_DOWN       1104
#define IDC_BTN_FILE_PLAY       1105

class MainWindow {
public:
    MainWindow() = default;
    ~MainWindow() {
        RemoveTrayIcon();
        if (m_hAccel) {
            DestroyAcceleratorTable(m_hAccel);
            m_hAccel = NULL;
        }
        if (m_hFontClock) DeleteObject(m_hFontClock);
        if (m_hFontNormal) DeleteObject(m_hFontNormal);
        if (m_hFontBold) DeleteObject(m_hFontBold);
        if (m_hFontSmall) DeleteObject(m_hFontSmall);
        if (m_hFontStatus) DeleteObject(m_hFontStatus);
        if (m_hBrushStatusGreen) DeleteObject(m_hBrushStatusGreen);
        if (m_hBrushStatusRed) DeleteObject(m_hBrushStatusRed);
        if (m_hBrushStatusGray) DeleteObject(m_hBrushStatusGray);
        if (m_hIconSm) DestroyIcon(m_hIconSm);
    }

    HWND GetHwnd() const { return m_hwnd; }
    HACCEL GetAcceleratorTable() const { return m_hAccel; }

    bool Create(HINSTANCE hInstance, int nCmdShow) {
        m_hInstance = hInstance;

        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
        wc.lpfnWndProc = WndProcThunk;
        wc.hInstance = hInstance;
        wc.lpszClassName = APP_WINDOW_CLASS_W;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszMenuName = NULL;
        wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
        wc.hIconSm = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
        m_hIconSm = wc.hIconSm;
        m_uTaskbarRestartMsg = RegisterWindowMessageW(L"TaskbarCreated");
        m_uRestoreInstanceMsg = RegisterWindowMessageW(APP_RESTORE_MSG_W);

        if (!RegisterClassExW(&wc)) return false;

        CreateMainMenu();

        int winW = 1140;
        int winH = 720;
        int scrW = GetSystemMetrics(SM_CXSCREEN);
        int scrH = GetSystemMetrics(SM_CYSCREEN);
        int posX = (scrW - winW) / 2;
        int posY = (scrH - winH) / 2;

        m_hwnd = CreateWindowExW(
            WS_EX_ACCEPTFILES,
            wc.lpszClassName,
            APP_NAME_W,
            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            posX, posY, winW, winH,
            NULL, m_hMenu, hInstance, this
        );

        if (!m_hwnd) return false;

        DragAcceptFiles(m_hwnd, TRUE);

        // Allow WM_DROPFILES across privilege levels if elevated
        typedef BOOL (WINAPI *ChangeWindowMessageFilterProc)(UINT, DWORD);
        HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
        if (hUser32) {
            ChangeWindowMessageFilterProc pFilter = (ChangeWindowMessageFilterProc)GetProcAddress(hUser32, "ChangeWindowMessageFilter");
            if (pFilter) {
                pFilter(WM_DROPFILES, 1); // MSGFLT_ADD
                pFilter(0x0049, 1);       // WM_COPYGLOBALDATA
                pFilter(WM_COPYDATA, 1);
            }
        }

        SendMessage(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)wc.hIcon);
        SendMessage(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)wc.hIconSm);

        ShowWindow(m_hwnd, nCmdShow);
        UpdateWindow(m_hwnd);

        // Load active preset or initialize on startup
        LoadInitialPreset();

        // Apply loaded/default language to all UI controls
        UpdateLanguageUI();

        // Initialize System Tray Icon
        InitTrayIcon();

        // Create accelerator table for global keyboard shortcuts
        ACCEL accels[4];
        accels[0].fVirt = FVIRTKEY;
        accels[0].key = VK_F1;
        accels[0].cmd = IDM_HELP_MANUAL;

        accels[1].fVirt = FCONTROL | FVIRTKEY;
        accels[1].key = 'S';
        accels[1].cmd = IDM_FILE_SAVE;

        accels[2].fVirt = FCONTROL | FVIRTKEY;
        accels[2].key = 'N';
        accels[2].cmd = IDM_PRESET_NEW;

        accels[3].fVirt = FVIRTKEY;
        accels[3].key = VK_DELETE;
        accels[3].cmd = IDM_DELETE_ITEM;

        m_hAccel = CreateAcceleratorTableW(accels, 4);

        // Start 500ms interval timer
        SetTimer(m_hwnd, ID_TIMER_TICK, 500, NULL);
        OnTimerTick();

        return true;
    }

    bool IsGroupConflicted(int idx) const {
        if (idx < 0 || idx >= (int)m_items.size()) return false;
        const auto& a = m_items[idx];
        if (!a.enabled) return false;

        for (int i = 0; i < (int)m_items.size(); ++i) {
            if (i == idx) continue;
            const auto& b = m_items[i];
            if (!b.enabled) continue;
            if (a.hour == b.hour && a.minute == b.minute && a.second == b.second) {
                for (int d = 0; d < 7; ++d) {
                    if (a.IsDayActive(d) && b.IsDayActive(d)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    std::wstring CopyAudioFileToPreset(const std::wstring& srcPath) {
        if (srcPath.empty()) return srcPath;
        if (m_currentPresetPath.empty()) return srcPath;

        std::wstring presetFolder = PresetManager::GetDirectoryOf(m_currentPresetPath);
        if (presetFolder.empty()) return srcPath;

        std::wstring audioFolder = presetFolder + L"\\audio";
        CreateDirectoryW(audioFolder.c_str(), NULL);

        const wchar_t* fn = wcsrchr(srcPath.c_str(), L'\\');
        std::wstring fileName = fn ? (fn + 1) : srcPath;
        std::wstring destPath = audioFolder + L"\\" + fileName;

        if (_wcsicmp(srcPath.c_str(), destPath.c_str()) == 0) {
            return destPath;
        }

        if (CopyFileW(srcPath.c_str(), destPath.c_str(), FALSE)) {
            return destPath;
        }

        return srcPath;
    }

private:
    HINSTANCE m_hInstance = NULL;
    HWND m_hwnd = NULL;
    HACCEL m_hAccel = NULL;
    HMENU m_hMenu = NULL;

    // Top Header
    HWND m_hClockLabel = NULL;
    HWND m_hTimezoneLabel = NULL;
    HWND m_hNextLabel = NULL;
    HWND m_hWaveformPanel = NULL;
    HWND m_hComboLabel = NULL;
    HWND m_hComboPresets = NULL;       // Dropdown for presets in presets/ folder
    HWND m_hBtnManagePreset = NULL;    // Manage Preset dropdown button (New, Edit, Delete)
    HWND m_hPresetStatusLabel = NULL;   // Top right status indicator
    HBRUSH m_hBrushStatusGreen = NULL;
    HBRUSH m_hBrushStatusRed = NULL;
    HBRUSH m_hBrushStatusGray = NULL;

    // Left Panel (Schedule groups)
    HWND m_hLeftTitle = NULL;
    HWND m_hListView = NULL;
    HWND m_hBtnGrpAdd = NULL;
    HWND m_hBtnGrpEdit = NULL;
    HWND m_hBtnGrpDel = NULL;

    // Right Panel (Audio files in selected group)
    HWND m_hRightTitle = NULL;
    HWND m_hRightListView = NULL;
    HWND m_hBtnFileAdd = NULL;
    HWND m_hBtnFileDel = NULL;
    HWND m_hBtnFileUp = NULL;
    HWND m_hBtnFileDown = NULL;

    // Status Bar
    HWND m_hStatusBar = NULL;

    HFONT m_hFontClock = NULL;
    HFONT m_hFontNormal = NULL;
    HFONT m_hFontBold = NULL;
    HFONT m_hFontSmall = NULL;
    HFONT m_hFontStatus = NULL;

    std::vector<ScheduleGroup> m_items;
    int m_nextId = 1;
    std::wstring m_currentPresetPath;
    std::wstring m_timezoneStr;
    bool m_isDirty = false;
    std::wstring m_lastPlayingGroup;

    // Text caching to prevent UI flicker
    std::wstring m_lastClockText;
    std::wstring m_lastNextText;
    std::wstring m_lastPresetStatusText;
    std::wstring m_lastStatusPart0;
    std::wstring m_lastStatusPart1;

    // System Tray & Background execution
    HICON m_hIconSm = NULL;
    bool m_isMinimizedToTray = false;
    bool m_balloonShownOnce = false;
    UINT m_uTaskbarRestartMsg = 0;
    UINT m_uRestoreInstanceMsg = 0;
    std::wstring m_lastTrayTip;

    static LRESULT CALLBACK WndProcThunk(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        MainWindow* pThis = NULL;
        if (uMsg == WM_NCCREATE) {
            CREATESTRUCT* pCS = (CREATESTRUCT*)lParam;
            pThis = (MainWindow*)pCS->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
            pThis->m_hwnd = hwnd;
        } else {
            pThis = (MainWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }

        if (pThis) {
            return pThis->WndProc(hwnd, uMsg, wParam, lParam);
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    static LRESULT CALLBACK HeaderSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        if (uMsg == WM_ERASEBKGND) {
            return 1; // Suppress background erase on column header to prevent flicker
        }
        return DefSubclassProc(hwnd, uMsg, wParam, lParam);
    }

    static LRESULT CALLBACK WaveformPanelSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        MainWindow* pThis = (MainWindow*)dwRefData;
        if (!pThis) return DefSubclassProc(hwnd, uMsg, wParam, lParam);

        switch (uMsg) {
            case WM_ERASEBKGND:
                return 1;

            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                RECT rc;
                GetClientRect(hwnd, &rc);
                int w = rc.right - rc.left;
                int h = rc.bottom - rc.top;

                if (w > 0 && h > 0) {
                    HDC memDC = CreateCompatibleDC(hdc);
                    HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
                    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

                    pThis->DrawWaveformPanel(memDC, rc);

                    BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);
                    SelectObject(memDC, oldBmp);
                    DeleteObject(memBmp);
                    DeleteDC(memDC);
                }
                EndPaint(hwnd, &ps);
                return 0;
            }
        }
        return DefSubclassProc(hwnd, uMsg, wParam, lParam);
    }

    void DrawWaveformPanel(HDC hdc, const RECT& rc) {
        // Draw card background
        HBRUSH bgBrush = CreateSolidBrush(RGB(248, 250, 253));
        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(214, 224, 236));
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, bgBrush);
        HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 6, 6);
        SelectObject(hdc, oldBrush);
        DeleteObject(bgBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(borderPen);

        auto progress = AudioEngine::Instance().GetActiveProgress();
        int gIdx = GetSelectedGroupIndex();

        std::wstring textTop;
        double ratio = 0.0;
        COLORREF activeColor = RGB(0, 120, 215);

        if (progress.isPlaying) {
            ratio = progress.progressRatio;
            activeColor = progress.isPaused ? RGB(230, 130, 0) : RGB(0, 120, 215);

            std::wstring icon = progress.isPaused ? L"⏸ " : L"🔊 ";
            std::wstring trackInfo = L" [" + std::to_wstring(progress.currentTrackNum) + L"/" + std::to_wstring(progress.totalTracks) + L"] ";
            std::wstring timeStr = AudioEngine::FormatMs(progress.groupElapsedMs) + L" / " + AudioEngine::FormatMs(progress.groupTotalMs);
            textTop = icon + progress.groupName + trackInfo + timeStr;
        } else if (gIdx >= 0 && gIdx < (int)m_items.size()) {
            const auto& grp = m_items[gIdx];
            long long totalMs = 0;
            for (const auto& f : grp.audioFiles) {
                totalMs += AudioEngine::GetAudioDurationMs(f);
            }
            std::wstring grpTime = AudioEngine::FormatMs(totalMs);
            if (Lang::current == Language::Thai) {
                textTop = L"🎵 " + grp.name + L" (รวม " + std::to_wstring(grp.audioFiles.size()) + L" เพลง) • เวลาทั้งหมด: " + grpTime;
            } else {
                textTop = L"🎵 " + grp.name + L" (" + std::to_wstring(grp.audioFiles.size()) + L" tracks) • Total: " + grpTime;
            }
            ratio = 0.0;
            activeColor = RGB(160, 175, 195);
        } else {
            textTop = (Lang::current == Language::Thai)
                ? L"🎵 พร้อมเล่นเสียงตามเวลาที่กำหนด"
                : L"🎵 Ready for scheduled playback";
            ratio = 0.0;
            activeColor = RGB(190, 200, 212);
        }

        // Draw Text
        HFONT oldFont = (HFONT)SelectObject(hdc, m_hFontBold ? m_hFontBold : m_hFontNormal);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(35, 45, 60));

        RECT rcText = { rc.left + 8, rc.top + 2, rc.right - 8, rc.top + 20 };
        DrawTextW(hdc, textTop.c_str(), -1, &rcText, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        SelectObject(hdc, oldFont);

        // Draw Waveform Bars
        int waveLeft = rc.left + 8;
        int waveRight = rc.right - 8;
        int waveTop = rc.top + 21;
        int waveBottom = rc.bottom - 4;
        int waveH = waveBottom - waveTop;
        int waveW = waveRight - waveLeft;

        if (waveW > 10 && waveH > 4) {
            static const int pattern[] = {
                5, 9, 14, 11, 18, 24, 16, 22, 28, 19,
                12, 23, 29, 25, 17, 10, 20, 27, 30, 21,
                15, 19, 26, 20, 14, 23, 28, 16, 11, 22,
                26, 17, 13, 21, 15, 10, 6, 4, 9, 17,
                25, 18, 11, 6, 4
            };
            const int patLen = (int)(sizeof(pattern) / sizeof(pattern[0]));

            int barWidth = 3;
            int barGap = 2;
            int step = barWidth + barGap;
            int activePixelX = waveLeft + (int)(waveW * ratio);

            for (int x = waveLeft; x + barWidth <= waveRight; x += step) {
                int idx = ((x - waveLeft) / step) % patLen;
                int h = (pattern[idx] * waveH) / 30;
                if (h < 3) h = 3;
                if (h > waveH) h = waveH;

                int y = waveBottom - h;
                COLORREF barColor = (x <= activePixelX && ratio > 0.0) ? activeColor : RGB(214, 223, 233);

                HBRUSH bBrush = CreateSolidBrush(barColor);
                RECT rcBar = { x, y, x + barWidth, waveBottom };
                FillRect(hdc, &rcBar, bBrush);
                DeleteObject(bBrush);
            }
        }
    }

    static LRESULT CALLBACK LeftListViewSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        MainWindow* pThis = (MainWindow*)dwRefData;
        if (!pThis) return DefSubclassProc(hwnd, uMsg, wParam, lParam);

        switch (uMsg) {
            case WM_ERASEBKGND:
                return 1;

            case WM_DROPFILES: {
                HDROP hDrop = (HDROP)wParam;
                POINT pt;
                if (DragQueryPoint(hDrop, &pt)) {
                    LVHITTESTINFO lvhti{};
                    lvhti.pt = pt;
                    ListView_HitTest(hwnd, &lvhti);
                    if (lvhti.iItem >= 0) {
                        pThis->SetLeftListViewSelection(lvhti.iItem);
                        pThis->RefreshRightPanel(lvhti.iItem);
                    }
                }
                pThis->OnDropExternalFiles(hDrop);
                DragFinish(hDrop);
                return 0;
            }

            case WM_KEYDOWN: {
                if (wParam == VK_DELETE) {
                    pThis->OnDeleteGroup();
                    return 0;
                }
                if (wParam == VK_F1) {
                    ManualDialog::Show(pThis->m_hwnd);
                    return 0;
                }
                if (GetKeyState(VK_CONTROL) & 0x8000) {
                    if (wParam == 'S' || wParam == 's') {
                        pThis->OnSavePresetQuick();
                        return 0;
                    } else if (wParam == 'N' || wParam == 'n') {
                        pThis->OnNewPreset();
                        return 0;
                    }
                }
                break;
            }

            case WM_KEYUP:
                pThis->UpdateAllButtonsState();
                break;
        }
        return DefSubclassProc(hwnd, uMsg, wParam, lParam);
    }

    static LRESULT CALLBACK RightListViewSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        MainWindow* pThis = (MainWindow*)dwRefData;
        if (!pThis) return DefSubclassProc(hwnd, uMsg, wParam, lParam);

        switch (uMsg) {
            case WM_ERASEBKGND:
                return 1; // Double buffered, suppress background erase

            case WM_DROPFILES: {
                HDROP hDrop = (HDROP)wParam;
                pThis->OnDropExternalFiles(hDrop);
                DragFinish(hDrop);
                return 0;
            }

            case WM_KEYDOWN: {
                if (wParam == VK_DELETE) {
                    pThis->OnRemoveFileFromSelectedGroup();
                    return 0;
                }
                if (wParam == VK_F1) {
                    ManualDialog::Show(pThis->m_hwnd);
                    return 0;
                }
                if (GetKeyState(VK_CONTROL) & 0x8000) {
                    if (wParam == 'S' || wParam == 's') {
                        pThis->OnSavePresetQuick();
                        return 0;
                    } else if (wParam == 'N' || wParam == 'n') {
                        pThis->OnNewPreset();
                        return 0;
                    }
                }
                if (GetKeyState(VK_MENU) & 0x8000 || GetKeyState(VK_CONTROL) & 0x8000) {
                    if (wParam == VK_UP) {
                        pThis->MoveSelectedRightAudioItem(-1);
                        return 0;
                    } else if (wParam == VK_DOWN) {
                        pThis->MoveSelectedRightAudioItem(1);
                        return 0;
                    }
                }
                break;
            }

            case WM_KEYUP:
                pThis->UpdateRightButtonsState();
                break;
        }
        return DefSubclassProc(hwnd, uMsg, wParam, lParam);
    }

    void CreateMainMenu() {
        m_hMenu = CreateMenu();
        HMENU hFileMenu = CreatePopupMenu();
        AppendMenuW(hFileMenu, MF_STRING, IDM_PRESET_NEW, Lang::MenuPresetNew());
        AppendMenuW(hFileMenu, MF_STRING, IDM_PRESET_EDIT, Lang::MenuPresetEdit());
        AppendMenuW(hFileMenu, MF_STRING, IDM_PRESET_DEL, Lang::MenuPresetDelete());
        AppendMenuW(hFileMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_SAVE, Lang::MenuFileSave());
        AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_SAVEAS, Lang::MenuFileSaveAs());
        AppendMenuW(hFileMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_EXIT, Lang::MenuFileExit());
        AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hFileMenu, Lang::MenuFile());

        HMENU hBackupMenu = CreatePopupMenu();
        AppendMenuW(hBackupMenu, MF_STRING, IDM_BACKUP_EXPORT_ASCH, Lang::MenuExportPortable());
        AppendMenuW(hBackupMenu, MF_STRING, IDM_BACKUP_IMPORT, Lang::MenuImportPortable());
        AppendMenuW(hBackupMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hBackupMenu, MF_STRING, IDM_BACKUP_EXPORT_ZIP, Lang::MenuBackupAllZip());
        AppendMenuW(hBackupMenu, MF_STRING, IDM_BACKUP_RESTORE_ZIP, Lang::MenuRestoreAllZip());
        AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hBackupMenu, Lang::MenuBackup());

        HMENU hLangMenu = CreatePopupMenu();
        AppendMenuW(hLangMenu, MF_STRING | (Lang::current == Language::English ? MF_CHECKED : 0), IDM_LANG_ENGLISH, Lang::MenuLangEnglish());
        AppendMenuW(hLangMenu, MF_STRING | (Lang::current == Language::Thai ? MF_CHECKED : 0), IDM_LANG_THAI, Lang::MenuLangThai());
        AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hLangMenu, Lang::MenuLanguage());

        HMENU hHelpMenu = CreatePopupMenu();
        AppendMenuW(hHelpMenu, MF_STRING, IDM_HELP_MANUAL, Lang::MenuManual());
        AppendMenuW(hHelpMenu, MF_STRING, IDM_HELP_SETTINGS, Lang::MenuSettings());
        AppendMenuW(hHelpMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hHelpMenu, MF_STRING, IDM_HELP_ABOUT, Lang::MenuAbout());
        AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, Lang::MenuHelp());
    }

    std::wstring GetSystemTimezoneString() {
        // Dynamic lookup: GetDynamicTimeZoneInformation is Vista+, fallback to GetTimeZoneInformation on Windows XP
        typedef DWORD (WINAPI *PFN_GetDynamicTimeZoneInformation)(PDYNAMIC_TIME_ZONE_INFORMATION);
        HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");
        PFN_GetDynamicTimeZoneInformation pfnGetDynamic = hKernel ?
            (PFN_GetDynamicTimeZoneInformation)GetProcAddress(hKernel, "GetDynamicTimeZoneInformation") : NULL;

        std::wstring tzDisplay;
        if (pfnGetDynamic) {
            DYNAMIC_TIME_ZONE_INFORMATION dtzi{};
            pfnGetDynamic(&dtzi);

            std::wstring subKey = CRYPT_W(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\") + std::wstring(dtzi.TimeZoneKeyName);
            HKEY hKey;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                wchar_t display[256] = { 0 };
                DWORD sz = sizeof(display);
                if (RegQueryValueExW(hKey, L"Display", NULL, NULL, (LPBYTE)display, &sz) == ERROR_SUCCESS) {
                    tzDisplay = display;
                }
                RegCloseKey(hKey);
            }

            if (tzDisplay.empty()) {
                int offsetMin = -(dtzi.Bias + dtzi.StandardBias);
                int offsetH = offsetMin / 60;
                int offsetM = abs(offsetMin % 60);
                wchar_t buf[128];
                swprintf_s(buf, L"(UTC%+03d:%02d) %s", offsetH, offsetM, dtzi.StandardName);
                tzDisplay = buf;
            }
        } else {
            // Windows XP / Server 2003 fallback: use standard GetTimeZoneInformation
            TIME_ZONE_INFORMATION tzi{};
            GetTimeZoneInformation(&tzi);
            int offsetMin = -(tzi.Bias + tzi.StandardBias);
            int offsetH = offsetMin / 60;
            int offsetM = abs(offsetMin % 60);
            wchar_t buf[128];
            swprintf_s(buf, L"(UTC%+03d:%02d) %s", offsetH, offsetM, tzi.StandardName);
            tzDisplay = buf;
        }
        return tzDisplay;
    }

    LRESULT WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        if (uMsg == m_uTaskbarRestartMsg && m_uTaskbarRestartMsg != 0) {
            InitTrayIcon();
            return 0;
        }

        if (uMsg == m_uRestoreInstanceMsg && m_uRestoreInstanceMsg != 0) {
            RestoreFromTray();
            return 0;
        }

        switch (uMsg) {
            case WM_CREATE:
                OnCreate();
                return 0;

            case WM_ERASEBKGND:
                return 1; // Handled in WM_PAINT to prevent flicker

            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_BTNFACE + 1));
                EndPaint(hwnd, &ps);
                return 0;
            }

            case WM_CTLCOLORSTATIC: {
                HDC hdcStatic = (HDC)wParam;
                HWND hwndCtl = (HWND)lParam;
                if (hwndCtl == m_hPresetStatusLabel) {
                    SetBkMode(hdcStatic, TRANSPARENT);
                    if (m_currentPresetPath.empty() && m_items.empty()) {
                        SetTextColor(hdcStatic, RGB(110, 110, 110));
                        return (LRESULT)m_hBrushStatusGray;
                    } else if (m_isDirty || m_currentPresetPath.empty()) {
                        SetTextColor(hdcStatic, RGB(190, 20, 20)); // Red
                        return (LRESULT)m_hBrushStatusRed;
                    } else {
                        SetTextColor(hdcStatic, RGB(0, 130, 30));  // Green
                        return (LRESULT)m_hBrushStatusGreen;
                    }
                }
                SetBkMode(hdcStatic, TRANSPARENT);
                return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
            }

            case WM_SYSCOMMAND:
                if ((wParam & 0xFFF0) == SC_MINIMIZE) {
                    MinimizeToTray();
                    return 0;
                }
                break;

            case WM_SIZE:
                if (wParam == SIZE_MINIMIZED) {
                    MinimizeToTray();
                    return 0;
                }
                OnSize(LOWORD(lParam), HIWORD(lParam));
                return 0;

            case WM_TRAYICON: {
                switch (lParam) {
                    case WM_LBUTTONDBLCLK:
                        RestoreFromTray();
                        break;
                    case WM_LBUTTONUP:
                        if (m_isMinimizedToTray || !IsWindowVisible(hwnd) || IsIconic(hwnd)) {
                            RestoreFromTray();
                        } else {
                            SetForegroundWindow(hwnd);
                        }
                        break;
                    case WM_RBUTTONUP:
                    case WM_CONTEXTMENU:
                        ShowTrayContextMenu();
                        break;
                    case NIN_BALLOONUSERCLICK:
                        RestoreFromTray();
                        break;
                }
                return 0;
            }

            case WM_HELP:
                ManualDialog::Show(hwnd);
                return TRUE;

            case WM_KEYDOWN: {
                if (wParam == VK_F1) {
                    ManualDialog::Show(hwnd);
                    return 0;
                }
                if (wParam == VK_DELETE) {
                    SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDM_DELETE_ITEM, 0), 0);
                    return 0;
                }
                if (GetKeyState(VK_CONTROL) & 0x8000) {
                    if (wParam == 'S' || wParam == 's') {
                        OnSavePresetQuick();
                        return 0;
                    } else if (wParam == 'N' || wParam == 'n') {
                        OnNewPreset();
                        return 0;
                    }
                }
                break;
            }

            case WM_TIMER:
                if (wParam == ID_TIMER_TICK) {
                    OnTimerTick();
                }
                return 0;

            case WM_DROPFILES: {
                HDROP hDrop = (HDROP)wParam;
                OnDropExternalFiles(hDrop);
                DragFinish(hDrop);
                return 0;
            }

            case MM_MCINOTIFY:
                if (wParam == MCI_NOTIFY_SUCCESSFUL) {
                    AudioEngine::Instance().OnTrackFinished();
                    UpdateAllPlayIcons();
                }
                return 0;

            case WM_NOTIFY: {
                LPNMHDR pnm = (LPNMHDR)lParam;
                if (pnm->idFrom == IDC_LISTVIEW) {
                    if (pnm->code == LVN_ITEMCHANGED) {
                        NMLISTVIEW* pnmv = (NMLISTVIEW*)lParam;
                        if (pnmv->uNewState & LVIS_SELECTED) {
                            RefreshRightPanel(pnmv->iItem);
                        } else if ((pnmv->uOldState & LVIS_SELECTED) && !(pnmv->uNewState & LVIS_SELECTED)) {
                            int sel = GetSelectedGroupIndex();
                            if (sel < 0) {
                                RefreshRightPanel(-1);
                            }
                        }
                        UpdateAllButtonsState();
                        if (m_hWaveformPanel) InvalidateRect(m_hWaveformPanel, NULL, FALSE);
                    } else if (pnm->code == NM_CLICK) {
                        LVHITTESTINFO lvhti{};
                        DWORD pos = GetMessagePos();
                        POINT pt = { GET_X_LPARAM(pos), GET_Y_LPARAM(pos) };
                        ScreenToClient(m_hListView, &pt);
                        lvhti.pt = pt;
                        ListView_SubItemHitTest(m_hListView, &lvhti);
                        if (lvhti.iItem >= 0) {
                            if (lvhti.iSubItem == 1) { // Clicked on Play column!
                                PlayGroupByIndex(lvhti.iItem, true);
                                return 0;
                            } else if (lvhti.iSubItem == 2) { // Clicked on Status column!
                                SetLeftListViewSelection(lvhti.iItem);
                                m_items[lvhti.iItem].enabled = !m_items[lvhti.iItem].enabled;
                                m_isDirty = true;
                                UpdateGroupRowInListView(lvhti.iItem);
                                UpdatePresetStatusBanner();
                                RefreshRightPanel(lvhti.iItem);
                                UpdateAllButtonsState();
                                InvalidateRect(m_hListView, NULL, TRUE);
                                return 0;
                            } else if (lvhti.iSubItem == 6) { // Clicked on Manage column [▲] [▼]
                                RECT rcSubItem;
                                ListView_GetSubItemRect(m_hListView, lvhti.iItem, 6, LVIR_BOUNDS, &rcSubItem);
                                int clickX = pt.x - rcSubItem.left;
                                int midX = (rcSubItem.right - rcSubItem.left) / 2;
                                int cur = lvhti.iItem;
                                if (clickX < midX) {
                                    // Move Up
                                    if (cur > 0 && cur < (int)m_items.size()) {
                                        std::swap(m_items[cur], m_items[cur - 1]);
                                        m_isDirty = true;
                                        UpdatePresetStatusBanner();
                                        RefreshListView(cur - 1);
                                        SetFocus(m_hListView);
                                    }
                                } else {
                                    // Move Down
                                    if (cur >= 0 && cur < (int)m_items.size() - 1) {
                                        std::swap(m_items[cur], m_items[cur + 1]);
                                        m_isDirty = true;
                                        UpdatePresetStatusBanner();
                                        RefreshListView(cur + 1);
                                        SetFocus(m_hListView);
                                    }
                                }
                                return 0;
                            }
                        } else {
                            SetLeftListViewSelection(-1);
                            RefreshRightPanel(-1);
                            UpdateAllButtonsState();
                        }
                    } else if (pnm->code == NM_DBLCLK) {
                        OnEditGroup();
                    } else if (pnm->code == NM_CUSTOMDRAW) {
                        LPNMLVCUSTOMDRAW pLVCD = (LPNMLVCUSTOMDRAW)lParam;
                        if (pLVCD->nmcd.dwDrawStage == CDDS_PREPAINT) {
                            return CDRF_NOTIFYITEMDRAW;
                        }
                        if (pLVCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
                            int itemIdx = (int)pLVCD->nmcd.dwItemSpec;
                            bool isSel = (ListView_GetItemState(m_hListView, itemIdx, LVIS_SELECTED) & LVIS_SELECTED) != 0;
                            if (IsGroupConflicted(itemIdx) && !isSel) {
                                pLVCD->clrTextBk = RGB(255, 238, 220); // Warm soft orange background
                                pLVCD->clrText = RGB(190, 70, 0);       // Warm orange text
                            }
                            return CDRF_NOTIFYSUBITEMDRAW;
                        }
                        if (pLVCD->nmcd.dwDrawStage == (CDDS_SUBITEM | CDDS_ITEMPREPAINT)) {
                            int itemIdx = (int)pLVCD->nmcd.dwItemSpec;
                            int subItem = pLVCD->iSubItem;
                            bool isSel = (ListView_GetItemState(m_hListView, itemIdx, LVIS_SELECTED) & LVIS_SELECTED) != 0;

                            if (itemIdx >= 0 && itemIdx < (int)m_items.size()) {
                                bool conflicted = IsGroupConflicted(itemIdx);
                                if (conflicted && !isSel) {
                                    pLVCD->clrTextBk = RGB(255, 238, 220);
                                }

                                if (subItem == 0) { // Order #
                                    pLVCD->clrText = RGB(100, 100, 100);
                                } else if (subItem == 1) { // Play column
                                    bool isPlayingThis = AudioEngine::Instance().IsGroupPlaying(m_items[itemIdx].name);
                                    pLVCD->clrText = isPlayingThis ? RGB(230, 90, 0) : RGB(0, 102, 204);
                                } else if (subItem == 2) { // Status column
                                    if (m_items[itemIdx].enabled) {
                                        pLVCD->clrText = RGB(0, 140, 0);   // Green
                                    } else {
                                        pLVCD->clrText = RGB(200, 20, 20); // Red
                                    }
                                } else if (subItem == 3) { // Time column
                                    if (conflicted) {
                                        pLVCD->clrText = RGB(210, 70, 0);  // Orange
                                    }
                                } else if (subItem == 6) { // Manage column [▲] [▼]
                                    pLVCD->clrText = RGB(0, 102, 204);
                                }
                            }
                            return CDRF_DODEFAULT;
                        }
                    }
                } else if (pnm->idFrom == IDC_RIGHT_LISTVIEW) {
                    if (pnm->code == LVN_ITEMCHANGED) {
                        UpdateRightButtonsState();
                    } else if (pnm->code == NM_CLICK) {
                        LVHITTESTINFO lvhti{};
                        DWORD pos = GetMessagePos();
                        POINT pt = { GET_X_LPARAM(pos), GET_Y_LPARAM(pos) };
                        ScreenToClient(m_hRightListView, &pt);
                        lvhti.pt = pt;
                        ListView_SubItemHitTest(m_hRightListView, &lvhti);
                        if (lvhti.iItem >= 0) {
                            SetRightListViewSelection(lvhti.iItem);
                            SetFocus(m_hRightListView);
                            UpdateRightButtonsState();
                            if (lvhti.iSubItem == 1) { // Controls column: ▶ ⏸ ⏹
                                RECT rcSubItem;
                                ListView_GetSubItemRect(m_hRightListView, lvhti.iItem, 1, LVIR_BOUNDS, &rcSubItem);
                                int clickX = pt.x - rcSubItem.left;
                                int cellW = rcSubItem.right - rcSubItem.left;
                                if (clickX < cellW / 3) {
                                    PlaySingleFileByIndex(lvhti.iItem);
                                } else if (clickX < (cellW * 2) / 3) {
                                    PauseOrResumeTrack(lvhti.iItem);
                                } else {
                                    StopTrack();
                                }
                                return 0;
                            } else if (lvhti.iSubItem == 4) { // Manage column: [✏️] [▲] [▼]
                                RECT rcSubItem;
                                ListView_GetSubItemRect(m_hRightListView, lvhti.iItem, 4, LVIR_BOUNDS, &rcSubItem);
                                int clickX = pt.x - rcSubItem.left;
                                int cellW = rcSubItem.right - rcSubItem.left;
                                int cur = lvhti.iItem;
                                int gIdx = GetSelectedGroupIndex();
                                if (gIdx >= 0 && gIdx < (int)m_items.size()) {
                                    auto& files = m_items[gIdx].audioFiles;
                                    if (clickX < cellW / 3) {
                                        // [✏️] Rename
                                        OnRenameAudioFile(cur);
                                    } else if (clickX < (cellW * 2) / 3) {
                                        // [▲] Move Up
                                        if (cur > 0 && cur < (int)files.size()) {
                                            std::swap(files[cur], files[cur - 1]);
                                            m_isDirty = true;
                                            UpdatePresetStatusBanner();
                                            RefreshRightPanel(gIdx, cur - 1);
                                            UpdateGroupRowInListView(gIdx);
                                            SetFocus(m_hRightListView);
                                        }
                                    } else {
                                        // [▼] Move Down
                                        if (cur >= 0 && cur < (int)files.size() - 1) {
                                            std::swap(files[cur], files[cur + 1]);
                                            m_isDirty = true;
                                            UpdatePresetStatusBanner();
                                            RefreshRightPanel(gIdx, cur + 1);
                                            UpdateGroupRowInListView(gIdx);
                                            SetFocus(m_hRightListView);
                                        }
                                    }
                                }
                                return 0;
                            }
                        } else {
                            SetRightListViewSelection(-1);
                            UpdateRightButtonsState();
                        }
                    } else if (pnm->code == NM_DBLCLK) {
                        int fileIdx = GetSelectedRightFileIndex();
                        if (fileIdx >= 0) {
                            PlaySingleFileByIndex(fileIdx);
                        }
                        return 0;
                    } else if (pnm->code == NM_CUSTOMDRAW) {
                        LPNMLVCUSTOMDRAW pLVCD = (LPNMLVCUSTOMDRAW)lParam;
                        if (pLVCD->nmcd.dwDrawStage == CDDS_PREPAINT) return CDRF_NOTIFYITEMDRAW;
                        if (pLVCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) return CDRF_NOTIFYSUBITEMDRAW;
                        if (pLVCD->nmcd.dwDrawStage == (CDDS_SUBITEM | CDDS_ITEMPREPAINT)) {
                            int itemIdx = (int)pLVCD->nmcd.dwItemSpec;
                            int subItem = pLVCD->iSubItem;
                            int gIdx = GetSelectedGroupIndex();
                            if (gIdx >= 0 && gIdx < (int)m_items.size()) {
                                const auto& files = m_items[gIdx].audioFiles;
                                if (itemIdx >= 0 && itemIdx < (int)files.size()) {
                                    const std::wstring& path = files[itemIdx];
                                    bool isCurrent = AudioEngine::Instance().IsTrackPlaying(path);
                                    bool isPaused = AudioEngine::Instance().IsTrackPaused(path);

                                    if (subItem == 0) { // Order #
                                        pLVCD->clrText = RGB(100, 100, 100);
                                    } else if (subItem == 1) { // Controls
                                        if (isCurrent && !isPaused) {
                                            pLVCD->clrText = RGB(0, 140, 40); // Green
                                        } else if (isPaused) {
                                            pLVCD->clrText = RGB(220, 100, 0); // Orange
                                        } else {
                                            pLVCD->clrText = RGB(0, 102, 204); // Blue
                                        }
                                    } else if (subItem == 2) { // Duration
                                        pLVCD->clrText = RGB(90, 90, 90);
                                    } else if (subItem == 4) { // Manage column [✏️] [▲] [▼]
                                        pLVCD->clrText = RGB(0, 102, 204);
                                    }
                                }
                            }
                            return CDRF_DODEFAULT;
                        }
                    }
                }
                return 0;
            }

            case WM_COMMAND: {
                int id = LOWORD(wParam);
                int code = HIWORD(wParam);

                if (id == IDC_COMBO_PRESETS && code == CBN_SELCHANGE) {
                    int sel = (int)SendMessage(m_hComboPresets, CB_GETCURSEL, 0, 0);
                    if (sel != CB_ERR) {
                        wchar_t buf[256] = { 0 };
                        SendMessage(m_hComboPresets, CB_GETLBTEXT, sel, (LPARAM)buf);
                        LoadPresetByName(buf);
                    }
                    return 0;
                }

                switch (id) {
                    // Left Panel Actions (Groups)
                    case IDC_BTN_GRP_ADD: OnAddGroup(); break;
                    case IDC_BTN_GRP_EDIT: OnEditGroup(); break;
                    case IDC_BTN_GRP_DEL: OnDeleteGroup(); break;
                    case IDC_BTN_GRP_TOGGLE: OnToggleActive(); break;
                    case IDM_DELETE_ITEM: {
                        HWND hFocus = GetFocus();
                        if (hFocus == m_hRightListView || hFocus == m_hBtnFileDel) {
                            OnRemoveFileFromSelectedGroup();
                        } else if (hFocus == m_hListView || hFocus == m_hBtnGrpDel) {
                            OnDeleteGroup();
                        } else {
                            int rSel = GetSelectedRightFileIndex();
                            int lSel = GetSelectedGroupIndex();
                            if (rSel >= 0) {
                                OnRemoveFileFromSelectedGroup();
                            } else if (lSel >= 0) {
                                OnDeleteGroup();
                            }
                        }
                        break;
                    }

                    // File / Preset Actions
                    case IDC_BTN_MANAGE_PRESET: OnManagePresetDropdown(); break;
                    case IDM_PRESET_NEW:
                    case IDC_BTN_NEW_PRESET:
                    case IDM_FILE_NEW: OnNewPreset(); break;
                    case IDM_PRESET_EDIT: OnRenameCurrentPreset(); break;
                    case IDM_FILE_SAVE: OnSavePresetQuick(); break;
                    case IDM_FILE_SAVEAS: OnSavePresetDialog(); break;
                    case IDM_PRESET_DEL:
                    case IDC_BTN_DEL_PRESET:
                    case IDM_FILE_DELETE_PRESET: OnDeletePreset(); break;

                    // Backup & Portable Preset Actions
                    case IDM_BACKUP_EXPORT_ASCH: OnExportAschPackage(); break;
                    case IDM_BACKUP_IMPORT: OnImportPortablePreset(); break;
                    case IDM_BACKUP_EXPORT_ZIP: OnBackupAllToZip(); break;
                    case IDM_BACKUP_RESTORE_ZIP: OnRestoreAllFromZip(); break;
                    case IDM_BACKUP_OPEN_FOLDER: OnOpenPresetsFolder(); break;

                    // Right Panel Actions (Audio files)
                    case IDC_BTN_FILE_ADD: OnAddFilesToSelectedGroup(); break;
                    case IDC_BTN_FILE_DEL: OnRemoveFileFromSelectedGroup(); break;
                    case IDC_BTN_FILE_UP: MoveSelectedRightAudioItem(-1); break;
                    case IDC_BTN_FILE_DOWN: MoveSelectedRightAudioItem(1); break;

                    // Language & Settings Actions
                    case IDM_LANG_ENGLISH: {
                        Lang::current = Language::English;
                        AppSettings::Save();
                        UpdateLanguageUI();
                        break;
                    }
                    case IDM_LANG_THAI: {
                        Lang::current = Language::Thai;
                        AppSettings::Save();
                        UpdateLanguageUI();
                        break;
                    }
                    case IDM_HELP_SETTINGS: {
                        if (SettingsDialog::Show(hwnd)) {
                            UpdateLanguageUI();
                        }
                        break;
                    }

                    case IDM_TRAY_OPEN:
                        RestoreFromTray();
                        break;
                    case IDM_TRAY_STOP:
                        AudioEngine::Instance().Stop();
                        UpdateAllPlayIcons();
                        UpdateTrayTooltip();
                        break;
                    case IDM_TRAY_EXIT:
                        SendMessage(hwnd, WM_CLOSE, 0, 0);
                        break;

                    case IDM_FILE_EXIT:
                        SendMessage(hwnd, WM_CLOSE, 0, 0);
                        break;
                    case IDM_HELP_MANUAL:
                        ManualDialog::Show(hwnd);
                        break;
                    case IDM_HELP_ABOUT:
                        AboutDialog::Show(hwnd);
                        break;
                }
                return 0;
            }

            case WM_CLOSE:
                if (m_isMinimizedToTray) {
                    RestoreFromTray();
                }
                if (ConfirmExitWithSave()) {
                    DestroyWindow(hwnd);
                }
                return 0;

            case WM_DESTROY:
                RemoveTrayIcon();
                KillTimer(hwnd, ID_TIMER_TICK);
                AudioEngine::Instance().Stop();
                PostQuitMessage(0);
                return 0;
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    void OnCreate() {
        m_hFontClock = CreateFontW(32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        m_hFontNormal = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        m_hFontBold = CreateFontW(15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        m_hFontSmall = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        m_hFontStatus = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        m_timezoneStr = GetSystemTimezoneString();

        // Header: Digital Clock
        m_hClockLabel = CreateWindowW(L"STATIC", L"00:00:00", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                      20, 10, 150, 36, m_hwnd, NULL, m_hInstance, NULL);
        SendMessage(m_hClockLabel, WM_SETFONT, (WPARAM)m_hFontClock, TRUE);

        // Header: Timezone Label
        std::wstring tzText = L"🌏 " + m_timezoneStr;
        m_hTimezoneLabel = CreateWindowW(L"STATIC", tzText.c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT,
                                         20, 48, 250, 20, m_hwnd, NULL, m_hInstance, NULL);
        SendMessage(m_hTimezoneLabel, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);

        // Header: Next Event Info Label (Center)
        m_hNextLabel = CreateWindowW(L"STATIC", L"กำลังตรวจสอบรายการถัดไป...", WS_CHILD | WS_VISIBLE,
                                     280, 16, 400, 36, m_hwnd, NULL, m_hInstance, NULL);
        SendMessage(m_hNextLabel, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

        // Header: Audio Waveform & Group Duration Panel (Below Next Event Label)
        m_hWaveformPanel = CreateWindowExW(
            0, L"STATIC", L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
            250, 48, 400, 44,
            m_hwnd, NULL, m_hInstance, NULL
        );
        SetWindowSubclass(m_hWaveformPanel, WaveformPanelSubclassProc, 21, (DWORD_PTR)this);

        // Header: Preset Selector Label & ComboBox & Delete Button (Center-Right)
        m_hComboLabel = CreateWindowW(L"STATIC", L"📁 ชุดการเล่น (Preset):", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                      690, 12, 230, 18, m_hwnd, NULL, m_hInstance, NULL);
        SendMessage(m_hComboLabel, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);

        m_hComboPresets = CreateWindowExW(
            0, L"COMBOBOX", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST | WS_TABSTOP,
            690, 32, 150, 200,
            m_hwnd, (HMENU)IDC_COMBO_PRESETS, m_hInstance, NULL
        );
        SendMessage(m_hComboPresets, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

        m_hBtnManagePreset = CreateWindowW(
            L"BUTTON", Lang::BtnManagePreset(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            0, 0, 0, 0, m_hwnd, (HMENU)IDC_BTN_MANAGE_PRESET, m_hInstance, NULL
        );
        SendMessage(m_hBtnManagePreset, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

        // Header: Preset Save Status Indicator (Top-Right)
        m_hBrushStatusGreen = CreateSolidBrush(RGB(225, 245, 230));
        m_hBrushStatusRed   = CreateSolidBrush(RGB(255, 230, 230));
        m_hBrushStatusGray  = CreateSolidBrush(RGB(242, 242, 242));

        m_hPresetStatusLabel = CreateWindowExW(
            WS_EX_STATICEDGE,
            L"STATIC", Lang::StatusSaved(),
            WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
            930, 10, 200, 48, m_hwnd, NULL, m_hInstance, NULL
        );
        SendMessage(m_hPresetStatusLabel, WM_SETFONT, (WPARAM)m_hFontStatus, TRUE);

        auto CreateBtn = [this](const wchar_t* text, int id) {
            HWND h = CreateWindowW(L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                  0, 0, 0, 0, m_hwnd, (HMENU)(UINT_PTR)id, m_hInstance, NULL);
            SendMessage(h, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);
            return h;
        };

        // ================= LEFT PANEL (Schedule Groups) =================
        m_hLeftTitle = CreateWindowW(L"STATIC", Lang::GroupsTitle(),
                                     WS_CHILD | WS_VISIBLE, 20, 78, 560, 22, m_hwnd, NULL, m_hInstance, NULL);
        SendMessage(m_hLeftTitle, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

        m_hListView = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            WC_LISTVIEWW,
            L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
            20, 104, 560, 450,
            m_hwnd, (HMENU)IDC_LISTVIEW, m_hInstance, NULL
        );
        SendMessage(m_hListView, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);
        ListView_SetExtendedListViewStyle(m_hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

        HWND hHeader = ListView_GetHeader(m_hListView);
        if (hHeader) {
            SetWindowSubclass(hHeader, HeaderSubclassProc, 10, 0);
        }
        DragAcceptFiles(m_hListView, TRUE);
        SetWindowSubclass(m_hListView, LeftListViewSubclassProc, 13, (DWORD_PTR)this);

        // ListView Columns (0: Order, 1: Play, 2: Status, 3: Time, 4: Days, 5: Name, 6: Drag Handle ▦)
        LVCOLUMNW lvc = { sizeof(LVCOLUMNW) };
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;

        lvc.iSubItem = 0; lvc.pszText = (LPWSTR)Lang::ColOrder(); lvc.cx = 45; lvc.fmt = LVCFMT_CENTER;
        ListView_InsertColumn(m_hListView, 0, &lvc);

        lvc.iSubItem = 1; lvc.pszText = (LPWSTR)Lang::ColGroupPlay(); lvc.cx = 65; lvc.fmt = LVCFMT_CENTER;
        ListView_InsertColumn(m_hListView, 1, &lvc);

        lvc.iSubItem = 2; lvc.pszText = (LPWSTR)Lang::ColGroupStatus(); lvc.cx = 65; lvc.fmt = LVCFMT_CENTER;
        ListView_InsertColumn(m_hListView, 2, &lvc);

        lvc.iSubItem = 3; lvc.pszText = (LPWSTR)Lang::ColGroupStartTime(); lvc.cx = 85; lvc.fmt = LVCFMT_CENTER;
        ListView_InsertColumn(m_hListView, 3, &lvc);

        lvc.iSubItem = 4; lvc.pszText = (LPWSTR)Lang::ColGroupDays(); lvc.cx = 100; lvc.fmt = LVCFMT_LEFT;
        ListView_InsertColumn(m_hListView, 4, &lvc);

        lvc.iSubItem = 5; lvc.pszText = (LPWSTR)Lang::ColGroupName(); lvc.cx = 170; lvc.fmt = LVCFMT_LEFT;
        ListView_InsertColumn(m_hListView, 5, &lvc);

        lvc.iSubItem = 6; lvc.pszText = (LPWSTR)Lang::ColManage(); lvc.cx = 80; lvc.fmt = LVCFMT_CENTER;
        ListView_InsertColumn(m_hListView, 6, &lvc);

        // Left Panel Buttons (Groups) - Add, Edit, Del
        m_hBtnGrpAdd  = CreateBtn(Lang::BtnAddGroup(), IDC_BTN_GRP_ADD);
        m_hBtnGrpEdit = CreateBtn(Lang::BtnEditGroup(), IDC_BTN_GRP_EDIT);
        m_hBtnGrpDel  = CreateBtn(Lang::BtnDelGroup(), IDC_BTN_GRP_DEL);

        // ================= RIGHT PANEL (Audio Files in Selected Group) =================
        m_hRightTitle = CreateWindowW(L"STATIC", Lang::AudioTitle(),
                                     WS_CHILD | WS_VISIBLE, 600, 78, 500, 22, m_hwnd, NULL, m_hInstance, NULL);
        SendMessage(m_hRightTitle, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

        m_hRightListView = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            WC_LISTVIEWW,
            L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
            600, 104, 500, 450,
            m_hwnd, (HMENU)IDC_RIGHT_LISTVIEW, m_hInstance, NULL
        );
        SendMessage(m_hRightListView, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);
        ListView_SetExtendedListViewStyle(m_hRightListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
        DragAcceptFiles(m_hRightListView, TRUE);

        HWND hRightHeader = ListView_GetHeader(m_hRightListView);
        if (hRightHeader) {
            SetWindowSubclass(hRightHeader, HeaderSubclassProc, 11, 0);
        }
        SetWindowSubclass(m_hRightListView, RightListViewSubclassProc, 12, (DWORD_PTR)this);

        // Right ListView Columns (0: Order, 1: Controls, 2: Duration, 3: Filename, 4: Manage)
        LVCOLUMNW rlvc = { sizeof(LVCOLUMNW) };
        rlvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;

        rlvc.iSubItem = 0; rlvc.pszText = (LPWSTR)Lang::ColOrder(); rlvc.cx = 45; rlvc.fmt = LVCFMT_CENTER;
        ListView_InsertColumn(m_hRightListView, 0, &rlvc);

        rlvc.iSubItem = 1; rlvc.pszText = (LPWSTR)Lang::ColAudioControls(); rlvc.cx = 85; rlvc.fmt = LVCFMT_CENTER;
        ListView_InsertColumn(m_hRightListView, 1, &rlvc);

        rlvc.iSubItem = 2; rlvc.pszText = (LPWSTR)Lang::ColAudioDuration(); rlvc.cx = 70; rlvc.fmt = LVCFMT_CENTER;
        ListView_InsertColumn(m_hRightListView, 2, &rlvc);

        rlvc.iSubItem = 3; rlvc.pszText = (LPWSTR)Lang::ColAudioFilename(); rlvc.cx = 230; rlvc.fmt = LVCFMT_LEFT;
        ListView_InsertColumn(m_hRightListView, 3, &rlvc);

        rlvc.iSubItem = 4; rlvc.pszText = (LPWSTR)Lang::ColManage(); rlvc.cx = 110; rlvc.fmt = LVCFMT_CENTER;
        ListView_InsertColumn(m_hRightListView, 4, &rlvc);

        // Right Panel Buttons (Audio Files)
        m_hBtnFileAdd  = CreateBtn(Lang::BtnAddAudio(), IDC_BTN_FILE_ADD);
        m_hBtnFileDel  = CreateBtn(Lang::BtnDelAudio(), IDC_BTN_FILE_DEL);

        // Status Bar
        m_hStatusBar = CreateWindowExW(0, STATUSCLASSNAMEW, NULL, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                                       0, 0, 0, 0, m_hwnd, NULL, m_hInstance, NULL);
        SendMessage(m_hStatusBar, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);
        int parts[] = { 700, -1 };
        SendMessage(m_hStatusBar, SB_SETPARTS, 2, (LPARAM)parts);
        SendMessage(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)Lang::ReadyStatus());

        UpdatePresetStatusBanner();
        UpdateAllButtonsState();
    }

    void UpdateLanguageUI() {
        SetWindowTextW(m_hwnd, Lang::WindowTitle());

        if (m_hMenu) {
            DestroyMenu(m_hMenu);
            CreateMainMenu();
            SetMenu(m_hwnd, m_hMenu);
            DrawMenuBar(m_hwnd);
        }

        SetWindowTextW(m_hComboLabel, Lang::PresetLabel());
        SetWindowTextW(m_hBtnManagePreset, Lang::BtnManagePreset());

        SetWindowTextW(m_hLeftTitle, Lang::GroupsTitle());
        SetWindowTextW(m_hBtnGrpAdd, Lang::BtnAddGroup());
        SetWindowTextW(m_hBtnGrpEdit, Lang::BtnEditGroup());
        SetWindowTextW(m_hBtnGrpDel, Lang::BtnDelGroup());

        LVCOLUMNW lvc = { sizeof(LVCOLUMNW) };
        lvc.mask = LVCF_TEXT;
        lvc.pszText = (LPWSTR)Lang::ColOrder(); ListView_SetColumn(m_hListView, 0, &lvc);
        lvc.pszText = (LPWSTR)Lang::ColGroupPlay(); ListView_SetColumn(m_hListView, 1, &lvc);
        lvc.pszText = (LPWSTR)Lang::ColGroupStatus(); ListView_SetColumn(m_hListView, 2, &lvc);
        lvc.pszText = (LPWSTR)Lang::ColGroupStartTime(); ListView_SetColumn(m_hListView, 3, &lvc);
        lvc.pszText = (LPWSTR)Lang::ColGroupDays(); ListView_SetColumn(m_hListView, 4, &lvc);
        lvc.pszText = (LPWSTR)Lang::ColGroupName(); ListView_SetColumn(m_hListView, 5, &lvc);
        lvc.pszText = (LPWSTR)Lang::ColManage(); ListView_SetColumn(m_hListView, 6, &lvc);

        SetWindowTextW(m_hRightTitle, Lang::AudioTitle());
        SetWindowTextW(m_hBtnFileAdd, Lang::BtnAddAudio());
        SetWindowTextW(m_hBtnFileDel, Lang::BtnDelAudio());

        LVCOLUMNW rlvc = { sizeof(LVCOLUMNW) };
        rlvc.mask = LVCF_TEXT;
        rlvc.pszText = (LPWSTR)Lang::ColOrder(); ListView_SetColumn(m_hRightListView, 0, &rlvc);
        rlvc.pszText = (LPWSTR)Lang::ColAudioControls(); ListView_SetColumn(m_hRightListView, 1, &rlvc);
        rlvc.pszText = (LPWSTR)Lang::ColAudioDuration(); ListView_SetColumn(m_hRightListView, 2, &rlvc);
        rlvc.pszText = (LPWSTR)Lang::ColAudioFilename(); ListView_SetColumn(m_hRightListView, 3, &rlvc);
        rlvc.pszText = (LPWSTR)Lang::ColManage(); ListView_SetColumn(m_hRightListView, 4, &rlvc);

        UpdatePresetStatusBanner();
        UpdateNextEventBanner();
        m_lastTrayTip.clear();
        UpdateTrayTooltip();
        if (m_hWaveformPanel) InvalidateRect(m_hWaveformPanel, NULL, FALSE);
        for (size_t i = 0; i < m_items.size(); ++i) {
            UpdateGroupRowInListView((int)i);
        }
        RefreshRightPanel(GetSelectedGroupIndex());
        UpdateAllButtonsState();
        if (m_hWaveformPanel) InvalidateRect(m_hWaveformPanel, NULL, FALSE);
    }

    void OnSize(int width, int height) {
        if (!m_hListView || !m_hRightListView) return;
        SendMessage(m_hStatusBar, WM_SIZE, 0, 0);

        RECT rcStatus;
        GetWindowRect(m_hStatusBar, &rcStatus);
        int statusH = rcStatus.bottom - rcStatus.top;

        int margin = 18;
        int gap = 16;

        // Position Top-Right Preset Indicator
        int statusBadgeW = 180;
        int statusBadgeX = width - margin - statusBadgeW;
        SetWindowPos(m_hPresetStatusLabel, NULL, statusBadgeX, 10, statusBadgeW, 48, SWP_NOZORDER);

        // Position Preset ComboBox & Manage Preset Button
        int manageBtnW = 100;
        int manageBtnX = statusBadgeX - manageBtnW - 10;
        int comboW = 175;
        int comboX = manageBtnX - comboW - 8;
        SetWindowPos(m_hComboLabel, NULL, comboX, 12, comboW + manageBtnW + 8, 18, SWP_NOZORDER);
        SetWindowPos(m_hComboPresets, NULL, comboX, 32, comboW, 28, SWP_NOZORDER);
        SetWindowPos(m_hBtnManagePreset, NULL, manageBtnX, 32, manageBtnW, 28, SWP_NOZORDER);

        // Position Center Next Event Label and Waveform Panel
        int nextW = comboX - 250 - 15;
        if (nextW < 140) nextW = 140;
        SetWindowPos(m_hNextLabel, NULL, 250, 10, nextW, 36, SWP_NOZORDER);
        SetWindowPos(m_hWaveformPanel, NULL, 250, 48, nextW, 44, SWP_NOZORDER);

        // Calculate Split Panels for Schedule View
        int topY = 70;
        int listY = topY + 26;
        int btnAreaH = 36;
        int listH = height - listY - btnAreaH - statusH - 12;
        if (listH < 150) listH = 150;

        int totalW = width - (margin * 2) - gap;
        if (totalW < 400) totalW = 400;

        int leftW = (totalW * 55) / 100;
        if (leftW < 400) leftW = 400;
        int rightW = totalW - leftW;
        if (rightW < 340) rightW = 340;
        int rightX = margin + leftW + gap;

        // Position Left Panel (Groups)
        SetWindowPos(m_hLeftTitle, NULL, margin, topY, leftW, 22, SWP_NOZORDER);
        SetWindowPos(m_hListView, NULL, margin, listY, leftW, listH, SWP_NOZORDER);

        int btnY1 = listY + listH + 8;
        int lbW3 = (leftW - 12) / 3;
        SetWindowPos(m_hBtnGrpAdd, NULL, margin, btnY1, lbW3, 28, SWP_NOZORDER);
        SetWindowPos(m_hBtnGrpEdit, NULL, margin + lbW3 + 6, btnY1, lbW3, 28, SWP_NOZORDER);
        SetWindowPos(m_hBtnGrpDel, NULL, margin + (lbW3 * 2) + 12, btnY1, lbW3, 28, SWP_NOZORDER);

        // Position Right Panel (Audio Files)
        SetWindowPos(m_hRightTitle, NULL, rightX, topY, rightW, 22, SWP_NOZORDER);
        SetWindowPos(m_hRightListView, NULL, rightX, listY, rightW, listH, SWP_NOZORDER);

        int rbW2 = (rightW - 8) / 2;
        SetWindowPos(m_hBtnFileAdd, NULL, rightX, btnY1, rbW2, 28, SWP_NOZORDER);
        SetWindowPos(m_hBtnFileDel, NULL, rightX + rbW2 + 8, btnY1, rbW2, 28, SWP_NOZORDER);
    }

    void PopulatePresetsCombo(const std::wstring& selectName = L"") {
        if (!m_hComboPresets) return;
        SendMessage(m_hComboPresets, CB_RESETCONTENT, 0, 0);

        std::wstring presetsRoot = PresetManager::GetPresetsRootDirectory();
        WIN32_FIND_DATAW ffd;
        std::wstring searchPattern = presetsRoot + L"\\*";
        HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &ffd);

        std::vector<std::wstring> presetList;
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                    wcscmp(ffd.cFileName, L".") != 0 && wcscmp(ffd.cFileName, L"..") != 0) {
                    presetList.push_back(ffd.cFileName);
                }
            } while (FindNextFileW(hFind, &ffd));
            FindClose(hFind);
        }

        std::sort(presetList.begin(), presetList.end());

        int selectIndex = -1;
        std::wstring targetToSelect = selectName;
        if (targetToSelect.empty() && !m_currentPresetPath.empty()) {
            const wchar_t* p = wcsrchr(m_currentPresetPath.c_str(), L'\\');
            if (p) {
                std::wstring temp = m_currentPresetPath.substr(0, p - m_currentPresetPath.c_str());
                const wchar_t* p2 = wcsrchr(temp.c_str(), L'\\');
                if (p2) targetToSelect = (p2 + 1);
            }
        }

        for (size_t i = 0; i < presetList.size(); ++i) {
            SendMessage(m_hComboPresets, CB_ADDSTRING, 0, (LPARAM)presetList[i].c_str());
            if (_wcsicmp(presetList[i].c_str(), targetToSelect.c_str()) == 0) {
                selectIndex = (int)i;
            }
        }

        if (selectIndex >= 0) {
            SendMessage(m_hComboPresets, CB_SETCURSEL, selectIndex, 0);
        } else if (!presetList.empty()) {
            SendMessage(m_hComboPresets, CB_SETCURSEL, 0, 0);
        } else {
            SendMessage(m_hComboPresets, CB_SETCURSEL, -1, 0);
        }

        UpdateAllButtonsState();
    }

    void LoadPresetByName(const std::wstring& name) {
        if (name.empty()) return;
        std::wstring presetsRoot = PresetManager::GetPresetsRootDirectory();
        std::wstring folder = presetsRoot + L"\\" + name;
        std::wstring jsonPath = folder + L"\\preset.json";
        std::wstring legacyJson = folder + L"\\" + name + L".json";

        if (PathFileExistsW(jsonPath.c_str())) {
            if (_wcsicmp(legacyJson.c_str(), jsonPath.c_str()) != 0 && PathFileExistsW(legacyJson.c_str())) {
                DeleteFileW(legacyJson.c_str());
            }
        } else if (PathFileExistsW(legacyJson.c_str())) {
            jsonPath = legacyJson;
        }

        std::vector<ScheduleGroup> loaded;
        if (PresetManager::LoadFromFile(jsonPath, loaded)) {
            m_items = loaded;
            m_currentPresetPath = jsonPath;
            m_isDirty = false;
            PresetManager::SetLastActivePreset(name);
            PopulatePresetsCombo(name);
            RefreshListView(0);
            UpdatePresetTitle();
            UpdatePresetStatusBanner();
            std::wstring st = L"📂 โหลดชุดการเล่น [" + name + L"] สำเร็จ (" + std::to_wstring(m_items.size()) + L" กลุ่ม)";
            SendMessage(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)st.c_str());
        } else {
            MessageBoxW(m_hwnd, L"ไม่สามารถเปิดไฟล์ชุดการเล่นนี้ได้", L"ผิดพลาด", MB_OK | MB_ICONERROR);
        }
    }

    void UpdatePresetStatusBanner() {
        std::wstring statusText;
        if (m_currentPresetPath.empty() && m_items.empty()) {
            statusText = (Lang::current == Language::Thai) ? L"⚪ สถานะ: ไม่มีชุดการเล่น" : L"⚪ Status: No Preset";
        } else if (m_isDirty || m_currentPresetPath.empty()) {
            statusText = Lang::StatusUnsaved();
        } else {
            statusText = Lang::StatusSaved();
        }

        if (m_lastPresetStatusText != statusText) {
            m_lastPresetStatusText = statusText;
            SetWindowTextW(m_hPresetStatusLabel, statusText.c_str());
        }
        if (m_hPresetStatusLabel) {
            InvalidateRect(m_hPresetStatusLabel, NULL, TRUE);
            UpdateWindow(m_hPresetStatusLabel);
        }
    }

    int GetSelectedGroupIndex() {
        if (!m_hListView) return -1;
        return ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
    }

    int GetSelectedRightFileIndex() {
        if (!m_hRightListView) return -1;
        return ListView_GetNextItem(m_hRightListView, -1, LVNI_SELECTED);
    }

    void SetLeftListViewSelection(int index) {
        ListView_SetItemState(m_hListView, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
        if (index >= 0 && index < ListView_GetItemCount(m_hListView)) {
            ListView_SetItemState(m_hListView, index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            ListView_EnsureVisible(m_hListView, index, FALSE);
        }
        UpdateAllButtonsState();
    }

    void SetRightListViewSelection(int index) {
        ListView_SetItemState(m_hRightListView, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
        if (index >= 0 && index < ListView_GetItemCount(m_hRightListView)) {
            ListView_SetItemState(m_hRightListView, index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            ListView_EnsureVisible(m_hRightListView, index, FALSE);
        }
        UpdateRightButtonsState();
    }

    void UpdateLeftButtonsState() {
        bool hasPreset = !m_currentPresetPath.empty();
        int gIdx = GetSelectedGroupIndex();
        bool hasSelectedGroup = (hasPreset && gIdx >= 0 && gIdx < (int)m_items.size());

        if (m_hBtnGrpAdd)  EnableWindow(m_hBtnGrpAdd, TRUE);
        if (m_hBtnGrpEdit) EnableWindow(m_hBtnGrpEdit, hasSelectedGroup);
        if (m_hBtnGrpDel)  EnableWindow(m_hBtnGrpDel, hasSelectedGroup);
    }

    void UpdateRightButtonsState() {
        bool hasPreset = !m_currentPresetPath.empty();
        int gIdx = GetSelectedGroupIndex();
        bool hasGroup = (hasPreset && gIdx >= 0 && gIdx < (int)m_items.size());
        int fIdx = GetSelectedRightFileIndex();
        bool hasFile = hasGroup && (fIdx >= 0 && fIdx < (int)m_items[gIdx].audioFiles.size());
        int fileCount = hasGroup ? (int)m_items[gIdx].audioFiles.size() : 0;

        if (m_hBtnFileAdd)  EnableWindow(m_hBtnFileAdd, hasGroup);
        if (m_hBtnFileDel)  EnableWindow(m_hBtnFileDel, hasFile);
        if (m_hBtnFileUp)   EnableWindow(m_hBtnFileUp, hasFile && (fIdx > 0));
        if (m_hBtnFileDown) EnableWindow(m_hBtnFileDown, hasFile && (fIdx < fileCount - 1));
    }

    void UpdateAllButtonsState() {
        UpdateLeftButtonsState();
        UpdateRightButtonsState();
    }

    void UpdateAllPlayIcons() {
        for (size_t i = 0; i < m_items.size(); ++i) {
            bool isPlayingThis = AudioEngine::Instance().IsGroupPlaying(m_items[i].name);
            std::wstring playStr = isPlayingThis ? Lang::BtnPlaying() : Lang::BtnPlay();
            ListView_SetItemText(m_hListView, (int)i, 1, (LPWSTR)playStr.c_str());
        }
        RefreshRightPanelRows();
    }

    void RefreshRightPanelRows() {
        int gIdx = GetSelectedGroupIndex();
        if (gIdx < 0 || gIdx >= (int)m_items.size()) return;
        const auto& files = m_items[gIdx].audioFiles;
        for (size_t i = 0; i < files.size(); ++i) {
            const std::wstring& filePath = files[i];
            bool isThisPlaying = AudioEngine::Instance().IsTrackPlaying(filePath);
            bool isThisPaused = AudioEngine::Instance().IsTrackPaused(filePath);

            std::wstring ctrlStr;
            if (isThisPlaying && !isThisPaused) {
                ctrlStr = L"🔊  ⏸  ⏹";
            } else if (isThisPaused) {
                ctrlStr = L"▶  ⏸  ⏹";
            } else {
                ctrlStr = L"▶  ⏸  ⏹";
            }
            ListView_SetItemText(m_hRightListView, (int)i, 1, (LPWSTR)ctrlStr.c_str());
        }
    }

    void RefreshRightPanel(int groupIndex = -1, int selectFileIndex = -1) {
        if (groupIndex == -1) {
            groupIndex = GetSelectedGroupIndex();
        }

        ListView_DeleteAllItems(m_hRightListView);

        if (groupIndex >= 0 && groupIndex < (int)m_items.size()) {
            const auto& grp = m_items[groupIndex];
            std::wstring title = (Lang::current == Language::Thai)
                ? (L"🎵 รายการเสียงในกลุ่ม: " + grp.name + L" (" + std::to_wstring(grp.audioFiles.size()) + L" ไฟล์):")
                : (L"🎵 Group Audio Tracks: " + grp.name + L" (" + std::to_wstring(grp.audioFiles.size()) + L" tracks):");
            SetWindowTextW(m_hRightTitle, title.c_str());

            for (size_t i = 0; i < grp.audioFiles.size(); ++i) {
                const std::wstring& filePath = grp.audioFiles[i];
                bool isThisPlaying = AudioEngine::Instance().IsTrackPlaying(filePath);
                bool isThisPaused = AudioEngine::Instance().IsTrackPaused(filePath);

                LVITEMW lvi{};
                lvi.mask = LVIF_TEXT;
                lvi.iItem = (int)i;
                lvi.iSubItem = 0;

                // Col 0: Order #
                std::wstring numStr = std::to_wstring(i + 1);
                lvi.pszText = (LPWSTR)numStr.c_str();
                ListView_InsertItem(m_hRightListView, &lvi);

                // Col 1: Audio Controls
                std::wstring ctrlStr;
                if (isThisPlaying && !isThisPaused) {
                    ctrlStr = L"🔊  ⏸  ⏹";
                } else if (isThisPaused) {
                    ctrlStr = L"▶  ⏸  ⏹";
                } else {
                    ctrlStr = L"▶  ⏸  ⏹";
                }
                ListView_SetItemText(m_hRightListView, (int)i, 1, (LPWSTR)ctrlStr.c_str());

                // Col 2: Duration
                std::wstring durStr = AudioEngine::GetAudioDuration(filePath);
                ListView_SetItemText(m_hRightListView, (int)i, 2, (LPWSTR)durStr.c_str());

                // Col 3: Audio Filename
                const wchar_t* fn = wcsrchr(filePath.c_str(), L'\\');
                std::wstring dispName = fn ? (fn + 1) : filePath;
                ListView_SetItemText(m_hRightListView, (int)i, 3, (LPWSTR)dispName.c_str());

                // Col 4: Manage
                ListView_SetItemText(m_hRightListView, (int)i, 4, (LPWSTR)L"[✏️]  [▲]  [▼]");
            }

            if (!grp.audioFiles.empty() && selectFileIndex >= 0 && selectFileIndex < (int)grp.audioFiles.size()) {
                SetRightListViewSelection(selectFileIndex);
            } else {
                SetRightListViewSelection(-1);
            }

            UpdateRightButtonsState();
        } else {
            SetWindowTextW(m_hRightTitle, (Lang::current == Language::Thai) ? L"🎵 รายการเสียงในกลุ่ม (กรุณาเลือกกลุ่มทางฝั่งซ้าย):" : L"🎵 Group Audio Tracks (Please select a group on the left):");
            SetRightListViewSelection(-1);
            UpdateRightButtonsState();
        }
        if (m_hWaveformPanel) InvalidateRect(m_hWaveformPanel, NULL, FALSE);
    }

    void RefreshListView(int selectIdx = 0) {
        ListView_DeleteAllItems(m_hListView);

        for (size_t i = 0; i < m_items.size(); ++i) {
            const auto& it = m_items[i];
            LVITEMW lvi{};
            lvi.mask = LVIF_TEXT;
            lvi.iItem = (int)i;
            lvi.iSubItem = 0;

            // Col 0: Order #
            std::wstring numStr = std::to_wstring(i + 1);
            lvi.pszText = (LPWSTR)numStr.c_str();
            ListView_InsertItem(m_hListView, &lvi);

            // Col 1: Play Button
            bool isPlayingThis = AudioEngine::Instance().IsGroupPlaying(it.name);
            std::wstring playStr = isPlayingThis ? Lang::BtnPlaying() : Lang::BtnPlay();
            ListView_SetItemText(m_hListView, (int)i, 1, (LPWSTR)playStr.c_str());

            // Col 2: Status
            std::wstring statusStr = it.enabled ? Lang::StatusActive() : Lang::StatusInactive();
            ListView_SetItemText(m_hListView, (int)i, 2, (LPWSTR)statusStr.c_str());

            // Col 3: Time (with warning if conflicted)
            bool conflicted = IsGroupConflicted((int)i);
            std::wstring timeStr = (conflicted ? L"⚠️ " : L"") + it.GetTimeString();
            ListView_SetItemText(m_hListView, (int)i, 3, (LPWSTR)timeStr.c_str());

            // Col 4: Active Days
            std::wstring daysStr = it.GetDaysSummary();
            ListView_SetItemText(m_hListView, (int)i, 4, (LPWSTR)daysStr.c_str());

            // Col 5: Group Name
            ListView_SetItemText(m_hListView, (int)i, 5, (LPWSTR)it.name.c_str());

            // Col 6: Manage
            ListView_SetItemText(m_hListView, (int)i, 6, (LPWSTR)L"[▲]  [▼]");
        }

        if (!m_items.empty()) {
            if (selectIdx < 0 || selectIdx >= (int)m_items.size()) selectIdx = 0;
            SetLeftListViewSelection(selectIdx);
            RefreshRightPanel(selectIdx);
        } else {
            SetLeftListViewSelection(-1);
            RefreshRightPanel(-1);
        }

        UpdateNextEventBanner();
        UpdateTrayTooltip();
        if (m_hWaveformPanel) InvalidateRect(m_hWaveformPanel, NULL, FALSE);
        UpdatePresetStatusBanner();
        UpdateAllButtonsState();
        if (m_hWaveformPanel) InvalidateRect(m_hWaveformPanel, NULL, FALSE);
    }

    void UpdateGroupRowInListView(int idx) {
        if (idx < 0 || idx >= (int)m_items.size()) return;
        const auto& it = m_items[idx];

        std::wstring numStr = std::to_wstring(idx + 1);
        ListView_SetItemText(m_hListView, idx, 0, (LPWSTR)numStr.c_str());

        bool isPlayingThis = AudioEngine::Instance().IsGroupPlaying(it.name);
        std::wstring playStr = isPlayingThis ? Lang::BtnPlaying() : Lang::BtnPlay();
        ListView_SetItemText(m_hListView, idx, 1, (LPWSTR)playStr.c_str());

        std::wstring statusStr = it.enabled ? Lang::StatusActive() : Lang::StatusInactive();
        ListView_SetItemText(m_hListView, idx, 2, (LPWSTR)statusStr.c_str());

        bool conflicted = IsGroupConflicted(idx);
        std::wstring timeStr = (conflicted ? L"⚠️ " : L"") + it.GetTimeString();
        ListView_SetItemText(m_hListView, idx, 3, (LPWSTR)timeStr.c_str());

        std::wstring daysStr = it.GetDaysSummary();
        ListView_SetItemText(m_hListView, idx, 4, (LPWSTR)daysStr.c_str());

        ListView_SetItemText(m_hListView, idx, 5, (LPWSTR)it.name.c_str());

        ListView_SetItemText(m_hListView, idx, 6, (LPWSTR)L"[▲]  [▼]");

        UpdateNextEventBanner();
        UpdateTrayTooltip();
        if (m_hWaveformPanel) InvalidateRect(m_hWaveformPanel, NULL, FALSE);
        InvalidateRect(m_hListView, NULL, TRUE);
        if (m_hWaveformPanel) InvalidateRect(m_hWaveformPanel, NULL, FALSE);
    }

    void PlayGroupByIndex(int idx, bool isManual = true) {
        if (idx < 0 || idx >= (int)m_items.size()) return;

        // If manual click and this group is currently playing, clicking again stops it
        if (isManual && AudioEngine::Instance().IsGroupPlaying(m_items[idx].name)) {
            AudioEngine::Instance().StopGroup(m_items[idx].name);
            UpdateAllPlayIcons();
            SendMessage(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)L"⏹️ หยุดเสียงแล้ว");
            return;
        }

        if (m_items[idx].audioFiles.empty()) {
            if (isManual) {
                MessageBoxW(m_hwnd, L"กลุ่มนี้ยังไม่มีไฟล์เสียง กรุณากดปุ่มเพิ่มไฟล์ในฝั่งขวา", L"ข้อผิดพลาด", MB_OK | MB_ICONWARNING);
            }
            return;
        }

        SetLeftListViewSelection(idx);
        RefreshRightPanel(idx);

        if (AudioEngine::Instance().PlayGroup(m_items[idx].name, m_items[idx].audioFiles, m_hwnd, isManual)) {
            UpdateAllPlayIcons();
            std::wstring st = L"🔊 กำลังเล่นกลุ่ม: " + m_items[idx].name;
            SendMessage(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)st.c_str());
        }
    }

    // ================= Drag & Drop External Files =================
    void OnDropExternalFiles(HDROP hDrop) {
        UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
        if (count == 0) return;

        // If a single .asch portable package is dropped, offer to import it immediately
        if (count == 1) {
            wchar_t szFirst[MAX_PATH] = { 0 };
            if (DragQueryFileW(hDrop, 0, szFirst, MAX_PATH)) {
                std::wstring firstPath = szFirst;
                size_t dot = firstPath.rfind(L'.');
                if (dot != std::wstring::npos) {
                    std::wstring ext = firstPath.substr(dot);
                    for (auto& c : ext) c = towlower(c);
                    if (ext == L".asch") {
                        std::wstring q = L"คุณได้ลากไฟล์แพ็กเกจชุดการเล่น Portable (.asch) เข้ามา:\n\n" + firstPath + L"\n\nต้องการนำเข้า (Import) ชุดการเล่นนี้หรือไม่?";
                        if (MessageBoxW(m_hwnd, q.c_str(), L"นำเข้าชุดการเล่นแบบ Portable", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                            DoImportAschPackage(firstPath);
                            return;
                        }
                    }
                }
            }
        }

        if (m_items.empty()) {
            ScheduleGroup g;
            g.id = m_nextId++;
            SYSTEMTIME st;
            GetLocalTime(&st);
            g.hour = st.wHour;
            g.minute = (st.wMinute + 5) % 60;
            g.name = L"กลุ่มไฟล์ที่ลากเข้ามา";
            m_items.push_back(g);
            RefreshListView(0);
        }

        int gIdx = GetSelectedGroupIndex();
        if (gIdx < 0 || gIdx >= (int)m_items.size()) {
            gIdx = 0;
            SetLeftListViewSelection(0);
        }

        auto IsAudioExt = [](const std::wstring& p) -> bool {
            size_t dot = p.rfind(L'.');
            if (dot == std::wstring::npos) return false;
            std::wstring ext = p.substr(dot);
            for (auto& c : ext) c = towlower(c);
            return (ext == L".mp3" || ext == L".wav" || ext == L".wma" ||
                    ext == L".m4a" || ext == L".aac" || ext == L".flac" ||
                    ext == L".ogg" || ext == L".mid");
        };

        auto CollectAudioFiles = [&](auto& self, const std::wstring& path, std::vector<std::wstring>& out) -> void {
            if (PathIsDirectoryW(path.c_str())) {
                WIN32_FIND_DATAW fd;
                std::wstring searchPattern = path + L"\\*";
                HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &fd);
                if (hFind != INVALID_HANDLE_VALUE) {
                    do {
                        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
                        std::wstring sub = path + L"\\" + fd.cFileName;
                        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                            self(self, sub, out);
                        } else if (IsAudioExt(sub)) {
                            out.push_back(sub);
                        }
                    } while (FindNextFileW(hFind, &fd));
                    FindClose(hFind);
                }
            } else if (IsAudioExt(path)) {
                out.push_back(path);
            }
        };

        std::vector<std::wstring> collected;
        for (UINT i = 0; i < count; ++i) {
            wchar_t szFile[MAX_PATH] = { 0 };
            if (DragQueryFileW(hDrop, i, szFile, MAX_PATH)) {
                CollectAudioFiles(CollectAudioFiles, szFile, collected);
            }
        }

        if (!collected.empty()) {
            for (const auto& f : collected) {
                std::wstring copiedPath = CopyAudioFileToPreset(f);
                m_items[gIdx].audioFiles.push_back(copiedPath);
            }
            m_isDirty = true;
            RefreshRightPanel(gIdx, (int)m_items[gIdx].audioFiles.size() - 1);
            UpdateGroupRowInListView(gIdx);
            UpdatePresetStatusBanner();

            std::wstring stMsg = L"📥 ลากเพิ่มไฟล์เสียงสำเร็จ: " + std::to_wstring(collected.size()) + L" ไฟล์ลงในกลุ่ม [" + m_items[gIdx].name + L"]";
            SendMessage(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)stMsg.c_str());
        }
    }

    // ================= Left Panel Actions (Groups) =================
    void OnAddGroup() {
        if (m_currentPresetPath.empty()) {
            int ret = MessageBoxW(m_hwnd,
                (Lang::current == Language::Thai)
                    ? L"ยังไม่ได้เลือกหรือสร้างชุดการเล่น (Preset)\n\nคุณต้องการสร้างชุดการเล่นใหม่ตอนนี้หรือไม่?"
                    : L"No Preset selected or created.\n\nWould you like to create a new Preset now?",
                (Lang::current == Language::Thai) ? L"สร้างชุดการเล่นใหม่" : L"Create New Preset",
                MB_YESNO | MB_ICONQUESTION);
            if (ret == IDYES) {
                OnNewPreset();
            }
            return;
        }

        ScheduleGroup group;
        group.id = m_nextId++;
        SYSTEMTIME st;
        GetLocalTime(&st);
        group.hour = st.wHour;
        group.minute = st.wMinute;
        group.second = 0;

        if (GroupDialog::Show(m_hwnd, group, false)) {
            for (auto& file : group.audioFiles) {
                file = CopyAudioFileToPreset(file);
            }
            m_items.push_back(group);
            m_isDirty = true;
            RefreshListView((int)m_items.size() - 1);
        }
    }

    void OnEditGroup() {
        if (m_currentPresetPath.empty()) return;
        int idx = GetSelectedGroupIndex();
        if (idx < 0 || idx >= (int)m_items.size()) {
            MessageBoxW(m_hwnd,
                (Lang::current == Language::Thai)
                    ? L"กรุณาเลือกกลุ่มที่ต้องการแก้ไขทางฝั่งซ้ายก่อน"
                    : L"Please select a group on the left to edit.",
                (Lang::current == Language::Thai) ? L"แจ้งเตือน" : L"Notice",
                MB_OK | MB_ICONINFORMATION);
            return;
        }

        if (GroupDialog::Show(m_hwnd, m_items[idx], true)) {
            for (auto& file : m_items[idx].audioFiles) {
                file = CopyAudioFileToPreset(file);
            }
            m_isDirty = true;
            RefreshListView(idx);
        }
    }

    void OnDeleteGroup() {
        if (m_currentPresetPath.empty()) return;
        int idx = GetSelectedGroupIndex();
        if (idx < 0 || idx >= (int)m_items.size()) {
            MessageBoxW(m_hwnd,
                (Lang::current == Language::Thai)
                    ? L"กรุณาเลือกกลุ่มที่ต้องการลบก่อน"
                    : L"Please select a group to delete.",
                (Lang::current == Language::Thai) ? L"แจ้งเตือน" : L"Notice",
                MB_OK | MB_ICONINFORMATION);
            return;
        }

        std::wstring msg = (Lang::current == Language::Thai)
            ? (L"คุณต้องการลบกลุ่ม \"" + m_items[idx].name + L"\" หรือไม่?")
            : (L"Are you sure you want to delete group \"" + m_items[idx].name + L"\"?");
        std::wstring title = (Lang::current == Language::Thai) ? L"ยืนยันการลบ" : L"Confirm Delete";
        if (MessageBoxW(m_hwnd, msg.c_str(), title.c_str(), MB_YESNO | MB_ICONQUESTION) == IDYES) {
            m_items.erase(m_items.begin() + idx);
            m_isDirty = true;

            // Auto-persist deletion to preset file immediately
            if (!m_currentPresetPath.empty()) {
                std::wstring folder = PresetManager::GetDirectoryOf(m_currentPresetPath);
                if (PathFileExistsW(folder.c_str())) {
                    PresetManager::SaveToFile(m_currentPresetPath, m_items);
                    PresetManager::SaveToFile(folder + L"\\preset.json", m_items);
                    m_isDirty = false;
                }
            }

            int nextSel = (idx >= (int)m_items.size()) ? (int)m_items.size() - 1 : idx;
            RefreshListView(nextSel);
            UpdatePresetStatusBanner();
        }
    }

    void OnToggleActive() {
        int idx = GetSelectedGroupIndex();
        if (idx < 0 || idx >= (int)m_items.size()) return;
        m_items[idx].enabled = !m_items[idx].enabled;
        m_isDirty = true;
        UpdateGroupRowInListView(idx);
        UpdatePresetStatusBanner();
        UpdateAllButtonsState();
    }

    void OnSavePresetQuick() {
        // If no preset has been saved yet, prompt with Save As dialog
        if (m_currentPresetPath.empty()) {
            OnSavePresetDialog();
            return;
        }

        std::wstring currentFolder = PresetManager::GetDirectoryOf(m_currentPresetPath);
        std::wstring presetName = L"default";
        const wchar_t* p = wcsrchr(currentFolder.c_str(), L'\\');
        if (p && wcslen(p + 1) > 0) {
            presetName = (p + 1);
        }

        std::wstring outFolder;
        int copiedCount = 0;
        if (PresetManager::SavePresetPackage(presetName, m_items, true, outFolder, copiedCount)) {
            m_isDirty = false;
            PresetManager::SetLastActivePreset(presetName);
            UpdatePresetStatusBanner();
            RefreshRightPanel(GetSelectedGroupIndex());
            std::wstring st = L"💾 บันทึกชุดการเล่น [" + presetName + L"] สำเร็จเรียบร้อย";
            SendMessage(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)st.c_str());
        } else {
            MessageBoxW(m_hwnd, L"เกิดข้อผิดพลาดในการบันทึกชุดการเล่น", L"ผิดพลาด", MB_OK | MB_ICONERROR);
        }
    }

    void OnSavePresetDialog() {
        std::wstring defaultName = L"ชุดเสียงของฉัน";
        if (!m_currentPresetPath.empty()) {
            const wchar_t* p = wcsrchr(m_currentPresetPath.c_str(), L'\\');
            if (p) {
                std::wstring temp = m_currentPresetPath.substr(0, p - m_currentPresetPath.c_str());
                const wchar_t* p2 = wcsrchr(temp.c_str(), L'\\');
                if (p2 && wcslen(p2 + 1) > 0) defaultName = (p2 + 1);
            }
        }

        bool copyAudio = true;
        if (SavePresetDialog::Show(m_hwnd, defaultName, copyAudio)) {
            std::wstring outFolder;
            int copiedCount = 0;
            if (PresetManager::SavePresetPackage(defaultName, m_items, copyAudio, outFolder, copiedCount)) {
                m_currentPresetPath = outFolder + L"\\preset.json";
                m_isDirty = false;
                PresetManager::SetLastActivePreset(defaultName);
                UpdatePresetTitle();
                UpdatePresetStatusBanner();
                PopulatePresetsCombo(defaultName);
                RefreshListView(GetSelectedGroupIndex());

                std::wstring msg = L"บันทึกชุดการเล่นสำเร็จ!\n\n"
                                   L"📁 สร้างโฟลเดอร์: " + outFolder + L"\n"
                                   L"🎵 รวมไฟล์เสียง: " + std::to_wstring(copiedCount) + L" ไฟล์\n"
                                   L"📄 บันทึกข้อมูล: preset.json";
                MessageBoxW(m_hwnd, msg.c_str(), L"บันทึกเรียบร้อย", MB_OK | MB_ICONINFORMATION);
            } else {
                MessageBoxW(m_hwnd, L"เกิดข้อผิดพลาดในการบันทึกชุดการเล่น", L"ผิดพลาด", MB_OK | MB_ICONERROR);
            }
        }
    }

    void OnManagePresetDropdown() {
        if (!m_hBtnManagePreset) return;

        HMENU hMenu = CreatePopupMenu();
        if (!hMenu) return;

        bool hasPreset = !m_currentPresetPath.empty();

        AppendMenuW(hMenu, MF_STRING, IDM_PRESET_NEW, Lang::MenuPresetNew());
        AppendMenuW(hMenu, (hasPreset ? MF_STRING : (MF_STRING | MF_GRAYED)), IDM_PRESET_EDIT, Lang::MenuPresetEdit());
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hMenu, (hasPreset ? MF_STRING : (MF_STRING | MF_GRAYED)), IDM_PRESET_DEL, Lang::MenuPresetDelete());

        RECT rcBtn;
        GetWindowRect(m_hBtnManagePreset, &rcBtn);

        SetForegroundWindow(m_hwnd);
        TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN, rcBtn.left, rcBtn.bottom, 0, m_hwnd, NULL);
        PostMessage(m_hwnd, WM_NULL, 0, 0);
        DestroyMenu(hMenu);
    }

    void OnRenameCurrentPreset() {
        if (m_currentPresetPath.empty()) {
            MessageBoxW(m_hwnd,
                (Lang::current == Language::Thai ? L"กรุณาเลือกชุดการเล่นที่ต้องการเปลี่ยนชื่อก่อน" : L"Please select a preset to rename first."),
                (Lang::current == Language::Thai ? L"แจ้งเตือน" : L"Warning"),
                MB_OK | MB_ICONINFORMATION);
            return;
        }

        std::wstring curFolder = PresetManager::GetDirectoryOf(m_currentPresetPath);
        const wchar_t* p = wcsrchr(curFolder.c_str(), L'\\');
        std::wstring oldName = p ? (p + 1) : curFolder;
        if (oldName.empty()) return;

        std::wstring newName;
        if (RenamePresetDialog::Show(m_hwnd, oldName, newName)) {
            if (newName.empty() || newName == oldName) return;

            // Stop playback before renaming folder
            AudioEngine::Instance().Stop();

            std::wstring presetsRoot = PresetManager::GetPresetsRootDirectory();
            std::wstring oldDir = presetsRoot + L"\\" + oldName;
            std::wstring newDir = presetsRoot + L"\\" + newName;

            if (!MoveFileW(oldDir.c_str(), newDir.c_str())) {
                MessageBoxW(m_hwnd,
                    (Lang::current == Language::Thai
                        ? L"เกิดข้อผิดพลาด ไม่สามารถเปลี่ยนชื่อโฟลเดอร์พรีเซ็ตได้ (อาจมีโปรแกรมอื่นเปิดไฟล์อยู่)"
                        : L"Failed to rename preset directory. A file may be in use."),
                    (Lang::current == Language::Thai ? L"ข้อผิดพลาด" : L"Error"),
                    MB_OK | MB_ICONERROR);
                return;
            }

            // Update m_currentPresetPath to new directory
            m_currentPresetPath = newDir + L"\\preset.json";

            // If legacy <oldName>.json existed, rename it
            std::wstring oldJson = newDir + L"\\" + oldName + L".json";
            std::wstring newJson = newDir + L"\\" + newName + L".json";
            if (PathFileExistsW(oldJson.c_str())) {
                MoveFileW(oldJson.c_str(), newJson.c_str());
            }

            // Update in-memory audio files path prefix from oldDir to newDir
            for (auto& grp : m_items) {
                for (auto& file : grp.audioFiles) {
                    if (file.rfind(oldDir, 0) == 0) {
                        file = newDir + file.substr(oldDir.length());
                    }
                }
            }

            // Save updated preset package
            std::wstring outFolder;
            int copied = 0;
            PresetManager::SavePresetPackage(newName, m_items, false, outFolder, copied);

            // Update last active preset
            PresetManager::SetLastActivePreset(newName);

            // Refresh ComboBox & UI
            PopulatePresetsCombo(newName);
            UpdatePresetTitle();
            UpdatePresetStatusBanner();
            UpdateTrayTooltip();

            std::wstring notice = (Lang::current == Language::Thai)
                ? (L"✏️ เปลี่ยนชื่อชุดการเล่นเป็น [" + newName + L"] สำเร็จเรียบร้อย")
                : (L"✏️ Preset renamed to [" + newName + L"] successfully");
            SendMessage(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)notice.c_str());
        }
    }

    void OnNewPreset() {
        std::wstring presetName = L"ชุดเสียงใหม่";
        bool copyCurrent = false;

        if (NewPresetDialog::Show(m_hwnd, presetName, copyCurrent)) {
            AudioEngine::Instance().Stop();

            std::vector<ScheduleGroup> newItems;
            if (copyCurrent) {
                newItems = m_items;
            }

            std::wstring outFolder;
            int copied = 0;
            if (PresetManager::SavePresetPackage(presetName, newItems, copyCurrent, outFolder, copied)) {
                m_items = newItems;
                m_currentPresetPath = outFolder + L"\\preset.json";
                m_isDirty = false;
                PresetManager::SetLastActivePreset(presetName);
                PopulatePresetsCombo(presetName);
                RefreshListView(0);
                UpdatePresetTitle();
                UpdatePresetStatusBanner();

                std::wstring msg = L"✨ สร้างชุดการเล่น [" + presetName + L"] สำเร็จเรียบร้อย!\n\n"
                                   L"📁 โฟลเดอร์: " + outFolder + L"\n"
                                   L"คุณสามารถเริ่มกดปุ่ม '+ สร้างกลุ่มใหม่' หรือเพิ่มไฟล์เสียงได้ทันที";
                MessageBoxW(m_hwnd, msg.c_str(), L"สร้างชุดการเล่นสำเร็จ", MB_OK | MB_ICONINFORMATION);
            } else {
                MessageBoxW(m_hwnd, L"เกิดข้อผิดพลาดในการสร้างชุดการเล่นใหม่", L"ผิดพลาด", MB_OK | MB_ICONERROR);
            }
        }
    }

    // ================= Backup & Portable Preset System =================
    void OnExportAschPackage() {
        // Determine which preset to export
        std::wstring presetName;
        int sel = (int)SendMessage(m_hComboPresets, CB_GETCURSEL, 0, 0);
        if (sel != CB_ERR) {
            wchar_t buf[256] = { 0 };
            SendMessage(m_hComboPresets, CB_GETLBTEXT, sel, (LPARAM)buf);
            presetName = buf;
        }

        if (presetName.empty() && !m_currentPresetPath.empty()) {
            const wchar_t* p = wcsrchr(m_currentPresetPath.c_str(), L'\\');
            if (p) {
                std::wstring temp = m_currentPresetPath.substr(0, p - m_currentPresetPath.c_str());
                const wchar_t* p2 = wcsrchr(temp.c_str(), L'\\');
                if (p2) presetName = (p2 + 1);
            }
        }

        if (presetName.empty()) {
            MessageBoxW(m_hwnd, L"ไม่พบชุด Preset ที่ต้องการส่งออก กรุณาเลือกหรือสร้างชุดการเล่นก่อน", L"แจ้งเตือน", MB_OK | MB_ICONWARNING);
            return;
        }

        wchar_t szFile[MAX_PATH] = { 0 };
        std::wstring defaultFileName = presetName + L".asch";
        wcsncpy_s(szFile, defaultFileName.c_str(), MAX_PATH);

        OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
        ofn.hwndOwner = m_hwnd;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = L"แพ็กเกจชุดการเล่น Audio Schedule (*.asch)\0*.asch\0ไฟล์ทั้งหมด (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrDefExt = L"asch";
        ofn.lpstrTitle = L"บันทึกไฟล์แพ็กเกจชุดการเล่นแบบ Portable (.asch)";
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_EXPLORER;

        if (!GetSaveFileNameW(&ofn)) {
            return;
        }

        std::wstring destAschPath = szFile;

        // Auto-save current in-memory changes first so latest changes are packed
        if (!m_currentPresetPath.empty() && m_isDirty) {
            PresetManager::SaveToFile(m_currentPresetPath, m_items);
            std::wstring folder = PresetManager::GetDirectoryOf(m_currentPresetPath);
            PresetManager::SaveToFile(folder + L"\\preset.json", m_items);
            m_isDirty = false;
            UpdatePresetStatusBanner();
        }

        std::vector<ScheduleGroup> exportGroups;
        std::wstring curDir = !m_currentPresetPath.empty() ? PresetManager::GetDirectoryOf(m_currentPresetPath) : L"";
        const wchar_t* pCur = wcsrchr(curDir.c_str(), L'\\');
        std::wstring curPresetName = pCur ? (pCur + 1) : L"";

        if (!curPresetName.empty() && _wcsicmp(curPresetName.c_str(), presetName.c_str()) == 0) {
            exportGroups = m_items;
        } else {
            std::wstring pJson = PresetManager::GetPresetsRootDirectory() + L"\\" + presetName + L"\\" + presetName + L".json";
            if (!PathFileExistsW(pJson.c_str())) {
                pJson = PresetManager::GetPresetsRootDirectory() + L"\\" + presetName + L"\\preset.json";
            }
            if (!PresetManager::LoadFromFile(pJson, exportGroups)) {
                MessageBoxW(m_hwnd, L"ไม่สามารถอ่านข้อมูลชุดการเล่นที่ต้องการส่งออกได้", L"ผิดพลาด", MB_OK | MB_ICONERROR);
                return;
            }
        }

        int copiedAudio = 0;
        std::wstring errorMsg;
        if (PresetManager::ExportAschPackage(presetName, exportGroups, destAschPath, copiedAudio, errorMsg)) {
            std::wstring msg = L"🔒 แพ็กส่งออกชุดการเล่นแบบ Portable (.asch) สำเร็จเรียบร้อย!\n\n"
                               L"📦 ไฟล์แพ็กเกจ: " + destAschPath + L"\n"
                               L"⏰ กลุ่มตั้งเวลา: " + std::to_wstring(exportGroups.size()) + L" กลุ่ม\n"
                               L"🎵 ไฟล์เสียงที่ถูกเข้ารหัสบรรจุในไฟล์: " + std::to_wstring(copiedAudio) + L" ไฟล์\n\n"
                               L"🛡️ ไฟล์นี้ถูกรวมเป็นไฟล์เดียวและเข้ารหัส ไม่สามารถเปิดอ่านหรือเปิดฟังจากภายนอกได้โดยตรง\n"
                               L"จะต้องนำเข้า (Import) ผ่านโปรแกรม Audio Schedule เท่านั้น\n\n"
                               L"ต้องการเปิดโฟลเดอร์ที่เก็บไฟล์นี้เลยหรือไม่?";
            if (MessageBoxW(m_hwnd, msg.c_str(), L"ส่งออก Portable สำเร็จ", MB_YESNO | MB_ICONINFORMATION) == IDYES) {
                wchar_t dirOnly[MAX_PATH] = { 0 };
                wcscpy_s(dirOnly, destAschPath.c_str());
                PathRemoveFileSpecW(dirOnly);
                ShellExecuteW(m_hwnd, L"open", dirOnly, NULL, NULL, SW_SHOWNORMAL);
            }
            std::wstring st = L"🔒 แพ็กส่งออก Portable [.asch] สำเร็จ: " + presetName;
            SendMessage(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)st.c_str());
        } else {
            std::wstring err = L"เกิดข้อผิดพลาดในการแพ็กไฟล์:\n" + errorMsg;
            MessageBoxW(m_hwnd, err.c_str(), L"ผิดพลาด", MB_OK | MB_ICONERROR);
        }
    }

    bool DoImportAschPackage(const std::wstring& selectedPath) {
        // Check if selected file is .asch
        size_t dot = selectedPath.rfind(L'.');
        std::wstring ext = (dot != std::string::npos) ? selectedPath.substr(dot) : L"";
        for (auto& c : ext) c = towlower(c);

        if (ext != L".asch") {
            MessageBoxW(m_hwnd, L"ระบบรองรับเฉพาะไฟล์แพ็กเกจ .asch เท่านั้น กรุณาเลือกไฟล์ .asch ที่ถูกต้อง", L"รูปแบบไฟล์ไม่ถูกต้อง", MB_OK | MB_ICONWARNING);
            return false;
        }

        std::wstring presetName;
        if (!PresetManager::PeekAschPresetName(selectedPath, presetName)) {
            const wchar_t* pFn = wcsrchr(selectedPath.c_str(), L'\\');
            std::wstring fn = pFn ? (pFn + 1) : selectedPath;
            size_t d = fn.rfind(L'.');
            presetName = (d != std::string::npos) ? fn.substr(0, d) : fn;
        }

        std::wstring targetFolder = PresetManager::GetPresetsRootDirectory() + L"\\" + presetName;
        std::wstring targetNameOverride = presetName;
        if (PathFileExistsW(targetFolder.c_str())) {
            std::wstring prompt = L"ชุดการเล่นชื่อ [" + presetName + L"] มีอยู่ในโปรแกรมแล้ว\n\n"
                                  L"• กด 'Yes' เพื่อเขียนทับชุดเดิม\n"
                                  L"• กด 'No' เพื่อบันทึกเป็นชื่อใหม่ [" + presetName + L"_imported]\n"
                                  L"• กด 'Cancel' เพื่อยกเลิก";
            int ret = MessageBoxW(m_hwnd, prompt.c_str(), L"พบชุดการเล่นชื่อซ้ำ", MB_YESNOCANCEL | MB_ICONQUESTION);
            if (ret == IDCANCEL) {
                return false;
            } else if (ret == IDNO) {
                targetNameOverride = presetName + L"_imported";
                int c = 1;
                while (PathFileExistsW((PresetManager::GetPresetsRootDirectory() + L"\\" + targetNameOverride).c_str())) {
                    targetNameOverride = presetName + L"_imported_" + std::to_wstring(c++);
                }
            }
        }

        std::wstring finalPresetName;
        int groupsCount = 0;
        int audioCount = 0;
        std::wstring errorMsg;
        bool ok = PresetManager::ImportAschPackage(selectedPath, targetNameOverride, finalPresetName, groupsCount, audioCount, errorMsg);

        if (ok) {
            AudioEngine::Instance().Stop();
            PopulatePresetsCombo(finalPresetName);
            LoadPresetByName(finalPresetName);

            std::wstring msg = L"📥 นำเข้าชุดการเล่นแบบ Portable (.asch) สำเร็จเรียบร้อย!\n\n"
                               L"📦 ชื่อชุด: " + finalPresetName + L"\n"
                               L"⏰ กลุ่มตั้งเวลา: " + std::to_wstring(groupsCount) + L" กลุ่ม\n"
                               L"🎵 ไฟล์เสียงที่นำเข้าและถอดรหัส: " + std::to_wstring(audioCount) + L" ไฟล์\n\n"
                               L"ระบบได้โหลดชุดการเล่นนี้ขึ้นมาให้ใช้งานทันที";
            MessageBoxW(m_hwnd, msg.c_str(), L"นำเข้า Portable สำเร็จ", MB_OK | MB_ICONINFORMATION);
            std::wstring st = L"📥 นำเข้าชุดการเล่น [" + finalPresetName + L"] เรียบร้อยแล้ว";
            SendMessage(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)st.c_str());
            return true;
        } else {
            std::wstring err = L"เกิดข้อผิดพลาดในการนำเข้าชุดการเล่น:\n" + errorMsg;
            MessageBoxW(m_hwnd, err.c_str(), L"ผิดพลาด", MB_OK | MB_ICONERROR);
            return false;
        }
    }

    void OnImportPortablePreset() {
        wchar_t szFile[MAX_PATH] = { 0 };
        OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
        ofn.hwndOwner = m_hwnd;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = L"แพ็กเกจชุดการเล่น Audio Schedule (*.asch)\0*.asch\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrDefExt = L"asch";
        ofn.lpstrTitle = L"เลือกไฟล์แพ็กเกจชุดการเล่นแบบ Portable (.asch) ที่ต้องการนำเข้า";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;

        if (!GetOpenFileNameW(&ofn)) {
            return;
        }

        DoImportAschPackage(szFile);
    }

    void OnBackupAllToZip() {
        if (!m_currentPresetPath.empty() && m_isDirty) {
            PresetManager::SaveToFile(m_currentPresetPath, m_items);
            std::wstring folder = PresetManager::GetDirectoryOf(m_currentPresetPath);
            PresetManager::SaveToFile(folder + L"\\preset.json", m_items);
            m_isDirty = false;
            UpdatePresetStatusBanner();
        }

        SYSTEMTIME st;
        GetLocalTime(&st);
        wchar_t defaultZipName[128];
        swprintf_s(defaultZipName, APP_BACKUP_ZIP_PREFIX_W L"_%04d%02d%02d_%02d%02d%02d.zip",
                   st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

        wchar_t szFile[MAX_PATH] = { 0 };
        wcsncpy_s(szFile, defaultZipName, MAX_PATH);

        OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
        ofn.hwndOwner = m_hwnd;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = L"ไฟล์สำรอง ZIP (*.zip)\0*.zip\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrDefExt = L"zip";
        ofn.lpstrTitle = L"บันทึกไฟล์สำรองทุกชุดเป็นไฟล์ ZIP (.zip)";
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_EXPLORER;

        if (!GetSaveFileNameW(&ofn)) {
            return;
        }

        std::wstring targetZipPath = szFile;
        if (targetZipPath.length() < 4 || _wcsicmp(targetZipPath.substr(targetZipPath.length() - 4).c_str(), L".zip") != 0) {
            targetZipPath += L".zip";
        }

        std::wstring activePresetName;
        int sel = (int)SendMessage(m_hComboPresets, CB_GETCURSEL, 0, 0);
        if (sel != CB_ERR) {
            wchar_t buf[256] = { 0 };
            SendMessage(m_hComboPresets, CB_GETLBTEXT, sel, (LPARAM)buf);
            activePresetName = buf;
        }

        SendMessage(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)L"⏳ กำลังสำรองข้อมูลทุกชุดและสร้างไฟล์ ZIP...");

        int presetCount = 0;
        int totalAudioCount = 0;
        std::wstring errorMsg;

        if (PresetManager::BackupAllToZip(targetZipPath, activePresetName, m_items, presetCount, totalAudioCount, errorMsg)) {
            std::wstring msg = L"📦 สำรองข้อมูลทุกชุดเป็นไฟล์ ZIP สำเร็จเรียบร้อย!\n\n"
                               L"📁 ไฟล์สำรอง ZIP: " + targetZipPath + L"\n"
                               L"📊 รวมชุด Preset ทั้งหมด: " + std::to_wstring(presetCount) + L" ชุด\n"
                               L"🎵 ไฟล์เสียงที่ถูกแพ็กรวม: " + std::to_wstring(totalAudioCount) + L" ไฟล์\n\n"
                               L"🛡️ ทุกชุดถูกบันทึกเป็นไฟล์ Portable (.asch) แต่ละชุด และรวมไว้ในไฟล์ ZIP เดียว\n"
                               L"คุณสามารถนำไฟล์ ZIP นี้ไปกู้คืนบนเครื่องอื่นได้ทันที\n\n"
                               L"ต้องการเปิดโฟลเดอร์ที่เก็บไฟล์สำรองเลยหรือไม่?";
            if (MessageBoxW(m_hwnd, msg.c_str(), L"สำรองข้อมูลสำเร็จ", MB_YESNO | MB_ICONINFORMATION) == IDYES) {
                wchar_t dirOnly[MAX_PATH] = { 0 };
                wcscpy_s(dirOnly, targetZipPath.c_str());
                PathRemoveFileSpecW(dirOnly);
                ShellExecuteW(m_hwnd, L"open", dirOnly, NULL, NULL, SW_SHOWNORMAL);
            }
            std::wstring st = L"📦 สำรองข้อมูลทุกชุดเป็นไฟล์ ZIP (" + std::to_wstring(presetCount) + L" ชุด) เรียบร้อย";
            SendMessage(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)st.c_str());
        } else {
            std::wstring err = L"เกิดข้อผิดพลาดในการสำรองข้อมูล:\n" + errorMsg;
            MessageBoxW(m_hwnd, err.c_str(), L"ผิดพลาด", MB_OK | MB_ICONERROR);
            SendMessage(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)L"❌ เกิดข้อผิดพลาดในการสำรองข้อมูล");
        }
    }

    void OnRestoreAllFromZip() {
        wchar_t szFile[MAX_PATH] = { 0 };
        OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
        ofn.hwndOwner = m_hwnd;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = L"ไฟล์สำรอง ZIP (*.zip)\0*.zip\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrDefExt = L"zip";
        ofn.lpstrTitle = L"เลือกไฟล์สำรอง ZIP ที่ต้องการกู้คืนข้อมูล (.zip)";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;

        if (!GetOpenFileNameW(&ofn)) {
            return;
        }

        std::wstring srcZip = szFile;

        std::wstring confirm = L"คุณต้องการกู้คืนชุด Preset ทั้งหมดจากไฟล์ ZIP นี้ใช่หรือไม่?\n\n"
                               L"📁 ไฟล์สำรอง: " + srcZip + L"\n\n"
                               L"(หมายเหตุ: หากมีชุดชื่อซ้ำ ข้อมูลในไฟล์สำรองจะเขียนทับชุดเดิม)";
        if (MessageBoxW(m_hwnd, confirm.c_str(), L"ยืนยันการกู้คืนข้อมูล", MB_YESNO | MB_ICONQUESTION) != IDYES) {
            return;
        }

        AudioEngine::Instance().Stop();
        SendMessage(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)L"⏳ กำลังแตกไฟล์และกู้คืนชุด Preset ทั้งหมด...");

        int restoredCount = 0;
        int restoredAudioCount = 0;
        std::wstring errorMsg;

        if (PresetManager::RestoreAllFromZip(srcZip, restoredCount, restoredAudioCount, errorMsg)) {
            PopulatePresetsCombo();
            LoadInitialPreset();

            std::wstring msg = L"📂 กู้คืนชุดการเล่นทั้งหมดจากไฟล์ ZIP สำเร็จ!\n\n"
                               L"📊 จำนวนชุด Preset ที่กู้คืน: " + std::to_wstring(restoredCount) + L" ชุด\n"
                               L"🎵 ไฟล์เสียงที่ถอดรหัสนำเข้า: " + std::to_wstring(restoredAudioCount) + L" ไฟล์";
            MessageBoxW(m_hwnd, msg.c_str(), L"กู้คืนข้อมูลสำเร็จ", MB_OK | MB_ICONINFORMATION);
            SendMessage(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)L"📂 กู้คืนข้อมูลสำเร็จเรียบร้อย");
        } else {
            std::wstring err = L"เกิดข้อผิดพลาดในการกู้คืนข้อมูล:\n" + errorMsg;
            MessageBoxW(m_hwnd, err.c_str(), L"ผิดพลาด", MB_OK | MB_ICONERROR);
            SendMessage(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)L"❌ เกิดข้อผิดพลาดในการกู้คืนข้อมูล");
        }
    }

    void OnOpenPresetsFolder() {
        std::wstring folder = PresetManager::GetPresetsRootDirectory();
        ShellExecuteW(m_hwnd, L"open", folder.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }

    void UpdatePresetTitle() {
        SetWindowTextW(m_hwnd, APP_NAME_W);
    }

    bool ConfirmExitWithSave() {
        // If there are unsaved changes or an unsaved new preset with items
        bool isUnsaved = m_isDirty || (m_currentPresetPath.empty() && !m_items.empty());
        if (!isUnsaved) {
            return true; // Already saved or empty, exit directly
        }

        std::wstring msg = L"ชุดการเล่นมีการแก้ไขและยังไม่ได้บันทึก\n\n"
                           L"คุณต้องการบันทึกการเปลี่ยนแปลงก่อนออกจากโปรแกรมหรือไม่?\n\n"
                           L"• กด 'ใช่ (Yes)' เพื่อบันทึกข้อมูล\n"
                           L"• กด 'ไม่ใช่ (No)' เพื่อออกจากโปรแกรมโดยไม่บันทึก\n"
                           L"• กด 'ยกเลิก (Cancel)' เพื่อกลับสู่โปรแกรม";

        int ret = MessageBoxW(m_hwnd, msg.c_str(), L"บันทึกการเปลี่ยนแปลงก่อนปิดโปรแกรม", MB_YESNOCANCEL | MB_ICONQUESTION);
        if (ret == IDYES) {
            if (m_currentPresetPath.empty()) {
                OnSavePresetDialog();
                return !m_isDirty; // If saved, proceed with exit; if cancelled Save As dialog, stay in program
            } else {
                OnSavePresetQuick();
                return true;
            }
        } else if (ret == IDNO) {
            return true; // User chose not to save, proceed to exit
        } else {
            return false; // Cancel, stay in program
        }
    }

    void LoadInitialPreset() {
        std::wstring presetsRoot = PresetManager::GetPresetsRootDirectory();

        // 1. Clean up legacy auto-generated demo preset "default" if it was generated previously
        std::wstring demoFolder = presetsRoot + L"\\default";
        if (PathFileExistsW(demoFolder.c_str())) {
            std::vector<ScheduleGroup> checkGroups;
            std::wstring jsonPath = demoFolder + L"\\default.json";
            if (!PathFileExistsW(jsonPath.c_str())) {
                jsonPath = demoFolder + L"\\preset.json";
            }
            if (PresetManager::LoadFromFile(jsonPath, checkGroups)) {
                if (checkGroups.size() == 1 && checkGroups[0].name == L"กลุ่มตัวอย่างเคารพธงชาติ") {
                    PresetManager::DeletePresetFolder(demoFolder);
                    if (_wcsicmp(PresetManager::GetLastActivePreset().c_str(), L"default") == 0) {
                        PresetManager::SetLastActivePreset(L"");
                    }
                }
            }
        }

        std::wstring lastActive = PresetManager::GetLastActivePreset();

        // 2. If last active preset exists, load it
        if (!lastActive.empty() && PathFileExistsW((presetsRoot + L"\\" + lastActive).c_str())) {
            LoadPresetByName(lastActive);
            return;
        }

        // 3. Otherwise find the first available preset folder
        WIN32_FIND_DATAW ffd;
        HANDLE hFind = FindFirstFileW((presetsRoot + L"\\*").c_str(), &ffd);
        std::wstring firstPreset;
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                    wcscmp(ffd.cFileName, L".") != 0 && wcscmp(ffd.cFileName, L"..") != 0) {
                    firstPreset = ffd.cFileName;
                    break;
                }
            } while (FindNextFileW(hFind, &ffd));
            FindClose(hFind);
        }

        if (!firstPreset.empty()) {
            LoadPresetByName(firstPreset);
            return;
        }

        // 4. No presets exist: start completely clean and empty (no demo preset bundled)
        m_items.clear();
        m_currentPresetPath.clear();
        m_isDirty = false;
        PopulatePresetsCombo(L"");
        RefreshListView(-1);
        UpdatePresetTitle();
        UpdatePresetStatusBanner();
        UpdateAllButtonsState();
    }

    // ================= Right Panel Actions (Audio Files) =================
    void OnAddFilesToSelectedGroup() {
        int gIdx = GetSelectedGroupIndex();
        if (gIdx < 0 || gIdx >= (int)m_items.size()) {
            MessageBoxW(m_hwnd, L"กรุณาเลือกกลุ่มทางฝั่งซ้ายก่อนเพิ่มไฟล์เสียง", L"แจ้งเตือน", MB_OK | MB_ICONINFORMATION);
            return;
        }

        wchar_t szFiles[32768] = { 0 };
        OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
        ofn.hwndOwner = m_hwnd;
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
                std::wstring dest = CopyAudioFileToPreset(dir);
                m_items[gIdx].audioFiles.push_back(dest);
            } else {
                while (*p != L'\0') {
                    std::wstring fileName = p;
                    std::wstring fullPath = dir;
                    if (!fullPath.empty() && fullPath.back() != L'\\') fullPath += L'\\';
                    fullPath += fileName;
                    std::wstring dest = CopyAudioFileToPreset(fullPath);
                    m_items[gIdx].audioFiles.push_back(dest);
                    p += fileName.length() + 1;
                }
            }
            m_isDirty = true;
            UpdatePresetStatusBanner();
            RefreshRightPanel(gIdx);
            int newSel = (int)m_items[gIdx].audioFiles.size() - 1;
            if (newSel >= 0) {
                SetRightListViewSelection(newSel);
            }
            UpdateGroupRowInListView(gIdx);
        }
    }

    void OnRemoveFileFromSelectedGroup() {
        int gIdx = GetSelectedGroupIndex();
        if (gIdx < 0 || gIdx >= (int)m_items.size()) return;

        auto& files = m_items[gIdx].audioFiles;
        int cur = GetSelectedRightFileIndex();
        if (cur < 0 || cur >= (int)files.size()) {
            MessageBoxW(m_hwnd, L"กรุณาเลือกไฟล์เสียงที่ต้องการลบในฝั่งขวา", L"แจ้งเตือน", MB_OK | MB_ICONINFORMATION);
            return;
        }

        const std::wstring& targetPath = files[cur];
        const wchar_t* fn = wcsrchr(targetPath.c_str(), L'\\');
        std::wstring dispName = fn ? (fn + 1) : targetPath;

        std::wstring confirmMsg = L"คุณต้องการลบไฟล์เสียงนี้ออกจากกลุ่มหรือไม่?\n\n"
                                  L"🎵 ไฟล์: " + dispName + L"\n"
                                  L"📁 ที่อยู่: " + targetPath + L"\n\n"
                                  L"(หมายเหตุ: ไฟล์เสียงต้นฉบับจะยังคงอยู่ ไม่ถูกลบออกจากเครื่อง)";
        if (MessageBoxW(m_hwnd, confirmMsg.c_str(), L"ยืนยันการลบไฟล์เสียงออกจากกลุ่ม", MB_YESNO | MB_ICONQUESTION) != IDYES) {
            return;
        }

        files.erase(files.begin() + cur);
        m_isDirty = true;
        UpdatePresetStatusBanner();
        RefreshRightPanel(gIdx);
        if (cur >= (int)files.size()) cur = (int)files.size() - 1;
        if (cur >= 0) {
            SetRightListViewSelection(cur);
        }
        UpdateGroupRowInListView(gIdx);
    }

    void MoveSelectedRightAudioItem(int delta) {
        int gIdx = GetSelectedGroupIndex();
        if (gIdx < 0 || gIdx >= (int)m_items.size()) return;

        auto& files = m_items[gIdx].audioFiles;
        if (files.empty()) return;

        int cur = GetSelectedRightFileIndex();
        if (cur < 0 || cur >= (int)files.size()) {
            cur = 0;
            SetRightListViewSelection(0);
        }

        int target = cur + delta;
        if (target < 0 || target >= (int)files.size()) return;

        std::swap(files[cur], files[target]);
        m_isDirty = true;
        UpdatePresetStatusBanner();
        RefreshRightPanel(gIdx, target);
        UpdateGroupRowInListView(gIdx);
        SetFocus(m_hRightListView);
    }

    void OnRenameAudioFile(int trackIdx) {
        int gIdx = GetSelectedGroupIndex();
        if (gIdx < 0 || gIdx >= (int)m_items.size()) return;
        auto& files = m_items[gIdx].audioFiles;
        if (trackIdx < 0 || trackIdx >= (int)files.size()) return;

        std::wstring currentPath = files[trackIdx];
        const wchar_t* fn = wcsrchr(currentPath.c_str(), L'\\');
        std::wstring oldFileName = fn ? (fn + 1) : currentPath;

        std::wstring newFileName;
        if (RenameAudioDialog::Show(m_hwnd, oldFileName, newFileName)) {
            if (newFileName.empty() || _wcsicmp(newFileName.c_str(), oldFileName.c_str()) == 0) {
                return;
            }

            // Stop playback if playing to prevent file lock
            if (AudioEngine::Instance().IsTrackPlaying(currentPath)) {
                AudioEngine::Instance().Stop();
            }

            std::wstring dir;
            if (fn) {
                dir = currentPath.substr(0, fn - currentPath.c_str());
            }

            std::wstring newPath = dir.empty() ? newFileName : (dir + L"\\" + newFileName);

            // Check if destination file already exists
            DWORD attrs = GetFileAttributesW(newPath.c_str());
            if (attrs != INVALID_FILE_ATTRIBUTES) {
                std::wstring msg = (Lang::current == Language::Thai)
                    ? (L"ไฟล์ชื่อ \"" + newFileName + L"\" มีอยู่แล้ว กรุณาตั้งชื่ออื่น")
                    : (L"A file named \"" + newFileName + L"\" already exists. Please choose a different name.");
                MessageBoxW(m_hwnd, msg.c_str(), Lang::DialogTitleWarning(), MB_OK | MB_ICONWARNING);
                return;
            }

            // Rename file on disk if current exists
            DWORD srcAttrs = GetFileAttributesW(currentPath.c_str());
            if (srcAttrs != INVALID_FILE_ATTRIBUTES) {
                if (!MoveFileW(currentPath.c_str(), newPath.c_str())) {
                    std::wstring errMsg = (Lang::current == Language::Thai)
                        ? L"ไม่สามารถเปลี่ยนชื่อไฟล์ในดิสก์ได้ กรุณาตรวจสอบสิทธิ์การเขียนหรือปิดโปรแกรมอื่นที่เปิดไฟล์นี้อยู่"
                        : L"Failed to rename file on disk. Please check write permissions or close any program using it.";
                    MessageBoxW(m_hwnd, errMsg.c_str(), Lang::DialogTitleError(), MB_OK | MB_ICONERROR);
                    return;
                }
            }

            files[trackIdx] = newPath;
            m_isDirty = true;
            UpdatePresetStatusBanner();
            RefreshRightPanel(gIdx, trackIdx);
            UpdateGroupRowInListView(gIdx);
            if (m_hWaveformPanel) InvalidateRect(m_hWaveformPanel, NULL, FALSE);
        }
    }

    void OnPlayAllInGroup() {
        int gIdx = GetSelectedGroupIndex();
        if (gIdx < 0 || gIdx >= (int)m_items.size()) {
            MessageBoxW(m_hwnd, L"กรุณาเลือกกลุ่มทางฝั่งซ้ายที่ต้องการเล่นก่อน", L"แจ้งเตือน", MB_OK | MB_ICONINFORMATION);
            return;
        }
        PlayGroupByIndex(gIdx);
    }

    void PlaySingleFileByIndex(int fileIdx) {
        int gIdx = GetSelectedGroupIndex();
        if (gIdx < 0 || gIdx >= (int)m_items.size()) return;
        auto& files = m_items[gIdx].audioFiles;
        if (fileIdx < 0 || fileIdx >= (int)files.size()) return;

        SetRightListViewSelection(fileIdx);
        UpdateRightButtonsState();

        const std::wstring& path = files[fileIdx];
        AudioEngine::Instance().PlaySingle(path, m_hwnd);
        UpdateAllPlayIcons();

        const wchar_t* fn = wcsrchr(path.c_str(), L'\\');
        std::wstring fileName = fn ? (fn + 1) : path;
        std::wstring st = L"🔊 กำลังเล่นไฟล์: " + fileName;
        SendMessage(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)st.c_str());
    }

    void PauseOrResumeTrack(int fileIdx) {
        int gIdx = GetSelectedGroupIndex();
        if (gIdx < 0 || gIdx >= (int)m_items.size()) return;
        auto& files = m_items[gIdx].audioFiles;
        if (fileIdx < 0 || fileIdx >= (int)files.size()) return;

        const std::wstring& path = files[fileIdx];
        if (AudioEngine::Instance().IsTrackPlaying(path)) {
            if (AudioEngine::Instance().IsTrackPaused(path)) {
                AudioEngine::Instance().Resume();
                SendMessage(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)L"▶️ เล่นต่อ...");
            } else {
                AudioEngine::Instance().Pause();
                SendMessage(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)L"⏸️ พักเสียงชั่วคราว");
            }
            RefreshRightPanelRows();
        } else {
            // Not currently playing this track, so start playing it
            PlaySingleFileByIndex(fileIdx);
        }
    }

    void StopTrack() {
        AudioEngine::Instance().Stop();
        UpdateAllPlayIcons();
        SendMessage(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)L"⏹️ หยุดเสียงแล้ว");
    }

    void OnPlaySelectedFile() {
        int cur = GetSelectedRightFileIndex();
        if (cur < 0) {
            MessageBoxW(m_hwnd, L"กรุณาเลือกไฟล์ที่ต้องการทดลองฟังในฝั่งขวา", L"แจ้งเตือน", MB_OK | MB_ICONINFORMATION);
            return;
        }
        PlaySingleFileByIndex(cur);
    }

    void OnDeletePreset() {
        int sel = (int)SendMessage(m_hComboPresets, CB_GETCURSEL, 0, 0);
        if (sel == CB_ERR) {
            MessageBoxW(m_hwnd, L"กรุณาเลือกชุด Preset ที่ต้องการลบในกล่องเลือกชุดด้านบนก่อน", L"แจ้งเตือน", MB_OK | MB_ICONINFORMATION);
            return;
        }

        wchar_t buf[256] = { 0 };
        SendMessage(m_hComboPresets, CB_GETLBTEXT, sel, (LPARAM)buf);
        std::wstring presetName = buf;
        if (presetName.empty()) return;

        PresetManager::PresetDeleteInfo info = PresetManager::AnalyzePresetForDeletion(presetName);

        if (DeletePresetDialog::Show(m_hwnd, info)) {
            AudioEngine::Instance().Stop();

            if (PresetManager::DeletePresetFolder(info.presetFolder)) {
                // If deleting default, remove root default_preset.json as well
                std::wstring rootPreset = PresetManager::GetAppDirectory() + L"\\default_preset.json";
                if (_wcsicmp(presetName.c_str(), L"default") == 0 && PathFileExistsW(rootPreset.c_str())) {
                    DeleteFileW(rootPreset.c_str());
                }

                // If the deleted preset was active, clear active preset
                std::wstring curFolder;
                if (!m_currentPresetPath.empty()) {
                    curFolder = PresetManager::GetDirectoryOf(m_currentPresetPath);
                }
                if (_wcsicmp(curFolder.c_str(), info.presetFolder.c_str()) == 0) {
                    m_currentPresetPath.clear();
                    m_items.clear();
                    m_isDirty = false;
                    PresetManager::SetLastActivePreset(L"");
                }

                // Refresh ComboBox with remaining presets
                PopulatePresetsCombo();

                // If any presets remain, load the first one; otherwise clear UI
                int newCount = (int)SendMessage(m_hComboPresets, CB_GETCOUNT, 0, 0);
                if (newCount > 0) {
                    wchar_t firstBuf[256] = { 0 };
                    SendMessage(m_hComboPresets, CB_GETLBTEXT, 0, (LPARAM)firstBuf);
                    LoadPresetByName(firstBuf);
                } else {
                    m_items.clear();
                    m_currentPresetPath.clear();
                    m_isDirty = false;
                    RefreshListView(-1);
                    UpdatePresetTitle();
                    UpdatePresetStatusBanner();
                }

                std::wstring successMsg = L"ลบชุดการเล่น [" + presetName + L"] สำเร็จ!\n\n"
                                          L"🗑️ ลบไฟล์ในชุด Preset: " + std::to_wstring(info.filesInPreset.size()) + L" ไฟล์\n"
                                          L"🔗 ไฟล์ภายนอกที่ Link มา (ปลอดภัย ไม่ถูกลบ): " + std::to_wstring(info.filesLinkedSafe.size()) + L" ไฟล์";
                MessageBoxW(m_hwnd, successMsg.c_str(), L"ลบชุดสำเร็จ", MB_OK | MB_ICONINFORMATION);
            } else {
                MessageBoxW(m_hwnd, L"ไม่สามารถลบโฟลเดอร์ชุดการเล่นได้ อาจมีไฟล์ที่กำลังถูกใช้งานอยู่", L"ข้อผิดพลาด", MB_OK | MB_ICONERROR);
            }
        }
    }

    // ================= Real-time Timer Tick (Flicker-Free) =================
    void OnTimerTick() {
        // Continuous Anti-Debug and Tampering Watchdog
        Security::AntiDebug::Enforce();

        SYSTEMTIME st;
        GetLocalTime(&st);

        // Advance track if current track finished
        AudioEngine::Instance().CheckAndAdvanceTrack();

        // Update Play icon in ListView if playback state changed
        std::wstring curPlayingGroup = AudioEngine::Instance().IsPlaying() ? AudioEngine::Instance().GetGroupName() : L"";
        if (m_lastPlayingGroup != curPlayingGroup) {
            m_lastPlayingGroup = curPlayingGroup;
            UpdateAllPlayIcons();
        }

        // Update Clock Display (only set text if second actually changed)
        wchar_t clockBuf[32];
        swprintf_s(clockBuf, L"%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
        if (m_lastClockText != clockBuf) {
            m_lastClockText = clockBuf;
            SetWindowTextW(m_hClockLabel, clockBuf);
        }

        // Check if any schedule matches right now
        int todayInt = st.wYear * 10000 + st.wMonth * 100 + st.wDay;
        for (size_t idx = 0; idx < m_items.size(); ++idx) {
            auto& item = m_items[idx];
            if (item.enabled && item.IsDayActive(st.wDayOfWeek)) {
                if (item.hour == st.wHour && item.minute == st.wMinute && item.second == st.wSecond) {
                    if (item.lastTriggeredDate != todayInt) {
                        item.lastTriggeredDate = todayInt;
                        if (!item.audioFiles.empty()) {
                            TriggerScheduleItem(item);
                            PlayGroupByIndex((int)idx, false);
                        } else {
                            std::wstring emptyMsg = L"🔔 [ถึงเวลา] กลุ่ม: " + item.name + L" (ยังไม่มีไฟล์เสียงในกลุ่ม)";
                            SendMessage(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)emptyMsg.c_str());
                        }
                    }
                }
            }
        }

        UpdateNextEventBanner();
        UpdateTrayTooltip();
        if (m_hWaveformPanel) InvalidateRect(m_hWaveformPanel, NULL, FALSE);

        // Update Audio Playback status on Status Bar
        std::wstring part0;
        if (AudioEngine::Instance().IsPlaying()) {
            part0 = L"🔊 " + AudioEngine::Instance().GetPlaybackStatusString();
        } else {
            part0 = L"พร้อมทำงาน | จำนวนกลุ่มทั้งหมด: " + std::to_wstring(m_items.size()) + L" กลุ่ม";
        }
        if (m_lastStatusPart0 != part0) {
            m_lastStatusPart0 = part0;
            SendMessage(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)part0.c_str());
        }

        wchar_t dateBuf[128];
        static const wchar_t* const fullDayNames[] = { L"วันอาทิตย์", L"วันจันทร์", L"วันอังคาร", L"วันพุธ", L"วันพฤหัสบดี", L"วันศุกร์", L"วันเสาร์" };
        swprintf_s(dateBuf, L"%s %02d/%02d/%04d | %s",
                  fullDayNames[st.wDayOfWeek], st.wDay, st.wMonth, st.wYear,
                  m_timezoneStr.c_str());
        if (m_lastStatusPart1 != dateBuf) {
            m_lastStatusPart1 = dateBuf;
            SendMessage(m_hStatusBar, SB_SETTEXTW, 1, (LPARAM)dateBuf);
        }
    }

    void TriggerScheduleItem(const ScheduleGroup& item) {
        std::wstring notifyMsg = L"🔔 [ถึงเวลาแล้ว] กำลังเริ่มกลุ่ม: " + item.name;
        SendMessage(m_hStatusBar, SB_SETTEXTW, 0, (LPARAM)notifyMsg.c_str());
        if (m_isMinimizedToTray) {
            ShowTrayBalloon(
                Lang::TrayTriggerTitle(),
                std::wstring(Lang::TrayTriggerMsg()) + item.name
            );
        }
    }

    void UpdateNextEventBanner() {
        SYSTEMTIME st;
        GetLocalTime(&st);
        int curSec = st.wHour * 3600 + st.wMinute * 60 + st.wSecond;

        int minDiff = 86400 * 7;
        const ScheduleGroup* nextItem = NULL;

        for (const auto& item : m_items) {
            if (!item.enabled) continue;

            for (int dayOffset = 0; dayOffset < 7; ++dayOffset) {
                int checkDay = (st.wDayOfWeek + dayOffset) % 7;
                if (!item.IsDayActive(checkDay)) continue;

                int itemSec = item.hour * 3600 + item.minute * 60 + item.second;
                int diff = (dayOffset * 86400) + (itemSec - curSec);

                if (diff > 0 && diff < minDiff) {
                    minDiff = diff;
                    nextItem = &item;
                    break;
                }
            }
        }

        std::wstring banner;
        if (nextItem) {
            int hRem = minDiff / 3600;
            int mRem = (minDiff % 3600) / 60;
            int sRem = minDiff % 60;

            wchar_t buf[300];
            if (Lang::current == Language::Thai) {
                swprintf_s(buf, L"⏰ กลุ่มถัดไป: %s (%s, %d ไฟล์)\nเหลืออีก %02d ชม. %02d นาที %02d วินาที",
                          nextItem->name.c_str(), nextItem->GetTimeString().c_str(),
                          (int)nextItem->audioFiles.size(),
                          hRem, mRem, sRem);
            } else {
                swprintf_s(buf, L"⏰ Next: %s (%s, %d tracks)\nCountdown: %02dh %02dm %02ds",
                          nextItem->name.c_str(), nextItem->GetTimeString().c_str(),
                          (int)nextItem->audioFiles.size(),
                          hRem, mRem, sRem);
            }
            banner = buf;
        } else {
            banner = (Lang::current == Language::Thai)
                ? L"ไม่มีกลุ่มที่จะเล่นในขณะนี้ (หรือยังไม่ได้เปิดใช้งาน)"
                : L"No upcoming active schedule";
        }

        if (m_lastNextText != banner) {
            m_lastNextText = banner;
            SetWindowTextW(m_hNextLabel, banner.c_str());
        }
    }

    // ------------------------------------------------------------------------
    // System Tray Management
    // ------------------------------------------------------------------------
    void InitTrayIcon() {
        if (!m_hwnd) return;
        NOTIFYICONDATAW nid = {};
        nid.cbSize = sizeof(NOTIFYICONDATAW);
        nid.hWnd = m_hwnd;
        nid.uID = 1;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_TRAYICON;
        nid.hIcon = m_hIconSm ? m_hIconSm : LoadIconW(NULL, IDI_APPLICATION);
        wcsncpy_s(nid.szTip, L"Audio Schedule", _TRUNCATE);

        if (!Shell_NotifyIconW(NIM_ADD, &nid)) {
            nid.cbSize = NOTIFYICONDATAW_V3_SIZE;
            Shell_NotifyIconW(NIM_ADD, &nid);
        }

        nid.uVersion = NOTIFYICON_VERSION;
        Shell_NotifyIconW(NIM_SETVERSION, &nid);
    }

    void RemoveTrayIcon() {
        if (!m_hwnd) return;
        NOTIFYICONDATAW nid = {};
        nid.cbSize = sizeof(NOTIFYICONDATAW);
        nid.hWnd = m_hwnd;
        nid.uID = 1;
        if (!Shell_NotifyIconW(NIM_DELETE, &nid)) {
            nid.cbSize = NOTIFYICONDATAW_V3_SIZE;
            Shell_NotifyIconW(NIM_DELETE, &nid);
        }
    }

    void MinimizeToTray() {
        if (!m_hwnd) return;
        ShowWindow(m_hwnd, SW_HIDE);
        m_isMinimizedToTray = true;
        UpdateTrayTooltip();
        ShowTrayNotificationBalloon();
    }

    void RestoreFromTray() {
        if (!m_hwnd) return;
        m_isMinimizedToTray = false;
        ShowWindow(m_hwnd, SW_SHOW);
        if (IsIconic(m_hwnd)) {
            ShowWindow(m_hwnd, SW_RESTORE);
        }
        HWND hTarget = GetLastActivePopup(m_hwnd);
        if (!hTarget || !IsWindow(hTarget)) hTarget = m_hwnd;
        SetForegroundWindow(hTarget);
        BringWindowToTop(hTarget);
    }

    void ShowTrayContextMenu() {
        static DWORD s_lastTime = 0;
        DWORD now = GetTickCount();
        if (now - s_lastTime < 250) return; // Debounce duplicate events
        s_lastTime = now;

        POINT pt;
        GetCursorPos(&pt);

        HMENU hMenu = CreatePopupMenu();
        if (!hMenu) return;

        AppendMenuW(hMenu, MF_STRING, IDM_TRAY_OPEN, Lang::TrayOpen());
        SetMenuDefaultItem(hMenu, IDM_TRAY_OPEN, FALSE);

        if (AudioEngine::Instance().IsPlaying()) {
            AppendMenuW(hMenu, MF_STRING, IDM_TRAY_STOP, Lang::TrayStop());
        }

        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hMenu, MF_STRING, IDM_TRAY_EXIT, Lang::TrayExit());

        SetForegroundWindow(m_hwnd);
        TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_RIGHTALIGN, pt.x, pt.y, 0, m_hwnd, NULL);
        PostMessage(m_hwnd, WM_NULL, 0, 0);
        DestroyMenu(hMenu);
    }

    void ShowTrayBalloon(const std::wstring& title, const std::wstring& msg) {
        if (!m_hwnd) return;
        NOTIFYICONDATAW nid = {};
        nid.cbSize = sizeof(NOTIFYICONDATAW);
        nid.hWnd = m_hwnd;
        nid.uID = 1;
        nid.uFlags = NIF_INFO;
        nid.dwInfoFlags = NIIF_INFO;
        wcsncpy_s(nid.szInfoTitle, title.c_str(), _TRUNCATE);
        wcsncpy_s(nid.szInfo, msg.c_str(), _TRUNCATE);

        if (!Shell_NotifyIconW(NIM_MODIFY, &nid)) {
            nid.cbSize = NOTIFYICONDATAW_V3_SIZE;
            Shell_NotifyIconW(NIM_MODIFY, &nid);
        }
    }

    void ShowTrayNotificationBalloon() {
        if (m_balloonShownOnce) return;
        m_balloonShownOnce = true;
        ShowTrayBalloon(
            Lang::TrayMinimizedTitle(),
            Lang::TrayMinimizedMsg()
        );
    }

    void UpdateTrayTooltip() {
        if (!m_hwnd) return;

        std::wstring tip = L"Audio Schedule";
        if (AudioEngine::Instance().IsPlaying()) {
            tip += L"\n" + std::wstring(Lang::current == Language::Thai ? L"🔊 กำลังเล่น: " : L"🔊 Playing: ") + AudioEngine::Instance().GetPlaybackStatusString();
        } else {
            SYSTEMTIME st;
            GetLocalTime(&st);
            int curSec = st.wHour * 3600 + st.wMinute * 60 + st.wSecond;
            int minDiff = 86400 * 7;
            const ScheduleGroup* nextItem = NULL;

            for (const auto& item : m_items) {
                if (!item.enabled) continue;
                for (int dayOffset = 0; dayOffset < 7; ++dayOffset) {
                    int checkDay = (st.wDayOfWeek + dayOffset) % 7;
                    if (!item.IsDayActive(checkDay)) continue;
                    int itemSec = item.hour * 3600 + item.minute * 60 + item.second;
                    int diff = (dayOffset * 86400) + (itemSec - curSec);
                    if (diff > 0 && diff < minDiff) {
                        minDiff = diff;
                        nextItem = &item;
                        break;
                    }
                }
            }

            if (nextItem) {
                tip += L"\n" + std::wstring(Lang::current == Language::Thai ? L"⏰ รายการถัดไป: " : L"⏰ Next: ") + nextItem->name + L" (" + nextItem->GetTimeString() + L")";
            }
        }

        if (tip.length() >= 127) {
            tip = tip.substr(0, 124) + L"...";
        }

        if (m_lastTrayTip == tip) return;
        m_lastTrayTip = tip;

        NOTIFYICONDATAW nid = {};
        nid.cbSize = sizeof(NOTIFYICONDATAW);
        nid.hWnd = m_hwnd;
        nid.uID = 1;
        nid.uFlags = NIF_TIP;
        wcsncpy_s(nid.szTip, tip.c_str(), _TRUNCATE);

        if (!Shell_NotifyIconW(NIM_MODIFY, &nid)) {
            nid.cbSize = NOTIFYICONDATAW_V3_SIZE;
            Shell_NotifyIconW(NIM_MODIFY, &nid);
        }
    }
};
