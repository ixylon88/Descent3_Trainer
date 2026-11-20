// SPDX-License-Identifier: GPL-3.0-or-later
using System;
using System.Globalization;
using System.Resources;

[assembly: NeutralResourcesLanguage("en")]

namespace D3SaveEditor.Core {
    internal static class CoreText {
        private static readonly ResourceManager Messages = new ResourceManager(
            "D3SaveEditor.Core.Resources.Messages", typeof(CoreText).Assembly);

        internal static string Text(string key, params object[] arguments) {
            string value = Messages.GetString(key) ?? throw new InvalidOperationException($"Missing core resource: {key}");
            return arguments.Length == 0 ? value : string.Format(CultureInfo.CurrentCulture, value, arguments);
        }
    }
}
