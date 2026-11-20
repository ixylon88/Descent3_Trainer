// SPDX-License-Identifier: GPL-3.0-or-later
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Windows;
using System.Windows.Automation;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Media;
using System.Windows.Threading;
using D3SaveEditor.Core;
using D3SaveEditor.Gui;

namespace D3SaveEditor.Tests
{
    internal sealed class UiTests
    {
        private int passed;

        internal void Run(string fixture)
        {
            string root = Path.GetDirectoryName(fixture);
            string source = Path.Combine(root, "saveg000");
            File.Copy(fixture, source);
            var view = new MainView();
            try
            {
                view.Show();
                ChangeLanguage(view, "de");
                Update(view);
                Check(!Field<Button>(view, "SaveButton").IsEnabled, "Save requires a loaded file");
                Check(view.OpenPath(source), "WPF opens independent synthetic fixture");
                Update(view);

                var save = Field<Button>(view, "SaveButton");
                var energy = Field<TextBox>(view, "EnergyInput");
                var rapidFire = Field<CheckBox>(view, "RapidFireInput");
                Check(save.IsEnabled && energy.Text.StartsWith("100", StringComparison.Ordinal), "Loaded player values are displayed");
                Check(rapidFire.IsEnabled && rapidFire.IsChecked == false && Equals(rapidFire.Content, "Schnellfeuer"),
                    "Rapid fire displays its saved state in German");
                var tabs = Field<TabControl>(view, "EditorTabs");
                tabs.SelectedItem = Field<TabItem>(view, "InventoryTab");
                Update(view);
                Check(!view.Title.StartsWith("*", StringComparison.Ordinal), "Viewing inventory leaves the save unchanged");
                Click(save);
                Check(File.ReadAllBytes(source).SequenceEqual(File.ReadAllBytes(fixture)), "Initial save preserves exact bytes");

                tabs.SelectedIndex = 1;
                Update(view);
                SelectWeapon(tabs, 4);
                Update(view);
                Check(WeaponCheck(tabs, "Fusionskanone").IsChecked == true, "Choosing active primary grants that weapon");

                tabs.SelectedIndex = 2;
                Update(view);
                SelectWeapon(tabs, 0);
                Update(view);
                Check(WeaponCheck(tabs, "Erschütterungsgeschoß").IsChecked == false, "Choosing no secondary removes its ownership");
                WeaponCheck(tabs, "Black Shark Geschoß").SetCurrentValue(ToggleButton.IsCheckedProperty, true);
                Update(view);
                Text(Find<TextBox>(tabs, box => AutomationProperties.GetName(box) == "Munition Black Shark Geschoß"), "65535");

                tabs.SelectedIndex = 0;
                Update(view);
                energy.Text = "200";
                Field<TextBox>(view, "ShieldsInput").Text = "200";
                Field<TextBox>(view, "AfterburnerInput").Text = "5";
                Field<TextBox>(view, "LivesInput").Text = "50";
                Field<TextBox>(view, "ScoreInput").Text = "999";
                foreach (string name in new[] { "GodModeInput", "CloakInput", "RapidFireInput", "CoolerInput", "QuadInput", "MapInput" })
                    Field<CheckBox>(view, name).IsChecked = true;
                Field<CheckBox>(view, "ConverterInput").IsChecked = false;

                tabs.SelectedItem = Field<TabItem>(view, "CountermeasureTab");
                Update(view);
                Check(Equals(Find<ComboBox>(tabs, box => box.Name == "ActiveWeapon").SelectedItem, "Hüpfende Betty"),
                    "The saved countermeasure selection is visible");
                var chaffInput = Find<TextBox>(tabs, box => AutomationProperties.GetName(box) == "Menge Störspreu");
                Check(chaffInput.Text == "2", "Saved Chaff count is displayed");
                var countInputs = Descendants(tabs).OfType<TextBox>()
                    .Where(box => AutomationProperties.GetName(box).StartsWith("Menge ", StringComparison.Ordinal)).ToArray();
                var maxButtons = Descendants(tabs).OfType<Button>()
                    .Where(button => Equals(button.Content, "Max") && countInputs.Any(box => ReferenceEquals(box.DataContext, button.DataContext))).ToArray();
                Click(Find<Button>(tabs, button => Equals(button.Content, "Alle abwählen")));
                Update(view);
                Check(countInputs.Length == 5 && maxButtons.Length == 5 &&
                    countInputs.All(box => !box.IsEnabled && box.Text == "0") && maxButtons.All(button => !button.IsEnabled),
                    "Clearing all countermeasures disables the count inputs and Max buttons");
                Click(Find<Button>(tabs, button => Equals(button.Content, "Alle auswählen")));
                Update(view);
                Check(countInputs.All(box => box.IsEnabled) && maxButtons.All(button => button.IsEnabled),
                    "Selecting the available countermeasures re-enables their controls");
                Click(Find<Button>(tabs, button => Equals(button.Content, "Vorhandene: Spiellimit")));
                Update(view);
                Check(chaffInput.Text == "12" && Find<TextBox>(tabs, box => AutomationProperties.GetName(box) == "Menge GunBoy").Text == "4",
                    "Countermeasure fill uses the individual game limits");
                WeaponCheck(tabs, "Hüpfende Betty").SetCurrentValue(ToggleButton.IsCheckedProperty, false);
                SelectWeapon(tabs, 4);
                ChangeLanguage(view, "en");
                Update(view);
                Check(Equals(Field<TabItem>(view, "CountermeasureTab").Header, "Countermeasures") &&
                    Equals(Find<ComboBox>(tabs, box => box.Name == "ActiveWeapon").SelectedItem, "GunBoy") &&
                    Find<ComboBox>(tabs, box => box.Name == "ActiveWeapon").Items.Contains("Bouncing Betty") &&
                    Find<TextBox>(tabs, box => AutomationProperties.GetName(box) == "Count for Chaff").Text == "12",
                    "English updates captions and preserves draft values and active selection");
                ChangeLanguage(view, "de");
                Update(view);

                tabs.SelectedItem = Field<TabItem>(view, "InventoryTab");
                Update(view);
                var cargo = Field<ItemsControl>(view, "CargoInputs");
                var ships = Field<ItemsControl>(view, "ShipInputs");
                var inventory = Field<ListBox>(view, "SavedInventory").Items.Cast<string>().ToArray();
                Check(CountInput(cargo, 0).Text == "2" && inventory.Any(item => item.Contains("Mission cargo")) &&
                    inventory.All(item => !item.Contains("Saved Chaff")), "Saved inventory lists only player inventory entries");
                var pyro = Find<CheckBox>(ships, box => Equals(box.Content, "Pyro-GL"));
                var phoenix = Find<CheckBox>(ships, box => Equals(box.Content, "Phoenix"));
                Check(pyro.IsChecked == true && !pyro.IsEnabled && phoenix.IsChecked == false && phoenix.IsEnabled,
                    "Already allowed ships and available unlocks are distinguished");
                var blackPyro = Field<CheckBox>(view, "BlackPyroUnlockInput");
                var blackPyroMode = Field<Button>(view, "PreferBlackPyroButton");
                Check(Equals(blackPyro.Content, "Black Pyro") && blackPyro.IsChecked == true && blackPyroMode.IsEnabled,
                    "Black Pyro and its saved permission are visible");

                Text(CountInput(cargo, 0), "4");
                phoenix.SetCurrentValue(ToggleButton.IsCheckedProperty, true);
                Update(view);
                Click(save);

                var saved = SaveFiles.Open(source);
                var options = saved.CurrentOptions();
                Check(saved.Energy == 200 && saved.Shields == 200 && options.Lives == 50 && options.Score == 999,
                    "WPF save writes edited player values");
                Check(options.Primary == 4 && options.Secondary == 19 && options.Ammo[19] == 65535,
                    "WPF save writes selected weapons and maximum ammo");
                Check(saved.Invulnerable && saved.IsCloaked && saved.HasEquipment("Afterburner") && saved.HasQuad && !saved.HasEquipment("Converter"),
                    "WPF save writes effects and equipment");
                Check(saved.HasRapidFire && rapidFire.IsChecked == true, "WPF saves and displays enabled rapid fire");
                Check(saved.InventoryItems.Any(item => item.Name == "Force Wall" && item.Count == 4)
                    && saved.CountermeasureItems.Any(item => item.Name == "Saved Chaff" && item.Count == 12)
                    && saved.CountermeasureItems.All(item => item.Name != "Saved Betty"), "WPF save writes inventory edits");
                Check(saved.ActiveCountermeasure?.Id == saved.CountermeasureWeaponId(3), "WPF saves the chosen active countermeasure");
                Check(saved.ShipPermissions == 0x40000023u && saved.ShipName == "Pyro-GL", "WPF saves the unlock by ship name");
                Check(Directory.GetFiles(root, "*.bak-*").Length == 2, "Both saves create backups");

                byte[] before = File.ReadAllBytes(source);
                Click(save);
                Check(File.ReadAllBytes(source).SequenceEqual(before), "Unchanged resave preserves exact bytes");
                Click(blackPyroMode);
                Update(view);
                Check(Find<CheckBox>(ships, box => Equals(box.Content, "Pyro-GL")).IsChecked == false && blackPyro.IsChecked == true,
                    "The Black Pyro button clears other visible permissions immediately");
                Click(save);
                Check(SaveFiles.Open(source).PrefersBlackPyro && !blackPyroMode.IsEnabled,
                    "WPF saves the Black Pyro briefing mode and shows the saved state");
                tabs.SelectedIndex = 0;
                Update(view);
                Click(Field<Button>(view, "EnergyHighButton"));
                Click(Field<Button>(view, "ShieldsHighButton"));
                Click(Field<Button>(view, "AfterburnerHighButton"));
                Click(save);
                var highValues = SaveFiles.Open(source);
                Check(highValues.Energy == 999999 && highValues.Shields == 999999 && highValues.Afterburner == 999999,
                    "WPF high-value presets save 999,999 for all three supplies");
                rapidFire.IsChecked = false;
                Click(save);
                Check(!SaveFiles.Open(source).HasRapidFire && rapidFire.IsChecked == false, "WPF can turn rapid fire off again");
                Console.WriteLine(passed + " WPF assertions passed.");
            }
            finally
            {
                // Discard temporary edits before closing after a failed assertion.
                var reset = Field<Button>(view, "ResetButton");
                if (reset.IsEnabled)
                    Click(reset);
                view.Close();
            }
        }

        private static T Field<T>(MainView view, string name) where T : FrameworkElement => (T)view.FindName(name);
        private static void SelectWeapon(TabControl tabs, int index) =>
            Find<ComboBox>(tabs, box => box.Name == "ActiveWeapon").SetCurrentValue(Selector.SelectedIndexProperty, index);
        private static CheckBox WeaponCheck(DependencyObject tab, string name) =>
            Find<CheckBox>(tab, box => AutomationProperties.GetName(box) == name);
        private static TextBox CountInput(ItemsControl items, int index) =>
            Find<TextBox>(items.ItemContainerGenerator.ContainerFromIndex(index), box => box.Name == "SupplyCount");
        private static void Click(Button button) => button.RaiseEvent(new RoutedEventArgs(ButtonBase.ClickEvent));
        private static void ChangeLanguage(MainView view, string code)
        {
            var button = Field<Button>(view, "LanguageButton");
            Click(button);
            var item = button.ContextMenu.Items.Cast<MenuItem>().Single(entry => Equals(entry.Tag, code));
            item.RaiseEvent(new RoutedEventArgs(MenuItem.ClickEvent));
        }
        private static void Text(TextBox input, string text) => input.SetCurrentValue(TextBox.TextProperty, text);

        private static void Update(Window window)
        {
            window.UpdateLayout();
            window.Dispatcher.Invoke(() => { }, DispatcherPriority.ApplicationIdle);
        }

        private static T Find<T>(DependencyObject root, Func<T, bool> matches) where T : DependencyObject
        {
            var result = Descendants(root).OfType<T>().FirstOrDefault(matches);
            return result ?? throw new InvalidOperationException(typeof(T).Name + " not found");
        }

        private static IEnumerable<DependencyObject> Descendants(DependencyObject root)
        {
            for (int i = 0; i < VisualTreeHelper.GetChildrenCount(root); i++)
            {
                var child = VisualTreeHelper.GetChild(root, i);
                yield return child;
                foreach (var descendant in Descendants(child))
                    yield return descendant;
            }
        }

        private void Check(bool condition, string description)
        {
            if (!condition)
                throw new InvalidOperationException(description);
            passed++;
            Console.WriteLine("PASS: " + description);
        }
    }
}
