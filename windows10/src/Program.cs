// SPDX-License-Identifier: GPL-3.0-or-later
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Windows.Forms;

namespace D3SaveEditor {
    internal static class Program {
        [STAThread]
        private static void Main(string[] args) {
            Application.EnableVisualStyles(); Application.SetCompatibleTextRenderingDefault(false);
            Application.SetUnhandledExceptionMode(UnhandledExceptionMode.CatchException);
            Application.ThreadException+=(sender,e)=>MessageBox.Show(e.Exception.Message,"Descent 3 Savegame Editor",MessageBoxButtons.OK,MessageBoxIcon.Error);
            var form=new EditorForm();
            if(args.Length>0 && args[0]!="--capture")form.Shown+=(s,e)=>form.OpenPath(args[0]);
            // Developer-only screenshot route. No fixture is created or offered as
            // a playable savegame. Also permits repeatable layout QA on Mono.
            if(args.Length>=3 && args[0]=="--capture") {
                form.Shown+=(s,e)=> {
                    form.OpenPath(args[1]);
                    int tab=0; if(args.Length>3)int.TryParse(args[3],out tab);form.ShowTab(tab);
                    var timer=new Timer { Interval=500 };
                    timer.Tick+=(a,b)=> { timer.Stop(); using(var image=new Bitmap(form.Width,form.Height)) { using(var graphics=Graphics.FromImage(image))graphics.CopyFromScreen(form.Location,Point.Empty,form.Size);image.Save(args[2]); } form.Close(); };
                    timer.Start();
                };
            }
            Application.Run(form);
        }
    }
    public sealed class EditorForm : Form {
        private readonly Color Ink=Color.FromArgb(21,36,56), Accent=Color.FromArgb(27,102,174);
        private readonly ToolTip tips=new ToolTip { AutoPopDelay=14000,InitialDelay=350,ReshowDelay=100 };
        private readonly ShipLimits limits=new ShipLimits();
        private readonly TabControl tabs=new TabControl();
        private readonly Button save=new Button(),copy=new Button(),reset=new Button();
        private readonly Label info=new Label(),status=new Label(),ammoHintPrimary=new Label(),ammoHintSecondary=new Label();
        private readonly TextBox review=new TextBox();
        private readonly NumericUpDown energy=Number(1000000000,3),shields=Number(1000000000,3),afterburner=Number(1000000,3),lives=Number(255,0),score=Number(int.MaxValue,0);
        private readonly CheckBox god=new CheckBox(),cloak=new CheckBox(),cooler=new CheckBox(),converter=new CheckBox(),quad=new CheckBox(),fullMap=new CheckBox();
        private readonly Label godTime=new Label(),cloakTime=new Label(),mapInfo=new Label();
        private readonly CheckBox[] owned=new CheckBox[20];
        private readonly NumericUpDown[] ammo=new NumericUpDown[20];
        private readonly Button[] maximum=new Button[20];
        private readonly Label[] capLabels=new Label[20];
        private readonly ComboBox primary=new ComboBox(),secondary=new ComboBox();
        private readonly List<Control> needsFile=new List<Control>();
        private SaveGame game;
        private string openedPath;
        private bool loading,dirty,godTouched,cloakTouched,coolerTouched,converterTouched,quadTouched;

        public EditorForm() {
            Text="Descent 3 · Savegame Editor";
            StartPosition=FormStartPosition.CenterScreen;ClientSize=new Size(1020,760);MinimumSize=new Size(900,700);
            Font=new Font("Segoe UI",9.5f);BackColor=Color.FromArgb(243,246,250);ForeColor=Ink;
            AutoScaleMode=AutoScaleMode.Dpi;
            var root=new TableLayoutPanel { Dock=DockStyle.Fill,RowCount=6,ColumnCount=1,Padding=new Padding(16),BackColor=BackColor };
            root.RowStyles.Add(new RowStyle(SizeType.Absolute,78));root.RowStyles.Add(new RowStyle(SizeType.Absolute,48));
            root.RowStyles.Add(new RowStyle(SizeType.Absolute,63));root.RowStyles.Add(new RowStyle(SizeType.Percent,100));
            root.RowStyles.Add(new RowStyle(SizeType.Absolute,30));root.RowStyles.Add(new RowStyle(SizeType.Absolute,27));Controls.Add(root);
            var header=new Panel { Dock=DockStyle.Fill,BackColor=Ink,Margin=new Padding(0,0,0,8),Padding=new Padding(16,10,16,10) };
            var title=new Label { Text="DESCENT 3",ForeColor=Color.White,Font=new Font("Segoe UI",20,FontStyle.Bold),Location=new Point(14,4),AutoSize=true };
            var subtitle=new Label { Text="SAVEGAME EDITOR  /  Waffen · Ausrüstung · Versorgung",ForeColor=Color.FromArgb(180,207,232),Location=new Point(17,44),AutoSize=true };
            header.Controls.Add(title);header.Controls.Add(subtitle);root.Controls.Add(header,0,0);
            var toolbar=new FlowLayoutPanel { Dock=DockStyle.Fill,WrapContents=false,Margin=new Padding(0),Padding=new Padding(0,3,0,0) };
            toolbar.Controls.Add(Btn("Spielstand öffnen…",(s,e)=>ChooseOpen(),155,true));
            SetupButton(save,"Speichern + Backup",160,(s,e)=>SaveTo(false));toolbar.Controls.Add(save);
            SetupButton(copy,"Kopie speichern…",145,(s,e)=>SaveTo(true));toolbar.Controls.Add(copy);
            SetupButton(reset,"Originalwerte",123,(s,e)=>LoadValues());toolbar.Controls.Add(reset);
            toolbar.Controls.Add(Btn("Schiffslimits laden…",(s,e)=>LoadLimits(),165));
            toolbar.Controls.Add(Btn("Hilfe",(s,e)=>HelpText(),65));root.Controls.Add(toolbar,0,1);
            info.Dock=DockStyle.Fill;info.Margin=new Padding(0,4,0,7);info.Padding=new Padding(10,6,8,2);info.BackColor=Color.White;
            info.Text="Noch kein Spielstand geöffnet.\r\nIm Spiel speichern, dann eine Datei wie saveg000 aus dem Ordner savegame öffnen.";
            info.AutoEllipsis=true;root.Controls.Add(info,0,2);
            tabs.Dock=DockStyle.Fill;tabs.Padding=new Point(18,8);tabs.Margin=new Padding(0);root.Controls.Add(tabs,0,3);
            BuildPlayerTab();BuildWeaponTab(false);BuildWeaponTab(true);BuildReviewTab();
            status.Dock=DockStyle.Fill;status.Text="Bereit · Dateien können auch in das Fenster gezogen werden.";status.TextAlign=ContentAlignment.MiddleLeft;root.Controls.Add(status,0,4);
            root.Controls.Add(new Label { Text="Offline · Format 0–2 · 32/64-Bit-Strukturen · Ingame-Test mit echtem Spielstand noch ausstehend",Dock=DockStyle.Fill,ForeColor=Color.FromArgb(89,104,120),TextAlign=ContentAlignment.MiddleLeft,Font=new Font(Font.FontFamily,8.5f) },0,5);
            needsFile.AddRange(new Control[]{save,copy,reset,energy,shields,afterburner,lives,score,god,cloak,cooler,converter,quad,fullMap,primary,secondary});
            foreach(var c in needsFile)c.Enabled=false;
            foreach(var c in new[]{energy,shields,afterburner,lives,score})c.ValueChanged+=Changed;
            god.CheckedChanged+=(s,e)=> { if(!loading){godTouched=true;Changed(s,e);} };
            cloak.CheckedChanged+=(s,e)=> { if(!loading){cloakTouched=true;Changed(s,e);} };
            cooler.CheckedChanged+=(s,e)=> { if(!loading){coolerTouched=true;Changed(s,e);} };
            converter.CheckedChanged+=(s,e)=> { if(!loading){converterTouched=true;Changed(s,e);} };
            quad.CheckedChanged+=(s,e)=> { if(!loading){quadTouched=true;Changed(s,e);} };
            fullMap.CheckedChanged+=Changed;
            AllowDrop=true;DragEnter+=(s,e)=> { if(e.Data.GetDataPresent(DataFormats.FileDrop))e.Effect=DragDropEffects.Copy; };
            DragDrop+=(s,e)=> { var files=(string[])e.Data.GetData(DataFormats.FileDrop);if(files.Length==1)OpenPath(files[0]); };
            FormClosing+=(s,e)=> { if(!MaybeDiscard())e.Cancel=true; };
        }
        private static NumericUpDown Number(decimal maximum,int places) {
            return new NumericUpDown { Minimum=0,Maximum=maximum,DecimalPlaces=places,ThousandsSeparator=true,Width=145,Height=28,Margin=new Padding(3,5,3,3) };
        }
        private Button Btn(string text,EventHandler handler,int width=120,bool accent=false) {
            var b=new Button();SetupButton(b,text,width,handler);
            if(accent){b.BackColor=Accent;b.ForeColor=Color.White;b.FlatStyle=FlatStyle.Flat;b.FlatAppearance.BorderSize=0;}
            return b;
        }
        private static void SetupButton(Button button,string text,int width,EventHandler handler) {
            button.Text=text;button.Width=width;button.Height=32;button.Margin=new Padding(0,0,7,0);button.UseVisualStyleBackColor=true;button.Click+=handler;
        }
        private static CheckBox Check(CheckBox box,string text) { box.Text=text;box.AutoSize=true;box.Margin=new Padding(5,9,3,4);return box; }
        private static Label LabelFor(string text) { return new Label { Text=text,AutoSize=true,Margin=new Padding(4,9,8,4) }; }
        private TabPage Page(string name) {
            var p=new TabPage(name) { BackColor=Color.White,Padding=new Padding(16),AutoScroll=true };tabs.TabPages.Add(p);return p;
        }
        private static GroupBox Group(string text) { return new GroupBox { Text=text,Dock=DockStyle.Fill,Padding=new Padding(12),Margin=new Padding(0,2,12,12) }; }
        private void BuildPlayerTab() {
            var page=Page("Spieler / Ausrüstung");
            var layout=new TableLayoutPanel { Dock=DockStyle.Top,Height=430,ColumnCount=2,RowCount=2 };
            layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,53));layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,47));
            layout.RowStyles.Add(new RowStyle(SizeType.Absolute,218));layout.RowStyles.Add(new RowStyle(SizeType.Absolute,210));page.Controls.Add(layout);
            var supply=Group("Versorgung");layout.Controls.Add(supply,0,0);
            var values=new TableLayoutPanel { Dock=DockStyle.Fill,ColumnCount=3,RowCount=5 };
            values.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,135));values.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,150));values.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));
            AddNumberRow(values,0,"Energie",energy,"Max 200",()=> { if(game!=null)energy.Value=200; });
            AddNumberRow(values,1,"Schild",shields,"Max 200",()=> { if(game!=null)shields.Value=200; });
            AddNumberRow(values,2,"Nachbrenner (s)",afterburner,"Voll (5 s)",()=> { if(game!=null)afterburner.Value=5; });
            AddNumberRow(values,3,"Leben",lives,"",null);score.Minimum=int.MinValue;AddNumberRow(values,4,"Punktestand",score,"",null);
            supply.Controls.Add(values);
            var effects=Group("Effekte");layout.Controls.Add(effects,1,0);
            var effectRows=new TableLayoutPanel { Dock=DockStyle.Fill,ColumnCount=2,RowCount=4 };
            effectRows.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));effectRows.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,115));
            effectRows.Controls.Add(Check(god,"God-Mode / unverwundbar"),0,0);
            var renewGod=Btn("Erneuern",(s,e)=> { if(game!=null){god.Checked=true;godTouched=true;Changed(s,e);} },110);effectRows.Controls.Add(renewGod,1,0);needsFile.Add(renewGod);
            godTime.AutoSize=true;godTime.Margin=new Padding(5,0,0,8);effectRows.Controls.Add(godTime,0,1);effectRows.SetColumnSpan(godTime,2);
            effectRows.Controls.Add(Check(cloak,"Cloaked-Mode / getarnt"),0,2);
            var renewCloak=Btn("Erneuern",(s,e)=> { if(game!=null){cloak.Checked=true;cloakTouched=true;Changed(s,e);} },110);effectRows.Controls.Add(renewCloak,1,2);needsFile.Add(renewCloak);
            cloakTime.AutoSize=true;cloakTime.Margin=new Padding(5,0,0,0);effectRows.Controls.Add(cloakTime,0,3);effectRows.SetColumnSpan(cloakTime,2);effects.Controls.Add(effectRows);
            tips.SetToolTip(god,"An-/Abwählen setzt oder entfernt die Unverwundbarkeit. „Erneuern“ setzt die lange Restdauer auch bei bereits aktivem Effekt.");
            tips.SetToolTip(cloak,"Tarnung wird am Spielerobjekt gesetzt. Fade-Übergänge werden beim Ändern beendet.");
            var equipment=Group("Ausrüstung");layout.Controls.Add(equipment,0,1);
            var equipmentRows=new FlowLayoutPanel { Dock=DockStyle.Fill,FlowDirection=FlowDirection.TopDown,WrapContents=false };
            equipmentRows.Controls.Add(Check(cooler,"Nachbrenner-Kühler"));equipmentRows.Controls.Add(Check(converter,"Energie → Schild-Konverter"));equipmentRows.Controls.Add(Check(quad,"Quad-Laser"));equipment.Controls.Add(equipmentRows);
            tips.SetToolTip(cooler,"Inventargegenstand „Afterburner“: verdoppelt die Nachladerate des Nachbrenners.");
            var map=Group("Karte");layout.Controls.Add(map,1,1);
            var mapRows=new FlowLayoutPanel { Dock=DockStyle.Fill,FlowDirection=FlowDirection.TopDown,WrapContents=false };
            mapRows.Controls.Add(Check(fullMap,"Gesamte Karte aufdecken"));mapInfo.AutoSize=true;mapInfo.MaximumSize=new Size(360,0);mapInfo.Margin=new Padding(5,8,0,0);mapRows.Controls.Add(mapInfo);
            mapRows.Controls.Add(new Label { Text="Ohne Häkchen bleibt die gespeicherte Erkundung erhalten.",AutoSize=true,MaximumSize=new Size(350,0),Margin=new Padding(5,12,0,0) });map.Controls.Add(mapRows);
        }
        private void AddNumberRow(TableLayoutPanel panel,int row,string label,NumericUpDown number,string action,Action handler) {
            panel.Controls.Add(LabelFor(label),0,row);panel.Controls.Add(number,1,row);
            if(handler!=null) { var button=Btn(action,(s,e)=>handler(),95);button.Margin=new Padding(4,3,0,0);panel.Controls.Add(button,2,row);needsFile.Add(button); }
        }
        private void BuildWeaponTab(bool secondaryTab) {
            var page=Page(secondaryTab ? "Sekundärwaffen" : "Primärwaffen");
            var outer=new TableLayoutPanel { Dock=DockStyle.Top,Height=414,ColumnCount=1,RowCount=4 };
            outer.RowStyles.Add(new RowStyle(SizeType.Absolute,37));outer.RowStyles.Add(new RowStyle(SizeType.Absolute,300));outer.RowStyles.Add(new RowStyle(SizeType.Absolute,39));outer.RowStyles.Add(new RowStyle(SizeType.Absolute,38));page.Controls.Add(outer);
            var active=new FlowLayoutPanel { Dock=DockStyle.Fill,WrapContents=false };active.Controls.Add(LabelFor("Aktive Waffe:"));
            var combo=secondaryTab ? secondary : primary;combo.DropDownStyle=ComboBoxStyle.DropDownList;combo.Width=240;combo.Margin=new Padding(7,4,0,0);
            if(secondaryTab)combo.Items.Add("Keine Sekundärwaffe");
            int first=secondaryTab ? 10 : 0;
            for(int i=first;i<first+10;i++)combo.Items.Add(SaveGame.WeaponNames[i]);
            combo.SelectedIndexChanged+=(s,e)=> {
                if(loading || game==null)return;
                int index=secondaryTab ? combo.SelectedIndex+9 : combo.SelectedIndex;
                if(secondaryTab && combo.SelectedIndex==0) { for(int i=10;i<20;i++)owned[i].Checked=false; }
                else if(index>=first && index<first+10)owned[index].Checked=true;
                Changed(s,e);
            };
            active.Controls.Add(combo);outer.Controls.Add(active,0,0);
            var table=new TableLayoutPanel { Dock=DockStyle.Fill,Margin=new Padding(0),RowCount=11,ColumnCount=5,CellBorderStyle=TableLayoutPanelCellBorderStyle.Single,BackColor=Color.FromArgb(240,244,249) };
            foreach(float width in new float[]{100,270,165,110,225})table.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,width));
            table.RowStyles.Add(new RowStyle(SizeType.Absolute,28));
            for(int row=1;row<11;row++)table.RowStyles.Add(new RowStyle(SizeType.Absolute,26));
            foreach(var label in new[]{"Vorhanden","Waffe","Munition / Stück","Maximieren","Geladenes Schiffslimit"}) {
                int col=Array.IndexOf(new[]{"Vorhanden","Waffe","Munition / Stück","Maximieren","Geladenes Schiffslimit"},label);
                table.Controls.Add(new Label { Text=label,Dock=DockStyle.Fill,TextAlign=ContentAlignment.MiddleLeft,Font=new Font(Font,FontStyle.Bold),Padding=new Padding(5,0,3,0) },col,0);
            }
            for(int i=first;i<first+10;i++) {
                int index=i,row=i-first+1;
                owned[i]=new CheckBox { AutoSize=true,Anchor=AnchorStyles.None,Enabled=false,AccessibleName=SaveGame.WeaponNames[i]+" vorhanden" };
                table.Controls.Add(owned[i],0,row);table.Controls.Add(new Label { Text=SaveGame.WeaponNames[i],Dock=DockStyle.Fill,TextAlign=ContentAlignment.MiddleLeft,Padding=new Padding(6,0,0,0) },1,row);
                ammo[i]=Number(65535,0);ammo[i].AccessibleName="Munition "+SaveGame.WeaponNames[i];ammo[i].Dock=DockStyle.Fill;ammo[i].Margin=new Padding(6,2,6,1);ammo[i].Enabled=false;table.Controls.Add(ammo[i],2,row);
                maximum[i]=Btn("Max",(s,e)=>MaxOne(index),85);maximum[i].Dock=DockStyle.Fill;maximum[i].Margin=new Padding(5,1,5,1);maximum[i].Height=26;maximum[i].Enabled=false;table.Controls.Add(maximum[i],3,row);
                capLabels[i]=new Label { Text="–",Dock=DockStyle.Fill,TextAlign=ContentAlignment.MiddleLeft,Padding=new Padding(8,0,0,0) };table.Controls.Add(capLabels[i],4,row);
                owned[i].CheckedChanged+=(s,e)=> { if(!loading && game!=null) { if(!owned[index].Checked)ammo[index].Value=0;RefreshWeapons();Changed(s,e); } };
                ammo[i].ValueChanged+=Changed;
            }
            outer.Controls.Add(table,0,1);
            var actions=new FlowLayoutPanel { Dock=DockStyle.Fill,Padding=new Padding(0,8,0,0),WrapContents=false };
            var select=Btn("Alle auswählen",(s,e)=>SetAll(first,true),135);actions.Controls.Add(select);needsFile.Add(select);
            var clear=Btn("Alle abwählen",(s,e)=>SetAll(first,false),135);actions.Controls.Add(clear);needsFile.Add(clear);
            var shipMax=Btn("Vorhandene: Schiff-Max",(s,e)=>MaxAll(first,true),205);actions.Controls.Add(shipMax);needsFile.Add(shipMax);
            var storageMax=Btn("Vorhandene: 65.535",(s,e)=>MaxAll(first,false),190);actions.Controls.Add(storageMax);needsFile.Add(storageMax);
            outer.Controls.Add(actions,0,2);
            var hint=secondaryTab ? ammoHintSecondary : ammoHintPrimary;hint.Dock=DockStyle.Fill;hint.Padding=new Padding(0,7,0,0);
            hint.Text="„Max“ nutzt geladene Schiffslimits, sonst 65.535 (Speichermaximum).\r\nSchiffslimits laden: Table.gam oder d3.hog aus deiner Descent-3-Installation auswählen.";
            outer.Controls.Add(hint,0,3);
        }
        private void BuildReviewTab() {
            var page=Page("Datei / Änderungen");
            var panel=new TableLayoutPanel { Dock=DockStyle.Fill,ColumnCount=1,RowCount=2 };panel.RowStyles.Add(new RowStyle(SizeType.Absolute,42));panel.RowStyles.Add(new RowStyle(SizeType.Percent,100));page.Controls.Add(panel);
            var check=Btn("Änderungen prüfen",(s,e)=>CheckChanges(),190);panel.Controls.Add(check,0,0);needsFile.Add(check);
            review.Dock=DockStyle.Fill;review.Multiline=true;review.ReadOnly=true;review.ScrollBars=ScrollBars.Both;review.WordWrap=false;review.Font=new Font("Consolas",10);review.BackColor=Color.FromArgb(248,250,253);
            review.Text="Die Dateianalyse und die Prüfung der geplanten Änderungen erscheinen hier.\r\n\r\nDer Editor verändert vorhandene Spielstände. Er erzeugt keinen vollständigen\r\nLevelzustand allein aus dem Quelltext.";panel.Controls.Add(review,0,1);
        }
        public void ShowTab(int tab) { tabs.SelectedIndex=Math.Max(0,Math.Min(tabs.TabCount-1,tab)); }
        private void Changed(object sender,EventArgs e) { if(!loading && game!=null) { dirty=true;Text="* Descent 3 · Savegame Editor";status.Text="Änderungen noch nicht gespeichert."; } }
        private bool MaybeDiscard() {
            return !dirty || MessageBox.Show(this,"Die Änderungen sind noch nicht gespeichert. Verwerfen?","Ungespeicherte Änderungen",MessageBoxButtons.YesNo,MessageBoxIcon.Question)==DialogResult.Yes;
        }
        private void ChooseOpen() {
            using(var dialog=new OpenFileDialog { Title="Descent-3-Spielstand öffnen (z. B. saveg000)",Filter="Spielstände (saveg*)|saveg*|Alle Dateien (*.*)|*.*",CheckFileExists=true })
                if(dialog.ShowDialog(this)==DialogResult.OK)OpenPath(dialog.FileName);
        }
        public void OpenPath(string path) {
            if(!MaybeDiscard())return;
            try {
                var parsed=SaveFiles.Open(path);game=parsed;openedPath=Path.GetFullPath(path);LoadValues();status.Text="Spielstand geladen. Das Original wurde nicht verändert.";
            } catch(Exception ex) { Error(ex); }
        }
        private void LoadValues() {
            if(game==null)return;
            loading=true;
            try {
                foreach(var c in needsFile)c.Enabled=true;
                var x=game.CurrentOptions();energy.Value=(decimal)x.Energy;shields.Value=Clamp(shields,x.Shields);afterburner.Value=(decimal)x.AfterburnerSeconds;lives.Value=x.Lives;score.Value=x.Score;
                god.Checked=game.Invulnerable;cloak.Checked=game.IsCloaked;cooler.Checked=game.HasEquipment("Afterburner");converter.Checked=game.HasEquipment("Converter");quad.Checked=game.HasQuad;fullMap.Checked=false;
                cooler.Enabled=game.FindEquipment("Afterburner")>=0;converter.Enabled=game.FindEquipment("Converter")>=0;quad.Enabled=game.FindEquipment("QuadLaser")>=0 && game.SupportsQuad;
                tips.SetToolTip(cooler,cooler.Enabled ? "Passiver Nachbrenner-Kühler im Inventar." : "Die Definition „Afterburner“ fehlt im Spielstand.");
                tips.SetToolTip(converter,converter.Enabled ? "Auswählbarer Energie-Schild-Konverter im Inventar." : "Die Definition „Converter“ fehlt im Spielstand.");
                tips.SetToolTip(quad,quad.Enabled ? "Setzt Inventargegenstand und beide Laser-Waffenbatterien." : "Gegenstandsdefinition oder passende Waffenbatterien fehlen.");
                for(int i=0;i<20;i++) { owned[i].Enabled=true;owned[i].Checked=x.Owned[i];ammo[i].Value=x.Ammo[i]; }
                primary.SelectedIndex=x.Primary;secondary.SelectedIndex=x.Owned.Skip(10).Any(y=>y) ? x.Secondary-9 : 0;
                godTime.Text=EffectTime(game.Invulnerable,game.InvulnerableTime);cloakTime.Text=EffectTime(game.IsCloaked,game.CloakTime);
                mapInfo.Text="Gespeichert sichtbar: "+game.RevealedRooms+" von "+(game.HighestRoom+1)+" Raumplätzen.";
                info.Text=Path.GetFileName(openedPath)+"  ·  "+game.Description+"\r\n"+game.Callsign+"  ·  "+game.ShipName+"  ·  "+game.Mission+" / Level "+game.Level+"  ·  Format "+game.Version+"  ·  "+game.Layout.Name;
                godTouched=cloakTouched=coolerTouched=converterTouched=quadTouched=false;dirty=false;Text="Descent 3 · Savegame Editor";RefreshWeapons();
                review.Text=FileReport()+"\r\nKeine geplanten Änderungen.\r\n";
            } finally { loading=false; }
            status.Text="Originalwerte geladen.";
        }
        private static decimal Clamp(NumericUpDown n,float value) { return Math.Max(n.Minimum,Math.Min(n.Maximum,(decimal)value)); }
        private static string EffectTime(bool enabled,float seconds) { return !enabled ? "Gespeichert: inaktiv" : seconds>=100000000 ? "Gespeichert: dauerhaft (lange Restdauer)" : "Gespeichert: aktiv, Restdauer "+seconds.ToString("N1")+" s"; }
        private void RefreshWeapons() {
            if(game==null)return;
            ushort[] caps=limits.ForShip(game.ShipName);
            for(int i=0;i<20;i++) {
                bool usesAmmo=i>=10 || i==1 || i==6 || i==7;
                ammo[i].Enabled=owned[i].Checked && usesAmmo;maximum[i].Enabled=owned[i].Checked && usesAmmo;
                capLabels[i].Text=!usesAmmo ? "Energiewaffe" : caps==null ? "nicht geladen" : caps[i].ToString("N0");
                tips.SetToolTip(maximum[i],caps==null ? "Setzt das Speichermaximum 65.535." : "Setzt das Schiffslimit "+caps[i]+".");
            }
            string text=caps==null ? "„Max“ setzt 65.535 (Speichermaximum). Für normale Spielgrenzen „Schiffslimits laden“ wählen."
                : "„Max“ nutzt die Grenzen für "+game.ShipName+" aus "+limits.Source+".\r\n„Vorhandene: 65.535“ setzt ausdrücklich das Speichermaximum.";
            ammoHintPrimary.Text=text;ammoHintSecondary.Text=text;
            if(!loading) {
                loading=true;
                try {
                    if(primary.SelectedIndex<0 || !owned[primary.SelectedIndex].Checked)primary.SelectedIndex=Array.FindIndex(owned,0,10,x=>x.Checked);
                    int selected=secondary.SelectedIndex+9;
                    if(selected<10 || !owned[selected].Checked) {
                        int first=Array.FindIndex(owned,10,10,x=>x.Checked);secondary.SelectedIndex=first<0 ? 0 : first-9;
                    }
                } finally { loading=false; }
            }
        }
        private void SetAll(int first,bool value) { if(game==null)return;for(int i=first;i<first+10;i++)owned[i].Checked=value; }
        private void MaxOne(int index) {
            if(game==null)return;var caps=limits.ForShip(game.ShipName);ammo[index].Value=caps==null ? 65535 : caps[index];
        }
        private void MaxAll(int first,bool ship) {
            if(game==null)return;var caps=limits.ForShip(game.ShipName);
            if(ship && caps==null) { MessageBox.Show(this,"Bitte zuerst Table.gam oder d3.hog über „Schiffslimits laden“ auswählen.","Schiffslimits fehlen",MessageBoxButtons.OK,MessageBoxIcon.Information);return; }
            for(int i=first;i<first+10;i++)if(owned[i].Checked && ammo[i].Enabled)ammo[i].Value=ship ? caps[i] : 65535;
        }
        private void LoadLimits() {
            using(var dialog=new OpenFileDialog { Title="Spieldaten mit Schiffsdefinitionen laden",Filter="Spieldaten (*.hog;*.mn3;*.gam)|*.hog;*.mn3;*.gam|Alle Dateien (*.*)|*.*" }) {
                if(dialog.ShowDialog(this)!=DialogResult.OK)return;
                try { limits.Load(dialog.FileName);RefreshWeapons();status.Text=limits.Ships.Count+" Schiffsdefinitionen geladen aus "+Path.GetFileName(dialog.FileName)+"."; }
                catch(Exception ex) { Error(ex); }
            }
        }
        private static float FloatValue(NumericUpDown n,float original) { return n.Value==(decimal)original ? original : (float)n.Value; }
        private EditOptions ReadOptions() {
            var x=game.CurrentOptions();x.Energy=FloatValue(energy,x.Energy);x.Shields=FloatValue(shields,x.Shields);x.AfterburnerSeconds=FloatValue(afterburner,x.AfterburnerSeconds);
            x.Lives=(byte)lives.Value;x.Score=(int)score.Value;
            for(int i=0;i<20;i++){x.Owned[i]=owned[i].Checked;x.Ammo[i]=(ushort)ammo[i].Value;}
            x.Primary=primary.SelectedIndex;x.Secondary=secondary.SelectedIndex==0 ? x.Secondary : secondary.SelectedIndex+9;
            x.GodMode=godTouched ? (bool?)god.Checked : null;x.Cloaked=cloakTouched ? (bool?)cloak.Checked : null;
            x.Cooler=coolerTouched ? (bool?)cooler.Checked : null;x.Converter=converterTouched ? (bool?)converter.Checked : null;x.QuadLasers=quadTouched ? (bool?)quad.Checked : null;
            x.RevealMap=fullMap.Checked;return x;
        }
        private string FileReport() {
            return "DATEI\r\n"+openedPath+"\r\nSHA-256: "+game.Hash+"\r\n\r\n"+
                "FORMAT\r\nVersion "+game.Version+" · "+game.Layout.Name+"\r\nSpielerblock: 0x"+game.PlayerOffset.ToString("X")+"\r\n"+
                "Karte: "+game.MapCount+" Einträge · Inventar: "+game.InventoryItems.Count+" Gegenstandsarten\r\n\r\n";
        }
        private EditResult CheckChanges() {
            if(game==null)return null;
            try {
                var result=game.Edit(ReadOptions());
                review.Text=FileReport()+"GEPRÜFTE ÄNDERUNGEN\r\n"+(result.Changes.Count==0 ? "Keine Änderungen. Bytefolge identisch." : string.Join("\r\n",result.Changes.Select(x=>"• "+x).ToArray()))+
                    "\r\n\r\nAusgabe erneut erfolgreich eingelesen.\r\nGröße: "+game.Bytes.Length+" → "+result.Bytes.Length+" Byte\r\n";
                status.Text=result.Changes.Count==0 ? "Prüfung bestanden: keine Änderung." : "Prüfung bestanden: "+result.Changes.Count+" Änderungsgruppen.";return result;
            } catch(Exception ex) { review.Text=FileReport()+"PRÜFUNG FEHLGESCHLAGEN\r\n"+ex.Message;Error(ex);return null; }
        }
        private void SaveTo(bool asCopy) {
            var result=CheckChanges();if(result==null)return;
            string destination=openedPath;
            if(asCopy) {
                using(var dialog=new SaveFileDialog { Title="Bearbeiteten Spielstand als Kopie speichern",Filter="Alle Dateien (*.*)|*.*",AddExtension=false,FileName=Path.GetFileName(openedPath)+".edited",InitialDirectory=Path.GetDirectoryName(openedPath),OverwritePrompt=true }) {
                    if(dialog.ShowDialog(this)!=DialogResult.OK)return;destination=dialog.FileName;
                }
            }
            try {
                string backup=SaveFiles.Write(destination,result,openedPath,game.Hash);
                game=result.Parsed;openedPath=Path.GetFullPath(destination);LoadValues();
                status.Text="Gespeichert: "+Path.GetFileName(destination)+(backup==null ? "" : " · Backup: "+Path.GetFileName(backup));
            } catch(Exception ex) { Error(ex); }
        }
        private void Error(Exception ex) { status.Text="Fehler: "+ex.Message;MessageBox.Show(this,ex.Message,"Datei nicht geändert",MessageBoxButtons.OK,MessageBoxIcon.Error); }
        private void HelpText() {
            MessageBox.Show(this,"Descent 3 Savegame Editor 1.0\r\n\r\n1. Im Spiel speichern und das Spiel beenden.\r\n2. saveg000, saveg001 usw. aus dem Ordner savegame öffnen.\r\n3. Werte ändern und „Speichern + Backup“ wählen.\r\n4. Den Spielstand im Spiel neu laden.\r\n\r\nBei jedem Überschreiben entsteht daneben eine datierte .bak-Datei.\r\nKopien mit Endung .edited zum Laden in einen passenden savegNNN-Slot umbenennen.\r\n\r\n„Max“: Schiffslimit aus Table.gam / d3.hog, sonst 65.535.\r\n„Erneuern“: God-Mode / Tarnung mit 1.000.000.000 s Restdauer.\r\nEffekte können durch Levelwechsel oder Skripte aufgehoben werden.\r\n\r\nQuelltextbasiert entwickelt; mit synthetischen Dateien geprüft.\r\nReale Spielstände und das Laden im Spiel sind noch nicht getestet.\r\nUnterstützt Windows-Strukturen mit 496 bzw. 536 Byte; Format 0–2.\r\n\r\nGPL-3.0-or-later · Quelle: github.com/DescentDevelopers/Descent3", "Anleitung",MessageBoxButtons.OK,MessageBoxIcon.Information);
        }
    }
}
