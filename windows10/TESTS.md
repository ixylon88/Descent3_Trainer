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

## Ausführen

Das Testprojekt verwendet MSTest. Der Paket-Restore lädt die Testabhängigkeiten
aus NuGet.

In Visual Studio `windows10/D3SaveEditor.sln` öffnen und im **Test-Explorer**
**Alle ausführen** wählen. Auf der Kommandozeile unter Windows im Repo-Hauptordner:

```powershell
.\test-windows10.cmd
```

Das Projekt `D3SaveEditor.Tests` enthält die elf von `make_fixtures.cpp` erzeugten
synthetischen Testdateien in `tests/fixtures.zip`. Beide Tests entpacken ihre Daten
beim Start in eigene temporäre Ordner. Unter Linux erzeugt `sh test-windows10.sh`
die Testdateien mit einem C++17-Compiler neu und führt die Datentests mit Mono aus.
Die Mono-Entwicklungswerkzeuge `resgen` und `al` bauen die Textressourcen für
beide Sprachen mit.

Wenn sich der Generator ändert, lässt sich das Archiv im Repo-Hauptordner mit
den folgenden Befehlen neu erzeugen.

```sh
sh test-windows10.sh
python3 -m zipfile -c windows10/tests/fixtures.zip windows10/test-output/fixture*
```
