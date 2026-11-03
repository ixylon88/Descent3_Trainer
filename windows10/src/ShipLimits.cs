// SPDX-License-Identifier: GPL-3.0-or-later
// Based on the documented reads in manage.cpp / shippage.cpp / cfile.cpp.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace D3SaveEditor {
    public sealed class ShipLimits {
        public readonly Dictionary<string,ushort[]> Ships=new Dictionary<string,ushort[]>(StringComparer.OrdinalIgnoreCase);
        public string Source { get; private set; }
        public void Load(string path) {
            // Parsing completes before the currently loaded data is modified.
            var parsed=new ShipLimits();
            using(var f=new FileStream(path,FileMode.Open,FileAccess.Read,FileShare.Read)) {
                if(f.Length<5)throw new FormatProblem("Die Spieldatendatei ist zu kurz.");
                var magic=new byte[4]; ReadExact(f,magic);
                if(Encoding.ASCII.GetString(magic)=="HOG2") parsed.ReadArchive(f);
                else {
                    if(f.Length>64*1024*1024)throw new FormatProblem("Die Spieldatentabelle ist größer als 64 MiB.");
                    f.Position=0; var data=new byte[(int)f.Length];ReadExact(f,data);parsed.ReadTable(data);
                }
            }
            if(parsed.Ships.Count==0)throw new FormatProblem("Keine unterstützten Schiffsdefinitionen gefunden (Tabellenformat 1–6).");
            foreach(var kv in parsed.Ships)Ships[kv.Key]=kv.Value;
            Source=Path.GetFileName(path);
        }
        private static void ReadExact(Stream stream,byte[] bytes) {
            int pos=0; while(pos<bytes.Length) { int n=stream.Read(bytes,pos,bytes.Length-pos); if(n<=0)throw new FormatProblem("Spieldatendatei ist abgeschnitten.");pos+=n; }
        }
        private void ReadArchive(FileStream stream) {
            var header=new byte[8];ReadExact(stream,header);
            uint count=SaveGame.U32(header,0),dataOffset=SaveGame.U32(header,4);
            long indexOffset=(long)dataOffset-(long)count*48;
            if(count>1000000 || (indexOffset!=68 && indexOffset!=72) || dataOffset>stream.Length)
                throw new FormatProblem("Unbekanntes HOG2-Archivformat.");
            var tables=new List<Tuple<string,long,int>>();
            stream.Position=indexOffset; long current=dataOffset;
            for(uint i=0;i<count;i++) {
                var entry=new byte[48];ReadExact(stream,entry);
                string name=SaveGame.Fixed(entry,0,36);uint length=SaveGame.U32(entry,40);
                if(length>stream.Length-current)throw new FormatProblem("Ein Archiveintrag überschreitet die Dateigröße.");
                if(name.EndsWith(".gam",StringComparison.OrdinalIgnoreCase) && length<=64*1024*1024)
                    tables.Add(Tuple.Create(name,current,(int)length));
                current+=length;
            }
            // Load Table.gam first, then extra/mission tables, just as overrides.
            foreach(var t in tables.OrderBy(x=>string.Equals(x.Item1,"Table.gam",StringComparison.OrdinalIgnoreCase) ? 0 : 1)) {
                stream.Position=t.Item2;var bytes=new byte[t.Item3];ReadExact(stream,bytes);ReadTable(bytes);
            }
        }
        internal void ReadTable(byte[] data) {
            var r=new Reader(data);
            while(r.Pos<r.End) {
                int type=r.U8(), start=r.Pos, length=r.I32();
                if(length<4 || length>r.End-start)throw new FormatProblem("Eine Spieldatenseite hat eine ungültige Länge.");
                int end=start+length;
                if(type==6)ReadShip(new Reader(data,r.Pos,end));
                r.Pos=end;
            }
        }
        private void ReadShip(Reader r) {
            int version=r.I16();
            if(version<1 || version>6)throw new FormatProblem("Unbekannte Schiffsdefinition: Version "+version+".");
            string name=r.Z();
            for(int i=0;i<6;i++)r.Z(); // cockpit, HUD, four models
            r.Skip(8+68+12); // LOD, physics chunk, size/armor/flags
            var limits=new ushort[21];
            for(int i=0;i<21;i++) {
                r.U8();r.Z();r.Z();r.Z();int cap=r.I32();
                if(cap<0 || cap>65535)throw new FormatProblem("Das Schiff verwendet eine unbekannte Munitionsgrenze.");
                limits[i]=(ushort)cap;
                r.Skip(version>=6 ? 211 : 210);
                for(int j=0;j<16;j++)r.Z();
            }
            if(r.Pos!=r.End)throw new FormatProblem("Die Schiffsdefinition enthält unbekannte Zusatzdaten.");
            Ships[name]=limits;
        }
        public ushort[] ForShip(string name) { ushort[] values;return Ships.TryGetValue(name,out values) ? (ushort[])values.Clone() : null; }
    }
}
