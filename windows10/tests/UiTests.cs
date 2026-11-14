// SPDX-License-Identifier: GPL-3.0-or-later
using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Windows.Forms;
using D3SaveEditor;

internal static class UiTests {
    private static int passed;
    private static T Get<T>(EditorForm form,string field) { return (T)typeof(EditorForm).GetField(field,BindingFlags.Instance|BindingFlags.NonPublic).GetValue(form); }
    private static void Check(bool condition,string text) { if(!condition)throw new Exception("FAIL: "+text);passed++;Console.WriteLine("PASS: "+text); }
    [STAThread]
    private static void Main(string[] args) {
        Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
        string root=Path.Combine(Path.GetDirectoryName(args[0]),"ui-tests-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(root);
        string source=Path.Combine(root,"saveg000");File.Copy(args[0],source);
        var form=new EditorForm();
        try {
            form.Show();Application.DoEvents();form.OpenPath(source);Application.DoEvents();
            Check(Get<Button>(form,"save").Enabled && Get<NumericUpDown>(form,"energy").Value==100.125m,"GUI opens independent synthetic fixture");
            var owned=Get<CheckBox[]>(form,"owned");var ammo=Get<NumericUpDown[]>(form,"ammo");
            Get<ComboBox>(form,"primary").SelectedIndex=4;
            Check(owned[4].Checked,"choosing active primary grants that weapon");
            Get<ComboBox>(form,"secondary").SelectedIndex=0;
            Check(owned.Skip(10).All(x=>!x.Checked) && ammo[10].Value==0,"choosing no secondary removes secondary weapons and ammo");
            owned[19].Checked=true;ammo[19].Value=65535;
            Get<NumericUpDown>(form,"energy").Value=200;Get<NumericUpDown>(form,"shields").Value=200;
            Get<NumericUpDown>(form,"afterburner").Value=5;Get<NumericUpDown>(form,"lives").Value=50;Get<NumericUpDown>(form,"score").Value=999;
            foreach(string field in new[]{"god","cloak","cooler","quad","fullMap"})Get<CheckBox>(form,field).Checked=true;
            Get<CheckBox>(form,"converter").Checked=false;
            form.ShowTab(3);Application.DoEvents();
            var cargo=Get<NumericUpDown[]>(form,"cargoCounts");
            var measures=Get<NumericUpDown[]>(form,"countermeasureCounts");
            var ships=Get<CheckBox[]>(form,"unlockShips");
            var inventory=Get<ListBox>(form,"savedInventory");
            Check(cargo[0].Visible && cargo[0].Value==2 && measures[0].Value==2 && measures[3].Maximum==4,"GUI shows both inventories and countermeasure limits");
            Check(inventory.Items.Cast<string>().Any(item=>item.Contains("Mission cargo")),"GUI shows the saved mission inventory");
            Check(ships[0].Checked && !ships[0].Enabled && ships[1].Enabled && !ships[1].Checked,"GUI distinguishes allowed and unlockable ships");
            cargo[0].Value=4;measures[0].Value=12;measures[1].Value=0;ships[1].Checked=true;
            Get<Button>(form,"save").PerformClick();Application.DoEvents();
            var saved=SaveFiles.Open(source);var options=saved.CurrentOptions();
            Check(saved.Energy==200 && saved.Shields==200 && options.Lives==50 && options.Score==999,"GUI save writes edited player values");
            Check(options.Primary==4 && options.Secondary==19 && options.Ammo[19]==65535,"GUI save writes selected weapons and maximum ammo");
            Check(saved.Invulnerable && saved.IsCloaked && saved.HasEquipment("Afterburner") && saved.HasQuad && !saved.HasEquipment("Converter"),"GUI save writes requested effects and equipment");
            Check(saved.InventoryItems.Any(item=>item.Name=="Force Wall" && item.Count==4) &&
                saved.CountermeasureItems.Any(item=>item.Name=="Saved Chaff" && item.Count==12) &&
                saved.CountermeasureItems.All(item=>item.Name!="Saved Betty"),"GUI save writes addition and removal counts");
            Check(saved.ShipPermissions==0x40000023u && saved.ShipName=="Pyro-GL","GUI saves permissions by name");
            Check(Directory.GetFiles(root,"*.bak-*").Length==1,"GUI save creates backup");
            byte[] before=File.ReadAllBytes(source);Get<Button>(form,"save").PerformClick();Application.DoEvents();
            Check(before.SequenceEqual(File.ReadAllBytes(source)),"GUI unchanged resave preserves exact bytes");
            Console.WriteLine("\n"+passed+" GUI assertions passed on Mono/X11, not native Windows.");
        } finally {
            typeof(EditorForm).GetField("dirty",BindingFlags.Instance|BindingFlags.NonPublic).SetValue(form,false);
            form.Close();form.Dispose();Directory.Delete(root,true);
        }
    }
}
