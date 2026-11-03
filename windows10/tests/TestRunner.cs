// SPDX-License-Identifier: GPL-3.0-or-later
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using D3SaveEditor;

internal static class Tests {
    private static int passed;
    private static void Check(bool condition,string label) {
        if(!condition)throw new Exception("FAIL: "+label);passed++;Console.WriteLine("PASS: "+label);
    }
    private static void Reject(Action action,string label) {
        try { action(); } catch(FormatProblem) { Check(true,label);return; }throw new Exception("Not rejected: "+label);
    }
    private static byte[] Changed(byte[] bytes,int offset,byte[] change) {var copy=(byte[])bytes.Clone();Buffer.BlockCopy(change,0,copy,offset,change.Length);return copy;}
    private static void Main(string[] args) {
        string dir=args.Length>0 ? args[0] : ".";
        foreach(string name in new[]{"fixture32","fixture64","fixture32-noeffects","fixture64-guided","fixture32-snapshot","fixture32-v1","fixture32-v0"}) {
            byte[] original=File.ReadAllBytes(Path.Combine(dir,name));var s=SaveGame.Parse(original);
            Check(s.Energy==100.125f && s.Shields==87.25f && s.Callsign=="Testpilot",name+": fields match independent C++ writer");
            Check(s.Layout.Size==(name.StartsWith("fixture64") ? 536 : 496),name+": player ABI");
            Check(s.CurrentOptions().Ammo[10]==4 && s.HasEquipment("Converter"),name+": weapon and inventory reads");
            var untouched=s.Edit(s.CurrentOptions());Check(untouched.Bytes.SequenceEqual(original),name+": unchanged save is byte-identical");
            var x=s.CurrentOptions();x.Energy=200;x.Shields=200;x.AfterburnerSeconds=5;x.Lives=99;x.Score=123456;
            x.GodMode=true;x.Cloaked=true;x.Cooler=true;x.Converter=false;x.QuadLasers=true;x.RevealMap=true;
            for(int i=0;i<20;i++){x.Owned[i]=true;x.Ammo[i]=(ushort)(i<10 ? i*17 : 65535);}
            x.Primary=9;x.Secondary=19;
            var edited=s.Edit(x);var again=SaveGame.Parse(edited.Bytes);var read=again.CurrentOptions();
            Check(again.Invulnerable && again.InvulnerableTime==1e9f && again.IsCloaked && again.CloakTime==1e9f,name+": persistent effect flags and timers");
            Check(again.Energy==200 && again.Shields==200 && again.Afterburner==5 && read.Lives==99 && read.Score==123456,name+": player values");
            Check(read.Primary==9 && read.Secondary==19 && read.Owned.All(v=>v) && read.Ammo.Skip(10).All(v=>v==65535),name+": all twenty weapons and secondary counts");
            Check(again.HasEquipment("Afterburner") && !again.HasEquipment("Converter") && again.HasEquipment("QuadLaser") && again.HasQuad,name+": inventory insertion/removal and both Quad batteries");
            Check((SaveGame.U32(again.Bytes,again.PlayerOffset+168)&0x80100000)==0x80100000,name+": flare and unknown ownership bits preserved");
            Check((SaveGame.U32(again.Bytes,again.PlayerOffset+84)&4096)!=0 && SaveGame.I32(again.Bytes,again.PlayerOffset+again.Layout.Permissions)==7,name+": unrelated player flags and fields preserved");
            Check(again.Bytes.Skip(again.MapOffset).Take(again.MapCount).All(v=>v==1),name+": whole automap");
            Check(edited.Bytes.Skip(edited.Bytes.Length-275).SequenceEqual(original.Skip(original.Length-275)),name+": opaque engine tail preserved");
            Check(again.Edit(again.CurrentOptions()).Bytes.SequenceEqual(edited.Bytes),name+": modified file reopens and round-trips");
            var disabled=again.CurrentOptions();disabled.GodMode=false;disabled.Cloaked=false;disabled.Cooler=false;disabled.QuadLasers=false;
            var off=again.Edit(disabled).Parsed;
            Check(!off.Invulnerable && !off.IsCloaked && off.InvulnerableTime==0 && off.CloakTime==0 && !off.HasQuad && !off.HasEquipment("Afterburner"),name+": effects and equipment can be disabled");
        }
        var baseBytes=File.ReadAllBytes(Path.Combine(dir,"fixture32"));var valid=SaveGame.Parse(baseBytes);
        Reject(()=>SaveGame.Parse(Changed(baseBytes,32,BitConverter.GetBytes((short)3))),"unknown version rejected");
        Reject(()=>SaveGame.Parse(Changed(baseBytes,valid.PlayerOffset-2,BitConverter.GetBytes((short)500))),"unknown raw ABI rejected");
        Reject(()=>SaveGame.Parse(Changed(baseBytes,valid.PlayerOffset+136,BitConverter.GetBytes((short)13))),"cross-link mismatch rejected");
        Reject(()=>SaveGame.Parse(Changed(baseBytes,valid.PlayerOffset+valid.Layout.Inventory+4,BitConverter.GetBytes(2))),"raw and serialized inventory mismatch rejected");
        Reject(()=>SaveGame.Parse(Changed(baseBytes,valid.MapOffset,new byte[]{2})),"unknown map values rejected");
        Reject(()=>SaveGame.Parse(baseBytes.Take(valid.PlayerOffset+300).ToArray()),"truncated player rejected");
        Reject(()=>SaveGame.Parse(baseBytes.Concat(baseBytes.Skip(valid.PlayerObject.Start)).ToArray()),"ambiguous valid player records rejected");
        Reject(()=>SaveGame.Parse(Changed(baseBytes,valid.PlayerObject.EffectOffset-2,BitConverter.GetBytes((short)168))),"unknown effect ABI rejected");
        var badOptions=valid.CurrentOptions();badOptions.Energy=float.NaN;Reject(()=>valid.Edit(badOptions),"NaN edits rejected");
        var disarmed=valid.CurrentOptions();disarmed.Owned[0]=false;Reject(()=>valid.Edit(disarmed),"removing final primary rejected");
        var fallback=valid.CurrentOptions();fallback.Primary=4;var corrected=valid.Edit(fallback).Parsed;
        Check(corrected.CurrentOptions().Primary==0,"equipped weapon corrected to owned primary");
        var finite=Changed(baseBytes,valid.PlayerOffset+84,BitConverter.GetBytes(4097u));SaveGame.Put(finite,valid.PlayerOffset+116,12.5f);
        var finiteGame=SaveGame.Parse(finite);Check(finiteGame.Edit(finiteGame.CurrentOptions()).Bytes.SequenceEqual(finite),"finite power-up timer preserved when untouched");
        var inactive=Changed(baseBytes,valid.PlayerOffset+168,BitConverter.GetBytes(0x80100001u));SaveGame.Put(inactive,valid.PlayerOffset+228,17);
        var inactiveGame=SaveGame.Parse(inactive);Check(inactiveGame.Edit(inactiveGame.CurrentOptions()).Bytes.SequenceEqual(inactive),"unused active-secondary index preserved in unchanged save");
        var unarmed=Changed(baseBytes,valid.PlayerOffset+168,BitConverter.GetBytes(0x80100400u));var unarmedGame=SaveGame.Parse(unarmed);
        Check(unarmedGame.Edit(unarmedGame.CurrentOptions()).Bytes.SequenceEqual(unarmed),"already unarmed source preserves exact unchanged bytes");
        var noDefinition=Changed(baseBytes,FindAscii(baseBytes,"Afterburner"),Encoding.ASCII.GetBytes("Otherburner"));var noDefGame=SaveGame.Parse(noDefinition);var noDefEdit=noDefGame.CurrentOptions();noDefEdit.Cooler=true;
        Reject(()=>noDefGame.Edit(noDefEdit),"missing equipment definition rejected");
        FileTests(dir,baseBytes);
        TableTests(dir);
        Console.WriteLine("\n"+passed+" assertions passed. No real in-game save/load test has been performed.");
    }
    private static int FindAscii(byte[] bytes,string value) {
        byte[] pattern=Encoding.ASCII.GetBytes(value);
        for(int i=0;i<=bytes.Length-pattern.Length;i++)if(bytes.Skip(i).Take(pattern.Length).SequenceEqual(pattern))return i;
        throw new Exception("pattern missing");
    }
    private static void FileTests(string dir,byte[] original) {
        string root=Path.Combine(dir,"file-tests-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(root);
        try {
            string source=Path.Combine(root,"saveg000"),copy=Path.Combine(root,"copy");File.WriteAllBytes(source,original);
            var game=SaveFiles.Open(source);var opts=game.CurrentOptions();opts.Energy=200;var result=game.Edit(opts);
            Check(SaveFiles.Write(copy,result,source,game.Hash)==null && File.ReadAllBytes(source).SequenceEqual(original),"copy write preserves source");
            Check(SaveFiles.Open(copy).Energy==200,"copy reopens");
            string backup=SaveFiles.Write(source,result,source,game.Hash);
            Check(backup!=null && File.ReadAllBytes(backup).SequenceEqual(original) && SaveFiles.Open(source).Energy==200,"atomic replace creates exact backup");
            bool refused=false;try{SaveFiles.Write(source,result,source,game.Hash);}catch(IOException){refused=true;}
            Check(refused && SaveFiles.Open(source).Energy==200,"external modifications detected before overwriting");
            var corrupt=new EditResult {Bytes=new byte[]{1,2,3}};Reject(()=>SaveFiles.Write(copy,corrupt,source,game.Hash),"invalid output never written");
            Check(SaveFiles.Open(copy).Energy==200 && !Directory.GetFiles(root,"*.tmp").Any(),"existing destination preserved and no temporary leftovers");
        } finally { Directory.Delete(root,true); }
    }
    private static void WriteZ(BinaryWriter w,string value){w.Write(Encoding.ASCII.GetBytes(value));w.Write((byte)0);}
    private static byte[] ShipTable(int version) {
        using(var pageStream=new MemoryStream())using(var page=new BinaryWriter(pageStream)) {
            page.Write((short)version);WriteZ(page,"Pyro-GL");for(int i=0;i<6;i++)WriteZ(page,"");page.Write(new byte[8+68+12]);
            for(int i=0;i<21;i++) {
                page.Write((byte)0);for(int j=0;j<3;j++)WriteZ(page,"");page.Write(i*7);page.Write(new byte[version>=6 ? 211 : 210]);
                for(int j=0;j<16;j++)WriteZ(page,"");
            }
            using(var stream=new MemoryStream())using(var writer=new BinaryWriter(stream)) {
                writer.Write((byte)6);writer.Write((int)pageStream.Length+4);writer.Write(pageStream.ToArray());return stream.ToArray();
            }
        }
    }
    private static void TableTests(string dir) {
        foreach(int version in new[]{5,6}) {
            var limits=new ShipLimits();limits.ReadTable(ShipTable(version));var caps=limits.ForShip("pyro-gl");
            Check(caps!=null && caps[10]==70 && caps[19]==133,"ship table v"+version+": limits decoded");
        }
        var bad=ShipTable(6);SaveGame.Put(bad,1,int.MaxValue);Reject(()=>new ShipLimits().ReadTable(bad),"table length bounds checked");
        string path=Path.Combine(dir,"test-limits.hog");
        try {
            var table=ShipTable(6);
            using(var stream=File.Create(path))using(var w=new BinaryWriter(stream)) {
                w.Write(Encoding.ASCII.GetBytes("HOG2"));w.Write(1u);w.Write(116u);w.Write(new byte[56]);
                var name=new byte[36];Encoding.ASCII.GetBytes("Table.gam").CopyTo(name,0);w.Write(name);w.Write(0u);w.Write((uint)table.Length);w.Write(0u);w.Write(table);
            }
            var limits=new ShipLimits();limits.Load(path);Check(limits.ForShip("Pyro-GL")[19]==133,"HOG2 archive loads ship table without loading whole archive");
        } finally {if(File.Exists(path))File.Delete(path);}
    }
}
