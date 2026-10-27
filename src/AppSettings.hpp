#pragma once
#include <windows.h>
#include <shlobj.h>
#include <string>
#include <filesystem>
#include "Security.hpp"
#include "Language.h"

enum class OverlapMode {
    Queue = 0,         // Default: sequential queue
    Simultaneous = 1   // Multi-channel concurrent playback
};

class AppSettings {
public:
    static inline OverlapMode overlapMode = OverlapMode::Queue;
    static std::wstring GetSettingsFilePath() {
        wchar_t appPath[MAX_PATH] = { 0 };
        GetModuleFileNameW(NULL, appPath, MAX_PATH);
        std::wstring dir = appPath;
        size_t pos = dir.find_last_of(L"\\/");
        if (pos != std::wstring::npos) dir = dir.substr(0, pos);

        std::wstring localIni = dir + L"\\settings.ini";

        // Test if writable
        HANDLE hFile = CreateFileW(localIni.c_str(), GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(hFile);
            return localIni;
        }

        // Fallback to %LocalAppData%\AudioSchedule
        wchar_t szAppData[MAX_PATH] = { 0 };
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, szAppData))) {
            std::wstring userDir = std::wstring(szAppData) + L"\\AudioSchedule";
            std::error_code ec;
            std::filesystem::create_directories(userDir, ec);
            return userDir + L"\\settings.ini";
        }
        return localIni;
    }

    static void Load() {
        std::wstring ini = GetSettingsFilePath();
        wchar_t langBuf[32] = { 0 };
        GetPrivateProfileStringW(L"General", L"Language", L"en", langBuf, 32, ini.c_str());
        if (_wcsicmp(langBuf, L"th") == 0) {
            Lang::current = Language::Thai;
        } else {
            Lang::current = Language::English; // Default to English
        }

        int modeVal = (int)GetPrivateProfileIntW(L"Audio", L"OverlapMode", 0, ini.c_str());
        overlapMode = (modeVal == 1) ? OverlapMode::Simultaneous : OverlapMode::Queue;
    }

    static void Save() {
        std::wstring ini = GetSettingsFilePath();
        const wchar_t* langCode = (Lang::current == Language::Thai) ? L"th" : L"en";
        WritePrivateProfileStringW(L"General", L"Language", langCode, ini.c_str());
        WritePrivateProfileStringW(L"Audio", L"OverlapMode", (overlapMode == OverlapMode::Simultaneous) ? L"1" : L"0", ini.c_str());
    }

    static bool IsAutoStartEnabled() {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, CRYPT_W(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run").c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            wchar_t val[MAX_PATH] = { 0 };
            DWORD dwSize = sizeof(val);
            DWORD dwType = 0;
            LSTATUS st = RegQueryValueExW(hKey, CRYPT_W(L"AudioSchedule").c_str(), NULL, &dwType, (LPBYTE)val, &dwSize);
            RegCloseKey(hKey);
            return (st == ERROR_SUCCESS && dwSize > 0);
        }
        return false;
    }

    static bool SetAutoStart(bool enable) {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, CRYPT_W(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run").c_str(), 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            if (enable) {
                wchar_t exePath[MAX_PATH] = { 0 };
                GetModuleFileNameW(NULL, exePath, MAX_PATH);
                std::wstring quoted = L"\"" + std::wstring(exePath) + L"\"";
                RegSetValueExW(hKey, CRYPT_W(L"AudioSchedule").c_str(), 0, REG_SZ, (const BYTE*)quoted.c_str(), (DWORD)((quoted.size() + 1) * sizeof(wchar_t)));
            } else {
                RegDeleteValueW(hKey, CRYPT_W(L"AudioSchedule").c_str());
            }
            RegCloseKey(hKey);
            return true;
        }
        return false;
    }
};
