// SPDX-License-Identifier: GPL-3.0-or-later
using System;
using System.IO;
using System.IO.Compression;
using Microsoft.VisualStudio.TestTools.UnitTesting;

[TestClass]
[DoNotParallelize]
public sealed class EditorTests
{
    [TestMethod]
    [TestCategory("Core")]
    public void SaveGameFormatAndFileOperations()
    {
        WithFixtures(Tests.Run);
    }

    [STATestMethod]
    [TestCategory("WPF")]
    public void EditorChangesAndSavesLoadout()
    {
        WithFixtures(folder => new UiTests().Run(Path.Combine(folder, "fixture32-loadout")));
    }

    private static void WithFixtures(Action<string> run)
    {
        string folder = Path.Combine(Path.GetTempPath(), "D3SaveEditor.Tests-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(folder);
        try
        {
            using (var resource = typeof(EditorTests).Assembly.GetManifestResourceStream("D3SaveEditor.Tests.fixtures.zip"))
            using (var archive = new ZipArchive(resource, ZipArchiveMode.Read))
                archive.ExtractToDirectory(folder);

            run(folder);
        }
        finally
        {
            Directory.Delete(folder, true);
        }
    }
}
