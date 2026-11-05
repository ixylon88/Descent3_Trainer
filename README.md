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
- Gespeicherte Kartensichtbarkeit vollständig aufdecken.
- Normale Munitionslimits aus eigenen Spieldaten lesen; alternativ 65.535 als technisches Maximum setzen.
- Geplante Ausgabe prüfen und beim Überschreiben Backups anlegen.

Die genauen Voraussetzungen, Grenzen und Bedienhinweise stehen in den jeweiligen
README-Dateien. Proprietäre Spieldaten und echte Spielstände sind nicht enthalten.

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

Die C/C#-Quellen, Tests, Buildskripte und Laufzeitkonfiguration wurden beim
Erstellen dieses Pakets nicht verändert. SHA-256 und Herkunft jedes dieser
Dateien sind in `SOURCE-PROVENANCE.json` festgehalten. Eine Vollständigkeits-
und Byteprüfung ist keine erneute Ausführung der früheren Anwendungstests.

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
