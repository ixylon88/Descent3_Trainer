# Descent 3 Savegame Editor for Windows 98

[Deutsch](README-Deutsch.md)

**Version 1.0.0, based on `d3-historical`.**

The build produces `D3EDIT98.EXE`, a standalone native 32-bit Win32 application
with English and German labels. It imports only classic Windows system
libraries and runs without .NET, MFC, KernelEx, or an additional Visual C++
runtime. The interface is designed for **800 × 600 or higher**.

The EXE targets the classic Windows loader. It uses PE32/x86, operating system
and subsystem versions 4.0, ANSI APIs, and a custom entry point without a C
runtime.

## Getting started

1. On a modern development computer, build with `build.cmd` or `sh build.sh`. Copy the resulting `D3EDIT98.EXE` to the Windows 98 computer and run it there. The [English quick-start guide](QUICKSTART.TXT) is included in the Windows 98 ZIP as `README.TXT`.
2. Create a save in Descent 3 and exit the game.
3. Choose **Open...** and open a file such as `saveg000` through `saveg007` in the `savegame` folder. These files usually have no extension.
4. Set the values and checkboxes you want. **File / changes → Check changes** shows the proposed output and verifies it by reading it back.
5. Choose **Save + backup**, then reload the save in the game.

**Save copy...** creates a separate file with the suggested extension
`.edited`. To make Descent 3 show the copy as a slot, put it in the save folder
under a slot name such as `saveg001`. The editor leaves `.cnt` companion files
alone.

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
| Effects | Toggle god mode, cloaking, and Rapid Fire. Extend god mode and cloaking |
| Equipment | Add or remove the afterburner cooler, energy-to-shield converter, and quad lasers |
| Countermeasures | Set counts and the active countermeasure, and refill owned countermeasures to the game limit |
| Inventory / ships | View the player's inventory, edit supported items, unlock ships, and enable the Black Pyro for a briefing |
| Guide-Bot | Edit the name, radio messages, and supplies when the editor can identify the script state |
| Map | Reveal the entire saved map |
| Other values | Edit lives and score |
| Files | Review the output, write a copy, back up the original, and detect changes to the original file after opening |

Keep the last remaining primary weapon. **No secondary weapon** removes all
secondary weapons and their ammo. Clearing a weapon's checkbox sets its displayed
ammo count to zero. The editor sets the active weapon to one you still have when
needed. It preserves the flare, additional ownership bits, and other data.

See the [main guide](../README.md#inventory-countermeasures-and-ship-permissions)
for instructions on the **Inventory / ships** tab.

## Refill limits

**Max 200** sets shields and energy to their normal maximum. **Full (5 s)**
refills the afterburner to its normal capacity. **Max 1M** sets the corresponding
value to 999,999.

The normal ammo capacities come from game data. **Load ship limits...** reads
them from `Table.gam`, `d3.hog`, or a HOG2 mission archive and matches them to the
saved ship name. For mods, load the corresponding data. A later definition
for a ship replaces an earlier one.

- **Owned: ship limit** refills only owned weapons to the loaded normal ship limits.
- **Owned: 65,535** uses the maximum value of the stored ammo counter. The game may reduce ammo to its normal limits when you switch ships.
- Each **Max** button uses the loaded ship limit, or 65,535 when none is available. The limit in use appears below the weapon list.

The editor changes the ammo counts stored directly in the save. New equipment
entries receive generic names. Recreating their original icons and localized
descriptions would require the proprietary game data.

## God mode, cloaking, and equipment

Invulnerability uses a flag and remaining time in the player block. Cloaking
is stored on the player object. Turning on either effect sets its remaining
duration to **1,000,000,000 seconds**. **Extend** also sets this duration for
an active effect. Saving without changing a timed effect preserves its remaining
time.

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

The editor supports little-endian save format versions 0–2 and the
**historical 32-bit, 496-byte player block**. It rejects modern 64-bit player
blocks and preserves the supported save's original layout.

The project owner tested the original editor with real saves on Windows 98.
The current build was checked with synthetic files and audited for PE headers
and all 49 imported functions. The new features still need in-game testing.
See [`TESTS.md`](TESTS.md).

## Source code and building

[`PROJEKTSTAND-Descent3.md`](../PROJEKTSTAND-Descent3.md) documents both
versions, including sources, layouts, decisions, tests, and open issues. The
[Windows 10/11 edition](../windows10/README.md) and its C# source are in the
neighboring folder. The source package contains no prebuilt EXEs. The original
complete project archives are kept separately.

The complete native source is in `src`, with tests in `tests`. Build the EXE on
a **modern development computer** with an i686 MinGW-w64 C compiler and its
`windres` resource compiler. The Windows 98 computer only needs the resulting
EXE.

The version in `src/version.h` controls the EXE version and the display next
to the language selector.

```sh
sh build.sh
python3 tests/pe_audit.py D3EDIT98.EXE
```

On a modern Windows computer with the appropriate MinGW compiler on PATH,
use `build.cmd`. Keep the custom CRT-free entry point and build without an
additional runtime.

The editor is licensed under **GPL-3.0-or-later**. See [`LICENSE`](LICENSE).
The package contains no proprietary game content. The save format and reused
C++ declarations come from Descent 3, Copyright (C) 2024 Parallax Software.
This editor is an independent community project.
