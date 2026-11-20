// SPDX-License-Identifier: GPL-3.0-or-later
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Windows;
using D3SaveEditor.Core;
using static D3SaveEditor.Gui.LocalizationService;

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
            limit = UsesAmmo ? T("LimitUnavailable") : T("EnergyWeapon");
        }

        public int Index { get; }
        public string Name => SaveGame.WeaponNames[Index];
        public string AmmoLabel => T("AmmoLabel", Name);
        public bool CanToggle => true;
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

        internal void RefreshText() => Notify(nameof(AmmoLabel));
    }

    internal sealed class WeaponGroup : BindableItem
    {
        private int selectedIndex = -1;
        private string hint = string.Empty;

        internal WeaponGroup(int firstIndex, bool hasNoneChoice)
        {
            FirstChoice = hasNoneChoice ? 1 : 0;
            Rows = Enumerable.Range(firstIndex, 10).Select(index => new WeaponRow(index)).ToArray();
        }

        public int FirstChoice { get; }
        public WeaponRow[] Rows { get; }
        public string[] Choices => FirstChoice == 0 ? Rows.Select(row => row.Name).ToArray() :
            new[] { T("NoSecondary") }.Concat(Rows.Select(row => row.Name)).ToArray();
        public string ActiveLabel => T("ActiveWeapon");
        public string NameHeading => T("Weapon");
        public string CountHeading => T("Ammo");
        public string LimitHeading => T("ShipLimit");
        public string MaxPresentLabel => T("FillShipLimit");
        public Visibility StorageMaximumVisibility => Visibility.Visible;
        public string[] ExtraEntries => Array.Empty<string>();

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

        internal void RefreshText()
        {
            Notify(nameof(Choices));
            Notify(nameof(SelectedIndex));
            Notify(nameof(ActiveLabel));
            Notify(nameof(NameHeading));
            Notify(nameof(CountHeading));
            Notify(nameof(LimitHeading));
            Notify(nameof(MaxPresentLabel));
            foreach (var row in Rows)
                row.RefreshText();
        }
    }

    internal class SupplyRow : BindableItem
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
        public string Hint => CanEdit ? T("SupplyRange", Maximum) : T("SupplyMissing");
        internal void RefreshText() => Notify(nameof(Hint));
    }

    internal sealed class ShipRow : BindableItem
    {
        private bool allowed;
        private bool canUnlock;
        private readonly string name;

        internal ShipRow(SaveGame game, string name)
        {
            Defined = game.IsShipDefined(name);
            allowed = game.IsShipAllowed(name);
            canUnlock = Defined && !allowed;
            this.name = name;
        }

        public string Label => Defined ? name : T("Missing", name);
        internal void RefreshText() => Notify(nameof(Label));
        internal bool Defined { get; }
        public bool CanUnlock
        {
            get => canUnlock;
            set => SetField(ref canUnlock, value);
        }
        public bool Allowed
        {
            get => allowed;
            set => SetField(ref allowed, value);
        }
    }

    internal sealed class GuideBotSupplyRow : BindableItem
    {
        private readonly string labelKey;
        private string countText;

        internal GuideBotSupplyRow(string labelKey, int count)
        {
            this.labelKey = labelKey;
            countText = count.ToString(CultureInfo.CurrentCulture);
        }

        public string Name => T(labelKey);
        internal void RefreshText() => Notify(nameof(Name));
        public string CountText
        {
            get => countText;
            set => SetField(ref countText, value);
        }
    }
}
