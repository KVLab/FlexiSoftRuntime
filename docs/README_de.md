# FlexiSoft Runtime

FlexiSoft Runtime ist eine unterstützende Service-Anwendung für SICK Flexi Soft. Sie wurde entwickelt, um wiederholte unnötige Einsätze der Instandhaltung an einer CRIPPA-Rohrbiegemaschine zu reduzieren, bei der häufig nur ein Zustand nach einer nicht korrekt geschlossenen oder mechanisch verschlissenen Schutzhaube / Einhausung zurückgesetzt werden muss.

Das Ziel ist nicht, die Instandhaltung bei echten Störungen zu ersetzen. Das Ziel ist, einen häufigen Bedienfehler abzufangen: Die Bedienung schließt die Schutzhaube nicht korrekt, Flexi Soft hält den Fehlerzustand, und die Instandhaltung muss nur zum Reset kommen. Die Runtime sagt der Bedienung verständlich, was zu prüfen ist, und ermöglicht das Auslösen eines vorbereiteten Reset-Impulses.

**FlexiSoft Runtime ist kein Bestandteil der Sicherheitsfunktion der Maschine.** Es handelt sich nur um eine Bedien- und Service-Software. Die sichere Auswertung, der sichere Zustand der Maschine und die Freigabe bzw. der Reset der Sicherheitslogik müssen immer durch das Flexi-Soft-Projekt und die Verdrahtung der Maschine sichergestellt werden.

---

## Wofür sie verwendet wird

Typischer Anwendungsfall:

1. Die Bedienung schließt die Schutzhaube / Einhausung der Maschine.
2. Wegen Verschleiß, nicht korrektem Schließen oder nicht gleichzeitigem Schalten der Sensoren bleibt in Flexi Soft ein Fehler aktiv.
3. Die Runtime zeigt eine Meldung mit einer klaren Anweisung für die Bedienung.
4. Die Bedienung prüft die Schutzhaube und bestätigt den Reset-Impuls.
5. Die Runtime schreibt das konfigurierte RS-232-Command-Bit über RK512.
6. Das Flexi-Soft-Projekt erzeugt aus diesem Bit einen kurzen Reset-Impuls.
7. Wenn derselbe Fehler wiederholt zurückkehrt, zeigt die Runtime eine wiederholte Meldung an, und die Bedienung muss die Instandhaltung rufen oder eine physische Prüfung durchführen.

Die Runtime behandelt also vor allem wiederkehrende, einfache und im Betrieb störende Situationen, in denen die Maschine mechanisch nicht korrekt geschlossen ist, aber kein Instandhaltungseinsatz nur zur Durchführung des Resets nötig ist.

---

## Was das Programm macht

- liest ausgewählte Zustände aus Flexi Soft über RK512,
- zeigt der Bedienung Fehlermeldungen an,
- ermöglicht das Bestätigen eines Reset-Impulses,
- überwacht die wiederholte Rückkehr desselben Fehlers,
- zeigt bei wiederholtem Fehler eine deutlichere wiederholte Meldung an,
- ermöglicht einen manuellen Command aus dem Tray-Menü,
- unterstützt eine mehrsprachige Benutzeroberfläche und lokalisierte Dokumentation,
- schreibt ein eigenes Service-Log der Anwendung.

Die Runtime **protokolliert keine Maschinenereignisse als Produktions- oder Sicherheitsaufzeichnung**. Sie protokolliert eigene Ereignisse: Start, Laden der Konfiguration, Kommunikationszustand, angezeigte Meldungen, Commands, Reconnect, Shutdown und ähnliche Service-Informationen.

---

## Kommunikation mit Flexi Soft

Die Runtime kommuniziert mit Flexi Soft über RK512.

Unterstützte Anschlussarten:

| Anschlussart | Verwendung |
| --- | --- |
| Direktes COM / RS-232 | Die Runtime läuft auf einem PC, der über einen physischen oder virtuellen seriellen Port mit Flexi Soft verbunden ist. |
| TCP über Ethernet/RS-232-Wandler | Die Runtime läuft auf dem Maschinen-PC und verbindet sich über die bestehende Maschinen-Netzwerkinfrastruktur mit der Flexi-Soft-CPU. |

Für die Integration in eine bestehende Maschine ist ein Ethernet/RS-232-Wandler meistens praktisch. Die Runtime kann dann direkt auf dem Maschinen-OP/PC laufen, typischerweise auch auf einem älteren System mit Windows XP. Die Kommunikation mit der Flexi-Soft-CPU kann über das vorhandene Maschinennetz erfolgen, statt eine separate lange serielle Leitung zu verlegen.

Der Eingriff in die Maschine bleibt dadurch minimal:

- Die Runtime läuft auf dem Maschinen-PC/OP.
- Flexi Soft bleibt die zentrale Sicherheitseinheit.
- Der Wandler überträgt nur die RK512-Kommunikation zwischen Runtime und RS-232-Schnittstelle von Flexi Soft.
- Die vorhandene Maschinen-Netzwerkinfrastruktur kann für die Verbindung Runtime ↔ Wandler genutzt werden.

---

## Wichtige Dateien für Integration und Service

Die normale Bedienung arbeitet nicht mit diesen Dateien. Für Integration und Service sind vor allem diese Dateien wichtig:

| Datei / Ordner | Bedeutung |
| --- | --- |
| `conf/config.json` | Kommunikation, überwachte Kanäle, Meldungstexte, Output-Command-Bits und Regeln für wiederholte Meldungen. |
| `conf/languages.json` | UI-Übersetzungen und optionale Schriftarten. |
| `conf/runtime_state.json` | Letzte Runtime-Auswahl des Benutzers, aktuell hauptsächlich die gewählte Sprache. |
| `FlexiSoftMdReader.exe` | Externer Dokumentations-Viewer, der von der Runtime verwendet wird. |
| `fonts/` | Optionale Schriftarten für Sprachen, für die möglicherweise keine passende Systemschrift vorhanden ist. |
| `docs/` | Lokalisierte Dokumentation. |
| `flexi_runtime.log` | Service-Log der Anwendung. |

Die About-Metadaten werden beim Build aus `src/about.h` direkt in `FlexiSoftRuntime.exe` eingebettet; sie liegen nicht in einer separaten Runtime-Konfigurationsdatei. Die eingebetteten About-Daten enthalten Produkt-/Versions-/Build-Informationen und lokalisierte Beschreibungen für EN/CZ/UK/FR/DE.

---

## Mehrsprachige Benutzeroberfläche

Die Standardsprache ist Englisch. Weitere Sprachen werden in `conf/languages.json` ergänzt.

Die Runtime ist nicht auf eine feste Anzahl von Sprachen beschränkt. Wenn Übersetzungen und optional Dokumentation ergänzt werden, kann eine weitere Sprache hinzugefügt werden.

Die Dokumentation wird entsprechend der aktiven Sprache geöffnet, wenn die passende Datei vorhanden ist:

```text
docs/README_<Sprache>.md
docs/MANUAL_<Sprache>.md
```

Wenn die lokalisierte Dokumentation fehlt, wird die Basisdokumentation ohne Sprachsuffix verwendet.

---

## Einstellung in Flexi Soft Designer

### RS-232-Routing für die CPU aktivieren

Im Flexi-Soft-Projekt muss folgende Option aktiviert sein:

```text
RS-232-Routing für CPU aktivieren
```

![RS-232-Routing-Einstellung für die CPU](assets/flexisoft_designer_rs232_routing.png)

Ohne diese Einstellung stehen die RS-232-Signale der CPU-Logik möglicherweise nicht zur Verfügung.

### Reset-Impuls aus einem RS-232-Bit

Die Runtime schreibt nur das RS-232-Command-Bit. Der eigentliche Reset-Impuls muss in der Flexi-Soft-CPU-Logik erzeugt werden.

Empfohlenes Prinzip:

```text
RS-232-Bit -> Flankenerkennung -> Timer / kurzer Impuls -> Reset-Relais oder Reset-Ausgang
```

Beispiel:

```text
RS232 0.0 -> Flankenerkennung -> Ausschaltverzögerung -> XTIO[1].Q1
RS232 0.1 -> Flankenerkennung -> Ausschaltverzögerung -> XTIO[1].Q2
```

![Beispiel für Reset-Relais aus RS-232-Bits](assets/flexisoft_designer_rs232_reset_relays.png)

Diese Logik wird wegen des sicheren Verhaltens bei Kommunikationsstörungen empfohlen. Wenn während eines Commands die Kommunikation ausfällt oder die Runtime nicht mehr läuft, dürfen überwachte Eingänge nicht statisch getrennt bleiben und ein Reset-Signal darf nicht dauerhaft aktiv bleiben. Der kurze Impuls muss deshalb vom Flexi-Soft-Projekt erzeugt werden, nicht nur von der Runtime.

Die Konfiguration in `conf/config.json` muss dazu passen, wie die RS-232-Command-Bits im Flexi-Soft-Projekt verwendet werden.

---

## Meldungen

Die Runtime zeigt Meldungen entsprechend den konfigurierten Kanälen an.

Eine normale Meldung sagt der Bedienung, was sie prüfen soll, zum Beispiel:

```text
Prüfe, ob die Schutzhaube vollständig geschlossen ist.
Öffne und schließe die Schutzhaube erneut korrekt.
Bestätige nach der Prüfung den Reset.
```

Die Bedienung hat typischerweise zwei Möglichkeiten:

| Auswahl | Bedeutung |
| --- | --- |
| Ja / YES | Löst nach der Prüfung den Reset-Impuls aus. |
| Nein / NO | Schließt die Meldung ohne Reset-Impuls. |

Die Meldungstexte werden in der Konfiguration eingestellt.

---

## Wiederholte Meldung

Eine wiederholte Meldung erscheint, wenn derselbe Fehler nach dem Reset-Impuls wiederholt zurückkehrt.

Das ist besonders wichtig bei einer mechanisch verschlissenen Schutzhaube / Einhausung:

- ein einmaliger Fehler kann nur ein nicht korrektes Schließen bedeuten,
- ein wiederholter Fehler bedeutet, dass Schutzhaube, Sensor oder Verdrahtung geprüft werden müssen,
- die Bedienung darf den Reset nicht endlos bestätigen, ohne die Ursache zu prüfen.

Beispiel für den Sinn einer wiederholten Meldung:

```text
Der Fehler kehrt wiederholt zurück.
Prüfe, ob die Schutzhaube vollständig geschlossen ist, und prüfe den Sensor.
Wenn diese Meldung erneut erscheint, Instandhaltung rufen.
```

---

## Tray-Symbol und Kanalzustand

Die Runtime läuft im Windows-Infobereich. Die Farbe des Tray-Symbols gibt eine schnelle Information über den Gesamtzustand der Anwendung und der Kommunikation.

| Symbolzustand | Bedeutung |
| --- | --- |
| Grün | Kommunikation mit der Flexi-Soft-CPU ist in Ordnung und kein überwachter Kanal ist im Fehler. |
| Rot | Kommunikation ist in Ordnung, aber mindestens ein überwachter Kanal meldet einen Fehler. |
| Gelb / neutral | Die Anwendung läuft, aber Kommunikation oder CPU-Zustand sind noch nicht vollständig bestätigt. Typisch beim Start, Reconnect oder Warten auf eine Antwort. |
| Grau / Kommunikationsfehler | Die Runtime hat keine gültige Verbindung zur Flexi-Soft-CPU oder die Kommunikation ist im Fehler. |

Das Tray-Menü zeigt auch den Zustand der überwachten Kanäle CH1 bis CH4. Für jeden aktivierten Kanal sind Name und Zustand sichtbar, zum Beispiel ob der Kanal aktiv/inaktiv ist und ob er OK oder im Fehler ist.

Dadurch muss die Bedienung nicht sofort die Flexi-Soft-Diagnose öffnen. Im Tray-Menü ist schnell sichtbar, welcher überwachte Eingang problematisch ist.

---

## Manueller Command aus dem Tray-Menü

Das Tray-Menü kann einen manuellen Command für einzelne Kanäle enthalten.

Der manuelle Command ist nur verfügbar, wenn die Kommunikation mit der Flexi-Soft-CPU gültig ist. Wenn die Kommunikation nicht in Ordnung ist, sind die Command-Einträge ausgegraut.

---

## Konfiguration neu laden, Reconnect und Beenden

Über das Tray-Menü kann die Konfiguration neu geladen werden. Dabei wird nicht nur `conf/config.json` neu geladen, sondern auch die Sprachdateien und die Runtime-Spracheinstellung. Das ist nützlich, wenn Meldungstexte, Übersetzungen oder Kommunikationsparameter geändert werden, ohne das Programm manuell neu zu starten.

Wenn während des Neuladens der Konfiguration eine Meldung geöffnet ist, schließt die Runtime sie. Wenn der Fehler weiterhin besteht, wird nach dem Neuladen eine neue aktuelle Meldung nach der neuen Konfiguration und den neuen Texten angezeigt.

Die About-Metadaten sind Bestandteil der EXE und werden durch das Neuladen der Konfiguration nicht geändert.

Über das Tray-Menü kann auch ein manueller Reconnect der Kommunikation ausgelöst werden. Das wird bei Problemen mit dem Wandler, der Netzwerkverbindung oder der CPU verwendet.

Die Anwendung wird über das Tray-Menü beendet. Beim Beenden schreibt die Runtime einen eigenen Shutdown-Eintrag in das Log.

---

## Log

Standard-Log:

```text
flexi_runtime.log
```

Das Log ist eine Service-Aufzeichnung der Anwendung selbst. Es ist immer auf Englisch, unabhängig von der Sprache der Benutzeroberfläche.

Das Log enthält zum Beispiel:

- Start der Anwendung,
- Laden der Konfiguration,
- Kommunikationszustand,
- Ausfall und Wiederherstellung der Kommunikation,
- Command ON/OFF,
- wiederholte Meldung,
- manueller Reconnect,
- Shutdown.

Das Log ist kein Ersatz für die Sicherheitsdiagnose von Flexi Soft und kein Produktionslog der Maschine.

---

## Dokumentation

Das Programm kann sprachlich lokalisierte Dokumentation öffnen.

Basisdateien:

```text
docs/README.md
docs/MANUAL.md
```

Lokalisierte Dateinamen:

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

Das verwendete Suffix muss dem Sprachcode in `languages.json` entsprechen.

Die Dokumentation wird normalerweise mit `FlexiSoftMdReader.exe` geöffnet (v1.0.1 im aktuellen Runtime-Paket). Die Runtime übergibt dem Reader das Dokument und die effektive UI-Schrift. Reader v1.0.1 enthält Kompatibilitätsbehandlung für die alte IE-Engine unter Windows XP. Wenn der Reader nicht verwendet werden kann, fällt die Runtime auf Notepad und anschließend auf ShellExecute zurück.

---

## Installation

Dieser Abschnitt wird nach Fertigstellung des Installationspakets ergänzt.

Vorgesehene Themen:

- Programminstallation,
- Ablageort der Konfiguration,
- Update ohne Überschreiben der Maschinenkonfiguration,
- Autostart,
- Service-Upgrade,
- Sicherung der Konfiguration.

---

## Grundprüfung nach der Inbetriebnahme

Nach der Inbetriebnahme prüfen:

- RS-232-Routing für die CPU ist in Flexi Soft Designer aktiviert.
- RS-232-Command-Bits sind in der Flexi-Soft-Logik eingebunden.
- Der Reset-Impuls wird in der Flexi-Soft-Logik als kurzer Impuls erzeugt.
- Die Runtime stellt die Kommunikation her.
- Die Farbe des Tray-Symbols entspricht dem tatsächlichen Kommunikations- und Kanalzustand.
- Der Zustand der überwachten Kanäle im Tray-Menü entspricht dem realen Flexi-Soft-Zustand.
- Die Meldungen passen zu den tatsächlichen Maschinenzuständen.
- Der Reset-Command schreibt das richtige Bit.
- Das Neuladen der Konfiguration lädt auch geänderte Übersetzungen und Sprachdateien.
- Die wiederholte Meldung erscheint, wenn derselbe Fehler wiederholt zurückkehrt.

---

## Autor

```text
KVLab - Vladimír Kopal
vladakopal@gmail.com
```
