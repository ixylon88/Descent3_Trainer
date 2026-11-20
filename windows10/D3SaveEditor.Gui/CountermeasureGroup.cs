// SPDX-License-Identifier: GPL-3.0-or-later
using System;
using System.Globalization;
using System.Linq;
using System.Windows;
using D3SaveEditor.Core;

namespace D3SaveEditor.Gui
{
    internal sealed class CountermeasureRow : SupplyRow
    {
        private bool owned;

        internal CountermeasureRow(SaveGame game, SupplyItem item, int index) : base(game, item)
        {
            WeaponId = game.CountermeasureWeaponId(index);
            owned = OriginalCount > 0;
            PropertyChanged += (sender, e) =>
            {
                if (e.PropertyName == nameof(CountText) && int.TryParse(CountText, out int count))
                    SetField(ref owned, count > 0, nameof(Owned));
            };
        }

        internal int WeaponId { get; }
        public string Name => Item.Name;
        public string AmmoLabel => "Menge " + Name;
        public bool CanToggle => CanEdit;
        public bool CanEditAmmo => CanEdit;
        public string Limit => CanEdit ? Item.Maximum.ToString("N0", CultureInfo.CurrentCulture) : "–";

        public string Ammo
        {
            get => CanEdit ? CountText : "–";
            set
            {
                if (CountText == value)
                    return;
                CountText = value;
                Notify(nameof(Ammo));
            }
        }

        public bool Owned
        {
            get => owned;
            set
            {
                if (CanEdit && value != owned)
                    Ammo = value ? "1" : "0";
            }
        }

        internal void Fill() => Ammo = Item.Maximum.ToString(CultureInfo.CurrentCulture);
    }

    internal sealed class CountermeasureGroup : BindableItem
    {
        private sealed class Choice
        {
            internal CountermeasureRow Row;
            internal InventoryItem Saved;
            internal string Name => Row?.Name ?? (Saved == null ? "Keine Gegenmaßnahme" : SavedName(Saved));
            internal bool Present => Row?.Owned ?? (Saved != null);
            internal int WeaponId => Row?.WeaponId ?? (Saved?.Type == 5 ? Saved.Id : -1);
        }

        private readonly Choice[] choices;
        private int selectedIndex;

        internal CountermeasureGroup(SaveGame game)
        {
            Rows = SaveGame.CountermeasureTypes.Select((item, index) => new CountermeasureRow(game, item, index)).ToArray();
            var other = game.CountermeasureItems.Where(item => item.Type != 5 || !Rows.Any(row => row.WeaponId == item.Id && row.CanEdit)).ToArray();
            choices = new[] { new Choice() }.Concat(Rows.Where(row => row.CanEdit).Select(row => new Choice { Row = row }))
                .Concat(other.Select(item => new Choice { Saved = item })).ToArray();
            Choices = choices.Select(choice => choice.Name).ToArray();
            ExtraEntries = other.Select(item => $"{SavedName(item)} × {item.Count}").ToArray();

            var active = game.ActiveCountermeasure;
            int indexOfActive = Array.FindIndex(choices, choice => active != null &&
                (ReferenceEquals(choice.Saved, active) || active.Type == 5 && choice.Row?.WeaponId == active.Id));
            selectedIndex = Math.Max(0, indexOfActive);
        }

        public CountermeasureRow[] Rows { get; }
        public string[] Choices { get; }
        public string[] ExtraEntries { get; }
        public string ActiveLabel => "Aktive Gegenmaßnahme";
        public string NameHeading => "Gegenmaßnahme";
        public string CountHeading => "Menge";
        public string LimitHeading => "Spiellimit";
        public string MaxPresentLabel => "Vorhandene: Spiellimit";
        public Visibility StorageMaximumVisibility => Visibility.Collapsed;
        public string Hint => "Die Limits gelten für alle Schiffe. Weitere gespeicherte Gegenmaßnahmen bleiben im Bestand.";

        public int SelectedIndex
        {
            get => selectedIndex;
            private set => SetField(ref selectedIndex, value);
        }

        internal int ActiveWeaponId => choices[SelectedIndex].WeaponId;

        internal void Select(int index)
        {
            if (index == 0)
            {
                SetOwned(false);
                return;
            }

            var row = choices[index].Row;
            if (row != null)
                row.Owned = true;
            SelectedIndex = index;
        }

        internal void SelectPresent()
        {
            if (choices[SelectedIndex].Present)
                return;

            SelectedIndex = Math.Max(0, Array.FindIndex(choices, choice => choice.Present));
        }

        internal void SetOwned(bool value)
        {
            foreach (var row in Rows.Where(row => row.CanToggle))
            {
                if (value)
                    row.Owned = true;
                else
                    row.Ammo = "0";
            }
            SelectPresent();
        }

        private static string SavedName(InventoryItem item) => string.IsNullOrEmpty(item.Name) ? $"ID {item.Id}" : item.Name;
    }
}
