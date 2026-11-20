# Descent 3 Savegame Editor · Windows 10 / 11

**Quelltextbasierte Vorschauversion 1.0.** Enthalten sind die fertige Windows-Anwendung und ihr vollständiger Quelltext. Es wird keine Installation benötigt. Die EXE verwendet das .NET Framework 4.x und Windows Forms. Sie läuft ohne Netzwerkzugriff und ohne Administratorrechte; im Zielordner werden Schreibrechte benötigt.

## Start und Verwendung

1. ZIP vollständig entpacken und **D3SaveEditor.exe** doppelt anklicken. Die Datei `D3SaveEditor.exe.config` daneben lassen.
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
| Karte | Gespeicherte Kartensichtbarkeit vollständig aufdecken; ohne Häkchen bleibt die gespeicherte Erkundung erhalten |
| Weitere Werte | Leben und Punktestand bearbeiten |
| Dateien | Änderungen prüfen, Kopie speichern, automatische Backups und Erkennung nachträglich veränderter Originaldateien |

Es muss mindestens eine bereits vorhandene Primärwaffe übrig bleiben. Wird eine gerade aktive Waffe entfernt, setzt der Editor bei Bedarf eine noch vorhandene Waffe als aktiv. Abgewählte Waffen erhalten in der Oberfläche Munition 0. Flare und unbekannte zusätzliche Besitzbits bleiben erhalten.

## Was „maximieren“ bedeutet

**Schild/Energie:** Die normalen Quelltext-Maxima betragen jeweils 200. Höhere Werte können bei Bedarf manuell eingegeben werden.

**Munition:** Der Spielstand enthält die aktuellen Mengen, aber nicht die normalen Kapazitäten des Schiffs. Diese stehen in den proprietären Spieldaten. Deshalb sind zwei Varianten vorhanden:

- **Vorhandene: Schiff-Max** verwendet die tatsächlichen Schiffsgrenzen. Dafür **Schiffslimits laden…** wählen und `Table.gam`, `d3.hog` oder ein passendes HOG2-Missionsarchiv aus deiner Installation öffnen. Die Werte werden anhand des gespeicherten Schiffsnamens zugeordnet. Bei Mods müssen deren passende Definitionen geladen werden; später geladene Definitionen gleichen Namens überschreiben frühere.
- **Vorhandene: 65.535** setzt bewusst das technische Maximum eines vorzeichenlosen 16-Bit-Munitionszählers. Das sind **nicht** die normalen Spielgrenzen. Das Spiel kann die Menge beispielsweise bei einem Schiffswechsel wieder auf seine normalen Grenzen reduzieren.

Die einzelnen **Max**-Knöpfe verwenden geladene Schiffslimits; wenn für das Schiff keine Definition geladen ist, setzen sie 65.535. Die Oberfläche zeigt die verwendete Variante an. Maximiert wird nur Munition der bereits angehakten Waffen.

## God-Mode und Tarnung

Descent 3 speichert dafür ein Flag und eine Restdauer. Der Editor setzt beim Aktivieren **1.000.000.000 Sekunden**. Es ist keine Veränderung der Spiel-EXE und kein laufender Trainer. Levelwechsel, Missionen, Skripte oder das Spiel selbst können solche Effekte wieder aufheben.

Bereits gespeicherte endliche Power-up-Effekte bleiben beim bloßen Öffnen/Speichern einschließlich ihrer Restdauer erhalten. Zum Verlängern eines schon aktiven Effekts **Erneuern** anklicken. Die Tarnung wird im Effektblock des Spielerobjekts bearbeitet; God-Mode im Spielerblock. Quad-Laser werden sowohl im Inventar als auch an den beiden Laser-Waffenbatterien gesetzt.

## Backups wiederherstellen

Beim Überschreiben entsteht beispielsweise `saveg000.bak-20261024-203000-123-a1b2c3`. Zum Wiederherstellen das Spiel beenden und den gewünschten Backup-Inhalt wieder unter den ursprünglichen Slotnamen `saveg000` kopieren. Der Editor überschreibt Sicherungen nicht.

## Kompatibilität und Prüfstand

Entwickelt anhand des von dir genannten [Descent-3-Quelltexts](https://github.com/DescentDevelopers/Descent3), festgehalten auf Commit `76049bb32d0a391ab35aedcd0716073c1e64d72a` vom 23.10.2026.

Unterstützt werden das dort beschriebene Little-Endian-Speicherformat **0, 1 und 2** und die bekannten Windows-Spielerstrukturen mit **496 Byte (32-Bit)** oder **536 Byte (64-Bit)**. Das ist keine Konvertierung zwischen Spielversionen oder Prozessorarchitekturen: Der bearbeitete Spielstand behält sein ursprüngliches Layout. Andere Builds und nicht eindeutig erkennbare Strukturen werden abgelehnt.

**Es lag kein echter Spielstand vor.** Die Tests verwenden künstliche, nicht spielbare Dateien, deren Spieler-, Effekt- und Waffenbatterie-Strukturen durch einen unabhängigen C++-Generator aus den Quelltextdeklarationen geschrieben werden. Das Laden eines bearbeiteten Spielstands im echten Spiel ist **noch nicht verifiziert**. Auch ein Start auf einem echten Windows-System konnte in der Entwicklungsumgebung nicht geprüft werden. Die Oberflächenprüfung erfolgte mit Mono/X11; Details stehen in `TESTS.md`.

Der Quelltext beschreibt das Speicherformat, ersetzt aber keinen vollständigen Levelzustand mit Mission, Objekten und Skripten. Deshalb erzeugt dieser Editor keinen komplett neuen, spielbaren Spielstand aus dem Nichts. Du benötigst zum Verwenden später eine im Spiel gespeicherte Datei.

## Quelltext und Neubau

Die Dateien in `src` enthalten den vollständigen Editor. Unter Windows kann `build.cmd` den Editor mit dem C#-Compiler des .NET Frameworks neu erstellen. Es sind keine NuGet-Pakete erforderlich. Unter Linux/Mono:

```sh
mcs -target:winexe -platform:anycpu -optimize+ \
  -win32manifest:src/app.manifest -out:D3SaveEditor.exe \
  -r:System.Windows.Forms -r:System.Drawing \
  src/AssemblyInfo.cs src/SaveGame.cs src/ShipLimits.cs src/Program.cs
```

Lizenz: **GPL-3.0-or-later**, siehe `LICENSE`. Es werden keine proprietären Spielinhalte mitgeliefert. Die Formatgrundlage und die übernommenen C++-Deklarationen stammen aus Descent 3, Copyright (C) 2024 Parallax Software. Dieses Projekt ist ein eigenständiger Editor und keine offizielle Veröffentlichung der Descent-Entwickler.
