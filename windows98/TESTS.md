# Prüfprotokoll · Windows-98-Version · 24.10.2026

## Bestanden

**299 Assertions** in den nativen C-Daten-/Dateitests, ausgeführt auf dem Linux-Testhost mit AddressSanitizer und UndefinedBehaviorSanitizer. LeakSanitizer wurde deaktiviert, weil ihm in dieser isolierten Umgebung der erforderliche Zugriff auf `/proc` fehlt; es wird deshalb kein bestandener Lecktest behauptet.

Ein unabhängiger C++17-Generator enthält die originalen Deklarationen des historischen Branches und bestätigt per `static_assert` die Größen und Offsets. 32-Bit-Pointer werden auf dem 64-Bit-Testhost explizit als 32-Bit-Felder abgebildet. Bestätigt wurden `player=496`, `effect_info_s=164`, `dynamic_wb_info=108`, `light_info=52` und `custom_anim=24` Byte.

Sechs synthetische Varianten wurden geprüft: Format 2 normal, fehlender Effektblock, Guided-Objekt, komprimiertes Vorschaubild sowie Format 1 und 0.

Die Assertions umfassen:

- Unveränderte Dateien bleiben byte-identisch, auch nach erneutem Öffnen bearbeiteter Ausgaben.
- Alle zwanzig Waffen samt aktiver Auswahl, Stückzahlen, Spielerwerten, Kartenbytes und Ausrüstung.
- Unverwundbarkeit und Tarnung inklusive langer Restdauer; endliche ursprüngliche Power-up-Zeiten bleiben unverändert, wenn sie nicht bearbeitet werden.
- Inventar-Einträge und Quad-Flags beider Laserbatterien werden gemeinsam geändert.
- Zusatzbits, Flare, weitere Spielerfelder und der undurchsichtige Dateianhang bleiben erhalten.
- Unbekannte Versionen und moderne 64-Bit-Strukturen, fehlerhafte Querverweise, inkonsistente Inventare, abgeschnittene Dateien, ungültige Zahlen und das Entfernen der letzten Primärwaffe werden abgelehnt.
- SHA-256 wird gegen die bekannten Vektoren für leer, `abc` und eine Million `a` geprüft.
- Kopien verändern das Original nicht; Backups enthalten die ursprünglichen Bytes; inzwischen veränderte Originale werden erkannt.
- Per Fehler-Injektion ausgelöste Austauschfehler führen zur Wiederherstellung. Wenn auch das Zurücksetzen fehlschlägt, bleibt der Originalinhalt im gemeldeten Backup erhalten.
- Schiffstabellen, HOG2-Archivoffsets und ihre Längenprüfungen.

Für dieselbe historische Eingabedatei und dieselben Optionen wurde zusätzlich die Ausgabe des vorherigen, separat in C# implementierten Editors erzeugt. **Die Ausgaben sind Byte für Byte identisch.**

## Prüfung der EXE

`tests/pe_audit.py` prüft die echte erzeugte EXE ohne Zusatzpakete:

- PE32 / x86, GUI-Subsystem.
- Betriebssystemversion 4.0 und Subsystemversion 4.0.
- Kein CLR-, TLS- oder moderner Load-Configuration-Initialisierungsblock.
- Keine CRT-, .NET-, MFC-, UCRT- oder KernelEx-Abhängigkeit.
- Ausschließlich `KERNEL32`, `USER32`, `GDI32`, `COMDLG32` und `COMCTL32`.
- Alle 49 importierten Funktionen stehen auf der expliziten Liste klassischer APIs; keine ordinalen Importe.

Der Compiler war GCC/MinGW-w64 13.2.0, Ziel i386 mit x87, ohne SSE. Die EXE verwendet einen eigenen `WinMainCRTStartup`-Einstieg und ist ungefähr 37 KiB groß. Die Versionsangaben allein wären kein ausreichender Kompatibilitätsnachweis; deshalb werden auch Abhängigkeiten und Initialisierungsverzeichnisse geprüft.

## Offen

**Kein echter Windows-98-Start, kein Lauf der nativen Oberfläche, keine echten Spieldaten und kein Ingame-Ladetest.** Auch ein Importaudit kann die tatsächliche Laufzeitumgebung nicht vollständig nachbilden. Die zuvor unter Mono geprüfte C#-Oberfläche ist eine andere Anwendung und wird hier nicht als Test der nativen Win98-Oberfläche gezählt.

Die erzeugten Testdateien sind keine spielbaren Speicherstände. Sie werden nicht als solche angeboten.

## Reproduzieren

Auf einem Linux-Rechner mit C/C++-Compiler und Python:

```sh
sh tests/run-tests.sh
```

Das ergibt 298 Assertions ohne den optionalen C#-Vergleich. Der enthaltene `tests/Golden.cs` erzeugt den Referenzfall aus dem vorherigen C#-Quelltext. Wird dessen Ausgabepfad dem C-Testprogramm als zweites Argument übergeben, kommt die 299. Assertion hinzu. Die Fortsetzungsnotiz beschreibt die genauen Optionen und Quelltextstände.
