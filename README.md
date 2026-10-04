# SpongeBob SquarePants: Battle for Bikini Bottom

[![Discord Badge]][discord]
[![Build Status]][actions]

[![Perfect Match]][progress]
[![Fuzzy Match]][progress]
[![Functions]][progress]

[progress]: https://zcanann.github.io/bfbb/
[Fuzzy Match]: https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fzcanann.github.io%2Fbfbb%2Fapi.json&query=fuzzy_match&label=Close%20Match&color=yellowgreen
[Perfect Match]: https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fzcanann.github.io%2Fbfbb%2Fapi.json&query=perfect_match&label=Matching&color=limegreen
[Functions]: https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fzcanann.github.io%2Fbfbb%2Fapi.json&query=functions_matched&label=Functions&color=lavender
[Build Status]: https://github.com/zcanann/bfbb/actions/workflows/build.yml/badge.svg
[actions]: https://github.com/zcanann/bfbb/actions/workflows/build.yml
[Discord Badge]: https://img.shields.io/discord/829152115322257436?color=%237289DA&logo=discord&logoColor=%23FFFFFF
[discord]: https://discord.gg/dVbGFdYU6A

A work-in-progress decompilation of SpongeBob SquarePants: Battle for Bikini Bottom.

The build supports these GameCube releases:

| Version | Release | Required original executable | SHA-1 |
| --- | --- | --- | --- |
| `GQPE78` (default) | USA | `orig/GQPE78/sys/main.dol` | `306526d90b48e99894c3138f5fc8f2716d9fecf6` |
| `GQPP78` | Europe | `orig/GQPP78/sys/main.dol` | `6da9022f06bfb62a203017ec38046ba2566dc0cf` |
| `GU4Y78` | German compilation, BFBB disc | `orig/GU4Y78/files/Game.dol` | `aeda497067bb53e4715db5b074535645a0ea9b3e` |

The German compilation's `sys/main.dol` is its game launcher. BFBB is
`files/Game.dol`; the launcher and the other game are outside this project's
build and progress totals. All versions produce `build/<version>/main.dol`.

This repository does **not** contain game assets or original executables.
An existing copy of the corresponding release is required.

# Dependencies

## Windows

On Windows, it's **highly recommended** to use native tooling. WSL or msys2 are **not** required.  
When running under WSL, [objdiff](#diffing) is unable to get filesystem notifications for automatic rebuilds.

- Install [Python](https://www.python.org/downloads/) and add it to `%PATH%`.
  - Also available from the [Windows Store](https://apps.microsoft.com/store/detail/python-311/9NRWMJP3717K).
- Download [ninja](https://github.com/ninja-build/ninja/releases) and add it to `%PATH%`.
  - Quick install via pip: `pip install ninja`

## macOS

- Install [ninja](https://github.com/ninja-build/ninja/wiki/Pre-built-Ninja-packages):

  ```sh
  brew install ninja
  ```

- Install [wine-crossover](https://github.com/Gcenx/homebrew-wine):

  ```sh
  brew install --cask --no-quarantine gcenx/wine/wine-crossover
  ```

After OS upgrades, if macOS complains about `Wine Crossover.app` being unverified, you can unquarantine it using:

```sh
sudo xattr -rd com.apple.quarantine '/Applications/Wine Crossover.app'
```

## Linux

- Install [ninja](https://github.com/ninja-build/ninja/wiki/Pre-built-Ninja-packages).
- For non-x86(\_64) platforms: Install wine from your package manager.
  - For x86(\_64), [wibo](https://github.com/decompals/wibo), a minimal 32-bit Windows binary wrapper, will be automatically downloaded and used.

# Building

- Clone the repository:

  ```sh
  git clone https://github.com/zcanann/bfbb.git
  ```

- Using [Dolphin Emulator](https://dolphin-emu.org/), extract the release to
  `orig/<version>`. Only the executable listed above is required.
  ![](assets/dolphin-extract.png)
- Configure and build the default USA release:

  ```sh
  python configure.py
  ninja all_source progress
  ```

- Select Europe or the German compilation with `--version`:

  ```sh
  python configure.py --version GQPP78
  ninja all_source progress
  ```

  Use `--version GU4Y78` for the German BFBB executable. Configure one version
  at a time; generated Ninja and objdiff configuration follow that selection,
  while compiled objects and reports remain in separate `build/<version>`
  directories. `progress` requires the complete source build and an exact
  retail executable checksum.

CI follows the same per-version build with independent `<version>_report`
artifacts. The [progress site][progress] includes a version selector; its root
badge API continues to report USA. Source completion is verified separately for
each version, so USA matching status is not automatically applied to PAL or the
German compilation. See [regional build notes](docs/MULTIVERSION.md).

# Visual Studio Code

If desired, use the recommended Visual Studio Code settings by renaming the `.vscode.example` directory to `.vscode`.

# Diffing

Once the initial build succeeds, an `objdiff.json` should exist in the project root.

Download the latest release from [encounter/objdiff](https://github.com/encounter/objdiff). Under project settings, set `Project directory`. The configuration should be loaded automatically.

Select an object from the left sidebar to begin diffing. Changes to the project will rebuild automatically: changes to source files, headers, `configure.py`, `splits.txt` or `symbols.txt`.

![](assets/objdiff.png)
