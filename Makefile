ARCH ?= 64

ifeq ($(ARCH),32)
    CXX = i686-w64-mingw32-g++
    WINDRES = i686-w64-mingw32-windres
    TARGET = AudioSchedule_x86.exe
    SUBSYS = 5.1
    RC_OBJ = src/app32.res
else
    CXX = g++
    WINDRES = windres
    TARGET = AudioSchedule.exe
    SUBSYS = 5.2
    RC_OBJ = src/app.res
endif

CXXFLAGS = -O2 -std=c++17 -Wall -DUNICODE -D_UNICODE -s -fno-ident
LDFLAGS = -static -static-libgcc -static-libstdc++ -mwindows -municode -Wl,--subsystem,windows:$(SUBSYS) -Wl,--strip-all -lcomctl32 -lcomdlg32 -lwinmm -lole32 -lshlwapi -lshell32

SRCS = src/main.cpp
RC_SRC = src/app.rc

all: $(TARGET)

$(RC_OBJ): $(RC_SRC) src/app.manifest assets/app.ico src/Version.h
	$(WINDRES) -i $(RC_SRC) -O coff -o $(RC_OBJ)

$(TARGET): $(SRCS) $(RC_OBJ) src/Version.h src/MainWindow.hpp src/GroupDialog.hpp src/NewPresetDialog.hpp src/RenamePresetDialog.hpp src/SavePresetDialog.hpp src/DeletePresetDialog.hpp src/AboutDialog.hpp src/ManualDialog.hpp src/AudioEngine.hpp src/ScheduleGroup.hpp src/PresetManager.hpp src/Security.hpp
	$(CXX) $(CXXFLAGS) $(SRCS) $(RC_OBJ) $(LDFLAGS) -o $(TARGET)

clean:
	-cmd /c "if exist AudioSchedule.exe del /Q AudioSchedule.exe"
	-cmd /c "if exist AudioSchedule_x86.exe del /Q AudioSchedule_x86.exe"
	-cmd /c "if exist src\app.res del /Q src\app.res"
	-cmd /c "if exist src\app32.res del /Q src\app32.res"
	-cmd /c "build.cmd clean"

portable:
	cmd /c "build.cmd portable"

installer:
	cmd /c "build.cmd installer"

dist:
	cmd /c "build.cmd all"
