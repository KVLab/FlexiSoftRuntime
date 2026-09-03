# FlexiSoft Runtime — integration and service manual

This manual follows the README and describes the practical setup of FlexiSoft Runtime. It is intended mainly for integration into an existing machine, configuration editing, preparation of message texts, and basic diagnostics.

FlexiSoft Runtime is an operator/service support application. It is not part of the machine safety function. Safe evaluation and reset of the safety logic must always be handled by the Flexi Soft project and by the machine wiring.

---

## 1. Principle

The runtime periodically reads status bits from Flexi Soft via RK512. From these bits, it evaluates the monitored channels and shows messages to the operator according to their state.

If the operator confirms a reset after checking the machine, the runtime writes the configured RS-232 command bit. The Flexi Soft project creates a short reset pulse from this bit.

Basic flow:

```text
FlexiSoftRuntime.exe
    -> RK512
    -> RS-232 or TCP/RS-232 converter
    -> Flexi Soft CPU
    -> project logic
    -> reset relay / output
```

---

## 2. Important files

| File / folder | Meaning |
| --- | --- |
| `conf/config.json` | Main runtime configuration: communication, channels, outputs, message texts, logging. |
| `conf/languages.json` | UI translations, language names, and optional fonts. |
| `conf/runtime_state.json` | Last runtime choices made by the user, for example the last selected language. |
| `docs/` | Localized documentation. |
| `docs/assets/` | Images used in the documentation. |
| `fonts/` | Optional fonts. |
| `flexi_runtime.log` | Service log of the application itself. |

About metadata are not stored in a separate runtime file. Product name, version, localized descriptions (EN/CZ/UK/FR/DE), build date, build type, toolset, and target system are compiled into `FlexiSoftRuntime.exe` from `src/about.h` at build time. Changing these values requires a new build; reloading configuration does not change the About data.

A practical manual deployment therefore does not need a separate version metadata file. The basic structure is:

```text
FlexiSoftRuntime.exe
FlexiSoftMdReader.exe          # recommended documentation viewer
conf/
  config.json
  languages.json
  runtime_state.json          # optional; created/updated by the runtime
docs/
fonts/                        # optional; required only for configured file fonts
```

The regular operator does not edit these files.

---

## 3. `conf/config.json` configuration

`conf/config.json` is the main machine configuration file. The recommended format is UTF-8.

JSON strings can use direct UTF-8 or `\uXXXX` escapes. Valid UTF-16 surrogate pairs are supported. Direct UTF-8 remains the preferred format for configuration and language files.

Basic structure:

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

Default application language. It is used on first startup or when no runtime language has been stored.

Recommendations:

- the base fallback is `en`,
- Czech uses `cz`,
- Ukrainian uses `uk`,
- additional languages can be added in `conf/languages.json`.

Changing the language at runtime does not overwrite `conf/config.json`.

---

### 3.2 `transport`

```json
"transport": "network"
```

Selects the communication method.

| Value | Meaning |
| --- | --- |
| `serial` | Direct COM/RS-232 port. |
| `network` | Network transport via TCP or UDP. |

For integration into a machine OP/PC, `network` with an Ethernet/RS-232 converter is typically the suitable mode.

---

## 4. Serial communication `serial`

The `serial` section is used when:

```json
"transport": "serial"
```

Example:

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

| Parameter | Meaning |
| --- | --- |
| `port` | COM port name, for example `COM4` or `COM5`. |
| `baud` | Serial line speed. |
| `data_bits` | Number of data bits. Usually `8`. |
| `parity` | Parity. Usually `N`. |
| `stop_bits` | Number of stop bits. Usually `1`. |
| `timeout_ms` | Timeout for serial read and write operations. |

For Flexi Soft in this project, the usual setting is:

```text
115200 8N1
```

---

## 5. Network communication `network`

The `network` section is used when:

```json
"transport": "network"
```

Example:

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

| Parameter | Meaning |
| --- | --- |
| `mode` | Network transport mode. |
| `host` | IP address or hostname of the converter. |
| `port` | TCP/UDP port of the converter. |
| `bind_host` | Local address for UDP bind. Usually does not need to be changed for TCP client mode. |
| `bind_port` | Local port for UDP bind. `0` means automatic. |
| `connect_timeout_ms` | Timeout for establishing a TCP connection. |
| `timeout_ms` | Timeout for reading a response. |

Supported `mode` values:

| Value | Meaning |
| --- | --- |
| `tcp_client` | The runtime connects as a TCP client to an Ethernet/RS-232 converter. Recommended mode. |
| `udp` | UDP mode. Use only if it matches the specific converter and has been tested. |

Practical machine deployment:

```text
machine OP/PC with runtime
    -> existing machine Ethernet
    -> Ethernet/RS-232 converter
    -> RS-232 Flexi Soft CPU
```

This avoids running a separate long RS-232 cable between the machine OP and the control cabinet.

---

## 6. RK512 `rk512`

Example:

```json
"rk512": {
  "device_local": "0x4F",
  "device_reply": "0x4D",
  "token_hex": "0F 0F 46 4C 58 54 30 31"
}
```

| Parameter | Meaning |
| --- | --- |
| `device_local` | Local RK512 address of the runtime application. |
| `device_reply` | Expected RK512 reply address from Flexi Soft. |
| `token_hex` | Token used before writing a command. |

Use `token_hex` in the configuration.

The values must match the verified RK512 communication of the specific project.

---

## 7. Polling `poll`

Example:

```json
"poll": {
  "period_ms": 1000,
  "read_block": "0x76",
  "read_size": 54
}
```

| Parameter | Meaning |
| --- | --- |
| `period_ms` | Status block read period. |
| `read_block` | RK512 block from which statuses are read. |
| `read_size` | Number of bytes to read. |

From the read block, the runtime evaluates channels according to `status_byte`, `on_bit`, and `ok_bit`.

---

## 8. Channels `inputs`

The runtime supports up to four channels. Each channel is one item in the `inputs` array.

Example:

```json
{
  "enabled": true,
  "name": "Left side enclosure",
  "status_byte": 0,
  "on_bit": 0,
  "ok_bit": 1,
  "alert_text_en": "Check that the left side of the enclosure is fully closed.",
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
    "text_en": "The fault keeps returning. Check the left guard and sensor."
  }
}
```

### 8.1 Basic channel parameters

| Parameter | Meaning |
| --- | --- |
| `enabled` | Enables/disables the channel. A disabled channel is not monitored and does not show messages. |
| `name` | Channel name shown in the tray menu and messages. |
| `status_byte` | Byte index in the read status block. |
| `on_bit` | Informational ON/OFF bit of the channel. |
| `ok_bit` | Bit used by the runtime to decide whether the channel is OK. |
| `alert_text_<language>` | Normal message text for the given language. |

The older key `alert_text` can be used as a fallback, but language-specific keys are recommended:

```text
alert_text_en
alert_text_cz
alert_text_uk
alert_text_fr
alert_text_de
```

If the active language has no text, the runtime uses English or a fallback text.

### 8.2 Output command `output`

```json
"output": {
  "block": "0x42",
  "byte": 0,
  "bit": 0,
  "pulse_ms": 1000
}
```

| Parameter | Meaning |
| --- | --- |
| `block` | RK512 block for writing the command. |
| `byte` | Byte in the written data. |
| `bit` | Bit in that byte. |
| `pulse_ms` | Time for which the runtime holds the command bit. |

During a command, the runtime sets the bit, waits for `pulse_ms`, and then clears the bit.

Important: even though the runtime holds the bit only for a limited time, a short reset pulse must also be created in the Flexi Soft logic. This prevents a permanent effect if communication or the application fails.

---

## 9. Repeated message `repeat_fault`

Repeat fault handles a situation where the same fault repeatedly returns after a reset pulse.

Example:

```json
"repeat_fault": {
  "count": 3,
  "window_ms": 30000,
  "ignore_after_command_ms": 1500,
  "text_en": "Check that the guard is fully closed."
}
```

| Parameter | Meaning |
| --- | --- |
| `count` | How many times the fault may return within the active window before a repeated message is created. Minimum is `1`. |
| `window_ms` | Time window for counting repeated fault returns. Minimum is `1000 ms`. |
| `ignore_after_command_ms` | Protective time after a reset command. During this time, the runtime ignores the transient input state caused by the hardware reset through relays. Negative values are treated as `0`. |
| `text_<language>` | Repeated-message text for the given language. |

Recommended language keys:

```text
text_en
text_cz
text_uk
text_fr
text_de
```

### 9.1 How repetition is counted

After reset confirmation, the runtime sends the reset command and starts the protective time `ignore_after_command_ms`.

This time is not primarily meant to “hide the return of the same fault”. Its real purpose is to filter out the transient input state during the hardware reset.

During reset, the monitored input channels are briefly opened by relays. In this state, both input channels can be OFF and Flexi Soft starts timing its channel simultaneity window again. From the runtime point of view, the input can briefly appear OK, even though it is only a transient state caused by the reset.

If the runtime treated this short state as a real OK state, it would incorrectly reset the repeated-message logic. Therefore, the transient state is not allowed to affect repeat logic during `ignore_after_command_ms`.

During `ignore_after_command_ms`, the runtime:

- does not treat a short OK state as a real repair of the input,
- does not reset the repetition counter,
- does not count the transient state as a new fault return,
- does not open a new message based on the transient state.

After `ignore_after_command_ms` expires, the runtime evaluates the real channel state again:

- if the channel is still in fault, the repetition counter is incremented,
- if the channel is truly OK after the protective time, the repetition counter is reset,
- if the counter reaches `count`, the repeated message is shown.

The operator waiting time before confirmation is counted into `window_ms`. The command duration, pulse duration, and protective time `ignore_after_command_ms` are not counted into the active window.

### 9.2 Active faults and repeat limit

Repeat counters are maintained independently for each monitored channel. Internally, the set of currently active faults (`errorMask`) and the set of channels that have reached the repeat limit (`repeatMask`) are separate. A repeat alert can therefore list all currently active faults, while the corrective repeat text is shown only for channels present in `repeatMask`.

---

## 10. UI `ui`

Example:

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

| Parameter | Meaning |
| --- | --- |
| `tray_tooltip` | Fallback tray icon tooltip text. |
| `tray_tooltip_<language>` | Localized tooltip for a specific language. |

`tray_tooltip` is kept as a legacy fallback. For current configurations, prefer the language-specific `tray_tooltip_<language>` keys.

Recommended keys are language-specific keys:

```text
tray_tooltip_en
tray_tooltip_cz
tray_tooltip_uk
tray_tooltip_fr
tray_tooltip_de
```

---

## 11. Logging `logging`

Example:

```json
"logging": {
  "enabled": true,
  "file": "flexi_runtime.log",
  "newest_first": true,
  "max_bytes": 65536
}
```

| Parameter | Meaning |
| --- | --- |
| `enabled` | Enables/disables logging. |
| `file` | Log file path. |
| `newest_first` | If `true`, newest lines are written at the top. |
| `max_bytes` | Approximate maximum log size. Minimum is `4096`. |

Recommended production mode:

```json
"newest_first": true
```

`newest_first=false` remains available as a legacy mode, but it is not the recommended production setting.

The log is always in English. The UI language has no effect on it.

---

## 12. Debug `debug`

Example:

```json
"debug": {
  "force_command_fail": false
}
```

| Parameter | Meaning |
| --- | --- |
| `force_command_fail` | Test option for forcing a command failure. In normal operation it must be `false`. |

This section is only for testing and diagnostics.

---

## 13. Runtime state `conf/runtime_state.json`

`conf/runtime_state.json` stores the last runtime choice made by the user, currently mainly the language.

Example:

```json
{
  "language": "cz"
}
```

If the file is missing, the language from `conf/config.json` is used. If the file is damaged, the runtime writes a warning to the log and uses the language from `conf/config.json`.

Changing the language at runtime does not overwrite `conf/config.json`.

If saving `runtime_state.json` fails, the runtime only writes a warning to the log. The already selected UI language remains active and is not reverted.

---

## 14. Translations `conf/languages.json`

`conf/languages.json` contains UI translations and optional font settings.

Basic structure:

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

The `languages` object contains individual languages. The language key is also the language code, for example:

```text
en
cz
uk
fr
de
```

### 14.2 Language parameters

| Parameter | Meaning |
| --- | --- |
| `name` | Language name shown in the tray menu. |
| `font_face` | System font name or font face. |
| `font_file` | Optional font file, relative to the application folder. |
| `strings` | UI translation keys. |

If `font_file` exists and can be loaded, the runtime uses this font. Otherwise it uses a fallback font. This matters especially for languages that may not have a suitable system font on older Windows systems.

### 14.3 Translation fallback rules

- English (`en`) is the base fallback.
- If the active language does not contain a text, the English text is used.
- If the text is missing even in English, the key name is displayed.
- A missing or empty `conf/languages.json` can be replaced by the runtime with a basic English template.

### 14.4 Important translation keys

#### Tray menu

| Key | Meaning |
| --- | --- |
| `tray.title` | Tray menu title. |
| `tray.reload_config` | Item for reloading configuration. |
| `tray.reconnect` | Item for communication reconnect. |
| `tray.about` | About item. |
| `tray.exit` | Program exit. |
| `tray.language` | Language submenu. |
| `tray.send_command` | Manual command text; supports the `{channel}` token. |

#### Status texts

| Key | Meaning |
| --- | --- |
| `status.ok` | OK state. |
| `status.error` | Error/fault state. |
| `status.on` | ON state. |
| `status.off` | OFF state. |
| `status.disabled` | Disabled channel. |

#### Messages

| Key | Meaning |
| --- | --- |
| `alert.input.window_title` | Window title of a normal message. |
| `alert.repeat.window_title` | Window title of a repeated message. |
| `alert.command_failed.window_title` | Window title of a command-failed message. |
| `alert.input.title` | Heading of a normal message. |
| `alert.repeat.title` | Heading of a repeated message. |
| `alert.command_failed.title` | Heading of a command-failed message. |
| `alert.repeat.prefix` | Introductory text for a repeated message. |
| `alert.active_errors` | Heading for the active fault list. |
| `alert.command_failed.text` | Command-failed text. |
| `alert.affected_channels` | Heading for the affected channel list. |
| `alert.input.footer_yes` | Explanation of the YES button in a normal message. |
| `alert.input.footer_no` | Explanation of the NO button in a normal message. |
| `alert.command_failed.footer_yes` | Explanation of YES in a command-failed message. |
| `alert.command_failed.footer_no` | Explanation of NO in a command-failed message. |

#### Buttons

| Key | Meaning |
| --- | --- |
| `button.yes` | Yes / YES button text. |
| `button.no` | No / NO button text. |
| `button.ok` | OK button text. |

#### About and documentation

| Key | Meaning |
| --- | --- |
| `about.title` | About dialog title. |
| `about.version` | Version label. |
| `about.build_date` | Build date label. |
| `about.open_readme` | Button for opening README. |
| `about.open_manual` | Button for opening MANUAL. |
| `about.open_log` | Button for opening the log. |
| `about.project_page` | Project page button. |
| `error.missing_readme` | Error for missing README. |
| `error.missing_manual` | Error for missing MANUAL. |
| `error.missing_log` | Error for missing log. |

---

## 15. Adding another language

Procedure:

1. Add a new language object to `languages.json`, for example `pl`.
2. Fill in `name`, `font_face`, and optionally `font_file`.
3. Add the texts in `strings`.
4. In `config.json`, set `"language": "pl"`, or select the language at runtime from the tray menu.
5. For documentation, add the matching files, for example:

```text
docs/README_pl.md
docs/MANUAL_pl.md
```

---

## 16. Documentation

The runtime opens documentation according to the active language.

Base documentation:

```text
docs/README.md
docs/MANUAL.md
```

Localized documentation:

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

If the localized file is missing, the base file without a language suffix is used.

Documentation is normally opened by the external `FlexiSoftMdReader.exe` (v1.0.1 in the current Runtime package). The runtime passes the selected document and the effective UI font to the Reader. Reader v1.0.1 contains compatibility handling for the legacy IE engine used on Windows XP. If the Reader cannot be used, the runtime falls back to Notepad and then to ShellExecute. The Reader supports the Markdown subset used by the supplied documentation, including images from `docs/assets/`.

---

## 17. Flexi Soft Designer setup

### 17.1 RS-232 routing

The project must have the following option enabled:

```text
Enable RS-232 routing for CPU
```

![RS-232 routing setting for CPU](assets/flexisoft_designer_rs232_routing.png)

### 17.2 Short pulse in CPU logic

The runtime writes the command bit, but the actual short pulse must also be created in the Flexi Soft project.

Recommended principle:

```text
RS-232 bit -> edge detection -> timer / short pulse -> reset relay or reset output
```

![Example reset relays from RS-232 bits](assets/flexisoft_designer_rs232_reset_relays.png)

Reason: if communication or the runtime fails, monitored inputs must not remain permanently disconnected and the reset signal must not remain permanently active.

---

## 18. Reloading configuration

Configuration can be reloaded from the tray menu.

The following files are reloaded:

- `conf/config.json`,
- `conf/languages.json`,
- `conf/runtime_state.json`.

About metadata are embedded in the executable and are not affected by configuration reload.

If a message is open, the runtime closes it. If the fault still persists, it shows a new message according to the current configuration and current texts.

Reconnect uses the same modeless-alert flow: the old alert is closed before reconnect, and if the fault is still present after communication returns, a new current alert is shown.

---

## 19. Tray icon and tray menu

The tray icon shows the overall communication and channel state:

| Icon state | Meaning |
| --- | --- |
| Green | Communication is OK and no monitored channel is in fault. |
| Red | Communication is OK, but at least one monitored channel is in fault. |
| Yellow / neutral | Startup, reconnect, or waiting for state confirmation. |
| Grey / communication error | The runtime does not have a valid connection to the Flexi Soft CPU. |

The tray menu shows the state of enabled channels CH1 to CH4. For each channel, its name and ON/OFF and OK/FAULT state are visible.

Manual commands are available only when communication with the CPU is valid.

---

## 20. Logging and diagnostics

The log is a service log of the application, not a production or safety log of the machine.

It logs for example:

- application startup,
- configuration loading,
- transport opening,
- communication error,
- communication recovery,
- message display,
- command ON/OFF,
- command failure,
- repeated message,
- reconnect,
- shutdown.

When the same communication error persists, the runtime limits log flooding. The same error is not logged every cycle unless the state or error text changes.

---

## 21. Troubleshooting

### Runtime does not connect

Check:

- transport type,
- IP address and converter port,
- COM port,
- serial parameters,
- wiring,
- converter power supply,
- RS-232 routing in the Flexi Soft project,
- RK512 addresses.

### Converter is reachable, but Flexi Soft does not respond

Check:

- whether the Flexi Soft CPU is in RUN,
- whether the RS-232 interface is connected correctly,
- whether RK512 settings match,
- whether the read block and size match,
- whether the project contains the required signals.

### Manual command is greyed out

The runtime does not have valid communication with the Flexi Soft CPU. Restore communication first or check the converter.

### Message returns after reset

After the protective time has expired, the monitored input is still in fault. Check the guard/enclosure, sensors, and wiring.

### Repeated message

The fault returns repeatedly after reset. The operator must not keep confirming it without a physical check. The guard, sensors, wiring, or safety logic must be checked.

### Command failed

The command bit could not be written or cleared. Check communication, the RK512 write block, token, and RS-232 routing.

---

## 22. Deployment checklist

Before handover, verify:

- RS-232 routing for the CPU is enabled.
- RS-232 command bits are connected in Flexi Soft logic.
- The reset pulse is created in Flexi Soft as a short pulse.
- Communication matches the settings in `config.json`.
- The read block and size match the project.
- Status bytes and bits match the project.
- Output command bits match the project.
- Message texts are clear for the operator.
- The repeated message clearly states when to call maintenance.
- `ignore_after_command_ms` covers only the transient hardware reset of inputs and does not reset repeat logic based on a false OK state.
- The tray icon and tray menu match the real state.
- Reloading configuration also loads modified languages.
- The log is readable.

---

## 23. Author

```text
KVLab - Vladimír Kopal
vladakopal@gmail.com
```
