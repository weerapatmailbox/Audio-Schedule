#pragma once
#include <string>

enum class Language {
    English = 0,
    Thai = 1
};

class Lang {
public:
    static inline Language current = Language::English;

    // Window Title
    static const wchar_t* WindowTitle() {
        return L"Audio Schedule v1.0.0";
    }

    // Tabs
    static const wchar_t* TabSchedule() {
        return (current == Language::Thai) ? L"📅 ตารางเวลา" : L"📅 Schedule";
    }
    static const wchar_t* TabSettings() {
        return (current == Language::Thai) ? L"⚙️ ตั้งค่า" : L"⚙️ Settings";
    }

    // Menu: File
    static const wchar_t* MenuFile() {
        return (current == Language::Thai) ? L"ไฟล์ (&File)" : L"&File";
    }
    static const wchar_t* MenuFileSave() {
        return (current == Language::Thai) ? L"💾 บันทึก (&Save)\tCtrl+S" : L"💾 &Save\tCtrl+S";
    }
    static const wchar_t* MenuFileSaveAs() {
        return (current == Language::Thai) ? L"💾 บันทึกเป็น... (Save &As...)" : L"💾 Save &As...";
    }
    static const wchar_t* MenuFileExit() {
        return (current == Language::Thai) ? L"ออกจากโปรแกรม (&Exit)" : L"E&xit";
    }

    // Menu: Backup
    static const wchar_t* MenuBackup() {
        return (current == Language::Thai) ? L"สำรองข้อมูล (&Backup)" : L"&Backup";
    }
    static const wchar_t* MenuExportPortable() {
        return (current == Language::Thai) ? L"🔒 ส่งออกชุดการเล่นแบบ Portable (.asch)..." : L"🔒 Export Portable Preset Package (.asch)...";
    }
    static const wchar_t* MenuImportPortable() {
        return (current == Language::Thai) ? L"📥 นำเข้าชุดการเล่นแบบ Portable (.asch)..." : L"📥 Import Portable Preset Package (.asch)...";
    }
    static const wchar_t* MenuBackupAllZip() {
        return (current == Language::Thai) ? L"📦 สำรองข้อมูลทุกชุดเป็นไฟล์ ZIP (.zip)..." : L"📦 Backup All Presets to ZIP (.zip)...";
    }
    static const wchar_t* MenuRestoreAllZip() {
        return (current == Language::Thai) ? L"📂 กู้คืนข้อมูลจากไฟล์ ZIP (.zip)..." : L"📂 Restore Presets from ZIP (.zip)...";
    }

    // Menu: Language
    static const wchar_t* MenuLanguage() {
        return (current == Language::Thai) ? L"ภาษา (&Language)" : L"&Language";
    }
    static const wchar_t* MenuLangEnglish() {
        return L"English (Default)";
    }
    static const wchar_t* MenuLangThai() {
        return L"ภาษาไทย (Thai)";
    }

    // Menu: Help
    static const wchar_t* MenuHelp() {
        return (current == Language::Thai) ? L"ช่วยเหลือ (&Help)" : L"&Help";
    }
    static const wchar_t* MenuManual() {
        return (current == Language::Thai) ? L"📖 คู่มือการใช้งาน (&Manual)\tF1" : L"📖 User &Manual\tF1";
    }
    static const wchar_t* MenuSettings() {
        return (current == Language::Thai) ? L"⚙️ ตั้งค่าระบบ (&Settings)..." : L"⚙️ &Settings...";
    }
    static const wchar_t* MenuAbout() {
        return (current == Language::Thai) ? L"ℹ️ เกี่ยวกับโปรแกรม (&About)" : L"ℹ️ &About Audio Schedule";
    }

    // Header items
    static const wchar_t* NextEventChecking() {
        return (current == Language::Thai) ? L"กำลังตรวจสอบรายการถัดไป..." : L"Checking next schedule...";
    }
    static const wchar_t* NextEventPrefix() {
        return (current == Language::Thai) ? L"รายการถัดไป: " : L"Next: ";
    }
    static const wchar_t* NoActiveSchedule() {
        return (current == Language::Thai) ? L"ไม่มีรายการที่จะเล่นในขณะนี้" : L"No upcoming schedule";
    }
    static const wchar_t* CountdownFormat() {
        return (current == Language::Thai) ? L" (อีก %02d:%02d:%02d)" : L" (in %02d:%02d:%02d)";
    }
    static const wchar_t* PresetLabel() {
        return (current == Language::Thai) ? L"📁 ชุดการเล่น (Preset):" : L"📁 Preset Package:";
    }
    static const wchar_t* BtnManagePreset() {
        return (current == Language::Thai) ? L"⚙️ จัดการ ▾" : L"⚙️ Manage ▾";
    }
    static const wchar_t* MenuPresetNew() {
        return (current == Language::Thai) ? L"➕ สร้างชุดการเล่นใหม่... (&New...)\tCtrl+N" : L"➕ Create &New Preset...\tCtrl+N";
    }
    static const wchar_t* MenuPresetEdit() {
        return (current == Language::Thai) ? L"✏️ เปลี่ยนชื่อชุดนี้... (&Rename...)" : L"✏️ &Rename Preset...";
    }
    static const wchar_t* MenuPresetDelete() {
        return (current == Language::Thai) ? L"🗑️ ลบชุดการเล่นนี้... (&Delete...)" : L"🗑️ &Delete Preset...";
    }
    static const wchar_t* BtnNewPreset() {
        return (current == Language::Thai) ? L"➕ สร้างชุด" : L"➕ New";
    }
    static const wchar_t* BtnDelPreset() {
        return (current == Language::Thai) ? L"🗑️ ลบชุด" : L"🗑️ Delete";
    }
    static const wchar_t* StatusSaved() {
        return (current == Language::Thai) ? L"🟢 สถานะ: บันทึกแล้ว" : L"🟢 Status: Saved";
    }
    static const wchar_t* StatusUnsaved() {
        return (current == Language::Thai) ? L"⚠️ สถานะ: ยังไม่บันทึก" : L"⚠️ Status: Unsaved";
    }

    // Left Panel (Schedule Groups)
    static const wchar_t* GroupsTitle() {
        return (current == Language::Thai) ? L"📋 รายการกลุ่มเวลา:" : L"📋 Scheduled Groups:";
    }
    static const wchar_t* ColGroupPlay() {
        return (current == Language::Thai) ? L"เล่นเสียง" : L"Play";
    }
    static const wchar_t* ColDragHandle() {
        return L"▦";
    }
    static const wchar_t* ColManage() {
        return (current == Language::Thai) ? L"จัดการ" : L"Manage";
    }
    static const wchar_t* ColOrder() {
        return (current == Language::Thai) ? L"ลำดับ" : L"#";
    }
    static const wchar_t* ColAudioControls() {
        return (current == Language::Thai) ? L"ควบคุม" : L"Controls";
    }
    static const wchar_t* ColGroupStatus() {
        return (current == Language::Thai) ? L"สถานะ" : L"Status";
    }
    static const wchar_t* ColGroupStartTime() {
        return (current == Language::Thai) ? L"เวลาเริ่ม" : L"Time";
    }
    static const wchar_t* ColGroupDays() {
        return (current == Language::Thai) ? L"วันที่เล่น" : L"Active Days";
    }
    static const wchar_t* ColGroupName() {
        return (current == Language::Thai) ? L"ชื่อกลุ่ม (Group Name)" : L"Group Name";
    }
    static const wchar_t* ColGroupFiles() {
        return (current == Language::Thai) ? L"จำนวนไฟล์" : L"Tracks";
    }
    static const wchar_t* BtnAddGroup() {
        return (current == Language::Thai) ? L"➕ สร้างกลุ่มใหม่" : L"➕ Add Group";
    }
    static const wchar_t* BtnEditGroup() {
        return (current == Language::Thai) ? L"✏️ ตั้งเวลา/วัน" : L"✏️ Edit Schedule";
    }
    static const wchar_t* BtnDelGroup() {
        return (current == Language::Thai) ? L"❌ ลบกลุ่ม" : L"❌ Delete Group";
    }

    // Right Panel (Audio Files)
    static const wchar_t* AudioTitle() {
        return (current == Language::Thai)
            ? L"🎵 รายการเสียงในกลุ่ม (ควบคุมเสียงในคอลัมน์ • ลากสลับลำดับได้):"
            : L"🎵 Group Audio Tracks (Controls in columns • Drag to reorder):";
    }
    static const wchar_t* ColAudioPlay() {
        return (current == Language::Thai) ? L"เล่น" : L"Play";
    }
    static const wchar_t* ColAudioPause() {
        return (current == Language::Thai) ? L"พัก" : L"Pause";
    }
    static const wchar_t* ColAudioStop() {
        return (current == Language::Thai) ? L"หยุด" : L"Stop";
    }
    static const wchar_t* ColAudioDuration() {
        return (current == Language::Thai) ? L"ความยาว" : L"Duration";
    }
    static const wchar_t* ColAudioFilename() {
        return (current == Language::Thai) ? L"ชื่อไฟล์เสียง" : L"Audio Filename";
    }
    static const wchar_t* ColAudioSource() {
        return (current == Language::Thai) ? L"แหล่งที่มา" : L"Source";
    }
    static const wchar_t* BtnAddAudio() {
        return (current == Language::Thai) ? L"➕ เพิ่มไฟล์เสียง..." : L"➕ Add Audio...";
    }
    static const wchar_t* BtnDelAudio() {
        return (current == Language::Thai) ? L"🗑️ ลบไฟล์เสียง" : L"🗑️ Remove Track";
    }
    static const wchar_t* BtnMoveUp() {
        return (current == Language::Thai) ? L"⬆️ ขึ้น" : L"⬆️ Move Up";
    }
    static const wchar_t* BtnMoveDown() {
        return (current == Language::Thai) ? L"⬇️ ลง" : L"⬇️ Move Down";
    }

    // Status Values & Badges
    static const wchar_t* StatusActive() {
        return (current == Language::Thai) ? L"🟢 เปิด" : L"🟢 Active";
    }
    static const wchar_t* StatusInactive() {
        return (current == Language::Thai) ? L"🔴 ปิด" : L"🔴 Inactive";
    }
    static const wchar_t* BtnPlay() {
        return (current == Language::Thai) ? L"▶️ เล่น" : L"▶️ Play";
    }
    static const wchar_t* BtnPlaying() {
        return (current == Language::Thai) ? L"🔊 เล่นอยู่" : L"🔊 Playing";
    }
    static const wchar_t* BtnPause() {
        return (current == Language::Thai) ? L"⏸️ พัก" : L"⏸️ Pause";
    }
    static const wchar_t* BtnResume() {
        return (current == Language::Thai) ? L"▶️ เล่นต่อ" : L"▶️ Resume";
    }
    static const wchar_t* BtnStop() {
        return (current == Language::Thai) ? L"⏹️ หยุด" : L"⏹️ Stop";
    }
    static const wchar_t* SourcePreset() {
        return (current == Language::Thai) ? L"📁 ใน Preset" : L"📁 In Preset";
    }
    static const wchar_t* SourceExternal() {
        return (current == Language::Thai) ? L"🔗 ลิงก์ภายนอก" : L"🔗 Linked File";
    }

    // Days of week
    static const wchar_t* DaySun() { return (current == Language::Thai) ? L"อา." : L"Sun"; }
    static const wchar_t* DayMon() { return (current == Language::Thai) ? L"จ." : L"Mon"; }
    static const wchar_t* DayTue() { return (current == Language::Thai) ? L"อ." : L"Tue"; }
    static const wchar_t* DayWed() { return (current == Language::Thai) ? L"พ." : L"Wed"; }
    static const wchar_t* DayThu() { return (current == Language::Thai) ? L"พฤ." : L"Thu"; }
    static const wchar_t* DayFri() { return (current == Language::Thai) ? L"ศ." : L"Fri"; }
    static const wchar_t* DaySat() { return (current == Language::Thai) ? L"ส." : L"Sat"; }
    static const wchar_t* EveryDay() { return (current == Language::Thai) ? L"ทุกวัน" : L"Daily"; }
    static const wchar_t* MonToFri() { return (current == Language::Thai) ? L"จันทร์-ศุกร์" : L"Mon - Fri"; }

    // Settings Panel
    static const wchar_t* SettingsGeneralGroup() {
        return (current == Language::Thai) ? L"การตั้งค่าระบบ (General Settings)" : L"System Settings";
    }
    static const wchar_t* SettingsAutoStartChk() {
        return (current == Language::Thai) ? L"เปิดโปรแกรมอัตโนมัติเมื่อเปิดเครื่อง (Start automatically when Windows boots)" : L"Start Audio Schedule automatically when Windows starts";
    }
    static const wchar_t* SettingsAutoStartDesc() {
        return (current == Language::Thai)
            ? L"เมื่อเปิดใช้งาน ระบบจะลงทะเบียนเปิด Audio Schedule อัตโนมัติใน Windows Startup Registry"
            : L"When enabled, Audio Schedule will launch automatically in the background on system startup.";
    }
    static const wchar_t* SettingsLanguageGroup() {
        return (current == Language::Thai) ? L"ภาษาของโปรแกรม (Language)" : L"Display Language";
    }
    static const wchar_t* SettingsLangPrompt() {
        return (current == Language::Thai) ? L"เลือกภาษาที่ต้องการใช้งานในโปรแกรม:" : L"Select user interface language:";
    }
    static const wchar_t* SettingsRadioEn() {
        return L"English (Default)";
    }
    static const wchar_t* SettingsRadioTh() {
        return L"ภาษาไทย (Thai)";
    }
    static const wchar_t* SettingsOverlapGroup() {
        return (current == Language::Thai) ? L"โหมดการเล่นเมื่อเวลาชนกัน (Playback on Conflict)" : L"Conflict / Overlap Playback Mode";
    }
    static const wchar_t* SettingsOverlapQueue() {
        return (current == Language::Thai) ? L"เล่นตามลำดับคิว (Queue / Sequential - ค่าเริ่มต้น)" : L"Queue / Sequential (Default - Play one after another)";
    }
    static const wchar_t* SettingsOverlapSimultaneous() {
        return (current == Language::Thai) ? L"เล่นซ้อนพร้อมกัน (Simultaneous / Multi-channel)" : L"Simultaneous / Multi-channel (Play all together)";
    }
    static const wchar_t* SettingsOverlapDesc() {
        return (current == Language::Thai)
            ? L"เมื่อถึงเวลาที่หลายกลุ่มตั้งเวลาตรงกัน: เลือกเล่นต่อคิวกันเพื่อไม่ให้เสียงตีกัน หรือเล่นซ้อนพร้อมกัน"
            : L"When multiple groups trigger at the same time: play in queue to avoid sound clash, or mix simultaneously.";
    }
    static const wchar_t* SettingsAppInfoGroup() {
        return (current == Language::Thai) ? L"ข้อมูลโปรแกรมและพื้นที่จัดเก็บ (Application & Storage Info)" : L"Application & Storage Info";
    }
    static const wchar_t* SettingsVersionLabel() {
        return (current == Language::Thai) ? L"เวอร์ชันของโปรแกรม:" : L"Application Version:";
    }
    static const wchar_t* SettingsPublisherLabel() {
        return (current == Language::Thai) ? L"ผู้พัฒนา:" : L"Publisher / Developer:";
    }
    static const wchar_t* SettingsStorageLabel() {
        return (current == Language::Thai) ? L"ตำแหน่งโฟลเดอร์ Presets:" : L"Presets Storage Location:";
    }
    static const wchar_t* BtnOpenAppFolder() {
        return (current == Language::Thai) ? L"📂 เปิดโฟลเดอร์โปรแกรม" : L"📂 Open App Folder";
    }
    static const wchar_t* BtnOpenPresetsFolder() {
        return (current == Language::Thai) ? L"📁 เปิดโฟลเดอร์ Presets" : L"📁 Open Presets Folder";
    }
    static const wchar_t* SettingsSavedNotice() {
        return (current == Language::Thai) ? L"💾 บันทึกการตั้งค่าเรียบร้อยแล้ว" : L"💾 Settings saved successfully";
    }
    static const wchar_t* SettingsDialogTitle() {
        return (current == Language::Thai) ? L"ตั้งค่าระบบ (Settings)" : L"Settings";
    }
    static const wchar_t* BtnSave() {
        return (current == Language::Thai) ? L"💾 บันทึก" : L"💾 Save";
    }
    static const wchar_t* BtnCancel() {
        return (current == Language::Thai) ? L"ยกเลิก" : L"Cancel";
    }

    // Dialogs & Messages
    static const wchar_t* ConfirmDeleteGroupTitle() {
        return (current == Language::Thai) ? L"ยืนยันการลบกลุ่ม" : L"Confirm Delete Group";
    }
    static const wchar_t* ConfirmDeleteGroupMsg() {
        return (current == Language::Thai)
            ? L"คุณแน่ใจหรือไม่ว่าต้องการลบกลุ่มเวลานี้?"
            : L"Are you sure you want to delete this schedule group?";
    }
    static const wchar_t* ConfirmDeleteAudioTitle() {
        return (current == Language::Thai) ? L"ยืนยันการลบไฟล์เสียง" : L"Confirm Delete Audio";
    }
    static const wchar_t* ConfirmDeleteAudioMsg() {
        return (current == Language::Thai)
            ? L"คุณแน่ใจหรือไม่ว่าต้องการลบไฟล์เสียงนี้ออกจากกลุ่ม?"
            : L"Are you sure you want to remove this audio track from the group?";
    }
    static const wchar_t* PromptNewPresetTitle() {
        return (current == Language::Thai) ? L"สร้างชุดการเล่นใหม่" : L"Create New Preset";
    }
    static const wchar_t* PromptSaveAsTitle() {
        return (current == Language::Thai) ? L"บันทึกชุดการเล่นเป็น..." : L"Save Preset As...";
    }
    static const wchar_t* ReadyStatus() {
        return (current == Language::Thai) ? L"พร้อมใช้งาน" : L"Ready";
    }
    static const wchar_t* NoPresetSelected() {
        return (current == Language::Thai) ? L"ยังไม่ได้เลือกชุดการเล่น" : L"No preset selected";
    }
    static const wchar_t* DialogTitleWarning() {
        return (current == Language::Thai) ? L"แจ้งเตือน" : L"Warning";
    }
    static const wchar_t* DialogTitleError() {
        return (current == Language::Thai) ? L"ข้อผิดพลาด" : L"Error";
    }

    // System Tray & Background Notifications
    static const wchar_t* TrayOpen() {
        return (current == Language::Thai) ? L"📂 เปิดหน้าต่างโปรแกรม" : L"📂 Open Audio Schedule";
    }
    static const wchar_t* TrayStop() {
        return (current == Language::Thai) ? L"⏹️ หยุดเล่นเสียง" : L"⏹️ Stop Playback";
    }
    static const wchar_t* TrayExit() {
        return (current == Language::Thai) ? L"❌ ออกจากโปรแกรม" : L"❌ Exit";
    }
    static const wchar_t* TrayMinimizedTitle() {
        return (current == Language::Thai) ? L"Audio Schedule ทำงานอยู่เบื้องหลัง" : L"Audio Schedule is Running";
    }
    static const wchar_t* TrayMinimizedMsg() {
        return (current == Language::Thai)
            ? L"โปรแกรมถูกย่อลง System Tray และยังคงทำงานตามกำหนดเวลา\nดับเบิลคลิกหรือคลิกที่ไอคอนเพื่อเปิดหน้าต่างอีกครั้ง"
            : L"Audio Schedule minimized to the System Tray.\nDouble-click or click the icon to open.";
    }
    static const wchar_t* TrayTriggerTitle() {
        return (current == Language::Thai) ? L"🔔 ถึงเวลาเล่นเสียง" : L"🔔 Scheduled Audio Triggered";
    }
    static const wchar_t* TrayTriggerMsg() {
        return (current == Language::Thai) ? L"กำลังเริ่มเล่นกลุ่ม: " : L"Starting group: ";
    }
};
