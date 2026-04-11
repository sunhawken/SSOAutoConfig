# SSSOAutoConfig

An SKSE plugin that automatically generates the per-character config files
required by [Skyrim Save System Overhaul 3 (SSSO3)](https://www.nexusmods.com/skyrimspecialedition/mods/122343),
so you never have to dig through save filenames by hand.

## What it does

SSSO3 expects a JSON file at `Data/SSSOConf/<CharacterName>.txt` containing
the character ID embedded in your save filenames. Normally you have to find
a save on disk, copy the ID out of the filename, create the text file, and
paste it in.

This plugin does all of that automatically:

1. **On every save** &mdash; parses the save path SKSE provides.
2. **On every load** &mdash; scans the Saves folder for matching files.
3. Extracts the character ID from the filename.
4. Writes `Data/SSSOConf/<Name>.txt` if one does not already exist.

If a valid (non-`notset`) config already exists for that character, it is
left alone.

## Requirements

### Skyrim Special Edition / Anniversary Edition
- Skyrim SE 1.6.317 or newer
- [SKSE](https://skse.silverlock.org/) matching your game version
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
- [Skyrim Save System Overhaul 3](https://www.nexusmods.com/skyrimspecialedition/mods/122343)

### Skyrim VR
- Skyrim VR 1.4.15.0
- [SKSE VR](https://skse.silverlock.org/)
- [Address Library for SkyrimVR](https://www.nexusmods.com/skyrimspecialedition/mods/58101)
  (the `version-1-4-15-0.csv` file in `Data/SKSE/Plugins/`)
- SSSO3 &mdash; if an SSSO3 VR build exists, otherwise the plugin will still
  run harmlessly but the generated config files won't be consumed

## Installation

Grab the latest `SSSOAutoConfig-*.zip` from the [Releases](https://github.com/sunhawken/SSOAutoConfig/releases)
page and drop the contents into your Skyrim install, or install the zip with
Vortex / Mod Organizer 2 like any other mod. A single DLL works for both
Skyrim SE/AE and Skyrim VR; install it under the appropriate game:

```
<Skyrim SE>/Data/SKSE/Plugins/SSSOAutoConfig.dll
<Skyrim VR>/Data/SKSE/Plugins/SSSOAutoConfig.dll
```

The plugin auto-detects which runtime it is loaded into via
`REL::Module::IsVR()` and uses the correct `My Games` subfolder
(`Skyrim Special Edition` vs `Skyrim VR`) when scanning for saves.

## How it works

Skyrim save filenames hex-encode the character name:

```
Save1_8981A6BA_0_4A616C_Tamriel_000113_20260406140126_1_1.ess
        \________________/
        character ID  (4A616C = "Jal")
```

SSSO3 needs that ID written into `Data/SSSOConf/Jal.txt`:

```json
{
    "information" : {
        "ID" : "8981A6BA_0_4A616C"
    }
}
```

The plugin hooks SKSE save/load events, converts the player name to hex,
finds the matching segment in the filename, and writes the JSON config.

## Unicode / Korean support

Player names containing non-ASCII characters (Korean Hangul, Japanese,
Chinese, Cyrillic, etc.) are supported. The plugin converts the player name
through UTF-16 before constructing the config file path so the filename
matches what the OS expects, regardless of the system ANSI codepage. The
hex-encoding loop operates on raw bytes, so it transparently handles UTF-8
multi-byte sequences as long as Skyrim writes the save filename using the
same encoding it returns from `GetName()`.

## Building from source

Prerequisites:

- Visual Studio 2022 or later with C++ workload
- CMake 3.24+
- [vcpkg](https://github.com/microsoft/vcpkg) with the
  [colorglass registry](https://gitlab.com/colorglass/vcpkg-colorglass)
  for `commonlibsse-ng`

```sh
git clone https://github.com/sunhawken/SSOAutoConfig.git
cd SSOAutoConfig
cmake --preset release
cmake --build build --preset release
```

The DLL is produced at `build/Release/SSSOAutoConfig.dll`. Post-build copy
steps deploy it automatically when you set either or both of these cache
variables in `CMakePresets.json`:

- `SKYRIM_FOLDER`   &mdash; your Skyrim SE install root
- `SKYRIMVR_FOLDER` &mdash; your Skyrim VR install root

Both point at the game directory (not `Data/`); the build appends
`Data/SKSE/Plugins/` automatically. A single DLL works for both games
thanks to CommonLibSSE-NG's multi-runtime support.

The build is statically linked &mdash; no external runtime DLLs (no
`spdlog.dll`, no `fmt.dll`, no MSVC runtime).

## Compatibility

- Skyrim SE 1.6.317 through 1.6.1179+ (Address Library handles version drift)
- Skyrim VR 1.4.15.0 (requires Address Library for SkyrimVR)
- Works alongside any other SKSE plugins
- Does not modify game records, scripts, or any other files
- Safe to install or remove at any time

## Notes

- Old-format saves (e.g. `Save 2 - Marina  Skyrim  00.27.31.ess`) do not
  contain hex-encoded character IDs. The plugin silently skips them &mdash;
  make one new save and the next load will pick it up.
- If your character name contains filesystem-invalid characters
  (`< > : " / \ | ? *`), the config file cannot be created. Use SSSO3's
  built-in "Bypass Check" option instead.
- Logs go to `Documents/My Games/Skyrim Special Edition/SKSE/SSSOAutoConfig.log`
  on SE, or `Documents/My Games/Skyrim VR/SKSE/SSSOAutoConfig.log` on VR.

## License

**All Rights Reserved** &mdash; see [LICENSE](LICENSE).

You are free to download a release build and use it as a Skyrim mod on
your own machine. Copying, modifying, redistributing, or bundling this
software (binary or source) with any other project requires prior written
permission from the author.

## Author

**Young Jonii** &mdash; [@sunhawken](https://github.com/sunhawken)
