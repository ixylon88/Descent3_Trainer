// SPDX-License-Identifier: GPL-3.0-or-later
using D3SaveEditor.Core;
using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Interop;
using System.Windows.Media.Imaging;

namespace D3SaveEditor.Gui
{
    public partial class MainView : Window
    {
        private const string EditorTitle = "Descent 3 · Savegame Editor";
        private readonly ShipLimits limits = new ShipLimits();
        private readonly WeaponGroup primary = new WeaponGroup(0, false);
        private readonly WeaponGroup secondary = new WeaponGroup(10, true);
        private readonly HashSet<CheckBox> touchedEffects = new HashSet<CheckBox>();
        private SupplyRow[] cargoRows;
        private CountermeasureGroup countermeasures;
        private ShipRow[] shipRows;
        private SaveGame game;
        private string openedPath;
        private bool loading;
        private bool dirty;
        private bool preferBlackPyro;

        public MainView()
        {
            InitializeComponent();
            //LoadIconWithFallback("d3_orb__2006+2.ico");
            PrimaryWeaponEditor.Content = primary;
            SecondaryWeaponEditor.Content = secondary;
            foreach (var row in primary.Rows.Concat(secondary.Rows))
                row.PropertyChanged += OnWeaponChanged;
        }

        private void Image_ImageFailed(object sender, ExceptionRoutedEventArgs e)
        {
            //MessageBox.Show(e.ErrorException.Message);
        }

        //private void LoadIconWithFallback(string pfad)
        //{
        //    if (File.Exists(pfad))
        //    {
        //        this.Icon = new BitmapImage(new Uri(pfad, UriKind.RelativeOrAbsolute));
        //    }
        //    else
        //    {
        //        using (Icon sysIcon = SystemIcons.Application)
        //        {
        //            this.Icon = Imaging.CreateBitmapSourceFromHIcon(
        //                sysIcon.Handle,
        //                Int32Rect.Empty,
        //                BitmapSizeOptions.FromEmptyOptions());
        //        }
        //    }
        //}

        public bool OpenPath(string path)
        {
            if (!MaybeDiscard())
                return false;

            try
            {
                string fullPath = Path.GetFullPath(path);
                var parsed = SaveFiles.Open(fullPath);
                LoadGame(parsed, fullPath);
                Status.Text = "Spielstand geladen. Das Original wurde nicht verändert.";
                return true;
            }
            catch (Exception ex)
            {
                ShowError(ex);
                return false;
            }
        }

        internal void ShowTab(int index)
        {
            EditorTabs.SelectedIndex = Math.Max(0, Math.Min(EditorTabs.Items.Count - 1, index));
        }

        private void OnChooseOpen(object sender, RoutedEventArgs e)
        {
            var dialog = new OpenFileDialog
            {
                Title = "Descent-3-Spielstand öffnen (z. B. saveg000)",
                Filter = "Spielstände (saveg*)|saveg*|Alle Dateien (*.*)|*.*",
                CheckFileExists = true
            };
            if (dialog.ShowDialog(this) == true)
                OpenPath(dialog.FileName);
        }

        private void OnDragOver(object sender, DragEventArgs e)
        {
            var files = e.Data.GetData(DataFormats.FileDrop) as string[];
            e.Effects = files?.Length == 1 ? DragDropEffects.Copy : DragDropEffects.None;
            e.Handled = true;
        }

        private void OnDrop(object sender, DragEventArgs e)
        {
            var files = e.Data.GetData(DataFormats.FileDrop) as string[];
            if (files?.Length == 1)
                OpenPath(files[0]);
            e.Handled = true;
        }

        private void OnClosing(object sender, CancelEventArgs e)
        {
            e.Cancel = !MaybeDiscard();
        }

        private bool MaybeDiscard()
        {
            return !dirty || MessageBox.Show(this, "Die Änderungen sind noch nicht gespeichert. Verwerfen?",
                "Ungespeicherte Änderungen", MessageBoxButton.YesNo, MessageBoxImage.Question) == MessageBoxResult.Yes;
        }

        private void OnReset(object sender, RoutedEventArgs e)
        {
            LoadValues();
            Status.Text = "Originalwerte geladen.";
        }

        private void LoadGame(SaveGame parsed, string fullPath)
        {
            game = parsed;
            openedPath = fullPath;
            LoadValues();
        }

        private void LoadValues()
        {
            loading = true;
            try
            {
                var original = game.CurrentOptions();
                EnergyInput.Text = FormatFloat(original.Energy);
                ShieldsInput.Text = FormatFloat(Math.Max(0, Math.Min(1000000000, original.Shields)));
                AfterburnerInput.Text = FormatFloat(original.AfterburnerSeconds);
                LivesInput.Text = original.Lives.ToString();
                ScoreInput.Text = original.Score.ToString();
                GodModeInput.IsChecked = game.Invulnerable;
                CloakInput.IsChecked = game.IsCloaked;
                CoolerInput.IsChecked = game.HasEquipment("Afterburner");
                ConverterInput.IsChecked = game.HasEquipment("Converter");
                QuadInput.IsChecked = game.HasQuad;
                MapInput.IsChecked = false;

                SetEquipmentAvailability(CoolerInput, game.FindEquipment("Afterburner") >= 0,
                    "Passiver Nachbrenner-Kühler im Inventar.", "Die Definition „Afterburner“ fehlt im Spielstand.");
                SetEquipmentAvailability(ConverterInput, game.FindEquipment("Converter") >= 0,
                    "Auswählbarer Energie-Schild-Konverter im Inventar.", "Die Definition „Converter“ fehlt im Spielstand.");
                SetEquipmentAvailability(QuadInput, game.FindEquipment("QuadLaser") >= 0 && game.SupportsQuad,
                    "Setzt Inventargegenstand und beide Laser-Waffenbatterien.", "Gegenstandsdefinition oder Waffenbatterien fehlen.");

                foreach (var row in primary.Rows.Concat(secondary.Rows))
                {
                    row.Owned = original.Owned[row.Index];
                    row.Ammo = original.Ammo[row.Index].ToString();
                }
                primary.SelectedIndex = original.Primary;
                secondary.SelectedIndex = original.Owned.Skip(10).Any(owned => owned) ? original.Secondary - 9 : 0;
                RefreshWeaponLimits();
                LoadInventory();

                touchedEffects.Clear();
                RefreshEffectTimers();
                MapInfo.Text = $"Gespeichert sichtbar: {game.RevealedRooms} von {game.HighestRoom + 1} Raumplätzen.";
                SaveTitle.Text = $"{Path.GetFileName(openedPath)}  ·  {game.Description}";
                SaveDetails.Text = $"{game.Callsign}  ·  {game.ShipName}  ·  {game.Mission} / Level {game.Level}  ·  Format {game.Version}  ·  {game.Layout.Name}";
                Review.Text = FileReport() + "\r\nKeine geplanten Änderungen.\r\n";

                SaveButton.IsEnabled = CopyButton.IsEnabled = ResetButton.IsEnabled = true;
                PlayerEditor.IsEnabled = PrimaryTab.IsEnabled = SecondaryTab.IsEnabled = CountermeasureTab.IsEnabled = InventoryTab.IsEnabled = ReviewButton.IsEnabled = true;
                dirty = false;
                Title = EditorTitle;
            }
            finally
            {
                loading = false;
            }
        }

        private static void SetEquipmentAvailability(CheckBox box, bool enabled, string available, string unavailable)
        {
            box.IsEnabled = enabled;
            box.ToolTip = enabled ? available : unavailable;
        }

        private void LoadInventory()
        {
            cargoRows = SaveGame.CargoItems.Select(item => new SupplyRow(game, item)).ToArray();
            countermeasures = new CountermeasureGroup(game);
            shipRows = SaveGame.StandardShips.Select(name => new ShipRow(game, name)).ToArray();
            foreach (var row in cargoRows.Concat<SupplyRow>(countermeasures.Rows))
                row.PropertyChanged += OnInventoryChanged;
            foreach (var row in shipRows)
                row.PropertyChanged += OnInventoryChanged;
            CargoInputs.ItemsSource = cargoRows;
            CountermeasureEditor.Content = countermeasures;
            ShipInputs.ItemsSource = shipRows;
            bool blackPyroDefined = game.IsShipDefined(SaveGame.BlackPyro);
            BlackPyroUnlockInput.Content = blackPyroDefined ? SaveGame.BlackPyro : SaveGame.BlackPyro + " (fehlt)";
            BlackPyroUnlockInput.IsChecked = game.IsShipAllowed(SaveGame.BlackPyro);
            BlackPyroUnlockInput.IsEnabled = blackPyroDefined && BlackPyroUnlockInput.IsChecked != true;
            preferBlackPyro = false;
            RefreshBlackPyroButton();

            var entries = game.InventoryItems.Select(InventoryLine).ToArray();
            SavedInventory.ItemsSource = entries.Length == 0 ? new[] { "Das Inventar ist leer." } : entries;
        }

        private static string InventoryLine(InventoryItem item)
        {
            string name = string.IsNullOrEmpty(item.Name) ? $"ID {(item.Id < 0 ? item.OriginalId : item.Id)}" : item.Name;
            return $"{name} × {item.Count}";
        }

        private void OnPreferBlackPyro(object sender, RoutedEventArgs e)
        {
            if (game == null || !game.IsShipDefined(SaveGame.BlackPyro))
                return;

            preferBlackPyro = true;
            foreach (var row in shipRows)
            {
                row.Allowed = false;
                row.CanUnlock = row.Defined;
            }
            BlackPyroUnlockInput.IsChecked = true;
            BlackPyroUnlockInput.IsEnabled = false;
            RefreshBlackPyroButton();
            MarkDirty();
        }

        private void RefreshBlackPyroButton()
        {
            PreferBlackPyroButton.IsEnabled = game.IsShipDefined(SaveGame.BlackPyro) &&
                ((!preferBlackPyro && !game.PrefersBlackPyro) || shipRows.Any(row => row.Allowed));
        }

        private void RefreshEffectTimers()
        {
            GodTime.Text = EffectTime(ReadEffect(GodModeInput), game.Invulnerable, game.InvulnerableTime);
            CloakTime.Text = EffectTime(ReadEffect(CloakInput), game.IsCloaked, game.CloakTime);
            GodRenewButton.IsEnabled = game.Invulnerable && !touchedEffects.Contains(GodModeInput);
            CloakRenewButton.IsEnabled = game.IsCloaked && !touchedEffects.Contains(CloakInput);
        }

        private static string EffectTime(bool? edit, bool enabled, float seconds)
        {
            if (edit.HasValue)
                return edit.Value ? "Beim Speichern: 1.000.000.000 s Restdauer" : "Beim Speichern: inaktiv";
            if (!enabled)
                return "Gespeichert: inaktiv";
            return seconds >= 100000000
                ? "Gespeichert: dauerhaft (lange Restdauer)"
                : $"Gespeichert: aktiv, Restdauer {seconds:N1} s";
        }

        private void OnInputChanged(object sender, RoutedEventArgs e)
        {
            if (!loading && game != null)
                MarkDirty();
        }

        private void OnInventoryChanged(object sender, PropertyChangedEventArgs e)
        {
            if (loading)
                return;

            if (sender is ShipRow && e.PropertyName == nameof(ShipRow.Allowed))
                RefreshBlackPyroButton();
            if (sender is CountermeasureRow && e.PropertyName == nameof(CountermeasureRow.Owned))
                countermeasures.SelectPresent();
            MarkDirty();
        }

        private void OnEffectChanged(object sender, RoutedEventArgs e)
        {
            if (loading || game == null)
                return;

            touchedEffects.Add((CheckBox)sender);
            RefreshEffectTimers();
            MarkDirty();
        }

        private void OnRenewEffect(object sender, RoutedEventArgs e)
        {
            var box = (CheckBox)((Button)sender).Tag;
            box.IsChecked = true;
            OnEffectChanged(box, e);
        }

        private void MarkDirty()
        {
            dirty = true;
            Title = "* " + EditorTitle;
            Status.Text = "Änderungen noch nicht gespeichert.";
        }

        private void OnMaxEnergy(object sender, RoutedEventArgs e) => EnergyInput.Text = "200";
        private void OnMaxShields(object sender, RoutedEventArgs e) => ShieldsInput.Text = "200";
        private void OnMaxAfterburner(object sender, RoutedEventArgs e) => AfterburnerInput.Text = "5";

        private void OnWeaponChanged(object sender, PropertyChangedEventArgs e)
        {
            if (loading || game == null)
                return;

            var row = (WeaponRow)sender;
            if (e.PropertyName == nameof(WeaponRow.Owned))
            {
                if (!row.Owned)
                    row.Ammo = "0";
                (row.Index < 10 ? primary : secondary).SelectOwnedWeapon();
            }

            if (e.PropertyName == nameof(WeaponRow.Ammo) || e.PropertyName == nameof(WeaponRow.Owned))
                MarkDirty();
        }

        private void OnActiveWeaponChanged(object sender, SelectionChangedEventArgs e)
        {
            var combo = (ComboBox)sender;
            int selection = combo.SelectedIndex;
            if (loading || game == null || selection < 0)
                return;

            if (combo.DataContext is CountermeasureGroup measures)
            {
                if (selection == measures.SelectedIndex)
                    return;
                measures.Select(selection);
                combo.SetCurrentValue(ComboBox.SelectedIndexProperty, measures.SelectedIndex);
                MarkDirty();
                return;
            }

            var group = combo.DataContext as WeaponGroup;
            if (group == null || selection == group.SelectedIndex)
                return;

            if (selection < group.FirstChoice)
                SetWeapons(group, false);
            else
                group.Rows[selection - group.FirstChoice].Owned = true;

            group.SelectedIndex = selection;
            MarkDirty();
        }

        private void OnSelectWeapons(object sender, RoutedEventArgs e) => SetOwned(((Button)sender).DataContext, true);
        private void OnClearWeapons(object sender, RoutedEventArgs e) => SetOwned(((Button)sender).DataContext, false);

        private static void SetOwned(object group, bool owned)
        {
            if (group is CountermeasureGroup measures)
                measures.SetOwned(owned);
            else
                SetWeapons((WeaponGroup)group, owned);
        }

        private static void SetWeapons(WeaponGroup group, bool owned)
        {
            foreach (var row in group.Rows)
                row.Owned = owned;
        }

        private void OnMaxWeapon(object sender, RoutedEventArgs e)
        {
            var context = ((Button)sender).DataContext;
            if (context is CountermeasureRow measure)
            {
                measure.Fill();
                return;
            }

            var row = (WeaponRow)context;
            var caps = limits.ForShip(game.ShipName);
            row.Ammo = (caps == null ? ushort.MaxValue : caps[row.Index]).ToString();
        }

        private void OnMaxShipWeapons(object sender, RoutedEventArgs e)
        {
            var context = ((Button)sender).DataContext;
            if (context is CountermeasureGroup measures)
            {
                foreach (var row in measures.Rows.Where(row => row.CanToggle && row.Owned))
                    row.Fill();
            }
            else
                MaxWeapons((WeaponGroup)context, true);
        }
        private void OnMaxStorageWeapons(object sender, RoutedEventArgs e) => MaxWeapons((WeaponGroup)((Button)sender).DataContext, false);

        private void MaxWeapons(WeaponGroup group, bool useShipLimit)
        {
            var caps = useShipLimit ? limits.ForShip(game.ShipName) : null;
            if (useShipLimit && caps == null)
            {
                MessageBox.Show(this, "Bitte über „Schiffslimits laden“ eine Spieldatendatei mit einer Definition für das aktuelle Schiff öffnen.",
                    "Schiffslimits fehlen", MessageBoxButton.OK, MessageBoxImage.Information);
                return;
            }

            foreach (var row in group.Rows.Where(row => row.CanEditAmmo))
                row.Ammo = (caps == null ? ushort.MaxValue : caps[row.Index]).ToString();
        }

        private void OnLoadLimits(object sender, RoutedEventArgs e)
        {
            var dialog = new OpenFileDialog
            {
                Title = "Spieldaten mit Schiffsdefinitionen laden",
                Filter = "Spieldaten (*.hog;*.mn3;*.gam)|*.hog;*.mn3;*.gam|Alle Dateien (*.*)|*.*"
            };
            if (dialog.ShowDialog(this) != true)
                return;

            try
            {
                limits.Load(dialog.FileName);
                if (game != null)
                    RefreshWeaponLimits();
                Status.Text = $"{limits.Ships.Count} Schiffsdefinitionen geladen aus {limits.Source}.";
            }
            catch (Exception ex)
            {
                ShowError(ex);
            }
        }

        private void RefreshWeaponLimits()
        {
            var caps = limits.ForShip(game.ShipName);
            foreach (var row in primary.Rows.Concat(secondary.Rows))
                row.Limit = !row.UsesAmmo ? "Energiewaffe" : caps == null ? "nicht geladen" : caps[row.Index].ToString("N0");

            string hint = caps == null
                ? "„Vorhandene: Schiffslimit“ braucht die Munitionsgrenzen des aktuellen Schiffs. Öffne dafür „Schiffslimits laden“. „Max“ pro Zeile setzt bis dahin 65.535."
                : $"„Vorhandene: Schiffslimit“ füllt alle vorhandenen Munitionswaffen bis zur angezeigten Kapazität für {game.ShipName} auf. „Max“ füllt jeweils eine Zeile.";
            primary.Hint = secondary.Hint = hint;
        }

        private static string FormatFloat(float number) => number.ToString("G9", CultureInfo.CurrentCulture);

        private static float ReadFloat(TextBox input, float original, float maximum, string name)
        {
            if (input.Text.Trim() == FormatFloat(original))
                return original;

            float value;
            if (float.TryParse(input.Text, NumberStyles.Float, CultureInfo.CurrentCulture, out value)
                && !float.IsNaN(value) && value >= 0 && value <= maximum)
                return value;

            throw new FormatProblem($"{name} muss eine Zahl von 0 bis {maximum:N0} sein.");
        }

        private static int ReadInteger(string text, int minimum, int maximum, string name)
        {
            int value;
            if (int.TryParse(text, NumberStyles.Integer, CultureInfo.CurrentCulture, out value) && value >= minimum && value <= maximum)
                return value;

            throw new FormatProblem($"{name} muss eine ganze Zahl von {minimum:N0} bis {maximum:N0} sein.");
        }

        private EditOptions ReadOptions()
        {
            var options = game.CurrentOptions();
            options.Energy = ReadFloat(EnergyInput, options.Energy, 1000000000, "Energie");
            options.Shields = ReadFloat(ShieldsInput, options.Shields, 1000000000, "Schild");
            options.AfterburnerSeconds = ReadFloat(AfterburnerInput, options.AfterburnerSeconds, 1000000, "Nachbrenner");
            options.Lives = (byte)ReadInteger(LivesInput.Text, 0, 255, "Leben");
            options.Score = ReadInteger(ScoreInput.Text, int.MinValue, int.MaxValue, "Punktestand");

            foreach (var row in primary.Rows.Concat(secondary.Rows))
            {
                options.Owned[row.Index] = row.Owned;
                options.Ammo[row.Index] = (ushort)ReadInteger(row.Ammo, 0, ushort.MaxValue, $"Munition {row.Name}");
            }
            options.Primary = primary.SelectedIndex;
            if (secondary.SelectedIndex > 0)
                options.Secondary = secondary.SelectedIndex + 9;

            options.GodMode = ReadEffect(GodModeInput);
            options.Cloaked = ReadEffect(CloakInput);
            options.Cooler = ReadEffect(CoolerInput);
            options.Converter = ReadEffect(ConverterInput);
            options.QuadLasers = ReadEffect(QuadInput);
            options.RevealMap = MapInput.IsChecked == true;
            ReadSupplies(cargoRows, options.CargoCounts);
            ReadSupplies(countermeasures.Rows, options.CountermeasureCounts);
            int activeMeasure = countermeasures.ActiveWeaponId;
            if (activeMeasure >= 0 && (game.ActiveCountermeasure?.Type != 5 || game.ActiveCountermeasure.Id != activeMeasure))
                options.ActiveCountermeasureWeaponId = activeMeasure;
            options.PreferBlackPyro = preferBlackPyro;
            for (int i = 0; i < shipRows.Length; i++)
                options.UnlockShips[i] = shipRows[i].CanUnlock && shipRows[i].Allowed;
            options.UnlockBlackPyro = !options.PreferBlackPyro && BlackPyroUnlockInput.IsChecked == true && !game.IsShipAllowed(SaveGame.BlackPyro);

            return options;
        }

        private bool? ReadEffect(CheckBox box) => touchedEffects.Contains(box) ? box.IsChecked : null;

        private static void ReadSupplies(SupplyRow[] rows, int?[] counts)
        {
            for (int i = 0; i < rows.Length; i++)
            {
                var row = rows[i];
                if (!row.CanEdit)
                    continue;
                int count = ReadInteger(row.CountText, 0, row.Maximum, row.Item.Name);
                if (count != row.OriginalCount)
                    counts[i] = count;
            }
        }

        private string FileReport()
        {
            return $"DATEI\r\n{openedPath}\r\nSHA-256: {game.Hash}\r\n\r\n" +
                $"FORMAT\r\nVersion {game.Version} · {game.Layout.Name}\r\nSpielerblock: 0x{game.PlayerOffset:X}\r\n" +
                $"Karte: {game.MapCount} Einträge · Inventar: {game.InventoryItems.Count} Arten · Gegenmaßnahmen: {game.CountermeasureItems.Count} Arten\r\n\r\n";
        }

        private void OnCheckChanges(object sender, RoutedEventArgs e) => CheckChanges();

        private EditResult CheckChanges()
        {
            try
            {
                var result = game.Edit(ReadOptions());
                string changes = result.Changes.Count == 0
                    ? "Keine Änderungen. Bytefolge identisch."
                    : string.Join("\r\n", result.Changes.Select(change => "• " + change));
                Review.Text = FileReport() + "GEPRÜFTE ÄNDERUNGEN\r\n" + changes +
                    $"\r\n\r\nAusgabe erneut erfolgreich eingelesen.\r\nGröße: {game.Bytes.Length} → {result.Bytes.Length} Byte\r\n";
                Status.Text = result.Changes.Count == 0
                    ? "Prüfung bestanden: keine Änderung."
                    : $"Prüfung bestanden: {result.Changes.Count} Änderungsgruppen.";
                return result;
            }
            catch (Exception ex)
            {
                Review.Text = FileReport() + "PRÜFUNG FEHLGESCHLAGEN\r\n" + ex.Message;
                ShowError(ex);
                return null;
            }
        }

        private void OnSave(object sender, RoutedEventArgs e) => SaveTo(false);
        private void OnSaveCopy(object sender, RoutedEventArgs e) => SaveTo(true);

        private void SaveTo(bool asCopy)
        {
            var result = CheckChanges();
            if (result == null)
                return;

            string destination = openedPath;
            if (asCopy)
            {
                var dialog = new SaveFileDialog
                {
                    Title = "Bearbeiteten Spielstand als Kopie speichern",
                    Filter = "Alle Dateien (*.*)|*.*",
                    AddExtension = false,
                    FileName = Path.GetFileName(openedPath) + ".edited",
                    InitialDirectory = Path.GetDirectoryName(openedPath),
                    OverwritePrompt = true
                };
                if (dialog.ShowDialog(this) != true)
                    return;
                destination = dialog.FileName;
            }

            try
            {
                string backup = SaveFiles.Write(destination, result, openedPath, game.Hash);
                LoadGame(result.Parsed, Path.GetFullPath(destination));
                Status.Text = $"Gespeichert: {Path.GetFileName(destination)}" +
                    (backup == null ? "" : $" · Backup: {Path.GetFileName(backup)}");
            }
            catch (Exception ex)
            {
                ShowError(ex);
            }
        }

        private void ShowError(Exception ex)
        {
            Status.Text = "Fehler: " + ex.Message;
            MessageBox.Show(this, ex.Message, EditorTitle, MessageBoxButton.OK, MessageBoxImage.Error);
        }

        private void OnHelp(object sender, RoutedEventArgs e)
        {
            MessageBox.Show(this,
                "1. Im Spiel speichern und das Spiel beenden.\r\n" +
                "2. saveg000, saveg001 usw. aus dem Ordner savegame öffnen.\r\n" +
                "3. Werte ändern und „Speichern + Backup“ wählen.\r\n" +
                "4. Den Spielstand im Spiel neu laden.\r\n\r\n" +
                "Bei jedem Überschreiben entsteht daneben eine datierte .bak-Datei.\r\n" +
                "Kopien mit Endung .edited zum Laden in einen passenden savegNNN-Slot umbenennen.\r\n\r\n" +
                "„Inventar / Schiffe“ bearbeitet Bestände und die Freigaben für das Briefing.\r\n" +
                "„Max“ nutzt das Schiffslimit aus Table.gam / d3.hog, sonst 65.535.\r\n" +
                "„Verlängern“ setzt 1.000.000.000 s Restdauer für einen aktiven Effekt.\r\n\r\n" +
                "Unterstützt Windows-Strukturen mit 496 bzw. 536 Byte und Format 0–2.",
                "Anleitung", MessageBoxButton.OK, MessageBoxImage.Information);
        }
    }
}
