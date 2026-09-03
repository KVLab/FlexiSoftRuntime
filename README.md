# FlexiSoft Runtime

FlexiSoft Runtime is a Win32 operator/service support application for SICK Flexi Soft systems. It reads selected Flexi Soft states through RK512, presents operator messages, can send configured command bits, and tracks repeated returns of the same fault.

**FlexiSoft Runtime is not part of the machine safety function.** Safe evaluation, the safe machine state, and the enable/reset logic must remain in the Flexi Soft project and machine wiring.

## Documentation

- [README / overview](docs/README.md)
- [Integration and service manual](docs/MANUAL.md)
- Localized documentation is available for CZ, UK, FR and DE in `docs/`.

Runtime documentation is normally opened by the external `FlexiSoftMdReader.exe`; the current package uses Reader v1.0.1 and includes compatibility handling for Windows XP.

## Runtime configuration

The main runtime files are:

```text
FlexiSoftRuntime.exe
FlexiSoftMdReader.exe
conf/
  config.json
  languages.json
  runtime_state.json
docs/
fonts/
```

About metadata are embedded into `FlexiSoftRuntime.exe` from `src/about.h` at build time. `src/about.h` is the source of the Runtime product/version/build metadata; release tagging should be derived from that version, not the other way around.

## Platform

The current Win32 Release configuration targets the legacy-compatible `v141_xp` toolset, static `/MT` runtime and Windows subsystem 5.01. The Runtime and its documentation Reader have been smoke-tested on Windows XP SP3 and Windows 11.

## Source and release status

Public release packages are generated from the verified Release build. The Runtime version is read from `src/about.h`, and the Git release tag is derived from that version.

## Author

KVLab — Vladimír Kopal

## License

FlexiSoft Runtime source code and documentation are licensed under the [MIT License](LICENSE).

The bundled `NotoSans-Regular.ttf` font is distributed under the SIL Open Font License 1.1; see `fonts/LICENSE-NotoSans-OFL.txt`.

The software is provided without warranty and is not part of the machine safety function.

## Trademark notice

SICK and Flexi Soft are trademarks of their respective owners. This project is independent and is not affiliated with or endorsed by SICK AG.