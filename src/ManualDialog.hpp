#pragma once
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include "Version.h"
#include "PresetManager.hpp"

class ManualDialog {
public:
    static void Show(HWND hParent) {
        ManualDialog dlg(hParent);
        dlg.DoModal();
    }

private:
    HWND m_hParent;
    bool m_isRunning = false;

    HWND m_hwndDlg = NULL;
    HWND m_hListCategories = NULL;
    HWND m_hEditContent = NULL;
    HWND m_hBtnClose = NULL;

    HFONT m_hFontTitle = NULL;
    HFONT m_hFontSub = NULL;
    HFONT m_hFontBold = NULL;
    HFONT m_hFontNormal = NULL;
    HFONT m_hFontContent = NULL;

    enum {
        IDC_LIST_CATS = 4001,
        IDC_EDIT_CONTENT = 4002,
        IDC_BTN_CLOSE = IDOK
    };

    struct ManualTopic {
        std::wstring title;
        std::wstring content;
    };

    std::vector<ManualTopic> m_topics;

    explicit ManualDialog(HWND hParent) : m_hParent(hParent) {
        InitTopics();
    }

    void InitTopics() {
        m_topics.push_back({
            L"🌟 1. ภาพรวมและการเริ่มต้น",
            L"============================================================\r\n"
            L"  🌟 ภาพรวมโปรแกรม Audio Schedule\r\n"
            L"============================================================\r\n\r\n"
            L"โปรแกรม Audio Schedule พัฒนาขึ้นสำหรับควบคุมและสั่งเล่นไฟล์เสียงอัตโนมัติตามตารางเวลาที่กำหนด รองรับการใช้งานในหลากหลายสถานที่ เช่น โรงเรียน (เพลงชาติ, ออดเข้า-ออกเรียน), โรงงาน/สำนักงาน (สัญญาณเตือนเริ่มงาน, เวลาพักเบรก), วัด/ชุมชน (เสียงตามสาย, รายการวิทยุชุมชน) และห้างร้านต่างๆ\r\n\r\n"
            L"โครงสร้างหน้าจอหลัก (Split-View Interface):\r\n"
            L"------------------------------------------------------------\r\n"
            L"1. แถบด้านบน (Header Bar):\r\n"
            L"   • นาฬิกา Real-time: แสดงเวลาปัจจุบันของเครื่องระดับวินาที\r\n"
            L"   • Timezone: แสดงเขตเวลาของเครื่อง เช่น (UTC+07:00) Bangkok\r\n"
            L"   • นับเวลาถอยหลัง (Countdown): แจ้งกลุ่มถัดไปที่จะเล่นพร้อมเวลานับถอยหลัง\r\n"
            L"   • กล่องเลือกชุด Preset: สลับชุดการทำงานได้ทันที\r\n"
            L"   • ปุ่มจัดการชุด (⚙️ จัดการ ▾): สร้างชุดใหม่, เปลี่ยนชื่อ, หรือลบชุด\r\n"
            L"   • สถานะบันทึก: แสดง '💾 บันทึกแล้ว' หรือ '⚠️ ยังไม่ได้บันทึก'\r\n\r\n"
            L"2. ฝั่งซ้าย (Left Panel) - รายการกลุ่มเวลา (Schedule Groups):\r\n"
            L"   • ตารางกลุ่มเวลา พร้อมปุ่มทดสอบเล่นเสียงทันที (▶️ เล่น / 🔊 เล่นอยู่)\r\n"
            L"   • สวิตช์สถานะเปิด/ปิด (🟢 เปิด / 🔴 ปิด) สลับการทำงานได้ด้วยคลิกเดียว\r\n"
            L"   • คอลัมน์เวลาเริ่ม, วันทำงาน, ชื่อกลุ่ม และจำนวนไฟล์เสียง\r\n\r\n"
            L"3. ฝั่งขวา (Right Panel) - รายการไฟล์เสียงในกลุ่ม (Audio Playlist):\r\n"
            L"   • แสดงเพลงทั้งหมดในกลุ่มที่เลือก พร้อมปุ่ม เล่น/พัก/หยุด แยกรายเพลง\r\n"
            L"   • คำนวณความยาวไฟล์เสียงอัตโนมัติ (เช่น 03:45)\r\n"
            L"   • ระบุแหล่งที่มา (📁 ใน Preset หรือ 🔗 ลิงก์ภายนอก)\r\n"
            L"   • รองรับการลากไฟล์เสียงมาวาง (Drag & Drop) และสลับลำดับเพลง\r\n"
        });

        m_topics.push_back({
            L"📁 2. การจัดการชุดการเล่น (Preset)",
            L"============================================================\r\n"
            L"  📁 การจัดการชุดการเล่น (Preset Management)\r\n"
            L"============================================================\r\n\r\n"
            L"ชุดการเล่น (Preset) คือการรวบรวมตารางเวลาและไฟล์เสียงทั้งหมดไว้เป็นหมวดหมู่ เช่น ชุด 'วันเปิดเรียนปกติ', 'วันสอบปลายภาค', 'วันจัดกิจกรรมพิเศษ'\r\n\r\n"
            L"วิธีสลับชุดการเล่น:\r\n"
            L"------------------------------------------------------------\r\n"
            L"• คลิกที่กล่องเลือกชุด (ComboBox) บนแถบด้านบน แล้วเลือกชุดที่ต้องการ\r\n"
            L"  โปรแกรมจะโหลดตารางเวลาและไฟล์เสียงของชุดนั้นขึ้นมาทันที\r\n\r\n"
            L"ปุ่ม '⚙️ จัดการ ▾' (Manage Preset Dropdown):\r\n"
            L"------------------------------------------------------------\r\n"
            L"กดปุ่ม '⚙️ จัดการ ▾' บนแถบด้านบน จะมีเมนูให้เลือก 3 รายการ:\r\n\r\n"
            L"1. ➕ สร้างชุดการเล่นใหม่... (New Preset / Ctrl+N):\r\n"
            L"   - พิมพ์ชื่อชุดที่ต้องการ เช่น 'ชุดวันหยุดเสาร์อาทิตย์'\r\n"
            L"   - เลือกว่าจะ 'คัดลอกกลุ่มเวลาจากชุดปัจจุบันมาด้วย' หรือไม่ (คัดลอกเพื่อเป็นต้นแบบ หรือเริ่มจากชุดว่างเปล่า)\r\n\r\n"
            L"2. ✏️ เปลี่ยนชื่อชุดนี้... (Rename Preset):\r\n"
            L"   - เปลี่ยนชื่อชุด Preset ปัจจุบันได้อย่างสะดวกรวดเร็ว\r\n"
            L"   - โปรแกรมจะเปลี่ยนชื่อโฟลเดอร์และปรับปรุงพาธไฟล์เสียงให้อัตโนมัติอย่างปลอดภัย\r\n\r\n"
            L"3. 🗑️ ลบชุดการเล่นนี้... (Delete Preset):\r\n"
            L"   - ระบบจะเปิดหน้าต่างตรวจสอบความปลอดภัย (Delete Preset Dialog)\r\n"
            L"   - แสดงรายการไฟล์ที่อยู่ในโฟลเดอร์ของชุดที่จะถูกลบ\r\n"
            L"   - แสดงรายการไฟล์ภายนอกที่จะปลอดภัย 100% ไม่ถูกลบออกจากเครื่อง\r\n\r\n"
            L"วิธีบันทึกชุดการเล่น (💾 บันทึก / Ctrl+S):\r\n"
            L"------------------------------------------------------------\r\n"
            L"• เมื่อมีการแก้ไขรายการ แถบด้านบนจะแสดง '⚠️ สถานะ: ยังไม่ได้บันทึก'\r\n"
            L"• กดปุ่ม '💾 บันทึกชุดปัจจุบัน' หรือกดคีย์ลัด Ctrl + S เพื่อบันทึกทันที\r\n"
            L"• สามารถบันทึกเป็นชื่อใหม่ได้ผ่านเมนู ไฟล์ > บันทึกเป็น (Save As)...\r\n"
        });

        m_topics.push_back({
            L"⏰ 3. การสร้างและตั้งเวลากลุ่มเสียง",
            L"============================================================\r\n"
            L"  ⏰ การสร้างและตั้งเวลากลุ่มเสียง (Schedule Groups)\r\n"
            L"============================================================\r\n\r\n"
            L"วิธีเพิ่มกลุ่มเวลาใหม่ (ปุ่ม ➕ เพิ่มกลุ่ม):\r\n"
            L"------------------------------------------------------------\r\n"
            L"1. คลิกปุ่ม '➕ เพิ่มกลุ่ม' ใต้ตารางฝั่งซ้าย\r\n"
            L"2. กำหนดรายละเอียดในหน้าต่างตั้งค่ากลุ่มเวลา:\r\n"
            L"   • ชื่อกลุ่ม: ตั้งชื่อเพื่อให้เข้าใจง่าย เช่น 'เพลงเคารพธงชาติ', 'ออดพักเที่ยง'\r\n"
            L"   • เวลาเริ่มเล่น: ตั้งเวลาในรูปแบบ ชั่วโมง:นาที:วินาที (ระบบ 24 ชม.)\r\n"
            L"   • วันทำงาน: ติ๊กเลือกวันที่ต้องการให้ทำงาน (จันทร์ - อาทิตย์)\r\n"
            L"     - มีปุ่มลัด 'เลือกทุกวัน' และ 'จันทร์ - ศุกร์' เพื่อความรวดเร็ว\r\n"
            L"   • สลับเปิด/ปิด: กำหนดให้กลุ่มนี้พร้อมทำงานทันทีหรือไม่\r\n"
            L"3. กด 'ตกลง' เพื่อบันทึกกลุ่มเวลา\r\n\r\n"
            L"การเปิด/ปิดการทำงานอย่างรวดเร็ว (One-Click Toggle):\r\n"
            L"------------------------------------------------------------\r\n"
            L"• สามารถคลิกที่ช่องคอลัมน์ 'สถานะ' (🟢 เปิด / 🔴 ปิด) ในตารางฝั่งซ้าย\r\n"
            L"  เพื่อสลับเปิดหรือปิดการทำงานของกลุ่มนั้นได้ทันทีโดยไม่ต้องเปิดแก้ไข\r\n\r\n"
            L"การแก้ไขและลบกลุ่มเวลา:\r\n"
            L"------------------------------------------------------------\r\n"
            L"• แก้ไข: ดับเบิลคลิกที่รายการกลุ่ม หรือเลือกรายการแล้วกดปุ่ม '✏️ แก้ไขกลุ่ม'\r\n"
            L"• ลบ: เลือกรายการกลุ่มแล้วกดปุ่ม '🗑️ ลบกลุ่ม' หรือกดปุ่ม Delete บนคีย์บอร์ด\r\n"
        });

        m_topics.push_back({
            L"🎵 4. จัดการไฟล์เสียงและเล่นเพลง",
            L"============================================================\r\n"
            L"  🎵 การจัดการไฟล์เสียงและควบคุมการเล่น (Audio Playlist)\r\n"
            L"============================================================\r\n\r\n"
            L"วิธีเพิ่มไฟล์เสียงลงในกลุ่ม:\r\n"
            L"------------------------------------------------------------\r\n"
            L"1. คลิกเลือกกลุ่มเวลาที่ต้องการที่ฝั่งซ้าย รายการเพลงของกลุ่มนั้นจะปรากฏฝั่งขวา\r\n"
            L"2. เพิ่มไฟล์เสียงได้ 2 วิธี:\r\n"
            L"   • วิธีที่ 1: กดปุ่ม '➕ เพิ่มไฟล์เสียง...' ฝั่งขวา (เลือกได้หลายไฟล์พร้อมกัน)\r\n"
            L"   • วิธีที่ 2: ลากไฟล์เสียงจาก Windows Explorer มาวางในตารางฝั่งขวาได้โดยตรง\r\n"
            L"3. รูปแบบไฟล์ที่รองรับ: MP3 (.mp3), WAV (.wav), WMA (.wma), M4A (.m4a)\r\n\r\n"
            L"ระบบการเล่นตามลำดับอัตโนมัติ:\r\n"
            L"------------------------------------------------------------\r\n"
            L"• เมื่อถึงเวลาที่ตั้งไว้ โปรแกรมจะเริ่มเล่นไฟล์เสียงลำดับที่ 1 จนจบ\r\n"
            L"• จากนั้นจะเล่นลำดับที่ 2, 3 ต่อเนื่องทันทีจนครบทุกเพลงในกลุ่มโดยอัตโนมัติ\r\n\r\n"
            L"การจัดลำดับไฟล์เสียง:\r\n"
            L"------------------------------------------------------------\r\n"
            L"• เลือกไฟล์เสียงในตาราง แล้วกดปุ่ม '⬆️ ขึ้น' หรือ '⬇️ ลง'\r\n"
            L"• หรือคลิกลากแถวในตารางเพื่อสลับลำดับได้ทันที (Drag & Drop)\r\n\r\n"
            L"การควบคุมการเล่นแบบแมนนวล:\r\n"
            L"------------------------------------------------------------\r\n"
            L"• เล่นทั้งกลุ่ม: กดปุ่ม '▶️ เล่น' ที่คอลัมน์แรกของกลุ่มฝั่งซ้าย (กดซ้ำเพื่อหยุด)\r\n"
            L"• ควบคุมรายเพลงในตารางฝั่งขวา:\r\n"
            L"   - ▶️ เล่น: สั่งเล่นเพลงนี้ทันที\r\n"
            L"   - ⏸️ พัก / ▶️ เล่นต่อ: พักเสียงชั่วคราว หรือเล่นต่อจากตำแหน่งเดิม\r\n"
            L"   - ⏹️ หยุด: สั่งหยุดเล่นเสียง\r\n"
        });

        m_topics.push_back({
            L"📦 5. ระบบสำรองข้อมูล (.asch / ZIP)",
            L"============================================================\r\n"
            L"  📦 ระบบสำรองและกู้คืนข้อมูล (Backup & Restore)\r\n"
            L"============================================================\r\n\r\n"
            L"โปรแกรมมีระบบสำรองข้อมูลระดับสูงผ่านเมนู 'สำรองข้อมูล (&Backup)' ด้านบน:\r\n\r\n"
            L"1. แพ็กเกจชุดการเล่นแบบพกพา (.asch):\r\n"
            L"------------------------------------------------------------\r\n"
            L"• ส่งออก (.asch): รวมการตั้งเวลาและบีบอัดไฟล์เสียงทั้งหมดของชุดนั้นลงในไฟล์เดียว\r\n"
            L"  เหมาะสำหรับนำไปแชร์ให้เพื่อนร่วมงาน หรือย้ายไปเปิดใช้งานบนเครื่องอื่น\r\n"
            L"• นำเข้า (.asch): เลือกไฟล์ .asch เพื่อนำเข้าชุดการตั้งเวลาและไฟล์เสียงเข้าสู่โปรแกรม\r\n\r\n"
            L"2. สำรองข้อมูลทุกชุดเป็นไฟล์ ZIP (.zip):\r\n"
            L"------------------------------------------------------------\r\n"
            L"• ส่งออก ZIP: สำรอง Presets ทุกชุดในโปรแกรมพร้อมไฟล์เสียงทั้งหมดลงในไฟล์ .zip\r\n"
            L"  เหมาะสำหรับการสำรองข้อมูลประจำเดือนหรือก่อนย้ายเครื่องคอมพิวเตอร์ใหม่\r\n"
            L"• กู้คืนจาก ZIP: กู้คืนชุด Presets ทั้งหมดกลับคืนมาในขั้นตอนเดียว\r\n\r\n"
            L"3. เปิดโฟลเดอร์เก็บข้อมูล:\r\n"
            L"------------------------------------------------------------\r\n"
            L"• เลือกเมนู สำรองข้อมูล > 📁 เปิดโฟลเดอร์เก็บข้อมูล Presets\r\n"
            L"  เพื่อเปิดดูโฟลเดอร์ presets/ บนคอมพิวเตอร์โดยตรง\r\n"
        });

        m_topics.push_back({
            L"⌨️ 6. ถาดระบบ คีย์ลัด และ FAQ",
            L"============================================================\r\n"
            L"  ⌨️ ถาดระบบ คีย์ลัด และคำถามที่พบบ่อย (System Tray, Shortcuts & FAQ)\r\n"
            L"============================================================\r\n\r\n"
            L"ระบบทำงานในถาดระบบ (System Tray & Background):\r\n"
            L"------------------------------------------------------------\r\n"
            L"• การกดย่อหน้าต่าง (_): เมื่อกดปุ่มย่อหน้าต่าง โปรแกรมจะย่อลงไปเก็บไว้ที่ถาดระบบ (System Tray / Notification Area บริเวณมุมขวาล่างใกล้นาฬิกา) โดยไม่เกะกะแถบ Taskbar\r\n"
            L"• การทำงานต่อเนื่อง: ขณะย่ออยู่ในถาดระบบ โปรแกรมยังคงนับเวลาถอยหลังและเปิดเสียงตามตารางเวลาอย่างแม่นยำ 100%\r\n"
            L"• บอลลูนแจ้งเตือน (Balloon Notification): เมื่อถึงเวลาเล่นเสียง จะมีข้อความแจ้งเตือนเด้งขึ้นมาบอกชื่อกลุ่มและจำนวนเพลงที่กำลังเล่น สามารถคลิกที่บอลลูนเพื่อเปิดหน้าต่างขึ้นมาได้ทันที\r\n"
            L"• การกู้คืนหน้าต่าง (Restore): คลิกซ้ายหรือดับเบิลคลิกที่ไอคอน Audio Schedule ในถาดระบบ\r\n"
            L"• เมนูคลิกขวาใน Tray: คลิกขวาที่ไอคอนเพื่อเปิดเมนู (เปิดหน้าต่างหลัก, ⏹️ สั่งหยุดเสียงทันทีฉุกเฉิน, ออกจากโปรแกรม)\r\n"
            L"• ปุ่มปิด (X): เป็นการปิดโปรแกรมตามปกติ (หากต้องการให้ทำงานเบื้องหลังให้กดย่อ _)\r\n\r\n"
            L"ตารางคีย์ลัดหลัก (Keyboard Shortcuts):\r\n"
            L"------------------------------------------------------------\r\n"
            L"• Ctrl + S : บันทึกชุดการเล่นปัจจุบันทันที\r\n"
            L"• Ctrl + N : สร้างชุดการเล่นใหม่\r\n"
            L"• Delete   : ลบกลุ่มเวลาที่เลือก (ฝั่งซ้าย) หรือลบไฟล์เสียงที่เลือก (ฝั่งขวา)\r\n"
            L"• Esc      : ปิดหน้าต่าง Popup / Dialog ปัจจุบัน\r\n"
            L"• F1       : เปิดหน้าต่างคู่มือการใช้งานนี้\r\n\r\n"
            L"คำถามที่พบบ่อย (FAQ):\r\n"
            L"------------------------------------------------------------\r\n"
            L"Q: ตัวเลือก 'Clean Install' ในตัวติดตั้ง มีพฤติกรรมอย่างไร?\r\n"
            L"A: หากเลือก Clean Install ตัวติดตั้งจะล้างไฟล์ทั้งหมดในโฟลเดอร์ติดตั้ง รวมถึงไฟล์เสียงและชุดการตั้งค่าเดิมทั้งหมดออกอย่างหมดจด เสมือนการถอนการติดตั้งแล้วติดตั้งใหม่จากศูนย์ เหมาะสำหรับผู้ที่ต้องการเริ่มต้นใหม่ทั้งหมด หากต้องการเก็บข้อมูลเดิมไว้ ให้ติดตั้งตามปกติโดยไม่ต้องเลือก Clean Install\r\n\r\n"
            L"Q: สามารถเปิดโปรแกรมขึ้นมาพร้อมกันหลายหน้าต่างได้หรือไม่?\r\n"
            L"A: โปรแกรมทำงานในรูปแบบ Single Instance หากกดเปิดโปรแกรมซ้ำอีกครั้ง ระบบจะดึงหน้าต่างโปรแกรมที่เปิดอยู่เดิมขึ้นมาด้านหน้าทันที (รวมถึงกรณีที่หน้าต่างซ่อนอยู่ในถาดระบบ System Tray) โดยจะไม่เปิดโปรแกรมซ้ำซ้อน เพื่อป้องกันเสียงหรือตารางเวลาทำงานชนกัน\r\n\r\n"
            L"Q: ถ้ากดย่อหน้าต่างแล้ว เสียงจะยังเล่นตามเวลาหรือไม่?\r\n"
            L"A: เล่นตามปกติครับ เมื่อกดย่อหน้าต่าง โปรแกรมจะลงไปทำงานใน System Tray บริเวณมุมขวาล่าง และส่งสัญญาณเสียงพร้อมแสดงบอลลูนแจ้งเตือนตามเวลาอย่างสมบูรณ์\r\n\r\n"
            L"Q: ถ้าคอมพิวเตอร์ Sleep หรือพักหน้าจอ เสียงจะดังหรือไม่?\r\n"
            L"A: เมื่อ Windows เข้าสู่ Sleep Mode ซีพียูจะหยุดทำงาน แนะนำให้ตั้งค่าใน Windows Power Options ให้ตั้ง Sleep เป็น 'Never' สำหรับเครื่องที่ใช้เป็นระบบเปิดเสียงอัตโนมัติประจำหน่วยงาน\r\n\r\n"
            L"Q: นำโปรแกรมไปเปิดเครื่องอื่น ไฟล์เสียงจะหลุดหรือไม่?\r\n"
            L"A: ไม่หลุดครับ เมื่อกดบันทึกชุดเสียง โปรแกรมจะคัดลอกไฟล์เสียงมาเก็บไว้ในโฟลเดอร์ presets/<ชื่อชุด>/audio/ ให้อัตโนมัติ สามารถคัดลอกโฟลเดอร์โปรแกรมใส่ Flash Drive ไปเปิดใช้งานบนเครื่องอื่นที่ใช้ Windows 10/11 ได้ทันที\r\n"
        });
    }

    bool DoModal() {
        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
        if (!GetClassInfoExW(GetModuleHandle(NULL), L"ATP_ManualDlgClass", &wc)) {
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.lpfnWndProc = DialogProcThunk;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = L"ATP_ManualDlgClass";
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
            RegisterClassExW(&wc);
        }

        int dlgW = 840;
        int dlgH = 640;

        RECT parentRect;
        if (m_hParent && IsWindow(m_hParent)) {
            GetWindowRect(m_hParent, &parentRect);
        } else {
            parentRect.left = 100;
            parentRect.right = 940;
            parentRect.top = 100;
            parentRect.bottom = 740;
        }

        int x = parentRect.left + (parentRect.right - parentRect.left - dlgW) / 2;
        int y = parentRect.top + (parentRect.bottom - parentRect.top - dlgH) / 2;
        if (x < 10) x = 10;
        if (y < 10) y = 10;

        std::wstring dlgTitle = L"📖 คู่มือการใช้งาน " APP_NAME_W L" (User Manual)";
        m_hwndDlg = CreateWindowExW(
            WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
            L"ATP_ManualDlgClass",
            dlgTitle.c_str(),
            WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
            x, y, dlgW, dlgH,
            m_hParent, NULL, GetModuleHandle(NULL), this
        );

        if (!m_hwndDlg) return false;

        if (m_hParent && IsWindow(m_hParent)) {
            EnableWindow(m_hParent, FALSE);
        }

        m_isRunning = true;
        MSG msg;
        while (m_isRunning) {
            BOOL bRet = GetMessageW(&msg, NULL, 0, 0);
            if (bRet == 0) {
                PostQuitMessage((int)msg.wParam);
                break;
            } else if (bRet == -1) {
                break;
            }

            if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
                m_isRunning = false;
                break;
            }

            if (!m_hwndDlg || !IsWindow(m_hwndDlg)) {
                break;
            }

            if (!IsDialogMessageW(m_hwndDlg, &msg)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }

        if (m_hwndDlg && IsWindow(m_hwndDlg)) {
            DestroyWindow(m_hwndDlg);
        }

        if (m_hParent && IsWindow(m_hParent)) {
            EnableWindow(m_hParent, TRUE);
            SetForegroundWindow(m_hParent);
        }

        return true;
    }

    static LRESULT CALLBACK DialogProcThunk(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        ManualDialog* pThis = NULL;
        if (uMsg == WM_NCCREATE) {
            CREATESTRUCTW* pCS = (CREATESTRUCTW*)lParam;
            pThis = (ManualDialog*)pCS->lpCreateParams;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        } else {
            pThis = (ManualDialog*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        }

        if (pThis) {
            return pThis->DialogProc(hwnd, uMsg, wParam, lParam);
        }
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    void OnSelectCategory() {
        int sel = (int)SendMessage(m_hListCategories, LB_GETCURSEL, 0, 0);
        if (sel >= 0 && sel < (int)m_topics.size()) {
            SetWindowTextW(m_hEditContent, m_topics[sel].content.c_str());
            SendMessage(m_hEditContent, EM_SETSEL, 0, 0);
            SendMessage(m_hEditContent, EM_SCROLLCARET, 0, 0);
        }
    }

    LRESULT DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
            case WM_CREATE: {
                m_hwndDlg = hwnd;

                // Create fonts
                m_hFontTitle = CreateFontW(-18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                          CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                m_hFontSub = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                m_hFontBold = CreateFontW(-14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                         CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                m_hFontNormal = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                           CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                m_hFontContent = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

                // Header Title
                std::wstring headerTitle = APP_NAME_W L" - คู่มือการใช้งาน (User Manual)";
                HWND hTitle = CreateWindowW(L"STATIC", headerTitle.c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT,
                                            20, 14, 780, 24, hwnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hTitle, WM_SETFONT, (WPARAM)m_hFontTitle, TRUE);

                // Subtitle
                HWND hSub = CreateWindowW(L"STATIC", L"คำแนะนำการใช้งานโปรแกรม, การตั้งเวลา, การจัดการชุดเสียง และระบบสำรองข้อมูล",
                                          WS_CHILD | WS_VISIBLE | SS_LEFT,
                                          20, 38, 780, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hSub, WM_SETFONT, (WPARAM)m_hFontSub, TRUE);

                // Separator
                CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
                              20, 62, 785, 2, hwnd, NULL, GetModuleHandle(NULL), NULL);

                // Left Panel: Category List Label
                HWND hCatLbl = CreateWindowW(L"STATIC", L"📑 เลือกหัวข้อคู่มือ:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                             20, 72, 230, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hCatLbl, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

                // Left Panel: ListBox
                m_hListCategories = CreateWindowExW(
                    WS_EX_CLIENTEDGE,
                    L"LISTBOX",
                    L"",
                    WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
                    20, 96, 230, 420,
                    hwnd, (HMENU)IDC_LIST_CATS, GetModuleHandle(NULL), NULL
                );
                SendMessage(m_hListCategories, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

                for (const auto& topic : m_topics) {
                    SendMessageW(m_hListCategories, LB_ADDSTRING, 0, (LPARAM)topic.title.c_str());
                }

                // Right Panel: Content Label
                HWND hContLbl = CreateWindowW(L"STATIC", L"📖 คำอธิบายและขั้นตอนการใช้งาน:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                              260, 72, 545, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hContLbl, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

                // Right Panel: Multi-line Read-Only Edit Control
                m_hEditContent = CreateWindowExW(
                    WS_EX_CLIENTEDGE,
                    L"EDIT",
                    L"",
                    WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                    260, 96, 545, 420,
                    hwnd, (HMENU)IDC_EDIT_CONTENT, GetModuleHandle(NULL), NULL
                );
                SendMessage(m_hEditContent, WM_SETFONT, (WPARAM)m_hFontContent, TRUE);

                // Select first category by default
                SendMessage(m_hListCategories, LB_SETCURSEL, 0, 0);
                OnSelectCategory();

                // Bottom Close Button
                m_hBtnClose = CreateWindowW(
                    L"BUTTON", L"ปิด (Close)",
                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                    685, 532, 120, 34, hwnd, (HMENU)IDC_BTN_CLOSE, GetModuleHandle(NULL), NULL
                );
                SendMessage(m_hBtnClose, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

                return 0;
            }

            case WM_CTLCOLORSTATIC: {
                HDC hdc = (HDC)wParam;
                HWND hCtrl = (HWND)lParam;
                if (hCtrl == m_hEditContent) {
                    SetBkColor(hdc, RGB(255, 255, 255));
                    return (LRESULT)GetStockObject(WHITE_BRUSH);
                }
                SetBkMode(hdc, TRANSPARENT);
                return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
            }

            case WM_COMMAND: {
                int id = LOWORD(wParam);
                int code = HIWORD(wParam);

                if (id == IDC_LIST_CATS && code == LBN_SELCHANGE) {
                    OnSelectCategory();
                    return 0;
                } else if (id == IDC_BTN_CLOSE || id == IDCANCEL) {
                    m_isRunning = false;
                    return 0;
                }
                return 0;
            }

            case WM_CLOSE:
                m_isRunning = false;
                return 0;

            case WM_DESTROY:
                if (m_hFontTitle) DeleteObject(m_hFontTitle);
                if (m_hFontSub) DeleteObject(m_hFontSub);
                if (m_hFontBold) DeleteObject(m_hFontBold);
                if (m_hFontNormal) DeleteObject(m_hFontNormal);
                if (m_hFontContent) DeleteObject(m_hFontContent);
                return 0;
        }
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
};
