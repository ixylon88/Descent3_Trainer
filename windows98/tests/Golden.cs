using System;
using System.IO;
using D3SaveEditor;
class Golden {
 static void Main(string[] args) {
  var game=SaveGame.Parse(File.ReadAllBytes(args[0]));var o=game.CurrentOptions();
  o.Energy=200;o.Shields=200;o.AfterburnerSeconds=5;o.Lives=99;o.Score=123456;
  o.GodMode=true;o.Cloaked=true;o.Cooler=true;o.Converter=false;o.QuadLasers=true;o.RevealMap=true;o.Primary=9;o.Secondary=19;
  for(int i=0;i<20;i++){o.Owned[i]=true;o.Ammo[i]=(ushort)(i<10?i*17:65535);}
  File.WriteAllBytes(args[1],game.Edit(o).Bytes);
 }
}
