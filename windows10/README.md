# Descent 3 Savegame Editor · Windows 10 / 11

[Deutsch](LIESMICH.md)

**Version 1.0.0.** The editor uses WPF and .NET Framework 4.8. It runs without network access or administrator privileges. Saving requires write access to the destination folder.

## Getting started

1. Extract the entire program ZIP and run **D3SaveEditor.exe**. Keep `D3SaveEditor.Core.dll` and `D3SaveEditor.exe.config` in the same folder. The `de` subfolder contains the German translation. To run from source, see [Source code and building](#source-code-and-building).
2. Create a save in Descent 3 and exit the game.
3. Click **Open save…** in the editor. Descent 3 typically names its save files `saveg000` through `saveg007`, without a file extension, in the `savegame` subfolder of the writable game directory. You can also drop a file into the window or pass it as a command-line argument.
4. Set the values and checkboxes you want. **File / changes → Check changes** shows the proposed changes.
5. **Save + backup** replaces the open save and creates a dated backup in the same folder. Reload the save in the game.

**Save copy…** writes a separate file with the suggested extension `.edited`. To make Descent 3 show the copy as a slot, place it in the correct save folder under a name such as `saveg001`. An existing destination file is backed up before it is overwritten. The editor leaves `.cnt` files alone.

### Optional launcher animation

To show the orb next to the image in the top right, supply the nine original BMPs,
`Anim0.bmp` through `Anim8.bmp`, from the
[Descent 3 launcher](https://github.com/DescentDevelopers/Descent3/tree/de4bebf5f2d2523261956d0f28603e426eb88c63/legacy/D3Launch/res).
You can place them next to `D3SaveEditor.exe`. For local builds, put them in
`pics/` at the repository root and the build will copy them to the output folder.
Git ignores these local frames.

The editor loads all nine frames into memory on startup. When all frames are
present and readable, the orb advances to the next frame every 100 ms at its
original size of 64 × 64 pixels. The orb stays hidden if any frame is missing
or unreadable.

## Features

| Area | Options |
|---|---|
| Primary weapons | Add or remove all ten weapons individually, choose the active weapon, and edit ammo for Vauss, Mass Driver, and Napalm |
| Secondary weapons | Add or remove all ten weapons individually, choose the active weapon, and set ammo counts from 0 to 65,535 |
| Supplies | Set shields and energy or fill them to 200, and set the afterburner charge or refill it to 5 seconds |
| Effects | Toggle god mode, cloaking, and Rapid Fire. Extend god mode and cloaking |
| Equipment | Add or remove the afterburner cooler, energy-to-shield converter, and quad lasers |
| Countermeasures | Set counts and the active countermeasure, and refill owned countermeasures to the game limit |
| Inventory / ships | View the player's inventory, edit supported items, unlock ships, and enable the Black Pyro for a briefing |
| Guide-Bot | Edit the name, radio messages, and supplies when the editor can identify the script state |
| Map | Reveal the entire saved map. Leave the box unchecked to keep the saved exploration |
| Other values | Edit lives and score |
| Files | Review changes, save a copy, create automatic backups, and detect changes to the original file after opening |

Keep at least one primary weapon from the save. If you remove the active weapon,
the editor selects one you still have when needed. Clearing a weapon's checkbox
sets its displayed ammo count to zero. The editor preserves the flare and any
additional unknown ownership bits.

See the [main guide](../README.md#inventory-countermeasures-and-ship-permissions)
for instructions on the **Inventory / ships** tab.

## Refill limits

**Max 200** sets shields and energy to their normal maximum. **Full (5 s)**
refills the afterburner to its normal capacity. **Max 1M** sets the corresponding
value to 999,999.

The save contains current ammo counts. Normal ship capacities come from the
proprietary game data. The editor provides two ways to refill ammo.

- **Owned: ship limit** uses the ship's normal capacities. Click **Load ship limits…** and open `Table.gam`, `d3.hog`, or the appropriate HOG2 mission archive from your installation. The editor matches the limits to the saved ship name. For mods, load the corresponding definitions. Later definitions override earlier ones with the same name.
- **Owned: 65,535** uses the maximum value of the unsigned 16-bit ammo counter. The game may reduce ammo to its normal limits when you switch ships.

Each **Max** button uses the loaded ship limit, or 65,535 when there is no
definition for the current ship. The editor shows which limit applies and refills
ammo only for weapons with their checkboxes selected.

## God mode and cloaking

Descent 3 saves a flag and the remaining time for each effect. When you turn
an effect on, the editor sets its duration to **1,000,000,000 seconds** in the
save file. Level changes, missions, scripts, or the game itself can end the effects.

Opening and saving a file without changing an existing timed power-up preserves
its remaining duration. Click **Extend** to renew an effect that is already
active. The editor changes cloaking in the effect block of the player object
and god mode in the player block. It sets quad lasers in the inventory and
in both laser weapon batteries.

## Restore a backup

Overwriting a save creates a backup such as `saveg000.bak-20261024-203000-123-a1b2c3`.
To restore it, exit the game and copy the backup over the original slot file,
such as `saveg000`. The editor preserves existing backups.

## Compatibility and testing

The editor uses the [Descent 3 source code](https://github.com/DescentDevelopers/Descent3)
at commit `76049bb32d0a391ab35aedcd0716073c1e64d72a` from October 23, 2026.

It supports the documented little-endian save format versions **0, 1, and 2**,
and the known Windows player structures of **496 bytes (32-bit)** and
**536 bytes (64-bit)**. The editor writes each save using its original layout.
It rejects unsupported builds and structures it cannot reliably identify.

The original editor was tested with real saves. The automated data and WPF tests
use synthetic files. See [`TESTS.md`](TESTS.md).

The source code describes the save format. A playable save also needs a
complete level state, including its mission, objects, and scripts. Start with
a file previously saved in the game.

## Source code and building

Open [`D3SaveEditor.sln`](D3SaveEditor.sln) in Visual Studio 2022 or newer and
press **F5** to start **D3SaveEditor.Gui**. Use the Visual Studio Installer to
install the **.NET desktop development** workload and the
**.NET Framework 4.8 targeting pack**.

| Project | Responsibilities |
| --- | --- |
| `D3SaveEditor.Gui` | WPF window in `MainView.xaml`, UI logic in `MainView.xaml.cs`, dialogs, and application startup |
| `D3SaveEditor.Core` | Save format, editing, file access, backups, and ship limits from game data |
| `D3SaveEditor.Tests` | Data tests and the WPF UI test in Visual Studio Test Explorer |

The `Version` property in `D3SaveEditor.Gui/D3SaveEditor.Gui.csproj` sets both
the product version and the version shown by the language selector.

The WPF service `LocalizationService` reads UI text from `UiStrings.resx` and
`UiStrings.de.resx`. WPF updates bound text through `INotifyPropertyChanged`
when the language changes. `Messages.resx` and `Messages.de.resx` contain
validation messages, change summaries, and weapon and countermeasure names
for the editor. Descent 3 uses its own language files. English is the neutral
language. The editor starts in German if Windows uses German as its display
language.

From the repository root on Windows, `build-windows10.cmd` builds the editor
in the Release configuration with the .NET SDK. `test-windows10.cmd` builds
and runs the test project. The program files are in `D3SaveEditor.Gui/bin/Debug/net48`
or `D3SaveEditor.Gui/bin/Release/net48`. The test project uses MSTest from NuGet.

The editor is licensed under **GPL-3.0-or-later**. See [`LICENSE`](LICENSE).
The package contains no proprietary game content. The save format reference and
reused C++ declarations come from Descent 3, Copyright (C) 2024 Parallax Software.
This editor is an independent community project.
