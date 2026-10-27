@echo off
chcp 65001 >nul
cd /d "%~dp0"

REM Read Application Version from src\Version.h
set "APP_VER=1.0.0"
if exist "src\Version.h" (
    for /f "tokens=3" %%A in ('findstr /C:"#define APP_VERSION_STRING" src\Version.h 2^>nul') do set "APP_VER=%%~A"
)

REM --------------------------------------------------------------------------
REM Toolchain Detection & Environment Auto-Configuration
REM --------------------------------------------------------------------------
REM Load local developer overrides if build.local.bat exists (ignored by Git)
if exist "build.local.bat" call "build.local.bat"
if exist "build.local.cmd" call "build.local.cmd"

REM --- Detect 64-bit MinGW Compiler ---
set "CXX64="
set "RC64="

REM 1. Check custom environment variables
if defined LLVM_MINGW_BIN (
    if exist "%LLVM_MINGW_BIN%\x86_64-w64-mingw32-g++.exe" (
        set "CXX64=%LLVM_MINGW_BIN%\x86_64-w64-mingw32-g++.exe"
        set "RC64=%LLVM_MINGW_BIN%\x86_64-w64-mingw32-windres.exe"
        set "PATH=%LLVM_MINGW_BIN%;%PATH%"
    ) else if exist "%LLVM_MINGW_BIN%\g++.exe" (
        set "CXX64=%LLVM_MINGW_BIN%\g++.exe"
        set "RC64=%LLVM_MINGW_BIN%\windres.exe"
        set "PATH=%LLVM_MINGW_BIN%;%PATH%"
    )
)
if not defined CXX64 if defined LLVM_MINGW_HOME (
    if exist "%LLVM_MINGW_HOME%\bin\x86_64-w64-mingw32-g++.exe" (
        set "CXX64=%LLVM_MINGW_HOME%\bin\x86_64-w64-mingw32-g++.exe"
        set "RC64=%LLVM_MINGW_HOME%\bin\x86_64-w64-mingw32-windres.exe"
        set "PATH=%LLVM_MINGW_HOME%\bin;%PATH%"
    )
)
if not defined CXX64 if defined MINGW64_BIN (
    if exist "%MINGW64_BIN%\g++.exe" (
        set "CXX64=%MINGW64_BIN%\g++.exe"
        set "RC64=%MINGW64_BIN%\windres.exe"
        set "PATH=%MINGW64_BIN%;%PATH%"
    ) else if exist "%MINGW64_BIN%\x86_64-w64-mingw32-g++.exe" (
        set "CXX64=%MINGW64_BIN%\x86_64-w64-mingw32-g++.exe"
        set "RC64=%MINGW64_BIN%\x86_64-w64-mingw32-windres.exe"
        set "PATH=%MINGW64_BIN%;%PATH%"
    )
)
if not defined CXX64 if defined MINGW64_HOME (
    if exist "%MINGW64_HOME%\bin\g++.exe" (
        set "CXX64=%MINGW64_HOME%\bin\g++.exe"
        set "RC64=%MINGW64_HOME%\bin\windres.exe"
        set "PATH=%MINGW64_HOME%\bin;%PATH%"
    ) else if exist "%MINGW64_HOME%\bin\x86_64-w64-mingw32-g++.exe" (
        set "CXX64=%MINGW64_HOME%\bin\x86_64-w64-mingw32-g++.exe"
        set "RC64=%MINGW64_HOME%\bin\x86_64-w64-mingw32-windres.exe"
        set "PATH=%MINGW64_HOME%\bin;%PATH%"
    )
)
if not defined CXX64 if defined MINGW_HOME (
    if exist "%MINGW_HOME%\bin\g++.exe" (
        set "CXX64=%MINGW_HOME%\bin\g++.exe"
        set "RC64=%MINGW_HOME%\bin\windres.exe"
        set "PATH=%MINGW_HOME%\bin;%PATH%"
    ) else if exist "%MINGW_HOME%\bin\x86_64-w64-mingw32-g++.exe" (
        set "CXX64=%MINGW_HOME%\bin\x86_64-w64-mingw32-g++.exe"
        set "RC64=%MINGW_HOME%\bin\x86_64-w64-mingw32-windres.exe"
        set "PATH=%MINGW_HOME%\bin;%PATH%"
    )
)

REM 2. Search common standard installation directories
if not defined CXX64 (
    for %%D in (
        "%USERPROFILE%\Documents\Book\mingw64\bin"
        "%USERPROFILE%\Documents\Book\mingw86\bin"
        "%USERPROFILE%\Documents\Book\llvm-mingw\bin"
        "C:\llvm-mingw\bin"
        "D:\llvm-mingw\bin"
        "%USERPROFILE%\llvm-mingw\bin"
        "%USERPROFILE%\scoop\apps\llvm-mingw\current\bin"
        "%LOCALAPPDATA%\Programs\llvm-mingw\bin"
        "C:\msys64\mingw64\bin"
        "C:\msys64\ucrt64\bin"
        "C:\msys64\clang64\bin"
        "D:\msys64\mingw64\bin"
        "D:\msys64\ucrt64\bin"
        "C:\mingw64\bin"
        "D:\mingw64\bin"
        "C:\mingw-w64\bin"
        "C:\winlibs\bin"
        "C:\winlibs-x86_64\bin"
        "C:\tools\mingw64\bin"
        "C:\tools\llvm-mingw\bin"
        "%USERPROFILE%\scoop\apps\mingw\current\bin"
        "%USERPROFILE%\scoop\apps\mingw-winlibs\current\bin"
        "%LOCALAPPDATA%\Programs\mingw64\bin"
        "%ProgramFiles%\mingw-w64\bin"
        "%ProgramFiles%\llvm-mingw\bin"
        "%~dp0toolchain\mingw64\bin"
        "%~dp0toolchain\llvm-mingw\bin"
        "%~dp0..\mingw64\bin"
        "%~dp0..\llvm-mingw\bin"
    ) do (
        if not defined CXX64 (
            if exist "%%~D\g++.exe" (
                set "CXX64=%%~D\g++.exe"
                set "RC64=%%~D\windres.exe"
                set "PATH=%%~D;%PATH%"
            ) else if exist "%%~D\x86_64-w64-mingw32-g++.exe" (
                set "CXX64=%%~D\x86_64-w64-mingw32-g++.exe"
                if exist "%%~D\x86_64-w64-mingw32-windres.exe" (
                    set "RC64=%%~D\x86_64-w64-mingw32-windres.exe"
                ) else if exist "%%~D\llvm-windres.exe" (
                    set "RC64=%%~D\llvm-windres.exe"
                ) else (
                    set "RC64=%%~D\windres.exe"
                )
                set "PATH=%%~D;%PATH%"
            )
        )
    )
)

REM 3. Fallback to System PATH
if not defined CXX64 (
    where x86_64-w64-mingw32-g++ >nul 2>&1
    if not errorlevel 1 (
        set "CXX64=x86_64-w64-mingw32-g++"
        where x86_64-w64-mingw32-windres >nul 2>&1
        if not errorlevel 1 (
            set "RC64=x86_64-w64-mingw32-windres"
        ) else (
            set "RC64=llvm-windres"
        )
    ) else (
        where g++ >nul 2>&1
        if not errorlevel 1 (
            set "CXX64=g++"
            set "RC64=windres"
        )
    )
)

REM Strip quotes if any
if defined CXX64 set "CXX64=%CXX64:"=%"
if defined RC64 set "RC64=%RC64:"=%"

REM --- Detect 32-bit MinGW Compiler ---
set "CXX32="
set "RC32="

REM 1. Check custom environment variables
if defined LLVM_MINGW_BIN (
    if exist "%LLVM_MINGW_BIN%\i686-w64-mingw32-g++.exe" (
        set "CXX32=%LLVM_MINGW_BIN%\i686-w64-mingw32-g++.exe"
        set "RC32=%LLVM_MINGW_BIN%\i686-w64-mingw32-windres.exe"
    )
)
if not defined CXX32 if defined LLVM_MINGW_HOME (
    if exist "%LLVM_MINGW_HOME%\bin\i686-w64-mingw32-g++.exe" (
        set "CXX32=%LLVM_MINGW_HOME%\bin\i686-w64-mingw32-g++.exe"
        set "RC32=%LLVM_MINGW_HOME%\bin\i686-w64-mingw32-windres.exe"
    )
)
if not defined CXX32 if defined MINGW32_BIN (
    if exist "%MINGW32_BIN%\g++.exe" (
        set "CXX32=%MINGW32_BIN%\g++.exe"
        set "RC32=%MINGW32_BIN%\windres.exe"
    ) else if exist "%MINGW32_BIN%\i686-w64-mingw32-g++.exe" (
        set "CXX32=%MINGW32_BIN%\i686-w64-mingw32-g++.exe"
        set "RC32=%MINGW32_BIN%\i686-w64-mingw32-windres.exe"
    )
)
if not defined CXX32 if defined MINGW32_HOME (
    if exist "%MINGW32_HOME%\bin\g++.exe" (
        set "CXX32=%MINGW32_HOME%\bin\g++.exe"
        set "RC32=%MINGW32_HOME%\bin\windres.exe"
    ) else if exist "%MINGW32_HOME%\bin\i686-w64-mingw32-g++.exe" (
        set "CXX32=%MINGW32_HOME%\bin\i686-w64-mingw32-g++.exe"
        set "RC32=%MINGW32_HOME%\bin\i686-w64-mingw32-windres.exe"
    )
)

REM 2. Search common standard installation directories
if not defined CXX32 (
    for %%D in (
        "%USERPROFILE%\Documents\Book\mingw86\bin"
        "%USERPROFILE%\Documents\Book\mingw32\bin"
        "%USERPROFILE%\Documents\Book\llvm-mingw\bin"
        "C:\llvm-mingw\bin"
        "D:\llvm-mingw\bin"
        "%USERPROFILE%\llvm-mingw\bin"
        "%USERPROFILE%\scoop\apps\llvm-mingw\current\bin"
        "%LOCALAPPDATA%\Programs\llvm-mingw\bin"
        "C:\msys64\mingw32\bin"
        "D:\msys64\mingw32\bin"
        "C:\mingw32\bin"
        "D:\mingw32\bin"
        "C:\MinGW\bin"
        "C:\tools\mingw32\bin"
        "C:\tools\llvm-mingw\bin"
        "%USERPROFILE%\scoop\apps\mingw32\current\bin"
        "%ProgramFiles%\llvm-mingw\bin"
        "%~dp0toolchain\mingw32\bin"
        "%~dp0toolchain\llvm-mingw\bin"
        "%~dp0..\mingw86\bin"
        "%~dp0..\mingw32\bin"
        "%~dp0..\llvm-mingw\bin"
    ) do (
        if not defined CXX32 (
            if exist "%%~D\i686-w64-mingw32-g++.exe" (
                set "CXX32=%%~D\i686-w64-mingw32-g++.exe"
                if exist "%%~D\i686-w64-mingw32-windres.exe" (
                    set "RC32=%%~D\i686-w64-mingw32-windres.exe"
                ) else if exist "%%~D\llvm-windres.exe" (
                    set "RC32=%%~D\llvm-windres.exe"
                ) else (
                    set "RC32=%%~D\windres.exe"
                )
            ) else if exist "%%~D\g++.exe" (
                set "CXX32=%%~D\g++.exe"
                set "RC32=%%~D\windres.exe"
            )
        )
    )
)

REM 3. Fallback to System PATH for cross-compiler
if not defined CXX32 (
    where i686-w64-mingw32-g++ >nul 2>&1
    if not errorlevel 1 (
        set "CXX32=i686-w64-mingw32-g++"
        where i686-w64-mingw32-windres >nul 2>&1
        if not errorlevel 1 (
            set "RC32=i686-w64-mingw32-windres"
        ) else (
            set "RC32=llvm-windres"
        )
    )
)

REM Strip quotes if any
if defined CXX32 set "CXX32=%CXX32:"=%"
if defined RC32 set "RC32=%RC32:"=%"

REM --- Verify 64-bit Compiler Availability ---
set "COMPILER_OK=0"
if defined CXX64 (
    "%CXX64%" --version >nul 2>&1
    if not errorlevel 1 set "COMPILER_OK=1"
)

if "%COMPILER_OK%"=="0" (
    call :prompt_custom_compiler_64
)
if "%COMPILER_OK%"=="0" (
    if "%~1"=="" pause
    exit /b 1
)

REM Ensure helper tools exist
if not exist "tools\menu.exe" (
    "%CXX64%" -O2 -std=c++17 -s -fno-ident -Wl,--strip-all tools/menu.cpp -o tools/menu.exe
)
if not exist "tools\packer.exe" (
    "%CXX64%" -O2 -std=c++17 -s -fno-ident -Wl,--strip-all tools/packer.cpp -o tools/packer.exe
)

REM If arguments are passed from command line, jump directly to argument handler
if not "%~1"=="" goto handle_args

:main_menu
cls
echo ======================================================================
echo             Audio Schedule - ระบบคอมไพล์และสร้างชุดติดตั้ง
echo                      Version %APP_VER%
echo ======================================================================
echo  [64-bit Toolchain]: %CXX64%
if not "%CXX32%"=="" (
echo  [32-bit Toolchain]: %CXX32%
) else (
echo  [32-bit Toolchain]: (ไม่พบในระบบ - พร้อมใช้งานเฉพาะ 64-bit)
)
echo ======================================================================

tools\menu.exe "เลือกประเภทที่ต้องการสร้าง (Select Build Target):" "สร้างตัวติดตั้ง (Build Installer - Setup .exe)" "สร้างแบบพกพา (Build Portable - ZIP)" "สร้างทั้ง 2 แบบ (Build Both - Portable + Installer)" "ล้างไฟล์ชั่วคราวและผลลัพธ์ (Clean Build & Dist)" "ออกจากโปรแกรม (Exit)"
set "ACT_CHOICE=%ERRORLEVEL%"

if "%ACT_CHOICE%"=="1" goto select_arch_for_installer
if "%ACT_CHOICE%"=="2" goto select_arch_for_portable
if "%ACT_CHOICE%"=="3" goto select_arch_for_both
if "%ACT_CHOICE%"=="4" goto op_clean
if "%ACT_CHOICE%"=="5" goto op_exit
if "%ACT_CHOICE%"=="0" goto op_exit
goto main_menu

REM --------------------------------------------------------------------------
REM Architecture Selection Menus (Interactive with Arrow Keys + Enter)
REM --------------------------------------------------------------------------
:select_arch_for_installer
cls
echo ======================================================================
echo       สร้างตัวติดตั้ง (Setup Installer) - เลือกสถาปัตยกรรม
echo ======================================================================
tools\menu.exe "เลือกสถาปัตยกรรม (Select Architecture):" "64-bit (x64) [แนะนำ - สำหรับ Windows 7/8/10/11 64-bit]" "32-bit (x86) [สำหรับ Windows 32-bit รองรับตั้งแต่ Windows XP ถึง 11]" "ทั้ง 2 สถาปัตยกรรม (Both 64-bit & 32-bit)" "ย้อนกลับสู่เมนูหลัก (Back)"
set "ARCH_CHOICE=%ERRORLEVEL%"
if "%ARCH_CHOICE%"=="1" goto do_installer_64
if "%ARCH_CHOICE%"=="2" goto do_installer_32
if "%ARCH_CHOICE%"=="3" goto do_installer_both
goto main_menu

:select_arch_for_portable
cls
echo ======================================================================
echo         สร้างแบบพกพา (Portable ZIP) - เลือกสถาปัตยกรรม
echo ======================================================================
tools\menu.exe "เลือกสถาปัตยกรรม (Select Architecture):" "64-bit (x64) [แนะนำ - สำหรับ Windows 7/8/10/11 64-bit]" "32-bit (x86) [สำหรับ Windows 32-bit รองรับตั้งแต่ Windows XP ถึง 11]" "ทั้ง 2 สถาปัตยกรรม (Both 64-bit & 32-bit)" "ย้อนกลับสู่เมนูหลัก (Back)"
set "ARCH_CHOICE=%ERRORLEVEL%"
if "%ARCH_CHOICE%"=="1" goto do_portable_64
if "%ARCH_CHOICE%"=="2" goto do_portable_32
if "%ARCH_CHOICE%"=="3" goto do_portable_both
goto main_menu

:select_arch_for_both
cls
echo ======================================================================
echo       สร้างทั้ง 2 แบบ (Portable + Installer) - เลือกสถาปัตยกรรม
echo ======================================================================
tools\menu.exe "เลือกสถาปัตยกรรม (Select Architecture):" "64-bit (x64) [แนะนำ - สำหรับ Windows 7/8/10/11 64-bit]" "32-bit (x86) [สำหรับ Windows 32-bit รองรับตั้งแต่ Windows XP ถึง 11]" "ทั้ง 2 สถาปัตยกรรม (Both 64-bit & 32-bit)" "ย้อนกลับสู่เมนูหลัก (Back)"
set "ARCH_CHOICE=%ERRORLEVEL%"
if "%ARCH_CHOICE%"=="1" goto do_both_64
if "%ARCH_CHOICE%"=="2" goto do_both_32
if "%ARCH_CHOICE%"=="3" goto do_both_all
goto main_menu

REM --------------------------------------------------------------------------
REM CLI Argument Handler
REM --------------------------------------------------------------------------
:handle_args
set "ARG_TARGET=%~1"
set "ARG_ARCH=%~2"
if "%ARG_ARCH%"=="" set "ARG_ARCH=64"

if "%ARG_TARGET%"=="clean" goto op_clean
if "%ARG_TARGET%"=="5" goto op_clean

if "%ARG_TARGET%"=="1" goto cli_installer
if /i "%ARG_TARGET%"=="installer" goto cli_installer
if /i "%ARG_TARGET%"=="setup" goto cli_installer

if "%ARG_TARGET%"=="2" goto cli_portable
if /i "%ARG_TARGET%"=="portable" goto cli_portable
if /i "%ARG_TARGET%"=="zip" goto cli_portable

if "%ARG_TARGET%"=="3" goto cli_both
if /i "%ARG_TARGET%"=="both" goto cli_both
if /i "%ARG_TARGET%"=="all" goto cli_both

echo การใช้งาน: build.cmd [installer^|portable^|both^|clean] [64^|32^|all]
exit /b 1

:cli_installer
if "%ARG_ARCH%"=="32" goto do_installer_32
if /i "%ARG_ARCH%"=="x86" goto do_installer_32
if /i "%ARG_ARCH%"=="all" goto do_installer_both
if /i "%ARG_ARCH%"=="both" goto do_installer_both
goto do_installer_64

:cli_portable
if "%ARG_ARCH%"=="32" goto do_portable_32
if /i "%ARG_ARCH%"=="x86" goto do_portable_32
if /i "%ARG_ARCH%"=="all" goto do_portable_both
if /i "%ARG_ARCH%"=="both" goto do_portable_both
goto do_portable_64

:cli_both
if "%ARG_ARCH%"=="32" goto do_both_32
if /i "%ARG_ARCH%"=="x86" goto do_both_32
if /i "%ARG_ARCH%"=="all" goto do_both_all
if /i "%ARG_ARCH%"=="both" goto do_both_all
goto do_both_64

REM --------------------------------------------------------------------------
REM Check if AudioSchedule processes are currently running
REM --------------------------------------------------------------------------
:check_running
tasklist /fi "imagename eq AudioSchedule.exe" 2>nul | find /i "AudioSchedule.exe" >nul
if not errorlevel 1 (
    echo [คำเตือน] ตรวจพบว่า AudioSchedule.exe กำลังเปิดทำงานอยู่
    echo กำลังปิดโปรแกรมอัตโนมัติเพื่อให้สามารถเขียนทับไฟล์ได้...
    taskkill /f /im AudioSchedule.exe >nul 2>&1
    ping 127.0.0.1 -n 2 >nul
)
tasklist /fi "imagename eq AudioSchedule_x86.exe" 2>nul | find /i "AudioSchedule_x86.exe" >nul
if not errorlevel 1 (
    echo [คำเตือน] ตรวจพบว่า AudioSchedule_x86.exe กำลังเปิดทำงานอยู่
    echo กำลังปิดโปรแกรมอัตโนมัติเพื่อให้สามารถเขียนทับไฟล์ได้...
    taskkill /f /im AudioSchedule_x86.exe >nul 2>&1
    ping 127.0.0.1 -n 2 >nul
)
exit /b 0

REM --------------------------------------------------------------------------
REM Subroutine: Compile 64-bit Application (AudioSchedule.exe)
REM --------------------------------------------------------------------------
:compile_app_64
call :check_running
echo.
echo [1/3] กำลังคอมไพล์ 64-bit Resource (src\app.rc)...
if not exist "build_staging\bin64" mkdir "build_staging\bin64" 2>nul & "%RC64%" -i src/app.rc -O coff -o src/app.res
if errorlevel 1 (
    echo [ERROR] ไม่สามารถคอมไพล์ 64-bit Resource ได้
    exit /b 1
)

echo [2/3] กำลังคอมไพล์ AudioSchedule.exe (Modern C++17, 64-bit)...
"%CXX64%" -O2 -std=c++17 -Wall -DUNICODE -D_UNICODE -s -fno-ident src/main.cpp src/app.res -static -static-libgcc -static-libstdc++ -mwindows -municode -Wl,--subsystem,windows:5.2 -Wl,--strip-all -lcomctl32 -lcomdlg32 -lwinmm -lole32 -lshlwapi -lshell32 -o build_staging\bin64\AudioSchedule.exe
if errorlevel 1 (
    echo [ERROR] คอมไพล์ AudioSchedule.exe 64-bit ไม่สำเร็จ
    exit /b 1
)
echo [3/3] คอมไพล์ AudioSchedule.exe (64-bit) เรียบร้อยแล้ว
exit /b 0

REM --------------------------------------------------------------------------
REM Subroutine: Compile 32-bit Application (AudioSchedule_x86.exe)
REM --------------------------------------------------------------------------
:compile_app_32
if "%CXX32%"=="" (
    echo.
    echo [ERROR] ไม่พบคอมไพเลอร์ 32-bit [i686-w64-mingw32 หรือ mingw86] ในระบบ
    echo ไม่สามารถคอมไพล์เวอร์ชัน 32-bit ได้
    exit /b 1
)
call :check_running
echo.
echo [1/3] กำลังคอมไพล์ 32-bit Resource (src\app.rc)...
if not exist "build_staging\bin32" mkdir "build_staging\bin32" 2>nul & "%RC32%" -i src/app.rc -O coff -o src/app32.res
if errorlevel 1 (
    echo [ERROR] ไม่สามารถคอมไพล์ 32-bit Resource ได้
    exit /b 1
)

echo [2/3] กำลังคอมไพล์ AudioSchedule_x86.exe (Modern C++17, 32-bit x86)...
"%CXX32%" -O2 -std=c++17 -Wall -DUNICODE -D_UNICODE -s -fno-ident src/main.cpp src/app32.res -static -static-libgcc -static-libstdc++ -mwindows -municode -Wl,--subsystem,windows:5.1 -Wl,--strip-all -lcomctl32 -lcomdlg32 -lwinmm -lole32 -lshlwapi -lshell32 -o build_staging\bin32\AudioSchedule_x86.exe
if errorlevel 1 (
    echo [ERROR] คอมไพล์ AudioSchedule_x86.exe 32-bit ไม่สำเร็จ
    exit /b 1
)
echo [3/3] คอมไพล์ AudioSchedule_x86.exe (32-bit) เรียบร้อยแล้ว
exit /b 0

REM --------------------------------------------------------------------------
REM Execution: Portable 64-bit (ZIP Only, No folder left in dist)
REM --------------------------------------------------------------------------
:do_portable_64
echo.
echo ======================================================================
echo              สร้างชุด Portable 64-bit (ZIP เท่านั้น)
echo ======================================================================
call :compile_app_64
if errorlevel 1 (
    if "%~1"=="" pause
    exit /b 1
)

set "STAGE_PORTABLE=build_staging\portable_x64\AudioSchedule"
if exist "%STAGE_PORTABLE%" rd /s /q "%STAGE_PORTABLE%"
mkdir "%STAGE_PORTABLE%" 2>nul
mkdir "dist" 2>nul

copy /Y build_staging\bin64\AudioSchedule.exe "%STAGE_PORTABLE%\" >nul
mkdir "%STAGE_PORTABLE%\presets" 2>nul & if exist presets xcopy /E /I /Y presets "%STAGE_PORTABLE%\presets" >nul

echo กำลังบีบอัดไฟล์ ZIP สำหรับ Portable (64-bit)...
set "ZIP_64=dist\AudioSchedule_v%APP_VER%_x64_Portable.zip"
if exist "%ZIP_64%" del /f /q "%ZIP_64%"

powershell -NoProfile -Command "Compress-Archive -Path '%STAGE_PORTABLE%' -DestinationPath '%ZIP_64%' -Force"

REM Clean up staging folder completely so ONLY ZIP remains
rd /s /q "build_staging" 2>nul
if exist "dist\portable" rd /s /q "dist\portable" 2>nul

if exist "%ZIP_64%" (
    echo.
    echo ======================================================================
    echo  [สำเร็จ] สร้างชุด Portable 64-bit เสร็จสมบูรณ์!
    echo ======================================================================
    echo  - ไฟล์ ZIP:     %ZIP_64%
        echo  - สถานะ:        สร้างเฉพาะไฟล์ ZIP เรียบร้อยแล้ว [ไม่เหลือโฟลเดอร์ค้าง]
    echo ======================================================================
) else (
    echo [ERROR] ไม่สามารถสร้างไฟล์ ZIP ได้
)

if "%~1"=="" (
    echo.
    echo กดปุ่มใดๆ เพื่อกลับสู่เมนูหลัก...
    pause >nul
    goto main_menu
)
exit /b 0

REM --------------------------------------------------------------------------
REM Execution: Portable 32-bit (ZIP Only, No folder left in dist)
REM --------------------------------------------------------------------------
:do_portable_32
echo.
echo ======================================================================
echo              สร้างชุด Portable 32-bit (ZIP เท่านั้น)
echo ======================================================================
call :compile_app_32
if errorlevel 1 (
    if "%~1"=="" pause
    exit /b 1
)

set "STAGE_PORTABLE=build_staging\portable_x86\AudioSchedule"
if exist "%STAGE_PORTABLE%" rd /s /q "%STAGE_PORTABLE%"
mkdir "%STAGE_PORTABLE%" 2>nul
mkdir "dist" 2>nul

copy /Y build_staging\bin32\AudioSchedule_x86.exe "%STAGE_PORTABLE%\AudioSchedule.exe" >nul
mkdir "%STAGE_PORTABLE%\presets" 2>nul & if exist presets xcopy /E /I /Y presets "%STAGE_PORTABLE%\presets" >nul

echo กำลังบีบอัดไฟล์ ZIP สำหรับ Portable (32-bit)...
set "ZIP_32=dist\AudioSchedule_v%APP_VER%_x86_Portable.zip"
if exist "%ZIP_32%" del /f /q "%ZIP_32%"

powershell -NoProfile -Command "Compress-Archive -Path '%STAGE_PORTABLE%' -DestinationPath '%ZIP_32%' -Force"

rd /s /q "build_staging" 2>nul
if exist "dist\portable" rd /s /q "dist\portable" 2>nul

if exist "%ZIP_32%" (
    echo.
    echo ======================================================================
    echo  [สำเร็จ] สร้างชุด Portable 32-bit เสร็จสมบูรณ์!
    echo ======================================================================
    echo  - ไฟล์ ZIP:     %ZIP_32%
        echo  - สถานะ:        สร้างเฉพาะไฟล์ ZIP เรียบร้อยแล้ว [ไม่เหลือโฟลเดอร์ค้าง]
    echo ======================================================================
) else (
    echo [ERROR] ไม่สามารถสร้างไฟล์ ZIP ได้
)

if "%~1"=="" (
    echo.
    echo กดปุ่มใดๆ เพื่อกลับสู่เมนูหลัก...
    pause >nul
    goto main_menu
)
exit /b 0

:do_portable_both
call :do_portable_64 silent
call :do_portable_32 silent & rd /s /q "build_staging" 2>nul
echo.
echo ======================================================================
echo  [สำเร็จเรียบร้อย] สร้างไฟล์ Portable ZIP ครบทั้ง 2 สถาปัตยกรรม!
echo ======================================================================
echo  1. 64-bit Portable: dist\AudioSchedule_v%APP_VER%_x64_Portable.zip
echo  2. 32-bit Portable: dist\AudioSchedule_v%APP_VER%_x86_Portable.zip
echo ======================================================================
if "%~1"=="" (
    echo.
    echo กดปุ่มใดๆ เพื่อกลับสู่เมนูหลัก...
    pause >nul
    goto main_menu
)
exit /b 0

REM --------------------------------------------------------------------------
REM Execution: Installer 64-bit
REM --------------------------------------------------------------------------
:do_installer_64
echo.
echo ======================================================================
echo                  สร้างตัวติดตั้ง 64-bit (Setup Installer)
echo ======================================================================
call :compile_app_64
if errorlevel 1 (
    if "%~1"=="" pause
    exit /b 1
)

echo [1/4] กำลังคอมไพล์ 64-bit Uninstaller.exe...
cd /d "%~dp0src\installer"
"%RC64%" -i uninstaller.rc -O coff -o uninstaller.res
"%CXX64%" -O2 -std=c++17 -Wall -DUNICODE -D_UNICODE -s -fno-ident Uninstaller.cpp uninstaller.res -static -static-libgcc -static-libstdc++ -mwindows -municode -Wl,--subsystem,windows:5.2 -Wl,--strip-all -lshlwapi -lshell32 -o ..\..\staging_uninst64.exe
if exist uninstaller.res del /f /q uninstaller.res
cd /d "%~dp0"

echo [2/4] จัดเตรียมแพ็กเกจข้อมูล 64-bit...
set "STAGING_DIR=build_staging\installer64_payload"
if exist "%STAGING_DIR%" rd /s /q "%STAGING_DIR%"
mkdir "%STAGING_DIR%" 2>nul
mkdir "dist" 2>nul

copy /Y build_staging\bin64\AudioSchedule.exe "%STAGING_DIR%\" >nul
move /Y staging_uninst64.exe "%STAGING_DIR%\Uninstall.exe" >nul
mkdir "%STAGING_DIR%\presets" 2>nul & if exist presets xcopy /E /I /Y presets "%STAGING_DIR%\presets" >nul

echo [3/4] รวมไฟล์ข้อมูลเป็น payload.bin...
tools\packer.exe "%STAGING_DIR%" "src\installer\payload.bin" >nul
rd /s /q "%STAGING_DIR%" 2>nul

echo [4/4] กำลังคอมไพล์ 64-bit Setup Installer...
cd /d "%~dp0src\installer"
"%RC64%" -i installer.rc -O coff -o installer.res
"%CXX64%" -O2 -std=c++17 -Wall -DUNICODE -D_UNICODE -s -fno-ident Installer.cpp installer.res -static -static-libgcc -static-libstdc++ -mwindows -municode -Wl,--subsystem,windows:5.2 -Wl,--strip-all -lcomctl32 -lole32 -luuid -lshlwapi -lshell32 -o ..\..\dist\AudioSchedule_Setup_v%APP_VER%_x64.exe
if exist installer.res del /f /q installer.res
if exist payload.bin del /f /q payload.bin
cd /d "%~dp0"

echo.
echo ======================================================================
echo  [สำเร็จ] สร้างตัวติดตั้ง 64-bit เสร็จสมบูรณ์!
echo ======================================================================
echo  - ไฟล์ตัวติดตั้ง: dist\AudioSchedule_Setup_v%APP_VER%_x64.exe
echo ======================================================================

if "%~1"=="" (
    echo.
    echo กดปุ่มใดๆ เพื่อกลับสู่เมนูหลัก...
    pause >nul
    goto main_menu
)
exit /b 0

REM --------------------------------------------------------------------------
REM Execution: Installer 32-bit
REM --------------------------------------------------------------------------
:do_installer_32
if "%CXX32%"=="" (
    echo.
    echo [ERROR] ไม่พบคอมไพเลอร์ 32-bit ในระบบ ไม่สามารถสร้าง Installer 32-bit ได้
    if "%~1"=="" pause
    exit /b 1
)
echo.
echo ======================================================================
echo                  สร้างตัวติดตั้ง 32-bit (Setup Installer)
echo ======================================================================
call :compile_app_32
if errorlevel 1 (
    if "%~1"=="" pause
    exit /b 1
)

echo [1/4] กำลังคอมไพล์ 32-bit Uninstaller.exe...
cd /d "%~dp0src\installer"
"%RC32%" -i uninstaller.rc -O coff -o uninstaller.res
"%CXX32%" -O2 -std=c++17 -Wall -DUNICODE -D_UNICODE -s -fno-ident Uninstaller.cpp uninstaller.res -static -static-libgcc -static-libstdc++ -mwindows -municode -Wl,--subsystem,windows:5.1 -Wl,--strip-all -lshlwapi -lshell32 -o ..\..\staging_uninst32.exe
if exist uninstaller.res del /f /q uninstaller.res
cd /d "%~dp0"

echo [2/4] จัดเตรียมแพ็กเกจข้อมูล 32-bit...
set "STAGING_DIR=build_staging\installer32_payload"
if exist "%STAGING_DIR%" rd /s /q "%STAGING_DIR%"
mkdir "%STAGING_DIR%" 2>nul
mkdir "dist" 2>nul

copy /Y build_staging\bin32\AudioSchedule_x86.exe "%STAGING_DIR%\AudioSchedule.exe" >nul
move /Y staging_uninst32.exe "%STAGING_DIR%\Uninstall.exe" >nul
mkdir "%STAGING_DIR%\presets" 2>nul & if exist presets xcopy /E /I /Y presets "%STAGING_DIR%\presets" >nul

echo [3/4] รวมไฟล์ข้อมูลเป็น payload.bin...
tools\packer.exe "%STAGING_DIR%" "src\installer\payload.bin" >nul
rd /s /q "%STAGING_DIR%" 2>nul

echo [4/4] กำลังคอมไพล์ 32-bit Setup Installer...
cd /d "%~dp0src\installer"
"%RC32%" -i installer.rc -O coff -o installer.res
"%CXX32%" -O2 -std=c++17 -Wall -DUNICODE -D_UNICODE -s -fno-ident Installer.cpp installer.res -static -static-libgcc -static-libstdc++ -mwindows -municode -Wl,--subsystem,windows:5.1 -Wl,--strip-all -lcomctl32 -lole32 -luuid -lshlwapi -lshell32 -o ..\..\dist\AudioSchedule_Setup_v%APP_VER%_x86.exe
if exist installer.res del /f /q installer.res
if exist payload.bin del /f /q payload.bin
cd /d "%~dp0"

echo.
echo ======================================================================
echo  [สำเร็จ] สร้างตัวติดตั้ง 32-bit เสร็จสมบูรณ์!
echo ======================================================================
echo  - ไฟล์ตัวติดตั้ง: dist\AudioSchedule_Setup_v%APP_VER%_x86.exe
echo ======================================================================

if "%~1"=="" (
    echo.
    echo กดปุ่มใดๆ เพื่อกลับสู่เมนูหลัก...
    pause >nul
    goto main_menu
)
exit /b 0

:do_installer_both
call :do_installer_64 silent
call :do_installer_32 silent & rd /s /q "build_staging" 2>nul
echo.
echo ======================================================================
echo  [สำเร็จเรียบร้อย] สร้างตัวติดตั้ง Setup ครบทั้ง 2 สถาปัตยกรรม!
echo ======================================================================
echo  1. 64-bit Setup: dist\AudioSchedule_Setup_v%APP_VER%_x64.exe
echo  2. 32-bit Setup: dist\AudioSchedule_Setup_v%APP_VER%_x86.exe
echo ======================================================================
if "%~1"=="" (
    echo.
    echo กดปุ่มใดๆ เพื่อกลับสู่เมนูหลัก...
    pause >nul
    goto main_menu
)
exit /b 0

REM --------------------------------------------------------------------------
REM Execution: Both (Portable + Installer)
REM --------------------------------------------------------------------------
:do_both_64
call :do_portable_64 silent
call :do_installer_64 silent
echo.
echo ======================================================================
echo  [สำเร็จเรียบร้อย] สร้างทั้ง Portable ZIP และ Setup (64-bit) เสร็จสมบูรณ์!
echo ======================================================================
echo  - Portable ZIP: dist\AudioSchedule_v%APP_VER%_x64_Portable.zip
echo  - Setup .exe:   dist\AudioSchedule_Setup_v%APP_VER%_x64.exe
echo ======================================================================
if "%~1"=="" (
    echo.
    echo กดปุ่มใดๆ เพื่อกลับสู่เมนูหลัก...
    pause >nul
    goto main_menu
)
exit /b 0

:do_both_32
call :do_portable_32 silent & rd /s /q "build_staging" 2>nul
call :do_installer_32 silent & rd /s /q "build_staging" 2>nul
echo.
echo ======================================================================
echo  [สำเร็จเรียบร้อย] สร้างทั้ง Portable ZIP และ Setup (32-bit) เสร็จสมบูรณ์!
echo ======================================================================
echo  - Portable ZIP: dist\AudioSchedule_v%APP_VER%_x86_Portable.zip
echo  - Setup .exe:   dist\AudioSchedule_Setup_v%APP_VER%_x86.exe
echo ======================================================================
if "%~1"=="" (
    echo.
    echo กดปุ่มใดๆ เพื่อกลับสู่เมนูหลัก...
    pause >nul
    goto main_menu
)
exit /b 0

:do_both_all
call :do_portable_64 silent
call :do_portable_32 silent & rd /s /q "build_staging" 2>nul
call :do_installer_64 silent
call :do_installer_32 silent & rd /s /q "build_staging" 2>nul
echo.
echo ======================================================================
echo  [สำเร็จเรียบร้อย] สร้างครบทุกรูปแบบทั้ง 64-bit และ 32-bit สมบูรณ์!
echo ======================================================================
echo  1. 64-bit Portable: dist\AudioSchedule_v%APP_VER%_x64_Portable.zip
echo  2. 32-bit Portable: dist\AudioSchedule_v%APP_VER%_x86_Portable.zip
echo  3. 64-bit Setup:    dist\AudioSchedule_Setup_v%APP_VER%_x64.exe
echo  4. 32-bit Setup:    dist\AudioSchedule_Setup_v%APP_VER%_x86.exe
echo ======================================================================
if "%~1"=="" (
    echo.
    echo กดปุ่มใดๆ เพื่อกลับสู่เมนูหลัก...
    pause >nul
    goto main_menu
)
exit /b 0

REM --------------------------------------------------------------------------
REM Clean Build and Dist
REM --------------------------------------------------------------------------
:op_clean
echo.
echo กำลังล้างไฟล์ชั่วคราวและผลลัพธ์การบิลด์ทั้งหมด...
if exist "src\app.res" del /f /q "src\app.res"
if exist "src\app32.res" del /f /q "src\app32.res"
if exist "src\installer\uninstaller.res" del /f /q "src\installer\uninstaller.res"
if exist "src\installer\installer.res" del /f /q "src\installer\installer.res"
if exist "src\installer\payload.bin" del /f /q "src\installer\payload.bin"
if exist "staging_uninst64.exe" del /f /q "staging_uninst64.exe"
if exist "staging_uninst32.exe" del /f /q "staging_uninst32.exe"
if exist "build_staging" rd /s /q "build_staging"
if exist "AudioSchedule.exe" del /f /q "AudioSchedule.exe"
if exist "AudioSchedule_x86.exe" del /f /q "AudioSchedule_x86.exe"
if exist "dist\portable" rd /s /q "dist\portable"
echo ล้างไฟล์ชั่วคราวเรียบร้อยแล้ว
echo.
if "%~1"=="" (
    echo กดปุ่มใดๆ เพื่อกลับสู่เมนูหลัก...
    pause >nul
    goto main_menu
)
exit /b 0

REM --------------------------------------------------------------------------
REM Subroutine: Prompt User for Custom 64-bit MinGW Path
REM --------------------------------------------------------------------------
:prompt_custom_compiler_64
echo.
echo ======================================================================
echo  [คำเตือน] ไม่พบคอมไพเลอร์ MinGW-w64 / LLVM-MinGW (64-bit) ในระบบ
echo ======================================================================
echo  ระบบได้สแกนหาใน:
echo   - Windows PATH
echo   - LLVM-MinGW (C:\llvm-mingw, D:\llvm-mingw, %USERPROFILE%\llvm-mingw...)
echo   - MSYS2 (C:\msys64\mingw64, ucrt64, clang64, D:\msys64\...)
echo   - Standalone MinGW (C:\mingw64, C:\mingw-w64, C:\winlibs, C:\tools\...)
echo   - Scoop (%USERPROFILE%\scoop\apps\llvm-mingw, apps\mingw...)
echo.
echo  แหล่งดาวน์โหลดแนะนำ (LLVM-MinGW มีทั้ง 64-bit และ 32-bit ในไฟล์เดียว):
echo    https://github.com/mstorsjo/llvm-mingw/releases
echo    (ดาวน์โหลด llvm-mingw-xxxx-ucrt-x86_64.zip แล้วแตกไฟล์ไว้ที่ใดก็ได้ เช่น C:\llvm-mingw)
echo.
echo  วิธีแก้ปัญหา:
echo   1. เพิ่มโฟลเดอร์ bin ของ MinGW/LLVM-MinGW เข้าใน System PATH ของ Windows
echo   2. หรือระบุโฟลเดอร์ bin ด้านล่างนี้ ระบบจะบันทึกลง build.local.bat ให้อัตโนมัติ
echo ======================================================================
echo.
set "USER_DIR="
set /p "USER_DIR=ระบุพาธโฟลเดอร์ bin ของ MinGW/LLVM-MinGW (หรือกด Enter เพื่อออก): "
if "%USER_DIR%"=="" exit /b 0

REM Remove quotes
set "USER_DIR=%USER_DIR:"=%"
if exist "%USER_DIR%\x86_64-w64-mingw32-g++.exe" (
    set "CXX64=%USER_DIR%\x86_64-w64-mingw32-g++.exe"
    if exist "%USER_DIR%\x86_64-w64-mingw32-windres.exe" (
        set "RC64=%USER_DIR%\x86_64-w64-mingw32-windres.exe"
    ) else if exist "%USER_DIR%\llvm-windres.exe" (
        set "RC64=%USER_DIR%\llvm-windres.exe"
    ) else (
        set "RC64=%USER_DIR%\windres.exe"
    )
    set "PATH=%USER_DIR%;%PATH%"
    set "COMPILER_OK=1"
    echo.
    echo [สำเร็จ] พบ LLVM-MinGW (64-bit) ใน: "%USER_DIR%"

    REM Auto-detect 32-bit if also present in LLVM-MinGW
    set "HAS_32=0"
    if exist "%USER_DIR%\i686-w64-mingw32-g++.exe" (
        set "CXX32=%USER_DIR%\i686-w64-mingw32-g++.exe"
        if exist "%USER_DIR%\i686-w64-mingw32-windres.exe" (
            set "RC32=%USER_DIR%\i686-w64-mingw32-windres.exe"
        ) else if exist "%USER_DIR%\llvm-windres.exe" (
            set "RC32=%USER_DIR%\llvm-windres.exe"
        ) else (
            set "RC32=%USER_DIR%\windres.exe"
        )
        set "HAS_32=1"
        echo [สำเร็จ] พบ LLVM-MinGW (32-bit i686) ในโฟลเดอร์เดียวกันด้วย!
    )

    echo บันทึกการตั้งค่าลง build.local.bat อัตโนมัติ...
    (
        echo REM Local MinGW / LLVM-MinGW Toolchain Overrides ^(Ignored by Git^)
        echo set "CXX64=%CXX64%"
        echo set "RC64=%RC64%"
        if "%HAS_32%"=="1" (
            echo set "CXX32=%CXX32%"
            echo set "RC32=%RC32%"
        )
        echo set "PATH=%USER_DIR%;%%%%PATH%%%%"
    ) > "build.local.bat"
    echo บันทึกเรียบร้อยแล้ว!
    echo.
) else if exist "%USER_DIR%\g++.exe" (
    set "CXX64=%USER_DIR%\g++.exe"
    set "RC64=%USER_DIR%\windres.exe"
    set "PATH=%USER_DIR%;%PATH%"
    set "COMPILER_OK=1"
    echo.
    echo [สำเร็จ] พบ MinGW ใน: "%USER_DIR%"
    echo บันทึกการตั้งค่าลง build.local.bat อัตโนมัติ...
    (
        echo REM Local MinGW Toolchain Overrides ^(Ignored by Git^)
        echo set "CXX64=%USER_DIR%\g++.exe"
        echo set "RC64=%USER_DIR%\windres.exe"
        echo set "PATH=%USER_DIR%;%%%%PATH%%%%"
    ) > "build.local.bat"
    echo บันทึกเรียบร้อยแล้ว!
    echo.
) else (
    echo.
    echo [ERROR] ไม่พบไฟล์ x86_64-w64-mingw32-g++.exe หรือ g++.exe ใน: "%USER_DIR%"
    echo กรุณาตรวจสอบพาธอีกครั้ง
)
exit /b 0

REM --------------------------------------------------------------------------
REM Subroutine: Prompt User for Custom 32-bit MinGW Path
REM --------------------------------------------------------------------------
:prompt_custom_compiler_32
echo.
echo ======================================================================
echo  [คำเตือน] ไม่พบคอมไพเลอร์ 32-bit ในระบบ
echo ======================================================================
echo  สำหรับการคอมไพล์ 32-bit (x86) ต้องใช้ i686-w64-mingw32-g++ หรือ mingw32/mingw86
echo  (แนะนำ LLVM-MinGW: https://github.com/mstorsjo/llvm-mingw/releases)
echo ======================================================================
echo.
set "USER_DIR32="
set /p "USER_DIR32=ระบุพาธโฟลเดอร์ bin ของ MinGW 32-bit (หรือกด Enter เพื่อข้าม): "
if "%USER_DIR32%"=="" exit /b 0

set "USER_DIR32=%USER_DIR32:"=%"
if exist "%USER_DIR32%\i686-w64-mingw32-g++.exe" (
    set "CXX32=%USER_DIR32%\i686-w64-mingw32-g++.exe"
    if exist "%USER_DIR32%\i686-w64-mingw32-windres.exe" (
        set "RC32=%USER_DIR32%\i686-w64-mingw32-windres.exe"
    ) else if exist "%USER_DIR32%\llvm-windres.exe" (
        set "RC32=%USER_DIR32%\llvm-windres.exe"
    ) else (
        set "RC32=%USER_DIR32%\windres.exe"
    )
    echo.
    echo [สำเร็จ] พบ 32-bit LLVM-MinGW ใน: "%USER_DIR32%"
    echo บันทึกการตั้งค่าลง build.local.bat...
    (
        echo set "CXX32=%CXX32%"
        echo set "RC32=%RC32%"
    ) >> "build.local.bat"
    echo บันทึกเรียบร้อยแล้ว!
    echo.
) else if exist "%USER_DIR32%\g++.exe" (
    set "CXX32=%USER_DIR32%\g++.exe"
    set "RC32=%USER_DIR32%\windres.exe"
    echo.
    echo [สำเร็จ] พบ 32-bit MinGW ใน: "%USER_DIR32%"
    echo บันทึกการตั้งค่าลง build.local.bat...
    (
        echo set "CXX32=%USER_DIR32%\g++.exe"
        echo set "RC32=%USER_DIR32%\windres.exe"
    ) >> "build.local.bat"
    echo บันทึกเรียบร้อยแล้ว!
    echo.
) else (
    echo.
    echo [ERROR] ไม่พบไฟล์ i686-w64-mingw32-g++.exe หรือ g++.exe ใน: "%USER_DIR32%"
)
exit /b 0

:op_exit
exit /b 0

