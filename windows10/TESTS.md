# Prüfprotokoll · 24.10.2026

**Ergebnis vom 24.10.2026: 123 Datentest-Assertions und 8 Oberflächentest-Assertions bestanden.**

**Inventar und Schiffsfreigaben, 14.11.2026.** 155 Datentest-Assertions und
13 Oberflächentest-Assertions bestanden unter Linux mit Mono 6.12 und Xvfb.
Zusätzliche 32- und 64-Bit-Testdateien prüfen gleichzeitige Änderungen beider
Listen, Auswahlindizes, Mengenlimits, fehlende Aufsammeldefinitionen und
Schiffserlaubnisse bei Lücken in der Schiffstabelle.

Der WPF-Build liefert `D3SaveEditor.exe` und `D3SaveEditor.Core.dll` für AnyCPU. Die Datentests prüfen den Spielstandcode in `D3SaveEditor.Core`.

## Datenformat und Dateischreiben

Der Generator in `tests/make_fixtures.cpp` enthält die vollständigen Spieler-, Effekt- und dynamischen Waffenbatterie-Deklarationen aus dem referenzierten Descent-3-Quelltext. Ein unabhängiger C++-Compiler schreibt daraus die Testdaten. Die Ergebnisse bestätigen insbesondere diese Strukturlängen:

| Struktur | Größe |
|---|---:|
| player, 32-Bit-Pointerlayout | 496 Byte |
| player, 64-Bit-Pointerlayout | 536 Byte |
| effect_info_s | 164 Byte |
| dynamic_wb_info | 108 Byte |
| light_info | 52 Byte |
| custom_anim | 24 Byte |

Geprüft wurden sieben Dateivarianten: 32 und 64 Bit, fehlender Effektblock, gespeichertes Guided-Objekt, komprimiertes Vorschaubild sowie Speicherformat 0 und 1 zusätzlich zu Format 2.

Die Tests prüfen unter anderem:

- Byte-identische Ausgabe beim unveränderten Wiederspeichern, auch nach einer Bearbeitung.
- Beide ABI-Layouts, exakte Energie-/Schildwerte und die Waffenfelder.
- Alle 20 Waffen, Sekundärmunition 65.535 und aktive Waffenauswahl.
- God-Mode und Tarnung inklusive Restdauer; Ein- und Ausschalten beider Effekte.
- Hinzufügen/Entfernen von Kühler, Konverter und Quad-Laser; beide Quad-Waffenbatterien.
- Unveränderte zusätzliche Flags, Flare, andere Spielerfelder und den undurchsichtigen Dateianhang.
- Vollständige Kartensichtbarkeit.
- Ablehnung unbekannter Versionen/Strukturlängen, mehrdeutiger Spielerblöcke, ungültiger Querverweise, abgeschnittener Dateien und inkonsistenter Inventare.
- Ablehnung nicht endlicher Eingabewerte und des Entfernens der letzten vorhandenen Primärwaffe.
- Exakte Backups, erneutes Öffnen geschriebener Dateien, Erkennen inzwischen veränderter Originaldateien und Schutz vorhandener Dateien bei ungültigen Ausgaben.
- Schiffstabellen Version 5 und 6, Längenprüfung und Lesen einer solchen Tabelle aus einem HOG2-Archiv.

## Oberfläche

Die oben genannten Oberflächenergebnisse beziehen sich auf den früheren WinForms-Editor.
Der Windows-Job führt den WPF-Oberflächentest mit einer synthetischen Datei aus.
Der Test öffnet `MainView`, bedient die WPF-Eingabefelder und speichert über
**Speichern + Backup**. Er prüft Spielerwerte, Waffenauswahl, Inventar,
Schiffsfreigaben, Backups und byte-identische Ausgabe bei unveränderten Werten.

## Verbleibende Grenzen

Die synthetischen Testdateien werden aus den dokumentierten Strukturen erstellt.
Ein Ingame-Test mit echten Spielständen ergänzt die automatisierten Prüfungen.
ABI-Varianten außerhalb der ausdrücklich unterstützten Größen werden abgelehnt.

Die Quelltextreferenz ist Commit `76049bb32d0a391ab35aedcd0716073c1e64d72a` des [Descent-3-Repositories](https://github.com/DescentDevelopers/Descent3).

Reproduzieren der Datentests auf einem System mit C++17-Compiler und Mono:

```sh
sh tests/run-tests.sh
```

Die Testdaten liegen in `test-output`. Der WPF-Test lässt sich unter Windows
aus `windows10` mit einer erzeugten `fixture32-loadout` ausführen:

```powershell
dotnet build tests/UiTests.csproj -c Release
.\tests\bin\Release\net48\UiTests.exe .\test-output\fixture32-loadout
```
