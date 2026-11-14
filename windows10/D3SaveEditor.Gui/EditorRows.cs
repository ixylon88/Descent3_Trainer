// SPDX-License-Identifier: GPL-3.0-or-later
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using System.Runtime.CompilerServices;
using D3SaveEditor.Core;

namespace D3SaveEditor.Gui
{
    internal abstract class BindableItem : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        protected bool SetField<T>(ref T field, T value, [CallerMemberName] string name = null)
        {
            if (EqualityComparer<T>.Default.Equals(field, value))
                return false;

            field = value;
            Notify(name);
            return true;
        }

        protected void Notify(string name)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }

    internal sealed class WeaponRow : BindableItem
    {
        private bool owned;
        private string ammo = "0";
        private string limit;

        internal WeaponRow(int index)
        {
            Index = index;
            limit = UsesAmmo ? "nicht geladen" : "Energiewaffe";
        }

        public int Index { get; }
        public string Name => SaveGame.WeaponNames[Index];
        public string AmmoLabel => "Munition " + Name;
        public bool UsesAmmo => SaveGame.UsesAmmo(Index);
        public bool CanEditAmmo => Owned && UsesAmmo;

        public bool Owned
        {
            get => owned;
            set
            {
                if (SetField(ref owned, value))
                    Notify(nameof(CanEditAmmo));
            }
        }

        public string Ammo
        {
            get => ammo;
            set => SetField(ref ammo, value);
        }

        public string Limit
        {
            get => limit;
            set => SetField(ref limit, value);
        }
    }

    internal sealed class WeaponGroup : BindableItem
    {
        private int selectedIndex = -1;
        private string hint = string.Empty;

        internal WeaponGroup(int firstIndex, bool hasNoneChoice)
        {
            FirstChoice = hasNoneChoice ? 1 : 0;
            Rows = Enumerable.Range(firstIndex, 10).Select(index => new WeaponRow(index)).ToArray();
            var choices = Rows.Select(row => row.Name);
            Choices = hasNoneChoice ? new[] { "Keine Sekundärwaffe" }.Concat(choices).ToArray() : choices.ToArray();
        }

        public int FirstChoice { get; }
        public WeaponRow[] Rows { get; }
        public string[] Choices { get; }

        public int SelectedIndex
        {
            get => selectedIndex;
            set => SetField(ref selectedIndex, value);
        }

        public string Hint
        {
            get => hint;
            set => SetField(ref hint, value);
        }

        internal void SelectOwnedWeapon()
        {
            int current = SelectedIndex - FirstChoice;
            if (current >= 0 && Rows[current].Owned)
                return;

            int firstOwned = Array.FindIndex(Rows, row => row.Owned);
            SelectedIndex = firstOwned < 0 ? FirstChoice - 1 : FirstChoice + firstOwned;
        }
    }

    internal sealed class SupplyRow : BindableItem
    {
        private string countText;

        internal SupplyRow(SaveGame game, SupplyItem item)
        {
            Item = item;
            OriginalCount = game.SupplyCount(item);
            Maximum = Math.Max(OriginalCount, item.Maximum);
            CanEdit = game.CanEditSupply(item);
            countText = OriginalCount.ToString(CultureInfo.CurrentCulture);
        }

        public SupplyItem Item { get; }
        public int OriginalCount { get; }
        public int Maximum { get; }
        public bool CanEdit { get; }
        public string CountText
        {
            get => countText;
            set => SetField(ref countText, value);
        }
        public string Hint => CanEdit ? $"Menge von 0 bis {Maximum:N0}" : "Die passende Definition fehlt in diesem Spielstand.";
    }

    internal sealed class ShipRow : BindableItem
    {
        private bool allowed;

        internal ShipRow(SaveGame game, string name)
        {
            bool defined = game.IsShipDefined(name);
            allowed = game.IsShipAllowed(name);
            CanUnlock = defined && !allowed;
            Label = defined ? name : name + " (fehlt)";
        }

        public string Label { get; }
        public bool CanUnlock { get; }
        public bool Allowed
        {
            get => allowed;
            set => SetField(ref allowed, value);
        }
    }
}
