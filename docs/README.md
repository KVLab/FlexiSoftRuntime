# FlexiSoft Runtime

FlexiSoft Runtime is a service support application for SICK Flexi Soft. It was created to reduce repeated unnecessary maintenance calls on a CRIPPA tube bending machine, where maintenance is often called only to reset a state caused by an incorrectly closed or mechanically worn guard/enclosure.

The goal is not to replace maintenance when there is a real fault. The goal is to filter out a common operator mistake: the operator does not close the guard correctly, Flexi Soft keeps the fault state active, and maintenance has to come only to perform a reset. The runtime tells the operator clearly what to check and allows them to trigger a prepared reset pulse.

**FlexiSoft Runtime is not part of the machine safety function.** It is operator support software only. Safe evaluation, the safe state of the machine, and the enable/reset of the safety logic must always be handled by the Flexi Soft project and by the machine wiring.

---

## What it is used for

Typical use case:

1. The operator closes the machine guard/enclosure.
2. Due to wear, incorrect closing, or non-simultaneous switching of sensors, a fault remains active in Flexi Soft.
3. The runtime shows a message with a clear instruction for the operator.
4. The operator checks the guard and confirms the reset pulse.
5. The runtime writes the configured RS-232 command bit via RK512.
6. The Flexi Soft project creates a short reset pulse from this bit.
7. If the same fault keeps returning, the runtime shows a repeated message and the operator must call maintenance or perform a physical check.

The runtime mainly deals with repeated, trivial, and operationally annoying situations where the machine is not mechanically closed correctly, but a maintenance intervention is not required just to perform the reset.

---

## What the program does

- reads selected states from Flexi Soft via RK512,
- shows fault messages to the operator,
- allows the operator to confirm a reset pulse,
- monitors repeated return of the same fault,
- shows a stronger repeated message when the same fault returns repeatedly,
- allows a manual command from the tray menu,
- supports a multilingual UI and localized documentation,
- writes its own runtime service log.

The runtime **does not log machine events as a production or safety recorder**. It logs its own events: startup, configuration loading, communication state, shown messages, commands, reconnect, shutdown, and similar service information.

---

## Communication with Flexi Soft

The runtime communicates with Flexi Soft via RK512.

Supported connection types:

| Connection type | Use |
| --- | --- |
| Direct COM / RS-232 | The runtime runs on a PC with a physical or virtual serial port connected to Flexi Soft. |
| TCP via Ethernet/RS-232 converter | The runtime runs on the machine PC and connects to the Flexi Soft CPU through the existing machine network infrastructure. |

For integration into an existing machine, an Ethernet/RS-232 converter is usually practical. The runtime can then run directly on the machine OP/PC, typically even on an older Windows XP system. Communication with the Flexi Soft CPU can use the existing machine network instead of a separate long serial cable.

This keeps the machine modification minimal:

- The runtime runs on the machine PC/OP.
- Flexi Soft remains the main safety unit.
- The converter only carries RK512 communication between the runtime and the RS-232 interface of Flexi Soft.
- The existing machine network infrastructure can be used for the runtime ↔ converter link.

---

## Important files for integration and service

The regular operator does not work with these files. For integration and service, the most important files are:

| File / folder | Meaning |
| --- | --- |
| `conf/config.json` | Communication, monitored channels, message texts, output command bits, and repeated-message rules. |
| `conf/languages.json` | UI translations and optional fonts. |
| `conf/runtime_state.json` | Last runtime choices made by the user, currently mainly the selected language. |
| `FlexiSoftMdReader.exe` | External documentation viewer used by the Runtime. |
| `fonts/` | Optional fonts for languages that may not have a suitable system font. |
| `docs/` | Localized documentation. |
| `flexi_runtime.log` | Service log of the application. |

About metadata are embedded in `FlexiSoftRuntime.exe` at build time from `src/about.h`; they are not stored in a separate runtime configuration file. The embedded About data contain the product/version/build information and localized descriptions for EN/CZ/UK/FR/DE.

---

## Multilingual UI

The default language is English. Additional languages are added in `conf/languages.json`.

The runtime is not limited to a fixed set of languages. If translations and, optionally, documentation are added, another language can be added.

Documentation opens according to the active language when the matching file exists:

```text
docs/README_<language>.md
docs/MANUAL_<language>.md
```

If localized documentation is missing, the base documentation file without a language suffix is used.

---

## Flexi Soft Designer setup

### Enable RS-232 routing for the CPU

The Flexi Soft project must have the following option enabled:

```text
Enable RS-232 routing for CPU
```

![RS-232 routing setting for CPU](assets/flexisoft_designer_rs232_routing.png)

Without this setting, RS-232 signals may not be available to the CPU logic.

### Reset pulse from an RS-232 bit

The runtime only writes the RS-232 command bit. The actual reset pulse must be created in the Flexi Soft CPU logic.

Recommended principle:

```text
RS-232 bit -> edge detection -> timer / short pulse -> reset relay or reset output
```

Example:

```text
RS232 0.0 -> edge detection -> off-delay timer -> XTIO[1].Q1
RS232 0.1 -> edge detection -> off-delay timer -> XTIO[1].Q2
```

![Example reset relays from RS-232 bits](assets/flexisoft_designer_rs232_reset_relays.png)

This logic is recommended for safe behavior during a communication fault. If communication drops during a command, or if the runtime stops running, monitored inputs must not remain statically disconnected and a reset signal must not remain permanently active. Therefore, the short pulse must be handled by the Flexi Soft project, not by the runtime alone.

The configuration in `conf/config.json` must match how the RS-232 command bits are used in the Flexi Soft project.

---

## Messages

The runtime shows messages according to the configured channels.

A normal message tells the operator what to check, for example:

```text
Check that the enclosure is fully closed.
Open and close the guard correctly again.
After checking, confirm the reset.
```

The operator typically has two options:

| Option | Meaning |
| --- | --- |
| Yes / YES | Triggers the reset pulse after the check. |
| No / NO | Closes the message without sending a reset pulse. |

Message texts are set in the configuration.

---

## Repeated message

A repeated message appears when the same fault returns repeatedly after the reset pulse.

This is important mainly for a mechanically worn guard/enclosure:

- a one-time fault may only mean incorrect closing,
- a repeated fault means the guard, sensor, or wiring needs inspection,
- the operator must not keep confirming the reset indefinitely without checking the cause.

Example of a repeated-message text:

```text
The fault keeps returning.
Check that the guard is fully closed and check the sensor.
If this message repeats, call maintenance.
```

---

## Tray icon and channel status

The runtime runs in the Windows notification area. The tray icon color gives quick information about the overall application and communication state.

| Icon state | Meaning |
| --- | --- |
| Green | Communication with the Flexi Soft CPU is OK and no monitored channel is in fault. |
| Red | Communication is OK, but at least one monitored channel reports a fault. |
| Yellow / neutral | The application is running, but communication or the CPU state is not fully confirmed yet. Typically during startup, reconnect, or while waiting for a response. |
| Grey / communication error | The runtime does not have a valid connection to the Flexi Soft CPU, or communication is in fault. |

The tray menu also shows the state of monitored channels CH1 to CH4. For each enabled channel, its name and state are visible, for example whether the channel is active/inactive and whether it is OK or in fault.

This means the operator does not need to open Flexi Soft diagnostics immediately. The tray menu quickly shows which monitored input is problematic.

---

## Manual command from the tray menu

The tray menu can contain a manual command for individual channels.

The manual command is available only when communication with the Flexi Soft CPU is valid. If communication is not OK, command items are greyed out.

---

## Reload configuration, reconnect, and exit

The tray menu can reload the configuration. This reloads not only `conf/config.json`, but also the language files and the runtime language setting. This is useful when editing message texts, translations, or communication parameters without manually restarting the program.

If a message is open during configuration reload, the runtime closes it. If the fault still persists, a new current message is shown after reload, using the new configuration and new texts.

About metadata are part of the executable and are not changed by configuration reload.

The tray menu can also trigger a manual communication reconnect. This is used when there is a problem with the converter, network connection, or CPU.

The application is exited from the tray menu. During exit, the runtime writes its own shutdown record to the log.

---

## Log

Default log:

```text
flexi_runtime.log
```

The log is a service record of the application itself. It is always in English regardless of the UI language.

The log contains, for example:

- application startup,
- configuration loading,
- communication state,
- communication loss and recovery,
- command ON/OFF,
- repeated message,
- manual reconnect,
- shutdown.

The log is not a replacement for Flexi Soft safety diagnostics or for a machine production log.

---

## Documentation

The program can open language-localized documentation.

Base file names:

```text
docs/README.md
docs/MANUAL.md
```

Localized file names:

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

The suffix must match the language code in `languages.json`.

Documentation is normally opened by `FlexiSoftMdReader.exe` (v1.0.1 in the current Runtime package). The runtime passes the document and effective UI font to the Reader. Reader v1.0.1 includes compatibility handling for the legacy IE engine on Windows XP. If the Reader cannot be used, the runtime falls back to Notepad and then to ShellExecute.

---

## Installation

This section will be completed after the installer package is finalized.

Expected topics:

- program installation,
- configuration location,
- update without overwriting machine configuration,
- autostart,
- service upgrade,
- configuration backup.

---

## Basic check after deployment

After deployment, verify:

- RS-232 routing for the CPU is enabled in Flexi Soft Designer.
- RS-232 command bits are connected in Flexi Soft logic.
- The reset pulse is created in Flexi Soft logic as a short pulse.
- The runtime connects to communication.
- The tray icon color matches the real communication and channel state.
- The monitored channel state in the tray menu matches the real Flexi Soft state.
- Messages match the real machine states.
- The reset command writes the correct bit.
- Reloading the configuration also loads modified translations and language files.
- The repeated message appears when the same fault returns repeatedly.

---

## Author

```text
KVLab - Vladimír Kopal
vladakopal@gmail.com
```
