// SPDX-License-Identifier: GPL-3.0-or-later
// Format research: DescentDevelopers/Descent3, commit 76049bb32d0a391ab35aedcd0716073c1e64d72a.
// The game format and declarations are copyright (C) 2024 Parallax Software.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;

namespace D3SaveEditor {
    public sealed class FormatProblem : Exception {
        public FormatProblem(string message) : base(message) { }
    }

    // All serialization primitives in gamesave.cpp use little endian. Raw player
    // structures are accepted only for the two explicitly described Windows ABIs.
    internal sealed class Reader {
        internal readonly byte[] Data;
        internal int Pos;
        internal readonly int End;
        internal Reader(byte[] data, int pos = 0, int end = -1) {
            Data = data; Pos = pos; End = end < 0 ? data.Length : end;
            Need(0);
        }
        internal void Need(int count) {
            if (count < 0 || Pos < 0 || Pos > End || count > End - Pos)
                throw new FormatProblem("Die Datei ist abgeschnitten oder eine Blocklänge ist ungültig.");
        }
        internal void Skip(int count) { Need(count); Pos += count; }
        internal byte U8() { Need(1); return Data[Pos++]; }
        internal ushort U16() { Need(2); ushort x = BitConverter.ToUInt16(Data, Pos); Pos += 2; return x; }
        internal short I16() { return unchecked((short)U16()); }
        internal int I32() { Need(4); int x = BitConverter.ToInt32(Data, Pos); Pos += 4; return x; }
        internal uint U32() { return unchecked((uint)I32()); }
        internal float F32() { Need(4); float x = BitConverter.ToSingle(Data, Pos); Pos += 4; return x; }
        internal string Z(int max = 512) {
            int start = Pos;
            for (int i = 0; i < max; i++) {
                if (U8() == 0) return SaveGame.Ansi.GetString(Data, start, Pos - start - 1);
            }
            throw new FormatProblem("Eine Zeichenkette hat keinen gültigen Abschluss.");
        }
        internal bool Bool() {
            byte x = U8(); if (x > 1) throw new FormatProblem("Ungültiger Schalter im Speicherformat."); return x == 1;
        }
        internal void Sized(int max = 32767) {
            int size = I16(); if (size < 0 || size > max) throw new FormatProblem("Ungültige Strukturgröße."); Skip(size);
        }
    }

    public sealed class PlayerLayout {
        public readonly int Size, PointerBytes, Inventory, CounterMeasures, Afterburner, GuidedPointer, Scalars, Permissions;
        internal PlayerLayout(int size, int ptr, int inv, int cm, int ab, int guided, int scalars, int perms) {
            Size=size; PointerBytes=ptr; Inventory=inv; CounterMeasures=cm; Afterburner=ab; GuidedPointer=guided; Scalars=scalars; Permissions=perms;
        }
        internal static readonly PlayerLayout[] Known = {
            new PlayerLayout(496,4,308,320,348,420,432,468),
            new PlayerLayout(536,8,312,336,376,448,468,504)
        };
        public string Name { get { return PointerBytes == 4 ? "32-Bit / 496 Byte" : "64-Bit / 536 Byte"; } }
    }

    public sealed class InventoryItem {
        public int Type, OriginalType, Id, OriginalId, Flags, Count, InventoryFlags;
        public string Description, Icon, Name;
        internal InventoryItem Clone() { return (InventoryItem)MemberwiseClone(); }
    }
    internal sealed class InventoryBlock {
        internal int Start, End, Selected;
        internal List<InventoryItem> Items = new List<InventoryItem>();
        internal static InventoryBlock Read(Reader r) {
            var result = new InventoryBlock { Start = r.Pos };
            int n = r.I32();
            if (n < 0 || n > 10) throw new FormatProblem("Das Inventar enthält eine unbekannte Anzahl von Einträgen.");
            for (int i=0; i<n; i++) {
                var item = new InventoryItem();
                item.Type=r.I32(); item.OriginalType=r.I32(); item.Id=r.I32(); item.OriginalId=r.I32();
                item.Flags=r.I32(); item.Count=r.I32();
                if (item.Count < 1 || item.Count > 1000000) throw new FormatProblem("Ungültige Inventar-Stückzahl.");
                item.Description=r.Z(); item.Icon=r.Z(); item.Name=r.Z(); item.InventoryFlags=r.I32();
                result.Items.Add(item);
            }
            result.Selected=r.I32();
            if (result.Selected < 0 || result.Selected >= Math.Max(n,1)) throw new FormatProblem("Ungültige Inventarauswahl.");
            result.End=r.Pos; return result;
        }
        internal byte[] Serialize() {
            using (var stream = new MemoryStream()) using (var w = new BinaryWriter(stream, SaveGame.Ansi)) {
                w.Write(Items.Count);
                foreach (var x in Items) {
                    w.Write(x.Type); w.Write(x.OriginalType); w.Write(x.Id); w.Write(x.OriginalId); w.Write(x.Flags); w.Write(x.Count);
                    WriteZ(w,x.Description); WriteZ(w,x.Icon); WriteZ(w,x.Name); w.Write(x.InventoryFlags);
                }
                w.Write(Selected); return stream.ToArray();
            }
        }
        private static void WriteZ(BinaryWriter w, string s) { w.Write(SaveGame.Ansi.GetBytes(s ?? "")); w.Write((byte)0); }
        internal InventoryBlock Copy() {
            return new InventoryBlock { Start=Start, End=End, Selected=Selected, Items=Items.Select(x=>x.Clone()).ToList() };
        }
    }

    internal sealed class ObjectBlock {
        internal int Start, End, Handle, Id, ShieldOffset, EffectRecord, EffectOffset = -1, EffectSize, BatteryStart, BatteryCount;
        internal byte Type;
        internal static ObjectBlock Read(byte[] data, int pos, int version) {
            var r = new Reader(data,pos);
            var x = new ObjectBlock { Start=pos };
            if (r.U32() != 0x00BADB0B) throw new FormatProblem("Objektmarkierung fehlt.");
            x.Type=r.U8();
            if (x.Type == 255) { x.End=r.Pos; return x; }
            if (x.Type > 25) throw new FormatProblem("Unbekannter Objekttyp.");
            r.U8(); // lighting_render_type
            if (r.Bool()) r.Skip(52); // sizeof(light_info), no pointers
            x.Handle=r.I32(); r.U8(); r.Skip(4+12+12+36); // dummy type, room, pos, last_pos, orient
            r.Skip(r.U8());
            x.Id=r.I16(); r.Skip(4); // object flags
            r.U8(); r.U8(); int renderType=r.U8();
            r.Skip(2+4); x.ShieldOffset=r.Pos; float shields=r.F32();
            if (!SaveGame.Finite(shields)) throw new FormatProblem("Ungültiger Schildwert.");
            r.Skip(3+12+4+8); // contents, creation/life times, parent, attach handles
            int nattach=r.I32();
            if (nattach<0 || nattach>1024) throw new FormatProblem("Unbekannte Anzahl von Anheftpunkten.");
            if (nattach>0) {
                int allocated=version>=2 ? r.I32() : 1;
                if (allocated!=0 && allocated!=1) throw new FormatProblem("Ungültige Anheftpunkt-Struktur.");
                if (allocated==1) r.Skip(4*nattach);
            }
            r.Skip(1+2+4+24+20);
            r.Skip(r.U8()); r.Skip(r.U8()); // custom script and module names
            r.Skip(2); r.Sized(); r.Sized(); // position counter, movement, control union
            if (r.Bool()) r.Sized(); // optional AI structure
            r.Sized(); r.Skip(4); // render union, size again
            if (renderType==1) r.Skip(24); // custom_anim
            int nb=r.U8();
            if (nb>21) throw new FormatProblem("Unbekannte Waffenbatterie-Struktur.");
            x.BatteryCount=nb; x.BatteryStart=r.Pos; r.Skip(nb*108);
            x.EffectRecord=r.Pos;
            if (r.Bool()) {
                int size=r.I16();
                if (size!=164) throw new FormatProblem("Unbekannte Effektstruktur. Dieses Spiel-Build wird noch nicht unterstützt.");
                x.EffectSize=size; x.EffectOffset=r.Pos; r.Skip(size);
            }
            x.End=r.Pos; return x;
        }
    }

    public sealed class EditOptions {
        public float Energy, Shields, AfterburnerSeconds;
        public byte Lives;
        public int Score, Primary, Secondary;
        public bool[] Owned = new bool[20];
        public ushort[] Ammo = new ushort[20];
        // null means keep the saved effect, including its finite duration.
        public bool? GodMode, Cloaked, Cooler, Converter, QuadLasers;
        public bool RevealMap;
    }
    internal sealed class Patch {
        internal int Start, Length;
        internal byte[] Data;
        internal Patch(int start,int length,byte[] data) { Start=start; Length=length; Data=data; }
    }
    public sealed class EditResult {
        public byte[] Bytes;
        public List<string> Changes = new List<string>();
        public SaveGame Parsed;
    }

    public sealed class SaveGame {
        internal static readonly Encoding Ansi=Encoding.GetEncoding(1252);
        public static readonly string[] WeaponNames = {
            "Laser", "Vauss", "Microwave", "Plasma", "Fusion", "Super Laser", "Mass Driver", "Napalm", "EMD Gun", "Omega",
            "Concussion", "Homing", "Impact Mortar", "Smart", "Mega", "Frag", "Guided", "Napalm Rocket", "Cyclone", "Black Shark"
        };
        public byte[] Bytes { get; private set; }
        public string Description { get; private set; }
        public string Mission { get; private set; }
        public int Level { get; private set; }
        public int Version { get; private set; }
        public int MapOffset { get; private set; }
        public int MapCount { get; private set; }
        public int HighestRoom { get; private set; }
        public int PlayerOffset { get; private set; }
        public PlayerLayout Layout { get; private set; }
        internal ObjectBlock PlayerObject;
        internal InventoryBlock Inventory, CounterMeasures;
        public string[][] Tables { get; private set; }
        public string Callsign { get { return Fixed(Bytes,PlayerOffset+64,20); } }
        public string ShipName {
            get { int i=I32(Bytes,PlayerOffset+60); return i>=0 && i<Tables[3].Length ? Tables[3][i] : "Unbekannt"; }
        }
        public float Energy { get { return F32(Bytes,PlayerOffset+104); } }
        public float Shields { get { return F32(Bytes,PlayerObject.ShieldOffset); } }
        public float Afterburner { get { return F32(Bytes,PlayerOffset+Layout.Afterburner); } }
        public bool Invulnerable { get { return (U32(Bytes,PlayerOffset+84)&1)!=0; } }
        public float InvulnerableTime { get { return F32(Bytes,PlayerOffset+116); } }
        public bool IsCloaked { get { return PlayerObject.EffectOffset>=0 && (U32(Bytes,PlayerObject.EffectOffset)&(1u|1024u))!=0; } }
        public float CloakTime { get { return PlayerObject.EffectOffset<0 ? 0 : F32(Bytes,PlayerObject.EffectOffset+12); } }
        public int RevealedRooms { get { int n=0; for(int i=0;i<=HighestRoom;i++) if(Bytes[MapOffset+i]!=0)n++; return n; } }
        public bool HasQuad { get { return PlayerObject.BatteryCount==21 && (U32(Bytes,PlayerObject.BatteryStart+104)&16)!=0; } }
        public bool SupportsQuad { get { return PlayerObject.BatteryCount==21; } }
        public IList<InventoryItem> InventoryItems { get { return Inventory.Items.AsReadOnly(); } }
        public string Hash { get { return HashBytes(Bytes); } }

        public static SaveGame Parse(byte[] bytes) {
            if (!BitConverter.IsLittleEndian) throw new FormatProblem("Der Editor benötigt ein Little-Endian-System.");
            if (bytes==null || bytes.Length<600 || bytes.Length>256*1024*1024)
                throw new FormatProblem("Keine unterstützte Descent-3-Spielstandsdatei (600 Byte bis 256 MiB).");
            var s=new SaveGame { Bytes=(byte[])bytes.Clone() };
            s.ParseHeader();
            var matches=new List<SaveGame>();
            // Rooms contain portal flags without saved portal counts. Without the
            // mission geometry we must not guess our way through that section.
            // Instead anchor to an exact known player ABI, validate both inventory
            // streams, and cross-link to its parsed OBJ_PLAYER record and all
            // subsequent object records up to the player prefix.
            var objects=new List<ObjectBlock>();
            for (int p=s.MapOffset+s.MapCount; p+7<bytes.Length; p++) {
                if (bytes[p]!=11 || bytes[p+1]!=219 || bytes[p+2]!=186 || bytes[p+3]!=0 || bytes[p+4]!=4) continue;
                try {
                    var obj=ObjectBlock.Read(bytes,p,s.Version);
                    if (obj.Id==0 && (obj.Handle&0x7ff)<1500 && (obj.BatteryCount==0 || obj.BatteryCount==21)) objects.Add(obj);
                } catch (FormatProblem) { }
            }
            if (objects.Count>64) throw new FormatProblem("Mehrdeutige Spielerobjekte. Die Datei wird nicht bearbeitet.");
            foreach (var obj in objects) {
                int end=obj.End, expected=(obj.Handle&0x7ff)+1;
                try {
                    while (end+5<bytes.Length && U32(bytes,end)==0x00BADB0B && expected<1500) {
                        var next=ObjectBlock.Read(bytes,end,s.Version);
                        if(next.Type!=255 && (next.Handle&0x7ff)!=expected) throw new FormatProblem("Ungültige Objektfolge.");
                        end=next.End; expected++;
                    }
                    foreach(var layout in PlayerLayout.Known) {
                        if(end+2+layout.Size>bytes.Length || I16(bytes,end)!=layout.Size) continue;
                        var copy=(SaveGame)s.MemberwiseClone();
                        copy.PlayerObject=obj; copy.PlayerOffset=end+2; copy.Layout=layout;
                        try { copy.ValidatePlayer(); matches.Add(copy); } catch(FormatProblem) { }
                    }
                } catch(FormatProblem) { }
            }
            if(matches.Count!=1) throw new FormatProblem(matches.Count==0
                ? "Kein eindeutig verknüpfter Spielerblock gefunden. Unterstützt: Speicherformat 0–2, Windows-Strukturen mit 496 oder 536 Byte. Andere Builds und beschädigte Dateien werden abgelehnt."
                : "Mehrere plausible Spielerblöcke gefunden. Die Datei wird nicht bearbeitet.");
            return matches[0];
        }

        private void ParseHeader() {
            var r=new Reader(Bytes);
            if (Array.IndexOf(Bytes,(byte)0,0,32)<0) throw new FormatProblem("Die Spielstandsbeschreibung ist ungültig.");
            Description=Fixed(Bytes,0,32); r.Skip(32); Version=r.I16();
            if(Version<0 || Version>2) throw new FormatProblem("Unbekannte Spielstandsversion: "+Version+". Unterstützt werden 0 bis 2.");
            if(r.Bool()) SkipSnapshot(r);
            Tables=new string[6][];
            for(int table=0;table<6;table++) {
                int n=r.I16(); if(n<0 || n>16384) throw new FormatProblem("Unbekannte Übersetzungstabelle.");
                var list=new string[n]; for(int i=0;i<n;i++) list[i]=r.Z(); Tables[table]=list;
            }
            for(int i=0;;i++) {
                if(i>16384) throw new FormatProblem("Bitmap-Tabelle ist nicht abgeschlossen.");
                int index=r.I16(); string name=r.Z(); if(index==-1) { if(name.Length!=0)throw new FormatProblem("Ungültiges Tabellenende."); break; }
                if(index<0) throw new FormatProblem("Ungültiger Bitmap-Index.");
            }
            Level=r.I16(); Mission=r.Z();
            if(Mission.Length==0 || Level<0 || Level>10000) throw new FormatProblem("Ungültige Missionsangaben.");
            r.Skip(4); float gameTime=r.F32(); if(!Finite(gameTime))throw new FormatProblem("Ungültige Spielzeit.");
            r.Skip(4+4+2);
            int weather=r.I32(); if(weather<0 || weather>65536)throw new FormatProblem("Ungültiger Wetterblock."); r.Skip(weather);
            int maxDoors=r.I32(),activeDoors=r.I32();
            if(maxDoors<0 || maxDoors>65536 || activeDoors<0 || activeDoors>maxDoors)throw new FormatProblem("Ungültiger Türblock.");
            r.Skip(maxDoors*4);
            HighestRoom=r.I16(); MapCount=r.I16();
            if(MapCount<1 || MapCount>16384 || HighestRoom< -1 || HighestRoom>=MapCount)throw new FormatProblem("Ungültiger Kartenblock.");
            MapOffset=r.Pos; r.Need(MapCount);
            for(int i=0;i<MapCount;i++) if(r.U8()>1)throw new FormatProblem("Unbekanntes Kartenformat.");
        }
        private static void SkipSnapshot(Reader r) {
            int idLen=r.U8(), cmap=r.U8(), type=r.U8();
            if(cmap!=0 || (type!=121 && type!=122 && type!=123))throw new FormatProblem("Unbekanntes Vorschaubildformat.");
            r.Z(); int mips=r.U8(); r.Skip(9); int width=r.U16(),height=r.U16(); r.Skip(2); r.Skip(idLen);
            if(width<1 || width>8192 || height<1 || height>8192 || mips<1 || mips>16)throw new FormatProblem("Ungültiges Vorschaubild.");
            for(int m=0;m<mips;m++) {
                int total=Math.Max(1,width>>m)*Math.Max(1,height>>m), pixels=0;
                while(pixels<total) {
                    int command=r.U8();
                    if(command==1 || command>250)throw new FormatProblem("Ungültige Bildkompression.");
                    int count=command==0 ? 1 : command;
                    if(count>total-pixels)throw new FormatProblem("Vorschaubild überschreitet seine Größe.");
                    r.Skip(2); pixels+=count;
                }
            }
        }
        private void ValidatePlayer() {
            int p=PlayerOffset;
            if(Array.IndexOf(Bytes,(byte)0,p+64,20)<0)throw new FormatProblem("Ungültiger Rufname.");
            if(!Finite(Energy) || Energy<0 || Energy>1000000000f || !Finite(Afterburner) || Afterburner<0 || Afterburner>1000000)
                throw new FormatProblem("Ungültige Spielerwerte.");
            if(I16(Bytes,p+136)!=(PlayerObject.Handle&0x7ff))throw new FormatProblem("Spieler und Spielerobjekt passen nicht zusammen.");
            int ship=I32(Bytes,p+60);
            if(ship<0 || ship>=Tables[3].Length || string.IsNullOrEmpty(Tables[3][ship]))throw new FormatProblem("Spielerschiff fehlt.");
            int primary=I32(Bytes,p+216), secondary=I32(Bytes,p+228);
            if(primary<0 || primary>9 || secondary<10 || secondary>19)throw new FormatProblem("Ungültige aktive Waffe.");
            for(int i=0;i<6;i++) {
                float v=F32(Bytes,p+Layout.Scalars+4*i);
                if(!Finite(v) || v<0 || v>1000000)throw new FormatProblem("Unbekannter Spieleraufbau.");
            }
            int invPos=p+Layout.Size;
            bool guided=false; for(int i=0;i<Layout.PointerBytes;i++) if(Bytes[p+Layout.GuidedPointer+i]!=0)guided=true;
            var r=new Reader(Bytes,invPos); if(guided)r.Skip(4);
            Inventory=InventoryBlock.Read(r); CounterMeasures=InventoryBlock.Read(r);
            if(I32(Bytes,p+Layout.Inventory+Layout.PointerBytes)!=Inventory.Items.Count ||
               I32(Bytes,p+Layout.CounterMeasures+Layout.PointerBytes)!=CounterMeasures.Items.Count)
                throw new FormatProblem("Spielerstruktur und Inventare haben unterschiedliche Größen.");
            // At least the following matcen count must exist; opaque engine/script
            // state is retained verbatim and never reconstructed by the editor.
            r.Need(4); int matcens=r.I32(); if(matcens<0 || matcens>4096)throw new FormatProblem("Ungültiger Folgeblock.");
        }
        public int FindEquipment(string name) {
            for(int i=0;i<Tables[0].Length;i++) if(string.Equals(Tables[0][i],name,StringComparison.OrdinalIgnoreCase))return i;
            return -1;
        }
        public bool HasEquipment(string name) {
            int id=FindEquipment(name); return id>=0 && Inventory.Items.Any(x=>x.Type==7 && x.Id==id && x.Count>0);
        }
        public EditOptions CurrentOptions() {
            var x=new EditOptions { Energy=Energy, Shields=Shields, AfterburnerSeconds=Afterburner, Lives=Bytes[PlayerOffset+108],
                Score=I32(Bytes,PlayerOffset+88), Primary=I32(Bytes,PlayerOffset+216), Secondary=I32(Bytes,PlayerOffset+228) };
            uint mask=U32(Bytes,PlayerOffset+168);
            for(int i=0;i<20;i++) { x.Owned[i]=(mask&(1u<<i))!=0; x.Ammo[i]=U16(Bytes,PlayerOffset+172+2*i); }
            return x;
        }
        public EditResult Edit(EditOptions edit) {
            if(edit==null || edit.Owned==null || edit.Owned.Length!=20 || edit.Ammo==null || edit.Ammo.Length!=20)
                throw new ArgumentException("Unvollständige Editieroptionen.");
            if(!Finite(edit.Energy) || edit.Energy<0 || edit.Energy>1000000000 || !Finite(edit.Shields) || edit.Shields<0 || edit.Shields>1000000000 ||
               !Finite(edit.AfterburnerSeconds) || edit.AfterburnerSeconds<0 || edit.AfterburnerSeconds>1000000)
                throw new FormatProblem("Energie, Schild und Nachbrenner müssen endliche, nicht negative Werte im Eingabebereich sein.");
            uint originalMask=U32(Bytes,PlayerOffset+168);
            if(!edit.Owned.Take(10).Any(x=>x) && (originalMask&0x3ffu)!=0)throw new FormatProblem("Mindestens eine Primärwaffe muss vorhanden sein.");
            var result=new EditResult(); var patches=new List<Patch>();
            var raw=new byte[Layout.Size]; Buffer.BlockCopy(Bytes,PlayerOffset,raw,0,raw.Length);
            Put(raw,104,edit.Energy); Put(raw,88,edit.Score); raw[108]=edit.Lives; Put(raw,Layout.Afterburner,edit.AfterburnerSeconds);
            uint mask=U32(raw,168)&~0xFFFFFu;
            for(int i=0;i<20;i++) { if(edit.Owned[i])mask|=1u<<i; Put(raw,172+2*i,edit.Ammo[i]); }
            Put(raw,168,mask);
            int primary=edit.Primary,secondary=edit.Secondary;
            if(primary<0 || primary>9 || (!edit.Owned[primary] && (mask!=originalMask || primary!=I32(Bytes,PlayerOffset+216)))) {
                primary=Array.FindIndex(edit.Owned,0,10,x=>x);if(primary<0)primary=0;
            }
            if(secondary<10 || secondary>19 || (!edit.Owned[secondary] && (mask!=originalMask || secondary!=I32(Bytes,PlayerOffset+228)))) {
                secondary=Array.FindIndex(edit.Owned,10,10,x=>x); if(secondary<0)secondary=10;
            }
            Put(raw,216,primary); Put(raw,228,secondary);
            if(edit.GodMode.HasValue) {
                uint flags=U32(raw,84)&~(1u|(1u<<22));
                if(edit.GodMode.Value)flags|=1;
                Put(raw,84,flags); Put(raw,116,edit.GodMode.Value ? 1e9f : 0f);
                result.Changes.Add(edit.GodMode.Value ? "God-Mode dauerhaft aktivieren" : "Unverwundbarkeit deaktivieren");
            }
            var inventory=Inventory.Copy();
            SetEquipment(inventory,"Afterburner","AB Cooler",128,edit.Cooler,result);
            SetEquipment(inventory,"Converter","Energy to Shield Converter",131,edit.Converter,result);
            if(edit.QuadLasers.HasValue) {
                if(!SupportsQuad)throw new FormatProblem("Die Waffenbatterien dieses Spielstands erlauben keine Quad-Laser-Bearbeitung.");
                SetEquipment(inventory,"QuadLaser","Quad Laser",132,edit.QuadLasers,result);
                foreach(int index in new[]{0,5}) {
                    int offset=PlayerObject.BatteryStart+108*index+104;
                    uint flags=U32(Bytes,offset)&~16u; if(edit.QuadLasers.Value)flags|=16;
                    patches.Add(new Patch(offset,4,BitConverter.GetBytes(flags)));
                }
            }
            if(inventory.Items.Count>10)throw new FormatProblem("Das Inventar ist voll (maximal 10 verschiedene Gegenstände).");
            Put(raw,Layout.Inventory+Layout.PointerBytes,inventory.Items.Count);
            byte[] invBytes=inventory.Serialize();
            if(!SliceEqual(Bytes,Inventory.Start,Inventory.End-Inventory.Start,invBytes))patches.Add(new Patch(Inventory.Start,Inventory.End-Inventory.Start,invBytes));
            if(edit.Cloaked.HasValue) {
                var effect=new byte[164];
                if(PlayerObject.EffectOffset>=0)Buffer.BlockCopy(Bytes,PlayerObject.EffectOffset,effect,0,164);
                else { Put(effect,4,1f); Put(effect,56,-1); Put(effect,140,1f); Put(effect,144,-1); Put(effect,148,-1); }
                uint flags=U32(effect,0)&~(1u|512u|1024u|4096u);
                if(edit.Cloaked.Value)flags|=1;
                Put(effect,0,flags); Put(effect,12,edit.Cloaked.Value ? 1e9f : 0f); Put(effect,36,0f); Put(effect,40,0f);
                if(PlayerObject.EffectOffset>=0)patches.Add(new Patch(PlayerObject.EffectOffset,164,effect));
                else if(edit.Cloaked.Value) {
                    var record=new byte[167]; record[0]=1; Put(record,1,(ushort)164); Buffer.BlockCopy(effect,0,record,3,164);
                    patches.Add(new Patch(PlayerObject.EffectRecord,1,record));
                }
                result.Changes.Add(edit.Cloaked.Value ? "Tarnung dauerhaft aktivieren" : "Tarnung deaktivieren");
            }
            if(edit.RevealMap) { var map=Enumerable.Repeat((byte)1,MapCount).ToArray(); patches.Add(new Patch(MapOffset,MapCount,map)); result.Changes.Add("Gesamte Karte aufdecken"); }
            patches.Add(new Patch(PlayerObject.ShieldOffset,4,BitConverter.GetBytes(edit.Shields)));
            patches.Add(new Patch(PlayerOffset,Layout.Size,raw));
            if(edit.Energy!=Energy)result.Changes.Add("Energie: "+Energy+" → "+edit.Energy);
            if(edit.Shields!=Shields)result.Changes.Add("Schild: "+Shields+" → "+edit.Shields);
            if(edit.AfterburnerSeconds!=Afterburner)result.Changes.Add("Nachbrennerladung ändern");
            if(edit.Lives!=Bytes[PlayerOffset+108] || edit.Score!=I32(Bytes,PlayerOffset+88))result.Changes.Add("Leben / Punktestand ändern");
            if(mask!=U32(Bytes,PlayerOffset+168) || !edit.Ammo.SequenceEqual(CurrentOptions().Ammo) || primary!=I32(Bytes,PlayerOffset+216) || secondary!=I32(Bytes,PlayerOffset+228))result.Changes.Add("Waffen / Munition / aktive Waffe ändern");
            result.Bytes=ApplyPatches(Bytes,patches); result.Parsed=Parse(result.Bytes);
            if(result.Bytes.SequenceEqual(Bytes))result.Changes.Clear();
            return result;
        }
        private void SetEquipment(InventoryBlock inv,string key,string name,int flags,bool? requested,EditResult result) {
            if(!requested.HasValue)return;
            int id=FindEquipment(key);
            if(id<0)throw new FormatProblem("Die Gegenstandsdefinition „"+key+"“ fehlt in diesem Spielstand.");
            var found=inv.Items.Find(x=>x.Type==7 && x.Id==id);
            if(requested.Value && found==null) {
                inv.Items.Add(new InventoryItem { Type=7,OriginalType=7,Id=id,OriginalId=id,Count=1,Flags=0,
                    InventoryFlags=flags,Description="",Icon="",Name=name });
                result.Changes.Add(name+" hinzufügen");
            } else if(!requested.Value && found!=null) {
                var selected=inv.Selected<inv.Items.Count ? inv.Items[inv.Selected] : null;
                inv.Items.RemoveAll(x=>x.Type==7 && x.Id==id);
                inv.Selected=selected==null ? 0 : Math.Max(0,inv.Items.IndexOf(selected));
                result.Changes.Add(name+" entfernen");
            }
        }
        internal static byte[] ApplyPatches(byte[] source,List<Patch> patches) {
            patches=patches.OrderBy(x=>x.Start).ToList();
            using(var stream=new MemoryStream()) {
                int previous=0;
                foreach(var p in patches) {
                    if(p.Start<previous || p.Start<0 || p.Length<0 || p.Start>source.Length-p.Length)throw new FormatProblem("Überlappende oder ungültige Änderungen.");
                    stream.Write(source,previous,p.Start-previous); stream.Write(p.Data,0,p.Data.Length); previous=p.Start+p.Length;
                }
                stream.Write(source,previous,source.Length-previous); return stream.ToArray();
            }
        }
        internal static bool SliceEqual(byte[] data,int start,int length,byte[] other) {
            if(length!=other.Length)return false; for(int i=0;i<length;i++)if(data[start+i]!=other[i])return false; return true;
        }
        internal static bool Finite(float f) { return !float.IsNaN(f) && !float.IsInfinity(f); }
        internal static string Fixed(byte[] data,int pos,int size) { int n=0; while(n<size && data[pos+n]!=0)n++; return Ansi.GetString(data,pos,n); }
        internal static short I16(byte[] d,int p) { return BitConverter.ToInt16(d,p); }
        internal static ushort U16(byte[] d,int p) { return BitConverter.ToUInt16(d,p); }
        internal static int I32(byte[] d,int p) { return BitConverter.ToInt32(d,p); }
        internal static uint U32(byte[] d,int p) { return BitConverter.ToUInt32(d,p); }
        internal static float F32(byte[] d,int p) { return BitConverter.ToSingle(d,p); }
        internal static void Put(byte[] d,int p,float value) { Buffer.BlockCopy(BitConverter.GetBytes(value),0,d,p,4); }
        internal static void Put(byte[] d,int p,int value) { Buffer.BlockCopy(BitConverter.GetBytes(value),0,d,p,4); }
        internal static void Put(byte[] d,int p,uint value) { Buffer.BlockCopy(BitConverter.GetBytes(value),0,d,p,4); }
        internal static void Put(byte[] d,int p,ushort value) { Buffer.BlockCopy(BitConverter.GetBytes(value),0,d,p,2); }
        public static string HashBytes(byte[] d) { using(var sha=SHA256.Create()) return BitConverter.ToString(sha.ComputeHash(d)).Replace("-","").ToLowerInvariant(); }
    }

    public static class SaveFiles {
        public static SaveGame Open(string path) {
            var info=new FileInfo(path); if(info.Length>256*1024*1024)throw new FormatProblem("Die Datei ist größer als 256 MiB.");
            return SaveGame.Parse(File.ReadAllBytes(path));
        }
        // Write a validated copy or atomically replace with a timestamped backup.
        // No fallback deletes the original, even if atomic replacement fails.
        public static string Write(string destination,EditResult result,string openedPath,string expectedHash) {
            SaveGame.Parse(result.Bytes);
            string full=Path.GetFullPath(destination),opened=Path.GetFullPath(openedPath);
            if(string.Equals(full,opened,StringComparison.OrdinalIgnoreCase) &&
               (!File.Exists(opened) || SaveGame.HashBytes(File.ReadAllBytes(opened))!=expectedHash))
                throw new IOException("Der geöffnete Spielstand wurde inzwischen verändert. Bitte erneut öffnen.");
            string dir=Path.GetDirectoryName(full);
            string temp=Path.Combine(dir,".d3edit-"+Guid.NewGuid().ToString("N")+".tmp");
            string backup=null;
            try {
                using(var fs=new FileStream(temp,FileMode.CreateNew,FileAccess.Write,FileShare.None)) { fs.Write(result.Bytes,0,result.Bytes.Length); fs.Flush(true); }
                if(File.Exists(full)) {
                    backup=full+".bak-"+DateTime.Now.ToString("yyyyMMdd-HHmmss-fff")+"-"+Guid.NewGuid().ToString("N").Substring(0,6);
                    File.Replace(temp,full,backup);
                } else File.Move(temp,full);
                return backup;
            } finally { if(File.Exists(temp))File.Delete(temp); }
        }
    }
}
