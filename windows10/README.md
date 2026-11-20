# Descent 3 Savegame Editor · Windows 10 / 11

**Vorschauversion 1.0.** Der Editor verwendet WPF und .NET Framework 4.8. Er läuft ohne Netzwerkzugriff und ohne Administratorrechte. Zum Speichern werden Schreibrechte im Zielordner benötigt.

## Start und Verwendung

1. Das Programm-ZIP vollständig entpacken und **D3SaveEditor.exe** starten. `D3SaveEditor.Core.dll` und `D3SaveEditor.exe.config` gehören in denselben Ordner. Für einen Start aus dem Quelltext siehe [Quelltext und Neubau](#quelltext-und-neubau).
2. In Descent 3 zunächst einen Spielstand anlegen und das Spiel beenden.
3. Im Editor **Spielstand öffnen…** anklicken. Descent 3 nennt die eigentlichen Speicherdateien üblicherweise `saveg000` bis `saveg007`, ohne Dateiendung, im Unterordner `savegame` des beschreibbaren Spielverzeichnisses. Eine Datei lässt sich auch in das Fenster ziehen oder als Startargument übergeben.
4. Gewünschte Werte und Häkchen einstellen. **Datei / Änderungen → Änderungen prüfen** zeigt die geplanten Änderungen.
5. **Speichern + Backup** ersetzt den geöffneten Spielstand und legt daneben eine datierte Sicherung an. Anschließend den Spielstand im Spiel neu laden.

**Kopie speichern…** legt stattdessen eine weitere Datei an. Der vorgeschlagene Name endet auf `.edited`. Damit Descent 3 diese Kopie als Slot anzeigt, muss sie im passenden Spielstandsordner unter einem Namen wie `saveg001` liegen. Vorhandene Dateien werden auch bei diesem Speichervorgang gesichert. `.cnt`-Dateien werden nicht verändert.

## Funktionen

| Bereich | Möglichkeiten |
|---|---|
| Primärwaffen | Alle zehn Waffen einzeln hinzufügen/entfernen; aktive Waffe wählen; Munition für Vauss, Mass Driver und Napalm bearbeiten |
| Sekundärwaffen | Alle zehn Waffen einzeln hinzufügen/entfernen; aktive Waffe und Stückzahlen von 0 bis 65.535 setzen |
| Versorgung | Schild und Energie setzen oder auf 200 maximieren; Nachbrennerladung setzen oder auf 5 Sekunden auffüllen |
| Effekte | God-Mode und Cloaked-Mode aktivieren/deaktivieren; lange Restdauer erneuern |
| Ausrüstung | Nachbrenner-Kühler, Energie-Schild-Konverter und Quad-Laser zuweisen/entfernen |
| Gegenmaßnahmen | Bestand und aktive Gegenmaßnahme einstellen, vorhandene Gegenmaßnahmen bis zum Spiellimit auffüllen |
| Inventar / Schiffe | Spielerinventar ansehen, unterstützte Gegenstände bearbeiten, Schiffe freischalten und den Black Pyro für ein Briefing vormerken |
| Guide-Bot | Name, Funkmeldungen und Vorräte bei eindeutig erkanntem Skriptzustand bearbeiten |
| Karte | Gespeicherte Kartensichtbarkeit vollständig aufdecken; ohne Häkchen bleibt die gespeicherte Erkundung erhalten |
| Weitere Werte | Leben und Punktestand bearbeiten |
| Dateien | Änderungen prüfen, Kopie speichern, automatische Backups und Erkennung nachträglich veränderter Originaldateien |

Es muss mindestens eine bereits vorhandene Primärwaffe übrig bleiben. Wird eine gerade aktive Waffe entfernt, setzt der Editor bei Bedarf eine noch vorhandene Waffe als aktiv. Abgewählte Waffen erhalten in der Oberfläche Munition 0. Flare und unbekannte zusätzliche Besitzbits bleiben erhalten.

Die Bedienung des Reiters **Inventar / Schiffe** steht in der
[gemeinsamen Anleitung](../README.md#inventar-und-schiffsfreigaben).

## Was „maximieren“ bedeutet

**Schild/Energie:** Die normalen Quelltext-Maxima betragen jeweils 200. Höhere Werte können bei Bedarf manuell eingegeben werden.

**Munition:** Der Spielstand enthält die aktuellen Mengen, aber nicht die normalen Kapazitäten des Schiffs. Diese stehen in den proprietären Spieldaten. Deshalb sind zwei Varianten vorhanden:

- **Vorhandene: Schiffslimit** verwendet die tatsächlichen Schiffsgrenzen. Dafür **Schiffslimits laden…** wählen und `Table.gam`, `d3.hog` oder ein passendes HOG2-Missionsarchiv aus deiner Installation öffnen. Die Werte werden anhand des gespeicherten Schiffsnamens zugeordnet. Bei Mods müssen deren passende Definitionen geladen werden; später geladene Definitionen gleichen Namens überschreiben frühere.
- **Vorhandene: 65.535** setzt bewusst das technische Maximum eines vorzeichenlosen 16-Bit-Munitionszählers. Das sind **nicht** die normalen Spielgrenzen. Das Spiel kann die Menge beispielsweise bei einem Schiffswechsel wieder auf seine normalen Grenzen reduzieren.

Die einzelnen **Max**-Knöpfe verwenden geladene Schiffslimits; wenn für das Schiff keine Definition geladen ist, setzen sie 65.535. Die Oberfläche zeigt die verwendete Variante an. Maximiert wird nur Munition der bereits angehakten Waffen.

## God-Mode und Tarnung

Descent 3 speichert dafür ein Flag und eine Restdauer. Der Editor setzt beim Aktivieren **1.000.000.000 Sekunden**. Es ist keine Veränderung der Spiel-EXE und kein laufender Trainer. Levelwechsel, Missionen, Skripte oder das Spiel selbst können solche Effekte wieder aufheben.

Bereits gespeicherte endliche Power-up-Effekte bleiben beim bloßen Öffnen/Speichern einschließlich ihrer Restdauer erhalten. Zum Verlängern eines schon aktiven Effekts **Verlängern** anklicken. Die Tarnung wird im Effektblock des Spielerobjekts bearbeitet; God-Mode im Spielerblock. Quad-Laser werden sowohl im Inventar als auch an den beiden Laser-Waffenbatterien gesetzt.

## Backups wiederherstellen

Beim Überschreiben entsteht beispielsweise `saveg000.bak-20261024-203000-123-a1b2c3`. Zum Wiederherstellen das Spiel beenden und den gewünschten Backup-Inhalt wieder unter den ursprünglichen Slotnamen `saveg000` kopieren. Der Editor überschreibt Sicherungen nicht.

## Kompatibilität und Prüfstand

Entwickelt anhand des von dir genannten [Descent-3-Quelltexts](https://github.com/DescentDevelopers/Descent3), festgehalten auf Commit `76049bb32d0a391ab35aedcd0716073c1e64d72a` vom 23.10.2026.

Unterstützt werden das dort beschriebene Little-Endian-Speicherformat **0, 1 und 2** und die bekannten Windows-Spielerstrukturen mit **496 Byte (32-Bit)** oder **536 Byte (64-Bit)**. Das ist keine Konvertierung zwischen Spielversionen oder Prozessorarchitekturen: Der bearbeitete Spielstand behält sein ursprüngliches Layout. Andere Builds und nicht eindeutig erkennbare Strukturen werden abgelehnt.

Der ursprüngliche Editor wurde mit echten Spielständen erprobt. Die automatisierten Daten- und WPF-Tests verwenden synthetische Dateien. Details stehen in [`TESTS.md`](TESTS.md).

Der Quelltext beschreibt das Speicherformat, ersetzt aber keinen vollständigen Levelzustand mit Mission, Objekten und Skripten. Deshalb erzeugt dieser Editor keinen komplett neuen, spielbaren Spielstand aus dem Nichts. Du benötigst zum Verwenden später eine im Spiel gespeicherte Datei.

## Quelltext und Neubau

In Visual Studio 2022 oder neuer [`D3SaveEditor.sln`](D3SaveEditor.sln) öffnen
und das Projekt **D3SaveEditor.Gui** mit **F5** starten. Im Visual Studio Installer
werden die Workload **.NET-Desktopentwicklung** und das **.NET Framework 4.8-Zielpaket** benötigt.

| Projekt | Aufgaben |
| --- | --- |
| `D3SaveEditor.Gui` | WPF-Fenster in `MainView.xaml`, Bedienlogik in `MainView.xaml.cs`, Dialoge und Programmstart |
| `D3SaveEditor.Core` | Spielstandformat, Änderungen, Dateizugriff, Backups, Schiffslimits aus den Spieldaten |
| `D3SaveEditor.Tests` | Datentests und WPF-Oberflächentest im Visual-Studio-Test-Explorer |

`build-windows10.cmd` im Repo-Hauptordner baut unter Windows mit dem .NET SDK die
Release-Konfiguration des Editors. `test-windows10.cmd` baut und startet das Testprojekt.
Die Programmdateien liegen in `D3SaveEditor.Gui/bin/Debug/net48` oder
`D3SaveEditor.Gui/bin/Release/net48`. Das Testprojekt verwendet MSTest über NuGet.

Lizenz: **GPL-3.0-or-later**, siehe `LICENSE`. Es werden keine proprietären Spielinhalte mitgeliefert. Die Formatgrundlage und die übernommenen C++-Deklarationen stammen aus Descent 3, Copyright (C) 2024 Parallax Software. Dieses Projekt ist ein eigenständiger Editor und keine offizielle Veröffentlichung der Descent-Entwickler.
