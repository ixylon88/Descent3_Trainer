# Prüfprotokoll · 24.10.2026

**Ergebnis: 123 Datentest-Assertions und 8 Oberflächentest-Assertions bestanden.**

Die fertige EXE wurde als Windows-GUI-Anwendung für AnyCPU kompiliert. Verwendet werden ausschließlich .NET-Framework-Bibliotheken; keine zusätzlichen Bibliotheken oder Spieldaten werden ausgeliefert.

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

Die Oberfläche wurde mit Mono 6.8 / X11 gestartet und visuell geprüft. Dabei wurden abgeschnittene Beschriftungen und Tabellenhöhen korrigiert.

Ein automatisierter Oberflächentest öffnet eine synthetische Datei, wählt Waffen, setzt Maximalmunition, Schild, Energie und Ausrüstung über die tatsächlichen Controls, betätigt **Speichern + Backup** und liest die gespeicherten Werte erneut aus. Ein zweiter, unveränderter Speichervorgang wird auf Byte-Identität geprüft. Alle acht Assertions sind bestanden.

## Verbleibende Grenzen

Es gab keinen echten Spielstand, keine proprietären Spieldaten und keinen echten Windows-Testrechner. Das Laden eines bearbeiteten Spielstands in Descent 3 und der native Windows-Start sind deshalb noch **nicht bestätigt**. Die synthetischen Testdateien sind keine spielbaren Spielstände und werden nicht als solche ausgeliefert. ABI-Varianten außerhalb der ausdrücklich unterstützten Größen werden abgelehnt.

Die Quelltextreferenz ist Commit `76049bb32d0a391ab35aedcd0716073c1e64d72a` des [Descent-3-Repositories](https://github.com/DescentDevelopers/Descent3).

Reproduzieren der Datentests auf einem System mit C++17-Compiler und Mono:

```sh
sh tests/run-tests.sh
```

Die optionale Durchführung der Oberflächentests ist am Ende dieses Skripts dokumentiert. Die Tests erzeugen ihre Daten ausschließlich im Unterordner `test-output`.
