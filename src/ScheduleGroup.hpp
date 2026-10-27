#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include "Language.h"

struct ScheduleGroup {
    int id = 0;
    std::wstring name;
    int hour = 8;
    int minute = 0;
    int second = 0;
    uint8_t daysMask = 0x7F; // All 7 days active by default (bits 0-6: Sun, Mon, Tue, Wed, Thu, Fri, Sat)
    std::vector<std::wstring> audioFiles; // Consecutive audio files to play
    bool enabled = true;
    
    // Runtime state (not saved)
    int lastTriggeredDate = 0; // YYYYMMDD

    bool IsDayActive(int dayOfWeek) const {
        if (dayOfWeek < 0 || dayOfWeek > 6) return false;
        return (daysMask & (1 << dayOfWeek)) != 0;
    }

    void SetDayActive(int dayOfWeek, bool active) {
        if (dayOfWeek < 0 || dayOfWeek > 6) return;
        if (active) {
            daysMask |= (1 << dayOfWeek);
        } else {
            daysMask &= ~(1 << dayOfWeek);
        }
    }

    std::wstring GetTimeString() const {
        wchar_t buf[16];
        swprintf_s(buf, L"%02d:%02d:%02d", hour, minute, second);
        return buf;
    }

    std::wstring GetDaysSummary() const {
        if (daysMask == 0x7F) return Lang::EveryDay();
        if (daysMask == 0x3E) return Lang::MonToFri();
        if (daysMask == 0x41) return (Lang::current == Language::Thai) ? L"เสาร์-อาทิตย์" : L"Sat - Sun";
        if (daysMask == 0) return (Lang::current == Language::Thai) ? L"ไม่มีวัน" : L"None";

        std::wstring result;
        for (int i = 0; i < 7; ++i) {
            if (IsDayActive(i)) {
                if (!result.empty()) result += L", ";
                switch (i) {
                    case 0: result += Lang::DaySun(); break;
                    case 1: result += Lang::DayMon(); break;
                    case 2: result += Lang::DayTue(); break;
                    case 3: result += Lang::DayWed(); break;
                    case 4: result += Lang::DayThu(); break;
                    case 5: result += Lang::DayFri(); break;
                    case 6: result += Lang::DaySat(); break;
                }
            }
        }
        return result;
    }

    std::wstring GetFilesSummary() const {
        if (audioFiles.empty()) return L"(ยังไม่มีไฟล์เสียงในกลุ่ม)";
        if (audioFiles.size() == 1) {
            const wchar_t* fn = wcsrchr(audioFiles[0].c_str(), L'\\');
            return fn ? (fn + 1) : audioFiles[0];
        }

        std::wstring summary = std::to_wstring(audioFiles.size()) + L" ไฟล์: ";
        for (size_t i = 0; i < audioFiles.size(); ++i) {
            if (i > 0) summary += L" ➔ ";
            const wchar_t* fn = wcsrchr(audioFiles[i].c_str(), L'\\');
            summary += fn ? (fn + 1) : audioFiles[i];
            if (i >= 2 && audioFiles.size() > 3) {
                summary += L" ➔ ...";
                break;
            }
        }
        return summary;
    }
};

// Typedef for backwards compatibility
typedef ScheduleGroup ScheduleItem;
