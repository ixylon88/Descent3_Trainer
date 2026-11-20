# Projektstand: Descent 3 Savegame Editor – Windows 98 und Windows 10/11

Stand: **24.10.2026**. Diese Datei ist die Fortsetzungsnotiz für spätere Änderungen. Sie soll zusammen mit dem vollständigen Projektarchiv erhalten bleiben; zur Weiterarbeit muss das Format nicht neu recherchiert werden.

## Auftrag und bisherige Entscheidungen

ixylon88 möchte einen grafischen Editor mit einzeln an-/abwählbaren Primär- und Sekundärwaffen, aktiver Waffenauswahl, frei setzbarem und maximierbarem Schild/Energie, God-Mode, Cloaked-Mode, maximaler Sekundärmunition, Nachbrenner-Kühler und voller Karte. Zusätzlich sind Konverter, Quad-Laser, Nachbrennerladung, Leben und Punktestand enthalten.

Zuerst wurde eine Version für Windows 10 und neuer gewünscht. Danach kam ausdrücklich eine **separate Windows-98-Version** für den historischen Branch hinzu. Es lag zunächst **kein echter Spielstand** vor. Die Editoren wurden anhand des Quelltexts entwickelt. ixylon88 hat beide Versionen später mit echten Spielständen getestet und im Probebetrieb für vorläufig einsatztaublich befunden.

## Erhaltene Artefakte

| Datei | Inhalt |
|---|---|
| `D3SaveEditor-Win98.zip` | Neues vollständiges Projektpaket mit nativer EXE, C-Quelltext, Tests, Buildskripten, Anleitung, dieser Notiz und vorherigem Windows-10-Paket |
| `D3EDIT98.EXE` im neuen Paket | Native Win32-Anwendung, allein ausführbar ohne zusätzliche Laufzeitpakete |
| `Vorgaenger-Windows10.zip` im neuen Paket | Unveränderte Kopie des vorherigen Pakets `D3SaveEditor-Windows.zip`; enthält die C#-EXE, alle Quellen und Tests |
| `PROJEKTSTAND-Descent3.md` | Diese Notiz, zusätzlich separat gespeichert und innerhalb des neuen ZIPs enthalten |

SHA-256 der nativen EXE: `a8b1292e3e05469087c890a6b1e7303d7f242fcdf5d69c2db7420d1b48943dc9`.

SHA-256 des unveränderten vorherigen Windows-10-ZIPs: `4ccf9916ef4ab31644c2344c9e61c22dbb957de08d3d55190fd544ebd32ca3fa`.

Zum Wiederaufnehmen zuerst diese Notiz und das neue Archiv nach Dateinamen suchen, das Archiv entpacken und die vorhandenen Quellen verwenden. Nicht von einer dauerhaft erhaltenen temporären Entwicklungsumgebung ausgehen.

## Quellen, unveränderlich festgehalten

| Version | Repository und Branch | Untersuchte Revision |
|---|---|---|
| Windows 10/11 | [DescentDevelopers/Descent3](https://github.com/DescentDevelopers/Descent3), `main` | `76049bb32d0a391ab35aedcd0716073c1e64d72a`, 23.10.2026 |
| Windows 98 | [DescentDevelopers/Descent3, d3-historical](https://github.com/DescentDevelopers/Descent3/tree/d3-historical) | `84e25b0b411284bad57b34f50fd976f1a8a2367f`, 16.04.2024 |

Wichtige Spieldateien: `Descent3/gamesave.cpp`, `loadstate.cpp`, `gamesave.h`, `player_external_struct.h`, `player_external.h`, `Inventory.cpp/.h`, `object_external_struct.h`, `robotfirestruct.h`, `GameCheat.cpp`, `object_lighting.cpp`, `object.cpp`, `multisafe.cpp`, `weapon.cpp`, `manage/shippage.cpp`, `manage/manage.cpp` sowie HOG2-/Bitmap-Leser. Im modernen Branch sind einige Header nach `lib` verschoben; im historischen liegen sie noch unter `Descent3`.

Lizenz der Projekte: GPL-3.0-or-later. Übernommene Deklarationen und Formatgrundlage: Copyright (C) 2024 Parallax Software. Keine proprietären Spielinhalte werden mitgeliefert.

## Dateiformat: entscheidende Erkenntnisse

Savegames heißen gewöhnlich `saveg000` bis `saveg007` ohne Endung. `.cnt` enthält einen separaten Restore-Zähler und wird nicht editiert.

Der Header ist keine universelle Magic-Signatur: 32 Byte Beschreibung, anschließend Little-Endian-16-Bit-Version 0–2, Flag und optional komprimiertes OGF-Vorschaubild. Danach folgen sechs Übersetzungstabellen mit nullterminierten Namen, eine zusätzliche Bitmap-Liste, Missions-/Levelwerte, Wetter-/Türdaten und die Raumsichtbarkeit.

Die Raumdaten enthalten Portalflags, aber keine vollständigen Portalanzahlen. Ohne Missionsgeometrie kann man diesen Abschnitt nicht vollständig linear überspringen. Beide Implementierungen verwenden deshalb streng geprüfte Verknüpfungen:

1. Header und dynamische Kartenposition regulär lesen.
2. Gespeicherte Objektmarkierung `0x00BADB0B` mit Typ `OBJ_PLAYER=4` suchen und den Objekt-Datensatz vollständig nach seiner Serialisierungsfolge lesen.
3. Lokalen Spieler `id=0` und `handle & 0x7ff` mit `player.objnum` verknüpfen.
4. Auch die nachfolgenden Objekte bis zum Größenpräfix des Spielerblocks regulär lesen; fortlaufende Objektnummern prüfen.
5. Bekannte Spielergröße, Feldbereiche, beide serialisierten Inventare einschließlich Rohstruktur-Zähler und den folgenden Matcen-Zähler prüfen.
6. Mehrdeutige Treffer und unbekannte Layouts ablehnen.

Das ist keine vollständige Validierung aller Level-/Skriptdaten. Undurchsichtige Bereiche werden unverändert übernommen. Niemals daraus eine Garantie ableiten, dass jede akzeptierte Datei im Spiel funktioniert.

Der historische `player` ist **496 Byte**, das unterstützte moderne 64-Bit-Layout **536 Byte**. Die Win98-Version akzeptiert ausschließlich das historische 32-Bit-Layout. Es findet keine Konvertierung zwischen Spielversionen oder Architekturen statt.

### Relevante Spieler-Offsets, relativ zum Strukturbeginn

| Feld | 32 Bit / 496 Byte | 64 Bit / 536 Byte |
|---|---:|---:|
| `ship_index` | 60 | 60 |
| `callsign[20]` | 64 | 64 |
| `flags` | 84 | 84 |
| `score` | 88 | 88 |
| `energy` | 104 | 104 |
| `lives` | 108 | 108 |
| `invulnerable_time` | 116 | 116 |
| `objnum` | 136 | 136 |
| `weapon_flags` | 168 | 168 |
| `weapon_ammo[21]`, uint16 | 172 | 172 |
| aktive Primärwaffe | 216 | 216 |
| aktive Sekundärwaffe | 228 | 228 |
| `inventory` | 308 | 312 |
| `counter_measures` | 320 | 336 |
| `afterburn_time_left` | 348 | 376 |
| `guided_obj`-Pointer | 420 | 448 |
| sechs Skalare | 432 | 468 |
| `ship_permissions` | 468 | 504 |

`Inventory` ist Rohspeicher mit Pointer, int-Zähler, Pointer. Der Rohzähler liegt ein Pointerfeld hinter dem Inventarbeginn. Nach dem Spielerblock wird gegebenenfalls ein Guided-Objekthandle gespeichert, wenn der rohe Guided-Pointer ungleich null ist. Danach folgen die beiden eigentlichen Inventarlisten. Diesen Zusammenhang erhalten; Pointer nicht pauschal nullen.

Inventarlisten: int32 Anzahl (maximal 10 Arten); pro Eintrag sechs int32 (`type`, `otype`, `id`, `oid`, `flags`, `count`), drei nullterminierte Strings (`description`, `icon_name`, `name`), int32 `iflags`; am Ende int32 Auswahlindex. Beim Bearbeiten auch den Rohzähler im Spielerblock aktualisieren. Der Engine-Lader rekonstruiert die Inventar-Pointer.

### Objekte, Waffen und Effekte

- Schild ist ein float im **Spielerobjekt**, nicht im Spielerblock. Seine Dateiposition muss beim Objektlesen ermittelt werden.
- `light_info`: 52 Byte. `custom_anim`: 24 Byte. `effect_info_s`: 164 Byte. Dynamische Waffenbatterie: 108 Byte, Flags bei Offset 104.
- Die Spielerbatterien haben 21 Einträge. Quad-Bit ist 16 und muss bei Laserindex 0 und Super-Laserindex 5 gesetzt/gelöscht werden.
- Im Effektblock: Flags bei 0, `cloak_time` bei 12, `fade_time` bei 36, `fade_max_time` bei 40.
- God-Mode: Spielerflag 1 plus `invulnerable_time`. Zum Deaktivieren auch das Benachrichtigungsbit `1<<22` löschen.
- Tarnung: Effektflag 1, Fade-in/-out-Bits 512/1024 und HUD-Bit 4096 beim Umschalten löschen, Restdauer setzen. Ein fehlender Effektblock kann mit neutralen Standardwerten hinzugefügt werden.
- Beide Editoren verwenden **1e9 Sekunden** als lange Restdauer. Das ist keine Engine-Modifikation; Skripte, Tod und Levelwechsel können Zustände ändern.
- Unberührte endliche Power-up-Zeiten bleiben unverändert. Die Oberfläche merkt sich explizit betätigte Effekt-Häkchen bzw. „Erneuern“.
- Normale Schild-/Energie-Maxima: jeweils 200. Normale Nachbrennerladung: 5 Sekunden.
- „Volle Karte“ ist eine Aktion, die die gesamte gespeicherte Sichtbarkeitsliste auf 1 setzt. Ein nicht gesetztes Häkchen lässt die bestehende Erkundung erhalten.

Waffenindizes 0–9: Laser, Vauss, Microwave, Plasma, Fusion, Super Laser, Mass Driver, Napalm, EMD Gun, Omega. Indizes 10–19: Concussion, Homing, Impact Mortar, Smart, Mega, Frag, Guided, Napalm Rocket, Cyclone, Black Shark. Flare ist Index 20; sein Besitzbit und unbekannte höhere Bits bleiben erhalten.

Ausrüstung nicht mit dem Nachbrenner-Flag verwechseln: Kühler = Objektnamen `Afterburner` in der ersten Übersetzungstabelle, Konverter = `Converter`, Quad = `QuadLaser`. Neue Inventareinträge verwenden generische Namen und `iflags` 128 / 131 / 132. Vorhandene Einträge bleiben einschließlich ihrer Metadaten erhalten. Originale Icons und sämtliche `Object_info`-Inventarflags werden nicht aus Spieldaten rekonstruiert; das ist ein möglicher späterer Ausbaupunkt.

### Normale Munitionskapazitäten

`weapon_ammo` ist uint16; das technische Speichermaximum ist 65.535. Die normalen Schiffsgrenzen stehen in den proprietären Schiffsdefinitionen, nicht im Savegame und nicht vollständig im Quelltext. Keine geratenen Standardkapazitäten einbauen.

Beide Editoren lesen passende Schiffsdefinitionen aus `Table.gam` bzw. HOG2-Archiven. Datenseite: Byte Seitentyp, int32 Länge einschließlich des Längenfelds; Schiffsseitentyp 6. Unterstützt sind Versionen 1–6; Versionen 5 und 6 wurden im modernen Editor geprüft, native Version 6 im Tabellen-/Archivtest. Ein Physics-Chunk hat 68 serialisierte Byte; die serialisierte statische Waffenbatterie hat 211 Byte ab Version 6, vorher 210; pro Waffenplatz folgen 16 Strings. Zuordnung erfolgt nach gespeichertem Schiffsnamen.

HOG2: Magic `HOG2`, uint32 Anzahl, uint32 Datenoffset; Indexeinträge je 48 Byte (Name 36, Flags 4, Länge 4, Timestamp 4). Indexbeginn wird aus Datenoffset minus Anzahl × 48 ermittelt und auf 68 bzw. 72 begrenzt. In Archiven `Table.gam` zuerst laden, danach Zusatzdefinitionen. Kein komplettes großes HOG in den Arbeitsspeicher laden.

## Aufbau der Implementierungen

### Native Windows-98-Version

| Datei | Zuständigkeit |
|---|---|
| `src/d3core.h/.c` | Parser, bekannte Layouts, Optionen, Inventaränderungen, Patches, Munitionslimit-Tabellen, Zahlenkonvertierung |
| `src/sha256.c` | Eigene portable SHA-256-Implementierung ohne CryptoAPI/CRT |
| `src/d3files.h/.c` | Datei-I/O, unverändertes Original erkennen, sichere Backups/Rücksetzversuch, Spieldaten-/HOG-Leser |
| `src/win98.c` | Native ANSI-Oberfläche mit vier Tabs und klassischem Dateidialog |
| `tests/make_historical_fixtures.cpp` | Unabhängiger Generator mit kopierten historischen C++-Deklarationen und Layout-Assertions |
| `tests/test_core.c` | Native Daten-/Datei-/Fehlerinjektions-Tests |
| `tests/Golden.cs` | Optionaler Vergleich gegen die C#-Implementierung |
| `tests/pe_audit.py` | Tatsächliche PE-Header und Importliste prüfen |

Ziel ist echtes Win9x: ANSI-APIs, `OPENFILENAMEA.lStructSize=76`, eigener `WinMainCRTStartup`, kein CRT-Startup, kein .NET/MFC/UCRT, keine Unicode-Schicht/KernelEx-Abhängigkeit. GCC/MinGW-w64 13.2.0, `-march=i386`, x87, kein SSE, `-nostdlib`, OS-/Subsystemversion 4.0. Es werden nur 49 explizit erlaubte Funktionen aus fünf System-DLLs importiert. Nicht bloß einen modernen Build durch Umschreiben der PE-Versionsnummer „Win98-kompatibel“ nennen.

Die EXE ist 37.888 Byte groß. Die Oberfläche hat feste Maße für mindestens 800 × 600. Zielpfade sind auf 200 ANSI-Zeichen begrenzt, Spielstände auf 32 MiB. Beide Grenzen sind dokumentiert und können später mit entsprechender Prüfung angepasst werden.

Dateiaustausch auf FAT: validierte neue Datei im Zielordner schreiben und flushen; Original in datiertes Backup umbenennen; neue Datei unter den Slotnamen umbenennen. Bei Fehler zurücksetzen, bei weiterem Fehler erhaltenes Backup ausdrücklich nennen. **Nicht als atomare/transaktionale Operation bezeichnen.** Stromausfall kann den Slotnamen zwischenzeitlich fehlen lassen, das Backup muss erhalten bleiben.

Die native Version verwendet für Datei-I/O unter Linux Testadapter. Fehler beim zweiten und dritten Umbenennen werden per `D3_TESTING`-Hook ausgelöst; diese Hooks sind im Release-Build nicht enthalten.

### Vorherige Windows-10/11-Version

Im inneren `Vorgaenger-Windows10.zip`:

- `src/SaveGame.cs`: Parser und Änderungen, bekannte 496-/536-Byte-Layouts.
- `src/ShipLimits.cs`: Spieldaten-/HOG2-Leser.
- `src/Program.cs`: Windows-Forms-Oberfläche.
- .NET Framework 4.x / AnyCPU; keine NuGet-Pakete.
- Automatische Backups über `File.Replace`, vorher SHA-256-Vergleich bei Originalüberschreibung.
- Vollständige weitere Anleitung und Tests im inneren Paket.

Dieses vorherige Paket wurde unverändert erhalten. Nicht versehentlich die Win98-EXE durch den .NET-Build ersetzen.

## Prüfstand und seine Grenzen

**Native Version: 299 Assertions bestanden** mit AddressSanitizer und UndefinedBehaviorSanitizer. LeakSanitizer konnte wegen fehlenden `/proc`-Zugriffs im isolierten Host nicht verwendet werden. Sechs historische Varianten: normal, fehlender Effektblock, Guided, komprimiertes Vorschaubild, Format 1, Format 0.

Zusätzlich zur Format-/Werteprüfung wurden exakte No-op-Roundtrips, Erhaltung undurchsichtiger Bytes, SHA-256-Vektoren (leer, `abc`, eine Million `a`), Backups, externe Änderungen, Austauschfehler und fehlgeschlagenes Zurücksetzen geprüft. Die echte EXE besteht das PE-/Importaudit (49 Funktionen; fünf klassische DLLs).

Der 299. Check ist ein Vergleich zur separat implementierten C#-Version: dieselbe historische Eingabe bearbeiten mit Energie/Schild 200, Nachbrenner 5, Leben 99, Score 123456, alle zwanzig Waffen vorhanden, Primärmunition `i*17`, Sekundärmunition 65535, aktive Waffen 9/19, God/Tarnung/Kühler/Quad/Karte an, Konverter aus. **Beide Ausgabedateien sind exakt byte-identisch**, Größe 13.154 Byte.

`tests/run-tests.sh` reproduziert 298 Checks ohne diesen optionalen C#-Vergleich. Für den Vergleich den alten C#-Quelltext aus dem inneren ZIP entpacken, `Golden.cs` mit `SaveGame.cs` und `ShipLimits.cs` kompilieren, Referenzausgabe erzeugen und deren Pfad dem nativen Testprogramm als zweites Argument übergeben.

**Vorherige moderne Version:** 123 Datentest-Assertions und acht GUI-Assertions unter Mono/X11 bestanden; diese 131 Prüfungen sind im damaligen Paket beschrieben. Sie sind kein Test der nativen Win98-Oberfläche.

## Sinnvolle nächste Schritte

1. Bei Startproblemen unter Win98: ggf. genaue Fehlermeldung, Win98-Ausgabe/Updates und EXE-Hash mit diesem Stand vergleichen; PE-Importliste erneut prüfen. Die Native-Quelle erhalten und keine moderne Laufzeit als stillen Ersatz einführen.
2. Bei Parserfehlern: Spielversion, Speicherversion und Strukturgröße prüfen. Unbekannte Layouts nicht mit ungeprüften Offsetänderungen überschreiben. Ggf. einen echten Beispielspeicherstand analysieren.
3. Bei Spiel-/Ausrüstungsproblemen: nachvollziehbar eine Option nach der anderen testen und den Vergleich gegen das unveränderte Original/Backup führen. Besonders originale Inventarflags/-symbole bei Bedarf aus passenden Spieldaten ableiten.
4. Bei Änderungen: betroffene Format-/Dateitests laufen lassen, native EXE neu bauen, Importaudit erhalten und diese Notiz sowie Prüfprotokoll aktualisieren. Originale Benutzerdateien nie zum Testen überschreiben.
5. Wieder ein vollständiges Projektpaket mit aktuellem Quelltext und aktualisierter Fortsetzungsnotiz sichern.


## Ergänzung: Quelltextbereitstellung für GitHub am 24.10.2026

`D3SaveEditor-Quelltext-GitHub.zip` enthält beide Varianten unter `windows98`
und `windows10`, ohne die fertigen EXE-Dateien oder das verschachtelte ZIP.
Die Programmquellen, Tests, Buildskripte und Laufzeitkonfiguration entsprechen
bytegenau den ursprünglichen Archiven; `SOURCE-PROVENANCE.json` dokumentiert dies.
Die ursprünglichen Projektarchive sind separat erhalten.

Bei der Paketprüfung fiel ein Widerspruch auf: `LICENSE` im bisherigen
Win98-Archiv enthält einen MIT-Text mit Copyright (c) 2024 Kevin Bentley,
während die Quelltexte und README GPL-3.0-or-later nennen. Die Quelltextausgabe
enthält den passenden vollständigen GPL-v3-Text; der vorgefundene MIT-Text
bleibt unter `docs/origin` dokumentiert. Dies ist keine pauschale
Neulizenzierung fremder Beiträge. Die Windows-10-Ausgabe enthielt bereits
den vollständigen GPL-v3-Text.

Die gemeinsame README enthält eine sachliche Kennzeichnung der Entwicklung
mit ChatGPT (OpenAI), getrennt von den vorhandenen Copyrightangaben für
Descent 3. Die historischen Testergebnisse werden unverändert dokumentiert;
beim Verpacken wurden die Anwendungstests nicht erneut ausgeführt.
