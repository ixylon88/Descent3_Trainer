# Descent 3 Savegame Editor

Quelltexte zweier grafischer Offline-Editoren für Descent-3-Spielstände.
Mit Hilfe von ChatGPT (OpenAI) entwickelt.

**Version 1.0.0.** Unterstützte Spielstände und Testumfang stehen in den Anleitungen der jeweiligen Variante.

## Varianten

| Ordner | Zielsystem und Technik | Grundlage des Dateiformats |
| --- | --- | --- |
| [`windows10`](windows10/README.md) | Windows 10/11, C#, .NET Framework 4.8, WPF | Descent3 `main`, Revision `76049bb32d0a391ab35aedcd0716073c1e64d72a`; bekannte Spielerlayouts 496/536 Byte |
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
- Pyro-GL, Phoenix, Magnum-AHT und den Black Pyro freischalten, sofern sie im
  Spielstand definiert sind. Automatische Wahl des Black Pyro beim Briefing ermöglichen.
- Gespeicherte Kartensichtbarkeit vollständig aufdecken.
- Normale Munitionslimits aus eigenen Spieldaten lesen; alternativ 65.535 als technisches Maximum setzen.
- Geplante Ausgabe prüfen und beim Überschreiben Backups anlegen.
- Deutsch und Englisch über das Sprachmenü umschalten. **Hilfe** enthält die
  Bedienhinweise als eigenen Reiter.

Die genauen Voraussetzungen, Grenzen und Bedienhinweise stehen in den jeweiligen
README-Dateien. Proprietäre Spieldaten und echte Spielstände sind nicht enthalten.

## Inventar, Gegenmaßnahmen und Schiffsfreigaben

Die Liste **Gespeicherte Einträge** im Reiter **Inventar / Schiffe** zeigt die
Einträge des Spielerinventars. Im Reiter **Gegenmaßnahmen** lassen sich der
Bestand und die aktive Gegenmaßnahme einstellen. Die Menge `0` entfernt einen
bearbeitbaren Eintrag. **Vorhandene: Spiellimit** füllt Chaff, Bouncing Betty,
Seeker Mine und Proximity Mine bis 12 Stück auf. Beim Gunboy sind es 4.
Beide Editoren zeigen für die 20 Standardwaffen und fünf Gegenmaßnahmen
dieselben deutschen und englischen Namen an. Die Namen orientieren sich an
der [Descent-3-Zeichenkettendatei](https://github.com/DescentDevelopers/Descent3/blob/de4bebf5f2d2523261956d0f28603e426eb88c63/scripts/data/fullhog/D3.STR).
Die Originalbeschreibungen, Symbole und Flags vorhandener Einträge bleiben bei
Mengenänderungen erhalten. Neu angelegte Einträge erhalten einen Textnamen und
ein leeres Symbolfeld.

Force Wall und Go Wingnut stammen aus den allgemeinen Spielskripten. Ob sie
als Spieler-Gegenstände verfügbar sind und welche Wirkung sie haben, hängt
von der Spielversion und den Spieldaten ab. Im Spielerinventar werden auch
Missionsgegenstände und vorhandene Guide-Bot-Einträge angezeigt.

Die Häkchen ergänzen die Schiffserlaubnisse im Spielstand. Die reguläre
Schiffsauswahl im Einzelspieler-Briefing bietet Pyro-GL, Phoenix und Magnum-AHT
an. In Mercenary schaltet ein Skript am Ende von Level 2 den Black Pyro frei
und sperrt den Phoenix.

**Nur Black Pyro erlauben** wählt alle anderen Schiffe ab. Das Spiel übernimmt
den Black Pyro beim nächsten passenden Briefing automatisch, weil er das einzige
erlaubte Schiff ist. In der Hauptkampagne wurde das beim Wechsel von Level 1 zu
Level 2 im Spiel bestätigt. Der Knopf ist auch in Mercenary verfügbar, wenn der
Spielstand eine Schiffsdefinition für den Black Pyro enthält. Das Spiel kann
die Schiffsfreigaben beim Fortschritt auch ins Pilotenprofil übernehmen.

Quelltextstellen für das Format und die Spielregeln

- Spielerdaten und beide Inventare in [`main`](https://github.com/DescentDevelopers/Descent3/blob/de4bebf5f2d2523261956d0f28603e426eb88c63/Descent3/gamesave.cpp#L962-L975) und [`d3-historical`](https://github.com/DescentDevelopers/Descent3/blob/84e25b0b411284bad57b34f50fd976f1a8a2367f/Descent3/gamesave.cpp#L1014-L1030).
- [Aufsammeldefinitionen und Mengenbegrenzungen](https://github.com/DescentDevelopers/Descent3/blob/de4bebf5f2d2523261956d0f28603e426eb88c63/Descent3/multisafe.cpp#L2800-L2981).
- [Zugelassene Schiffsnamen im Briefing](https://github.com/DescentDevelopers/Descent3/blob/de4bebf5f2d2523261956d0f28603e426eb88c63/Descent3/ship.cpp#L124-L129), [automatische Auswahl](https://github.com/DescentDevelopers/Descent3/blob/de4bebf5f2d2523261956d0f28603e426eb88c63/Descent3/TelCom.cpp#L3425-L3456) und [Mercenary-Skript](https://github.com/DescentDevelopers/Descent3/blob/de4bebf5f2d2523261956d0f28603e426eb88c63/scripts/Merc02.cpp#L5095-L5104).

## Guide-Bot

Der Reiter **Guide-Bot** bearbeitet den Namen im gespeicherten Level, die
regelmäßigen Funkmeldungen sowie Feuerlöscher, Go Wingnut, Fernsteuerung,
Begleitschutz und Antivirus. Der Name darf bis zu sieben ANSI-Zeichen und jeder
Vorrat 0 bis 127 Stück umfassen. Der gespeicherte Schildwert dient zur Anzeige.
Die Felder sind verfügbar, wenn das Objekt und sein OSIRIS-Skriptblock eindeutig
dem Spieler zugeordnet werden können und das gespeicherte Layout bekannt ist.
Die Tests verwenden synthetische Spielstände für beide ABI-Varianten.

- [Guide-Bot-Skriptzustand](https://github.com/DescentDevelopers/Descent3/blob/de4bebf5f2d2523261956d0f28603e426eb88c63/scripts/AIGame.cpp#L1476-L1536) und [Serialisierung der OSIRIS-Skriptblöcke](https://github.com/DescentDevelopers/Descent3/blob/de4bebf5f2d2523261956d0f28603e426eb88c63/Descent3/OsirisLoadandBind.cpp#L2993-L3015).

## Builds auf GitHub

Beide Workflows starten bei einem Push auf `dev`. Unter **Actions** lassen sie
sich auch manuell über **Run workflow** starten.

| Workflow | Prüfungen und Build | ZIP unter **Artifacts** |
| --- | --- | --- |
| Windows 98 | MinGW-w64-Build auf Ubuntu, native Datentests mit ASan/UBSan und Prüfung der Win98-Importe | `Descent3_Trainer-Win98` mit `D3EDIT98.EXE` |
| Windows 10 and 11 | Datentests mit Mono auf Ubuntu, WPF-Build und Oberflächentest unter Windows | `Descent3_Trainer-Win10-Win11` mit Programmdateien und dem Ordner `de` für die deutsche Übersetzung |

Nach einem erfolgreichen Lauf findest du das Programm-ZIP auf der Seite des
Laufs. Es enthält die Programmdateien, eine Kurzanleitung und die Lizenzhinweise.
`SOURCE.txt` verlinkt ein separates GitHub-Archiv mit dem vollständigen
Quelltext und den Buildskripten des gebauten Commits. `SHA256SUMS.txt` enthält
die Prüfsummen der Programmdateien.

## Bauen und testen

Die Skripte liegen im Repo-Hauptordner und werden auch von GitHub Actions verwendet.

| Ziel | Umgebung | Bauen | Testen |
| --- | --- | --- | --- |
| Windows 10/11 | Windows mit .NET SDK und .NET Framework 4.8-Zielpaket | `build-windows10.cmd` | `test-windows10.cmd` |
| Windows 10/11 | Linux mit C++17 und Mono-Entwicklungswerkzeugen | | `sh test-windows10.sh` für die Datentests |
| Windows 98 | Linux mit i686-MinGW-w64, GCC/G++ und Python 3 | `sh build-windows98.sh` | `sh test-windows98.sh` nach dem Build |
| Windows 98 | Windows mit i686-MinGW-w64 | `build-windows98.cmd` | |

In Visual Studio die Solution [`windows10/D3SaveEditor.sln`](windows10/D3SaveEditor.sln)
öffnen. **F5** startet `D3SaveEditor.Gui`. Im **Test-Explorer** führt **Alle ausführen**
die Datentests und den WPF-Oberflächentest aus `D3SaveEditor.Tests` aus.

Die Windows-98-Tests laufen auf dem Linux-Entwicklungsrechner mit ASan/UBSan
und prüfen die Importe der gebauten `windows98/D3EDIT98.EXE`. Auf den
Windows-98-Rechner wird die erzeugte EXE kopiert. Weitere Details stehen in
[`windows10/TESTS.md`](windows10/TESTS.md) und [`windows98/TESTS.md`](windows98/TESTS.md).

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
