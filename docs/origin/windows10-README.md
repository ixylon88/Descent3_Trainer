# Descent 3 Savegame Editor · Windows 10 / 11

[Deutsches Original](windows10-LIESMICH.md) · [Current manual](../../windows10/README.md)

**Archived English translation of the source-based version 1.0 preview.**
The original package included the built Windows application and its complete
source code. It required no installation. The EXE used .NET Framework 4.x and
Windows Forms. It ran without network access or administrator privileges.
Saving required write access to the destination folder.

## Getting started

1. Extract the entire ZIP and double-click **D3SaveEditor.exe**. Keep `D3SaveEditor.exe.config` next to it.
2. Create a save in Descent 3 and exit the game.
3. Click **Spielstand öffnen…** (Open save) in the editor. Descent 3 typically names its save files `saveg000` through `saveg007`, without an extension, in the `savegame` subfolder of the writable game directory. You can also drop a file into the window or pass it as a command-line argument.
4. Set the values and checkboxes you want. **Datei / Änderungen → Änderungen prüfen** (File / changes → Check changes) shows the proposed changes.
5. **Speichern + Backup** (Save + backup) replaces the open save and creates a dated backup in the same folder. Reload the save in the game.

**Kopie speichern…** (Save copy) writes a separate file with the suggested
extension `.edited`. To make Descent 3 show the copy as a slot, place it in
the correct save folder under a name such as `saveg001`. An existing
destination file is backed up before it is overwritten. The editor leaves
`.cnt` files alone.

## Features

| Area | Options |
|---|---|
| Primary weapons | Add or remove all ten weapons individually, choose the active weapon, and edit ammo for Vauss, Mass Driver, and Napalm |
| Secondary weapons | Add or remove all ten weapons individually, choose the active weapon, and set ammo counts from 0 to 65,535 |
| Supplies | Set shields and energy or fill them to 200, and set the afterburner charge or refill it to 5 seconds |
| Effects | Toggle god mode and cloaking, and renew their long remaining duration |
| Equipment | Add or remove the afterburner cooler, energy-to-shield converter, and quad lasers |
| Map | Reveal the entire saved map. Leave the box unchecked to keep the saved exploration |
| Other values | Edit lives and score |
| Files | Review changes, save a copy, create automatic backups, and detect changes to the original file after opening |

Keep at least one primary weapon from the save. If you remove the active
weapon, the editor selects one you still have when needed. Clearing a weapon's
checkbox sets its displayed ammo count to zero. The editor preserves the flare
and any additional unknown ownership bits.

## Refill limits

The normal maximum in the source code is 200 for both shields and energy.
You can enter higher values manually if needed.

The save contains current ammo counts. Normal ship capacities come from the
proprietary game data. The preview offered two ways to refill ammo.

- **Vorhandene: Schiff-Max** (Owned: ship limit) uses the ship's normal capacities. Click **Schiffslimits laden…** (Load ship limits) and open `Table.gam`, `d3.hog`, or the appropriate HOG2 mission archive from your installation. The editor matches the limits to the saved ship name. For mods, load the corresponding definitions. Later definitions override earlier ones with the same name.
- **Vorhandene: 65.535** (Owned: 65,535) uses the maximum value of the unsigned 16-bit ammo counter. The game may reduce ammo to its normal limits when you switch ships.

Each **Max** button uses the loaded ship limit, or 65,535 when there is no
definition for the current ship. The editor shows which limit applies and
refills ammo only for weapons with their checkboxes selected.

## God mode and cloaking

Descent 3 saves a flag and the remaining time for each effect. When you turn
an effect on, the editor sets its duration to **1,000,000,000 seconds** in the
save file. Level changes, missions, scripts, or the game itself can end the
effects.

Opening and saving a file without changing an existing timed power-up
preserves its remaining duration. Click **Erneuern** (Renew) to renew an
effect that is already active. The editor changes cloaking in the effect
block of the player object and god mode in the player block. It sets quad
lasers in the inventory and in both laser weapon batteries.

## Restore a backup

Overwriting a save creates a backup such as `saveg000.bak-20261024-203000-123-a1b2c3`.
To restore it, exit the game and copy the backup over the original slot file,
such as `saveg000`. The editor preserves existing backups.

## Compatibility and testing

The preview was based on the [Descent 3 source code](https://github.com/DescentDevelopers/Descent3)
at commit `76049bb32d0a391ab35aedcd0716073c1e64d72a` from October 23, 2026.

It supports the documented little-endian save format versions **0, 1, and 2**,
and the known Windows player structures of **496 bytes (32-bit)** and
**536 bytes (64-bit)**. Edited saves keep their original layout. The editor
rejects unsupported builds and structures it cannot reliably identify.

**The preview was tested with synthetic, non-playable files.** An independent
C++ generator wrote the player, effect, and weapon battery structures from
the source declarations. Loading an edited save in the game and starting the
editor on Windows had yet to be verified. The UI was tested with Mono/X11.
See the original package's `TESTS.md` for details.

The source code describes the save format. A playable save also needs a
complete level state, including its mission, objects, and scripts. Start with
a file previously saved in the game.

## Source code and building

In the original package, `src` contained the complete editor. On Windows,
`build.cmd` could rebuild the editor with the .NET Framework C# compiler
without NuGet packages. On Linux with Mono, the build command was

```sh
mcs -target:winexe -platform:anycpu -optimize+ \
  -win32manifest:src/app.manifest -out:D3SaveEditor.exe \
  -r:System.Windows.Forms -r:System.Drawing \
  src/AssemblyInfo.cs src/SaveGame.cs src/ShipLimits.cs src/Program.cs
```

The editor was licensed under **GPL-3.0-or-later**. See `LICENSE`.
The package contained no proprietary game content. The save format reference
and reused C++ declarations came from Descent 3, Copyright (C) 2024 Parallax
Software. This was an independent editor with no official release from the
Descent developers.
