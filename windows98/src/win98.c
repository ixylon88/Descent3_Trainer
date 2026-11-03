/* SPDX-License-Identifier: GPL-3.0-or-later
 * ANSI Win32 GUI. No .NET, Unicode layer, CRT, MFC or external VC runtime.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include "d3core.h"
#include "d3files.h"

enum {ID_OPEN=100,ID_SAVE,ID_COPY,ID_RESET,ID_LIMITS,ID_HELP,ID_TAB,
ID_ENERGY=120,ID_SHIELDS,ID_AB,ID_LIVES,ID_SCORE,ID_ENERGY_MAX,ID_SHIELD_MAX,ID_AB_MAX,
ID_GOD=140,ID_CLOAK,ID_COOLER,ID_CONVERTER,ID_QUAD,ID_MAP,ID_GOD_RENEW,ID_CLOAK_RENEW,
ID_PRIMARY=160,ID_SECONDARY,ID_CHECK,
ID_ALL_PRI=170,ID_NONE_PRI,ID_CAP_PRI,ID_STORE_PRI,ID_ALL_SEC,ID_NONE_SEC,ID_CAP_SEC,ID_STORE_SEC,
ID_OWN=200,ID_AMMO=240,ID_MAX=280};
typedef struct {HWND hwnd;int page,needs_file;} Widget;
static HINSTANCE instance;
static HWND main_window,tab,info,status,hint,review;
static HFONT font,title_font;
static Widget widgets[256];static int widget_count,selected_page,loading,dirty;
static HWND energy,shields,afterburner,lives,score,god,cloak,cooler,converter,quad,full_map,god_time,cloak_time,map_info;
static HWND owned[20],ammo[20],max_buttons[20],cap_labels[20],primary,secondary;
static D3Game *game;
static D3Buffer opened;
static D3U8 original_hash[32];
static char opened_path[260],energy_text[48],shield_text[48],ab_text[48],caps_ship[128];
static D3U16 caps[21];static int caps_loaded;
static int god_request=-1,cloak_request=-1,cooler_request=-1,converter_request=-1,quad_request=-1;

static int checked(HWND h){return SendMessageA(h,BM_GETCHECK,0,0)==BST_CHECKED;}
static void check(HWND h,int on){SendMessageA(h,BM_SETCHECK,on?BST_CHECKED:BST_UNCHECKED,0);}
static void text(HWND h,const char *s){SetWindowTextA(h,s);}
static void get_text(HWND h,char *s,int cap){GetWindowTextA(h,s,cap);}
static void set_status(const char *s){text(status,s);}
static void error_box(const char *s){set_status(s);MessageBoxA(main_window,s,"Descent 3 Editor - Fehler",MB_OK|MB_ICONEXCLAMATION);}
static void changed(void){if(!loading && game){dirty=1;SetWindowTextA(main_window,"* Descent 3 Savegame Editor - Windows 98");set_status("Aenderungen noch nicht gespeichert.");}}
static int discard(void){return !dirty || MessageBoxA(main_window,"Nicht gespeicherte Aenderungen verwerfen?","Aenderungen",MB_YESNO|MB_ICONQUESTION)==IDYES;}
static HWND control(const char *cls,const char *label,DWORD style,int x,int y,int w,int h,int id,int page,int needs_file){
    HWND hwnd=CreateWindowExA(d3_string_equal(cls,"EDIT")?WS_EX_CLIENTEDGE:0,cls,label,WS_CHILD|WS_VISIBLE|style,x,y,w,h,main_window,(HMENU)(INT_PTR)id,instance,0);
    SendMessageA(hwnd,WM_SETFONT,(WPARAM)font,TRUE);
    widgets[widget_count].hwnd=hwnd;widgets[widget_count].page=page;widgets[widget_count].needs_file=needs_file;widget_count++;
    return hwnd;
}
static HWND label(const char *s,int x,int y,int w,int h,int page){return control("STATIC",s,SS_LEFT,x,y,w,h,0,page,0);}
static HWND button(const char *s,int x,int y,int w,int id,int page,int needs){return control("BUTTON",s,BS_PUSHBUTTON|WS_TABSTOP,x,y,w,24,id,page,needs);}
static HWND checkbox(const char *s,int x,int y,int w,int id,int page){return control("BUTTON",s,BS_AUTOCHECKBOX|WS_TABSTOP,x,y,w,20,id,page,1);}
static HWND edit(int x,int y,int w,int id,int page,int numeric){
    HWND h=control("EDIT","",ES_AUTOHSCROLL|WS_TABSTOP|(numeric?ES_NUMBER:0),x,y,w,22,id,page,1);SendMessageA(h,EM_LIMITTEXT,40,0);return h;
}
static void group(const char *s,int x,int y,int w,int h,int page){control("BUTTON",s,BS_GROUPBOX,x,y,w,h,0,page,0);}
static void apply_visibility(void){int i;for(i=0;i<widget_count;i++)if(widgets[i].page>=0)ShowWindow(widgets[i].hwnd,widgets[i].page==selected_page?SW_SHOW:SW_HIDE);ShowWindow(hint,selected_page==1 || selected_page==2?SW_SHOW:SW_HIDE);}
static void enable_controls(void){int i;for(i=0;i<widget_count;i++)if(widgets[i].needs_file)EnableWindow(widgets[i].hwnd,game!=0);}
static int use_caps(void){return game && caps_loaded && d3_string_iequal(caps_ship,d3_ship(game));}
static int uses_ammo(int i){return i>=10 || i==1 || i==6 || i==7;}
static void refresh_weapons(int normalize){
    int i,p,s,first,old_loading=loading;char buffer[32];
    if(!game)return;
    for(i=0;i<20;i++){
        EnableWindow(ammo[i],checked(owned[i]) && uses_ammo(i));EnableWindow(max_buttons[i],checked(owned[i]) && uses_ammo(i));
        if(!uses_ammo(i))text(cap_labels[i],"Energiewaffe");else if(use_caps()){d3_format_u32(buffer,caps[i]);text(cap_labels[i],buffer);}else text(cap_labels[i],"nicht geladen");
    }
    text(hint,use_caps()?"Max verwendet die geladenen Schiffslimits. 65.535 setzt ausdruecklich das Speichermaximum.":"Max setzt 65.535 (Speichermaximum). Normale Grenzen: Schiffslimits aus Table.gam / d3.hog laden.");
    if(normalize){
        loading=1;p=(int)SendMessageA(primary,CB_GETCURSEL,0,0);
        if(p<0 || !checked(owned[p])){for(first=0;first<10;first++)if(checked(owned[first]))break;SendMessageA(primary,CB_SETCURSEL,first==10?(WPARAM)-1:(WPARAM)first,0);}
        s=(int)SendMessageA(secondary,CB_GETCURSEL,0,0)+9;
        if(s<10 || !checked(owned[s])){for(first=10;first<20;first++)if(checked(owned[first]))break;SendMessageA(secondary,CB_SETCURSEL,first==20?0:(WPARAM)(first-9),0);}
        loading=old_loading;
    }
}
static void append_number(char *out,D3U32 capacity,D3U32 value){char s[24];d3_format_u32(s,value);d3_append(out,capacity,s);}
static void effect_label(HWND h,int on,float duration){char s[100],n[48];if(!on){text(h,"Gespeichert: inaktiv");return;}if(duration>=1e8f){text(h,"Gespeichert: lange Restdauer");return;}d3_string(s,100,"Gespeichert: ");d3_format_float(n,duration);d3_append(s,100,n);d3_append(s,100," s Restdauer");text(h,s);}
static void file_report(char *out,D3U32 cap){
    int i;char hash[65];const char *hex="0123456789abcdef";
    for(i=0;i<32;i++){hash[2*i]=hex[original_hash[i]>>4];hash[2*i+1]=hex[original_hash[i]&15];}hash[64]=0;
    d3_string(out,cap,"DATEI\r\n");d3_append(out,cap,opened_path);d3_append(out,cap,"\r\n\r\nSHA-256: ");d3_append(out,cap,hash);
    d3_append(out,cap,"\r\nFormat: ");append_number(out,cap,(D3U32)game->version);d3_append(out,cap,"; historischer Spielerblock: 496 Byte\r\nSpieler: ");d3_append(out,cap,game->callsign);
    d3_append(out,cap,"\r\nSchiff: ");d3_append(out,cap,d3_ship(game));d3_append(out,cap,"\r\nInventararten: ");append_number(out,cap,(D3U32)game->inventory.count);d3_append(out,cap,"\r\n");
}
static void load_values(void){
    D3Options o;int i,visible=0,has_second=0;char s[1400],n[48];
    if(!game)return;loading=1;enable_controls();d3_current(game,&o);
    d3_format_float(energy_text,o.energy);d3_format_float(shield_text,o.shields);d3_format_float(ab_text,o.afterburner);text(energy,energy_text);text(shields,shield_text);text(afterburner,ab_text);
    d3_format_u32(n,(D3U32)o.lives);text(lives,n);d3_format_i32(n,o.score);text(score,n);
    check(god,d3_god(game));check(cloak,d3_cloak(game));check(cooler,d3_has_item(game,game->cooler_id));check(converter,d3_has_item(game,game->converter_id));check(quad,d3_quad(game));check(full_map,0);
    EnableWindow(cooler,game->cooler_id>=0);EnableWindow(converter,game->converter_id>=0);EnableWindow(quad,game->quad_id>=0 && game->object.batteries==21);
    for(i=0;i<20;i++){check(owned[i],o.owned[i]);d3_format_u32(n,o.ammo[i]);text(ammo[i],n);if(i>=10 && o.owned[i])has_second=1;}
    SendMessageA(primary,CB_SETCURSEL,o.primary,0);SendMessageA(secondary,CB_SETCURSEL,has_second?(WPARAM)(o.secondary-9):0,0);
    effect_label(god_time,d3_god(game),d3_float(game->data+game->player+116));effect_label(cloak_time,d3_cloak(game),game->object.has_effect?d3_float(game->data+game->object.effect+12):0);
    for(i=0;i<=game->highest_room;i++)if(game->data[game->map+i])visible++;
    d3_string(s,1400,"Gespeichert sichtbar: ");append_number(s,1400,(D3U32)visible);d3_append(s,1400," von ");append_number(s,1400,(D3U32)(game->highest_room+1));d3_append(s,1400," Raumplaetzen.\r\nOhne Haken bleibt die Erkundung erhalten.");text(map_info,s);
    d3_string(s,1400,opened_path);d3_append(s,1400,"\r\n");d3_append(s,1400,game->callsign);d3_append(s,1400," | ");d3_append(s,1400,d3_ship(game));d3_append(s,1400," | ");d3_append(s,1400,game->mission);d3_append(s,1400," / Level ");append_number(s,1400,(D3U32)game->level);text(info,s);
    file_report(s,1400);d3_append(s,1400,"\r\nKeine geplanten Aenderungen.");text(review,s);
    god_request=cloak_request=cooler_request=converter_request=quad_request=-1;dirty=0;SetWindowTextA(main_window,"Descent 3 Savegame Editor - Windows 98");refresh_weapons(0);loading=0;
}
static int load_path(const char *path,int ask){
    D3Buffer bytes;D3Game *parsed;const char *error;char full[260];DWORD n;
    if(ask && !discard())return 0;
    n=GetFullPathNameA(path,260,full,0);if(!n || n>=260){error_box("Ungueltiger oder zu langer ANSI-Dateipfad.");return 0;}
    if(!d3_read_file(full,&bytes,&error)){error_box(error);return 0;}
    parsed=(D3Game*)d3_alloc(sizeof(*parsed));if(!parsed){d3_free(bytes.data);error_box("Nicht genug Arbeitsspeicher.");return 0;}
    if(!d3_parse(parsed,bytes.data,bytes.size)){error_box(parsed->error);d3_free(parsed);d3_free(bytes.data);return 0;}
    d3_free(game);d3_free(opened.data);game=parsed;opened=bytes;d3_string(opened_path,260,full);d3_sha256(bytes.data,bytes.size,original_hash);load_values();set_status("Spielstand geladen. Original unveraendert.");return 1;
}
static int dialog_path(char path[260],int save,const char *title,const char *filter){
    OPENFILENAMEA ofn;BOOL ok;d3_zero(&ofn,sizeof(ofn));
    /* Win9x-compatible v4 layout; newer headers have a larger structure. */
    ofn.lStructSize=76;ofn.hwndOwner=main_window;ofn.lpstrFile=path;ofn.nMaxFile=260;ofn.lpstrFilter=filter;ofn.nFilterIndex=1;ofn.lpstrTitle=title;
    ofn.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR|(save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);
    ok=save?GetSaveFileNameA(&ofn):GetOpenFileNameA(&ofn);
    if(!ok && CommDlgExtendedError())error_box("Der Dateidialog konnte nicht geoeffnet werden.");return ok!=0;
}
static void choose_open(void){char path[260];path[0]=0;if(dialog_path(path,0,"Descent-3-Spielstand oeffnen (z.B. saveg000)","Spielstaende (saveg*)\0saveg*\0Alle Dateien\0*.*\0\0"))load_path(path,1);}
static int read_float(HWND h,const char *initial,float original,float *value){char s[64];get_text(h,s,64);if(d3_string_equal(s,initial)){*value=original;return 1;}return d3_number(s,value);}
static int read_options(D3Options *o){
    char s[64];D3I32 n;int i;d3_current(game,o);
    if(!read_float(energy,energy_text,o->energy,&o->energy) || !read_float(shields,shield_text,o->shields,&o->shields) || !read_float(afterburner,ab_text,o->afterburner,&o->afterburner))return 0;
    get_text(lives,s,64);if(!d3_integer(s,0,255,&n))return 0;o->lives=n;get_text(score,s,64);if(!d3_integer(s,(-2147483647-1),2147483647,&n))return 0;o->score=n;
    for(i=0;i<20;i++){o->owned[i]=(D3U8)checked(owned[i]);get_text(ammo[i],s,64);if(!d3_integer(s,0,65535,&n))return 0;o->ammo[i]=(D3U16)n;}
    o->primary=(int)SendMessageA(primary,CB_GETCURSEL,0,0);i=(int)SendMessageA(secondary,CB_GETCURSEL,0,0);if(i>0)o->secondary=i+9;
    o->god=god_request;o->cloak=cloak_request;o->cooler=cooler_request;o->converter=converter_request;o->quad=quad_request;o->reveal_map=checked(full_map);return 1;
}
static int build_output(D3Buffer *out){
    D3Options o;char s[4096],n[48];int i;
    if(!game)return 0;if(!read_options(&o)){error_box("Ungueltige Zahl. Dezimalpunkt oder -komma ist erlaubt; Munition 0 bis 65535, Leben 0 bis 255.");return 0;}
    if(!d3_edit(game,&o,out)){error_box(game->error);return 0;}
    file_report(s,4096);d3_append(s,4096,"\r\nGEPRUEFTE AUSGABE\r\nGroesse: ");append_number(s,4096,game->size);d3_append(s,4096," -> ");append_number(s,4096,out->size);d3_append(s,4096," Byte\r\n");
    if(out->size==game->size && d3_equal(out->data,game->data,out->size))d3_append(s,4096,"Keine Aenderung. Ausgabe ist byte-identisch.\r\n");
    else{
        d3_append(s,4096,"Energie: ");d3_format_float(n,o.energy);d3_append(s,4096,n);d3_append(s,4096," | Schild: ");d3_format_float(n,o.shields);d3_append(s,4096,n);d3_append(s,4096,"\r\n");
        for(i=0;i<20;i++)if(o.owned[i]){d3_append(s,4096,d3_weapon_names[i]);d3_append(s,4096,": ");append_number(s,4096,o.ammo[i]);d3_append(s,4096,"\r\n");}
        if(o.god>=0)d3_append(s,4096,o.god?"God-Mode: lange Restdauer\r\n":"God-Mode: aus\r\n");
        if(o.cloak>=0)d3_append(s,4096,o.cloak?"Tarnung: lange Restdauer\r\n":"Tarnung: aus\r\n");
        if(o.cooler>=0)d3_append(s,4096,o.cooler?"Kuehler hinzufuegen\r\n":"Kuehler entfernen\r\n");
        if(o.converter>=0)d3_append(s,4096,o.converter?"Konverter hinzufuegen\r\n":"Konverter entfernen\r\n");
        if(o.quad>=0)d3_append(s,4096,o.quad?"Quad-Laser an\r\n":"Quad-Laser aus\r\n");if(o.reveal_map)d3_append(s,4096,"Karte vollstaendig aufdecken\r\n");
    }
    d3_append(s,4096,"\r\nAusgabe wurde erneut erfolgreich eingelesen.\r\nNoch kein Ingame-Test mit echtem Spielstand.");text(review,s);set_status("Pruefung bestanden. Ausgabe konnte erneut gelesen werden.");return 1;
}
static void save_game(int copy){
    char path[260],s[900];D3Buffer out;D3WriteReport report;
    if(!build_output(&out))return;d3_string(path,260,opened_path);
    if(copy){d3_append(path,260,".edited");if(!dialog_path(path,1,"Bearbeiteten Spielstand als Kopie speichern","Alle Dateien\0*.*\0\0")){d3_free(out.data);return;}}
    if(!d3_write_file(opened_path,original_hash,path,&out,&report)){
        d3_string(s,900,report.error);if(report.recovery[0]){d3_append(s,900,"\r\nOriginal-Backup: ");d3_append(s,900,report.recovery);}error_box(s);d3_free(out.data);return;
    }
    /* Install the already validated output snapshot without discarding edits. */
    d3_free(opened.data);opened=out;d3_parse(game,opened.data,opened.size);d3_string(opened_path,260,path);d3_sha256(opened.data,opened.size,original_hash);load_values();
    d3_string(s,900,"Gespeichert.");if(report.backup[0]){d3_append(s,900," Backup: ");d3_append(s,900,report.backup);}set_status(s);
}
static void choose_limits(void){
    char path[260];D3U16 parsed[21];const char *error;if(!game)return;path[0]=0;
    if(!dialog_path(path,0,"Table.gam, d3.hog oder ein Missionsarchiv waehlen","Spieldaten\0*.gam;*.hog;*.mn3\0Alle Dateien\0*.*\0\0"))return;
    if(!d3_load_limits_file(path,d3_ship(game),parsed,&error)){error_box(error);return;}d3_copy(caps,parsed,42);d3_string(caps_ship,128,d3_ship(game));caps_loaded=1;refresh_weapons(0);set_status("Schiffslimits geladen. Max verwendet jetzt diese normalen Kapazitaeten.");
}
static void maximum_one(int i,int storage){char s[24];d3_format_u32(s,(!storage && use_caps())?caps[i]:65535);text(ammo[i],s);changed();}
static void select_all(int first,int on){int i;for(i=first;i<first+10;i++){check(owned[i],on);if(!on)text(ammo[i],"0");}refresh_weapons(1);changed();}
static void max_all(int first,int storage){int i;if(!storage && !use_caps()){error_box("Bitte zuerst passende Schiffslimits aus Table.gam / d3.hog laden.");return;}for(i=first;i<first+10;i++)if(checked(owned[i]) && uses_ammo(i))maximum_one(i,storage);}
static void help(void){
    MessageBoxA(main_window,"Descent 3 Savegame Editor - Windows 98\r\n\r\n1. Im Spiel speichern und das Spiel beenden.\r\n2. saveg000 bis saveg007 im Ordner savegame oeffnen.\r\n3. Werte aendern und Speichern + Backup waehlen.\r\n4. Im Spiel neu laden.\r\n\r\nJede ueberschriebene Datei wird zuvor als .bak gesichert.\r\nKopien mit .edited zum Laden passend in savegNNN umbenennen.\r\nAuf FAT ist der Austausch nicht transaktional: Bei Stromausfall\r\ndas Backup ggf. wieder unter den Slotnamen kopieren.\r\n\r\nMax: geladene Schiffslimits, sonst Speichermaximum 65.535.\r\nErneuern: God-Mode / Tarnung mit 1.000.000.000 s Restdauer.\r\nLevelwechsel und Skripte koennen Effekte aufheben.\r\n\r\nGrundlage: d3-historical, Spielerblock 496 Byte, Format 0-2.\r\nNative ANSI-Anwendung ohne .NET oder zusaetzliche Runtime.\r\nNoch nicht unter echtem Windows 98 oder im Spiel erprobt.\r\n\r\nGPL-3.0-or-later. Quelltext und Fortsetzungsnotiz im Paket.","Anleitung",MB_OK|MB_ICONINFORMATION);
}
static void player_page(void){
    group("Versorgung",24,146,338,181,0);
    label("Energie",38,170,100,18,0);energy=edit(145,166,100,ID_ENERGY,0,0);button("Max 200",255,165,92,ID_ENERGY_MAX,0,1);
    label("Schild",38,198,100,18,0);shields=edit(145,194,100,ID_SHIELDS,0,0);button("Max 200",255,193,92,ID_SHIELD_MAX,0,1);
    label("Nachbrenner (s)",38,226,100,18,0);afterburner=edit(145,222,100,ID_AB,0,0);button("Voll (5 s)",255,221,92,ID_AB_MAX,0,1);
    label("Leben",38,254,100,18,0);lives=edit(145,250,100,ID_LIVES,0,1);
    label("Punktestand",38,282,100,18,0);score=edit(145,278,155,ID_SCORE,0,0);
    group("Ausruestung",24,335,338,119,0);cooler=checkbox("Nachbrenner-Kuehler",38,356,300,ID_COOLER,0);converter=checkbox("Energie -> Schild-Konverter",38,384,300,ID_CONVERTER,0);quad=checkbox("Quad-Laser",38,412,300,ID_QUAD,0);
    group("Effekte",378,146,356,151,0);god=checkbox("God-Mode / unverwundbar",392,167,240,ID_GOD,0);button("Erneuern",640,165,79,ID_GOD_RENEW,0,1);god_time=label("",392,191,327,18,0);
    cloak=checkbox("Cloaked-Mode / getarnt",392,226,240,ID_CLOAK,0);button("Erneuern",640,224,79,ID_CLOAK_RENEW,0,1);cloak_time=label("",392,250,327,18,0);
    group("Karte",378,308,356,146,0);full_map=checkbox("Gesamte Karte aufdecken",392,331,325,ID_MAP,0);map_info=label("",392,365,326,55,0);
}
static void weapon_page(int first,int page){
    int i,y;HWND combo;label("Aktive Waffe:",28,151,100,18,page);
    combo=control("COMBOBOX","",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,131,147,230,200,first?ID_SECONDARY:ID_PRIMARY,page,1);
    if(first){secondary=combo;SendMessageA(combo,CB_ADDSTRING,0,(LPARAM)"Keine Sekundaerwaffe");}else primary=combo;
    for(i=first;i<first+10;i++)SendMessageA(combo,CB_ADDSTRING,0,(LPARAM)d3_weapon_names[i]);
    label("Vorhanden",28,181,75,18,page);label("Waffe",110,181,180,18,page);label("Munition / Stueck",298,181,115,18,page);label("Max",423,181,65,18,page);label("Schiffslimit",512,181,185,18,page);
    for(i=first;i<first+10;i++){
        y=201+(i-first)*23;owned[i]=checkbox("",51,y,35,ID_OWN+i,page);label(d3_weapon_names[i],110,y+2,180,18,page);
        ammo[i]=edit(298,y-1,106,ID_AMMO+i,page,1);SendMessageA(ammo[i],EM_LIMITTEXT,5,0);
        max_buttons[i]=control("BUTTON","Max",BS_PUSHBUTTON|WS_TABSTOP,420,y-1,70,22,ID_MAX+i,page,1);cap_labels[i]=label("nicht geladen",512,y+2,185,18,page);
    }
    button("Alle auswaehlen",28,437,130,first?ID_ALL_SEC:ID_ALL_PRI,page,1);button("Alle abwaehlen",166,437,130,first?ID_NONE_SEC:ID_NONE_PRI,page,1);
    button("Vorhandene: Schiff-Max",304,437,204,first?ID_CAP_SEC:ID_CAP_PRI,page,1);button("Vorhandene: 65.535",516,437,204,first?ID_STORE_SEC:ID_STORE_PRI,page,1);
}
static void make_ui(void){
    TCITEMA item;HWND heading;int i;const char *names[4]={"Spieler / Ausruestung","Primaerwaffen","Sekundaerwaffen","Datei / Pruefung"};
    font=(HFONT)GetStockObject(DEFAULT_GUI_FONT);title_font=CreateFontA(-18,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_SWISS,"MS Sans Serif");
    heading=label("DESCENT 3  -  SAVEGAME EDITOR / WINDOWS 98",12,10,736,25,-1);SendMessageA(heading,WM_SETFONT,(WPARAM)title_font,TRUE);
    button("Oeffnen...",12,42,105,ID_OPEN,-1,0);button("Speichern + Backup",123,42,144,ID_SAVE,-1,1);button("Kopie speichern...",273,42,128,ID_COPY,-1,1);
    button("Originalwerte",407,42,105,ID_RESET,-1,1);button("Schiffslimits laden...",518,42,150,ID_LIMITS,-1,1);button("Hilfe",674,42,74,ID_HELP,-1,0);
    info=label("Noch kein Spielstand geoeffnet. Im Spiel speichern, dann saveg000 usw. oeffnen.",12,76,736,34,-1);
    tab=control(WC_TABCONTROLA,"",WS_TABSTOP|WS_CLIPSIBLINGS,12,114,736,355,ID_TAB,-1,0);
    d3_zero(&item,sizeof(item));item.mask=TCIF_TEXT;for(i=0;i<4;i++){item.pszText=(LPSTR)names[i];SendMessageA(tab,TCM_INSERTITEMA,(WPARAM)i,(LPARAM)&item);}
    player_page();weapon_page(0,1);weapon_page(10,2);
    button("Aenderungen pruefen",28,147,178,ID_CHECK,3,1);
    review=control("EDIT","Geplante Aenderungen und Dateidaten erscheinen hier.\r\n\r\nDer Editor bearbeitet vorhandene Spielstaende; er erzeugt keinen\r\nvollstaendigen Levelzustand allein aus dem Quelltext.",ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL|WS_HSCROLL,28,180,701,276,0,3,0);
    hint=label("",12,475,736,16,-1);status=label("Vorschau: noch kein Test unter echtem Windows 98 oder im Spiel.",12,495,736,32,-1);
    enable_controls();selected_page=0;apply_visibility();
}
static LRESULT CALLBACK window_proc(HWND hwnd,UINT message,WPARAM w,LPARAM l){
    int id,code,i,selected;D3Buffer out;NMHDR *notify;
    switch(message){
    case WM_CREATE:main_window=hwnd;make_ui();return 0;
    case WM_NOTIFY:notify=(NMHDR*)l;if(notify->idFrom==ID_TAB && notify->code==TCN_SELCHANGE){selected_page=(int)SendMessageA(tab,TCM_GETCURSEL,0,0);apply_visibility();}return 0;
    case WM_COMMAND:
        id=LOWORD(w);code=HIWORD(w);if(loading)return 0;
        if(id==ID_OPEN){choose_open();return 0;}if(id==ID_HELP){help();return 0;}if(!game)return 0;
        if(code==EN_CHANGE){if((id>=ID_ENERGY && id<=ID_SCORE) || (id>=ID_AMMO && id<ID_AMMO+20))changed();return 0;}
        if(id==ID_PRIMARY && code==CBN_SELCHANGE){selected=(int)SendMessageA(primary,CB_GETCURSEL,0,0);if(selected>=0 && selected<10)check(owned[selected],1);refresh_weapons(1);changed();return 0;}
        if(id==ID_SECONDARY && code==CBN_SELCHANGE){selected=(int)SendMessageA(secondary,CB_GETCURSEL,0,0);if(selected==0)select_all(10,0);else if(selected>0){check(owned[selected+9],1);refresh_weapons(1);changed();}return 0;}
        if(code!=BN_CLICKED)return 0;
        if(id>=ID_OWN && id<ID_OWN+20){i=id-ID_OWN;if(!checked(owned[i]))text(ammo[i],"0");refresh_weapons(1);changed();return 0;}
        if(id>=ID_MAX && id<ID_MAX+20){maximum_one(id-ID_MAX,0);return 0;}
        switch(id){
        case ID_SAVE:save_game(0);break;case ID_COPY:save_game(1);break;case ID_RESET:load_values();set_status("Originalwerte geladen.");break;case ID_LIMITS:choose_limits();break;
        case ID_CHECK:if(build_output(&out))d3_free(out.data);break;
        case ID_ENERGY_MAX:text(energy,"200");changed();break;case ID_SHIELD_MAX:text(shields,"200");changed();break;case ID_AB_MAX:text(afterburner,"5");changed();break;
        case ID_GOD:god_request=checked(god);changed();break;case ID_CLOAK:cloak_request=checked(cloak);changed();break;
        case ID_GOD_RENEW:check(god,1);god_request=1;changed();break;case ID_CLOAK_RENEW:check(cloak,1);cloak_request=1;changed();break;
        case ID_COOLER:cooler_request=checked(cooler);changed();break;case ID_CONVERTER:converter_request=checked(converter);changed();break;case ID_QUAD:quad_request=checked(quad);changed();break;case ID_MAP:changed();break;
        case ID_ALL_PRI:select_all(0,1);break;case ID_NONE_PRI:select_all(0,0);break;case ID_ALL_SEC:select_all(10,1);break;case ID_NONE_SEC:select_all(10,0);break;
        case ID_CAP_PRI:max_all(0,0);break;case ID_STORE_PRI:max_all(0,1);break;case ID_CAP_SEC:max_all(10,0);break;case ID_STORE_SEC:max_all(10,1);break;
        }return 0;
    case WM_CLOSE:if(discard())DestroyWindow(hwnd);return 0;
    case WM_DESTROY:if(title_font)DeleteObject(title_font);PostQuitMessage(0);return 0;
    }
    return DefWindowProcA(hwnd,message,w,l);
}
/* Deliberate custom entry point: no MinGW CRT startup or runtime imports. */
void WINAPI WinMainCRTStartup(void){
    WNDCLASSA wc;RECT rect,work;MSG message;DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN;int x,y;const char *cmd;char argument[260];D3U32 n=0;
    instance=GetModuleHandleA(0);InitCommonControls();d3_zero(&wc,sizeof(wc));wc.style=CS_HREDRAW|CS_VREDRAW;wc.lpfnWndProc=window_proc;wc.hInstance=instance;
    wc.hIcon=LoadIconA(0,IDI_APPLICATION);wc.hCursor=LoadCursorA(0,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);wc.lpszClassName="D3SaveEditor98";
    if(!RegisterClassA(&wc))ExitProcess(1);rect.left=rect.top=0;rect.right=760;rect.bottom=532;AdjustWindowRectEx(&rect,style,FALSE,WS_EX_CONTROLPARENT);
    work.left=work.top=0;work.right=GetSystemMetrics(SM_CXSCREEN);work.bottom=GetSystemMetrics(SM_CYSCREEN);
    SystemParametersInfoA(SPI_GETWORKAREA,0,&work,0);
    x=work.left+(work.right-work.left-(rect.right-rect.left))/2;y=work.top+(work.bottom-work.top-(rect.bottom-rect.top))/2;if(x<0)x=0;if(y<0)y=0;
    main_window=CreateWindowExA(WS_EX_CONTROLPARENT,wc.lpszClassName,"Descent 3 Savegame Editor - Windows 98",style,x,y,rect.right-rect.left,rect.bottom-rect.top,0,0,instance,0);
    if(!main_window)ExitProcess(1);ShowWindow(main_window,SW_SHOW);UpdateWindow(main_window);
    cmd=GetCommandLineA();if(*cmd=='"'){cmd++;while(*cmd && *cmd!='"')cmd++;if(*cmd)cmd++;}else while(*cmd && *cmd!=' ')cmd++;
    while(*cmd==' ')cmd++;if(*cmd=='"'){cmd++;while(*cmd && *cmd!='"' && n<259)argument[n++]=*cmd++;}else while(*cmd && *cmd!=' ' && n<259)argument[n++]=*cmd++;argument[n]=0;
    if(n)load_path(argument,0);
    while(GetMessageA(&message,0,0,0)>0){if(!IsDialogMessageA(main_window,&message)){TranslateMessage(&message);DispatchMessageA(&message);}}
    d3_free(opened.data);d3_free(game);ExitProcess(0);
}
