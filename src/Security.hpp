#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace Security {

// ============================================================================
// LEVEL 2: COMPILE-TIME STRING OBFUSCATION
// ============================================================================

// Hash compile time to generate dynamic XOR seeds
constexpr uint32_t HashTimeSeed(const char* timeStr) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < 8; ++i) {
        hash = (hash ^ static_cast<uint8_t>(timeStr[i])) * 16777619u;
    }
    return hash;
}

constexpr uint8_t GlobalXorKey = static_cast<uint8_t>(HashTimeSeed(__TIME__) & 0xFF) ^ 0x6B;

template <size_t N, typename T>
class EncryptedString {
public:
    constexpr EncryptedString(const T* str, uint8_t key) : m_key(key), m_data{} {
        for (size_t i = 0; i < N; ++i) {
            m_data[i] = static_cast<T>(str[i] ^ static_cast<T>(key + static_cast<uint8_t>(i * 5 + 3)));
        }
    }

    std::basic_string<T> decrypt() const {
        std::basic_string<T> result;
        result.resize(N);
        for (size_t i = 0; i < N; ++i) {
            result[i] = static_cast<T>(m_data[i] ^ static_cast<T>(m_key + static_cast<uint8_t>(i * 5 + 3)));
        }
        return result;
    }

private:
    uint8_t m_key;
    T m_data[N];
};

// ============================================================================
// LEVEL 3: ANTI-DEBUGGING & ANTI-TAMPERING
// ============================================================================

class AntiDebug {
public:
    // Check if any debugging tool (IDA, x64dbg, Visual Studio, Cheat Engine) is attached
    static bool IsDebugged() {
        // 1. Direct Win32 API Check
        if (IsDebuggerPresent()) {
            return true;
        }

        // 2. CheckRemoteDebuggerPresent (Dynamically resolved for maximum Windows compatibility)
        typedef BOOL (WINAPI *pfnCheckRemoteDebuggerPresent)(HANDLE, PBOOL);
        HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");
        if (hKernel) {
            pfnCheckRemoteDebuggerPresent pCheck = 
                reinterpret_cast<pfnCheckRemoteDebuggerPresent>(reinterpret_cast<void*>(GetProcAddress(hKernel, "CheckRemoteDebuggerPresent")));
            if (pCheck) {
                BOOL isDebugger = FALSE;
                if (pCheck(GetCurrentProcess(), &isDebugger) && isDebugger) {
                    return true;
                }
            }
        }

        // 3. Hardware Breakpoint Registers Check (DR0 - DR3)
        alignas(16) CONTEXT ctx;
        ZeroMemory(&ctx, sizeof(ctx));
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        HANDLE hThread = GetCurrentThread();
        if (GetThreadContext(hThread, &ctx)) {
            if (ctx.Dr0 != 0 || ctx.Dr1 != 0 || ctx.Dr2 != 0 || ctx.Dr3 != 0) {
                return true;
            }
        }

        // 4. Low-level PEB BeingDebugged Check
#if defined(_WIN64)
        unsigned char* peb = (unsigned char*)__readgsqword(0x60);
        if (peb && peb[2] != 0) {
            return true;
        }
#elif defined(_M_IX86) || defined(__i386__)
        unsigned char* peb = nullptr;
        __asm__ volatile ("movl %%fs:0x30, %0" : "=r" (peb));
        if (peb && peb[2] != 0) {
            return true;
        }
#endif

        return false;
    }

    // Immediate action if debugger or memory tampering is detected
    static void Enforce() {
        if (IsDebugged()) {
            // Silently terminate process immediately to block inspection/dumping
            ExitProcess(0);
        }
    }
};

} // namespace Security

// Macro for obfuscating wide-character strings (Unicode)
#define CRYPT_W(str) \
    ([]() -> std::wstring { \
        constexpr size_t len = (sizeof(str) / sizeof(wchar_t)) - 1; \
        constexpr ::Security::EncryptedString<len, wchar_t> enc(str, ::Security::GlobalXorKey ^ 0x93); \
        return enc.decrypt(); \
    }())

// Macro for obfuscating narrow-character strings (ASCII/UTF-8)
#define CRYPT_A(str) \
    ([]() -> std::string { \
        constexpr size_t len = sizeof(str) - 1; \
        constexpr ::Security::EncryptedString<len, char> enc(str, ::Security::GlobalXorKey ^ 0x47); \
        return enc.decrypt(); \
    }())
