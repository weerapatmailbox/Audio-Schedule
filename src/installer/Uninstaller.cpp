#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <filesystem>
#include <thread>
#include <chrono>
#include "../Security.hpp"
#include "../Version.h"

namespace fs = std::filesystem;

#ifndef CSIDL_COMMON_PROGRAMS
#define CSIDL_COMMON_PROGRAMS 0x0017
#endif
#ifndef CSIDL_COMMON_DESKTOPDIRECTORY
#define CSIDL_COMMON_DESKTOPDIRECTORY 0x0019
#endif
#ifndef CSIDL_PROGRAM_FILESX86
#define CSIDL_PROGRAM_FILESX86 0x002a
#endif

static std::wstring GetExecutablePath() {
    wchar_t buffer[MAX_PATH] = { 0 };
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    return buffer;
}

static std::wstring GetParentDirectory(const std::wstring& path) {
    wchar_t buffer[MAX_PATH] = { 0 };
    wcscpy_s(buffer, path.c_str());
    PathRemoveFileSpecW(buffer);
    return buffer;
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
    return (isAdmin != FALSE);
}

static bool RequiresAdmin(const std::wstring& targetDir) {
    if (targetDir.empty()) return false;
    std::wstring progFiles = GetSpecialFolderPath(CSIDL_PROGRAM_FILES);
    if (!progFiles.empty() && _wcsnicmp(targetDir.c_str(), progFiles.c_str(), progFiles.length()) == 0) {
        return true;
    }
    std::wstring progFilesX86 = GetSpecialFolderPath(CSIDL_PROGRAM_FILESX86);
    if (!progFilesX86.empty() && _wcsnicmp(targetDir.c_str(), progFilesX86.c_str(), progFilesX86.length()) == 0) {
        return true;
    }
    return false;
}

static bool IsProcessRunning() {
    HWND hWnd = FindWindowW(APP_WINDOW_CLASS_W, NULL);
    return (hWnd != NULL);
}

static void DeleteRegistryKeyTree(HKEY hRootKey, const std::wstring& subKey) {
    RegDeleteTreeW(hRootKey, subKey.c_str());
}

static void DeleteRegistryValue(HKEY hRootKey, const std::wstring& subKey, const std::wstring& valueName) {
    HKEY hKey = NULL;
    if (RegOpenKeyExW(hRootKey, subKey.c_str(), 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegDeleteValueW(hKey, valueName.c_str());
        RegCloseKey(hKey);
    }
}

static void CleanPathString(std::wstring& path) {
    while (!path.empty() && (path.front() == L'\"' || path.front() == L'\'')) {
        path.erase(0, 1);
    }
    while (!path.empty() && (path.back() == L'\"' || path.back() == L'\'' || path.back() == L'\\' || path.back() == L'/')) {
        path.pop_back();
    }
}

static void ForceDeleteDirectory(const std::wstring& dirPath) {
    std::error_code ec;
    fs::path p(dirPath);
    if (!fs::exists(p, ec)) return;

    for (const auto& entry : fs::recursive_directory_iterator(p, fs::directory_options::skip_permission_denied, ec)) {
        SetFileAttributesW(entry.path().c_str(), FILE_ATTRIBUTE_NORMAL);
    }
    fs::remove_all(p, ec);
    if (fs::exists(p, ec)) {
        std::wstring rdCmd = L"/c timeout /t 1 /nobreak >nul & rd /s /q \"" + dirPath + L"\"";
        ShellExecuteW(NULL, L"open", L"cmd.exe", rdCmd.c_str(), NULL, SW_HIDE);
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    Security::AntiDebug::Enforce();
    // Immediately set working directory to %TEMP% so installDir is never locked
    wchar_t tempPath[MAX_PATH] = { 0 };
    GetTempPathW(MAX_PATH, tempPath);
    SetCurrentDirectoryW(tempPath);

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    bool isDoUninstall = false;
    bool isSilent = false;
    std::wstring targetDir;

    for (int i = 1; i < argc; ++i) {
        if (_wcsicmp(argv[i], L"--do-uninstall") == 0 && (i + 1 < argc)) {
            isDoUninstall = true;
            targetDir = argv[++i];
        } else if (_wcsicmp(argv[i], L"/S") == 0 || _wcsicmp(argv[i], L"--silent") == 0 || _wcsicmp(argv[i], L"-s") == 0) {
            isSilent = true;
        }
    }
    LocalFree(argv);

    CleanPathString(targetDir);

    if (!isDoUninstall) {
        std::wstring currentExe = GetExecutablePath();
        std::wstring installDir = GetParentDirectory(currentExe);

        // Check if uninstallation from this directory requires admin privileges and elevate if needed
        if (RequiresAdmin(installDir) && !IsRunAsAdmin()) {
            SHELLEXECUTEINFOW sei = { sizeof(sei) };
            sei.lpVerb = L"runas";
            sei.lpFile = currentExe.c_str();
            sei.lpParameters = lpCmdLine;
            sei.nShow = SW_SHOWNORMAL;
            if (ShellExecuteExW(&sei)) {
                return 0;
            } else {
                return 1;
            }
        }

        // Step 1: Confirmation Dialog (if not silent)
        if (!isSilent) {
            int res = MessageBoxW(
                NULL,
                L"คุณแน่ใจหรือไม่ว่าต้องการถอนการติดตั้ง Audio Schedule ออกจากคอมพิวเตอร์ของคุณ?\n\nAre you sure you want to uninstall Audio Schedule from your computer?",
                L"Audio Schedule - Uninstall",
                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2
            );

            if (res != IDYES) {
                return 0;
            }
        }

        // Check if application is currently open
        if (IsProcessRunning()) {
            HWND hWnd = FindWindowW(APP_WINDOW_CLASS_W, NULL);
            if (hWnd) {
                PostMessageW(hWnd, WM_CLOSE, 0, 0);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (IsProcessRunning()) {
                if (!isSilent) {
                    MessageBoxW(
                        NULL,
                        L"กรุณาปิดโปรแกรม Audio Schedule ก่อนดำเนินการถอนการติดตั้ง\n\nPlease close Audio Schedule before proceeding.",
                        L"Audio Schedule - Uninstall",
                        MB_OK | MB_ICONWARNING
                    );
                }
                return 1;
            }
        }

        // Copy this uninstaller to %TEMP% to allow deleting the install directory
        std::wstring tempExe = std::wstring(tempPath) + L"AudioSchedule_Uninstall_Temp.exe";

        CopyFileW(currentExe.c_str(), tempExe.c_str(), FALSE);

        // Launch temp uninstaller with target directory parameter
        std::wstring cmdParams = L"--do-uninstall \"" + installDir + L"\"";
        if (isSilent) cmdParams += L" --silent";
        ShellExecuteW(NULL, IsRunAsAdmin() ? L"runas" : L"open", tempExe.c_str(), cmdParams.c_str(), tempPath, SW_SHOWNORMAL);
        return 0;
    }

    // Step 2: Perform Uninstall from temp location
    // If target requires admin and current temp process is not admin, elevate
    if (RequiresAdmin(targetDir) && !IsRunAsAdmin()) {
        wchar_t tempExe[MAX_PATH] = { 0 };
        GetModuleFileNameW(NULL, tempExe, MAX_PATH);
        std::wstring cmdParams = L"--do-uninstall \"" + targetDir + L"\"";
        if (isSilent) cmdParams += L" --silent";
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpVerb = L"runas";
        sei.lpFile = tempExe;
        sei.lpParameters = cmdParams.c_str();
        sei.nShow = SW_SHOWNORMAL;
        if (ShellExecuteExW(&sei)) {
            return 0;
        }
    }

    // Allow parent process to fully exit and release all handles
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));

    // 1. Remove Desktop Shortcut (Both User and All Users)
    std::vector<int> desktopCsidls = { CSIDL_DESKTOPDIRECTORY, CSIDL_COMMON_DESKTOPDIRECTORY };
    for (int csidl : desktopCsidls) {
        std::wstring desktop = GetSpecialFolderPath(csidl);
        if (!desktop.empty()) {
            std::wstring shortcut = desktop + L"\\" + APP_NAME_W + L".lnk";
            DeleteFileW(shortcut.c_str());
        }
    }

    // 2. Remove Start Menu Shortcut & Folder (Both User and All Users)
    std::vector<int> smCsidls = { CSIDL_PROGRAMS, CSIDL_COMMON_PROGRAMS };
    for (int csidl : smCsidls) {
        std::wstring startMenu = GetSpecialFolderPath(csidl);
        if (!startMenu.empty()) {
            std::wstring smFolder = startMenu + L"\\" + APP_NAME_W;
            ForceDeleteDirectory(smFolder);
        }
    }

    // 3. Remove Windows Registry Uninstall Entries (Both HKCU and HKLM)
    std::wstring regKey = CRYPT_W(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\AudioSchedule");
    DeleteRegistryKeyTree(HKEY_CURRENT_USER, regKey);
    DeleteRegistryKeyTree(HKEY_LOCAL_MACHINE, regKey);

    // 4. Remove Auto-start Startup entries (Both HKCU and HKLM)
    std::wstring runKey = CRYPT_W(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run");
    DeleteRegistryValue(HKEY_CURRENT_USER, runKey, APP_NAME_W);
    DeleteRegistryValue(HKEY_LOCAL_MACHINE, runKey, APP_NAME_W);

    // 5. Remove Installed Files & Directory
    if (!targetDir.empty()) {
        ForceDeleteDirectory(targetDir);
    }

    // Step 3: Success Message (if not silent)
    if (!isSilent) {
        MessageBoxW(
            NULL,
            L"การถอนการติดตั้ง Audio Schedule เสร็จสมบูรณ์เรียบร้อยแล้ว\n\nAudio Schedule was successfully removed from your computer.",
            L"Audio Schedule - Uninstall",
            MB_OK | MB_ICONINFORMATION
        );
    }

    // Schedule temp exe self-deletion
    wchar_t tempExe[MAX_PATH] = { 0 };
    GetModuleFileNameW(NULL, tempExe, MAX_PATH);
    std::wstring cleanupCmd = L"/c timeout /t 1 /nobreak >nul & del /f /q \"" + std::wstring(tempExe) + L"\"";
    ShellExecuteW(NULL, L"open", L"cmd.exe", cleanupCmd.c_str(), NULL, SW_HIDE);

    return 0;
}
