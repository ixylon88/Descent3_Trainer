# Descent 3 Savegame Editor

Quelltexte zweier grafischer Offline-Editoren für Descent-3-Spielstände.
Mit Hilfe von ChatGPT (OpenAI) entwickelt.

**Vorschauversion 1.0:** Beide Versionen wurden unter den dafür entwickelten Umgebungen getestet und sind vorläufig einsatztauglich (beta).

## Varianten

| Ordner | Zielsystem und Technik | Grundlage des Dateiformats |
| --- | --- | --- |
| [`windows10`](windows10/README.md) | Windows 10/11, C#, .NET Framework 4.x, Windows Forms | Descent3 `main`, Revision `76049bb32d0a391ab35aedcd0716073c1e64d72a`; bekannte Spielerlayouts 496/536 Byte |
| [`windows98`](windows98/README.md) | Windows 98, natives C/ANSI-Win32, ohne zusätzliche Laufzeitpakete | Descent3 `d3-historical`, Revision `84e25b0b411284bad57b34f50fd976f1a8a2367f`; Spielerlayout 496 Byte |

Die Betriebssystemvariante und das Layout des gespeicherten Spielstands sind
verschiedene Eigenschaften. Die Editoren konvertieren keine Speicherformate.

## Funktionen

- Primär- und Sekundärwaffen einzeln hinzufügen/entfernen und aktive Waffen auswählen.
- Schild, Energie, Munition, Nachbrennerladung, Leben und Punktestand bearbeiten.
- God-Mode, Tarnung, Nachbrenner-Kühler, Konverter und Quad-Laser setzen.
- Gegenstände und Gegenmaßnahmen im Spielstand einsehen. Mengen für Force Wall,
  Go Wingnut sowie die fünf Standard-Gegenmaßnahmen bearbeiten, sofern die
  passenden Definitionen im Spielstand vorhanden sind.
- Pyro-GL, Phoenix und Magnum-AHT für die Schiffsauswahl der Kampagne freischalten.
- Gespeicherte Kartensichtbarkeit vollständig aufdecken.
- Normale Munitionslimits aus eigenen Spieldaten lesen; alternativ 65.535 als technisches Maximum setzen.
- Geplante Ausgabe prüfen und beim Überschreiben Backups anlegen.

Die genauen Voraussetzungen, Grenzen und Bedienhinweise stehen in den jeweiligen
README-Dateien. Proprietäre Spieldaten und echte Spielstände sind nicht enthalten.

## Inventar und Schiffsfreigaben

Im Reiter **Inventar / Schiffe** zeigt die Liste die gespeicherten Einträge
beider Inventare an. Für die bearbeitbaren Einträge entfernt die Menge `0` den
jeweiligen Eintrag. In der Standardausrüstung tragen Chaff, Bouncing Betty,
Seeker Mine und Proximity Mine jeweils bis zu 12 Stück. Beim Gunboy sind es 4.
Die Originalbeschreibungen, Symbole und Flags vorhandener Einträge bleiben bei
Mengenänderungen erhalten. Neu angelegte Einträge erhalten einen Textnamen und
ein leeres Symbolfeld.

Force Wall und Go Wingnut stammen aus den allgemeinen Spielskripten. Ob sie
als Spieler-Gegenstände verfügbar sind und welche Wirkung sie haben, hängt
von der Spielversion und den Spieldaten ab. Die Liste zeigt auch Guide-Bot und
Missionsgegenstände an. Deren Zustand kann an Levelobjekte und Skripte gekoppelt
sein.

Die Häkchen ergänzen die Schiffserlaubnisse im geladenen Spielstand. Das Schiff
wählst du beim nächsten Briefing mit Schiffsauswahl im Spiel. Das Pilotenprofil
speichert seine Freigaben pro Kampagne weiterhin über den normalen Spielfortschritt.

Das reguläre Einzelspielermenü in `main` und `d3-historical` bietet die drei
Standardschiffe an. In Mercenary schaltet das Level-2-Skript den Black Pyro frei
und sperrt den Phoenix. Für den Black Pyro in der Hauptkampagne gibt es die
historische Mod [Black Pyro Justice](https://www.descentforum.net/Pyromania/files/).

Quelltextstellen für das Format und die Spielregeln

- Spielerdaten und beide Inventare in [`main`](https://github.com/DescentDevelopers/Descent3/blob/de4bebf5f2d2523261956d0f28603e426eb88c63/Descent3/gamesave.cpp#L962-L975) und [`d3-historical`](https://github.com/DescentDevelopers/Descent3/blob/84e25b0b411284bad57b34f50fd976f1a8a2367f/Descent3/gamesave.cpp#L1014-L1030).
- [Aufsammeldefinitionen und Mengenbegrenzungen](https://github.com/DescentDevelopers/Descent3/blob/de4bebf5f2d2523261956d0f28603e426eb88c63/Descent3/multisafe.cpp#L2800-L2981).
- [TelCom-Auswahl](https://github.com/DescentDevelopers/Descent3/blob/de4bebf5f2d2523261956d0f28603e426eb88c63/Descent3/TelCom.cpp#L3603-L3633), [zugelassene Standardnamen](https://github.com/DescentDevelopers/Descent3/blob/de4bebf5f2d2523261956d0f28603e426eb88c63/Descent3/ship.cpp#L124-L129) und [Mercenary-Skript](https://github.com/DescentDevelopers/Descent3/blob/de4bebf5f2d2523261956d0f28603e426eb88c63/scripts/Merc02.cpp#L5095-L5104).

## Builds auf GitHub

Beide Workflows starten bei einem Push auf `dev`. Unter **Actions** lassen sie
sich auch manuell über **Run workflow** starten.

| Workflow | Prüfungen und Build | ZIP unter **Artifacts** |
| --- | --- | --- |
| Windows 98 | MinGW-w64-Build auf Ubuntu, native Datentests mit ASan/UBSan und Prüfung der Win98-Importe | `Descent3_Trainer-Win98` mit `D3EDIT98.EXE` |
| Windows 10 and 11 | Datentests und WinForms-Oberflächentest mit Mono auf Ubuntu, Build mit dem .NET-Framework-Compiler unter Windows | `Descent3_Trainer-Win10-Win11` mit `D3SaveEditor.exe` und `D3SaveEditor.exe.config` |

Nach einem erfolgreichen Lauf findest du das Programm-ZIP auf der Seite des
Laufs. Es enthält die Programmdateien, eine Kurzanleitung und die Lizenzhinweise.
`SOURCE.txt` verlinkt ein separates GitHub-Archiv mit dem vollständigen
Quelltext und den Buildskripten des gebauten Commits. `SHA256SUMS.txt` enthält
die Prüfsummen der Programmdateien.

## Kompilieren

Alle Builds erfolgen auf einem modernen Entwicklungsrechner.

**Windows 10/11:** Im Ordner `windows10` unter Windows `build.cmd` ausführen.
Das Skript verwendet den C#-Compiler des .NET Frameworks. Keine NuGet-Pakete nötig.
`D3SaveEditor.exe.config` gehört neben die erzeugte EXE.

**Windows 98:** Im Ordner `windows98` mit einem i686-MinGW-w64-Compiler
`build.cmd` unter Windows oder `sh build.sh` unter Linux ausführen.
Danach die Importprüfung starten:

```sh
cd windows98
sh build.sh
python3 tests/pe_audit.py D3EDIT98.EXE
```

Nur die erzeugte native EXE muss auf den Win98-Zielrechner kopiert werden.
Die besonderen Buildoptionen für i386, ANSI-APIs, PE-Version 4.0 und den
CRT-freien Einstieg bleiben erforderlich.

## Tests

Im jeweiligen Variantenordner unter Linux `sh tests/run-tests.sh` ausführen.
Für die moderne Variante werden C++17, Mono und `mcs` benötigt. Die native
Testsuite verwendet C++17, einen C-Compiler, ASan/UBSan und Python 3; ihr letzter
Schritt prüft die **zuvor gebaute** `D3EDIT98.EXE`.
Details und die historischen Prüfergebnisse stehen in `windows10/TESTS.md`
und `windows98/TESTS.md`.

`SOURCE-PROVENANCE.json` dokumentiert die Dateihashes und Herkunft des
ursprünglich importierten Quelltextpakets.

## Lizenz und Herkunft

**GPL-3.0-or-later**, siehe [`LICENSE`](LICENSE).
Die bereits vorhandenen SPDX-Hinweise und Copyrightangaben bleiben erhalten.
Formatgrundlage und übernommene C++-Deklarationen:
Copyright (C) 2024 Parallax Software, aus
[DescentDevelopers/Descent3](https://github.com/DescentDevelopers/Descent3).

Im früheren Win98-Paket enthielt `LICENSE` einen MIT-Text, obwohl README und
Quelltexte GPL-3.0-or-later ausweisen. Dieses Paket enthält den dazu passenden
GPL-v3-Lizenztext. Die frühere MIT-Datei bleibt zur Herkunftsdokumentation unter
`docs/origin` erhalten; daraus wird keine zusätzliche MIT-Lizenz für den
gesamten Editor abgeleitet. Einzelheiten in [`NOTICE.md`](NOTICE.md).

Bei GitHub-Veröffentlichung die Lizenztexte und vorhandenen Copyright- und
Herkunftshinweise beibehalten. Werden auch EXE-Dateien veröffentlicht, den dazu
gehörenden vollständigen Quelltext einschließlich Buildskripten zugänglich machen.
Die Erlaubnis zur Veröffentlichung folgt aus den jeweiligen Lizenzbedingungen;
die OpenAI-Ausgaberechte heben Rechte Dritter nicht auf.

## Mitwirkung

Projektidee und Anforderungen: ixylon88. Die Editor-Implementierungen und ihre
Dokumentation wurden im Dialog mit ChatGPT (OpenAI) erstellt. Die Kennzeichnung
beschreibt die Mitwirkung des KI-Werkzeugs; sie ist keine Behauptung eigener
Urheberrechte von ChatGPT oder einer Prüfung/Freigabe durch OpenAI.
Die Beiträge aus Descent 3 werden durch die vorhandenen Herkunftshinweise gewürdigt.

Eigenständiges Community-Projekt. Keine offizielle Veröffentlichung der
Descent-Entwickler oder von OpenAI.

Der technische Fortsetzungsstand steht in [`PROJEKTSTAND-Descent3.md`](PROJEKTSTAND-Descent3.md).
