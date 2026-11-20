// SPDX-License-Identifier: GPL-3.0-or-later
using System;
using System.Collections.Generic;
using System.Text;

using static D3SaveEditor.Core.CoreText;

namespace D3SaveEditor.Core {
    public sealed class GuideBotData {
        // AIGame.cpp: the fixed-size guidebot_data has no pointers in either supported ABI.
        private const int Size = 992, NameSize = 8, Powerups = 8, OwnerSlot = 40, Owner = 44, Flags = 96, Handle = 108;
        private const uint NoChatter = 8;
        public const int PowerupCount = 5, MaximumCount = 127;

        private readonly int offset;
        private readonly uint flags;
        public string Name { get; }
        public float Shields { get; }
        public bool Chatter { get { return (flags & NoChatter) == 0; } }
        public IReadOnlyList<int> Counts { get; }

        private GuideBotData(byte[] data, int offset, float shields) {
            this.offset = offset;
            flags = SaveGame.U32(data, offset + Flags);
            Name = SaveGame.Fixed(data, offset, NameSize);
            Shields = shields;
            var counts = new int[PowerupCount];
            for (int i = 0; i < counts.Length; i++) counts[i] = data[offset + Powerups + i];
            Counts = Array.AsReadOnly(counts);
        }

        internal static GuideBotData Find(SaveGame game) {
            int normal = game.FindEquipment("GuideBot"), red = game.FindEquipment("GuideBotRed");
            if (normal < 0 && red < 0) return null;

            // Matcens and legacy visual effects precede OSIRIS. Use its length-delimited
            // v1 directory and require a chunk tied to the actual saved robot and player.
            var bytes = game.Bytes;
            var signature = new byte[] { (byte)'O', (byte)'S', (byte)'I', (byte)'R', (byte)'I', (byte)'S', 0, 1 };
            GuideBotData found = null;
            int candidates = 0;
            for (int pos = game.CounterMeasures.End; pos <= bytes.Length - 12; pos++) {
                if (bytes[pos] != signature[0] || !Matches(bytes, pos, signature)) continue;
                if (++candidates > 16) return null;
                GuideBotData candidate;
                try { candidate = ReadState(game, pos, normal, red); }
                catch (FormatProblem) { continue; }
                if (candidate == null) continue;
                if (found != null) return null;
                found = candidate;
            }
            return found;
        }

        private static bool Matches(byte[] bytes, int offset, byte[] signature) {
            for (int i = 1; i < signature.Length; i++) if (bytes[offset + i] != signature[i]) return false;
            return true;
        }

        private static GuideBotData ReadState(SaveGame game, int start, int normal, int red) {
            var bytes = game.Bytes;
            int length = SaveGame.I32(bytes, start + 8);
            if (length < 90 || length > bytes.Length - start) return null;
            var r = new Reader(bytes, start + 12, start + length);
            r.Skip(4); // timer counter
            for (int i = 0; i < 64; i++) if (r.Bool()) r.Skip(34);

            int candidate = -1, handle = 0;
            for (int size; (size = r.I32()) != 0;) {
                int kind = r.U8();
                if (kind > 2) return null;
                int ownerHandle = kind == 2 ? 0 : r.I32();
                int id = r.I32(), body = r.Pos;
                r.Skip(size);
                if (kind != 0 || id != 4 || size != Size || SaveGame.I16(bytes, body + OwnerSlot) != 0 ||
                    SaveGame.I32(bytes, body + Owner) != game.PlayerObject.Handle || SaveGame.I32(bytes, body + Handle) != ownerHandle)
                    continue;
                if (candidate >= 0) return null;
                candidate = body;
                handle = ownerHandle;
            }
            if (candidate < 0 || Array.IndexOf(bytes, (byte)0, candidate, NameSize) < 0) return null;
            for (int i = 0; i < PowerupCount; i++) if (bytes[candidate + Powerups + i] > MaximumCount) return null;

            int modules = r.U8();
            if (modules > 64) return null;
            bool aiGame = false;
            for (int i = 0; i < modules; i++) {
                int size = r.U8(), nameStart = r.Pos;
                if (size < 2) return null;
                string name = r.Z(size);
                if (r.Pos != nameStart + size) return null;
                r.Skip(r.I32());
                aiGame |= name.Equals("AIGame", StringComparison.OrdinalIgnoreCase) ||
                    name.Equals("AIGame.dll", StringComparison.OrdinalIgnoreCase) || name.Equals("AIGame.so", StringComparison.OrdinalIgnoreCase);
            }
            r.Need(5); // OMMS header and terminator
            if (!aiGame) return null;
            ObjectBlock obj = FindObject(game, handle);
            if (obj == null || obj.Id < 0 || (obj.Id != normal && obj.Id != red) ||
                (obj.Type != 2 && !(obj.Type == 21 && obj.DummyType == 2))) return null;
            return new GuideBotData(bytes, candidate, SaveGame.F32(bytes, obj.ShieldOffset));
        }

        private static ObjectBlock FindObject(SaveGame game, int handle) {
            int slot = handle & 0x7ff, playerSlot = game.PlayerObject.Handle & 0x7ff;
            if (slot >= 1500 || slot == playerSlot) return null;
            byte[] bytes = game.Bytes;
            if (slot > playerSlot) {
                int pos = game.PlayerObject.End;
                for (int i = playerSlot + 1; i <= slot && pos < game.PlayerOffset - 2; i++) {
                    var obj = ObjectBlock.Read(bytes, pos, game.Version);
                    if (i == slot) return obj.Handle == handle ? obj : null;
                    pos = obj.End;
                }
                return null;
            }

            // For earlier slots, accept only a robot record that joins the known player sequence.
            ObjectBlock found = null;
            for (int p = game.MapOffset + game.MapCount; p <= game.PlayerObject.Start - 11; p++) {
                if (bytes[p] != 11 || SaveGame.U32(bytes, p) != 0x00BADB0B || (bytes[p+4] != 2 && bytes[p+4] != 21)) continue;
                int h = p + 7 + (bytes[p+6] == 1 ? 52 : 0);
                if (h > game.PlayerObject.Start - 4 || SaveGame.I32(bytes, h) != handle) continue;
                var obj = ObjectBlock.Read(bytes, p, game.Version);
                int end = obj.End;
                for (int i = slot + 1; i < playerSlot; i++) {
                    if (end >= game.PlayerObject.Start) { end = -1; break; }
                    var next = ObjectBlock.Read(bytes, end, game.Version);
                    if (next.Type != 255 && (next.Handle & 0x7ff) != i) { end = -1; break; }
                    end = next.End;
                }
                if (end != game.PlayerObject.Start) continue;
                if (found != null) return null;
                found = obj;
            }
            return found;
        }

        internal void Edit(EditOptions edit, byte[] original, List<Patch> patches, EditResult result) {
            var values = new byte[NameSize + PowerupCount];
            Buffer.BlockCopy(original, offset, values, 0, values.Length);
            if (edit.GuideBotName != null) {
                byte[] name = SaveGame.Ansi.GetBytes(edit.GuideBotName);
                if (name.Length == 0 || name.Length >= NameSize || SaveGame.Ansi.GetString(name) != edit.GuideBotName ||
                    Array.Exists(name, ch => ch < 32 || ch == 127))
                    throw new FormatProblem(Text("GuideBotName"));
                Array.Clear(values, 0, NameSize);
                Buffer.BlockCopy(name, 0, values, 0, name.Length);
            }
            if (edit.GuideBotCounts != null) {
                if (edit.GuideBotCounts.Length != PowerupCount) throw new ArgumentException(Text("GuideBotCounts"));
                for (int i = 0; i < PowerupCount; i++) {
                    int count = edit.GuideBotCounts[i];
                    if (count < 0 || count > MaximumCount) throw new FormatProblem(Text("GuideBotCountRange"));
                    values[Powerups + i] = (byte)count;
                }
            }
            bool changed = !SaveGame.SliceEqual(original, offset, values.Length, values);
            if (changed) patches.Add(new Patch(offset, values.Length, values));
            if (edit.GuideBotChatter.HasValue && edit.GuideBotChatter.Value != Chatter) {
                uint updated = edit.GuideBotChatter.Value ? flags & ~NoChatter : flags | NoChatter;
                patches.Add(new Patch(offset + Flags, 4, BitConverter.GetBytes(updated)));
                changed = true;
            }
            if (changed) result.Changes.Add(Text("GuideBotEdited"));
        }
    }
}
