# FlexiSoft Runtime

FlexiSoft Runtime je pomocná servisní aplikace pro SICK Flexi Soft. Vznikla kvůli opakovaným zbytečným zásahům údržby u stroje CRIPPA pro ohýbání trubek, kde se často chodí resetovat stav po špatně dovřeném nebo mechanicky opotřebeném krytu/sarkofágu.

Cíl není nahradit údržbu tam, kde je skutečná závada. Cíl je odfiltrovat běžnou obslužnou chybu: obsluha špatně zavře kryt, Flexi Soft drží chybový stav a údržba musí přijít pouze kvůli resetu. Runtime obsluze srozumitelně řekne, co má zkontrolovat, a umožní jí spustit předem připravený resetovací impuls.

**FlexiSoft Runtime není součást bezpečnostní funkce stroje.** Je to pouze obslužný software. Bezpečné vyhodnocení, bezpečný stav stroje a povolení/reset bezpečnostní logiky musí vždy zajišťovat Flexi Soft projekt a zapojení stroje.

---

## K čemu se používá

Typický případ použití:

1. Obsluha zavře kryt/sarkofág stroje.
2. Kvůli opotřebení, špatnému dovření nebo nesoučasnému sepnutí snímačů zůstane ve Flexi Soft chyba.
3. Runtime zobrazí hlášení s jasným pokynem pro obsluhu.
4. Obsluha zkontroluje kryt a potvrdí resetovací impuls.
5. Runtime zapíše nastavený RS-232 command bit přes RK512.
6. Flexi Soft projekt z tohoto bitu vytvoří krátký resetovací impuls.
7. Pokud se stejná chyba vrací opakovaně, Runtime zobrazí opakované hlášení a obsluha má volat údržbu / provést fyzickou kontrolu.

Runtime tak řeší hlavně opakované, banální a provozně otravné situace, kdy stroj není mechanicky správně dovřený, ale není nutný zásah údržby jen pro provedení resetu.

---

## Co program dělá

- čte vybrané stavy z Flexi Soft přes RK512,
- zobrazuje obsluze hlášení chyb,
- umožňuje potvrdit resetovací impuls,
- hlídá opakovaný návrat stejné chyby,
- při opakované chybě ukáže důraznější opakované hlášení,
- umožňuje ruční command z tray menu,
- podporuje vícejazyčné UI a lokalizovanou dokumentaci,
- zapisuje vlastní provozní log aplikace.

Runtime **neloguje události stroje jako výrobní nebo bezpečnostní záznamník**. Loguje své vlastní události: start, načtení konfigurace, stav komunikace, zobrazené chyby, commandy, reconnect, shutdown a podobné servisní informace.

---

## Komunikace s Flexi Soft

Runtime komunikuje s Flexi Soft přes RK512.

Podporované způsoby připojení:

| Způsob | Použití |
| --- | --- |
| Přímý COM / RS-232 | Runtime běží na PC, které má fyzický nebo virtuální sériový port připojený k Flexi Soft. |
| TCP přes Ethernet/RS-232 převodník | Runtime běží na PC stroje a k Flexi Soft CPU se připojuje přes stávající síťovou infrastrukturu stroje. |

Pro nasazení do stávajícího stroje je praktické použít Ethernet/RS-232 převodník. Runtime pak může běžet přímo na OP/PC stroje, typicky i na starším systému s Windows XP. Komunikace s Flexi Soft CPU může jít přes existující strojní síť místo samostatného dlouhého sériového vedení.

Tím zůstane zásah do stroje minimální:

- Runtime běží na strojním PC/OP.
- Flexi Soft zůstává hlavní bezpečnostní jednotkou.
- Převodník pouze přenáší RK512 komunikaci mezi runtime a RS-232 rozhraním Flexi Soft.
- Stávající síťová infrastruktura stroje může posloužit pro spojení runtime ↔ převodník.

---

## Důležité soubory pro integraci a servis

Běžná obsluha tyto soubory neřeší. Pro integraci a servis jsou důležité hlavně:

| Soubor / složka | Význam |
| --- | --- |
| `conf/config.json` | Komunikace, sledované kanály, texty hlášení, výstupní command bity a repeat fault pravidla. |
| `conf/languages.json` | Překlady UI a volitelné fonty. |
| `conf/runtime_state.json` | Poslední runtime volby uživatele, aktuálně hlavně vybraný jazyk. |
| `FlexiSoftMdReader.exe` | Externí prohlížeč dokumentace používaný Runtime. |
| `fonts/` | Volitelné fonty pro jazyky, které nemusí mít vhodný systémový font. |
| `docs/` | Lokalizovaná dokumentace. |
| `flexi_runtime.log` | Servisní log aplikace. |

Metadata okna About jsou při buildu vložená přímo do `FlexiSoftRuntime.exe` ze `src/about.h`; nejsou uložená v samostatném runtime konfiguračním souboru. Embedded About obsahuje údaje o produktu/verzi/buildu a lokalizované popisy pro EN/CZ/UK/FR/DE.

---

## Vícejazyčné UI

Výchozí jazyk je angličtina. Další jazyky se doplňují v `conf/languages.json`.

Runtime není pevně omezený na několik jazyků. Pokud se doplní překlady a případná dokumentace, je možné přidat další jazyk.

Dokumentace se otevírá podle aktivního jazyka, pokud existuje odpovídající soubor:

```text
docs/README_<jazyk>.md
docs/MANUAL_<jazyk>.md
```

Pokud lokalizovaná dokumentace chybí, použije se základní dokumentace bez jazykové přípony.

---

## Nastavení ve Flexi Soft Designeru

### Povolení RS-232 routing pro CPU

V projektu Flexi Soft musí být zapnuté:

```text
Aktivovat routing RS-232 pro CPU
```

![Nastavení RS-232 routing pro CPU](assets/flexisoft_designer_rs232_routing.png)

Bez tohoto nastavení nemusí být RS-232 signály dostupné pro logiku CPU.

### Resetovací impuls z RS-232 bitu

Runtime pouze zapíše RS-232 command bit. Vlastní resetovací impuls má být vytvořený v logice Flexi Soft CPU.

Doporučený princip:

```text
RS-232 bit -> detekce hrany -> časovač / krátký impuls -> resetovací relé nebo resetovací výstup
```

Příklad:

```text
RS232 0.0 -> detekce hrany -> časovač zpožděného vypnutí -> XTIO[1].Q1
RS232 0.1 -> detekce hrany -> časovač zpožděného vypnutí -> XTIO[1].Q2
```

![Příklad resetovacích relé z RS-232 bitů](assets/flexisoft_designer_rs232_reset_relays.png)

Tato logika je doporučená kvůli bezpečnému chování při poruše komunikace. Pokud by během commandu vypadla komunikace nebo runtime přestal běžet, nesmí dojít ke statickému odpojení sledovaných vstupů nebo k trvale aktivnímu resetovacímu signálu. Krátký impuls má proto zajišťovat Flexi Soft projekt, ne samotný runtime.

Konfigurace v `conf/config.json` musí odpovídat tomu, jak jsou RS-232 command bity použité ve Flexi Soft projektu.

---

## Hlášení

Runtime zobrazuje hlášení podle nakonfigurovaných kanálů.

Běžné hlášení obsluze říká, co má zkontrolovat, například:

```text
Zkontroluj dovření sarkofágu.
Otevři a znovu správně zavři kryt.
Po kontrole potvrď reset.
```

Obsluha má typicky dvě možnosti:

| Volba | Význam |
| --- | --- |
| Ano / YES | Po kontrole spustí resetovací impuls. |
| Ne / NO | Zavře hlášení bez resetovacího impulsu. |

Texty hlášení se nastavují v konfiguraci.

---

## Opakované hlášení

Opakované hlášení se zobrazí, pokud se stejná chyba po resetovacím impulsu vrací opakovaně.

To je důležité hlavně u mechanicky opotřebeného krytu/sarkofágu:

- jednorázová chyba může být jen špatné dovření,
- opakovaná chyba znamená, že kryt, snímač nebo kabeláž potřebují kontrolu,
- obsluha nemá donekonečna potvrzovat reset bez kontroly příčiny.

Příklad smyslu textu opakovaného hlášení:

```text
Chyba se opakovaně vrací.
Zkontroluj správné dovření krytu a stav snímače.
Pokud se hlášení opakuje, volej údržbu.
```

---

## Tray ikona a stav kanálů

Runtime běží v oznamovací oblasti Windows. Barva tray ikony dává rychlou informaci o celkovém stavu aplikace a komunikace.

| Stav ikony | Význam |
| --- | --- |
| Zelená | Komunikace s Flexi Soft CPU je v pořádku a žádný sledovaný kanál není v chybě. |
| Červená | Komunikace je v pořádku, ale alespoň jeden sledovaný kanál hlásí chybu. |
| Žlutá / neutrální | Aplikace běží, ale komunikace nebo stav CPU ještě nejsou plně potvrzené. Typicky při startu, reconnectu nebo při čekání na odpověď. |
| Šedá / chyba komunikace | Runtime nemá platné spojení s Flexi Soft CPU nebo je komunikace v chybě. |

Tray menu zobrazuje i stav sledovaných kanálů CH1 až CH4. U každého povoleného kanálu je vidět jeho název a stav, například jestli je kanál aktivní/neaktivní a jestli je v pořádku nebo v chybě.

Díky tomu obsluha nemusí hned otevírat diagnostiku Flexi Soft. Z tray menu je rychle vidět, který sledovaný vstup je problémový.

---

## Ruční command z tray menu

Tray menu obsahuje možnost poslat ruční command pro jednotlivé kanály.

Ruční command je dostupný pouze při platné komunikaci s Flexi Soft CPU. Pokud komunikace není v pořádku, položky commandů jsou zašedlé.

---

## Znovunačtení konfigurace, reconnect a ukončení

V tray menu je možné znovu načíst konfiguraci. Tím se znovu načte nejen `conf/config.json`, ale také jazykové soubory a runtime nastavení jazyka. To je užitečné při úpravě textů hlášení, překladů nebo komunikačních parametrů bez ručního restartu programu.

Pokud je při znovunačtení konfigurace otevřené hlášení, runtime ho zavře. Pokud chyba stále trvá, po znovunačtení se zobrazí nové aktuální hlášení podle nové konfigurace a nových textů.

Metadata okna About jsou součástí EXE a znovunačtení konfigurace je nemění.

V tray menu je možné ručně spustit také reconnect komunikace. Používá se při problému s převodníkem, síťovým spojením nebo CPU.

Ukončení aplikace se provádí z tray menu. Runtime při tom zapíše vlastní shutdown záznam do logu.

---

## Log

Výchozí log:

```text
flexi_runtime.log
```

Log je servisní záznam samotné aplikace. Je vždy v angličtině bez ohledu na jazyk UI.

Log obsahuje například:

- start aplikace,
- načtení konfigurace,
- stav komunikace,
- výpadek a obnovení komunikace,
- command ON/OFF,
- opakované hlášení,
- ruční reconnect,
- shutdown.

Log není náhrada za bezpečnostní diagnostiku Flexi Soft ani za strojní výrobní log.

---

## Dokumentace

Program umí otevřít jazykově lokalizovanou dokumentaci.

Základní názvy:

```text
docs/README.md
docs/MANUAL.md
```

Lokalizované názvy:

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

Používaný suffix musí odpovídat kódu jazyka v `languages.json`.

Dokumentace se běžně otevírá pomocí `FlexiSoftMdReader.exe` (v aktuálním Runtime balíčku v1.0.1). Runtime předává Readeru dokument a efektivní font UI. Reader v1.0.1 obsahuje kompatibilní obsluhu pro starý IE engine ve Windows XP. Pokud Reader nelze použít, runtime přejde na Notepad a následně na ShellExecute.

---

## Instalace

Tato část bude doplněná po dokončení instalačního balíčku.

Předpokládané oblasti:

- instalace programu,
- umístění konfigurace,
- aktualizace bez přepsání strojní konfigurace,
- autostart,
- servisní upgrade,
- záloha konfigurace.

---

## Základní kontrola po nasazení

Po nasazení ověř:

- RS-232 routing pro CPU je zapnutý ve Flexi Soft Designeru.
- RS-232 command bity jsou zapojené v logice Flexi Soft.
- Resetovací impuls je vytvořený v logice Flexi Soft jako krátký impuls.
- Runtime se připojí ke komunikaci.
- Barva tray ikony odpovídá skutečnému stavu komunikace a kanálů.
- Stav sledovaných kanálů v tray menu odpovídá reálnému stavu Flexi Soft.
- Hlášení odpovídají skutečným stavům stroje.
- Resetovací command odešle správný bit.
- Znovunačtení konfigurace načte i upravené překlady a jazykové soubory.
- Opakované hlášení se zobrazí při opakovaném návratu stejné chyby.

---

## Autor

```text
KVLab - Vladimír Kopal
vladakopal@gmail.com
```
