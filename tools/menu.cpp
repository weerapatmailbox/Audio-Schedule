#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

static std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0);
    std::wstring wstr(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &wstr[0], size);
    return wstr;
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    if (argc < 3) {
        std::cerr << "Usage: menu <Title> <Option1> <Option2> [Option3 ...]" << std::endl;
        return 0;
    }

    std::wstring title = Utf8ToWide(argv[1]);
    std::vector<std::wstring> options;
    for (int i = 2; i < argc; ++i) {
        options.push_back(Utf8ToWide(argv[i]));
    }

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);

    // If input is redirected (e.g. piped or running non-interactively), read from cin
    if (GetFileType(hIn) != FILE_TYPE_CHAR) {
        std::string line;
        if (std::getline(std::cin, line)) {
            int val = atoi(line.c_str());
            return val;
        }
        return 0;
    }

    DWORD prevConsoleMode = 0;
    GetConsoleMode(hIn, &prevConsoleMode);
    SetConsoleMode(hIn, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hOut, &csbi);
    WORD defaultAttr = csbi.wAttributes;

    CONSOLE_CURSOR_INFO cci;
    GetConsoleCursorInfo(hOut, &cci);
    BOOL prevCursorVisible = cci.bVisible;
    cci.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &cci);

    std::wcout << L"\n  " << title << L"\n";
    std::wcout << L"  ====================================================================\n";
    std::wcout << L"  ใช้ปุ่มลูกศร [↑] [↓] เพื่อเลื่อน และกด [Enter] เพื่อเลือก (ESC เพื่อยกเลิก)\n\n";

    GetConsoleScreenBufferInfo(hOut, &csbi);
    SHORT menuStartY = csbi.dwCursorPosition.Y;

    int selected = 0;
    int numOptions = static_cast<int>(options.size());
    bool running = true;
    int exitCode = 0;

    auto render = [&]() {
        for (int i = 0; i < numOptions; ++i) {
            COORD pos = { 2, static_cast<SHORT>(menuStartY + i) };
            SetConsoleCursorPosition(hOut, pos);

            if (i == selected) {
                SetConsoleTextAttribute(hOut, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                std::wstring line = L" ►  " + options[i] + L"   ";
                DWORD written = 0;
                WriteConsoleW(hOut, line.c_str(), (DWORD)line.size(), &written, NULL);
            } else {
                SetConsoleTextAttribute(hOut, defaultAttr);
                std::wstring line = L"    " + options[i] + L"   ";
                DWORD written = 0;
                WriteConsoleW(hOut, line.c_str(), (DWORD)line.size(), &written, NULL);
            }
        }
        SetConsoleTextAttribute(hOut, defaultAttr);
    };

    render();

    while (running) {
        INPUT_RECORD ir;
        DWORD read = 0;
        if (!ReadConsoleInputW(hIn, &ir, 1, &read) || read == 0) {
            continue;
        }

        if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown) {
            WORD vk = ir.Event.KeyEvent.wVirtualKeyCode;
            if (vk == VK_UP) {
                selected = (selected - 1 + numOptions) % numOptions;
                render();
            } else if (vk == VK_DOWN) {
                selected = (selected + 1) % numOptions;
                render();
            } else if (vk == VK_RETURN) {
                exitCode = selected + 1;
                running = false;
            } else if (vk == VK_ESCAPE) {
                exitCode = 0;
                running = false;
            } else if (vk >= '1' && vk <= '9') {
                int idx = vk - '1';
                if (idx < numOptions) {
                    selected = idx;
                    render();
                    exitCode = selected + 1;
                    running = false;
                }
            } else if (vk == '0') {
                exitCode = 0;
                running = false;
            }
        }
    }

    COORD endPos = { 0, static_cast<SHORT>(menuStartY + numOptions + 1) };
    SetConsoleCursorPosition(hOut, endPos);
    std::wcout << L"\n";

    cci.bVisible = prevCursorVisible;
    SetConsoleCursorInfo(hOut, &cci);
    SetConsoleMode(hIn, prevConsoleMode);

    return exitCode;
}
