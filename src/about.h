#pragma once

static const char ABOUT_JSON[] =
u8"{\r\n"
u8"  \"product\": \"FlexiSoft Runtime\",\r\n"
u8"  \"version\": \"1.0-beta1\",\r\n"
u8"\r\n"
u8"  \"description_en\": \"Application for detection of Flexi Soft safety input faults and controlled recovery using RK512 reset commands.\",\r\n"
u8"  \"description_cz\": \"Aplikace pro detekci chyb bezpečnostních vstupů Flexi Soft a jejich automatické obnovení pomocí řízeného resetu přes komunikaci RK512.\",\r\n"
u8"  \"description_uk\": \"Програма для виявлення помилок входів безпеки Flexi Soft та їх автоматичного відновлення за допомогою керованого скидання через комунікацію RK512.\",\r\n"
u8"  \"description_fr\": \"Application destinée à détecter les défauts des entrées de sécurité Flexi Soft et à effectuer une restauration contrôlée à l’aide de commandes de réinitialisation RK512.\",\r\n"
u8"  \"description_de\": \"Anwendung zur Erkennung von Fehlern an Flexi Soft-Sicherheitseingängen und zur kontrollierten Wiederherstellung mithilfe von RK512-Reset-Befehlen.\",\r\n"
u8"\r\n"
u8"  \"build_date\": \"" __DATE__ "\",\r\n"
u8"\r\n"
#ifdef _DEBUG
u8"  \"build_type\": \"Debug Win32 MTd\",\r\n"
#else
u8"  \"build_type\": \"Release Win32 MT\",\r\n"
#endif
u8"  \"build_toolset\": \"v141_xp\",\r\n"
u8"  \"target\": \"Windows XP SP3 32bit - Windows 11\",\r\n"
u8"\r\n"
u8"  \"author\": \"KVLab - Vladimír Kopal\",\r\n"
u8"  \"email\": \"vladakopal@gmail.com\",\r\n"
u8"  \"homepage\": \"\",\r\n"
u8"\r\n"
u8"  \"copyright\": \"© 2026 KVLab\"\r\n"
u8"}\r\n";