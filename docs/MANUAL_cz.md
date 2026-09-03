# FlexiSoft Runtime — manuál pro integraci a servis

Tento manuál navazuje na README a popisuje praktické nastavení FlexiSoft Runtime. Je určený hlavně pro integraci se stávajícím strojem, úpravu konfigurace, přípravu textů hlášení a základní diagnostiku.

FlexiSoft Runtime je obslužná servisní aplikace. Není součástí bezpečnostní funkce stroje. Bezpečné vyhodnocení a reset bezpečnostní logiky musí vždy zajišťovat Flexi Soft projekt a zapojení stroje.

---

## 1. Princip

Runtime pravidelně čte stavové bity z Flexi Soft přes RK512. Z těchto bitů vyhodnotí sledované kanály a podle jejich stavu zobrazuje hlášení obsluze.

Pokud obsluha po kontrole potvrdí reset, runtime zapíše nakonfigurovaný RS-232 command bit. Flexi Soft projekt z tohoto bitu vytvoří krátký resetovací impuls.

Základní tok:

```text
FlexiSoftRuntime.exe
    -> RK512
    -> RS-232 nebo TCP/RS-232 převodník
    -> Flexi Soft CPU
    -> logika projektu
    -> resetovací relé / výstup
```

---

## 2. Důležité soubory

| Soubor / složka | Význam |
| --- | --- |
| `conf/config.json` | Hlavní konfigurace runtime: komunikace, kanály, výstupy, texty hlášení, logování. |
| `conf/languages.json` | Překlady UI, názvy jazyků a volitelné fonty. |
| `conf/runtime_state.json` | Poslední runtime volby uživatele, například posledně vybraný jazyk. |
| `docs/` | Lokalizovaná dokumentace. |
| `docs/assets/` | Obrázky použité v dokumentaci. |
| `fonts/` | Volitelné fonty. |
| `flexi_runtime.log` | Servisní log samotné aplikace. |

Metadata okna About nejsou uložená v samostatném runtime souboru. Název produktu, verze, lokalizované popisy (EN/CZ/UK/FR/DE), datum buildu, typ buildu, toolset a cílový systém jsou při buildu zkompilované do `FlexiSoftRuntime.exe` ze souboru `src/about.h`. Jejich změna proto vyžaduje nový build; znovunačtení konfigurace údaje v About nemění.

Praktické ruční nasazení tedy nepotřebuje samostatný soubor s metadaty verze. Základní struktura je:

```text
FlexiSoftRuntime.exe
FlexiSoftMdReader.exe          # doporučený prohlížeč dokumentace
conf/
  config.json
  languages.json
  runtime_state.json          # volitelný; runtime ho podle potřeby vytvoří/aktualizuje
docs/
fonts/                        # volitelné; potřeba jen při použití fontu ze souboru
```

Běžná obsluha tyto soubory neupravuje.

---

## 3. Konfigurace `conf/config.json`

Soubor `conf/config.json` je hlavní strojní konfigurace. Doporučený formát je UTF-8.

Řetězce v JSON mohou být zapsané přímo v UTF-8 nebo pomocí escape `\uXXXX`. Podporované jsou i platné UTF-16 surrogate pairs. Pro konfiguraci a jazykové soubory zůstává preferovaný přímý UTF-8 zápis.

Základní struktura:

```json
{
  "language": "",
  "transport": "",
  "serial": {},
  "network": {},
  "rk512": {},
  "poll": {},
  "inputs": [],
  "ui": {},
  "logging": {},
  "debug": {}
}
```

### 3.1 `language`

```json
"language": "cz"
```

Výchozí jazyk aplikace. Použije se při prvním spuštění nebo pokud není uložený runtime jazyk.

Doporučení:

- základní fallback je `en`,
- čeština používá `cz`,
- ukrajinština používá `uk`,
- další jazyky lze doplnit v `conf/languages.json`.

Runtime změna jazyka nepřepisuje `conf/config.json`.

---

### 3.2 `transport`

```json
"transport": "network"
```

Určuje způsob komunikace.

| Hodnota | Význam |
| --- | --- |
| `serial` | Přímý COM/RS-232 port. |
| `network` | Síťový transport přes TCP nebo UDP. |

Pro integraci do OP/PC stroje je typicky vhodný režim `network` s Ethernet/RS-232 převodníkem.

---

## 4. Sériová komunikace `serial`

Sekce `serial` se používá při:

```json
"transport": "serial"
```

Příklad:

```json
"serial": {
  "port": "COM5",
  "baud": 115200,
  "data_bits": 8,
  "parity": "N",
  "stop_bits": 1,
  "timeout_ms": 2500
}
```

| Parametr | Význam |
| --- | --- |
| `port` | Název COM portu, například `COM4` nebo `COM5`. |
| `baud` | Rychlost sériové linky. |
| `data_bits` | Počet datových bitů. Běžně `8`. |
| `parity` | Parita. Běžně `N`. |
| `stop_bits` | Počet stop bitů. Běžně `1`. |
| `timeout_ms` | Timeout pro čtení a zápis na sériové lince. |

Pro Flexi Soft v tomto projektu se používá běžně:

```text
115200 8N1
```

---

## 5. Síťová komunikace `network`

Sekce `network` se používá při:

```json
"transport": "network"
```

Příklad:

```json
"network": {
  "mode": "tcp_client",
  "host": "192.168.0.7",
  "port": 4001,
  "bind_host": "0.0.0.0",
  "bind_port": 0,
  "connect_timeout_ms": 3000,
  "timeout_ms": 1500
}
```

| Parametr | Význam |
| --- | --- |
| `mode` | Režim síťového transportu. |
| `host` | IP adresa nebo název převodníku. |
| `port` | TCP/UDP port převodníku. |
| `bind_host` | Lokální adresa pro UDP bind. U TCP klienta obvykle není potřeba měnit. |
| `bind_port` | Lokální port pro UDP bind. `0` znamená automaticky. |
| `connect_timeout_ms` | Timeout pro navázání TCP spojení. |
| `timeout_ms` | Timeout pro čtení odpovědi. |

Podporované hodnoty `mode`:

| Hodnota | Význam |
| --- | --- |
| `tcp_client` | Runtime se připojí jako TCP klient k Ethernet/RS-232 převodníku. Doporučený režim. |
| `udp` | UDP režim. Použít jen pokud to odpovídá konkrétnímu převodníku a testům. |

Praktické nasazení na stroji:

```text
OP/PC stroje s runtime
    -> stávající Ethernet stroje
    -> Ethernet/RS-232 převodník
    -> RS-232 Flexi Soft CPU
```

Tím se nemusí tahat dlouhá samostatná RS-232 trasa mezi OP stroje a rozvaděčem.

---

## 6. RK512 `rk512`

Příklad:

```json
"rk512": {
  "device_local": "0x4F",
  "device_reply": "0x4D",
  "token_hex": "0F 0F 46 4C 58 54 30 31"
}
```

| Parametr | Význam |
| --- | --- |
| `device_local` | Lokální RK512 adresa runtime aplikace. |
| `device_reply` | Očekávaná RK512 adresa odpovědi z Flexi Soft. |
| `token_hex` | Token používaný před zápisem commandu. |

V konfiguraci používej `token_hex`.

Hodnoty musí odpovídat ověřené RK512 komunikaci konkrétního projektu.

---

## 7. Polling `poll`

Příklad:

```json
"poll": {
  "period_ms": 1000,
  "read_block": "0x76",
  "read_size": 54
}
```

| Parametr | Význam |
| --- | --- |
| `period_ms` | Perioda čtení stavového bloku. |
| `read_block` | RK512 blok, ze kterého se čtou stavy. |
| `read_size` | Počet čtených bajtů. |

Z načteného bloku runtime vyhodnocuje kanály podle `status_byte`, `on_bit` a `ok_bit`.

---

## 8. Kanály `inputs`

Runtime podporuje až čtyři kanály. Každý kanál je položka v poli `inputs`.

Příklad:

```json
{
  "enabled": true,
  "name": "Sarkofág levá strana",
  "status_byte": 0,
  "on_bit": 0,
  "ok_bit": 1,
  "alert_text_cz": "Zkontroluj dovření sarkofágu vlevo.",
  "output": {
    "block": "0x42",
    "byte": 0,
    "bit": 0,
    "pulse_ms": 1000
  },
  "repeat_fault": {
    "count": 3,
    "window_ms": 30000,
    "ignore_after_command_ms": 1500,
    "text_cz": "Chyba se opakovaně vrací. Zkontroluj kryt a snímač vlevo."
  }
}
```

### 8.1 Základní parametry kanálu

| Parametr | Význam |
| --- | --- |
| `enabled` | Zapnutí/vypnutí kanálu. Vypnutý kanál se nehlídá a nezobrazuje hlášení. |
| `name` | Název kanálu zobrazený v tray menu a hlášeních. |
| `status_byte` | Index bajtu v načteném stavovém bloku. |
| `on_bit` | Informativní ON/OFF bit kanálu. |
| `ok_bit` | Bit, podle kterého runtime rozhodne, jestli je kanál v pořádku. |
| `alert_text_<jazyk>` | Text běžného hlášení pro daný jazyk. |

Starší klíč `alert_text` může sloužit jako fallback, ale doporučené jsou jazykové klíče:

```text
alert_text_en
alert_text_cz
alert_text_uk
alert_text_fr
alert_text_de
```

Pokud aktivní jazyk nemá text, runtime použije angličtinu nebo fallback text.

### 8.2 Výstupní command `output`

```json
"output": {
  "block": "0x42",
  "byte": 0,
  "bit": 0,
  "pulse_ms": 1000
}
```

| Parametr | Význam |
| --- | --- |
| `block` | RK512 blok pro zápis commandu. |
| `byte` | Bajt v zapisovaných datech. |
| `bit` | Bit v daném bajtu. |
| `pulse_ms` | Doba držení command bitu v runtime. |

Runtime při commandu nastaví bit, počká `pulse_ms` a potom bit vypne.

Důležité: i když runtime drží bit jen po omezenou dobu, krátký resetovací impuls má být vytvořený také ve Flexi Soft logice. Tím se zabrání trvalému účinku při výpadku komunikace nebo aplikace.

---

## 9. Opakované hlášení `repeat_fault`

Repeat fault řeší situaci, kdy se stejná chyba po resetovacím impulsu opakovaně vrací.

Příklad:

```json
"repeat_fault": {
  "count": 3,
  "window_ms": 30000,
  "ignore_after_command_ms": 1500,
  "text_cz": "Zkontroluj správné dovření krytu."
}
```

| Parametr | Význam |
| --- | --- |
| `count` | Kolikrát se chyba může vrátit v aktivním okně, než vznikne opakované hlášení. Minimum je `1`. |
| `window_ms` | Časové okno pro počítání opakovaných návratů chyby. Minimum je `1000 ms`. |
| `ignore_after_command_ms` | Ochranný čas po resetovacím commandu. Během této doby runtime ignoruje přechodný stav vstupů způsobený HW resetem přes relé. Záporná hodnota se bere jako `0`. |
| `text_<jazyk>` | Text opakovaného hlášení pro daný jazyk. |

Doporučené jazykové klíče:

```text
text_en
text_cz
text_uk
text_fr
text_de
```

### 9.1 Jak se počítá opakování

Po potvrzení resetu runtime odešle resetovací command a spustí ochranný čas `ignore_after_command_ms`.

Tento čas není určený hlavně k tomu, aby se „nehlásil návrat stejné chyby“. Jeho skutečný účel je odfiltrovat přechodný stav vstupů během HW resetu.

Při resetu jsou sledované vstupní kanály na krátkou dobu rozepnuté pomocí relé. V tomto stavu mohou být oba kanály vstupu vypnuté a Flexi Soft znovu odpočítává svoji dobu souslednosti kanálů. Z pohledu runtime se proto může vstup na chvíli jevit jako OK, i když jde pouze o přechodný stav vyvolaný resetem.

Pokud by runtime tento krátký stav vyhodnotil jako skutečné OK, chybně by resetoval logiku opakovaného hlášení. Proto se během `ignore_after_command_ms` přechodný stav nepropíše do repeat logiky.

Během `ignore_after_command_ms` runtime:

- nepovažuje krátký OK stav za skutečnou opravu vstupu,
- neresetuje počítadlo opakování,
- nezapočítává přechodný stav jako nový návrat chyby,
- neotevírá nové hlášení podle přechodného stavu.

Po doběhnutí `ignore_after_command_ms` runtime znovu vyhodnotí skutečný stav kanálu:

- pokud je kanál stále v chybě, počítadlo opakování se zvýší,
- pokud je kanál po doběhnutí ochranného času skutečně v pořádku, počítadlo opakování se resetuje,
- pokud počítadlo dosáhne hodnoty `count`, zobrazí se opakované hlášení.

Do `window_ms` se počítá čekání obsluhy před potvrzením. Do aktivního okna se nepočítá doba commandu, doba impulsu a ochranný čas `ignore_after_command_ms`.

### 9.2 Aktivní chyby a repeat limit

Počítadla opakování se vedou samostatně pro každý sledovaný kanál. Interně jsou oddělené aktuálně aktivní chyby (`errorMask`) a kanály, které už dosáhly repeat limitu (`repeatMask`). Repeat alert proto může vypsat všechny právě aktivní chyby, zatímco opravný repeat text se zobrazí jen pro kanály obsažené v `repeatMask`.

---

## 10. UI `ui`

Příklad:

```json
"ui": {
  "tray_tooltip": "FlexiSoft RS232 Runtime",
  "tray_tooltip_en": "FlexiSoft RS232 Runtime",
  "tray_tooltip_cz": "FlexiSoft RS232 Runtime",
  "tray_tooltip_uk": "FlexiSoft RS232 Runtime",
  "tray_tooltip_fr": "FlexiSoft RS232 Runtime",
  "tray_tooltip_de": "FlexiSoft RS232 Runtime"
}
```

| Parametr | Význam |
| --- | --- |
| `tray_tooltip` | Fallback text tooltipu tray ikony. |
| `tray_tooltip_<jazyk>` | Lokalizovaný tooltip pro konkrétní jazyk. |

`tray_tooltip` zůstává jako legacy fallback. V aktuální konfiguraci je vhodné používat jazykové klíče `tray_tooltip_<jazyk>`.

Doporučuje se používat jazykové klíče:

```text
tray_tooltip_en
tray_tooltip_cz
tray_tooltip_uk
tray_tooltip_fr
tray_tooltip_de
```

---

## 11. Logování `logging`

Příklad:

```json
"logging": {
  "enabled": true,
  "file": "flexi_runtime.log",
  "newest_first": true,
  "max_bytes": 65536
}
```

| Parametr | Význam |
| --- | --- |
| `enabled` | Zapnutí/vypnutí logování. |
| `file` | Cesta k log souboru. |
| `newest_first` | Pokud je `true`, nejnovější řádky jsou nahoře. |
| `max_bytes` | Přibližná maximální velikost logu. Minimum je `4096`. |

Doporučený produkční režim:

```json
"newest_first": true
```

`newest_first=false` zůstává dostupné jako legacy režim, ale pro produkční nasazení se nedoporučuje.

Log je vždy anglicky. Jazyk UI na něj nemá vliv.

---

## 12. Debug `debug`

Příklad:

```json
"debug": {
  "force_command_fail": false
}
```

| Parametr | Význam |
| --- | --- |
| `force_command_fail` | Testovací volba pro vynucení selhání commandu. V běžném provozu musí být `false`. |

Tato sekce slouží pouze pro testování a diagnostiku.

---

## 13. Runtime stav `conf/runtime_state.json`

`conf/runtime_state.json` ukládá poslední runtime volbu uživatele, aktuálně hlavně jazyk.

Příklad:

```json
{
  "language": "cz"
}
```

Pokud soubor chybí, použije se jazyk z `conf/config.json`. Pokud je soubor poškozený, runtime zapíše warning do logu a použije jazyk z `conf/config.json`.

Runtime změna jazyka nepřepisuje `conf/config.json`.

Pokud se zápis `runtime_state.json` nepodaří, runtime pouze zapíše warning do logu. Už zvolený jazyk UI zůstane aktivní a nevrací se zpět.

---

## 14. Překlady `conf/languages.json`

Soubor `conf/languages.json` obsahuje překlady UI a volitelné nastavení fontu.

Základní struktura:

```json
{
  "languages": {
    "en": {
      "name": "",
      "font_face": "",
      "font_file": "",
      "strings": {}
    }
  }
}
```

### 14.1 `languages`

Objekt `languages` obsahuje jednotlivé jazyky. Klíč jazyka je současně jeho kód, například:

```text
en
cz
uk
fr
de
```

### 14.2 Parametry jazyka

| Parametr | Význam |
| --- | --- |
| `name` | Název jazyka zobrazený v tray menu. |
| `font_face` | Název systémového fontu nebo font face. |
| `font_file` | Volitelný font soubor, relativně ke složce aplikace. |
| `strings` | Překladové klíče UI. |

Pokud `font_file` existuje a jde načíst, runtime použije tento font. Pokud ne, použije fallback font. To je důležité hlavně pro jazyky, které na starších Windows nemusí mít vhodný systémový font.

### 14.3 Fallback pravidla překladů

- Angličtina (`en`) slouží jako základní fallback.
- Pokud aktivní jazyk neobsahuje některý text, použije se anglický text.
- Pokud text není ani v angličtině, zobrazí se název klíče.
- Chybějící nebo prázdný `conf/languages.json` může runtime nahradit základní anglickou šablonou.

### 14.4 Důležité překladové klíče

#### Tray menu

| Klíč | Význam |
| --- | --- |
| `tray.title` | Titulek tray menu. |
| `tray.reload_config` | Položka pro znovunačtení konfigurace. |
| `tray.reconnect` | Položka pro reconnect komunikace. |
| `tray.about` | Položka About. |
| `tray.exit` | Ukončení programu. |
| `tray.language` | Podmenu jazyků. |
| `tray.send_command` | Text ručního commandu; podporuje token `{channel}`. |

#### Stavové texty

| Klíč | Význam |
| --- | --- |
| `status.ok` | Stav OK. |
| `status.error` | Stav chyba. |
| `status.on` | Stav ON. |
| `status.off` | Stav OFF. |
| `status.disabled` | Vypnutý kanál. |

#### Hlášení

| Klíč | Význam |
| --- | --- |
| `alert.input.window_title` | Titulek okna běžného hlášení. |
| `alert.repeat.window_title` | Titulek okna opakovaného hlášení. |
| `alert.command_failed.window_title` | Titulek okna selhaného commandu. |
| `alert.input.title` | Nadpis běžného hlášení. |
| `alert.repeat.title` | Nadpis opakovaného hlášení. |
| `alert.command_failed.title` | Nadpis hlášení selhaného commandu. |
| `alert.repeat.prefix` | Úvodní text opakovaného hlášení. |
| `alert.active_errors` | Nadpis seznamu aktivních chyb. |
| `alert.command_failed.text` | Text selhaného commandu. |
| `alert.affected_channels` | Nadpis seznamu dotčených kanálů. |
| `alert.input.footer_yes` | Vysvětlení tlačítka YES/ANO u běžného hlášení. |
| `alert.input.footer_no` | Vysvětlení tlačítka NO/NE u běžného hlášení. |
| `alert.command_failed.footer_yes` | Vysvětlení YES/ANO u selhaného commandu. |
| `alert.command_failed.footer_no` | Vysvětlení NO/NE u selhaného commandu. |

#### Tlačítka

| Klíč | Význam |
| --- | --- |
| `button.yes` | Text tlačítka Ano / YES. |
| `button.no` | Text tlačítka Ne / NO. |
| `button.ok` | Text tlačítka OK. |

#### About a dokumentace

| Klíč | Význam |
| --- | --- |
| `about.title` | Titulek About dialogu. |
| `about.version` | Popisek verze. |
| `about.build_date` | Popisek data buildu. |
| `about.open_readme` | Tlačítko pro otevření README. |
| `about.open_manual` | Tlačítko pro otevření MANUAL. |
| `about.open_log` | Tlačítko pro otevření LOGu. |
| `about.project_page` | Tlačítko stránky projektu. |
| `error.missing_readme` | Chyba při chybějícím README. |
| `error.missing_manual` | Chyba při chybějícím MANUALu. |
| `error.missing_log` | Chyba při chybějícím logu. |

---

## 15. Přidání dalšího jazyka

Postup:

1. Do `conf/languages.json` přidej nový jazykový objekt, například `pl`.
2. Vyplň `name`, `font_face`, případně `font_file`.
3. Doplň texty ve `strings`.
4. Do `conf/config.json` lze nastavit `"language": "pl"`, nebo jazyk vybrat za běhu z tray menu.
5. Pro dokumentaci přidej odpovídající soubory, například:

```text
docs/README_pl.md
docs/MANUAL_pl.md
```

---

## 16. Dokumentace

Runtime otevírá dokumentaci podle aktivního jazyka.

Základní dokumentace:

```text
docs/README.md
docs/MANUAL.md
```

Lokalizovaná dokumentace:

```text
docs/README_cz.md
docs/MANUAL_cz.md
docs/README_uk.md
docs/MANUAL_uk.md
docs/README_fr.md
docs/MANUAL_fr.md
docs/README_de.md
docs/MANUAL_de.md
```

Pokud lokalizovaný soubor chybí, použije se základní soubor bez jazykového suffixu.

Dokumentace se běžně otevírá externím `FlexiSoftMdReader.exe` (v aktuálním Runtime balíčku v1.0.1). Runtime předává Readeru vybraný dokument a efektivní font UI. Reader v1.0.1 obsahuje kompatibilní obsluhu pro starý IE engine používaný ve Windows XP. Pokud Reader nelze použít, runtime přejde na Notepad a následně na ShellExecute. Reader podporuje Markdown subset používaný dodanou dokumentací včetně obrázků z `docs/assets/`.

---

## 17. Nastavení ve Flexi Soft Designeru

### 17.1 RS-232 routing

V projektu musí být zapnuté:

```text
Aktivovat routing RS-232 pro CPU
```

![Nastavení RS-232 routing pro CPU](assets/flexisoft_designer_rs232_routing.png)

### 17.2 Krátký impuls v logice CPU

Runtime zapisuje command bit, ale samotný krátký impuls má být vytvořený i ve Flexi Soft projektu.

Doporučený princip:

```text
RS-232 bit -> detekce hrany -> časovač / krátký impuls -> resetovací relé nebo resetovací výstup
```

![Příklad resetovacích relé z RS-232 bitů](assets/flexisoft_designer_rs232_reset_relays.png)

Důvod: při výpadku komunikace nebo pádu runtime nesmí vzniknout trvalé odpojení sledovaných vstupů ani trvale aktivní resetovací signál.

---

## 18. Znovunačtení konfigurace

Z tray menu lze znovu načíst konfiguraci.

Znovu se načítá:

- `conf/config.json`,
- `conf/languages.json`,
- `conf/runtime_state.json`.

Metadata okna About jsou součástí EXE a znovunačtení konfigurace je nemění.

Pokud je otevřené hlášení, runtime ho zavře. Pokud chyba stále trvá, zobrazí nové hlášení podle aktuální konfigurace a aktuálních textů.

Reconnect používá stejný modeless-alert flow: staré hlášení se před reconnectem zavře a pokud chyba po obnovení komunikace stále trvá, zobrazí se nové aktuální hlášení.

---

## 19. Tray ikona a tray menu

Tray ikona ukazuje celkový stav komunikace a kanálů:

| Stav ikony | Význam |
| --- | --- |
| Zelená | Komunikace je v pořádku a žádný sledovaný kanál není v chybě. |
| Červená | Komunikace je v pořádku, ale alespoň jeden sledovaný kanál je v chybě. |
| Žlutá / neutrální stav | Start, reconnect nebo čekání na potvrzení stavu. |
| Šedá / chyba komunikace | Runtime nemá platné spojení s Flexi Soft CPU. |

Tray menu zobrazuje stav povolených kanálů CH1 až CH4. U každého kanálu je vidět jeho název a stav ON/OFF a OK/CHYBA.

Ruční commandy jsou dostupné pouze při platné komunikaci s CPU.

---

## 20. Logování a diagnostika

Log je servisní log aplikace, ne výrobní ani bezpečnostní log stroje.

Loguje se například:

- start aplikace,
- načtení konfigurace,
- otevření transportu,
- chyba komunikace,
- obnovení komunikace,
- zobrazení hlášení,
- command ON/OFF,
- selhání commandu,
- opakované hlášení,
- reconnect,
- shutdown.

Při trvající stejné komunikační chybě runtime omezuje zahlcení logu. Stejná chyba se neloguje v každém cyklu, pokud se nemění stav ani text chyby.

---

## 21. Troubleshooting

### Runtime se nepřipojí

Zkontroluj:

- typ transportu,
- IP adresu a port převodníku,
- COM port,
- sériové parametry,
- kabeláž,
- napájení převodníku,
- RS-232 routing ve Flexi Soft projektu,
- RK512 adresy.

### Převodník je dostupný, ale Flexi Soft neodpovídá

Zkontroluj:

- zda je Flexi Soft CPU v RUN,
- zda je správně připojené RS-232 rozhraní,
- zda sedí RK512 nastavení,
- zda sedí čtený blok a velikost,
- zda projekt obsahuje potřebné signály.

### Ruční command je zašedlý

Runtime nemá platnou komunikaci s Flexi Soft CPU. Nejdřív obnov komunikaci nebo zkontroluj převodník.

### Hlášení se po resetu vrací

Sledovaný vstup je po doběhnutí ochranného času stále v chybě. Zkontroluj kryt/sarkofág, snímače a kabeláž.

### Opakované hlášení

Chyba se po resetu vrací opakovaně. Obsluha nemá pokračovat v odklikávání bez fyzické kontroly. Je potřeba kontrola krytu, snímačů, kabeláže nebo bezpečnostní logiky.

### Command selhal

Nepodařilo se provést zápis command bitu nebo jeho vypnutí. Zkontroluj komunikaci, RK512 write blok, token a RS-232 routing.

---

## 22. Kontrolní seznam nasazení

Před předáním ověř:

- RS-232 routing pro CPU je zapnutý.
- RS-232 command bity jsou zapojené v logice Flexi Soft.
- Resetovací impuls je vytvořený ve Flexi Soft jako krátký impuls.
- Komunikace odpovídá nastavení v `conf/config.json`.
- Čtený blok a velikost odpovídají projektu.
- Stavové bajty a bity odpovídají projektu.
- Výstupní command bity odpovídají projektu.
- Texty hlášení jsou srozumitelné pro obsluhu.
- Opakované hlášení jasně říká, kdy volat údržbu.
- `ignore_after_command_ms` překrývá jen přechodný HW reset vstupů a neresetuje repeat logiku falešným OK stavem.
- Tray ikona a tray menu odpovídají reálnému stavu.
- Znovunačtení konfigurace načte i upravené jazyky.
- Log je čitelný.

---

## 23. Autor

```text
KVLab - Vladimír Kopal
vladakopal@gmail.com
```
