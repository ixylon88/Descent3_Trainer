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
ID_OWN=200,ID_AMMO=240,ID_MAX=280,
ID_CARGO_COUNT=320,ID_COUNTERMEASURE_COUNT=330,ID_SHIP_UNLOCK=340,ID_BLACK_PYRO_UNLOCK=350,ID_BLACK_PYRO_PREFER};
typedef struct {HWND hwnd;int page,needs_file;} Widget;
static HINSTANCE instance;
static HWND main_window,tab,info,status,hint,review;
static HFONT font,title_font;
static Widget widgets[256];static int widget_count,selected_page,loading,dirty;
static HWND energy,shields,afterburner,lives,score,god,cloak,cooler,converter,quad,full_map,god_time,cloak_time,map_info;
static HWND god_renew,cloak_renew;
static HWND owned[20],ammo[20],max_buttons[20],cap_labels[20],primary,secondary;
static HWND cargo_counts[D3_CARGO_TYPES],countermeasure_counts[D3_COUNTERMEASURE_TYPES];
static HWND unlock_ships[D3_STANDARD_SHIPS],saved_inventory;
static HWND unlock_black_pyro,prefer_black_pyro_button;
static int prefer_black_pyro;
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
static void effect_label(HWND h,int request,int on,float duration){char s[100],n[48];if(request>=0){text(h,request?"Beim Speichern: 1.000.000.000 s Restdauer":"Beim Speichern: inaktiv");return;}if(!on){text(h,"Gespeichert: inaktiv");return;}if(duration>=1e8f){text(h,"Gespeichert: lange Restdauer");return;}d3_string(s,100,"Gespeichert: ");d3_format_float(n,duration);d3_append(s,100,n);d3_append(s,100," s Restdauer");text(h,s);}
static void refresh_effect_timers(void) {
    effect_label(god_time,god_request,d3_god(game),d3_float(game->data+game->player+116));
    effect_label(cloak_time,cloak_request,d3_cloak(game),game->object.has_effect?d3_float(game->data+game->object.effect+12):0);
    EnableWindow(god_renew,d3_god(game) && god_request<0);
    EnableWindow(cloak_renew,d3_cloak(game) && cloak_request<0);
}
static void file_report(char *out,D3U32 cap){
    int i;char hash[65];const char *hex="0123456789abcdef";
    for(i=0;i<32;i++){hash[2*i]=hex[original_hash[i]>>4];hash[2*i+1]=hex[original_hash[i]&15];}hash[64]=0;
    d3_string(out,cap,"DATEI\r\n");d3_append(out,cap,opened_path);d3_append(out,cap,"\r\n\r\nSHA-256: ");d3_append(out,cap,hash);
    d3_append(out,cap,"\r\nFormat: ");append_number(out,cap,(D3U32)game->version);d3_append(out,cap,"; historischer Spielerblock: 496 Byte\r\nSpieler: ");d3_append(out,cap,game->callsign);
    d3_append(out,cap,"\r\nSchiff: ");d3_append(out,cap,d3_ship(game));
    d3_append(out,cap,"\r\nInventararten: ");append_number(out,cap,(D3U32)game->inventory.count);
    d3_append(out,cap," | Gegenmassnahmen: ");append_number(out,cap,(D3U32)game->countermeasures.count);d3_append(out,cap," Arten\r\n");
}
static void load_supply_counts(HWND *inputs,int countermeasure,int size) {
    int i,count;
    char value[24];
    for (i=0; i<size; i++) {
        count=d3_supply_count(game,countermeasure,i);
        EnableWindow(inputs[i],count>=0);
        if (count<0) text(inputs[i],"-");
        else {
            d3_format_u32(value,(D3U32)count);
            text(inputs[i],value);
        }
    }
}
static void add_inventory_rows(const D3Inventory *inventory,const char *group_name) {
    int i;
    for (i=0; i<inventory->count; i++) {
        const D3Item *item=&inventory->items[i];
        char row[600],id[24];
        d3_string(row,sizeof(row),group_name);
        if (item->name[0]) d3_append(row,sizeof(row),item->name);
        else {
            d3_append(row,sizeof(row),"ID ");
            d3_format_i32(id,item->id<0 ? item->original_id : item->id);
            d3_append(row,sizeof(row),id);
        }
        d3_append(row,sizeof(row)," x ");
        append_number(row,sizeof(row),(D3U32)item->count);
        SendMessageA(saved_inventory,LB_ADDSTRING,0,(LPARAM)row);
    }
}
static void refresh_ship_controls(void) {
    int i,standard_checked=0;
    D3U32 bit,permissions=d3_ship_permissions(game);
    for (i=0; i<D3_STANDARD_SHIPS; i++) {
        bit=d3_ship_permission_bit(game,d3_standard_ships[i]);
        EnableWindow(unlock_ships[i],bit!=0 && (prefer_black_pyro || !(permissions & bit)));
        if (checked(unlock_ships[i])) standard_checked=1;
    }
    bit=d3_ship_permission_bit(game,D3_BLACK_PYRO);
    EnableWindow(unlock_black_pyro,!prefer_black_pyro && bit!=0 && !(permissions & bit));
    EnableWindow(prefer_black_pyro_button,bit!=0 &&
        ((!prefer_black_pyro && !d3_prefers_black_pyro(game)) || standard_checked));
}
static void select_black_pyro(void) {
    int i;
    if (!d3_ship_permission_bit(game,D3_BLACK_PYRO)) return;
    prefer_black_pyro=1;
    for (i=0; i<D3_STANDARD_SHIPS; i++) check(unlock_ships[i],0);
    check(unlock_black_pyro,1);
    refresh_ship_controls();
    changed();
}
static void load_inventory_values(void) {
    int i;
    D3U32 bit,permissions=d3_ship_permissions(game);
    char name[64];
    load_supply_counts(cargo_counts,0,D3_CARGO_TYPES);
    load_supply_counts(countermeasure_counts,1,D3_COUNTERMEASURE_TYPES);
    for (i=0; i<D3_STANDARD_SHIPS; i++) {
        bit=d3_ship_permission_bit(game,d3_standard_ships[i]);
        check(unlock_ships[i],(permissions & bit)!=0);
        d3_string(name,sizeof(name),d3_standard_ships[i]);
        if (!bit) d3_append(name,sizeof(name)," (fehlt)");
        text(unlock_ships[i],name);
    }
    bit=d3_ship_permission_bit(game,D3_BLACK_PYRO);
    check(unlock_black_pyro,(permissions & bit)!=0);
    text(unlock_black_pyro,bit ? D3_BLACK_PYRO : D3_BLACK_PYRO " (fehlt)");
    prefer_black_pyro=0;
    refresh_ship_controls();
    SendMessageA(saved_inventory,LB_RESETCONTENT,0,0);
    add_inventory_rows(&game->inventory,"Inventar: ");
    add_inventory_rows(&game->countermeasures,"Gegenmassnahme: ");
    if (!game->inventory.count && !game->countermeasures.count)
        SendMessageA(saved_inventory,LB_ADDSTRING,0,(LPARAM)"Inventar und Gegenmassnahmen sind leer.");
}
static void load_values(void){
    D3Options o;int i,visible=0,has_second=0;char s[1400],n[48];
    if(!game)return;loading=1;enable_controls();d3_current(game,&o);
    d3_format_float(energy_text,o.energy);d3_format_float(shield_text,o.shields);d3_format_float(ab_text,o.afterburner);text(energy,energy_text);text(shields,shield_text);text(afterburner,ab_text);
    d3_format_u32(n,(D3U32)o.lives);text(lives,n);d3_format_i32(n,o.score);text(score,n);
    check(god,d3_god(game));check(cloak,d3_cloak(game));check(cooler,d3_has_item(game,game->cooler_id));check(converter,d3_has_item(game,game->converter_id));check(quad,d3_quad(game));check(full_map,0);
    EnableWindow(cooler,game->cooler_id>=0);EnableWindow(converter,game->converter_id>=0);EnableWindow(quad,game->quad_id>=0 && game->object.batteries==21);
    load_inventory_values();
    for(i=0;i<20;i++){check(owned[i],o.owned[i]);d3_format_u32(n,o.ammo[i]);text(ammo[i],n);if(i>=10 && o.owned[i])has_second=1;}
    SendMessageA(primary,CB_SETCURSEL,o.primary,0);SendMessageA(secondary,CB_SETCURSEL,has_second?(WPARAM)(o.secondary-9):0,0);
    god_request=cloak_request=cooler_request=converter_request=quad_request=-1;
    refresh_effect_timers();
    for(i=0;i<=game->highest_room;i++)if(game->data[game->map+i])visible++;
    d3_string(s,1400,"Gespeichert sichtbar: ");append_number(s,1400,(D3U32)visible);d3_append(s,1400," von ");append_number(s,1400,(D3U32)(game->highest_room+1));d3_append(s,1400," Raumplaetzen.\r\nOhne Haken bleibt die Erkundung erhalten.");text(map_info,s);
    d3_string(s,1400,opened_path);d3_append(s,1400,"\r\n");d3_append(s,1400,game->callsign);d3_append(s,1400," | ");d3_append(s,1400,d3_ship(game));d3_append(s,1400," | ");d3_append(s,1400,game->mission);d3_append(s,1400," / Level ");append_number(s,1400,(D3U32)game->level);text(info,s);
    file_report(s,1400);d3_append(s,1400,"\r\nKeine geplanten Aenderungen.");text(review,s);
    dirty=0;SetWindowTextA(main_window,"Descent 3 Savegame Editor - Windows 98");refresh_weapons(0);loading=0;
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
static int read_supply_counts(HWND *inputs,int countermeasure,const D3Supply *items,int size,int *counts) {
    int i,current,maximum;
    D3I32 value;
    char input[24];
    for (i=0; i<size; i++) {
        current=d3_supply_count(game,countermeasure,i);
        if (current<0) continue;
        maximum=current>items[i].maximum ? current : items[i].maximum;
        get_text(inputs[i],input,sizeof(input));
        if (!d3_integer(input,0,maximum,&value)) return 0;
        if (value!=current) counts[i]=value;
    }
    return 1;
}
static int read_options(D3Options *o){
    char s[64];D3I32 n;D3U32 bit;int i;d3_current(game,o);
    if(!read_float(energy,energy_text,o->energy,&o->energy) || !read_float(shields,shield_text,o->shields,&o->shields) || !read_float(afterburner,ab_text,o->afterburner,&o->afterburner))return 0;
    get_text(lives,s,64);if(!d3_integer(s,0,255,&n))return 0;o->lives=n;get_text(score,s,64);if(!d3_integer(s,(-2147483647-1),2147483647,&n))return 0;o->score=n;
    for(i=0;i<20;i++){o->owned[i]=(D3U8)checked(owned[i]);get_text(ammo[i],s,64);if(!d3_integer(s,0,65535,&n))return 0;o->ammo[i]=(D3U16)n;}
    if (!read_supply_counts(cargo_counts,0,d3_cargo_items,D3_CARGO_TYPES,o->cargo_counts) ||
        !read_supply_counts(countermeasure_counts,1,d3_countermeasure_types,D3_COUNTERMEASURE_TYPES,o->countermeasure_counts)) return 0;
    o->prefer_black_pyro=(D3U8)prefer_black_pyro;
    for (i=0; i<D3_STANDARD_SHIPS; i++) {
        bit=d3_ship_permission_bit(game,d3_standard_ships[i]);
        o->unlock_ships[i]=(D3U8)(bit!=0 && (prefer_black_pyro || !(d3_ship_permissions(game) & bit)) && checked(unlock_ships[i]));
    }
    bit=d3_ship_permission_bit(game,D3_BLACK_PYRO);
    o->unlock_black_pyro=(D3U8)(!o->prefer_black_pyro && bit!=0 && !(d3_ship_permissions(game) & bit) && checked(unlock_black_pyro));
    o->primary=(int)SendMessageA(primary,CB_GETCURSEL,0,0);i=(int)SendMessageA(secondary,CB_GETCURSEL,0,0);if(i>0)o->secondary=i+9;
    o->god=god_request;o->cloak=cloak_request;o->cooler=cooler_request;o->converter=converter_request;o->quad=quad_request;o->reveal_map=checked(full_map);return 1;
}
static void report_supply_counts(char *report,D3U32 size,const D3Supply *items,const int *counts,int length) {
    int i;
    for (i=0; i<length; i++) {
        if (counts[i]<0) continue;
        d3_append(report,size,items[i].name);d3_append(report,size,": ");
        append_number(report,size,(D3U32)counts[i]);d3_append(report,size,"\r\n");
    }
}
static int build_output(D3Buffer *out){
    D3Options o;char s[4096],n[48];int i;
    if(!game)return 0;if(!read_options(&o)){error_box("Ungueltige Zahl. Bitte Spielerwerte, Munition und Inventarmengen pruefen.");return 0;}
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
        report_supply_counts(s,sizeof(s),d3_cargo_items,o.cargo_counts,D3_CARGO_TYPES);
        report_supply_counts(s,sizeof(s),d3_countermeasure_types,o.countermeasure_counts,D3_COUNTERMEASURE_TYPES);
        for (i=0; i<D3_STANDARD_SHIPS; i++) {
            if (!o.unlock_ships[i]) continue;
            d3_append(s,sizeof(s),d3_standard_ships[i]);d3_append(s,sizeof(s)," freischalten\r\n");
        }
        if (o.prefer_black_pyro) d3_append(s,sizeof(s),"Schiffsfreigaben auf Black Pyro zuruecksetzen und angewaehlte Schiffe erlauben\r\n");
        else if (o.unlock_black_pyro) d3_append(s,sizeof(s),"Black Pyro freischalten\r\n");
    }
    d3_append(s,4096,"\r\nAusgabe wurde erneut erfolgreich eingelesen.");text(review,s);set_status("Pruefung bestanden. Ausgabe konnte erneut gelesen werden.");return 1;
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
    MessageBoxA(main_window,"Descent 3 Savegame Editor - Windows 98\r\n\r\n1. Im Spiel speichern und das Spiel beenden.\r\n2. saveg000 bis saveg007 im Ordner savegame oeffnen.\r\n3. Werte aendern und Speichern + Backup waehlen.\r\n4. Im Spiel neu laden.\r\n\r\nJede ueberschriebene Datei wird zuvor als .bak gesichert.\r\nKopien mit .edited zum Laden passend in savegNNN umbenennen.\r\nBei unterbrochenem Dateiaustausch das Backup unter den Slotnamen kopieren.\r\n\r\nInventar / Schiffe bearbeitet Bestaende und Freigaben fuer das Briefing.\r\nMax nutzt das geladene Schiffslimit, sonst 65.535.\r\nVerlaengern setzt 1.000.000.000 s Restdauer fuer einen aktiven Effekt.\r\n\r\nGrundlage: d3-historical, Spielerblock 496 Byte, Format 0-2.\r\nGPL-3.0-or-later. Quelltext und Anleitung stehen im Repository.","Anleitung",MB_OK|MB_ICONINFORMATION);
}
static void player_page(void){
    group("Versorgung",24,146,338,181,0);
    label("Energie",38,170,100,18,0);energy=edit(145,166,100,ID_ENERGY,0,0);button("Max 200",255,165,92,ID_ENERGY_MAX,0,1);
    label("Schild",38,198,100,18,0);shields=edit(145,194,100,ID_SHIELDS,0,0);button("Max 200",255,193,92,ID_SHIELD_MAX,0,1);
    label("Nachbrenner (s)",38,226,100,18,0);afterburner=edit(145,222,100,ID_AB,0,0);button("Voll (5 s)",255,221,92,ID_AB_MAX,0,1);
    label("Leben",38,254,100,18,0);lives=edit(145,250,100,ID_LIVES,0,1);
    label("Punktestand",38,282,100,18,0);score=edit(145,278,155,ID_SCORE,0,0);
    group("Ausruestung",24,335,338,119,0);cooler=checkbox("Nachbrenner-Kuehler",38,356,300,ID_COOLER,0);converter=checkbox("Energie -> Schild-Konverter",38,384,300,ID_CONVERTER,0);quad=checkbox("Quad-Laser",38,412,300,ID_QUAD,0);
    group("Effekte",378,146,356,151,0);god=checkbox("God-Mode / unverwundbar",392,167,213,ID_GOD,0);god_renew=button("Verlaengern",610,165,109,ID_GOD_RENEW,0,1);god_time=label("",392,191,327,18,0);
    cloak=checkbox("Cloaked-Mode / getarnt",392,226,213,ID_CLOAK,0);cloak_renew=button("Verlaengern",610,224,109,ID_CLOAK_RENEW,0,1);cloak_time=label("",392,250,327,18,0);
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
static void inventory_page(int page) {
    int i,y;
    char name[60];
    group("Gegenstaende",24,146,218,166,page);
    for (i=0; i<D3_CARGO_TYPES; i++) {
        y=172+i*25;
        label(d3_cargo_items[i].name,36,y+2,115,18,page);
        cargo_counts[i]=edit(153,y,77,ID_CARGO_COUNT+i,page,1);
        SendMessageA(cargo_counts[i],EM_LIMITTEXT,7,0);
    }
    label("Kuehler, Konverter und Quad-Laser\r\nstehen im Spieler-Tab.",36,231,194,34,page);
    group("Gegenmassnahmen (Limit)",249,146,256,166,page);
    for (i=0; i<D3_COUNTERMEASURE_TYPES; i++) {
        y=172+i*25;
        d3_string(name,sizeof(name),d3_countermeasure_types[i].name);
        d3_append(name,sizeof(name)," (");
        append_number(name,sizeof(name),(D3U32)d3_countermeasure_types[i].maximum);
        d3_append(name,sizeof(name),")");
        label(name,261,y+2,137,18,page);
        countermeasure_counts[i]=edit(408,y,82,ID_COUNTERMEASURE_COUNT+i,page,1);
        SendMessageA(countermeasure_counts[i],EM_LIMITTEXT,7,0);
    }
    group("Schiffsfreigaben",513,146,221,166,page);
    for (i=0; i<D3_STANDARD_SHIPS; i++)
        unlock_ships[i]=checkbox(d3_standard_ships[i],525,166+i*20,193,ID_SHIP_UNLOCK+i,page);
    unlock_black_pyro=checkbox(D3_BLACK_PYRO,525,226,193,ID_BLACK_PYRO_UNLOCK,page);
    prefer_black_pyro_button=button("Nur Black Pyro erlauben",525,250,193,ID_BLACK_PYRO_PREFER,page,1);
    label("Der Black Pyro fehlt im Auswahlmenue. Der Knopf waehlt alle anderen Schiffe ab.\r\n"
          "So uebernimmt das Spiel ihn beim passenden Briefing automatisch, auch in der Hauptkampagne.",28,319,697,34,page);
    label("Gespeicherte Eintraege (0 entfernt einen bearbeitbaren Bestand)",28,359,697,18,page);
    saved_inventory=control("LISTBOX","",LBS_NOINTEGRALHEIGHT|WS_BORDER|WS_VSCROLL|WS_HSCROLL|WS_TABSTOP,28,378,701,78,0,page,1);
    SendMessageA(saved_inventory,LB_SETHORIZONTALEXTENT,2800,0);
}
static void make_ui(void){
    TCITEMA item;HWND heading;int i;const char *names[]={"Spieler / Ausruestung","Primaerwaffen","Sekundaerwaffen","Inventar / Schiffe","Datei / Pruefung"};
    font=(HFONT)GetStockObject(DEFAULT_GUI_FONT);title_font=CreateFontA(-18,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_SWISS,"MS Sans Serif");
    heading=label("DESCENT 3  -  SAVEGAME EDITOR / WINDOWS 98",12,10,736,25,-1);SendMessageA(heading,WM_SETFONT,(WPARAM)title_font,TRUE);
    button("Oeffnen...",12,42,105,ID_OPEN,-1,0);button("Speichern + Backup",123,42,144,ID_SAVE,-1,1);button("Kopie speichern...",273,42,128,ID_COPY,-1,1);
    button("Originalwerte",407,42,105,ID_RESET,-1,1);button("Schiffslimits laden...",518,42,150,ID_LIMITS,-1,1);button("Hilfe",674,42,74,ID_HELP,-1,0);
    info=label("Noch kein Spielstand geoeffnet. Im Spiel speichern, dann saveg000 usw. oeffnen.",12,76,736,34,-1);
    tab=control(WC_TABCONTROLA,"",WS_TABSTOP|WS_CLIPSIBLINGS,12,114,736,355,ID_TAB,-1,0);
    d3_zero(&item,sizeof(item));item.mask=TCIF_TEXT;for(i=0;i<(int)(sizeof(names)/sizeof(names[0]));i++){item.pszText=(LPSTR)names[i];SendMessageA(tab,TCM_INSERTITEMA,(WPARAM)i,(LPARAM)&item);}
    player_page();weapon_page(0,1);weapon_page(10,2);inventory_page(3);
    button("Aenderungen pruefen",28,147,178,ID_CHECK,4,1);
    review=control("EDIT","Geplante Aenderungen und Dateidaten erscheinen hier.\r\n\r\nDer Editor bearbeitet vorhandene Spielstaende; er erzeugt keinen\r\nvollstaendigen Levelzustand allein aus dem Quelltext.",ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL|WS_HSCROLL,28,180,701,276,0,4,0);
    hint=label("",12,475,736,16,-1);status=label("Bereit.",12,495,736,32,-1);
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
        if(code==EN_CHANGE){if((id>=ID_ENERGY && id<=ID_SCORE) || (id>=ID_AMMO && id<ID_AMMO+20) ||
            (id>=ID_CARGO_COUNT && id<ID_CARGO_COUNT+D3_CARGO_TYPES) ||
            (id>=ID_COUNTERMEASURE_COUNT && id<ID_COUNTERMEASURE_COUNT+D3_COUNTERMEASURE_TYPES))changed();return 0;}
        if(id==ID_PRIMARY && code==CBN_SELCHANGE){selected=(int)SendMessageA(primary,CB_GETCURSEL,0,0);if(selected>=0 && selected<10)check(owned[selected],1);refresh_weapons(1);changed();return 0;}
        if(id==ID_SECONDARY && code==CBN_SELCHANGE){selected=(int)SendMessageA(secondary,CB_GETCURSEL,0,0);if(selected==0)select_all(10,0);else if(selected>0){check(owned[selected+9],1);refresh_weapons(1);changed();}return 0;}
        if(code!=BN_CLICKED)return 0;
        if(id>=ID_SHIP_UNLOCK && id<ID_SHIP_UNLOCK+D3_STANDARD_SHIPS){refresh_ship_controls();changed();return 0;}
        if(id==ID_BLACK_PYRO_PREFER){select_black_pyro();return 0;}
        if(id==ID_BLACK_PYRO_UNLOCK){changed();return 0;}
        if(id>=ID_OWN && id<ID_OWN+20){i=id-ID_OWN;if(!checked(owned[i]))text(ammo[i],"0");refresh_weapons(1);changed();return 0;}
        if(id>=ID_MAX && id<ID_MAX+20){maximum_one(id-ID_MAX,0);return 0;}
        switch(id){
        case ID_SAVE:save_game(0);break;case ID_COPY:save_game(1);break;case ID_RESET:load_values();set_status("Originalwerte geladen.");break;case ID_LIMITS:choose_limits();break;
        case ID_CHECK:if(build_output(&out))d3_free(out.data);break;
        case ID_ENERGY_MAX:text(energy,"200");changed();break;case ID_SHIELD_MAX:text(shields,"200");changed();break;case ID_AB_MAX:text(afterburner,"5");changed();break;
        case ID_GOD:case ID_GOD_RENEW:god_request=checked(god);refresh_effect_timers();changed();break;
        case ID_CLOAK:case ID_CLOAK_RENEW:cloak_request=checked(cloak);refresh_effect_timers();changed();break;
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
