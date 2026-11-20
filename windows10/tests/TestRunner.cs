// SPDX-License-Identifier: GPL-3.0-or-later
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using D3SaveEditor.Core;

namespace D3SaveEditor.Tests
{
    internal static class Tests
    {
        private static int passed;
        private static void Check(bool condition, string label)
        {
            if (!condition) throw new Exception("FAIL: " + label); passed++; Console.WriteLine("PASS: " + label);
        }
        private static void Reject(Action action, string label)
        {
            try { action(); } catch (FormatProblem) { Check(true, label); return; }
            throw new Exception("Not rejected: " + label);
        }
        private static byte[] Changed(byte[] bytes, int offset, byte[] change) { var copy = (byte[])bytes.Clone(); Buffer.BlockCopy(change, 0, copy, offset, change.Length); return copy; }
        private static void Main(string[] args)
        {
            Run(args.Length > 0 ? args[0] : ".");
        }
        internal static void Run(string dir)
        {
            passed = 0;
            foreach (string name in new[] { "fixture32", "fixture64", "fixture32-noeffects", "fixture64-guided", "fixture32-snapshot", "fixture32-v1", "fixture32-v0" })
            {
                byte[] original = File.ReadAllBytes(Path.Combine(dir, name)); var s = SaveGame.Parse(original);
                Check(s.Energy == 100.125f && s.Shields == 87.25f && s.Callsign == "Testpilot", name + ": fields match independent C++ writer");
                Check(s.Layout.Size == (name.StartsWith("fixture64") ? 536 : 496), name + ": player ABI");
                Check(s.CurrentOptions().Ammo[10] == 4 && s.HasEquipment("Converter"), name + ": weapon and inventory reads");
                var untouched = s.Edit(s.CurrentOptions()); Check(untouched.Bytes.SequenceEqual(original), name + ": unchanged save is byte-identical");
                var x = s.CurrentOptions(); x.Energy = 200; x.Shields = 200; x.AfterburnerSeconds = 5; x.Lives = 99; x.Score = 123456;
                x.GodMode = true; x.Cloaked = true; x.Cooler = true; x.Converter = false; x.QuadLasers = true; x.RevealMap = true;
                for (int i = 0; i < 20; i++) { x.Owned[i] = true; x.Ammo[i] = (ushort)(i < 10 ? i * 17 : 65535); }
                x.Primary = 9; x.Secondary = 19;
                var edited = s.Edit(x); var again = SaveGame.Parse(edited.Bytes); var read = again.CurrentOptions();
                Check(again.Invulnerable && again.InvulnerableTime == 1e9f && again.IsCloaked && again.CloakTime == 1e9f, name + ": persistent effect flags and timers");
                Check(again.Energy == 200 && again.Shields == 200 && again.Afterburner == 5 && read.Lives == 99 && read.Score == 123456, name + ": player values");
                Check(read.Primary == 9 && read.Secondary == 19 && read.Owned.All(v => v) && read.Ammo.Skip(10).All(v => v == 65535), name + ": all twenty weapons and secondary counts");
                Check(again.HasEquipment("Afterburner") && !again.HasEquipment("Converter") && again.HasEquipment("QuadLaser") && again.HasQuad, name + ": inventory insertion/removal and both Quad batteries");
                Check((SaveGame.U32(again.Bytes, again.PlayerOffset + 168) & 0x80100000) == 0x80100000, name + ": flare and unknown ownership bits preserved");
                Check((SaveGame.U32(again.Bytes, again.PlayerOffset + 84) & 4096) != 0 && SaveGame.I32(again.Bytes, again.PlayerOffset + again.Layout.Permissions) == 7, name + ": unrelated player flags and fields preserved");
                Check(again.Bytes.Skip(again.MapOffset).Take(again.MapCount).All(v => v == 1), name + ": whole automap");
                Check(edited.Bytes.Skip(edited.Bytes.Length - 275).SequenceEqual(original.Skip(original.Length - 275)), name + ": opaque engine tail preserved");
                Check(again.Edit(again.CurrentOptions()).Bytes.SequenceEqual(edited.Bytes), name + ": modified file reopens and round-trips");
                var disabled = again.CurrentOptions(); disabled.GodMode = false; disabled.Cloaked = false; disabled.Cooler = false; disabled.QuadLasers = false;
                var off = again.Edit(disabled).Parsed;
                Check(!off.Invulnerable && !off.IsCloaked && off.InvulnerableTime == 0 && off.CloakTime == 0 && !off.HasQuad && !off.HasEquipment("Afterburner"), name + ": effects and equipment can be disabled");
            }
            var baseBytes = File.ReadAllBytes(Path.Combine(dir, "fixture32")); var valid = SaveGame.Parse(baseBytes);
            var missingShip = valid.CurrentOptions(); missingShip.UnlockBlackPyro = true;
            Reject(() => valid.Edit(missingShip), "Black Pyro unlock requires a saved definition");
            Reject(() => SaveGame.Parse(Changed(baseBytes, 32, BitConverter.GetBytes((short)3))), "unknown version rejected");
            Reject(() => SaveGame.Parse(Changed(baseBytes, valid.PlayerOffset - 2, BitConverter.GetBytes((short)500))), "unknown raw ABI rejected");
            Reject(() => SaveGame.Parse(Changed(baseBytes, valid.PlayerOffset + 136, BitConverter.GetBytes((short)13))), "cross-link mismatch rejected");
            Reject(() => SaveGame.Parse(Changed(baseBytes, valid.PlayerOffset + valid.Layout.Inventory + 4, BitConverter.GetBytes(2))), "raw and serialized inventory mismatch rejected");
            Reject(() => SaveGame.Parse(Changed(baseBytes, valid.MapOffset, new byte[] { 2 })), "unknown map values rejected");
            Reject(() => SaveGame.Parse(baseBytes.Take(valid.PlayerOffset + 300).ToArray()), "truncated player rejected");
            Reject(() => SaveGame.Parse(baseBytes.Concat(baseBytes.Skip(valid.PlayerObject.Start)).ToArray()), "ambiguous valid player records rejected");
            Reject(() => SaveGame.Parse(Changed(baseBytes, valid.PlayerObject.EffectOffset - 2, BitConverter.GetBytes((short)168))), "unknown effect ABI rejected");
            var badOptions = valid.CurrentOptions(); badOptions.Energy = float.NaN; Reject(() => valid.Edit(badOptions), "NaN edits rejected");
            var disarmed = valid.CurrentOptions(); disarmed.Owned[0] = false; Reject(() => valid.Edit(disarmed), "removing final primary rejected");
            var fallback = valid.CurrentOptions(); fallback.Primary = 4; var corrected = valid.Edit(fallback).Parsed;
            Check(corrected.CurrentOptions().Primary == 0, "equipped weapon corrected to owned primary");
            var finite = Changed(baseBytes, valid.PlayerOffset + 84, BitConverter.GetBytes(4097u)); SaveGame.Put(finite, valid.PlayerOffset + 116, 12.5f);
            var finiteGame = SaveGame.Parse(finite); Check(finiteGame.Edit(finiteGame.CurrentOptions()).Bytes.SequenceEqual(finite), "finite power-up timer preserved when untouched");
            var inactive = Changed(baseBytes, valid.PlayerOffset + 168, BitConverter.GetBytes(0x80100001u)); SaveGame.Put(inactive, valid.PlayerOffset + 228, 17);
            var inactiveGame = SaveGame.Parse(inactive); Check(inactiveGame.Edit(inactiveGame.CurrentOptions()).Bytes.SequenceEqual(inactive), "unused active-secondary index preserved in unchanged save");
            var unarmed = Changed(baseBytes, valid.PlayerOffset + 168, BitConverter.GetBytes(0x80100400u)); var unarmedGame = SaveGame.Parse(unarmed);
            Check(unarmedGame.Edit(unarmedGame.CurrentOptions()).Bytes.SequenceEqual(unarmed), "already unarmed source preserves exact unchanged bytes");
            var noDefinition = Changed(baseBytes, FindAscii(baseBytes, "Afterburner"), Encoding.ASCII.GetBytes("Otherburner")); var noDefGame = SaveGame.Parse(noDefinition); var noDefEdit = noDefGame.CurrentOptions(); noDefEdit.Cooler = true;
            Reject(() => noDefGame.Edit(noDefEdit), "missing equipment definition rejected");
            foreach (string name in new[] { "fixture32-loadout", "fixture64-loadout" })
                LoadoutTests(Path.Combine(dir, name));
            foreach (string name in new[] { "fixture32-guidebot", "fixture64-guidebot" })
                GuideBotTests(File.ReadAllBytes(Path.Combine(dir, name)));
            FileTests(dir, baseBytes);
            TableTests(dir);
            LocalizationTest();
            Console.WriteLine("\n" + passed + " assertions passed. No real in-game save/load test has been performed.");
        }
        private static void LocalizationTest()
        {
            var original = CultureInfo.CurrentUICulture;
            try
            {
                foreach (var language in new[] { "de", "en" })
                {
                    CultureInfo.CurrentUICulture = CultureInfo.GetCultureInfo(language);
                    string message = null;
                    try { SaveGame.Parse(new byte[1]); }
                    catch (FormatProblem ex) { message = ex.Message; }
                    Check(message?.StartsWith(language == "de" ? "Keine unterstützte" : "Unsupported", StringComparison.Ordinal) == true,
                        "Parser errors follow the selected interface language");
                }
            }
            finally { CultureInfo.CurrentUICulture = original; }
        }
        private static void GuideBotTests(byte[] bytes)
        {
            var original = SaveGame.Parse(bytes);
            int body = FindAscii(bytes, "Buddy");
            Check(original.GuideBot?.Name == "Buddy" && original.GuideBot.Shields == 20 && original.GuideBot.Chatter &&
                original.GuideBot.Counts.SequenceEqual(new[] { 3, 1, 2, 0, 1 }), "GuideBot memory and owning robot are decoded");
            Check(original.Edit(original.CurrentOptions()).Bytes.SequenceEqual(bytes), "untouched GuideBot is byte-identical");
            var edit = original.CurrentOptions();
            edit.GuideBotName = "Buddy 2";
            edit.GuideBotChatter = false;
            edit.GuideBotCounts = new[] { 0, 127, 3, 4, 5 };
            var saved = original.Edit(edit).Parsed;
            Check(saved.GuideBot?.Name == "Buddy 2" && !saved.GuideBot.Chatter && saved.GuideBot.Counts.SequenceEqual(edit.GuideBotCounts),
                "GuideBot name, chatter and supplies round-trip");
            var expected = Changed(bytes, body, Encoding.ASCII.GetBytes("Buddy 2\0"));
            for (int i = 0; i < edit.GuideBotCounts.Length; i++) expected[body + 8 + i] = (byte)edit.GuideBotCounts[i];
            SaveGame.Put(expected, body + 96, 0x28u);
            Check(saved.Bytes.SequenceEqual(expected), "GuideBot edit preserves the rest of the script state and the save");
            var foreign = SaveGame.Parse(Changed(bytes, body + 44, BitConverter.GetBytes(0x1002)));
            Check(foreign.GuideBot == null, "a GuideBot chunk owned by another player is not offered");
            Reject(() => foreign.Edit(edit), "unrecognized GuideBot cannot be edited");
            edit.GuideBotCounts[0] = 128;
            Reject(() => original.Edit(edit), "GuideBot signed-byte supply limit is enforced");
        }
        private static void LoadoutTests(string path)
        {
            string name = Path.GetFileName(path);
            var original = SaveGame.Parse(File.ReadAllBytes(path));
            var untouched = original.Edit(original.CurrentOptions());
            Check(untouched.Bytes.SequenceEqual(original.Bytes), name + ": unchanged inventories stay byte-identical");
            Check(original.InventoryItems.Count == 3 && original.CountermeasureItems.Count == 3, name + ": both lists are decoded");
            Check(original.IsShipAllowed(SaveGame.BlackPyro) && !original.PrefersBlackPyro, name + ": existing Black Pyro permission is visible");
            var locked = SaveGame.Parse(Changed(original.Bytes, original.PlayerOffset + original.Layout.Permissions, BitConverter.GetBytes(0x40000009u)));
            var unlock = locked.CurrentOptions(); unlock.UnlockBlackPyro = true;
            var unlocked = locked.Edit(unlock).Parsed;
            Check(unlocked.IsShipAllowed(SaveGame.BlackPyro) && unlocked.ShipPermissions == 0x4000000bu,
                name + ": unlocking Black Pyro retains other permissions");
            var prefer = unlocked.CurrentOptions(); prefer.PreferBlackPyro = true;
            var preferred = unlocked.Edit(prefer).Parsed;
            Check(preferred.PrefersBlackPyro && preferred.ShipPermissions == 0x40000002u && preferred.ShipName == original.ShipName,
                name + ": Black Pyro mode clears the other ship bits and preserves the active ship and unrelated bits");
            Check(preferred.Edit(preferred.CurrentOptions()).Bytes.SequenceEqual(preferred.Bytes), name + ": saved Black Pyro mode round-trips");
            var edit = original.CurrentOptions();
            edit.CargoCounts[0] = 4;
            edit.CargoCounts[1] = 1;
            edit.CountermeasureCounts[0] = 8;
            edit.CountermeasureCounts[1] = 0;
            edit.CountermeasureCounts[2] = 12;
            edit.CountermeasureCounts[3] = 4;
            edit.CountermeasureCounts[4] = 5;
            edit.UnlockShips[1] = edit.UnlockShips[2] = true;
            var result = original.Edit(edit);
            var saved = result.Parsed;
            Check(saved.InventoryItems.Count == 4 && saved.CountermeasureItems.Count == 5, name + ": both list counts and raw counts agree");
            Check(saved.ShipName == "Pyro-GL" && saved.ShipPermissions == 0x4000002bu, name + ": ship bits use sparse table positions");
            Check(saved.InventoryItems[1].Count == 4 && saved.InventoryItems[1].Icon == "force.ogf" && saved.InventoryItems[1].InventoryFlags == 0x193,
                name + ": original cargo flags and icon retained");
            Check(saved.InventoryItems[2].Id == -1 && saved.InventoryItems[2].Name == "Mission cargo" && saved.Inventory.Selected == 2,
                name + ": selected mission entry preserved");
            Check(saved.InventoryItems[3].Type == 7 && saved.InventoryItems[3].OriginalId == 5 && saved.InventoryItems[3].Count == 1,
                name + ": new cargo entry has matching object IDs");
            Check(saved.CountermeasureItems[0].Count == 8 && saved.CountermeasureItems[0].Name == "Saved Chaff" && saved.CountermeasureItems[0].Icon == "chaff.ogf" &&
                saved.CountermeasureItems[0].InventoryFlags == 0x187, name + ": existing countermeasure metadata survives a quantity change");
            Check(saved.CounterMeasures.Selected == 0 && saved.CountermeasureItems[1].Name == "Custom countermeasure",
                name + ": removed selection falls back to a valid entry and custom entries survive");
            Check(saved.CountermeasureItems[2].Id == 5 && saved.CountermeasureItems[2].OriginalType == 7 && saved.CountermeasureItems[2].OriginalId == 8 &&
                saved.CountermeasureItems[2].Count == 12 && saved.CountermeasureItems[2].InventoryFlags == 0x87,
                name + ": new countermeasure records encode weapon and pickup IDs");
            Check(saved.CountermeasureItems[3].Count == 4 && saved.CountermeasureItems[4].Count == 5, name + ": Gunboy and Proximity Mine counts");
            Check(result.Bytes.Skip(saved.CounterMeasures.End).SequenceEqual(original.Bytes.Skip(original.CounterMeasures.End)) &&
                SaveGame.I32(result.Bytes, saved.PlayerOffset + saved.Layout.Size) == 0x1001,
                name + ": opaque tail and optional guided projectile handle preserved");
            Check(saved.Edit(saved.CurrentOptions()).Bytes.SequenceEqual(result.Bytes), name + ": edited file reopens and round-trips");
            edit.ActiveCountermeasureWeaponId = original.CountermeasureWeaponId(2);
            Check(original.Edit(edit).Parsed.ActiveCountermeasure.Id == edit.ActiveCountermeasureWeaponId,
                name + ": selection targets a newly added countermeasure after reindexing");
            edit.ActiveCountermeasureWeaponId = original.CountermeasureWeaponId(1);
            Reject(() => original.Edit(edit), name + ": removed countermeasure cannot be selected");
            edit.ActiveCountermeasureWeaponId = null;

            edit.CountermeasureCounts[3] = 5;
            Reject(() => original.Edit(edit), name + ": Gunboy maximum enforced");
            var missing = SaveGame.Parse(Changed(original.Bytes, FindAscii(original.Bytes, "Seeker3Pack"), Encoding.ASCII.GetBytes("MissingPack")));
            Check(!missing.CanEditSupply(SaveGame.CountermeasureTypes[2]), name + ": no new entry when pickup definition is missing");
            edit = missing.CurrentOptions(); edit.CountermeasureCounts[2] = 1;
            Reject(() => missing.Edit(edit), name + ": missing pickup ID is never guessed");
        }
        private static int FindAscii(byte[] bytes, string value)
        {
            byte[] pattern = Encoding.ASCII.GetBytes(value);
            for (int i = 0; i <= bytes.Length - pattern.Length; i++) if (bytes.Skip(i).Take(pattern.Length).SequenceEqual(pattern)) return i;
            throw new Exception("pattern missing");
        }
        private static void FileTests(string dir, byte[] original)
        {
            string root = Path.Combine(dir, "file-tests-" + Guid.NewGuid().ToString("N")); Directory.CreateDirectory(root);
            try
            {
                string source = Path.Combine(root, "saveg000"), copy = Path.Combine(root, "copy"); File.WriteAllBytes(source, original);
                var game = SaveFiles.Open(source); var opts = game.CurrentOptions(); opts.Energy = 200; var result = game.Edit(opts);
                Check(SaveFiles.Write(copy, result, source, game.Hash) == null && File.ReadAllBytes(source).SequenceEqual(original), "copy write preserves source");
                Check(SaveFiles.Open(copy).Energy == 200, "copy reopens");
                string backup = SaveFiles.Write(source, result, source, game.Hash);
                Check(backup != null && File.ReadAllBytes(backup).SequenceEqual(original) && SaveFiles.Open(source).Energy == 200, "atomic replace creates exact backup");
                bool refused = false; try { SaveFiles.Write(source, result, source, game.Hash); } catch (IOException) { refused = true; }
                Check(refused && SaveFiles.Open(source).Energy == 200, "external modifications detected before overwriting");
                var corrupt = new EditResult { Bytes = new byte[] { 1, 2, 3 } }; Reject(() => SaveFiles.Write(copy, corrupt, source, game.Hash), "invalid output never written");
                Check(SaveFiles.Open(copy).Energy == 200 && !Directory.GetFiles(root, "*.tmp").Any(), "existing destination preserved and no temporary leftovers");
            }
            finally { Directory.Delete(root, true); }
        }
        private static void WriteZ(BinaryWriter w, string value) { w.Write(Encoding.ASCII.GetBytes(value)); w.Write((byte)0); }
        private static byte[] ShipTable(int version)
        {
            using (var pageStream = new MemoryStream()) using (var page = new BinaryWriter(pageStream))
            {
                page.Write((short)version); WriteZ(page, "Pyro-GL"); for (int i = 0; i < 6; i++) WriteZ(page, ""); page.Write(new byte[8 + 68 + 12]);
                for (int i = 0; i < 21; i++)
                {
                    page.Write((byte)0); for (int j = 0; j < 3; j++) WriteZ(page, ""); page.Write(i * 7); page.Write(new byte[version >= 6 ? 211 : 210]);
                    for (int j = 0; j < 16; j++) WriteZ(page, "");
                }
                using (var stream = new MemoryStream()) using (var writer = new BinaryWriter(stream))
                {
                    writer.Write((byte)6); writer.Write((int)pageStream.Length + 4); writer.Write(pageStream.ToArray()); return stream.ToArray();
                }
            }
        }
        private static void TableTests(string dir)
        {
            foreach (int version in new[] { 5, 6 })
            {
                var limits = new ShipLimits(); limits.ReadTable(ShipTable(version)); var caps = limits.ForShip("pyro-gl");
                Check(caps != null && caps[10] == 70 && caps[19] == 133, "ship table v" + version + ": limits decoded");
            }
            var bad = ShipTable(6); SaveGame.Put(bad, 1, int.MaxValue); Reject(() => new ShipLimits().ReadTable(bad), "table length bounds checked");
            string path = Path.Combine(dir, "test-limits.hog");
            try
            {
                var table = ShipTable(6);
                using (var stream = File.Create(path)) using (var w = new BinaryWriter(stream))
                {
                    w.Write(Encoding.ASCII.GetBytes("HOG2")); w.Write(1u); w.Write(116u); w.Write(new byte[56]);
                    var name = new byte[36]; Encoding.ASCII.GetBytes("Table.gam").CopyTo(name, 0); w.Write(name); w.Write(0u); w.Write((uint)table.Length); w.Write(0u); w.Write(table);
                }
                var limits = new ShipLimits(); limits.Load(path); Check(limits.ForShip("Pyro-GL")[19] == 133, "HOG2 archive loads ship table without loading whole archive");
            }
            finally { if (File.Exists(path)) File.Delete(path); }
        }
    }
}
