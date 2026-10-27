#pragma once

// ============================================================================
// Audio Schedule - Version & Application Metadata
// กำหนดและแก้ไขข้อมูลของโปรแกรม (ชื่อโปรเจกต์, เวอร์ชัน, ผู้พัฒนา) ได้ที่นี่ที่เดียว
// ทุกส่วนของโปรแกรม (GUI, About Box, Resource .rc) จะดึงข้อมูลจากไฟล์นี้โดยอัตโนมัติ
// ============================================================================

// --- ข้อมูลชื่อโปรเจกต์และแอปพลิเคชัน (Project & Application Name) ---
#define APP_NAME                "Audio Schedule"
#define APP_NAME_W              L"Audio Schedule"
#define APP_EXE_NAME            "AudioSchedule.exe"
#define APP_EXE_NAME_W          L"AudioSchedule.exe"
#define APP_WINDOW_CLASS_W      L"AudioSchedule_MainClass"
#define APP_BACKUP_ZIP_PREFIX   "AudioSchedule_Backup"
#define APP_BACKUP_ZIP_PREFIX_W L"AudioSchedule_Backup"
#define APP_SINGLE_INSTANCE_MUTEX_W L"Softlunar_AudioSchedule_SingleInstance_Mutex"
#define APP_RESTORE_MSG_W           L"Softlunar_AudioSchedule_RestoreWindow_Msg"

// คำอธิบายโปรแกรม (Description)
#define APP_DESC                "Audio Schedule - Automated Audio Scheduler"
#define APP_DESC_W              L"โปรแกรมตั้งเวลาและเล่นไฟล์เสียงอัตโนมัติ (Automated Audio Scheduler)"

// --- ข้อมูลเลขเวอร์ชัน (Version Numbers) ---
#define APP_VERSION_MAJOR       1
#define APP_VERSION_MINOR       0
#define APP_VERSION_PATCH       0
#define APP_VERSION_BUILD       0

// ข้อความเวอร์ชัน
#define APP_VERSION_STRING      "1.0.0"
#define APP_VERSION_WSTRING     L"1.0.0"

// เลขเวอร์ชัน 4 ตัวสำหรับ Windows Resource (FILEVERSION, PRODUCTVERSION)
#define APP_VERSION_RC          1,0,0,0

// --- ข้อมูลผู้พัฒนา ทีม และเว็บไซต์ (Author, Team & Website) ---
#define APP_AUTHOR              "Mr.Weerapat Nusui"
#define APP_AUTHOR_W            L"Mr.Weerapat Nusui"
#define APP_TEAM                "Softlunar Foundation"
#define APP_TEAM_W              L"Softlunar Foundation"
#define APP_WEBSITE             "softlunar.com"
#define APP_WEBSITE_W           L"softlunar.com"
#define APP_WEBSITE_URL         "https://softlunar.com"
#define APP_WEBSITE_URL_W       L"https://softlunar.com"
#define APP_EMAIL               "weerapat.mailbox@gmail.com"
#define APP_EMAIL_W             L"weerapat.mailbox@gmail.com"
#define APP_COPYRIGHT_RC        "Copyright (c) 2026 Mr.Weerapat Nusui <weerapat.mailbox@gmail.com> & Softlunar Foundation"
#define APP_COPYRIGHT_W         L"Copyright (c) 2026 Mr.Weerapat Nusui <weerapat.mailbox@gmail.com> & Softlunar Foundation. All rights reserved."
