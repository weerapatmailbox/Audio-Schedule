#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <filesystem>
#include <thread>
#include <atomic>
#include <functional>
#include <cstdint>
#include "../Security.hpp"
#include "../Version.h"
#include "../AboutDialog.hpp"

namespace fs = std::filesystem;

#define IDR_PAYLOAD 1001

enum class WizardStep {
    Maintenance = 0,
    Eula = 1,
    Scope = 2,
    Destination = 3,
    Tasks = 4,
    Ready = 5,
    Installing = 6,
    Finished = 7
};

// UI Control IDs
#define IDC_BTN_ABOUT        100
#define IDC_BTN_BACK         101
#define IDC_BTN_NEXT         102
#define IDC_BTN_CANCEL       103

// Step Maintenance: Detected Existing Installation
#define IDC_BTN_MAINT_MODIFY  180
#define IDC_BTN_MAINT_CLEAN   181
#define IDC_BTN_MAINT_UNINST  182

// Step 0: EULA
#define IDC_EDIT_EULA        110
#define IDC_RADIO_EULA_ACCEPT 111
#define IDC_RADIO_EULA_DECLINE 112

// Step 1: Scope
#define IDC_RADIO_SCOPE_USER  120
#define IDC_RADIO_SCOPE_ADMIN 121

// Step 2: Destination
#define IDC_EDIT_DEST         130
#define IDC_BTN_BROWSE        131

// Step 3: Tasks
#define IDC_CHK_DESKTOP       140
#define IDC_CHK_STARTMENU     141
#define IDC_CHK_AUTOSTART     142

// Step 4: Ready
#define IDC_EDIT_SUMMARY      150

// Step 5: Installing
#define IDC_PROGRESS          160
#define IDC_STATUS_TEXT       161

// Step 6: Finished
#define IDC_CHK_LAUNCH        170

static HINSTANCE g_hInstance = NULL;
static HWND g_hMainWnd = NULL;
static HFONT g_hFontTitle = NULL;
static HFONT g_hFontSubtitle = NULL;
static HFONT g_hFontBold = NULL;
static HFONT g_hFontNormal = NULL;
static HFONT g_hFontSmall = NULL;
static HFONT g_hFontMono = NULL;
static HICON g_hAppIcon = NULL;

static WizardStep g_step = WizardStep::Eula;
static bool g_isThai = false; // Default: English
static bool g_isAdminMode = false;
static std::wstring g_destDir;
static bool g_createDesktop = true;
static bool g_createStartMenu = true;
static bool g_autoStart = false;
static bool g_launchAfter = true;
static bool g_isSilent = false;
static bool g_isElevated = false;

// Existing Installation Detection
static bool g_hasExistingInstall = false;
static std::wstring g_existingDir;
static bool g_existingIsAdmin = false;
static std::wstring g_existingVersion;

static std::atomic<bool> g_isInstalling(false);
static std::atomic<bool> g_installCompleted(false);

// Common Wizard Controls
static HWND g_hBannerTitle = NULL;
static HWND g_hBannerSubtitle = NULL;
static HWND g_hBannerIcon = NULL;

static HWND g_hBtnAbout = NULL;
static HWND g_hBtnBack = NULL;
static HWND g_hBtnNext = NULL;
static HWND g_hBtnCancel = NULL;

// Maintenance Step Controls
static HWND g_hLblMaintPrompt = NULL;
static HWND g_hBtnMaintModify = NULL;
static HWND g_hLblMaintModifyDesc = NULL;
static HWND g_hBtnMaintClean = NULL;
static HWND g_hLblMaintCleanDesc = NULL;
static HWND g_hBtnMaintUninstall = NULL;
static HWND g_hLblMaintUninstallDesc = NULL;

// Step Controls
static HWND g_hLblEulaPrompt = NULL;
static HWND g_hEditEula = NULL;
static HWND g_hRadioEulaAccept = NULL;
static HWND g_hRadioEulaDecline = NULL;

static HWND g_hLblScopePrompt = NULL;
static HWND g_hRadioScopeUser = NULL;
static HWND g_hLblScopeUserDesc = NULL;
static HWND g_hRadioScopeAdmin = NULL;
static HWND g_hLblScopeAdminDesc = NULL;
static HWND g_hLblScopeShield = NULL;

static HWND g_hLblDestPrompt = NULL;
static HWND g_hEditDest = NULL;
static HWND g_hBtnBrowse = NULL;
static HWND g_hLblDestSpace = NULL;

static HWND g_hLblTasksPrompt = NULL;
static HWND g_hChkDesktop = NULL;
static HWND g_hChkStartMenu = NULL;
static HWND g_hChkAutoStart = NULL;

static HWND g_hLblReadyPrompt = NULL;
static HWND g_hEditSummary = NULL;
static HWND g_hLblReadySub = NULL;

static HWND g_hLblInstalling = NULL;
static HWND g_hProgress = NULL;
static HWND g_hStatusText = NULL;

static HWND g_hLblFinishTitle = NULL;
static HWND g_hLblFinishDesc = NULL;
static HWND g_hChkLaunch = NULL;

static const wchar_t* const EULA_TEXT =
    L"MIT License\r\n\r\n"
    L"Copyright (c) 2026 Mr.Weerapat Nusui <weerapat.mailbox@gmail.com> & Softlunar Foundation\r\n\r\n"
    L"Author: Mr.Weerapat Nusui <weerapat.mailbox@gmail.com>\r\n"
    L"Publisher: Softlunar Foundation\r\n\r\n"
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
    L"SOFTWARE.";

static std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0);
    std::wstring wstr(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &wstr[0], size);
    return wstr;
}

// 32-byte rolling XOR key for payload obfuscation (Level 2)
static const uint8_t PAYLOAD_CIPHER_KEY[32] = {
    0x53, 0x6F, 0x66, 0x74, 0x6C, 0x75, 0x6E, 0x61, // "Softluna"
    0x72, 0x41, 0x75, 0x64, 0x69, 0x6F, 0x53, 0x63, // "rAudioSc"
    0x68, 0x65, 0x64, 0x75, 0x6C, 0x65, 0x32, 0x30, // "hedule20"
    0x32, 0x36, 0x50, 0x72, 0x6F, 0x74, 0x65, 0x63  // "26Protec"
};

inline void TransformPayload(uint8_t* data, size_t size, uint64_t streamPos) {
    for (size_t i = 0; i < size; ++i) {
        uint8_t keyByte = PAYLOAD_CIPHER_KEY[(streamPos + i) % 32];
        uint8_t salt = static_cast<uint8_t>((streamPos + i) * 3 + 7);
        data[i] ^= (keyByte ^ salt);
    }
}

static std::wstring GetSpecialFolderPath(int csidl) {
    wchar_t szPath[MAX_PATH] = { 0 };
    if (SUCCEEDED(SHGetFolderPathW(NULL, csidl, NULL, 0, szPath))) {
        return szPath;
    }
    return L"";
}

static bool IsRunAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return (isAdmin == TRUE);
}

static std::wstring GetUserProgramsDirectory() {
    std::wstring localAppData = GetSpecialFolderPath(CSIDL_LOCAL_APPDATA);
    if (!localAppData.empty()) {
        return localAppData + L"\\Programs\\AudioSchedule";
    }
    return L"C:\\AudioSchedule";
}

static std::wstring GetAdminProgramsDirectory() {
    std::wstring progFiles = GetSpecialFolderPath(CSIDL_PROGRAM_FILES);
    if (!progFiles.empty()) {
        return progFiles + L"\\AudioSchedule";
    }
    return L"C:\\Program Files\\AudioSchedule";
}

static bool CreateDirectoryRecursive(const std::wstring& dirPath) {
    std::error_code ec;
    return fs::create_directories(fs::path(dirPath), ec);
}

static bool CreateShortcut(const std::wstring& shortcutPath, const std::wstring& targetExe, const std::wstring& workDir, const std::wstring& description, const std::wstring& iconLocation) {
    bool needUninit = false;
    HRESULT hrCo = CoInitialize(NULL);
    if (hrCo == S_OK) {
        needUninit = true;
    }
    IShellLinkW* psl = NULL;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl);
    if (SUCCEEDED(hr)) {
        psl->SetPath(targetExe.c_str());
        psl->SetWorkingDirectory(workDir.c_str());
        if (!description.empty()) psl->SetDescription(description.c_str());
        if (!iconLocation.empty()) psl->SetIconLocation(iconLocation.c_str(), 0);

        IPersistFile* ppf = NULL;
        hr = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
        if (SUCCEEDED(hr)) {
            hr = ppf->Save(shortcutPath.c_str(), TRUE);
            ppf->Release();
        }
        psl->Release();
    }
    if (needUninit) {
        CoUninitialize();
    }
    return SUCCEEDED(hr);
}

static bool SetRegString(HKEY hRoot, const std::wstring& subKey, const std::wstring& valueName, const std::wstring& data) {
    HKEY hKey = NULL;
    if (RegCreateKeyExW(hRoot, subKey.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, valueName.c_str(), 0, REG_SZ, (const BYTE*)data.c_str(), (DWORD)((data.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

static bool SetRegDword(HKEY hRoot, const std::wstring& subKey, const std::wstring& valueName, DWORD data) {
    HKEY hKey = NULL;
    if (RegCreateKeyExW(hRoot, subKey.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, valueName.c_str(), 0, REG_DWORD, (const BYTE*)&data, sizeof(DWORD));
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

static bool IsAppRunning() {
    return (FindWindowW(APP_WINDOW_CLASS_W, NULL) != NULL);
}

static bool DetectExistingInstallation(std::wstring& outDir, bool& outIsAdmin, std::wstring& outVersion) {
    HKEY hKey = NULL;
    std::wstring regKey = CRYPT_W(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\AudioSchedule");

    // 1. Check HKCU (User Mode)
    if (RegOpenKeyExW(HKEY_CURRENT_USER, regKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t path[MAX_PATH] = { 0 };
        DWORD sz = sizeof(path);
        if (RegQueryValueExW(hKey, L"InstallLocation", NULL, NULL, (LPBYTE)path, &sz) == ERROR_SUCCESS) {
            wchar_t ver[64] = { 0 };
            DWORD verSz = sizeof(ver);
            RegQueryValueExW(hKey, L"DisplayVersion", NULL, NULL, (LPBYTE)ver, &verSz);
            RegCloseKey(hKey);

            std::wstring exe = std::wstring(path) + L"\\" + APP_EXE_NAME_W;
            if (PathFileExistsW(exe.c_str()) || PathFileExistsW(path)) {
                outDir = path;
                outIsAdmin = false;
                outVersion = (ver[0] != L'\0') ? ver : APP_VERSION_WSTRING;
                return true;
            }
        } else {
            RegCloseKey(hKey);
        }
    }

    // 2. Check HKLM (Admin Mode)
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, regKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t path[MAX_PATH] = { 0 };
        DWORD sz = sizeof(path);
        if (RegQueryValueExW(hKey, L"InstallLocation", NULL, NULL, (LPBYTE)path, &sz) == ERROR_SUCCESS) {
            wchar_t ver[64] = { 0 };
            DWORD verSz = sizeof(ver);
            RegQueryValueExW(hKey, L"DisplayVersion", NULL, NULL, (LPBYTE)ver, &verSz);
            RegCloseKey(hKey);

            std::wstring exe = std::wstring(path) + L"\\" + APP_EXE_NAME_W;
            if (PathFileExistsW(exe.c_str()) || PathFileExistsW(path)) {
                outDir = path;
                outIsAdmin = true;
                outVersion = (ver[0] != L'\0') ? ver : APP_VERSION_WSTRING;
                return true;
            }
        } else {
            RegCloseKey(hKey);
        }
    }

    // 3. Fallback: check standard folders
    std::wstring userDir = GetUserProgramsDirectory();
    if (PathFileExistsW((userDir + L"\\" + APP_EXE_NAME_W).c_str())) {
        outDir = userDir;
        outIsAdmin = false;
        outVersion = APP_VERSION_WSTRING;
        return true;
    }

    std::wstring adminDir = GetAdminProgramsDirectory();
    if (PathFileExistsW((adminDir + L"\\" + APP_EXE_NAME_W).c_str())) {
        outDir = adminDir;
        outIsAdmin = true;
        outVersion = APP_VERSION_WSTRING;
        return true;
    }

    return false;
}

static void PerformCleanRemoval(const std::wstring& targetDir, bool isAdmin, bool preservePresets = false) {
    // 1. Delete shortcuts
    int deskCsidl = isAdmin ? CSIDL_COMMON_DESKTOPDIRECTORY : CSIDL_DESKTOPDIRECTORY;
    std::wstring desk = GetSpecialFolderPath(deskCsidl);
    if (!desk.empty()) DeleteFileW((desk + L"\\" + APP_NAME_W + L".lnk").c_str());

    int smCsidl = isAdmin ? CSIDL_COMMON_PROGRAMS : CSIDL_PROGRAMS;
    std::wstring sm = GetSpecialFolderPath(smCsidl);
    if (!sm.empty()) {
        std::error_code ec;
        fs::remove_all(fs::path(sm + L"\\" + APP_NAME_W), ec);
    }

    // 2. Remove registry keys
    HKEY hRoot = isAdmin ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
    std::wstring regKey = CRYPT_W(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\AudioSchedule");
    RegDeleteTreeW(hRoot, regKey.c_str());

    std::wstring runKey = CRYPT_W(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run");
    HKEY hRun = NULL;
    if (RegOpenKeyExW(hRoot, runKey.c_str(), 0, KEY_SET_VALUE, &hRun) == ERROR_SUCCESS) {
        RegDeleteValueW(hRun, APP_NAME_W);
        RegCloseKey(hRun);
    }

    // 3. Delete directory (optionally preserve presets folder)
    if (!targetDir.empty() && PathFileExistsW(targetDir.c_str())) {
        std::error_code ec;
        if (preservePresets) {
            for (const auto& entry : fs::directory_iterator(targetDir, fs::directory_options::skip_permission_denied, ec)) {
                std::wstring fn = entry.path().filename().wstring();
                if (_wcsicmp(fn.c_str(), L"presets") == 0) {
                    continue; // Keep presets folder safe!
                }
                for (const auto& sub : fs::recursive_directory_iterator(entry.path(), fs::directory_options::skip_permission_denied, ec)) {
                    SetFileAttributesW(sub.path().c_str(), FILE_ATTRIBUTE_NORMAL);
                }
                SetFileAttributesW(entry.path().c_str(), FILE_ATTRIBUTE_NORMAL);
                fs::remove_all(entry.path(), ec);
            }
        } else {
            for (const auto& entry : fs::recursive_directory_iterator(targetDir, fs::directory_options::skip_permission_denied, ec)) {
                SetFileAttributesW(entry.path().c_str(), FILE_ATTRIBUTE_NORMAL);
            }
            fs::remove_all(targetDir, ec);
            if (PathFileExistsW(targetDir.c_str())) {
                wchar_t szPath[MAX_PATH + 2] = { 0 };
                wcsncpy_s(szPath, targetDir.c_str(), MAX_PATH);
                szPath[targetDir.length() + 1] = L'\0';
                SHFILEOPSTRUCTW sfos{};
                sfos.wFunc = FO_DELETE;
                sfos.pFrom = szPath;
                sfos.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
                SHFileOperationW(&sfos);
            }
        }
    }

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
}

// Extraction and setup implementation
static bool PerformInstallation(
    const std::wstring& destDir,
    bool isAdminScope,
    bool createDesktop,
    bool createStartMenu,
    bool autoStart,
    bool launchAfter,
    bool isSilent,
    std::function<void(int pos, int maxPos, const std::wstring& status)> onProgress
) {
    if (onProgress) onProgress(5, 100, g_isThai ? L"กำลังเตรียมการติดตั้ง..." : L"Preparing installation files...");

    HRSRC hRes = FindResourceW(g_hInstance, MAKEINTRESOURCEW(IDR_PAYLOAD), RT_RCDATA);
    if (!hRes) {
        if (!isSilent) MessageBoxW(g_hMainWnd, L"Payload resource missing!", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    HGLOBAL hMem = LoadResource(g_hInstance, hRes);
    if (!hMem) {
        if (!isSilent) MessageBoxW(g_hMainWnd, L"Cannot load payload resource!", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    DWORD resSize = SizeofResource(g_hInstance, hRes);
    const uint8_t* pData = (const uint8_t*)LockResource(hMem);
    bool isEncrypted = false;
    if (!pData || resSize < 8) {
        if (!isSilent) MessageBoxW(g_hMainWnd, L"Invalid payload resource!", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }
    if (memcmp(pData, "ASPE", 4) == 0) {
        isEncrypted = true;
    } else if (memcmp(pData, "ASPK", 4) == 0) {
        isEncrypted = false;
    } else {
        if (!isSilent) MessageBoxW(g_hMainWnd, L"Invalid payload signature!", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    size_t offset = 4;
    int32_t fileCount = *(const int32_t*)(pData + offset);
    offset += 4;
    uint64_t cipherStreamPos = 0;

    int totalSteps = fileCount + 6;
    CreateDirectoryRecursive(destDir);

    // Extract files
    for (int32_t i = 0; i < fileCount && offset < resSize; ++i) {
        if (offset + 2 > resSize) break;
        int16_t pathLen = *(const int16_t*)(pData + offset);
        offset += 2;

        if (offset + pathLen > resSize) break;
        std::string relPathUtf8;
        if (isEncrypted) {
            std::vector<uint8_t> decPath(pData + offset, pData + offset + pathLen);
            TransformPayload(decPath.data(), decPath.size(), cipherStreamPos);
            cipherStreamPos += decPath.size();
            relPathUtf8.assign((const char*)decPath.data(), decPath.size());
        } else {
            relPathUtf8.assign((const char*)(pData + offset), pathLen);
        }
        offset += pathLen;

        if (offset + 8 > resSize) break;
        int64_t fileSize = *(const int64_t*)(pData + offset);
        offset += 8;

        if (offset + fileSize > resSize) break;
        const uint8_t* fileBytes = pData + offset;
        offset += fileSize;

        std::wstring relPathW = Utf8ToWide(relPathUtf8);
        for (wchar_t& ch : relPathW) {
            if (ch == L'/') ch = L'\\';
        }

        std::wstring targetFilePath = destDir + L"\\" + relPathW;
        std::wstring parentDir = targetFilePath.substr(0, targetFilePath.find_last_of(L"\\/"));
        CreateDirectoryRecursive(parentDir);

        if (onProgress) onProgress(i + 1, totalSteps, (g_isThai ? L"กำลังแตกไฟล์: " : L"Extracting: ") + relPathW);

        HANDLE hFile = CreateFileW(
            targetFilePath.c_str(),
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD bytesWritten = 0;
            if (isEncrypted) {
                std::vector<uint8_t> decBytes(fileBytes, fileBytes + fileSize);
                TransformPayload(decBytes.data(), decBytes.size(), cipherStreamPos);
                cipherStreamPos += fileSize;
                WriteFile(hFile, decBytes.data(), (DWORD)fileSize, &bytesWritten, NULL);
            } else {
                WriteFile(hFile, fileBytes, (DWORD)fileSize, &bytesWritten, NULL);
            }
            CloseHandle(hFile);
        } else if (isEncrypted) {
            cipherStreamPos += fileSize;
        }
    }

    std::wstring targetExe = destDir + L"\\" + APP_EXE_NAME_W;
    std::wstring uninstallerExe = destDir + L"\\Uninstall.exe";
    std::wstring installerBackupExe = destDir + L"\\Installer.exe";

    // Copy running installer to destination directory so Windows 'Modify' can launch it
    wchar_t currentModulePath[MAX_PATH] = { 0 };
    if (GetModuleFileNameW(NULL, currentModulePath, MAX_PATH)) {
        if (_wcsicmp(currentModulePath, installerBackupExe.c_str()) != 0) {
            CopyFileW(currentModulePath, installerBackupExe.c_str(), FALSE);
        }
    }

    // Grant write permissions on presets folder for standard users if installed under Admin scope
    if (isAdminScope) {
        std::wstring icaclsCmd = L"/c icacls \"" + destDir + L"\\presets\" /grant *S-1-5-32-545:(OI)(CI)M /t /q";
        ShellExecuteW(NULL, L"open", L"cmd.exe", icaclsCmd.c_str(), NULL, SW_HIDE);
    }

    // Shortcuts
    if (createDesktop) {
        if (onProgress) onProgress(fileCount + 1, totalSteps, g_isThai ? L"กำลังสร้างทางลัดบนเดสก์ท็อป..." : L"Creating Desktop shortcut...");
        int csidl = isAdminScope ? CSIDL_COMMON_DESKTOPDIRECTORY : CSIDL_DESKTOPDIRECTORY;
        std::wstring desktop = GetSpecialFolderPath(csidl);
        if (desktop.empty()) desktop = GetSpecialFolderPath(CSIDL_DESKTOPDIRECTORY);
        if (!desktop.empty()) {
            std::wstring shortcut = desktop + L"\\" + APP_NAME_W + L".lnk";
            CreateShortcut(shortcut, targetExe, destDir, APP_DESC_W, targetExe);
            SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW, shortcut.c_str(), NULL);
        }
        if (isAdminScope) {
            std::wstring userDesktop = GetSpecialFolderPath(CSIDL_DESKTOPDIRECTORY);
            if (!userDesktop.empty() && _wcsicmp(userDesktop.c_str(), desktop.c_str()) != 0) {
                std::wstring userShortcut = userDesktop + L"\\" + APP_NAME_W + L".lnk";
                CreateShortcut(userShortcut, targetExe, destDir, APP_DESC_W, targetExe);
                SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW, userShortcut.c_str(), NULL);
            }
        }
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    }

    if (createStartMenu) {
        if (onProgress) onProgress(fileCount + 2, totalSteps, g_isThai ? L"กำลังสร้างทางลัดใน Start Menu..." : L"Creating Start Menu shortcut...");
        int csidl = isAdminScope ? CSIDL_COMMON_PROGRAMS : CSIDL_PROGRAMS;
        std::wstring startMenu = GetSpecialFolderPath(csidl);
        if (startMenu.empty()) startMenu = GetSpecialFolderPath(CSIDL_PROGRAMS);
        if (!startMenu.empty()) {
            std::wstring smFolder = startMenu + L"\\" + APP_NAME_W;
            CreateDirectoryRecursive(smFolder);
            std::wstring appShortcut = smFolder + L"\\" + APP_NAME_W + L".lnk";
            CreateShortcut(appShortcut, targetExe, destDir, APP_DESC_W, targetExe);
            SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW, appShortcut.c_str(), NULL);

            if (PathFileExistsW(uninstallerExe.c_str())) {
                std::wstring uninstShortcut = smFolder + L"\\Uninstall " + APP_NAME_W + L".lnk";
                CreateShortcut(uninstShortcut, uninstallerExe, destDir, L"Uninstall " APP_NAME_W, uninstallerExe);
                SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW, uninstShortcut.c_str(), NULL);
            }
        }
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    }

    // Auto-start on boot
    if (autoStart) {
        HKEY hRoot = isAdminScope ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
        std::wstring runKey = CRYPT_W(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run");
        std::wstring quotedExe = L"\"" + targetExe + L"\"";
        SetRegString(hRoot, runKey, L"AudioSchedule", quotedExe);
    }

    // Register in Windows Uninstall Registry
    if (onProgress) onProgress(fileCount + 4, totalSteps, g_isThai ? L"กำลังลงทะเบียนข้อมูลโปรแกรมในระบบ Windows..." : L"Registering application in Windows...");
    HKEY hRootKey = isAdminScope ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
    std::wstring regKey = CRYPT_W(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\AudioSchedule");
    SetRegString(hRootKey, regKey, L"DisplayName", APP_NAME_W);
    SetRegString(hRootKey, regKey, L"DisplayVersion", APP_VERSION_WSTRING);
    SetRegString(hRootKey, regKey, L"Publisher", APP_TEAM_W);
    SetRegString(hRootKey, regKey, L"InstallLocation", destDir);
    SetRegString(hRootKey, regKey, L"UninstallString", L"\"" + uninstallerExe + L"\"");
    SetRegString(hRootKey, regKey, L"ModifyPath", L"\"" + installerBackupExe + L"\"");
    SetRegString(hRootKey, regKey, L"DisplayIcon", L"\"" + targetExe + L"\",0");
    SetRegString(hRootKey, regKey, L"HelpLink", APP_WEBSITE_URL_W);
    SetRegString(hRootKey, regKey, L"URLInfoAbout", APP_WEBSITE_URL_W);
    SetRegString(hRootKey, regKey, L"Comments", APP_DESC_W);
    SetRegDword(hRootKey, regKey, L"NoModify", 0);
    SetRegDword(hRootKey, regKey, L"NoRepair", 1);

    if (onProgress) onProgress(totalSteps, totalSteps, g_isThai ? L"การติดตั้งเสร็จสมบูรณ์เรียบร้อยแล้ว!" : L"Installation completed successfully!");

    // NOTE: Application must NOT launch here. It will only launch if user checks the launch box and clicks Finish on the Finished step.

    return true;
}

static void DoInstallThread() {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    g_isInstalling = true;

    auto progressCallback = [](int pos, int maxPos, const std::wstring& status) {
        if (g_hProgress) {
            SendMessage(g_hProgress, PBM_SETRANGE32, 0, maxPos);
            SendMessage(g_hProgress, PBM_SETPOS, pos, 0);
        }
        if (g_hStatusText) {
            SetWindowTextW(g_hStatusText, status.c_str());
        }
    };

    bool ok = PerformInstallation(
        g_destDir,
        g_isAdminMode,
        g_createDesktop,
        g_createStartMenu,
        g_autoStart,
        g_launchAfter,
        g_isSilent,
        progressCallback
    );

    g_isInstalling = false;
    g_installCompleted = ok;

    CoUninitialize();

    if (g_hMainWnd) {
        PostMessageW(g_hMainWnd, WM_APP + 1, ok ? 1 : 0, 0);
    }
}

// Forward declarations of UI helpers
static void UpdateWizardUI();
static void SetWizardStep(WizardStep step);

static void UpdateWizardUI() {
    if (!g_hMainWnd) return;

    // Window Title
    std::wstring title = std::wstring(APP_NAME_W) + L" " + APP_VERSION_WSTRING +
        (g_isThai ? L" - ตัวช่วยสร้างการติดตั้ง (Setup Wizard)" : L" - Setup Wizard");
    SetWindowTextW(g_hMainWnd, title.c_str());

    // Navigation Buttons
    SetWindowTextW(g_hBtnAbout, g_isThai ? L"ℹ️ เกี่ยวกับ..." : L"ℹ️ About...");
    SetWindowTextW(g_hBtnBack, g_isThai ? L"< ย้อนกลับ" : L"< Back");
    SetWindowTextW(g_hBtnCancel, g_isThai ? L"ยกเลิก" : L"Cancel");

    if (g_step == WizardStep::Finished) {
        SetWindowTextW(g_hBtnNext, g_isThai ? L"เสร็จสิ้น (Finish)" : L"Finish");
    } else if (g_step == WizardStep::Ready) {
        SetWindowTextW(g_hBtnNext, g_isThai ? L"ติดตั้ง (Install)" : L"Install");
    } else {
        SetWindowTextW(g_hBtnNext, g_isThai ? L"ถัดไป >" : L"Next >");
    }

    // Banner Text
    switch (g_step) {
        case WizardStep::Maintenance:
            SetWindowTextW(g_hBannerTitle, g_isThai ? L"ตัวเลือกการบำรุงรักษาโปรแกรม (Maintenance)" : L"Maintenance Options");
            SetWindowTextW(g_hBannerSubtitle, g_isThai ? L"ตรวจพบว่า Audio Schedule ได้รับการติดตั้งอยู่แล้วในเครื่องนี้" : L"Audio Schedule is already installed on this computer.");
            break;
        case WizardStep::Eula:
            SetWindowTextW(g_hBannerTitle, g_isThai ? L"ข้อตกลงและเงื่อนไขการใช้งาน (License Agreement)" : L"License Agreement");
            SetWindowTextW(g_hBannerSubtitle, g_isThai ? L"โปรดอ่านข้อตกลงและเงื่อนไขอย่างละเอียดก่อนดำเนินการต่อ" : L"Please read the following license terms carefully before continuing.");
            break;
        case WizardStep::Scope:
            SetWindowTextW(g_hBannerTitle, g_isThai ? L"เลือกโหมดการติดตั้ง (Installation Scope)" : L"Select Installation Scope");
            SetWindowTextW(g_hBannerSubtitle, g_isThai ? L"เลือกว่าต้องการติดตั้งสำหรับผู้ใช้คนเดียวหรือทุกคนในคอมพิวเตอร์นี้" : L"Choose whether to install Audio Schedule for yourself or all users.");
            break;
        case WizardStep::Destination:
            SetWindowTextW(g_hBannerTitle, g_isThai ? L"เลือกตำแหน่งติดตั้ง (Destination Location)" : L"Select Destination Location");
            SetWindowTextW(g_hBannerSubtitle, g_isThai ? L"โฟลเดอร์ที่โปรแกรมติดตั้งจะคัดลอกไฟล์ลงในเครื่องของคุณ" : L"Where should Audio Schedule be installed on your computer?");
            break;
        case WizardStep::Tasks:
            SetWindowTextW(g_hBannerTitle, g_isThai ? L"เลือกงานเพิ่มเติม (Additional Tasks)" : L"Select Additional Tasks");
            SetWindowTextW(g_hBannerSubtitle, g_isThai ? L"เลือกการสร้างทางลัดและการเปิดโปรแกรมอัตโนมัติ" : L"Which additional tasks should Setup perform?");
            break;
        case WizardStep::Ready:
            SetWindowTextW(g_hBannerTitle, g_isThai ? L"พร้อมสำหรับการติดตั้ง (Ready to Install)" : L"Ready to Install");
            SetWindowTextW(g_hBannerSubtitle, g_isThai ? L"คลิก 'ติดตั้ง' เพื่อเริ่มต้นกระบวนการติดตั้งไฟล์ลงในเครื่อง" : L"Setup is now ready to begin installing Audio Schedule on your computer.");
            break;
        case WizardStep::Installing:
            SetWindowTextW(g_hBannerTitle, g_isThai ? L"กำลังดำเนินการติดตั้ง (Installing)" : L"Installing Files");
            SetWindowTextW(g_hBannerSubtitle, g_isThai ? L"โปรดรอสักครู่ขณะที่ระบบกำลังแตกไฟล์และตั้งค่าโปรแกรม..." : L"Please wait while Setup installs Audio Schedule on your computer...");
            break;
        case WizardStep::Finished:
            SetWindowTextW(g_hBannerTitle, g_isThai ? L"การติดตั้งเสร็จสมบูรณ์ (Installation Complete)" : L"Installation Complete");
            SetWindowTextW(g_hBannerSubtitle, g_isThai ? L"Audio Schedule ได้รับการติดตั้งบนเครื่องของคุณเรียบร้อยแล้ว" : L"Audio Schedule has been successfully installed on your computer.");
            break;
    }

    // Step Maintenance
    std::wstring maintPrompt = g_isThai ?
        (L"ตรวจพบ Audio Schedule v" + g_existingVersion + L" ติดตั้งอยู่ที่:\r\n" + g_existingDir) :
        (L"Audio Schedule v" + g_existingVersion + L" was detected at:\r\n" + g_existingDir);
    SetWindowTextW(g_hLblMaintPrompt, maintPrompt.c_str());

    SetWindowTextW(g_hBtnMaintModify, g_isThai ? L"1. แก้ไข\n(Modify)" : L"1. Modify");
    SetWindowTextW(g_hLblMaintModifyDesc, g_isThai ?
        L"อัปเดตไฟล์โปรแกรม ซ่อมแซมส่วนประกอบ และตั้งค่าทางลัดใหม่ โดยที่ข้อมูลเพลงและตารางเวลายังคงอยู่ครบถ้วน" :
        L"Update files, restore missing components, and configure shortcuts without affecting your schedules or presets.");

    SetWindowTextW(g_hBtnMaintClean, g_isThai ? L"2. ติดตั้งใหม่ทั้งหมด\n(Clean Install)" : L"2. Clean Install");
    SetWindowTextW(g_hLblMaintCleanDesc, g_isThai ?
        L"ลบไฟล์โปรแกรม ข้อมูลตารางเวลา และโฟลเดอร์พรีเซ็ตเสียง (Presets) เดิมทั้งหมดออกเหมือนถอนการติดตั้ง แล้วเริ่มติดตั้งใหม่ตั้งแต่ต้น" :
        L"Completely wipe all existing application files, schedules, and presets like a full uninstall, then reinstall fresh from scratch.");

    SetWindowTextW(g_hBtnMaintUninstall, g_isThai ? L"3. ถอนการติดตั้ง\n(Uninstall)" : L"3. Uninstall");
    SetWindowTextW(g_hLblMaintUninstallDesc, g_isThai ?
        L"ถอนการติดตั้งและลบโปรแกรม Audio Schedule รวมถึงทางลัดและการลงทะเบียนทั้งหมดออกจากเครื่องนี้" :
        L"Completely remove Audio Schedule, all shortcuts, and registered system settings from this computer.");

    // Step 0: EULA
    SetWindowTextW(g_hLblEulaPrompt, g_isThai ?
        L"โปรดอ่านข้อตกลงสัญญาอนุญาตต่อไปนี้ คุณต้องยอมรับข้อตกลงก่อนดำเนินการต่อ:" :
        L"Please read the following license agreement. You must accept the terms before continuing:");
    SetWindowTextW(g_hRadioEulaAccept, g_isThai ? L"ฉันยอมรับข้อตกลง (I accept the agreement)" : L"I accept the agreement");
    SetWindowTextW(g_hRadioEulaDecline, g_isThai ? L"ฉันไม่ยอมรับข้อตกลง (I do not accept the agreement)" : L"I do not accept the agreement");

    // Step 1: Scope
    SetWindowTextW(g_hLblScopePrompt, g_isThai ?
        L"เลือกลักษณะการติดตั้งที่เหมาะสมกับคอมพิวเตอร์ของคุณ:" :
        L"Select the installation scope that fits your machine:");
    SetWindowTextW(g_hRadioScopeUser, g_isThai ?
        L"ติดตั้งสำหรับผู้ใช้ปัจจุบันเท่านั้น (User Mode - แนะนำ)" :
        L"Install for current user only (User Mode - Recommended)");
    SetWindowTextW(g_hLblScopeUserDesc, g_isThai ?
        L"• ติดตั้งลงใน %LocalAppData%\\Programs\\AudioSchedule\n• ไม่ต้องใช้สิทธิ์ Administrator (ปลอดภัย 100% ไม่ติด UAC)" :
        L"• Installs into %LocalAppData%\\Programs\\AudioSchedule\n• No administrator privileges required (runs without UAC prompt)");
    SetWindowTextW(g_hRadioScopeAdmin, g_isThai ?
        L"ติดตั้งสำหรับทุกคนในคอมพิวเตอร์เครื่องนี้ (Admin Mode)" :
        L"Install for all users on this computer (Admin Mode)");
    SetWindowTextW(g_hLblScopeAdminDesc, g_isThai ?
        L"• ติดตั้งลงใน Program Files (เช่น C:\\Program Files\\AudioSchedule)\n• จำเป็นต้องใช้สิทธิ์ Administrator ในการติดตั้ง" :
        L"• Installs into system Program Files (e.g. C:\\Program Files\\AudioSchedule)\n• Requires Administrator privileges during installation");
    SetWindowTextW(g_hLblScopeShield, g_isThai ?
        L"🛡️ ระบบจะขอสิทธิ์ผู้ดูแลระบบ (UAC) เมื่อเลือก Admin Mode" :
        L"🛡️ Administrator privileges will be requested when proceeding in Admin Mode.");

    // Step 2: Destination
    SetWindowTextW(g_hLblDestPrompt, g_isThai ?
        L"โปรแกรมติดตั้งจะติดตั้ง Audio Schedule ลงในโฟลเดอร์ต่อไปนี้:" :
        L"Setup will install Audio Schedule into the following destination folder:");
    SetWindowTextW(g_hBtnBrowse, g_isThai ? L"เรียกดู... (Browse)" : L"Browse...");
    SetWindowTextW(g_hLblDestSpace, g_isThai ?
        L"พื้นที่ว่างในดิสก์ที่ต้องการ: อย่างน้อย 25 MB" :
        L"At least 25 MB of free disk space is required.");

    // Step 3: Tasks
    SetWindowTextW(g_hLblTasksPrompt, g_isThai ?
        L"เลือกงานเพิ่มเติมที่ต้องการให้ระบบดำเนินการ:" :
        L"Select the additional tasks you would like Setup to perform:");
    SetWindowTextW(g_hChkDesktop, g_isThai ?
        L"สร้างทางลัดบนหน้าเดสก์ท็อป (Create Desktop Shortcut)" :
        L"Create a desktop shortcut");
    SetWindowTextW(g_hChkStartMenu, g_isThai ?
        L"สร้างทางลัดในเมนูเริ่มต้น (Create Start Menu Shortcut)" :
        L"Create a Start Menu shortcut");
    SetWindowTextW(g_hChkAutoStart, g_isThai ?
        L"เปิดโปรแกรมอัตโนมัติเมื่อเปิดเครื่อง (Start automatically when Windows boots)" :
        L"Start Audio Schedule automatically when Windows starts");

    // Step 4: Ready
    SetWindowTextW(g_hLblReadyPrompt, g_isThai ?
        L"สรุปรายการการติดตั้งที่คุณได้เลือกไว้:" :
        L"Summary of your selected installation settings:");
    SetWindowTextW(g_hLblReadySub, g_isThai ?
        L"กดปุ่ม 'ติดตั้ง' เพื่อเริ่มต้น หรือกดย้อนกลับหากต้องการแก้ไขข้อมูล" :
        L"Click 'Install' to begin, or click 'Back' to review or change settings.");

    // Step 5: Installing
    SetWindowTextW(g_hLblInstalling, g_isThai ?
        L"ระบบกำลังติดตั้งไฟล์ โปรดรอสักครู่..." :
        L"Extracting files and setting up system components, please wait...");

    // Step 6: Finished
    SetWindowTextW(g_hLblFinishTitle, g_isThai ?
        L"การติดตั้ง Audio Schedule เสร็จสมบูรณ์เรียบร้อยแล้ว!" :
        L"Audio Schedule has been successfully installed!");
    SetWindowTextW(g_hLblFinishDesc, g_isThai ?
        L"คุณสามารถเปิดใช้งานโปรแกรมได้ทันที หรือเปิดจากทางลัดที่สร้างไว้\n\nกดปุ่ม 'เสร็จสิ้น' เพื่อปิดตัวช่วยติดตั้ง" :
        L"You may launch the application now or use the created shortcuts.\n\nClick 'Finish' to exit Setup.");
    SetWindowTextW(g_hChkLaunch, g_isThai ?
        L"เปิดโปรแกรม Audio Schedule ทันที (Launch Audio Schedule now)" :
        L"Launch Audio Schedule now");

    // Update Summary Box if on Ready Step
    if (g_step == WizardStep::Ready && g_hEditSummary) {
        std::wstring sum;
        sum += (g_isThai ? L"โหมดการติดตั้ง:\r\n  " : L"Installation Scope:\r\n  ");
        sum += (g_isAdminMode ?
            (g_isThai ? L"Admin Mode (สำหรับทุกคนในเครื่อง)" : L"Admin Mode (All users on this machine)") :
            (g_isThai ? L"User Mode (สำหรับผู้ใช้ปัจจุบันเท่านั้น)" : L"User Mode (Current user only)"));
        sum += (g_isThai ? L"\r\n\r\nโฟลเดอร์ปลายทาง:\r\n  " : L"\r\n\r\nDestination Folder:\r\n  ");
        sum += g_destDir;
        sum += (g_isThai ? L"\r\n\r\nงานเพิ่มเติม:\r\n" : L"\r\n\r\nSelected Tasks:\r\n");
        if (g_createDesktop) {
            sum += (g_isThai ? L"  • สร้างทางลัดบนหน้าเดสก์ท็อป\r\n" : L"  • Create Desktop shortcut\r\n");
        }
        if (g_createStartMenu) {
            sum += (g_isThai ? L"  • สร้างทางลัดในเมนูเริ่มต้น (Start Menu)\r\n" : L"  • Create Start Menu shortcut\r\n");
        }
        if (g_autoStart) {
            sum += (g_isThai ? L"  • เปิดโปรแกรมอัตโนมัติเมื่อเปิดเครื่อง\r\n" : L"  • Start automatically on Windows boot\r\n");
        }
        SetWindowTextW(g_hEditSummary, sum.c_str());
    }
}

static void SetWizardStep(WizardStep step) {
    g_step = step;

    auto Show = [](HWND h, bool visible) {
        if (h) ShowWindow(h, visible ? SW_SHOW : SW_HIDE);
    };

    Show(g_hLblMaintPrompt, step == WizardStep::Maintenance);
    Show(g_hBtnMaintModify, step == WizardStep::Maintenance);
    Show(g_hLblMaintModifyDesc, step == WizardStep::Maintenance);
    Show(g_hBtnMaintClean, step == WizardStep::Maintenance);
    Show(g_hLblMaintCleanDesc, step == WizardStep::Maintenance);
    Show(g_hBtnMaintUninstall, step == WizardStep::Maintenance);
    Show(g_hLblMaintUninstallDesc, step == WizardStep::Maintenance);

    Show(g_hLblEulaPrompt, step == WizardStep::Eula);
    Show(g_hEditEula, step == WizardStep::Eula);
    Show(g_hRadioEulaAccept, step == WizardStep::Eula);
    Show(g_hRadioEulaDecline, step == WizardStep::Eula);

    Show(g_hLblScopePrompt, step == WizardStep::Scope);
    Show(g_hRadioScopeUser, step == WizardStep::Scope);
    Show(g_hLblScopeUserDesc, step == WizardStep::Scope);
    Show(g_hRadioScopeAdmin, step == WizardStep::Scope);
    Show(g_hLblScopeAdminDesc, step == WizardStep::Scope);
    Show(g_hLblScopeShield, step == WizardStep::Scope);

    Show(g_hLblDestPrompt, step == WizardStep::Destination);
    Show(g_hEditDest, step == WizardStep::Destination);
    Show(g_hBtnBrowse, step == WizardStep::Destination);
    Show(g_hLblDestSpace, step == WizardStep::Destination);

    Show(g_hLblTasksPrompt, step == WizardStep::Tasks);
    Show(g_hChkDesktop, step == WizardStep::Tasks);
    Show(g_hChkStartMenu, step == WizardStep::Tasks);
    Show(g_hChkAutoStart, step == WizardStep::Tasks);

    Show(g_hLblReadyPrompt, step == WizardStep::Ready);
    Show(g_hEditSummary, step == WizardStep::Ready);
    Show(g_hLblReadySub, step == WizardStep::Ready);

    Show(g_hLblInstalling, step == WizardStep::Installing);
    Show(g_hProgress, step == WizardStep::Installing);
    Show(g_hStatusText, step == WizardStep::Installing);

    Show(g_hLblFinishTitle, step == WizardStep::Finished);
    Show(g_hLblFinishDesc, step == WizardStep::Finished);
    Show(g_hChkLaunch, step == WizardStep::Finished);

    // Button states
    if (step == WizardStep::Maintenance) {
        Show(g_hBtnBack, FALSE);
        Show(g_hBtnNext, FALSE);
        Show(g_hBtnCancel, TRUE);
        EnableWindow(g_hBtnCancel, TRUE);
    } else if (step == WizardStep::Finished) {
        Show(g_hBtnBack, FALSE);
        Show(g_hBtnNext, TRUE);
        EnableWindow(g_hBtnNext, TRUE);
        Show(g_hBtnCancel, FALSE);
    } else if (step == WizardStep::Installing) {
        Show(g_hBtnBack, TRUE);
        EnableWindow(g_hBtnBack, FALSE);
        Show(g_hBtnNext, TRUE);
        EnableWindow(g_hBtnNext, FALSE);
        Show(g_hBtnCancel, TRUE);
        EnableWindow(g_hBtnCancel, FALSE);
    } else {
        Show(g_hBtnBack, TRUE);
        EnableWindow(g_hBtnBack, (step != WizardStep::Eula));
        Show(g_hBtnNext, TRUE);
        Show(g_hBtnCancel, TRUE);
        EnableWindow(g_hBtnCancel, TRUE);

        if (step == WizardStep::Eula) {
            bool accepted = (SendMessage(g_hRadioEulaAccept, BM_GETCHECK, 0, 0) == BST_CHECKED);
            EnableWindow(g_hBtnNext, accepted);
        } else {
            EnableWindow(g_hBtnNext, TRUE);
        }
    }

    EnableWindow(g_hBtnAbout, (step != WizardStep::Installing));
    UpdateWizardUI();

    if (g_hMainWnd) InvalidateRect(g_hMainWnd, NULL, TRUE);
}

static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            g_hMainWnd = hWnd;

            g_hFontTitle = CreateFontW(17, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            g_hFontSubtitle = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                          CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            g_hFontBold = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                      CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            g_hFontNormal = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            g_hFontSmall = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            g_hFontMono = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                      CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas");

            // Top Banner Text and Icon (Direct children of hWnd)
            g_hBannerTitle = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 20, 12, 440, 22, hWnd, NULL, g_hInstance, NULL);
            SendMessage(g_hBannerTitle, WM_SETFONT, (WPARAM)g_hFontTitle, TRUE);
            g_hBannerSubtitle = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 20, 36, 440, 24, hWnd, NULL, g_hInstance, NULL);
            SendMessage(g_hBannerSubtitle, WM_SETFONT, (WPARAM)g_hFontSubtitle, TRUE);

            g_hAppIcon = (HICON)LoadImageW(g_hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON, 36, 36, LR_DEFAULTCOLOR);
            g_hBannerIcon = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ICON, 480, 14, 36, 36, hWnd, NULL, g_hInstance, NULL);
            SendMessage(g_hBannerIcon, STM_SETICON, (WPARAM)g_hAppIcon, 0);

            // Bottom Navigation Buttons
            g_hBtnAbout = CreateWindowW(L"BUTTON", L"ℹ️ About...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 348, 115, 28, hWnd, (HMENU)IDC_BTN_ABOUT, g_hInstance, NULL);
            SendMessage(g_hBtnAbout, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

            g_hBtnBack = CreateWindowW(L"BUTTON", L"< Back", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 235, 348, 90, 28, hWnd, (HMENU)IDC_BTN_BACK, g_hInstance, NULL);
            SendMessage(g_hBtnBack, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

            g_hBtnNext = CreateWindowW(L"BUTTON", L"Next >", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 332, 348, 90, 28, hWnd, (HMENU)IDC_BTN_NEXT, g_hInstance, NULL);
            SendMessage(g_hBtnNext, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

            g_hBtnCancel = CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 429, 348, 90, 28, hWnd, (HMENU)IDC_BTN_CANCEL, g_hInstance, NULL);
            SendMessage(g_hBtnCancel, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

            // Step Maintenance Controls
            g_hLblMaintPrompt = CreateWindowW(L"STATIC", L"", WS_CHILD, 20, 78, 500, 36, hWnd, NULL, g_hInstance, NULL);
            SendMessage(g_hLblMaintPrompt, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

            g_hBtnMaintModify = CreateWindowW(L"BUTTON", L"1. Modify", WS_CHILD | BS_PUSHBUTTON | BS_MULTILINE, 20, 122, 145, 56, hWnd, (HMENU)IDC_BTN_MAINT_MODIFY, g_hInstance, NULL);
            SendMessage(g_hBtnMaintModify, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

            g_hLblMaintModifyDesc = CreateWindowW(L"STATIC", L"", WS_CHILD, 175, 124, 345, 52, hWnd, NULL, g_hInstance, NULL);
            SendMessage(g_hLblMaintModifyDesc, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

            g_hBtnMaintClean = CreateWindowW(L"BUTTON", L"2. Clean Install", WS_CHILD | BS_PUSHBUTTON | BS_MULTILINE, 20, 188, 145, 56, hWnd, (HMENU)IDC_BTN_MAINT_CLEAN, g_hInstance, NULL);
            SendMessage(g_hBtnMaintClean, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

            g_hLblMaintCleanDesc = CreateWindowW(L"STATIC", L"", WS_CHILD, 175, 190, 345, 52, hWnd, NULL, g_hInstance, NULL);
            SendMessage(g_hLblMaintCleanDesc, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

            g_hBtnMaintUninstall = CreateWindowW(L"BUTTON", L"3. Uninstall", WS_CHILD | BS_PUSHBUTTON | BS_MULTILINE, 20, 254, 145, 56, hWnd, (HMENU)IDC_BTN_MAINT_UNINST, g_hInstance, NULL);
            SendMessage(g_hBtnMaintUninstall, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

            g_hLblMaintUninstallDesc = CreateWindowW(L"STATIC", L"", WS_CHILD, 175, 256, 345, 52, hWnd, NULL, g_hInstance, NULL);
            SendMessage(g_hLblMaintUninstallDesc, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

            // Step 0: EULA Controls
            g_hLblEulaPrompt = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 20, 80, 500, 20, hWnd, NULL, g_hInstance, NULL);
            SendMessage(g_hLblEulaPrompt, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

            g_hEditEula = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", EULA_TEXT, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                                          20, 105, 500, 168, hWnd, (HMENU)IDC_EDIT_EULA, g_hInstance, NULL);
            SendMessage(g_hEditEula, WM_SETFONT, (WPARAM)g_hFontMono, TRUE);

            g_hRadioEulaAccept = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_TABSTOP, 20, 282, 500, 22, hWnd, (HMENU)IDC_RADIO_EULA_ACCEPT, g_hInstance, NULL);
            SendMessage(g_hRadioEulaAccept, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

            g_hRadioEulaDecline = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_TABSTOP, 20, 308, 500, 22, hWnd, (HMENU)IDC_RADIO_EULA_DECLINE, g_hInstance, NULL);
            SendMessage(g_hRadioEulaDecline, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
            SendMessage(g_hRadioEulaDecline, BM_SETCHECK, BST_CHECKED, 0);

            // Step 1: Scope Controls
            g_hLblScopePrompt = CreateWindowW(L"STATIC", L"", WS_CHILD, 20, 80, 500, 20, hWnd, NULL, g_hInstance, NULL);
            SendMessage(g_hLblScopePrompt, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

            g_hRadioScopeUser = CreateWindowW(L"BUTTON", L"", WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP, 20, 110, 500, 24, hWnd, (HMENU)IDC_RADIO_SCOPE_USER, g_hInstance, NULL);
            SendMessage(g_hRadioScopeUser, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);
            SendMessage(g_hRadioScopeUser, BM_SETCHECK, g_isAdminMode ? BST_UNCHECKED : BST_CHECKED, 0);

            g_hLblScopeUserDesc = CreateWindowW(L"STATIC", L"", WS_CHILD, 42, 136, 478, 38, hWnd, NULL, g_hInstance, NULL);
            SendMessage(g_hLblScopeUserDesc, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

            g_hRadioScopeAdmin = CreateWindowW(L"BUTTON", L"", WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP, 20, 185, 500, 24, hWnd, (HMENU)IDC_RADIO_SCOPE_ADMIN, g_hInstance, NULL);
            SendMessage(g_hRadioScopeAdmin, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);
            SendMessage(g_hRadioScopeAdmin, BM_SETCHECK, g_isAdminMode ? BST_CHECKED : BST_UNCHECKED, 0);

            g_hLblScopeAdminDesc = CreateWindowW(L"STATIC", L"", WS_CHILD, 42, 211, 478, 38, hWnd, NULL, g_hInstance, NULL);
            SendMessage(g_hLblScopeAdminDesc, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

            g_hLblScopeShield = CreateWindowW(L"STATIC", L"", WS_CHILD, 42, 258, 478, 24, hWnd, NULL, g_hInstance, NULL);
            SendMessage(g_hLblScopeShield, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

            // Step 2: Destination Controls
            g_hLblDestPrompt = CreateWindowW(L"STATIC", L"", WS_CHILD, 20, 90, 500, 20, hWnd, NULL, g_hInstance, NULL);
            SendMessage(g_hLblDestPrompt, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

            if (g_destDir.empty()) {
                g_destDir = g_isAdminMode ? GetAdminProgramsDirectory() : GetUserProgramsDirectory();
            }
            g_hEditDest = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", g_destDir.c_str(), WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
                                          20, 120, 395, 26, hWnd, (HMENU)IDC_EDIT_DEST, g_hInstance, NULL);
            SendMessage(g_hEditDest, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

            g_hBtnBrowse = CreateWindowW(L"BUTTON", L"", WS_CHILD | BS_PUSHBUTTON, 425, 119, 95, 28, hWnd, (HMENU)IDC_BTN_BROWSE, g_hInstance, NULL);
            SendMessage(g_hBtnBrowse, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

            g_hLblDestSpace = CreateWindowW(L"STATIC", L"", WS_CHILD, 20, 160, 500, 20, hWnd, NULL, g_hInstance, NULL);
            SendMessage(g_hLblDestSpace, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

            // Step 3: Tasks Controls
            g_hLblTasksPrompt = CreateWindowW(L"STATIC", L"", WS_CHILD, 20, 90, 500, 20, hWnd, NULL, g_hInstance, NULL);
            SendMessage(g_hLblTasksPrompt, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

            g_hChkDesktop = CreateWindowW(L"BUTTON", L"", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 20, 125, 500, 24, hWnd, (HMENU)IDC_CHK_DESKTOP, g_hInstance, NULL);
            SendMessage(g_hChkDesktop, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
            SendMessage(g_hChkDesktop, BM_SETCHECK, g_createDesktop ? BST_CHECKED : BST_UNCHECKED, 0);

            g_hChkStartMenu = CreateWindowW(L"BUTTON", L"", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 20, 155, 500, 24, hWnd, (HMENU)IDC_CHK_STARTMENU, g_hInstance, NULL);
            SendMessage(g_hChkStartMenu, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
            SendMessage(g_hChkStartMenu, BM_SETCHECK, g_createStartMenu ? BST_CHECKED : BST_UNCHECKED, 0);

            g_hChkAutoStart = CreateWindowW(L"BUTTON", L"", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 20, 185, 500, 24, hWnd, (HMENU)IDC_CHK_AUTOSTART, g_hInstance, NULL);
            SendMessage(g_hChkAutoStart, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
            SendMessage(g_hChkAutoStart, BM_SETCHECK, g_autoStart ? BST_CHECKED : BST_UNCHECKED, 0);

            // Step 4: Ready Controls
            g_hLblReadyPrompt = CreateWindowW(L"STATIC", L"", WS_CHILD, 20, 80, 500, 20, hWnd, NULL, g_hInstance, NULL);
            SendMessage(g_hLblReadyPrompt, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

            g_hEditSummary = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                                             20, 105, 500, 175, hWnd, (HMENU)IDC_EDIT_SUMMARY, g_hInstance, NULL);
            SendMessage(g_hEditSummary, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

            g_hLblReadySub = CreateWindowW(L"STATIC", L"", WS_CHILD, 20, 295, 500, 36, hWnd, NULL, g_hInstance, NULL);
            SendMessage(g_hLblReadySub, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

            // Step 5: Installing Controls
            g_hLblInstalling = CreateWindowW(L"STATIC", L"", WS_CHILD, 20, 100, 500, 20, hWnd, NULL, g_hInstance, NULL);
            SendMessage(g_hLblInstalling, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

            g_hProgress = CreateWindowExW(0, PROGRESS_CLASSW, NULL, WS_CHILD | PBS_SMOOTH, 20, 135, 500, 22, hWnd, (HMENU)IDC_PROGRESS, g_hInstance, NULL);

            g_hStatusText = CreateWindowW(L"STATIC", L"", WS_CHILD | SS_LEFTNOWORDWRAP, 20, 168, 500, 20, hWnd, (HMENU)IDC_STATUS_TEXT, g_hInstance, NULL);
            SendMessage(g_hStatusText, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

            // Step 6: Finished Controls
            g_hLblFinishTitle = CreateWindowW(L"STATIC", L"", WS_CHILD, 20, 100, 500, 30, hWnd, NULL, g_hInstance, NULL);
            SendMessage(g_hLblFinishTitle, WM_SETFONT, (WPARAM)g_hFontTitle, TRUE);

            g_hLblFinishDesc = CreateWindowW(L"STATIC", L"", WS_CHILD, 20, 140, 500, 80, hWnd, NULL, g_hInstance, NULL);
            SendMessage(g_hLblFinishDesc, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

            g_hChkLaunch = CreateWindowW(L"BUTTON", L"", WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 20, 240, 500, 24, hWnd, (HMENU)IDC_CHK_LAUNCH, g_hInstance, NULL);
            SendMessage(g_hChkLaunch, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);
            SendMessage(g_hChkLaunch, BM_SETCHECK, g_launchAfter ? BST_CHECKED : BST_UNCHECKED, 0);

            if (g_isElevated) {
                SetWizardStep(WizardStep::Installing);
                std::thread(DoInstallThread).detach();
            } else if (g_hasExistingInstall) {
                SetWizardStep(WizardStep::Maintenance);
            } else {
                SetWizardStep(WizardStep::Eula);
            }
            return 0;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            HWND hCtl = (HWND)lParam;
            if (hCtl == g_hBannerTitle || hCtl == g_hBannerSubtitle || hCtl == g_hBannerIcon) {
                SetBkMode(hdc, TRANSPARENT);
                SetBkColor(hdc, RGB(255, 255, 255));
                return (LRESULT)GetStockObject(WHITE_BRUSH);
            }
            if (hCtl == g_hLblScopeShield) {
                SetTextColor(hdc, RGB(0, 102, 204));
            }
            SetBkMode(hdc, TRANSPARENT);
            SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
            return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
        }

        case WM_CTLCOLORBTN: {
            HDC hdc = (HDC)wParam;
            SetBkMode(hdc, TRANSPARENT);
            SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
            return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
        }

        case WM_ERASEBKGND:
            return 1; // Handled in WM_PAINT to prevent flicker and color mismatches

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rcClient;
            GetClientRect(hWnd, &rcClient);

            // Fill banner area with white
            RECT rcBanner = { 0, 0, rcClient.right, 68 };
            FillRect(hdc, &rcBanner, (HBRUSH)GetStockObject(WHITE_BRUSH));

            // Fill body area with COLOR_BTNFACE
            RECT rcBody = { 0, 68, rcClient.right, rcClient.bottom };
            FillRect(hdc, &rcBody, GetSysColorBrush(COLOR_BTNFACE));

            // Draw separator line under banner (y = 68)
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(210, 210, 210));
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            MoveToEx(hdc, 0, 68, NULL);
            LineTo(hdc, rcClient.right, 68);

            // Draw separator line above buttons (y = 336)
            MoveToEx(hdc, 0, 336, NULL);
            LineTo(hdc, rcClient.right, 336);

            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);

            if (id == IDC_BTN_ABOUT) {
                AboutDialog::Show(hWnd);
                return 0;
            }

            if (id == IDC_BTN_MAINT_MODIFY) {
                g_destDir = g_existingDir;
                g_isAdminMode = g_existingIsAdmin;
                SetWindowTextW(g_hEditDest, g_destDir.c_str());
                SetWizardStep(WizardStep::Tasks);
                return 0;
            }

            if (id == IDC_BTN_MAINT_CLEAN) {
                int r = MessageBoxW(hWnd,
                    g_isThai ?
                    L"การติดตั้งใหม่แบบหมดจด (Clean Install) จะทำการลบไฟล์โปรแกรม ข้อมูลตารางเวลา และโฟลเดอร์พรีเซ็ตเสียง (Presets) เดิมทั้งหมดออกจากเครื่อง เสมือนถอนการติดตั้งแล้วเริ่มติดตั้งใหม่ตั้งแต่ต้น\n\nคุณต้องการดำเนินการต่อหรือไม่?" :
                    L"Clean Install will completely remove all existing application files, schedules, and presets as if performing a full uninstall, then begin a fresh installation.\n\nDo you want to proceed?",
                    L"Clean Install Confirmation", MB_YESNO | MB_ICONQUESTION);
                if (r == IDYES) {
                    if (IsAppRunning()) {
                        HWND hApp = FindWindowW(APP_WINDOW_CLASS_W, NULL);
                        if (hApp) PostMessageW(hApp, WM_CLOSE, 0, 0);
                        Sleep(600);
                    }
                    PerformCleanRemoval(g_existingDir, g_existingIsAdmin, false); // wipe everything!
                    g_hasExistingInstall = false;
                    SetWizardStep(WizardStep::Eula);
                }
                return 0;
            }

            if (id == IDC_BTN_MAINT_UNINST) {
                int r = MessageBoxW(hWnd,
                    g_isThai ?
                    L"คุณแน่ใจหรือไม่ว่าต้องการถอนการติดตั้ง Audio Schedule ออกจากคอมพิวเตอร์เครื่องนี้?" :
                    L"Are you sure you want to completely uninstall Audio Schedule from this computer?",
                    L"Uninstall Confirmation", MB_YESNO | MB_ICONQUESTION);
                if (r == IDYES) {
                    if (IsAppRunning()) {
                        HWND hApp = FindWindowW(APP_WINDOW_CLASS_W, NULL);
                        if (hApp) PostMessageW(hApp, WM_CLOSE, 0, 0);
                        Sleep(600);
                    }
                    PerformCleanRemoval(g_existingDir, g_existingIsAdmin);
                    MessageBoxW(hWnd,
                        g_isThai ?
                        L"Audio Schedule ได้รับการถอนการติดตั้งออกจากคอมพิวเตอร์เรียบร้อยแล้ว" :
                        L"Audio Schedule has been successfully uninstalled from your computer.",
                        L"Uninstall Complete", MB_OK | MB_ICONINFORMATION);
                    DestroyWindow(hWnd);
                }
                return 0;
            }

            if (id == IDC_RADIO_EULA_ACCEPT || id == IDC_RADIO_EULA_DECLINE) {
                bool accepted = (SendMessage(g_hRadioEulaAccept, BM_GETCHECK, 0, 0) == BST_CHECKED);
                EnableWindow(g_hBtnNext, accepted);
                return 0;
            }

            if (id == IDC_RADIO_SCOPE_USER) {
                g_isAdminMode = false;
                g_destDir = GetUserProgramsDirectory();
                SetWindowTextW(g_hEditDest, g_destDir.c_str());
                return 0;
            }

            if (id == IDC_RADIO_SCOPE_ADMIN) {
                g_isAdminMode = true;
                g_destDir = GetAdminProgramsDirectory();
                SetWindowTextW(g_hEditDest, g_destDir.c_str());
                return 0;
            }

            if (id == IDC_BTN_BROWSE) {
                BROWSEINFOW bi = { 0 };
                bi.hwndOwner = hWnd;
                bi.lpszTitle = g_isThai ? L"เลือกโฟลเดอร์สำหรับติดตั้ง Audio Schedule:" : L"Select Audio Schedule Destination Folder:";
                bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

                PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
                if (pidl) {
                    wchar_t selected[MAX_PATH] = { 0 };
                    if (SHGetPathFromIDListW(pidl, selected)) {
                        std::wstring path = selected;
                        if (path.back() != L'\\' && path.back() != L'/') {
                            path += L"\\AudioSchedule";
                        }
                        g_destDir = path;
                        SetWindowTextW(g_hEditDest, path.c_str());
                    }
                    CoTaskMemFree(pidl);
                }
                return 0;
            }

            if (id == IDC_BTN_BACK) {
                if (g_hasExistingInstall && g_step == WizardStep::Tasks) {
                    SetWizardStep(WizardStep::Maintenance);
                    return 0;
                }
                switch (g_step) {
                    case WizardStep::Scope:       SetWizardStep(WizardStep::Eula); break;
                    case WizardStep::Destination: SetWizardStep(WizardStep::Scope); break;
                    case WizardStep::Tasks:       SetWizardStep(WizardStep::Destination); break;
                    case WizardStep::Ready:       SetWizardStep(WizardStep::Tasks); break;
                    default: break;
                }
                return 0;
            }

            if (id == IDC_BTN_NEXT) {
                if (g_step == WizardStep::Finished) {
                    g_launchAfter = (SendMessage(g_hChkLaunch, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    if (g_launchAfter) {
                        std::wstring targetExe = g_destDir + L"\\" + APP_EXE_NAME_W;
                        ShellExecuteW(NULL, L"open", targetExe.c_str(), NULL, g_destDir.c_str(), SW_SHOWNORMAL);
                    }
                    DestroyWindow(hWnd);
                    return 0;
                }
                switch (g_step) {
                    case WizardStep::Eula:
                        SetWizardStep(WizardStep::Scope);
                        break;
                    case WizardStep::Scope:
                        g_isAdminMode = (SendMessage(g_hRadioScopeAdmin, BM_GETCHECK, 0, 0) == BST_CHECKED);
                        if (g_destDir.empty() || g_destDir == GetUserProgramsDirectory() || g_destDir == GetAdminProgramsDirectory()) {
                            g_destDir = g_isAdminMode ? GetAdminProgramsDirectory() : GetUserProgramsDirectory();
                            SetWindowTextW(g_hEditDest, g_destDir.c_str());
                        }
                        SetWizardStep(WizardStep::Destination);
                        break;
                    case WizardStep::Destination: {
                        wchar_t buf[MAX_PATH] = { 0 };
                        GetWindowTextW(g_hEditDest, buf, MAX_PATH);
                        g_destDir = buf;
                        while (!g_destDir.empty() && (g_destDir.back() == L'\\' || g_destDir.back() == L' ' || g_destDir.back() == L'/')) {
                            g_destDir.pop_back();
                        }
                        if (g_destDir.empty()) {
                            MessageBoxW(hWnd, g_isThai ? L"กรุณาระบุตำแหน่งโฟลเดอร์สำหรับติดตั้ง" : L"Please specify an installation folder.", L"Notice", MB_OK | MB_ICONWARNING);
                            return 0;
                        }
                        SetWizardStep(WizardStep::Tasks);
                        break;
                    }
                    case WizardStep::Tasks:
                        g_createDesktop = (SendMessage(g_hChkDesktop, BM_GETCHECK, 0, 0) == BST_CHECKED);
                        g_createStartMenu = (SendMessage(g_hChkStartMenu, BM_GETCHECK, 0, 0) == BST_CHECKED);
                        g_autoStart = (SendMessage(g_hChkAutoStart, BM_GETCHECK, 0, 0) == BST_CHECKED);
                        SetWizardStep(WizardStep::Ready);
                        break;
                    case WizardStep::Ready: {
                        if (IsAppRunning()) {
                            int r = MessageBoxW(hWnd,
                                g_isThai ?
                                L"ตรวจพบว่าโปรแกรม Audio Schedule กำลังเปิดทำงานอยู่\nคุณต้องการปิดโปรแกรมเพื่อดำเนินการติดตั้งต่อหรือไม่?" :
                                L"Audio Schedule is currently running.\nWould you like to close it to proceed with installation?",
                                L"Audio Schedule", MB_YESNO | MB_ICONQUESTION);
                            if (r == IDYES) {
                                HWND hApp = FindWindowW(APP_WINDOW_CLASS_W, NULL);
                                if (hApp) PostMessageW(hApp, WM_CLOSE, 0, 0);
                                Sleep(600);
                            } else {
                                return 0;
                            }
                        }

                        // Check elevation if Admin Mode was selected
                        if (g_isAdminMode && !IsRunAsAdmin()) {
                            wchar_t myPath[MAX_PATH] = { 0 };
                            GetModuleFileNameW(NULL, myPath, MAX_PATH);
                            std::wstring params = L"--elevated --mode admin --dest \"" + g_destDir + L"\""
                                + L" --desktop " + (g_createDesktop ? L"1" : L"0")
                                + L" --startmenu " + (g_createStartMenu ? L"1" : L"0")
                                + L" --autostart " + (g_autoStart ? L"1" : L"0")
                                + L" --launch " + (g_launchAfter ? L"1" : L"0");

                            SHELLEXECUTEINFOW sei = { sizeof(SHELLEXECUTEINFOW) };
                            sei.lpVerb = L"runas";
                            sei.lpFile = myPath;
                            sei.lpParameters = params.c_str();
                            sei.nShow = SW_SHOWNORMAL;
                            if (ShellExecuteExW(&sei)) {
                                DestroyWindow(hWnd);
                                return 0;
                            } else {
                                MessageBoxW(hWnd,
                                    g_isThai ?
                                    L"การติดตั้งใน Admin Mode จำเป็นต้องได้รับสิทธิ์ Administrator กรุณากด Yes บนหน้าต่างยืนยัน" :
                                    L"Admin Mode installation requires Administrator privileges. Please accept the UAC prompt.",
                                    L"Privileges Required", MB_OK | MB_ICONWARNING);
                                return 0;
                            }
                        }

                        SetWizardStep(WizardStep::Installing);
                        std::thread(DoInstallThread).detach();
                        break;
                    }
                    default: break;
                }
                return 0;
            }

            if (id == IDC_BTN_CANCEL) {
                int r = MessageBoxW(hWnd,
                    g_isThai ? L"คุณต้องการยกเลิกการติดตั้งโปรแกรมหรือไม่?" : L"Are you sure you want to exit Setup?",
                    L"Audio Schedule Setup", MB_YESNO | MB_ICONQUESTION);
                if (r == IDYES) {
                    DestroyWindow(hWnd);
                }
                return 0;
            }
            break;
        }

        case WM_APP + 1: { // Installation completed callback
            bool success = (wParam == 1);
            if (success) {
                SetWizardStep(WizardStep::Finished);
            } else {
                MessageBoxW(hWnd,
                    g_isThai ? L"การติดตั้งไม่สำเร็จ กรุณาลองใหม่อีกครั้ง" : L"Installation failed. Please try again.",
                    L"Error", MB_OK | MB_ICONERROR);
                SetWizardStep(WizardStep::Ready);
            }
            return 0;
        }

        case WM_CLOSE: {
            if (g_isInstalling) {
                MessageBoxW(hWnd,
                    g_isThai ? L"กำลังติดตั้งไฟล์ ไม่สามารถปิดหน้าต่างได้ในขณะนี้" : L"Installation is in progress. Please wait.",
                    L"Notice", MB_OK | MB_ICONWARNING);
                return 0;
            }
            if (g_step != WizardStep::Finished) {
                int r = MessageBoxW(hWnd,
                    g_isThai ? L"คุณต้องการยกเลิกการติดตั้งโปรแกรมหรือไม่?" : L"Are you sure you want to exit Setup?",
                    L"Audio Schedule Setup", MB_YESNO | MB_ICONQUESTION);
                if (r != IDYES) return 0;
            }
            DestroyWindow(hWnd);
            return 0;
        }

        case WM_DESTROY: {
            if (g_hFontTitle) DeleteObject(g_hFontTitle);
            if (g_hFontSubtitle) DeleteObject(g_hFontSubtitle);
            if (g_hFontBold) DeleteObject(g_hFontBold);
            if (g_hFontNormal) DeleteObject(g_hFontNormal);
            if (g_hFontSmall) DeleteObject(g_hFontSmall);
            if (g_hFontMono) DeleteObject(g_hFontMono);
            if (g_hAppIcon) DestroyIcon(g_hAppIcon);
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    Security::AntiDebug::Enforce();
    g_hInstance = hInstance;
    CoInitialize(NULL);

    // Detect system language
    if (GetUserDefaultUILanguage() == 0x041e) { // Thai
        g_isThai = true;
    }

    // Parse command line arguments
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int i = 1; i < argc; ++i) {
        if (_wcsicmp(argv[i], L"/S") == 0 || _wcsicmp(argv[i], L"--silent") == 0 || _wcsicmp(argv[i], L"-s") == 0) {
            g_isSilent = true;
        } else if (_wcsicmp(argv[i], L"--elevated") == 0) {
            g_isElevated = true;
        } else if (_wcsicmp(argv[i], L"--mode") == 0 && i + 1 < argc) {
            std::wstring m = argv[++i];
            g_isAdminMode = (_wcsicmp(m.c_str(), L"admin") == 0);
        } else if (_wcsicmp(argv[i], L"--dest") == 0 && i + 1 < argc) {
            g_destDir = argv[++i];
        } else if (_wcsicmp(argv[i], L"--desktop") == 0 && i + 1 < argc) {
            g_createDesktop = (_wcsicmp(argv[++i], L"1") == 0);
        } else if (_wcsicmp(argv[i], L"--startmenu") == 0 && i + 1 < argc) {
            g_createStartMenu = (_wcsicmp(argv[++i], L"1") == 0);
        } else if (_wcsicmp(argv[i], L"--autostart") == 0 && i + 1 < argc) {
            g_autoStart = (_wcsicmp(argv[++i], L"1") == 0);
        } else if (_wcsicmp(argv[i], L"--launch") == 0 && i + 1 < argc) {
            g_launchAfter = (_wcsicmp(argv[++i], L"1") == 0);
        }
    }
    LocalFree(argv);

    if (g_destDir.empty()) {
        g_destDir = g_isAdminMode ? GetAdminProgramsDirectory() : GetUserProgramsDirectory();
    }

    // Silent Install Execution
    if (g_isSilent) {
        bool ok = PerformInstallation(g_destDir, g_isAdminMode, g_createDesktop, g_createStartMenu, g_autoStart, false, true, nullptr);
        return ok ? 0 : 1;
    }

    // Detect existing installation
    if (!g_isElevated && !g_isSilent) {
        g_hasExistingInstall = DetectExistingInstallation(g_existingDir, g_existingIsAdmin, g_existingVersion);
        if (g_hasExistingInstall) {
            g_step = WizardStep::Maintenance;
        }
    }

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icex);

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"AudioSchedule_Installer_Wizard";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
    RegisterClassExW(&wc);

    int winW = 546;
    int winH = 425;
    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (scrW - winW) / 2;
    int posY = (scrH - winH) / 2;

    HWND hWnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        APP_NAME_W L" Setup",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        posX, posY, winW, winH,
        NULL, NULL, hInstance, NULL
    );

    if (!hWnd) return 1;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CoUninitialize();
    return (int)msg.wParam;
}
