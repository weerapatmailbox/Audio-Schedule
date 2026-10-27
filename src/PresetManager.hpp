#pragma once
#include "ScheduleGroup.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <windows.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <shlobj.h>

class PresetManager {
public:
    static std::wstring GetAppDirectory() {
        wchar_t szPath[MAX_PATH] = { 0 };
        GetModuleFileNameW(NULL, szPath, MAX_PATH);
        wchar_t* p = wcsrchr(szPath, L'\\');
        if (p) *p = L'\0';
        return szPath;
    }

    static std::wstring GetPresetsRootDirectory() {
        std::wstring dir = GetAppDirectory() + L"\\presets";
        if (!PathFileExistsW(dir.c_str())) {
            CreateDirectoryW(dir.c_str(), NULL);
        }
        return dir;
    }

    static std::string WideToUtf8(const std::wstring& wstr) {
        if (wstr.empty()) return "";
        int size = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string str(size, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &str[0], size, NULL, NULL);
        return str;
    }

    static std::wstring Utf8ToWide(const std::string& str) {
        if (str.empty()) return L"";
        int size = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0);
        std::wstring wstr(size, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &wstr[0], size);
        return wstr;
    }

    static std::string EscapeJsonString(const std::string& s) {
        std::ostringstream o;
        for (char c : s) {
            switch (c) {
                case '"': o << "\\\""; break;
                case '\\': o << "\\\\"; break;
                case '\b': o << "\\b"; break;
                case '\f': o << "\\f"; break;
                case '\n': o << "\\n"; break;
                case '\r': o << "\\r"; break;
                case '\t': o << "\\t"; break;
                default:
                    o << c;
                    break;
            }
        }
        return o.str();
    }

    static bool SavePresetPackage(const std::wstring& presetName,
                                  std::vector<ScheduleGroup>& inOutItems,
                                  bool copyAudioFiles,
                                  std::wstring& outFolder,
                                  int& outCopiedCount) {
        outCopiedCount = 0;
        if (presetName.empty()) return false;

        // Clean presetName of invalid filesystem characters
        std::wstring safeName;
        for (wchar_t c : presetName) {
            if (c == L'\\' || c == L'/' || c == L':' || c == L'*' || c == L'?' || c == L'"' || c == L'<' || c == L'>' || c == L'|') {
                safeName += L'_';
            } else {
                safeName += c;
            }
        }

        std::wstring presetsRoot = GetPresetsRootDirectory();
        std::wstring presetFolder = presetsRoot + L"\\" + safeName;
        if (!PathFileExistsW(presetFolder.c_str())) {
            CreateDirectoryW(presetFolder.c_str(), NULL);
        }

        std::wstring audioFolder = presetFolder + L"\\audio";
        if (copyAudioFiles && !PathFileExistsW(audioFolder.c_str())) {
            CreateDirectoryW(audioFolder.c_str(), NULL);
        }

        outFolder = presetFolder;

        // Prepare copy and relative path replacement
        std::vector<ScheduleGroup> itemsToSave = inOutItems;

        if (copyAudioFiles) {
            for (size_t g = 0; g < itemsToSave.size(); ++g) {
                for (size_t f = 0; f < itemsToSave[g].audioFiles.size(); ++f) {
                    const std::wstring& srcPath = inOutItems[g].audioFiles[f];
                    if (srcPath.empty() || !PathFileExistsW(srcPath.c_str())) continue;

                    // Extract filename
                    const wchar_t* pName = wcsrchr(srcPath.c_str(), L'\\');
                    std::wstring baseFileName = pName ? (pName + 1) : srcPath;

                    // Separate stem and extension
                    std::wstring stem = baseFileName;
                    std::wstring ext;
                    size_t dotPos = baseFileName.rfind(L'.');
                    if (dotPos != std::string::npos) {
                        stem = baseFileName.substr(0, dotPos);
                        ext = baseFileName.substr(dotPos);
                    }

                    // Check if already in the target audio folder
                    std::wstring targetFullPath = audioFolder + L"\\" + baseFileName;
                    std::wstring relPath = L"audio\\" + baseFileName;

                    int counter = 1;
                    while (PathFileExistsW(targetFullPath.c_str())) {
                        if (_wcsicmp(srcPath.c_str(), targetFullPath.c_str()) == 0) {
                            // File is identical / already in target
                            break;
                        }
                        // Disambiguate duplicate names
                        std::wstring newName = stem + L"_" + std::to_wstring(counter++) + ext;
                        targetFullPath = audioFolder + L"\\" + newName;
                        relPath = L"audio\\" + newName;
                    }

                    // Copy file if not the same
                    if (_wcsicmp(srcPath.c_str(), targetFullPath.c_str()) != 0) {
                        if (CopyFileW(srcPath.c_str(), targetFullPath.c_str(), FALSE)) {
                            outCopiedCount++;
                        }
                    }

                    // Update path for saving (relative to preset folder)
                    itemsToSave[g].audioFiles[f] = relPath;

                    // Update in-memory item to newly copied local file
                    inOutItems[g].audioFiles[f] = targetFullPath;
                }
            }
        }

        std::wstring jsonPath = presetFolder + L"\\preset.json";
        bool ok = SaveToFile(jsonPath, itemsToSave);

        // Delete any legacy duplicate <safeName>.json if exists
        std::wstring legacyJson = presetFolder + L"\\" + safeName + L".json";
        if (_wcsicmp(legacyJson.c_str(), jsonPath.c_str()) != 0 && PathFileExistsW(legacyJson.c_str())) {
            DeleteFileW(legacyJson.c_str());
        }

        return ok;
    }

    static bool SaveToFile(const std::wstring& filePath, const std::vector<ScheduleGroup>& items) {
        std::ofstream file(filePath.c_str(), std::ios::binary);
        if (!file.is_open()) return false;

        std::ostringstream ss;
        ss << "[\n";
        for (size_t i = 0; i < items.size(); ++i) {
            const auto& item = items[i];
            ss << "  {\n";
            ss << "    \"id\": " << item.id << ",\n";
            ss << "    \"name\": \"" << EscapeJsonString(WideToUtf8(item.name)) << "\",\n";
            ss << "    \"hour\": " << item.hour << ",\n";
            ss << "    \"minute\": " << item.minute << ",\n";
            ss << "    \"second\": " << item.second << ",\n";
            ss << "    \"daysMask\": " << (int)item.daysMask << ",\n";
            ss << "    \"enabled\": " << (item.enabled ? "true" : "false") << ",\n";
            ss << "    \"audioFiles\": [\n";
            for (size_t f = 0; f < item.audioFiles.size(); ++f) {
                ss << "      \"" << EscapeJsonString(WideToUtf8(item.audioFiles[f])) << "\""
                   << (f + 1 < item.audioFiles.size() ? ",\n" : "\n");
            }
            ss << "    ]\n";
            ss << "  }" << (i + 1 < items.size() ? ",\n" : "\n");
        }
        ss << "]\n";

        std::string content = ss.str();
        file.write(content.data(), content.size());
        return true;
    }

    static bool LoadFromFile(const std::wstring& filePath, std::vector<ScheduleGroup>& items) {
        std::ifstream file(filePath.c_str(), std::ios::binary);
        if (!file.is_open()) return false;

        std::string content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

        // Extract directory of the json file to resolve relative audio paths
        wchar_t fileDir[MAX_PATH] = { 0 };
        wcscpy_s(fileDir, filePath.c_str());
        PathRemoveFileSpecW(fileDir);
        std::wstring baseDir = fileDir;

        items.clear();

        size_t pos = 0;
        int nextId = 1;
        while ((pos = content.find('{', pos)) != std::string::npos) {
            size_t endObj = content.find('}', pos);
            size_t arrPos = content.find("\"audioFiles\"", pos);
            if (arrPos != std::string::npos && arrPos < endObj) {
                size_t arrEnd = content.find(']', arrPos);
                if (arrEnd != std::string::npos) {
                    endObj = content.find('}', arrEnd);
                }
            }
            if (endObj == std::string::npos) break;

            std::string objStr = content.substr(pos, endObj - pos + 1);
            ScheduleGroup item;
            item.id = nextId++;

            ExtractInt(objStr, "id", item.id);
            std::string nameStr;
            if (ExtractString(objStr, "name", nameStr)) {
                item.name = Utf8ToWide(nameStr);
            }
            ExtractInt(objStr, "hour", item.hour);
            ExtractInt(objStr, "minute", item.minute);
            ExtractInt(objStr, "second", item.second);
            int dm = 0x7F;
            if (ExtractInt(objStr, "daysMask", dm)) {
                item.daysMask = (uint8_t)dm;
            }
            bool en = true;
            if (ExtractBool(objStr, "enabled", en)) {
                item.enabled = en;
            }

            // Extract audioFiles array
            std::vector<std::string> fileList;
            if (ExtractStringArray(objStr, "audioFiles", fileList)) {
                for (const auto& f : fileList) {
                    std::wstring rawPath = Utf8ToWide(f);
                    std::wstring resolvedPath = ResolveAudioPath(baseDir, rawPath);
                    item.audioFiles.push_back(resolvedPath);
                }
            } else {
                // Backward compatibility: single audioPath
                std::string singleAudio;
                if (ExtractString(objStr, "audioPath", singleAudio) && !singleAudio.empty()) {
                    std::wstring rawPath = Utf8ToWide(singleAudio);
                    std::wstring resolvedPath = ResolveAudioPath(baseDir, rawPath);
                    item.audioFiles.push_back(resolvedPath);
                }
            }

            items.push_back(item);
            pos = endObj + 1;
        }

        return true;
    }

private:
    static std::wstring ResolveAudioPath(const std::wstring& baseDir, const std::wstring& rawPath) {
        if (rawPath.empty()) return L"";

        // Check if rawPath is relative
        if (PathIsRelativeW(rawPath.c_str())) {
            wchar_t combined[MAX_PATH] = { 0 };
            PathCombineW(combined, baseDir.c_str(), rawPath.c_str());
            return combined;
        }

        // If rawPath is absolute and exists, use it
        if (PathFileExistsW(rawPath.c_str())) {
            return rawPath;
        }

        // If absolute doesn't exist, check inside baseDir\audio\filename
        const wchar_t* pFn = wcsrchr(rawPath.c_str(), L'\\');
        std::wstring fileName = pFn ? (pFn + 1) : rawPath;

        std::wstring localAudio = baseDir + L"\\audio\\" + fileName;
        if (PathFileExistsW(localAudio.c_str())) {
            return localAudio;
        }

        std::wstring localRoot = baseDir + L"\\" + fileName;
        if (PathFileExistsW(localRoot.c_str())) {
            return localRoot;
        }

        return rawPath;
    }

    static bool ExtractString(const std::string& src, const std::string& key, std::string& outVal) {
        std::string pattern = "\"" + key + "\"";
        size_t p = src.find(pattern);
        if (p == std::string::npos) return false;
        p = src.find(':', p);
        if (p == std::string::npos) return false;
        p = src.find('"', p);
        if (p == std::string::npos) return false;
        size_t start = p + 1;
        std::string val;
        for (size_t i = start; i < src.size(); ++i) {
            if (src[i] == '\\' && i + 1 < src.size()) {
                val += src[i + 1];
                i++;
            } else if (src[i] == '"') {
                outVal = val;
                return true;
            } else {
                val += src[i];
            }
        }
        return false;
    }

    static bool ExtractStringArray(const std::string& src, const std::string& key, std::vector<std::string>& outList) {
        std::string pattern = "\"" + key + "\"";
        size_t p = src.find(pattern);
        if (p == std::string::npos) return false;
        p = src.find('[', p);
        if (p == std::string::npos) return false;
        size_t endArr = src.find(']', p);
        if (endArr == std::string::npos) return false;

        std::string arrSub = src.substr(p + 1, endArr - p - 1);
        size_t i = 0;
        while ((i = arrSub.find('"', i)) != std::string::npos) {
            size_t start = i + 1;
            std::string item;
            for (size_t k = start; k < arrSub.size(); ++k) {
                if (arrSub[k] == '\\' && k + 1 < arrSub.size()) {
                    item += arrSub[k + 1];
                    k++;
                } else if (arrSub[k] == '"') {
                    outList.push_back(item);
                    i = k + 1;
                    break;
                } else {
                    item += arrSub[k];
                }
            }
        }
        return true;
    }

    static bool ExtractInt(const std::string& src, const std::string& key, int& outVal) {
        std::string pattern = "\"" + key + "\"";
        size_t p = src.find(pattern);
        if (p == std::string::npos) return false;
        p = src.find(':', p);
        if (p == std::string::npos) return false;
        p++;
        while (p < src.size() && (src[p] == ' ' || src[p] == '\t' || src[p] == '\r' || src[p] == '\n')) p++;
        size_t end = p;
        while (end < src.size() && (isdigit((unsigned char)src[end]) || src[end] == '-')) end++;
        if (end > p) {
            outVal = std::stoi(src.substr(p, end - p));
            return true;
        }
        return false;
    }

    static bool ExtractBool(const std::string& src, const std::string& key, bool& outVal) {
        std::string pattern = "\"" + key + "\"";
        size_t p = src.find(pattern);
        if (p == std::string::npos) return false;
        p = src.find(':', p);
        if (p == std::string::npos) return false;
        if (src.find("true", p) != std::string::npos && src.find("true", p) < src.find('\n', p)) {
            outVal = true;
            return true;
        }
        if (src.find("false", p) != std::string::npos && src.find("false", p) < src.find('\n', p)) {
            outVal = false;
            return true;
        }
        return false;
    }

public:
    static std::wstring GetDirectoryOf(const std::wstring& filePath) {
        if (filePath.empty()) return L"";
        const wchar_t* p = wcsrchr(filePath.c_str(), L'\\');
        if (p) {
            return filePath.substr(0, p - filePath.c_str());
        }
        return L"";
    }

    struct PresetDeleteInfo {
        std::wstring presetName;
        std::wstring presetFolder;
        std::vector<std::wstring> filesInPreset;    // Located inside preset folder (will be deleted)
        std::vector<std::wstring> filesLinkedSafe;  // External linked files (will NOT be deleted)
        bool exists = false;
    };

    static PresetDeleteInfo AnalyzePresetForDeletion(const std::wstring& presetName) {
        PresetDeleteInfo info;
        info.presetName = presetName;
        info.presetFolder = GetPresetsRootDirectory() + L"\\" + presetName;
        info.exists = PathFileExistsW(info.presetFolder.c_str()) ? true : false;

        // Collect all actual files in preset's audio folder
        std::wstring audioDir = info.presetFolder + L"\\audio";
        if (PathFileExistsW(audioDir.c_str())) {
            WIN32_FIND_DATAW ffd;
            HANDLE hFind = FindFirstFileW((audioDir + L"\\*").c_str(), &ffd);
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        info.filesInPreset.push_back(audioDir + L"\\" + ffd.cFileName);
                    }
                } while (FindNextFileW(hFind, &ffd));
                FindClose(hFind);
            }
        }

        // Also inspect the JSON to identify linked external files vs files inside preset
        std::wstring jsonPath = info.presetFolder + L"\\" + presetName + L".json";
        if (!PathFileExistsW(jsonPath.c_str())) {
            jsonPath = info.presetFolder + L"\\preset.json";
        }
        std::vector<ScheduleGroup> items;
        if (LoadFromFile(jsonPath, items)) {
            for (const auto& grp : items) {
                for (const auto& f : grp.audioFiles) {
                    if (f.empty()) continue;

                    bool isInside = false;
                    if (f.find(L":") == std::wstring::npos) {
                        isInside = true;
                    } else if (_wcsnicmp(f.c_str(), info.presetFolder.c_str(), info.presetFolder.length()) == 0) {
                        isInside = true;
                    }

                    if (!isInside) {
                        bool already = false;
                        for (const auto& s : info.filesLinkedSafe) {
                            if (_wcsicmp(s.c_str(), f.c_str()) == 0) { already = true; break; }
                        }
                        if (!already) info.filesLinkedSafe.push_back(f);
                    } else {
                        // Ensure it's in filesInPreset if exists on disk
                        std::wstring fullPath = f;
                        if (f.find(L":") == std::wstring::npos) {
                            fullPath = info.presetFolder + L"\\" + f;
                        }
                        bool already = false;
                        for (const auto& s : info.filesInPreset) {
                            if (_wcsicmp(s.c_str(), fullPath.c_str()) == 0) { already = true; break; }
                        }
                        if (!already && PathFileExistsW(fullPath.c_str())) {
                            info.filesInPreset.push_back(fullPath);
                        }
                    }
                }
            }
        }
        return info;
    }

    static std::wstring GetActivePresetFilePath() {
        return GetPresetsRootDirectory() + L"\\active_preset.txt";
    }

    static std::wstring GetLastActivePreset() {
        std::wstring cfg = GetActivePresetFilePath();
        std::ifstream in(cfg.c_str(), std::ios::binary);
        if (in.is_open()) {
            std::string utf8Line;
            std::getline(in, utf8Line);
            std::wstring name = Utf8ToWide(utf8Line);
            size_t start = name.find_first_not_of(L" \t\r\n");
            size_t end = name.find_last_not_of(L" \t\r\n");
            if (start != std::wstring::npos && end != std::wstring::npos) {
                return name.substr(start, end - start + 1);
            }
        }
        return L"";
    }

    static void SetLastActivePreset(const std::wstring& presetName) {
        std::wstring cfg = GetActivePresetFilePath();
        if (presetName.empty()) {
            DeleteFileW(cfg.c_str());
            return;
        }
        std::ofstream out(cfg.c_str(), std::ios::binary);
        if (out.is_open()) {
            std::string utf8 = WideToUtf8(presetName);
            out.write(utf8.data(), utf8.size());
        }
    }

    static bool DeletePresetFolder(const std::wstring& presetFolder) {
        if (presetFolder.empty() || !PathFileExistsW(presetFolder.c_str())) return false;

        std::error_code ec;
        std::filesystem::remove_all(std::filesystem::path(presetFolder), ec);
        if (!PathFileExistsW(presetFolder.c_str())) return true;

        wchar_t szPath[MAX_PATH + 2] = { 0 };
        wcsncpy_s(szPath, presetFolder.c_str(), MAX_PATH);
        szPath[presetFolder.length() + 1] = L'\0'; // Double NULL for SHFILEOPSTRUCT

        SHFILEOPSTRUCTW sfos{};
        sfos.wFunc = FO_DELETE;
        sfos.pFrom = szPath;
        sfos.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
        return (SHFileOperationW(&sfos) == 0);
    }

    // ================= Portable Preset Export & Import =================
    static bool BrowseForFolder(HWND hwndOwner, const wchar_t* title, std::wstring& outPath) {
        BROWSEINFOW bi{};
        bi.hwndOwner = hwndOwner;
        bi.lpszTitle = title;
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_USENEWUI;
        LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
        if (pidl != 0) {
            wchar_t path[MAX_PATH] = { 0 };
            if (SHGetPathFromIDListW(pidl, path)) {
                outPath = path;
                CoTaskMemFree(pidl);
                return true;
            }
            CoTaskMemFree(pidl);
        }
        return false;
    }

    static bool ExportPortablePreset(const std::wstring& sourcePresetName,
                                    const std::vector<ScheduleGroup>& groups,
                                    const std::wstring& targetDirectory,
                                    std::wstring& outExportedPath,
                                    int& outCopiedCount) {
        outCopiedCount = 0;
        if (sourcePresetName.empty() || targetDirectory.empty()) return false;

        std::wstring safeName;
        for (wchar_t c : sourcePresetName) {
            if (c == L'\\' || c == L'/' || c == L':' || c == L'*' || c == L'?' || c == L'"' || c == L'<' || c == L'>' || c == L'|') {
                safeName += L'_';
            } else {
                safeName += c;
            }
        }
        if (safeName.empty()) safeName = L"Portable_Preset";

        std::wstring packageDir = targetDirectory + L"\\" + safeName;
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(packageDir), ec);
        std::wstring audioFolder = packageDir + L"\\audio";
        std::filesystem::create_directories(std::filesystem::path(audioFolder), ec);

        outExportedPath = packageDir;

        std::vector<ScheduleGroup> portableItems = groups;
        for (size_t g = 0; g < portableItems.size(); ++g) {
            for (size_t f = 0; f < portableItems[g].audioFiles.size(); ++f) {
                const std::wstring& srcPath = groups[g].audioFiles[f];
                if (srcPath.empty() || !PathFileExistsW(srcPath.c_str())) continue;

                const wchar_t* pName = wcsrchr(srcPath.c_str(), L'\\');
                std::wstring baseFileName = pName ? (pName + 1) : srcPath;

                std::wstring stem = baseFileName;
                std::wstring ext;
                size_t dotPos = baseFileName.rfind(L'.');
                if (dotPos != std::string::npos) {
                    stem = baseFileName.substr(0, dotPos);
                    ext = baseFileName.substr(dotPos);
                }

                std::wstring destFullPath = audioFolder + L"\\" + baseFileName;
                std::wstring relPath = L"audio\\" + baseFileName;

                int counter = 1;
                while (PathFileExistsW(destFullPath.c_str())) {
                    if (_wcsicmp(srcPath.c_str(), destFullPath.c_str()) == 0) {
                        break;
                    }
                    std::wstring newName = stem + L"_" + std::to_wstring(counter++) + ext;
                    destFullPath = audioFolder + L"\\" + newName;
                    relPath = L"audio\\" + newName;
                }

                if (_wcsicmp(srcPath.c_str(), destFullPath.c_str()) != 0) {
                    if (CopyFileW(srcPath.c_str(), destFullPath.c_str(), FALSE)) {
                        outCopiedCount++;
                    }
                } else {
                    outCopiedCount++;
                }

                portableItems[g].audioFiles[f] = relPath;
            }
        }

        std::wstring jsonPath = packageDir + L"\\" + safeName + L".json";
        bool ok = SaveToFile(jsonPath, portableItems);
        std::wstring aliasPath = packageDir + L"\\preset.json";
        SaveToFile(aliasPath, portableItems);

        std::wstring markerPath = packageDir + L"\\PORTABLE_PRESET.txt";
        std::ofstream marker(markerPath.c_str(), std::ios::binary);
        if (marker.is_open()) {
            std::ostringstream ss;
            ss << "Audio Time Player - Portable Preset Package\n";
            ss << "Preset Name: " << WideToUtf8(sourcePresetName) << "\n";
            ss << "Groups Count: " << portableItems.size() << "\n";
            ss << "Audio Files Count: " << outCopiedCount << "\n";
            ss << "Portable Relative Audio: Enabled (audio\\*)\n\n";
            ss << "วิธีนำเข้าชุดนี้ไปเปิดใช้งานบนเครื่องอื่น:\n";
            ss << "1. เปิดโปรแกรม Audio Time Player บนเครื่องปลายทาง\n";
            ss << "2. ไปที่เมนู: สำรองข้อมูล (Backup) -> นำเข้าชุดการเล่น Portable (Import Preset)\n";
            ss << "3. เลือกไฟล์ " << WideToUtf8(safeName) << ".json หรือโฟลเดอร์นี้เพื่อเริ่มใช้งานได้ทันที\n";
            std::string str = ss.str();
            marker.write(str.data(), str.size());
        }

        return ok;
    }

    static bool ImportPortablePreset(const std::wstring& sourceJsonOrFolderPath,
                                    const std::wstring& targetPresetNameOverride,
                                    std::wstring& outFinalPresetName,
                                    int& outGroupsCount,
                                    int& outAudioCount,
                                    std::wstring& outErrorMsg) {
        outGroupsCount = 0;
        outAudioCount = 0;

        if (sourceJsonOrFolderPath.empty() || !PathFileExistsW(sourceJsonOrFolderPath.c_str())) {
            outErrorMsg = L"ไม่พบไฟล์หรือโฟลเดอร์ที่ระบุ";
            return false;
        }

        std::wstring srcFolder;
        std::wstring srcJson;

        if (PathIsDirectoryW(sourceJsonOrFolderPath.c_str())) {
            srcFolder = sourceJsonOrFolderPath;
            WIN32_FIND_DATAW ffd;
            HANDLE hFind = FindFirstFileW((srcFolder + L"\\*.json").c_str(), &ffd);
            if (hFind != INVALID_HANDLE_VALUE) {
                std::wstring firstJson = srcFolder + L"\\" + ffd.cFileName;
                bool foundSpecific = false;
                do {
                    std::wstring curName = ffd.cFileName;
                    if (_wcsicmp(curName.c_str(), L"preset.json") == 0) {
                        srcJson = srcFolder + L"\\" + curName;
                        foundSpecific = true;
                        break;
                    }
                } while (FindNextFileW(hFind, &ffd));
                FindClose(hFind);

                if (!foundSpecific) {
                    srcJson = firstJson;
                }
            } else {
                outErrorMsg = L"ไม่พบไฟล์ข้อมูล .json ในโฟลเดอร์ที่เลือก";
                return false;
            }
        } else {
            srcJson = sourceJsonOrFolderPath;
            wchar_t dirBuf[MAX_PATH] = { 0 };
            wcscpy_s(dirBuf, srcJson.c_str());
            PathRemoveFileSpecW(dirBuf);
            srcFolder = dirBuf;
        }

        std::vector<ScheduleGroup> loaded;
        if (!LoadFromFile(srcJson, loaded)) {
            outErrorMsg = L"ไม่สามารถอ่านไฟล์ข้อมูลชุดการเล่นได้ หรือรูปแบบไฟล์ไม่ถูกต้อง";
            return false;
        }

        std::wstring presetName = targetPresetNameOverride;
        if (presetName.empty()) {
            const wchar_t* pFn = wcsrchr(srcJson.c_str(), L'\\');
            std::wstring jsonFileName = pFn ? (pFn + 1) : srcJson;
            size_t dot = jsonFileName.rfind(L'.');
            presetName = (dot != std::string::npos) ? jsonFileName.substr(0, dot) : jsonFileName;
            if (_wcsicmp(presetName.c_str(), L"preset") == 0) {
                const wchar_t* pDirName = wcsrchr(srcFolder.c_str(), L'\\');
                if (pDirName && wcslen(pDirName + 1) > 0) {
                    presetName = (pDirName + 1);
                }
            }
        }

        std::wstring safeName;
        for (wchar_t c : presetName) {
            if (c == L'\\' || c == L'/' || c == L':' || c == L'*' || c == L'?' || c == L'"' || c == L'<' || c == L'>' || c == L'|') {
                safeName += L'_';
            } else {
                safeName += c;
            }
        }
        if (safeName.empty()) safeName = L"Imported_Preset";

        std::wstring presetsRoot = GetPresetsRootDirectory();
        std::wstring targetFolder = presetsRoot + L"\\" + safeName;

        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(targetFolder), ec);
        std::wstring targetAudioDir = targetFolder + L"\\audio";
        std::filesystem::create_directories(std::filesystem::path(targetAudioDir), ec);

        // Copy audio directory if present
        std::wstring srcAudioDir = srcFolder + L"\\audio";
        if (PathFileExistsW(srcAudioDir.c_str())) {
            WIN32_FIND_DATAW ffd;
            HANDLE hFind = FindFirstFileW((srcAudioDir + L"\\*").c_str(), &ffd);
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        std::wstring srcFile = srcAudioDir + L"\\" + ffd.cFileName;
                        std::wstring dstFile = targetAudioDir + L"\\" + ffd.cFileName;
                        if (CopyFileW(srcFile.c_str(), dstFile.c_str(), FALSE)) {
                            outAudioCount++;
                        }
                    }
                } while (FindNextFileW(hFind, &ffd));
                FindClose(hFind);
            }
        }

        // Also check if any loose audio files in srcFolder need copying
        for (const auto& grp : loaded) {
            for (const auto& f : grp.audioFiles) {
                if (PathFileExistsW(f.c_str())) {
                    const wchar_t* fn = wcsrchr(f.c_str(), L'\\');
                    std::wstring baseName = fn ? (fn + 1) : f;
                    std::wstring dstFile = targetAudioDir + L"\\" + baseName;
                    if (!PathFileExistsW(dstFile.c_str())) {
                        if (CopyFileW(f.c_str(), dstFile.c_str(), FALSE)) {
                            outAudioCount++;
                        }
                    }
                }
            }
        }

        // Save target JSONs
        std::wstring targetJson = targetFolder + L"\\" + safeName + L".json";
        SaveToFile(targetJson, loaded);
        std::wstring targetAlias = targetFolder + L"\\preset.json";
        SaveToFile(targetAlias, loaded);

        outFinalPresetName = safeName;
        outGroupsCount = (int)loaded.size();
        return true;
    }

    // ================= ZIP Backup & Restore of All Portable Presets =================
    static std::wstring Base64EncodeUtf16(const std::wstring& str) {
        static const char b64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(str.data());
        size_t len = str.size() * sizeof(wchar_t);

        std::string b64;
        b64.reserve(((len + 2) / 3) * 4);
        for (size_t i = 0; i < len; i += 3) {
            uint32_t val = (bytes[i] << 16);
            if (i + 1 < len) val |= (bytes[i + 1] << 8);
            if (i + 2 < len) val |= bytes[i + 2];

            b64.push_back(b64Chars[(val >> 18) & 0x3F]);
            b64.push_back(b64Chars[(val >> 12) & 0x3F]);
            b64.push_back((i + 1 < len) ? b64Chars[(val >> 6) & 0x3F] : '=');
            b64.push_back((i + 2 < len) ? b64Chars[val & 0x3F] : '=');
        }
        return std::wstring(b64.begin(), b64.end());
    }

    static bool RunPowerShellScriptHidden(const std::wstring& script) {
        std::wstring b64 = Base64EncodeUtf16(script);
        std::wstring cmd = L"powershell.exe -NoProfile -NonInteractive -WindowStyle Hidden -EncodedCommand " + b64;

        STARTUPINFOW si = { sizeof(si) };
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        PROCESS_INFORMATION pi{};
        std::wstring cmdLine = cmd;
        if (!CreateProcessW(NULL, &cmdLine[0], NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            return false;
        }
        WaitForSingleObject(pi.hProcess, 120000);
        DWORD exitCode = 1;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return (exitCode == 0);
    }

    static bool CreateZipFromDirectory(const std::wstring& zipPath, const std::wstring& sourceDir) {
        if (PathFileExistsW(zipPath.c_str())) {
            DeleteFileW(zipPath.c_str());
        }

        // PowerShell Compress-Archive with -Path '<sourceDir>\*' puts .asch files at root (no '.' folder)
        // With Base64 -EncodedCommand, Thai Unicode names and paths are 100% preserved
        std::wstring psScript = L"$ProgressPreference = 'SilentlyContinue'\n"
                                L"Compress-Archive -Path '" + sourceDir + L"\\*' -DestinationPath '" + zipPath + L"' -Force\n";

        if (RunPowerShellScriptHidden(psScript) && PathFileExistsW(zipPath.c_str())) {
            return true;
        }
        return false;
    }

    static bool ExtractZipToDirectory(const std::wstring& zipPath, const std::wstring& targetDir) {
        if (!PathFileExistsW(zipPath.c_str())) return false;
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(targetDir), ec);

        std::wstring psScript = L"$ProgressPreference = 'SilentlyContinue'\n"
                                L"Expand-Archive -LiteralPath '" + zipPath + L"' -DestinationPath '" + targetDir + L"' -Force\n";

        return RunPowerShellScriptHidden(psScript);
    }

    static bool BackupAllToZip(const std::wstring& targetZipPath,
                               const std::wstring& activePresetName,
                               const std::vector<ScheduleGroup>& activePresetItems,
                               int& outPresetCount,
                               int& outTotalAudioCount,
                               std::wstring& outErrorMsg) {
        outPresetCount = 0;
        outTotalAudioCount = 0;

        if (targetZipPath.empty()) {
            outErrorMsg = L"ไม่ได้ระบุพาธไฟล์ ZIP ปลายทาง";
            return false;
        }

        wchar_t tempBase[MAX_PATH] = { 0 };
        GetTempPathW(MAX_PATH, tempBase);
        std::wstring stagingDir = std::wstring(tempBase) + L"AudioSchedule_Backup_" + std::to_wstring(GetTickCount());
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(stagingDir), ec);
        if (ec) {
            outErrorMsg = L"ไม่สามารถสร้างโฟลเดอร์ชั่วคราวสำหรับจัดเตรียมแพ็กเกจได้";
            return false;
        }

        std::wstring presetsRoot = GetPresetsRootDirectory();
        WIN32_FIND_DATAW ffd;
        HANDLE hFind = FindFirstFileW((presetsRoot + L"\\*").c_str(), &ffd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                    wcscmp(ffd.cFileName, L".") != 0 && wcscmp(ffd.cFileName, L"..") != 0) {
                    std::wstring pName = ffd.cFileName;
                    std::wstring pDir = presetsRoot + L"\\" + pName;

                    std::vector<ScheduleGroup> groups;
                    if (!activePresetName.empty() && _wcsicmp(pName.c_str(), activePresetName.c_str()) == 0) {
                        groups = activePresetItems;
                    } else {
                        std::wstring jsonPath = pDir + L"\\" + pName + L".json";
                        if (!PathFileExistsW(jsonPath.c_str())) {
                            jsonPath = pDir + L"\\preset.json";
                        }
                        LoadFromFile(jsonPath, groups);
                    }

                    std::wstring safeName;
                    for (wchar_t c : pName) {
                        if (c == L'\\' || c == L'/' || c == L':' || c == L'*' || c == L'?' || c == L'"' || c == L'<' || c == L'>' || c == L'|') {
                            safeName += L'_';
                        } else {
                            safeName += c;
                        }
                    }
                    if (safeName.empty()) safeName = L"Preset";

                    std::wstring targetAsch = stagingDir + L"\\" + safeName + L".asch";
                    int audioCount = 0;
                    std::wstring exportErr;
                    if (ExportAschPackage(pName, groups, targetAsch, audioCount, exportErr)) {
                        outPresetCount++;
                        outTotalAudioCount += audioCount;
                    }
                }
            } while (FindNextFileW(hFind, &ffd));
            FindClose(hFind);
        }

        if (outPresetCount == 0) {
            std::filesystem::remove_all(std::filesystem::path(stagingDir), ec);
            outErrorMsg = L"ไม่พบชุด Preset ใดๆ ในระบบสำหรับสำรองข้อมูล";
            return false;
        }

        bool zipOk = CreateZipFromDirectory(targetZipPath, stagingDir);
        std::filesystem::remove_all(std::filesystem::path(stagingDir), ec);

        if (!zipOk || !PathFileExistsW(targetZipPath.c_str())) {
            outErrorMsg = L"ไม่สามารถบีบอัดไฟล์เป็น ZIP ได้";
            return false;
        }

        return true;
    }

    static bool RestoreAllFromZip(const std::wstring& sourceZipPath,
                                  int& outRestoredPresetCount,
                                  int& outRestoredAudioCount,
                                  std::wstring& outErrorMsg) {
        outRestoredPresetCount = 0;
        outRestoredAudioCount = 0;

        if (sourceZipPath.empty() || !PathFileExistsW(sourceZipPath.c_str())) {
            outErrorMsg = L"ไม่พบไฟล์ ZIP ที่ระบุ";
            return false;
        }

        wchar_t tempBase[MAX_PATH] = { 0 };
        GetTempPathW(MAX_PATH, tempBase);
        std::wstring extractDir = std::wstring(tempBase) + L"AudioSchedule_Restore_" + std::to_wstring(GetTickCount());
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(extractDir), ec);

        if (!ExtractZipToDirectory(sourceZipPath, extractDir)) {
            std::filesystem::remove_all(std::filesystem::path(extractDir), ec);
            outErrorMsg = L"ไม่สามารถแตกไฟล์ ZIP สำรองได้ (ไฟล์อาจชำรุดหรือไม่ถูกต้อง)";
            return false;
        }

        for (const auto& entry : std::filesystem::recursive_directory_iterator(extractDir, ec)) {
            if (entry.is_regular_file()) {
                std::wstring ext = entry.path().extension().wstring();
                for (auto& c : ext) c = towlower(c);
                if (ext == L".asch" || ext == L".atp" || ext == L".atpkg") {
                    std::wstring pkgPath = entry.path().wstring();
                    std::wstring finalName;
                    int groupsCount = 0;
                    int audioCount = 0;
                    std::wstring impErr;
                    if (ImportAschPackage(pkgPath, L"", finalName, groupsCount, audioCount, impErr)) {
                        outRestoredPresetCount++;
                        outRestoredAudioCount += audioCount;
                    }
                }
            }
        }

        std::filesystem::remove_all(std::filesystem::path(extractDir), ec);

        if (outRestoredPresetCount == 0) {
            outErrorMsg = L"ไม่พบไฟล์แพ็กเกจชุดการเล่น (.asch หรือ .atp) ภายในไฟล์ ZIP นี้";
            return false;
        }

        return true;
    }

    // ================= Proprietary Encrypted Package (.asch / .atp) =================
    static void EncryptDecryptBytes(uint8_t* data, size_t length, size_t seedOffset = 0) {
        static const uint8_t KEY[32] = {
            0x58, 0x41, 0x54, 0x50, 0x23, 0x7E, 0x9A, 0x3C,
            0x8B, 0x1F, 0x4D, 0x6E, 0x05, 0x72, 0xFA, 0x89,
            0x31, 0xC4, 0x67, 0x8E, 0xB2, 0x19, 0x63, 0x4F,
            0xAA, 0x55, 0xDE, 0xAD, 0xBE, 0xEF, 0x77, 0x99
        };
        for (size_t i = 0; i < length; ++i) {
            size_t idx = (i + seedOffset);
            data[i] ^= (KEY[idx % 32] ^ (uint8_t)((idx * 13 + 0x47) & 0xFF));
        }
    }

    static bool PeekAschPresetName(const std::wstring& pkgFilePath, std::wstring& outPresetName) {
        std::ifstream in(pkgFilePath.c_str(), std::ios::binary);
        if (!in.is_open()) return false;
        char magic[8] = { 0 };
        in.read(magic, 8);
        if (memcmp(magic, "ASCHPAK1", 8) != 0 && memcmp(magic, "ATPPACK1", 8) != 0) return false;
        uint32_t version = 0, flags = 0;
        in.read(reinterpret_cast<char*>(&version), sizeof(version));
        in.read(reinterpret_cast<char*>(&flags), sizeof(flags));
        uint32_t presetNameLen = 0;
        in.read(reinterpret_cast<char*>(&presetNameLen), sizeof(presetNameLen));
        if (presetNameLen == 0 || presetNameLen > 1024) return false;
        std::string presetUtf8(presetNameLen, '\0');
        in.read(&presetUtf8[0], presetNameLen);
        outPresetName = Utf8ToWide(presetUtf8);
        return !outPresetName.empty();
    }

    static bool ExportAschPackage(const std::wstring& sourcePresetName,
                                 const std::vector<ScheduleGroup>& groups,
                                 const std::wstring& targetAschFilePath,
                                 int& outAudioCount,
                                 std::wstring& outErrorMsg) {
        outAudioCount = 0;
        if (sourcePresetName.empty() || targetAschFilePath.empty()) {
            outErrorMsg = L"ข้อมูลชุดการเล่นหรือไฟล์ปลายทางไม่ถูกต้อง";
            return false;
        }

        std::ofstream out(targetAschFilePath.c_str(), std::ios::binary);
        if (!out.is_open()) {
            outErrorMsg = L"ไม่สามารถสร้างไฟล์แพ็กเกจปลายทางได้ (โปรดตรวจสอบสิทธิ์การเขียนไฟล์)";
            return false;
        }

        // Header: Magic "ASCHPAK1"
        const char magic[8] = { 'A', 'S', 'C', 'H', 'P', 'A', 'K', '1' };
        out.write(magic, 8);
        uint32_t version = 1;
        out.write(reinterpret_cast<const char*>(&version), sizeof(version));
        uint32_t flags = 0x01; // encrypted
        out.write(reinterpret_cast<const char*>(&flags), sizeof(flags));

        // Prepare portable items with relative audio paths "audio\..."
        std::vector<ScheduleGroup> portableItems = groups;
        struct AudioFileInfo {
            std::wstring srcFullPath;
            std::wstring archiveFileName;
        };
        std::vector<AudioFileInfo> audioList;

        for (size_t g = 0; g < portableItems.size(); ++g) {
            for (size_t f = 0; f < portableItems[g].audioFiles.size(); ++f) {
                const std::wstring& srcPath = groups[g].audioFiles[f];
                if (srcPath.empty() || !PathFileExistsW(srcPath.c_str())) continue;

                const wchar_t* pName = wcsrchr(srcPath.c_str(), L'\\');
                std::wstring baseFileName = pName ? (pName + 1) : srcPath;

                bool found = false;
                std::wstring targetArchiveName;
                for (const auto& a : audioList) {
                    if (_wcsicmp(a.srcFullPath.c_str(), srcPath.c_str()) == 0) {
                        targetArchiveName = a.archiveFileName;
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    std::wstring stem = baseFileName;
                    std::wstring ext;
                    size_t dotPos = baseFileName.rfind(L'.');
                    if (dotPos != std::string::npos) {
                        stem = baseFileName.substr(0, dotPos);
                        ext = baseFileName.substr(dotPos);
                    }
                    targetArchiveName = baseFileName;
                    int counter = 1;
                    bool nameCollision = true;
                    while (nameCollision) {
                        nameCollision = false;
                        for (const auto& a : audioList) {
                            if (_wcsicmp(a.archiveFileName.c_str(), targetArchiveName.c_str()) == 0) {
                                targetArchiveName = stem + L"_" + std::to_wstring(counter++) + ext;
                                nameCollision = true;
                                break;
                            }
                        }
                    }
                    AudioFileInfo info;
                    info.srcFullPath = srcPath;
                    info.archiveFileName = targetArchiveName;
                    audioList.push_back(info);
                }

                portableItems[g].audioFiles[f] = L"audio\\" + targetArchiveName;
            }
        }

        // Generate JSON string
        std::string jsonStr;
        {
            std::ostringstream ss;
            ss << "[\n";
            for (size_t i = 0; i < portableItems.size(); ++i) {
                const auto& item = portableItems[i];
                ss << "  {\n";
                ss << "    \"id\": " << item.id << ",\n";
                ss << "    \"name\": \"" << EscapeJsonString(WideToUtf8(item.name)) << "\",\n";
                ss << "    \"hour\": " << item.hour << ",\n";
                ss << "    \"minute\": " << item.minute << ",\n";
                ss << "    \"second\": " << item.second << ",\n";
                ss << "    \"daysMask\": " << (int)item.daysMask << ",\n";
                ss << "    \"enabled\": " << (item.enabled ? "true" : "false") << ",\n";
                ss << "    \"audioFiles\": [\n";
                for (size_t f = 0; f < item.audioFiles.size(); ++f) {
                    ss << "      \"" << EscapeJsonString(WideToUtf8(item.audioFiles[f])) << "\"";
                    if (f + 1 < item.audioFiles.size()) ss << ",";
                    ss << "\n";
                }
                ss << "    ]\n";
                ss << "  }";
                if (i + 1 < portableItems.size()) ss << ",";
                ss << "\n";
            }
            ss << "]\n";
            jsonStr = ss.str();
        }

        // Write Metadata
        std::string presetUtf8 = WideToUtf8(sourcePresetName);
        uint32_t presetNameLen = (uint32_t)presetUtf8.size();
        out.write(reinterpret_cast<const char*>(&presetNameLen), sizeof(presetNameLen));
        out.write(presetUtf8.data(), presetNameLen);

        uint32_t groupCount = (uint32_t)portableItems.size();
        out.write(reinterpret_cast<const char*>(&groupCount), sizeof(groupCount));

        uint32_t fileCount = (uint32_t)audioList.size();
        out.write(reinterpret_cast<const char*>(&fileCount), sizeof(fileCount));

        // Encrypt and write JSON
        uint64_t jsonLen = (uint64_t)jsonStr.size();
        out.write(reinterpret_cast<const char*>(&jsonLen), sizeof(jsonLen));
        std::vector<uint8_t> encJson(jsonStr.begin(), jsonStr.end());
        EncryptDecryptBytes(encJson.data(), encJson.size(), 0);
        out.write(reinterpret_cast<const char*>(encJson.data()), encJson.size());

        // Encrypt and write Audio Files
        for (size_t a = 0; a < audioList.size(); ++a) {
            std::string arcNameUtf8 = WideToUtf8(audioList[a].archiveFileName);
            uint32_t arcNameLen = (uint32_t)arcNameUtf8.size();
            out.write(reinterpret_cast<const char*>(&arcNameLen), sizeof(arcNameLen));
            out.write(arcNameUtf8.data(), arcNameLen);

            std::ifstream audioIn(audioList[a].srcFullPath.c_str(), std::ios::binary);
            if (!audioIn.is_open()) {
                uint64_t zeroSize = 0;
                out.write(reinterpret_cast<const char*>(&zeroSize), sizeof(zeroSize));
                continue;
            }

            audioIn.seekg(0, std::ios::end);
            uint64_t fileSize = (uint64_t)audioIn.tellg();
            audioIn.seekg(0, std::ios::beg);

            out.write(reinterpret_cast<const char*>(&fileSize), sizeof(fileSize));

            // Stream and encrypt in 64KB chunks
            const size_t CHUNK = 65536;
            std::vector<uint8_t> buffer(CHUNK);
            uint64_t bytesRemaining = fileSize;
            size_t totalOffset = 0;

            while (bytesRemaining > 0) {
                size_t toRead = (size_t)std::min<uint64_t>(bytesRemaining, CHUNK);
                audioIn.read(reinterpret_cast<char*>(buffer.data()), toRead);
                size_t actualRead = (size_t)audioIn.gcount();
                if (actualRead == 0) break;

                EncryptDecryptBytes(buffer.data(), actualRead, totalOffset);
                out.write(reinterpret_cast<const char*>(buffer.data()), actualRead);

                totalOffset += actualRead;
                bytesRemaining -= actualRead;
            }
            outAudioCount++;
        }

        // Footer: Magic End
        const char magicEnd[4] = { 'A', 'S', 'C', 'H' };
        out.write(magicEnd, 4);

        return true;
    }

    static bool ImportAschPackage(const std::wstring& pkgFilePath,
                                 const std::wstring& targetPresetNameOverride,
                                 std::wstring& outFinalPresetName,
                                 int& outGroupsCount,
                                 int& outAudioCount,
                                 std::wstring& outErrorMsg) {
        outGroupsCount = 0;
        outAudioCount = 0;

        std::ifstream in(pkgFilePath.c_str(), std::ios::binary);
        if (!in.is_open()) {
            outErrorMsg = L"ไม่สามารถเปิดไฟล์แพ็กเกจได้";
            return false;
        }

        // Check header
        char magic[8] = { 0 };
        in.read(magic, 8);
        if (memcmp(magic, "ASCHPAK1", 8) != 0 && memcmp(magic, "ATPPACK1", 8) != 0) {
            outErrorMsg = L"ไฟล์นี้ไม่ใช่แพ็กเกจของ Audio Schedule หรือไฟล์เสียหาย";
            return false;
        }

        uint32_t version = 0;
        in.read(reinterpret_cast<char*>(&version), sizeof(version));
        uint32_t flags = 0;
        in.read(reinterpret_cast<char*>(&flags), sizeof(flags));

        // Read preset name
        uint32_t presetNameLen = 0;
        in.read(reinterpret_cast<char*>(&presetNameLen), sizeof(presetNameLen));
        if (presetNameLen == 0 || presetNameLen > 1024) {
            outErrorMsg = L"ข้อมูลแพ็กเกจไม่ถูกต้อง (Preset Name Corrupt)";
            return false;
        }
        std::string presetUtf8(presetNameLen, '\0');
        in.read(&presetUtf8[0], presetNameLen);
        std::wstring presetName = Utf8ToWide(presetUtf8);

        uint32_t groupCount = 0;
        in.read(reinterpret_cast<char*>(&groupCount), sizeof(groupCount));

        uint32_t fileCount = 0;
        in.read(reinterpret_cast<char*>(&fileCount), sizeof(fileCount));

        // Read and decrypt JSON
        uint64_t jsonLen = 0;
        in.read(reinterpret_cast<char*>(&jsonLen), sizeof(jsonLen));
        if (jsonLen == 0 || jsonLen > 100 * 1024 * 1024) {
            outErrorMsg = L"ข้อมูลตารางตั้งเวลาในแพ็กเกจเสียหาย";
            return false;
        }
        std::vector<uint8_t> jsonBuf(jsonLen);
        in.read(reinterpret_cast<char*>(jsonBuf.data()), jsonLen);
        EncryptDecryptBytes(jsonBuf.data(), jsonBuf.size(), 0);
        std::string jsonStr(jsonBuf.begin(), jsonBuf.end());

        // Determine destination folder
        std::wstring safeName = targetPresetNameOverride.empty() ? presetName : targetPresetNameOverride;
        std::wstring cleanSafe;
        for (wchar_t c : safeName) {
            if (c == L'\\' || c == L'/' || c == L':' || c == L'*' || c == L'?' || c == L'"' || c == L'<' || c == L'>' || c == L'|') {
                cleanSafe += L'_';
            } else {
                cleanSafe += c;
            }
        }
        if (cleanSafe.empty()) cleanSafe = L"Imported_Package";

        std::wstring presetsRoot = GetPresetsRootDirectory();
        std::wstring targetFolder = presetsRoot + L"\\" + cleanSafe;

        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(targetFolder), ec);
        std::wstring targetAudioDir = targetFolder + L"\\audio";
        std::filesystem::create_directories(std::filesystem::path(targetAudioDir), ec);

        // Extract and decrypt Audio Files
        for (uint32_t a = 0; a < fileCount; ++a) {
            uint32_t arcNameLen = 0;
            in.read(reinterpret_cast<char*>(&arcNameLen), sizeof(arcNameLen));
            if (arcNameLen == 0 || arcNameLen > 512) break;

            std::string arcNameUtf8(arcNameLen, '\0');
            in.read(&arcNameUtf8[0], arcNameLen);
            std::wstring arcName = Utf8ToWide(arcNameUtf8);

            uint64_t fileSize = 0;
            in.read(reinterpret_cast<char*>(&fileSize), sizeof(fileSize));

            std::wstring dstAudioPath = targetAudioDir + L"\\" + arcName;
            std::ofstream audioOut(dstAudioPath.c_str(), std::ios::binary);

            const size_t CHUNK = 65536;
            std::vector<uint8_t> buffer(CHUNK);
            uint64_t bytesRemaining = fileSize;
            size_t totalOffset = 0;

            while (bytesRemaining > 0) {
                size_t toRead = (size_t)std::min<uint64_t>(bytesRemaining, CHUNK);
                in.read(reinterpret_cast<char*>(buffer.data()), toRead);
                size_t actualRead = (size_t)in.gcount();
                if (actualRead == 0) break;

                EncryptDecryptBytes(buffer.data(), actualRead, totalOffset);
                if (audioOut.is_open()) {
                    audioOut.write(reinterpret_cast<const char*>(buffer.data()), actualRead);
                }

                totalOffset += actualRead;
                bytesRemaining -= actualRead;
            }
            if (audioOut.is_open()) {
                audioOut.close();
                outAudioCount++;
            }
        }

        // Save single target JSON: preset.json
        std::wstring targetJson = targetFolder + L"\\preset.json";
        std::ofstream jsonOut(targetJson.c_str(), std::ios::binary);
        if (jsonOut.is_open()) {
            jsonOut.write(jsonStr.data(), jsonStr.size());
            jsonOut.close();
        }

        // Clean up legacy duplicate <cleanSafe>.json if exists
        std::wstring legacyJson = targetFolder + L"\\" + cleanSafe + L".json";
        if (_wcsicmp(legacyJson.c_str(), targetJson.c_str()) != 0 && PathFileExistsW(legacyJson.c_str())) {
            DeleteFileW(legacyJson.c_str());
        }

        outFinalPresetName = cleanSafe;
        outGroupsCount = (int)groupCount;
        return true;
    }

    // Backward-compatibility aliases
    static bool PeekAtpPresetName(const std::wstring& path, std::wstring& outName) {
        return PeekAschPresetName(path, outName);
    }

    static bool ExportAtpPackage(const std::wstring& src, const std::vector<ScheduleGroup>& g, const std::wstring& dst, int& ac, std::wstring& err) {
        return ExportAschPackage(src, g, dst, ac, err);
    }

    static bool ImportAtpPackage(const std::wstring& path, const std::wstring& nameOverride, std::wstring& finalName, int& gc, int& ac, std::wstring& err) {
        return ImportAschPackage(path, nameOverride, finalName, gc, ac, err);
    }
};
