// SPDX-License-Identifier: GPL-3.0-or-later
using System;
using System.ComponentModel;
using System.Globalization;
using System.Resources;
using System.Windows.Data;

namespace D3SaveEditor.Gui
{
    public sealed class LocalizationService : INotifyPropertyChanged
    {
        private static readonly ResourceManager Strings = new ResourceManager(
            "D3SaveEditor.Gui.Resources.UiStrings", typeof(LocalizationService).Assembly);

        public event PropertyChangedEventHandler PropertyChanged;

        public string this[string key] => T(key);

        internal static bool IsEnglish => CultureInfo.CurrentUICulture.TwoLetterISOLanguageName == "en";

        internal void ChangeLanguage(string code)
        {
            var culture = CultureInfo.GetCultureInfo(code);
            if (culture.Equals(CultureInfo.CurrentUICulture))
                return;

            CultureInfo.CurrentUICulture = culture;
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(Binding.IndexerName));
        }

        internal static string T(string key, params object[] arguments)
        {
            string value = Strings.GetString(key) ?? throw new InvalidOperationException($"Missing UI resource: {key}");
            return arguments.Length == 0 ? value : string.Format(CultureInfo.CurrentCulture, value, arguments);
        }
    }
}
