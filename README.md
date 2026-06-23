# stagePLTR

**An open-source tool for designing attractive stage plots and tech riders for bands.**

stagePLTR is a cross-platform desktop application (macOS, Windows, Linux) for laying
out stage plots and producing clean, professional tech riders. It's built for working
musicians and engineers, and aims to feel native on whichever OS you run it on.

> **Status: early development.** The core editor is being built and the symbol library
> starts small — focused on immediate needs — and will broaden over time. Expect things
> to change.

---

## Features

**Now / in progress**
- Cross-platform native desktop app (Qt 6 / C++)
- Stage-plot canvas with a vector (SVG) symbol library — instruments, mics, monitors,
  DI boxes, risers, and more
- Native menus, keyboard shortcuts, and dialogs per platform

**Planned**
- Tech-rider document generation (with PDF export)
- A broader, community-driven symbol library
- Longer term, band-business tooling (e.g. sales, merch, and show-payment tracking)

---

## Building from source

### Prerequisites

| Tool | Version | Notes |
|------|---------|-------|
| Qt | 6 (Widgets, Svg) | Qt 6 required |
| CMake | 3.25+ | Required for the bundled presets |
| Ninja | any recent | Preset generator (`Ninja Multi-Config`) |
| A C++20 compiler | — | AppleClang / MSVC / GCC / Clang |

On macOS, Qt 6 via Homebrew (`brew install qt ninja cmake`) is the tested setup.

### Configure & build

The project ships [`CMakePresets.json`](CMakePresets.json) with per-OS presets that wire
in the Qt prefix automatically. Pick the preset for your platform:

```bash
# macOS
cmake --preset macos
cmake --build --preset macos-debug      # or macos-release

# Linux
cmake --preset linux
cmake --build --preset linux-debug

# Windows
cmake --preset windows
cmake --build --preset windows-debug
```

If Qt lives somewhere non-standard, override the prefix without editing anything:

```bash
QT_PREFIX=/path/to/qt cmake --preset macos
```

Personal, machine-specific tweaks go in `CMakeUserPresets.json` (git-ignored).

---

## Project layout

```
stagePLTR/
├── main.cpp, mainwindow.*     Application source
├── resources.qrc             Qt resource bundle (plot symbols, UI glyphs)
├── assets/                   Non-code assets (see assets/README.md)
│   ├── plot/                 Stage-plot symbol library (SVG)
│   ├── ui/                   App-chrome glyphs (SVG)
│   └── app-icon/             App icon: iconsmaker source + generated bundles
├── docs/                     User Guide, Developer Guide, and doc images
├── CMakeLists.txt
└── CMakePresets.json
```

### Application icons

Platform icons are generated from a single source SVG by
[`iconsmaker`](https://github.com/ideocentric/iconsmaker). The generated bundles are
committed under `assets/app-icon/generated/`, so a normal build needs no extra tooling.
To regenerate after changing the artwork, see
[`assets/app-icon/README.md`](assets/app-icon/README.md).

---

## Documentation

User and Developer guides live under [`docs/`](docs/). The Developer Guide lists the full
set of tools required to build and package the project. Versioned releases include PDF
copies of the guides with screenshots.

---

## Contributing

stagePLTR is open source and contributions are welcome. The project is in early days, so
the best first step is to open an issue to discuss an idea before sending a change. Please
keep the cross-platform, native-feel goals in mind.

---

## License

stagePLTR is **dual-licensed**:

- **Code** — **GNU General Public License v3.0 or later** (`GPL-3.0-or-later`);
  see [`LICENSE`](LICENSE).
- **Visual art** (the SVG device/object/figure icons) — **Creative Commons
  Attribution 4.0 International** (`CC-BY-4.0`); see [`LICENSE-assets`](LICENSE-assets)
  and [`LICENSE-CC-BY-4.0.txt`](LICENSE-CC-BY-4.0.txt).

Some figures are produced from MakeHuman-community asset packs (each CC0 or
CC-BY); per-asset provenance and attributions are tracked in
[`figures/ASSETS.md`](figures/ASSETS.md).

---

## Acknowledgments

Portions of this project were developed with the assistance of
[Claude](https://www.anthropic.com/claude) (Anthropic), used as a coding and
documentation aid.