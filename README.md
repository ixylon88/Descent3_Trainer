# Descent 3 Savegame Editor

[Deutsch](LIESMICH.md)

Source code for two graphical offline editors for Descent 3 savegames.
Developed with help from ChatGPT (OpenAI).

**Version 1.0.0.** The manual for each edition lists the supported saves and the tests that have been run.

## Editions

| Folder | Target system and technology | Save format reference |
| --- | --- | --- |
| [`windows10`](windows10/README.md) | Windows 10/11, C#, .NET Framework 4.8, WPF | Descent 3 `main` at revision `76049bb32d0a391ab35aedcd0716073c1e64d72a`, with known player layouts of 496 and 536 bytes |
| [`windows98`](windows98/README.md) | Windows 98, native C and ANSI Win32, with no additional runtime packages | Descent 3 `d3-historical` at revision `84e25b0b411284bad57b34f50fd976f1a8a2367f`, with a 496-byte player layout |

The editor's target operating system and the layout of the save file are
independent. The editors preserve the original save format.

## Features

- Add or remove individual primary weapons, secondary weapons, and countermeasures. Set the active selection and ammo counts.
- Read normal ammo limits from your own game data or set the technical maximum of 65,535.
- Edit shields, energy, afterburner charge, lives, and score, including values above the in-game limits.
- Set god mode, cloaking, Rapid Fire, the afterburner cooler, the energy-to-shield converter, and quad lasers.
- View the items stored in a save. Edit counts for Force Wall, Go Wingnut, and the five standard countermeasures when the corresponding definitions are present in the save.
- Unlock the Pyro-GL, Phoenix, Magnum-AHT, and Black Pyro when they are defined in the save. Have the game choose the Black Pyro automatically at the briefing, including in the main campaign.
- Reveal the entire saved map.
- Review proposed changes and create backups when overwriting a save.
- Switch between English and German in the language menu. The **Help** tab contains instructions.

The manuals for each edition cover requirements, limits, and instructions in
detail. The repository contains no proprietary game data or real saves.

In the **Effects** group on the player tab, **Rapid Fire** multiplies the base
delay between shots by `0.7`, the same value used by the
[Rapid Fire pickup](https://github.com/DescentDevelopers/Descent3/blob/de4bebf5f2d2523261956d0f28603e426eb88c63/scripts/generic.cpp#L380).
Turning it off restores the normal multiplier of `1.0`. An existing pickup
timer can end the saved effect.

## Inventory, countermeasures, and ship permissions

The **Saved entries** list on the **Inventory / ships** tab displays the
player's inventory. The **Countermeasures** tab lets you edit counts and choose
the active countermeasure. A count of `0` removes an editable entry.
**Owned: game limit** fills Chaff, Bouncing Betty, Seeker Mine, and Proximity
Mine to 12 each, and Gunboy to 4. The names of the 20 standard weapons and
five countermeasures follow the
[Descent 3 string file](https://github.com/DescentDevelopers/Descent3/blob/de4bebf5f2d2523261956d0f28603e426eb88c63/scripts/data/fullhog/D3.STR).
Editing counts preserves the original descriptions, icons, and flags of existing
entries. New entries receive a text name and an empty icon field.

Force Wall and Go Wingnut come from the general game scripts. Their availability
as player items and their effects depend on the game version and game data.
The player inventory also displays mission items and existing Guide-Bot entries.

The checkboxes add ship permissions to the save. The regular single-player
briefing offers the Pyro-GL, Phoenix, and Magnum-AHT. In Mercenary, a script at
the end of level 2 unlocks the Black Pyro and locks the Phoenix.

**Allow only the Black Pyro** clears the other ship checkboxes. At the next
suitable briefing, the game automatically selects the Black Pyro as the only
permitted ship. This worked in the **main campaign** when advancing from level 1
to level 2 in the game. The button is also available in Mercenary if the save
contains a ship definition for the Black Pyro. The game may also carry the ship
permissions over to the pilot profile as you progress.

### Save format and game rules

- Player data and both inventories in [`main`](https://github.com/DescentDevelopers/Descent3/blob/de4bebf5f2d2523261956d0f28603e426eb88c63/Descent3/gamesave.cpp#L962-L975) and [`d3-historical`](https://github.com/DescentDevelopers/Descent3/blob/84e25b0b411284bad57b34f50fd976f1a8a2367f/Descent3/gamesave.cpp#L1014-L1030).
- [Pickup definitions and quantity limits](https://github.com/DescentDevelopers/Descent3/blob/de4bebf5f2d2523261956d0f28603e426eb88c63/Descent3/multisafe.cpp#L2800-L2981).
- [Ship names allowed in the briefing](https://github.com/DescentDevelopers/Descent3/blob/de4bebf5f2d2523261956d0f28603e426eb88c63/Descent3/ship.cpp#L124-L129), [automatic selection](https://github.com/DescentDevelopers/Descent3/blob/de4bebf5f2d2523261956d0f28603e426eb88c63/Descent3/TelCom.cpp#L3425-L3456), and the [Mercenary script](https://github.com/DescentDevelopers/Descent3/blob/de4bebf5f2d2523261956d0f28603e426eb88c63/scripts/Merc02.cpp#L5095-L5104).

## Guide-Bot

The **Guide-Bot** tab edits the robot's name in the saved level, regular radio
messages, and supplies of Fire Extinguisher, Go Wingnut, Remote Control,
Guardian, and Antivirus. The name can contain up to seven ANSI characters, and
each supply can have a count from 0 to 127. The tab displays the saved shield
value. The fields are available when the editor can identify the player's
Guide-Bot object and its OSIRIS script block and recognizes the saved layout.
The tests use synthetic saves for both ABI variants.

- [Guide-Bot script state](https://github.com/DescentDevelopers/Descent3/blob/de4bebf5f2d2523261956d0f28603e426eb88c63/scripts/AIGame.cpp#L1476-L1536) and [serialization of OSIRIS script blocks](https://github.com/DescentDevelopers/Descent3/blob/de4bebf5f2d2523261956d0f28603e426eb88c63/Descent3/OsirisLoadandBind.cpp#L2993-L3015).

## Builds on GitHub

Both workflows run when you push to `dev`. You can also use **Run workflow**
under **Actions** to start them manually.

| Workflow | Tests and build | ZIP under **Artifacts** |
| --- | --- | --- |
| Windows 98 | MinGW-w64 build on Ubuntu, native data tests with ASan/UBSan, and an audit of the Win98 imports | `Descent3_Trainer-Win98` with `D3EDIT98.EXE` |
| Windows 10 and 11 | Data tests with Mono on Ubuntu, plus a WPF build and UI test on Windows | `Descent3_Trainer-Win10-Win11` with the program files and the `de` folder for the German translation |

After a successful run, download the ZIP from the run page. It contains the
program files, quick-start instructions, and license notices. The Windows 98 ZIP
includes the [English quick-start guide](windows98/QUICKSTART.TXT) as `README.TXT`
and the [German quick-start guide](windows98/LIESMICH.TXT). `SOURCE.txt`
links to a separate GitHub archive with the complete source code and build
scripts for that commit. `SHA256SUMS.txt` contains checksums for the program files.

## Build and test

The scripts are in the repository root. GitHub Actions uses the same scripts.

| Target | Environment | Build | Test |
| --- | --- | --- | --- |
| Windows 10/11 | Windows with the .NET SDK and .NET Framework 4.8 targeting pack | `build-windows10.cmd` | `test-windows10.cmd` |
| Windows 10/11 | Linux with C++17 and Mono development tools | | `sh test-windows10.sh` for the data tests |
| Windows 98 | Linux with i686 MinGW-w64, GCC/G++, and Python 3 | `sh build-windows98.sh` | `sh test-windows98.sh` after building |
| Windows 98 | Windows with i686 MinGW-w64 | `build-windows98.cmd` | |

Open [`windows10/D3SaveEditor.sln`](windows10/D3SaveEditor.sln) in Visual Studio.
Press **F5** to start `D3SaveEditor.Gui`. In **Test Explorer**, **Run All Tests**
runs the data tests and the WPF UI test from `D3SaveEditor.Tests`.

The Windows 98 tests run on the Linux development machine with ASan/UBSan and
check the imports of the built `windows98/D3EDIT98.EXE`. Copy the resulting EXE
to the Windows 98 computer. See
[`windows10/TESTS.md`](windows10/TESTS.md) and [`windows98/TESTS.md`](windows98/TESTS.md).

`SOURCE-PROVENANCE.json` records file hashes and the origin of the source
packages that were originally imported.

## License and provenance

The editor is licensed under **GPL-3.0-or-later**. See [`LICENSE`](LICENSE).
The original SPDX and copyright notices are included. The save format reference
and reused C++ declarations come from Descent 3, Copyright (C) 2024 Parallax
Software, from [DescentDevelopers/Descent3](https://github.com/DescentDevelopers/Descent3).

The earlier Win98 package included the MIT license text in `LICENSE`, while
its README and source files specified GPL-3.0-or-later. This package includes
the corresponding GPLv3 text. The former MIT license file is preserved for
provenance under `docs/origin`. It grants no additional MIT license for the
editor as a whole. See [`NOTICE.md`](NOTICE.md).

When publishing on GitHub, include the license texts and existing copyright and
provenance notices. If you publish EXE files, also make the complete corresponding
source code and build scripts available. The applicable licenses govern
permission to publish. Third-party rights still apply to OpenAI output.

## Credits

Project idea and requirements by ixylon88. The editors and their documentation
were developed in collaboration with ChatGPT/Codex (OpenAI). This credits the
AI tool's contribution without claiming copyright for ChatGPT/Codex or review
or approval by OpenAI. The existing provenance notices credit the contributions
from Descent 3.

An independent community project. See the technical project status in
[`PROJEKTSTAND-Descent3.md`](PROJEKTSTAND-Descent3.md).
