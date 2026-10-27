#pragma once
#include <windows.h>
#include <mmsystem.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "AppSettings.hpp"

class AudioEngine {
public:
    struct Channel {
        int id = 0;
        std::wstring alias;
        std::wstring groupName;
        std::vector<std::wstring> playlist;
        size_t currentTrackIndex = 0;
        bool isPlaying = false;
        bool isPaused = false;
        HWND hwndNotify = NULL;
    };

    struct QueuedGroup {
        std::wstring groupName;
        std::vector<std::wstring> files;
        HWND hwndNotify = NULL;
    };

    static AudioEngine& Instance() {
        static AudioEngine instance;
        return instance;
    }

    bool PlayGroup(const std::wstring& groupName, const std::vector<std::wstring>& files, HWND hwndNotify = NULL, bool isManual = false) {
        if (files.empty()) return false;

        if (AppSettings::overlapMode == OverlapMode::Queue) {
            if (isManual) {
                // Manual click overrides: stop all channels & clear queue
                Stop();
                m_groupQueue.clear();
                return StartChannel(0, groupName, files, hwndNotify);
            } else {
                // Scheduled automatic trigger
                if (!IsActivePlaybackRunning()) {
                    return StartChannel(0, groupName, files, hwndNotify);
                } else {
                    // Check if identical group already in queue to avoid infinite duplicates
                    for (const auto& q : m_groupQueue) {
                        if (q.groupName == groupName) return true;
                    }
                    QueuedGroup q;
                    q.groupName = groupName;
                    q.files = files;
                    q.hwndNotify = hwndNotify;
                    m_groupQueue.push_back(q);
                    return true;
                }
            }
        } else {
            // Simultaneous mode:
            if (isManual) {
                // If this group is already playing, restart it on its channel
                int existingCh = FindChannelByGroup(groupName);
                if (existingCh >= 0) {
                    StopChannel(existingCh);
                    return StartChannel(existingCh, groupName, files, hwndNotify);
                }
            }
            int chId = GetNextFreeChannelId();
            return StartChannel(chId, groupName, files, hwndNotify);
        }
    }

    bool PlaySingle(const std::wstring& filePath, HWND hwndNotify = NULL) {
        std::vector<std::wstring> single = { filePath };
        Stop();
        m_groupQueue.clear();
        return StartChannel(0, L"", single, hwndNotify);
    }

    void Stop() {
        for (auto& ch : m_channels) {
            std::wstring stopCmd = L"stop " + ch.alias;
            mciSendStringW(stopCmd.c_str(), NULL, 0, NULL);
            std::wstring closeCmd = L"close " + ch.alias;
            mciSendStringW(closeCmd.c_str(), NULL, 0, NULL);
        }
        m_channels.clear();
        m_groupQueue.clear();
        PlaySoundW(NULL, NULL, 0); // Stop any legacy PlaySound
    }

    void StopGroup(const std::wstring& groupName) {
        // Remove from queue if present
        for (auto it = m_groupQueue.begin(); it != m_groupQueue.end(); ) {
            if (it->groupName == groupName) {
                it = m_groupQueue.erase(it);
            } else {
                ++it;
            }
        }

        // Stop active channel
        int chId = FindChannelByGroup(groupName);
        if (chId >= 0) {
            StopChannel(chId);
            // In Queue mode, if queue not empty, start next
            if (AppSettings::overlapMode == OverlapMode::Queue && !m_groupQueue.empty()) {
                QueuedGroup next = m_groupQueue.front();
                m_groupQueue.erase(m_groupQueue.begin());
                StartChannel(0, next.groupName, next.files, next.hwndNotify);
            }
        }
    }

    bool Pause() {
        bool anyPaused = false;
        for (auto& ch : m_channels) {
            if (ch.isPlaying && !ch.isPaused) {
                std::wstring cmd = L"pause " + ch.alias;
                mciSendStringW(cmd.c_str(), NULL, 0, NULL);
                ch.isPaused = true;
                anyPaused = true;
            }
        }
        return anyPaused;
    }

    bool Resume() {
        bool anyResumed = false;
        for (auto& ch : m_channels) {
            if (ch.isPlaying && ch.isPaused) {
                std::wstring cmd = L"resume " + ch.alias;
                if (ch.hwndNotify) cmd += L" notify";
                mciSendStringW(cmd.c_str(), NULL, 0, ch.hwndNotify);
                ch.isPaused = false;
                anyResumed = true;
            }
        }
        return anyResumed;
    }

    bool PauseOrResumeTrack(const std::wstring& filePath) {
        for (auto& ch : m_channels) {
            if (ch.isPlaying && ch.currentTrackIndex < ch.playlist.size() && ch.playlist[ch.currentTrackIndex] == filePath) {
                if (ch.isPaused) {
                    std::wstring cmd = L"resume " + ch.alias;
                    if (ch.hwndNotify) cmd += L" notify";
                    mciSendStringW(cmd.c_str(), NULL, 0, ch.hwndNotify);
                    ch.isPaused = false;
                    return true;
                } else {
                    std::wstring cmd = L"pause " + ch.alias;
                    mciSendStringW(cmd.c_str(), NULL, 0, NULL);
                    ch.isPaused = true;
                    return true;
                }
            }
        }
        return false;
    }

    bool IsPaused() const {
        for (const auto& ch : m_channels) {
            if (ch.isPlaying && ch.isPaused) return true;
        }
        return false;
    }

    bool IsPlaying() const {
        if (!m_groupQueue.empty()) return true;
        for (const auto& ch : m_channels) {
            if (ch.isPlaying) return true;
        }
        return false;
    }

    bool IsActivePlaybackRunning() const {
        for (const auto& ch : m_channels) {
            if (ch.isPlaying) return true;
        }
        return false;
    }

    bool IsGroupPlaying(const std::wstring& groupName) const {
        for (const auto& ch : m_channels) {
            if (ch.isPlaying && ch.groupName == groupName) return true;
        }
        return false;
    }

    bool IsTrackPlaying(const std::wstring& filePath) const {
        for (const auto& ch : m_channels) {
            if (ch.isPlaying && ch.currentTrackIndex < ch.playlist.size() && ch.playlist[ch.currentTrackIndex] == filePath) {
                return true;
            }
        }
        return false;
    }

    bool IsTrackPaused(const std::wstring& filePath) const {
        for (const auto& ch : m_channels) {
            if (ch.isPlaying && ch.isPaused && ch.currentTrackIndex < ch.playlist.size() && ch.playlist[ch.currentTrackIndex] == filePath) {
                return true;
            }
        }
        return false;
    }

    void OnTrackFinished(int chId = -1) {
        if (chId >= 0) {
            int idx = FindChannelIndexById(chId);
            if (idx >= 0) AdvanceChannel(idx);
            return;
        }

        // Default: advance channel 0 if exists
        if (!m_channels.empty()) {
            AdvanceChannel(0);
        }
    }

    void CheckAndAdvanceTrack() {
        for (int i = (int)m_channels.size() - 1; i >= 0; --i) {
            if (!m_channels[i].isPlaying || m_channels[i].isPaused) continue;

            wchar_t statusBuf[128] = { 0 };
            std::wstring statCmd = L"status " + m_channels[i].alias + L" mode";
            mciSendStringW(statCmd.c_str(), statusBuf, 128, NULL);
            if (wcscmp(statusBuf, L"stopped") == 0) {
                AdvanceChannel(i);
            }
        }
    }

    std::wstring GetCurrentTrackPath() const {
        if (!m_channels.empty() && m_channels[0].currentTrackIndex < m_channels[0].playlist.size()) {
            return m_channels[0].playlist[m_channels[0].currentTrackIndex];
        }
        return L"";
    }

    std::wstring GetCurrentTrackName() const {
        std::wstring path = GetCurrentTrackPath();
        if (path.empty()) return L"";
        const wchar_t* fn = wcsrchr(path.c_str(), L'\\');
        return fn ? (fn + 1) : path;
    }

    std::wstring GetGroupName() const {
        if (m_channels.empty()) return L"";
        if (m_channels.size() == 1) return m_channels[0].groupName;

        std::wstring names;
        for (size_t i = 0; i < m_channels.size(); ++i) {
            if (i > 0) names += L", ";
            names += m_channels[i].groupName;
        }
        return names;
    }

    size_t GetCurrentTrackIndex() const {
        if (!m_channels.empty()) return m_channels[0].currentTrackIndex;
        return 0;
    }

    size_t GetTotalTracks() const {
        if (!m_channels.empty()) return m_channels[0].playlist.size();
        return 0;
    }

    size_t GetQueuedCount() const {
        return m_groupQueue.size();
    }

    size_t GetActiveChannelCount() const {
        size_t count = 0;
        for (const auto& ch : m_channels) {
            if (ch.isPlaying) count++;
        }
        return count;
    }

    std::wstring GetPlaybackStatusString() const {
        if (!IsPlaying()) return L"ไม่ได้เล่นเสียง (Stopped)";

        std::wstring str;
        if (IsPaused()) {
            str += L"⏸️ [พักชั่วคราว] ";
        } else {
            str += L"🔊 ";
        }

        if (m_channels.size() > 1) {
            str += L"กำลังเล่นพร้อมกัน (" + std::to_wstring(m_channels.size()) + L" กลุ่ม): " + GetGroupName();
        } else if (!m_channels.empty()) {
            if (!m_channels[0].groupName.empty()) {
                str += L"กลุ่ม: [" + m_channels[0].groupName + L"] ";
            }
            str += L"เพลงที่ (" + std::to_wstring(m_channels[0].currentTrackIndex + 1) + L"/" + std::to_wstring(m_channels[0].playlist.size()) + L"): ";
            str += GetCurrentTrackName();
        }

        if (!m_groupQueue.empty()) {
            str += L" | ⏳ ในคิวรอ: " + std::to_wstring(m_groupQueue.size()) + L" กลุ่ม";
        }
        return str;
    }

    // ================= Audio Duration Utility =================
    static long long GetWavDurationMs(const std::wstring& filePath) {
        FILE* f = NULL;
        if (_wfopen_s(&f, filePath.c_str(), L"rb") != 0 || !f) return -1;
        char header[12];
        if (fread(header, 1, 12, f) != 12 || memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0) {
            fclose(f);
            return -1;
        }
        DWORD avgBytesPerSec = 0;
        DWORD dataSize = 0;
        char chunkId[4];
        DWORD chunkSize = 0;
        while (fread(chunkId, 1, 4, f) == 4 && fread(&chunkSize, 4, 1, f) == 1) {
            if (memcmp(chunkId, "fmt ", 4) == 0) {
                BYTE fmtData[16];
                DWORD toRead = (chunkSize >= 16) ? 16 : chunkSize;
                if (fread(fmtData, 1, toRead, f) == toRead) {
                    avgBytesPerSec = *(DWORD*)(fmtData + 8);
                    if (chunkSize > toRead) fseek(f, chunkSize - toRead, SEEK_CUR);
                } else break;
            } else if (memcmp(chunkId, "data", 4) == 0) {
                dataSize = chunkSize;
                break;
            } else {
                fseek(f, chunkSize, SEEK_CUR);
            }
        }
        fclose(f);
        if (avgBytesPerSec > 0 && dataSize > 0) {
            return (long long)dataSize * 1000 / avgBytesPerSec;
        }
        return -1;
    }

    struct PlaybackProgressInfo {
        bool isPlaying = false;
        bool isPaused = false;
        std::wstring groupName;
        std::wstring currentTrackPath;
        size_t currentTrackNum = 0;
        size_t totalTracks = 0;
        long long currentTrackElapsedMs = 0;
        long long currentTrackTotalMs = 0;
        long long groupElapsedMs = 0;
        long long groupTotalMs = 0;
        double progressRatio = 0.0; // 0.0 to 1.0
    };

    PlaybackProgressInfo GetActiveProgress() {
        PlaybackProgressInfo info;
        for (const auto& ch : m_channels) {
            if (ch.isPlaying && !ch.playlist.empty()) {
                info.isPlaying = true;
                info.isPaused = ch.isPaused;
                info.groupName = ch.groupName;
                info.totalTracks = ch.playlist.size();
                info.currentTrackNum = ch.currentTrackIndex + 1;
                if (ch.currentTrackIndex < ch.playlist.size()) {
                    info.currentTrackPath = ch.playlist[ch.currentTrackIndex];
                }

                long long groupTotal = 0;
                long long elapsedBefore = 0;
                for (size_t i = 0; i < ch.playlist.size(); ++i) {
                    long long dur = GetAudioDurationMs(ch.playlist[i]);
                    groupTotal += dur;
                    if (i < ch.currentTrackIndex) {
                        elapsedBefore += dur;
                    }
                }
                info.groupTotalMs = groupTotal;

                wchar_t posBuf[64] = { 0 };
                std::wstring statCmd = L"status " + ch.alias + L" position";
                mciSendStringW(statCmd.c_str(), posBuf, 64, NULL);
                long long trackPos = _wtoi64(posBuf);
                info.currentTrackElapsedMs = trackPos;
                info.currentTrackTotalMs = GetAudioDurationMs(info.currentTrackPath);

                info.groupElapsedMs = elapsedBefore + trackPos;
                if (info.groupTotalMs > 0) {
                    info.progressRatio = (double)info.groupElapsedMs / (double)info.groupTotalMs;
                    if (info.progressRatio < 0.0) info.progressRatio = 0.0;
                    if (info.progressRatio > 1.0) info.progressRatio = 1.0;
                }
                return info;
            }
        }
        return info;
    }

    static std::wstring FormatSeconds(long long totalSec) {
        if (totalSec < 0) totalSec = 0;
        int m = (int)(totalSec / 60);
        int s = (int)(totalSec % 60);
        wchar_t buf[32];
        swprintf_s(buf, L"%02d:%02d", m, s);
        return buf;
    }

    static std::wstring FormatMs(long long totalMs) {
        return FormatSeconds((totalMs + 500) / 1000);
    }

    static long long GetAudioDurationMs(const std::wstring& filePath) {
        if (filePath.empty() || !PathFileExistsW(filePath.c_str())) return 0;

        static std::map<std::wstring, long long> s_durationMsCache;
        auto it = s_durationMsCache.find(filePath);
        if (it != s_durationMsCache.end()) return it->second;

        long long msResult = 0;

        // Try fast WAV parser if it's .wav
        if (filePath.size() >= 4) {
            std::wstring ext = filePath.substr(filePath.size() - 4);
            if (_wcsicmp(ext.c_str(), L".wav") == 0) {
                long long wavMs = GetWavDurationMs(filePath);
                if (wavMs > 0) {
                    s_durationMsCache[filePath] = wavMs;
                    return wavMs;
                }
            }
        }

        // Try MCI probing
        wchar_t shortPath[MAX_PATH] = { 0 };
        DWORD len = GetShortPathNameW(filePath.c_str(), shortPath, MAX_PATH);
        const wchar_t* pathPtr = (len > 0) ? shortPath : filePath.c_str();

        std::wstring alias = L"probe_" + std::to_wstring(GetTickCount()) + L"_" + std::to_wstring(rand());
        std::wstring openCmd = L"open \"" + std::wstring(pathPtr) + L"\" type mpegvideo alias " + alias;
        MCIERROR err = mciSendStringW(openCmd.c_str(), NULL, 0, NULL);
        if (err != 0) {
            openCmd = L"open \"" + std::wstring(pathPtr) + L"\" alias " + alias;
            err = mciSendStringW(openCmd.c_str(), NULL, 0, NULL);
        }

        if (err == 0) {
            std::wstring setCmd = L"set " + alias + L" time format milliseconds";
            mciSendStringW(setCmd.c_str(), NULL, 0, NULL);
            wchar_t lenBuf[64] = { 0 };
            std::wstring statCmd = L"status " + alias + L" length";
            if (mciSendStringW(statCmd.c_str(), lenBuf, 64, NULL) == 0) {
                msResult = _wtoi64(lenBuf);
            }
            std::wstring closeCmd = L"close " + alias;
            mciSendStringW(closeCmd.c_str(), NULL, 0, NULL);
        }

        s_durationMsCache[filePath] = msResult;
        return msResult;
    }

    static std::wstring GetAudioDuration(const std::wstring& filePath) {
        long long ms = GetAudioDurationMs(filePath);
        if (ms <= 0) return L"--:--";
        return FormatMs(ms);
    }

private:
    AudioEngine() {}
    ~AudioEngine() { Stop(); }

    int GetNextFreeChannelId() {
        int id = 0;
        while (true) {
            bool found = false;
            for (const auto& ch : m_channels) {
                if (ch.id == id) { found = true; break; }
            }
            if (!found) return id;
            id++;
        }
    }

    int FindChannelIndexById(int id) const {
        for (size_t i = 0; i < m_channels.size(); ++i) {
            if (m_channels[i].id == id) return (int)i;
        }
        return -1;
    }

    int FindChannelByGroup(const std::wstring& groupName) const {
        for (const auto& ch : m_channels) {
            if (ch.groupName == groupName) return ch.id;
        }
        return -1;
    }

    bool StartChannel(int chId, const std::wstring& groupName, const std::vector<std::wstring>& files, HWND hwndNotify) {
        // If channel exists, remove it
        int existingIdx = FindChannelIndexById(chId);
        if (existingIdx >= 0) {
            CloseChannel(chId);
        }

        Channel ch;
        ch.id = chId;
        ch.alias = L"atp_ch_" + std::to_wstring(chId);
        ch.groupName = groupName;
        ch.playlist = files;
        ch.currentTrackIndex = 0;
        ch.isPlaying = false;
        ch.isPaused = false;
        ch.hwndNotify = hwndNotify;

        m_channels.push_back(ch);
        int idx = (int)m_channels.size() - 1;
        return PlayChannelTrack(m_channels[idx]);
    }

    void StopChannel(int chId) {
        int idx = FindChannelIndexById(chId);
        if (idx >= 0) {
            std::wstring stopCmd = L"stop " + m_channels[idx].alias;
            mciSendStringW(stopCmd.c_str(), NULL, 0, NULL);
            std::wstring closeCmd = L"close " + m_channels[idx].alias;
            mciSendStringW(closeCmd.c_str(), NULL, 0, NULL);
            m_channels.erase(m_channels.begin() + idx);
        }
    }

    void CloseChannel(int chId) {
        StopChannel(chId);
    }

    void AdvanceChannel(int chIdx) {
        if (chIdx < 0 || chIdx >= (int)m_channels.size()) return;
        m_channels[chIdx].currentTrackIndex++;
        if (m_channels[chIdx].currentTrackIndex < m_channels[chIdx].playlist.size()) {
            PlayChannelTrack(m_channels[chIdx]);
        } else {
            // Channel finished all tracks
            int chId = m_channels[chIdx].id;
            CloseChannel(chId);

            // If in Queue mode, start next queued group
            if (AppSettings::overlapMode == OverlapMode::Queue) {
                if (!m_groupQueue.empty()) {
                    QueuedGroup next = m_groupQueue.front();
                    m_groupQueue.erase(m_groupQueue.begin());
                    StartChannel(0, next.groupName, next.files, next.hwndNotify);
                }
            }
        }
    }

    bool PlayChannelTrack(Channel& ch) {
        if (ch.currentTrackIndex >= ch.playlist.size()) {
            return false;
        }

        const std::wstring& filePath = ch.playlist[ch.currentTrackIndex];
        if (filePath.empty()) {
            int chId = ch.id;
            int idx = FindChannelIndexById(chId);
            if (idx >= 0) AdvanceChannel(idx);
            return false;
        }

        // Close previous track on this alias
        std::wstring closeCmd = L"close " + ch.alias;
        mciSendStringW(closeCmd.c_str(), NULL, 0, NULL);

        // Convert to short path name to avoid MCI parsing issues with spaces or unicode
        wchar_t shortPath[MAX_PATH] = { 0 };
        DWORD len = GetShortPathNameW(filePath.c_str(), shortPath, MAX_PATH);
        const wchar_t* pathPtr = (len > 0) ? shortPath : filePath.c_str();

        std::wstring openCmd = L"open \"" + std::wstring(pathPtr) + L"\" type mpegvideo alias " + ch.alias;
        MCIERROR err = mciSendStringW(openCmd.c_str(), NULL, 0, NULL);
        if (err != 0) {
            openCmd = L"open \"" + std::wstring(pathPtr) + L"\" alias " + ch.alias;
            err = mciSendStringW(openCmd.c_str(), NULL, 0, NULL);
        }

        if (err != 0) {
            // Fallback for simple WAV files
            if (filePath.size() >= 4) {
                std::wstring ext = filePath.substr(filePath.size() - 4);
                if (_wcsicmp(ext.c_str(), L".wav") == 0) {
                    PlaySoundW(filePath.c_str(), NULL, SND_FILENAME | SND_ASYNC);
                    ch.isPlaying = true;
                    ch.isPaused = false;
                    return true;
                }
            }
            int chId = ch.id;
            int idx = FindChannelIndexById(chId);
            if (idx >= 0) AdvanceChannel(idx);
            return false;
        }

        std::wstring playCmd = L"play " + ch.alias + L" from 0";
        if (ch.hwndNotify) playCmd += L" notify";
        mciSendStringW(playCmd.c_str(), NULL, 0, ch.hwndNotify);

        ch.isPlaying = true;
        ch.isPaused = false;
        return true;
    }

    std::vector<Channel> m_channels;
    std::vector<QueuedGroup> m_groupQueue;
};
