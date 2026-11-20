# Descent 3 Savegame Editor for Windows 98

[Deutsches Original](windows98-README-Deutsch.md) · [Current manual](../../windows98/README.md)

**Archived English translation of the source-based version 1.0 preview,
based on `d3-historical`.**

`D3EDIT98.EXE` was a standalone native 32-bit Win32 application with German
labels. It imported only classic Windows system libraries and ran without
.NET, MFC, KernelEx, or an additional Visual C++ runtime. The interface was
designed for **800 × 600 or higher**.

The EXE targeted the classic Windows loader. It used PE32/x86, operating system
and subsystem versions 4.0, ANSI APIs, and a custom entry point without the C
runtime. The build omitted a Unicode layer, a .NET header, and newer
initialization routines. **Starting the editor on Windows 98 and loading an
edited save in the game had yet to be tested.** The preview had undergone
static checks.

## Getting started

1. Extract the archive and copy `D3EDIT98.EXE` to the Windows 98 computer. The EXE runs on its own. `LIESMICH.TXT` contains the quick-start guide.
2. Create a save in Descent 3 and exit the game.
3. Choose **Oeffnen…** (Open) and open a file such as `saveg000` through `saveg007` in the `savegame` folder. These files usually have no extension.
4. Set the values and checkboxes you want. **Datei / Pruefung → Aenderungen pruefen** (File / changes → Check changes) shows the proposed output and verifies it by reading it back.
5. Choose **Speichern + Backup** (Save + backup), then reload the save in the game.

**Kopie speichern…** (Save copy) creates a separate file with the suggested
extension `.edited`. To make Descent 3 show the copy as a slot, put it in the
save folder under a slot name such as `saveg001`. The editor leaves `.cnt`
companion files alone.

The editor uses ANSI file paths on Windows 98. The complete destination path
can contain up to 200 characters to leave room for backup names. The editor
supports saves up to 32 MiB. Use a short path such as
`C:\D3\savegame\saveg000` to stay within the limit.

## Features

| Area | Options |
|---|---|
| Primary weapons | Select or clear each of the ten weapons, choose the active weapon, and set stored ammo counts for Vauss, Mass Driver, and Napalm |
| Secondary weapons | Select or clear each of the ten weapons, choose the active weapon, and set ammo counts from 0 to 65,535 |
| Supplies | Set shields and energy or fill each to 200, and refill the afterburner to 5 seconds |
| Effects | Toggle god mode and cloaking, and renew their remaining duration |
| Equipment | Add or remove the afterburner cooler, energy-to-shield converter, and quad lasers |
| Map | Reveal the entire saved map |
| Other values | Edit lives and score |
| Files | Review the output, write a copy, back up the original, and detect changes to the original file after opening |

Keep the last remaining primary weapon. **Keine Sekundaerwaffe** (No secondary
weapon) removes all secondary weapons and their ammo. Clearing a weapon's
checkbox sets its displayed ammo count to zero. The editor selects an active
weapon you still have when needed. It preserves the flare, additional ownership
bits, and other data.

## Maximum ammo

Normal ammo capacities come from game data. **Schiffslimits laden…** (Load ship
limits) reads them from `Table.gam`, `d3.hog`, or a HOG2 mission archive and
matches them to the saved ship name. For mods, load the corresponding data.
A later definition for a ship replaces an earlier one.

- **Vorhandene: Schiff-Max** (Owned: ship limit) refills only owned weapons to the loaded normal ship limits.
- **Vorhandene: 65.535** (Owned: 65,535) uses the maximum value of the stored ammo counter. The game may reduce ammo to its normal limits when you switch ships.
- Each **Max** button uses the loaded ship limit, or 65,535 when none is available. The limit in use appears below the weapon list.

The editor changes the ammo counts stored directly in the save. New equipment
entries receive generic names. Recreating their original icons and localized
descriptions would require the proprietary game data.

## God mode, cloaking, and equipment

Invulnerability uses a flag and remaining time in the player block. Cloaking
is stored on the player object. Turning on either effect sets its remaining
duration to **1,000,000,000 seconds**. **Erneuern** (Renew) also sets this duration
for an active effect. Saving without changing a timed effect preserves its
remaining time.

The editor sets quad lasers in the inventory and both laser weapon batteries.
The afterburner cooler is the inventory item `Afterburner` and the converter is
`Converter`. You can add items when their definitions are in the save's
translation table.

The editor works on saved files and leaves the game EXE untouched. Level
changes, deaths, scripts, and game rules can reset effects or equipment.

## Backups and recovery

Overwriting a save creates a backup such as `saveg000.bak-20261024-210000-0`.
The editor first verifies the new file, writes it to the destination folder,
and flushes it to disk. It then renames the original to the backup name and
puts the new file in its place. If the last step fails, the editor tries to
restore the original. If restoration fails too, the error message gives the
path to the preserved backup.

On FAT, the file replacement uses two separate renames. A power loss in between
can temporarily leave the slot file missing. To recover, copy the preserved
backup back to the original name, such as `saveg000`. The editor preserves
existing backups.

## Compatibility and testing

The format is based on the [`d3-historical` branch](https://github.com/DescentDevelopers/Descent3/tree/d3-historical)
at commit `84e25b0b411284bad57b34f50fd976f1a8a2367f` from April 16, 2024.

The preview supports little-endian save format versions 0–2 and the
**historical 32-bit, 496-byte player block**. It rejects modern 64-bit player
blocks and preserves the supported save's original layout.

**The preview passed 299 data and file checks.** These included byte-for-byte
comparisons with the separate C# implementation, SHA-256 test vectors, six
historical test variants, and simulated failures during file replacement and
rollback. AddressSanitizer and UndefinedBehaviorSanitizer were enabled. The
built EXE was audited for PE headers and all 49 imported functions. See the
original package's `TESTS.md` for details.

The preview was tested without real saves or proprietary game data. The test
files were generated from the historical C++ declarations and could not be
played. They were not distributed as saves. The native UI and loading a save
in the game still needed testing on Windows 98.

## Source code and building

`PROJEKTSTAND-Descent3.md` documented both versions at the time, including
sources, layouts, decisions, tests, and open issues. The original package also
included `Vorgaenger-Windows10.zip`, the full previous Windows 10/11 package
with C# source code, so development could resume from that archive.

The complete native source was in `src`, and the tests were in `tests`.
Rebuilding required an i686 MinGW-w64 C compiler on a **modern development
computer**. The Windows 98 computer needed only the resulting EXE.

```sh
sh build.sh
python3 tests/pe_audit.py D3EDIT98.EXE
```

On a modern Windows computer with the appropriate MinGW compiler on PATH,
use `build.cmd`. Keep the custom CRT-free entry point and build without an
additional runtime.

The editor was licensed under **GPL-3.0-or-later**. See `LICENSE`. The save
format and reused C++ declarations came from Descent 3, Copyright (C) 2024
Parallax Software. The package contained no proprietary game content. This
was an independent editor with no official release from the Descent developers.
