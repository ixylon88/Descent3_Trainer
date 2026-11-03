# Descent 3 Savegame Editor für Windows 98

**Quelltextbasierte Vorschauversion 1.0, Grundlage `d3-historical`.**

Nach dem Neubau ist `D3EDIT98.EXE` eine eigenständige, native 32-Bit-Win32-Anwendung mit deutschen Beschriftungen. Sie braucht **kein .NET, MFC, KernelEx oder zusätzliches Visual-C++-Laufzeitpaket**. Es werden ausschließlich klassische Windows-Systembibliotheken importiert. Die Oberfläche ist für **800 × 600 oder größer** ausgelegt.

Die EXE wurde gezielt für den klassischen Windows-Loader gebaut: PE32/x86, Betriebssystem- und Subsystemversion 4.0, ANSI-APIs, eigener Programmeinstieg ohne CRT, keine Unicode-Schicht, kein .NET-Header und keine neueren Initialisierungsroutinen. **Ein tatsächlicher Start unter Windows 98 und das Laden eines bearbeiteten Spielstands im Spiel sind noch nicht getestet.** Eine statische Prüfung ist kein Ersatz für diesen Praxistest.

## Verwenden

1. Auf einem modernen Entwicklungsrechner mit `build.cmd` oder `sh build.sh` kompilieren und die erzeugte `D3EDIT98.EXE` auf den Windows-98-Rechner kopieren. Für das Starten genügt diese EXE; `LIESMICH.TXT` enthält die Kurzanleitung für den alten Editor.
2. In Descent 3 einen Spielstand anlegen und das Spiel beenden.
3. **Oeffnen…** wählen und eine Datei wie `saveg000` bis `saveg007` im Ordner `savegame` öffnen. Diese Dateien haben normalerweise keine Endung.
4. Werte und Häkchen einstellen. **Datei / Pruefung → Aenderungen pruefen** zeigt die geplante Ausgabe. Dabei wird sie erneut eingelesen und geprüft.
5. **Speichern + Backup** wählen und anschließend den Spielstand im Spiel neu laden.

**Kopie speichern…** erzeugt stattdessen eine weitere Datei, vorgeschlagen mit der Endung `.edited`. Damit Descent 3 sie als Slot anzeigt, muss sie im Spielstandsordner unter einem passenden Slotnamen wie `saveg001` liegen. Die `.cnt`-Begleitdateien werden nicht geändert.

Windows 98 verwendet hier ANSI-Dateipfade. Verwendet werden maximal 200 Zeichen für den vollständigen Zielpfad, damit Platz für Backupnamen bleibt. Unterstützt werden Spielstände bis 32 MiB. Ein kurzer Pfad wie `C:\D3\savegame\saveg000` vermeidet Probleme mit langen Pfaden.

## Funktionen

| Bereich | Bearbeitungsmöglichkeiten |
|---|---|
| Primärwaffen | Alle zehn Waffen an-/abwählen, aktive Waffe bestimmen; gespeicherte Munitionszähler für Vauss, Mass Driver und Napalm setzen |
| Sekundärwaffen | Alle zehn Waffen an-/abwählen, aktive Waffe und Stückzahlen von 0 bis 65.535 setzen |
| Versorgung | Schild und Energie setzen oder jeweils auf 200 maximieren; Nachbrenner auf 5 Sekunden auffüllen |
| Effekte | God-Mode und Cloaked-Mode aktivieren/deaktivieren und Restdauer erneuern |
| Ausrüstung | Nachbrenner-Kühler, Energie-Schild-Konverter und Quad-Laser hinzufügen/entfernen |
| Karte | Gespeicherte Kartensichtbarkeit vollständig aufdecken |
| Weitere Werte | Leben und Punktestand ändern |
| Dateiverwaltung | Ausgabe prüfen, Kopie schreiben, Original sichern, nachträglich geändertes Original erkennen |

Eine vorhandene letzte Primärwaffe kann nicht entfernt werden. **Keine Sekundaerwaffe** entfernt sämtliche Sekundärwaffen samt Munition. Abgewählte Waffen erhalten in der Oberfläche Munition 0. Aktive Waffen werden bei Bedarf auf vorhandene Waffen zurückgesetzt. Flare, zusätzliche Besitzbits und die sonstigen Daten bleiben erhalten.

## Maximale Munition

Die normalen Kapazitäten stehen in den Spieldaten, nicht im Spielstand und nicht als vollständige Liste im Quelltext. **Schiffslimits laden…** liest sie aus `Table.gam`, `d3.hog` oder einem HOG2-Missionsarchiv anhand des gespeicherten Schiffsnamens. Bei Mods die passenden Moddaten verwenden; später geladene Definitionen ersetzen frühere für das gewählte Schiff.

- **Vorhandene: Schiff-Max** füllt nur bereits vorhandene Waffen bis zu den geladenen normalen Schiffsgrenzen auf.
- **Vorhandene: 65.535** setzt ausdrücklich das technische Speichermaximum. Dies sind nicht die normalen Spielgrenzen. Das Spiel kann solche Mengen bei einem Schiffswechsel wieder begrenzen.
- Die einzelnen **Max**-Knöpfe verwenden geladene Schiffsgrenzen, sonst 65.535. Die verwendete Variante steht unter der Waffenliste.

Es werden die direkt gespeicherten Munitionszähler bearbeitet. Neue Ausrüstungsgegenstände erhalten generische Namen; ihre Spielsymbole und lokalisierten Beschreibungen werden nicht aus proprietären Spieldaten rekonstruiert.

## God-Mode, Tarnung und Ausrüstung

Unverwundbarkeit braucht ein Flag und eine Restdauer im Spielerblock. Tarnung liegt am Spielerobjekt. Beim Aktivieren setzt der Editor **1.000.000.000 Sekunden** Restdauer. **Erneuern** setzt diese Dauer auch bei einem schon aktiven Effekt. Bereits gespeicherte endliche Effekte bleiben beim unveränderten Speichern erhalten.

Quad-Laser werden im Inventar und in beiden Laser-Waffenbatterien gesetzt. Der Nachbrenner-Kühler ist der Inventargegenstand `Afterburner`, der Konverter `Converter`. Gegenstände lassen sich nur zuweisen, wenn ihre Definition in der Übersetzungstabelle des Spielstands vorhanden ist.

Dies ist ein Dateieditor, kein laufender Trainer. Levelwechsel, Tod, Skripte und Spielregeln können Effekte oder Ausrüstung zurücksetzen. Der Editor verändert die Spiel-EXE nicht.

## Backups und Wiederherstellung

Vor dem Überschreiben entsteht beispielsweise `saveg000.bak-20261024-210000-0`. Der Editor schreibt und flusht zunächst eine geprüfte neue Datei im Zielordner. Anschließend benennt er das Original als Backup um und setzt die neue Datei an dessen Platz. Scheitert der letzte Schritt, versucht er das Original zurückzusetzen. Scheitert auch das, nennt die Fehlermeldung den Pfad des erhaltenen Original-Backups.

**Dieser Austausch ist auf FAT nicht transaktional.** Bei einem Stromausfall zwischen den Umbenennungen kann der Slotname vorübergehend fehlen. In diesem Fall das erhaltene Backup wieder unter den ursprünglichen Namen wie `saveg000` kopieren. Eine vorhandene Sicherungsdatei wird nicht überschrieben.

## Kompatibilität und Prüfstand

Grundlage ist der gewünschte [Branch `d3-historical`](https://github.com/DescentDevelopers/Descent3/tree/d3-historical), festgehalten auf Commit `84e25b0b411284bad57b34f50fd976f1a8a2367f` vom 16.04.2024.

Unterstützt werden das dortige Little-Endian-Speicherformat 0–2 und der **historische 32-Bit-Spielerblock mit 496 Byte**. Moderne 64-Bit-Spielerblöcke werden absichtlich abgelehnt. Das Speicherlayout wird nicht in andere Spielversionen konvertiert.

**299 Daten- und Dateiprüfungen sind bestanden**, darunter ein bytegenauer Vergleich mit der getrennten C#-Implementierung, SHA-256-Testvektoren, sechs historische Testvarianten und künstlich ausgelöste Fehler beim Austausch und Zurücksetzen. Adress- und Undefined-Behavior-Sanitizer waren aktiv. Die fertige EXE wurde zusätzlich auf PE-Header und sämtliche 49 importierten Funktionen geprüft. Details in `TESTS.md`.

Es lagen **keine echten Spielstände und keine proprietären Spieldaten** vor. Die Testdateien wurden aus den historischen C++-Deklarationen erzeugt; sie sind nicht spielbar und werden nicht als Spielstände ausgeliefert. Die native Oberfläche und das Laden im Spiel benötigen weiterhin einen echten Windows-98-Praxistest.

## Weiterarbeiten und Neubau

`../PROJEKTSTAND-Descent3.md` dokumentiert beide bisherigen Versionen, Quellen, Layouts, Entscheidungen, Tests und offene Punkte. Die Windows-10/11-Version mit C#-Quelltext liegt im Nachbarordner `../windows10`. Dieses Quelltextpaket enthält keine fertigen EXE-Dateien; die ursprünglichen vollständigen Projektarchive bleiben separat erhalten.

Der vollständige native Quelltext liegt unter `src`, Tests unter `tests`. Zum Neubau wird auf einem **modernen Entwicklungsrechner** ein i686-MinGW-w64-C-Compiler benötigt; auf dem Windows-98-Zielrechner ist kein Compiler erforderlich.

```sh
sh build.sh
python3 tests/pe_audit.py D3EDIT98.EXE
```

Auf einem modernen Windows-Rechner mit passendem MinGW-Compiler im PATH kann `build.cmd` verwendet werden. Keine zusätzliche Runtime anbinden und den eigenen CRT-freien Programmeinstieg beibehalten.

Lizenz: **GPL-3.0-or-later**, siehe `LICENSE`. Format und übernommene C++-Deklarationen stammen aus Descent 3, Copyright (C) 2024 Parallax Software. Es werden keine proprietären Spielinhalte mitgeliefert. Dies ist kein offizieller Editor der Descent-Entwickler.
