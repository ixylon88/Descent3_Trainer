/* SPDX-License-Identifier: GPL-3.0-or-later
 * ANSI Win32 GUI. No .NET, Unicode layer, CRT, MFC or external VC runtime.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include "d3core.h"
#include "d3text.h"
#include "d3files.h"
#include "version.h"

enum {ID_OPEN=100,ID_SAVE,ID_COPY,ID_RESET,ID_LIMITS,ID_LANGUAGE,ID_TAB,
ID_ENERGY=120,ID_SHIELDS,ID_AB,ID_LIVES,ID_SCORE,ID_ENERGY_MAX,ID_SHIELD_MAX,ID_AB_MAX,ID_ENERGY_HIGH,ID_SHIELD_HIGH,ID_AB_HIGH,
ID_GOD=140,ID_CLOAK,ID_COOLER,ID_CONVERTER,ID_QUAD,ID_MAP,ID_GOD_RENEW,ID_CLOAK_RENEW,ID_RAPID_FIRE,
ID_PRIMARY=160,ID_SECONDARY,ID_CHECK,
ID_ALL_PRI=170,ID_NONE_PRI,ID_CAP_PRI,ID_STORE_PRI,ID_ALL_SEC,ID_NONE_SEC,ID_CAP_SEC,ID_STORE_SEC,
ID_OWN=200,ID_AMMO=240,ID_MAX=280,
ID_CARGO_COUNT=320,ID_COUNTERMEASURE_COUNT=330,ID_SHIP_UNLOCK=340,ID_BLACK_PYRO_UNLOCK=350,ID_BLACK_PYRO_PREFER,
ID_COUNTERMEASURE_OWN=360,ID_COUNTERMEASURE_MAX=370,ID_COUNTERMEASURE_ACTIVE=380,ID_ALL_COUNTERMEASURES,ID_NONE_COUNTERMEASURES,ID_CAP_COUNTERMEASURES,
ID_GUIDEBOT_NAME=390,ID_GUIDEBOT_CHATTER,ID_GUIDEBOT_SUPPLY=400};
enum {PLAYER_PAGE,PRIMARY_PAGE,SECONDARY_PAGE,COUNTERMEASURE_PAGE,INVENTORY_PAGE,GUIDEBOT_PAGE,REVIEW_PAGE,HELP_PAGE};
typedef struct {HWND hwnd;int page,needs_file;D3Text caption;} Widget;
static HINSTANCE instance;
static HWND main_window,tab,info,status,hint,review,language_button;
static HFONT font,title_font;
static Widget widgets[256];static int widget_count,loading,dirty;
static HWND energy,shields,afterburner,lives,score,god,cloak,cooler,converter,quad,full_map,god_time,cloak_time,map_info;
static HWND god_renew,cloak_renew,rapid_fire;
static HWND owned[20],ammo[20],max_buttons[20],cap_labels[20],primary,secondary;
static HWND cargo_counts[D3_CARGO_TYPES],countermeasure_counts[D3_COUNTERMEASURE_TYPES];
static HWND countermeasure_owned[D3_COUNTERMEASURE_TYPES],countermeasure_max[D3_COUNTERMEASURE_TYPES];
static HWND countermeasure_active,extra_countermeasures;
static int countermeasure_choices[1+D3_COUNTERMEASURE_TYPES+D3_MAX_ITEMS],countermeasure_choice_count;
static HWND unlock_ships[D3_STANDARD_SHIPS],saved_inventory;
static HWND unlock_black_pyro,prefer_black_pyro_button;
static int prefer_black_pyro;
static HWND guidebot_info,guidebot_name,guidebot_chatter,guidebot_shield,guidebot_supplies[D3_GUIDEBOT_POWERUPS];
static D3Game *game;
static D3Buffer opened;
static D3U8 original_hash[32];
static char opened_path[260],energy_text[48],shield_text[48],ab_text[48],caps_ship[128];
static D3U16 caps[21];static int caps_loaded;
static const char high_player_value[]="999999";
static int god_request=-1,cloak_request=-1,cooler_request=-1,converter_request=-1,quad_request=-1;
static int rapid_fire_request=-1;
static const D3Text page_names[]={D3_TEXT_PLAYER_TAB,D3_TEXT_PRIMARY_TAB,D3_TEXT_SECONDARY_TAB,
    D3_TEXT_COUNTERMEASURE_TAB,D3_TEXT_INVENTORY_TAB,D3_TEXT_GUIDEBOT_TAB,D3_TEXT_REVIEW_TAB,D3_TEXT_HELP_TAB};
static const D3Text weapon_names[]={
    D3_TEXT_WEAPON_LASER,D3_TEXT_WEAPON_VAUSS,D3_TEXT_WEAPON_MICROWAVE,D3_TEXT_WEAPON_PLASMA,D3_TEXT_WEAPON_FUSION,
    D3_TEXT_WEAPON_SUPER_LASER,D3_TEXT_WEAPON_MASS_DRIVER,D3_TEXT_WEAPON_NAPALM,D3_TEXT_WEAPON_EMD_GUN,D3_TEXT_WEAPON_OMEGA,
    D3_TEXT_WEAPON_CONCUSSION,D3_TEXT_WEAPON_HOMING,D3_TEXT_WEAPON_IMPACT_MORTAR,D3_TEXT_WEAPON_SMART,D3_TEXT_WEAPON_MEGA,
    D3_TEXT_WEAPON_FRAG,D3_TEXT_WEAPON_GUIDED,D3_TEXT_WEAPON_NAPALM_ROCKET,D3_TEXT_WEAPON_CYCLONE,D3_TEXT_WEAPON_BLACK_SHARK
};
static const D3Text countermeasure_names[]={D3_TEXT_COUNTERMEASURE_CHAFF,D3_TEXT_COUNTERMEASURE_BETTY,
    D3_TEXT_COUNTERMEASURE_SEEKER_MINE,D3_TEXT_COUNTERMEASURE_GUNBOY,D3_TEXT_COUNTERMEASURE_PROX_MINE};

static int checked(HWND h){return SendMessageA(h,BM_GETCHECK,0,0)==BST_CHECKED;}
static void check(HWND h,int on){SendMessageA(h,BM_SETCHECK,on?BST_CHECKED:BST_UNCHECKED,0);}
static void text(HWND h,const char *s){SetWindowTextA(h,s);}
static void get_text(HWND h,char *s,int cap){GetWindowTextA(h,s,cap);}
static void set_status(const char *s){text(status,s);}
static void error_box(const char *s){set_status(s);MessageBoxA(main_window,s,T(ERROR_TITLE),MB_OK|MB_ICONEXCLAMATION);}
static void changed(void){if(!loading && game){dirty=1;SetWindowTextA(main_window,"* Descent 3 Savegame Editor - Windows 98");set_status(T(DIRTY));}}
static int discard(void){return !dirty || MessageBoxA(main_window,T(DISCARD_QUESTION),T(CHANGES),MB_YESNO|MB_ICONQUESTION)==IDYES;}
static HWND control(const char *cls,const char *label,DWORD style,int x,int y,int w,int h,int id,int page,int needs_file){
    if (page<0) style|=WS_VISIBLE;
    HWND hwnd=CreateWindowExA(d3_string_equal(cls,"EDIT")?WS_EX_CLIENTEDGE:0,cls,label,
        WS_CHILD|WS_CLIPSIBLINGS|style,x,y,w,h,main_window,(HMENU)(INT_PTR)id,instance,0);
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
static HWND remember_caption(HWND hwnd,D3Text caption) {
    widgets[widget_count-1].caption=caption;
    return hwnd;
}
static HWND local_label(D3Text caption,int x,int y,int w,int h,int page) {
    return remember_caption(label(d3_text(caption),x,y,w,h,page),caption);
}
static HWND local_button(D3Text caption,int x,int y,int w,int id,int page,int needs_file) {
    return remember_caption(button(d3_text(caption),x,y,w,id,page,needs_file),caption);
}
static HWND local_checkbox(D3Text caption,int x,int y,int w,int id,int page) {
    return remember_caption(checkbox(d3_text(caption),x,y,w,id,page),caption);
}
/* Child windows are created at the bottom of the sibling Z order.
   Create group frames after their contents so they paint behind them. */
static void local_group(D3Text caption,int x,int y,int w,int h,int page) {
    group(d3_text(caption),x,y,w,h,page);
    widgets[widget_count-1].caption=caption;
}
static void show_page(int page) {
    int i;
    for (i=0; i<widget_count; i++)
        if (widgets[i].page>=0 && widgets[i].page!=page) ShowWindow(widgets[i].hwnd,SW_HIDE);
    for (i=0; i<widget_count; i++)
        if (widgets[i].page==page) ShowWindow(widgets[i].hwnd,SW_SHOWNA);
    ShowWindow(hint,page==PRIMARY_PAGE || page==SECONDARY_PAGE ? SW_SHOWNA : SW_HIDE);
}
static void enable_controls(void){int i;for(i=0;i<widget_count;i++)if(widgets[i].needs_file)EnableWindow(widgets[i].hwnd,game!=0);}
static int use_caps(void){return game && caps_loaded && d3_string_iequal(caps_ship,d3_ship(game));}
static int uses_ammo(int i){return i>=10 || i==1 || i==6 || i==7;}
static void refresh_weapons(int normalize){
    int i,p,s,first,old_loading=loading;char buffer[32];
    if(!game)return;
    for(i=0;i<20;i++){
        EnableWindow(ammo[i],checked(owned[i]) && uses_ammo(i));EnableWindow(max_buttons[i],checked(owned[i]) && uses_ammo(i));
        if(!uses_ammo(i))text(cap_labels[i],T(ENERGY_WEAPON));else if(use_caps()){d3_format_u32(buffer,caps[i]);text(cap_labels[i],buffer);}else text(cap_labels[i],T(LIMIT_NOT_LOADED));
    }
    text(hint,use_caps()?T(LIMITS_HINT):T(LIMITS_MISSING_HINT));
    if(normalize){
        loading=1;p=(int)SendMessageA(primary,CB_GETCURSEL,0,0);
        if(p<0 || !checked(owned[p])){for(first=0;first<10;first++)if(checked(owned[first]))break;SendMessageA(primary,CB_SETCURSEL,first==10?(WPARAM)-1:(WPARAM)first,0);}
        s=(int)SendMessageA(secondary,CB_GETCURSEL,0,0)+9;
        if(s<10 || !checked(owned[s])){for(first=10;first<20;first++)if(checked(owned[first]))break;SendMessageA(secondary,CB_SETCURSEL,first==20?0:(WPARAM)(first-9),0);}
        loading=old_loading;
    }
}
static void append_number(char *out,D3U32 capacity,D3U32 value){char s[24];d3_format_u32(s,value);d3_append(out,capacity,s);}
static void effect_label(HWND h,int request,int on,float duration){char s[100],n[48];if(request>=0){text(h,request?T(EFFECT_LONG_PENDING):T(EFFECT_OFF_PENDING));return;}if(!on){text(h,T(EFFECT_OFF));return;}if(duration>=1e8f){text(h,T(EFFECT_LONG));return;}d3_string(s,100,T(SAVED_PREFIX));d3_format_float(n,duration);d3_append(s,100,n);d3_append(s,100,T(REMAINING));text(h,s);}
static void refresh_effect_timers(void) {
    effect_label(god_time,god_request,d3_god(game),d3_float(game->data+game->player+116));
    effect_label(cloak_time,cloak_request,d3_cloak(game),game->object.has_effect?d3_float(game->data+game->object.effect+12):0);
    EnableWindow(god_renew,d3_god(game) && god_request<0);
    EnableWindow(cloak_renew,d3_cloak(game) && cloak_request<0);
}
static void file_report(char *out,D3U32 cap){
    int i;char hash[65];const char *hex="0123456789abcdef";
    for(i=0;i<32;i++){hash[2*i]=hex[original_hash[i]>>4];hash[2*i+1]=hex[original_hash[i]&15];}hash[64]=0;
    d3_string(out,cap,T(FILE_HEADING));d3_append(out,cap,opened_path);d3_append(out,cap,"\r\n\r\nSHA-256: ");d3_append(out,cap,hash);
    d3_append(out,cap,"\r\nFormat: ");append_number(out,cap,(D3U32)game->version);d3_append(out,cap,T(PLAYER_LAYOUT));d3_append(out,cap,game->callsign);
    d3_append(out,cap,T(SHIP_LABEL));d3_append(out,cap,d3_ship(game));
    d3_append(out,cap,T(ITEM_TYPES));append_number(out,cap,(D3U32)game->inventory.count);
    d3_append(out,cap,T(MEASURE_TYPES));append_number(out,cap,(D3U32)game->countermeasures.count);d3_append(out,cap,T(TYPE_SUFFIX));
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
static const char *inventory_name(const D3Item *item,char id[32]) {
    char value[24];
    if (item->name[0]) return item->name;
    d3_format_i32(value,item->id<0 ? item->original_id : item->id);
    d3_string(id,32,"ID ");
    d3_append(id,32,value);
    return id;
}
static void add_inventory_row(HWND list,const D3Item *item) {
    char row[600],id[32];
    d3_string(row,sizeof(row),inventory_name(item,id));
    d3_append(row,sizeof(row)," x ");
    append_number(row,sizeof(row),(D3U32)item->count);
    SendMessageA(list,LB_ADDSTRING,0,(LPARAM)row);
}
static int present_countermeasure(int choice) {
    return choice>=D3_COUNTERMEASURE_TYPES || (choice>=0 && checked(countermeasure_owned[choice]));
}
static void refresh_countermeasures(int normalize) {
    int i,selected;
    char value[24];
    D3I32 count;
    for (i=0; i<D3_COUNTERMEASURE_TYPES; i++) {
        int available=d3_supply_count(game,1,i)>=0;
        int owned;
        get_text(countermeasure_counts[i],value,sizeof(value));
        if (d3_integer(value,0,1000000,&count)) check(countermeasure_owned[i],count>0);
        else if (!available) check(countermeasure_owned[i],0);
        owned=available && checked(countermeasure_owned[i]);
        EnableWindow(countermeasure_owned[i],available);
        EnableWindow(countermeasure_counts[i],owned);
        EnableWindow(countermeasure_max[i],owned);
    }
    if (!normalize) return;
    selected=(int)SendMessageA(countermeasure_active,CB_GETCURSEL,0,0);
    if (selected>=0 && present_countermeasure(countermeasure_choices[selected])) return;
    for (i=1; i<countermeasure_choice_count; i++) if (present_countermeasure(countermeasure_choices[i])) break;
    SendMessageA(countermeasure_active,CB_SETCURSEL,i==countermeasure_choice_count ? 0 : (WPARAM)i,0);
}
static int add_countermeasure_choice(int row,const char *name) {
    int index=(int)SendMessageA(countermeasure_active,CB_ADDSTRING,0,(LPARAM)name);
    if (index>=0) {
        countermeasure_choices[index]=row;
        countermeasure_choice_count++;
    }
    return index;
}
static void load_countermeasure_choices(void) {
    const D3Inventory *list=&game->countermeasures;
    const D3Item *active=list->count ? &list->items[list->selected] : 0;
    int i,j,index,selection=0;
    char id[32];
    SendMessageA(countermeasure_active,CB_RESETCONTENT,0,0);
    SendMessageA(extra_countermeasures,LB_RESETCONTENT,0,0);
    countermeasure_choice_count=0;
    add_countermeasure_choice(-1,T(NO_COUNTERMEASURE));
    for (i=0; i<D3_COUNTERMEASURE_TYPES; i++) {
        if (d3_supply_count(game,1,i)<0) continue;
        index=add_countermeasure_choice(i,d3_text(countermeasure_names[i]));
        if (index>=0 && active && active->type==5 && active->id==game->countermeasure_ids[i]) selection=index;
    }
    for (i=0; i<list->count; i++) {
        const D3Item *item=&list->items[i];
        for (j=0; j<D3_COUNTERMEASURE_TYPES; j++)
            if (item->type==5 && item->id==game->countermeasure_ids[j]) break;
        if (j!=D3_COUNTERMEASURE_TYPES) continue;
        index=add_countermeasure_choice(D3_COUNTERMEASURE_TYPES+i,inventory_name(item,id));
        add_inventory_row(extra_countermeasures,item);
        if (index>=0 && item==active) selection=index;
    }
    SendMessageA(countermeasure_active,CB_SETCURSEL,(WPARAM)selection,0);
}
static void load_countermeasures(void) {
    load_supply_counts(countermeasure_counts,1,D3_COUNTERMEASURE_TYPES);
    load_countermeasure_choices();
    refresh_countermeasures(0);
}
static void set_countermeasure_count(int i,int count) {
    char value[24];
    d3_format_u32(value,(D3U32)count);
    text(countermeasure_counts[i],value);
}
static void change_all_countermeasures(int mode) {
    int i;
    loading=1;
    for (i=0; i<D3_COUNTERMEASURE_TYPES; i++) {
        if (d3_supply_count(game,1,i)<0) continue;
        if (mode==0 || (mode==1 && !checked(countermeasure_owned[i]))) set_countermeasure_count(i,mode);
        if (mode==2 && checked(countermeasure_owned[i])) set_countermeasure_count(i,d3_countermeasure_types[i].maximum);
    }
    loading=0;
    refresh_countermeasures(1);
    changed();
}
static void select_countermeasure(void) {
    int selection=(int)SendMessageA(countermeasure_active,CB_GETCURSEL,0,0);
    int choice;
    if (selection<0) return;
    choice=countermeasure_choices[selection];
    if (choice<0) change_all_countermeasures(0);
    else if (!present_countermeasure(choice)) set_countermeasure_count(choice,1);
    changed();
}
static int read_countermeasure_selection(void) {
    int selection=(int)SendMessageA(countermeasure_active,CB_GETCURSEL,0,0);
    int choice;
    if (selection<0) return -1;
    choice=countermeasure_choices[selection];
    if (choice<0) return -1;
    if (choice<D3_COUNTERMEASURE_TYPES) return game->countermeasure_ids[choice];
    return game->countermeasures.items[choice-D3_COUNTERMEASURE_TYPES].type==5 ?
        game->countermeasures.items[choice-D3_COUNTERMEASURE_TYPES].id : -1;
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
static void refresh_ship_labels(void) {
    int i;
    char name[64];
    for (i=0; i<D3_STANDARD_SHIPS; i++) {
        d3_string(name,sizeof(name),d3_standard_ships[i]);
        if (!d3_ship_permission_bit(game,d3_standard_ships[i])) d3_append(name,sizeof(name),T(MISSING_SUFFIX));
        text(unlock_ships[i],name);
    }
    d3_string(name,sizeof(name),D3_BLACK_PYRO);
    if (!d3_ship_permission_bit(game,D3_BLACK_PYRO)) d3_append(name,sizeof(name),T(MISSING_SUFFIX));
    text(unlock_black_pyro,name);
}
static void load_inventory_values(void) {
    int i;
    D3U32 bit,permissions=d3_ship_permissions(game);
    load_supply_counts(cargo_counts,0,D3_CARGO_TYPES);
    load_countermeasures();
    for (i=0; i<D3_STANDARD_SHIPS; i++) {
        bit=d3_ship_permission_bit(game,d3_standard_ships[i]);
        check(unlock_ships[i],(permissions & bit)!=0);
    }
    bit=d3_ship_permission_bit(game,D3_BLACK_PYRO);
    check(unlock_black_pyro,(permissions & bit)!=0);
    refresh_ship_labels();
    prefer_black_pyro=0;
    refresh_ship_controls();
    SendMessageA(saved_inventory,LB_RESETCONTENT,0,0);
    for (i=0; i<game->inventory.count; i++) add_inventory_row(saved_inventory,&game->inventory.items[i]);
    if (!game->inventory.count)
        SendMessageA(saved_inventory,LB_ADDSTRING,0,(LPARAM)T(INVENTORY_EMPTY));
}
static void refresh_guidebot_text(void) {
    const D3GuideBot *bot=&game->guidebot;
    char value[48],line[112];
    text(guidebot_info,bot->offset ? T(BOT_INFO) : T(BOT_UNAVAILABLE));
    line[0]=0;
    if (bot->offset) {
        d3_format_float(value,bot->shields);
        d3_string(line,sizeof(line),T(BOT_SHIELDS));d3_append(line,sizeof(line),value);
    }
    text(guidebot_shield,line);
}
static void load_guidebot_values(void) {
    const D3GuideBot *bot=&game->guidebot;
    int i,available=bot->offset!=0;
    char value[24];
    refresh_guidebot_text();
    text(guidebot_name,bot->name);
    EnableWindow(guidebot_name,available);
    check(guidebot_chatter,bot->chatter);
    EnableWindow(guidebot_chatter,available);
    for (i=0; i<D3_GUIDEBOT_POWERUPS; i++) {
        d3_format_u32(value,bot->counts[i]);
        text(guidebot_supplies[i],available ? value : "-");
        EnableWindow(guidebot_supplies[i],available);
    }
}
static void refresh_map_text(void) {
    char line[192];
    int i,visible=0;
    for (i=0; i<=game->highest_room; i++) if (game->data[game->map+i]) visible++;
    d3_string(line,sizeof(line),T(MAP_PREFIX));
    append_number(line,sizeof(line),(D3U32)visible);
    d3_append(line,sizeof(line),T(OF));
    append_number(line,sizeof(line),(D3U32)(game->highest_room+1));
    d3_append(line,sizeof(line),T(ROOM_SUFFIX));
    text(map_info,line);
}
static void refresh_review_text(void) {
    char report[1400];
    file_report(report,sizeof(report));
    d3_append(report,sizeof(report),"\r\n");
    d3_append(report,sizeof(report),dirty ? T(DIRTY) : T(NO_CHANGES));
    text(review,report);
}
static void load_values(void){
    D3Options o;int i,has_second=0;char s[1400],n[48];
    if(!game)return;loading=1;enable_controls();d3_current(game,&o);
    d3_format_float(energy_text,o.energy);d3_format_float(shield_text,o.shields);d3_format_float(ab_text,o.afterburner);text(energy,energy_text);text(shields,shield_text);text(afterburner,ab_text);
    d3_format_u32(n,(D3U32)o.lives);text(lives,n);d3_format_i32(n,o.score);text(score,n);
    check(god,d3_god(game));check(cloak,d3_cloak(game));check(cooler,d3_has_item(game,game->cooler_id));check(converter,d3_has_item(game,game->converter_id));check(quad,d3_quad(game));check(full_map,0);
    check(rapid_fire,d3_rapid_fire(game));
    EnableWindow(cooler,game->cooler_id>=0);EnableWindow(converter,game->converter_id>=0);EnableWindow(quad,game->quad_id>=0 && game->object.batteries==21);
    load_inventory_values();
    load_guidebot_values();
    for(i=0;i<20;i++){check(owned[i],o.owned[i]);d3_format_u32(n,o.ammo[i]);text(ammo[i],n);if(i>=10 && o.owned[i])has_second=1;}
    SendMessageA(primary,CB_SETCURSEL,o.primary,0);SendMessageA(secondary,CB_SETCURSEL,has_second?(WPARAM)(o.secondary-9):0,0);
    god_request=cloak_request=rapid_fire_request=cooler_request=converter_request=quad_request=-1;
    refresh_effect_timers();
    refresh_map_text();
    d3_string(s,1400,opened_path);d3_append(s,1400,"\r\n");d3_append(s,1400,game->callsign);d3_append(s,1400," | ");d3_append(s,1400,d3_ship(game));d3_append(s,1400," | ");d3_append(s,1400,game->mission);d3_append(s,1400," / Level ");append_number(s,1400,(D3U32)game->level);text(info,s);
    dirty=0;refresh_review_text();SetWindowTextA(main_window,"Descent 3 Savegame Editor - Windows 98");refresh_weapons(0);loading=0;
}
static int load_path(const char *path,int ask){
    D3Buffer bytes;D3Game *parsed;const char *error;char full[260];DWORD n;
    if(ask && !discard())return 0;
    n=GetFullPathNameA(path,260,full,0);if(!n || n>=260){error_box(T(INVALID_PATH));return 0;}
    if(!d3_read_file(full,&bytes,&error)){error_box(error);return 0;}
    parsed=(D3Game*)d3_alloc(sizeof(*parsed));if(!parsed){d3_free(bytes.data);error_box(T(OUT_OF_MEMORY));return 0;}
    if(!d3_parse(parsed,bytes.data,bytes.size)){error_box(parsed->error);d3_free(parsed);d3_free(bytes.data);return 0;}
    d3_free(game);d3_free(opened.data);game=parsed;opened=bytes;d3_string(opened_path,260,full);d3_sha256(bytes.data,bytes.size,original_hash);load_values();set_status(T(LOADED));return 1;
}
static int dialog_path(char path[260],int save,const char *title,const char *filter){
    OPENFILENAMEA ofn;BOOL ok;d3_zero(&ofn,sizeof(ofn));
    /* Win9x-compatible v4 layout; newer headers have a larger structure. */
    ofn.lStructSize=76;ofn.hwndOwner=main_window;ofn.lpstrFile=path;ofn.nMaxFile=260;ofn.lpstrFilter=filter;ofn.nFilterIndex=1;ofn.lpstrTitle=title;
    ofn.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR|(save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);
    ok=save?GetSaveFileNameA(&ofn):GetOpenFileNameA(&ofn);
    if(!ok && CommDlgExtendedError())error_box(T(DIALOG_FAILED));return ok!=0;
}
static void choose_open(void){char path[260];path[0]=0;if(dialog_path(path,0,T(OPEN_TITLE),T(OPEN_FILTER)))load_path(path,1);}
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
    i=read_countermeasure_selection();
    if (game->countermeasures.count==0 || game->countermeasures.items[game->countermeasures.selected].type!=5 ||
        game->countermeasures.items[game->countermeasures.selected].id!=i) o->active_countermeasure_weapon_id=i;
    o->prefer_black_pyro=(D3U8)prefer_black_pyro;
    for (i=0; i<D3_STANDARD_SHIPS; i++) {
        bit=d3_ship_permission_bit(game,d3_standard_ships[i]);
        o->unlock_ships[i]=(D3U8)(bit!=0 && (prefer_black_pyro || !(d3_ship_permissions(game) & bit)) && checked(unlock_ships[i]));
    }
    bit=d3_ship_permission_bit(game,D3_BLACK_PYRO);
    o->unlock_black_pyro=(D3U8)(!o->prefer_black_pyro && bit!=0 && !(d3_ship_permissions(game) & bit) && checked(unlock_black_pyro));
    if (game->guidebot.offset) {
        get_text(guidebot_name,o->guidebot_name,sizeof(o->guidebot_name));
        o->rename_guidebot=!d3_string_equal(o->guidebot_name,game->guidebot.name);
        if (checked(guidebot_chatter)!=game->guidebot.chatter) o->guidebot_chatter=checked(guidebot_chatter);
        for (i=0; i<D3_GUIDEBOT_POWERUPS; i++) {
            get_text(guidebot_supplies[i],s,sizeof(s));
            if (!d3_integer(s,0,D3_GUIDEBOT_MAX_COUNT,&n)) return 0;
            if (n!=game->guidebot.counts[i]) o->guidebot_counts[i]=n;
        }
    }
    o->primary=(int)SendMessageA(primary,CB_GETCURSEL,0,0);i=(int)SendMessageA(secondary,CB_GETCURSEL,0,0);if(i>0)o->secondary=i+9;
    o->rapid_fire=rapid_fire_request;
    o->god=god_request;o->cloak=cloak_request;o->cooler=cooler_request;o->converter=converter_request;o->quad=quad_request;o->reveal_map=checked(full_map);return 1;
}
static void report_supply_counts(char *report,D3U32 size,const int *counts,int countermeasures) {
    int i,length=countermeasures ? D3_COUNTERMEASURE_TYPES : D3_CARGO_TYPES;
    for (i=0; i<length; i++) {
        if (counts[i]<0) continue;
        const char *name=countermeasures ? d3_text(countermeasure_names[i]) : d3_cargo_items[i].name;
        d3_append(report,size,name);d3_append(report,size,": ");
        append_number(report,size,(D3U32)counts[i]);d3_append(report,size,"\r\n");
    }
}
static int build_output(D3Buffer *out){
    D3Options o;char s[4096],n[48];int i;
    if(!game)return 0;if(!read_options(&o)){error_box(T(INVALID_NUMBER));return 0;}
    if(!d3_edit(game,&o,out)){error_box(game->error);return 0;}
    file_report(s,4096);d3_append(s,4096,T(VERIFIED_HEADING));append_number(s,4096,game->size);d3_append(s,4096," -> ");append_number(s,4096,out->size);d3_append(s,4096,T(SIZE_SUFFIX));
    if(out->size==game->size && d3_equal(out->data,game->data,out->size))d3_append(s,4096,T(IDENTICAL));
    else{
        d3_append(s,4096,T(ENERGY_PREFIX));d3_format_float(n,o.energy);d3_append(s,4096,n);d3_append(s,4096,T(SHIELD_PREFIX));d3_format_float(n,o.shields);d3_append(s,4096,n);d3_append(s,4096,"\r\n");
        for(i=0;i<20;i++)if(o.owned[i]){d3_append(s,4096,d3_text(weapon_names[i]));d3_append(s,4096,": ");append_number(s,4096,o.ammo[i]);d3_append(s,4096,"\r\n");}
        if(o.god>=0)d3_append(s,4096,o.god?T(GOD_LONG):T(GOD_OFF));
        if(o.cloak>=0)d3_append(s,4096,o.cloak?T(CLOAK_LONG):T(CLOAK_OFF));
        if(o.rapid_fire>=0)d3_append(s,4096,o.rapid_fire?T(RAPID_FIRE_ON):T(RAPID_FIRE_OFF));
        if(o.cooler>=0)d3_append(s,4096,o.cooler?T(COOLER_ADD):T(COOLER_REMOVE));
        if(o.converter>=0)d3_append(s,4096,o.converter?T(CONVERTER_ADD):T(CONVERTER_REMOVE));
        if(o.quad>=0)d3_append(s,4096,o.quad?T(QUAD_ON):T(QUAD_OFF));if(o.reveal_map)d3_append(s,4096,T(MAP_REVEALED));
        report_supply_counts(s,sizeof(s),o.cargo_counts,0);
        report_supply_counts(s,sizeof(s),o.countermeasure_counts,1);
        if (o.active_countermeasure_weapon_id>=0) {
            SendMessageA(countermeasure_active,WM_GETTEXT,sizeof(n),(LPARAM)n);
            d3_append(s,sizeof(s),T(ACTIVE_COUNTERMEASURE_REPORT));d3_append(s,sizeof(s),n);d3_append(s,sizeof(s),"\r\n");
        }
        for (i=0; i<D3_STANDARD_SHIPS; i++) {
            if (!o.unlock_ships[i]) continue;
            d3_append(s,sizeof(s),d3_standard_ships[i]);d3_append(s,sizeof(s),T(UNLOCK_SHIP_SUFFIX));
        }
        if (o.prefer_black_pyro) d3_append(s,sizeof(s),T(RESET_PERMISSIONS));
        else if (o.unlock_black_pyro) d3_append(s,sizeof(s),T(UNLOCK_BLACK_PYRO));
        if (o.rename_guidebot || o.guidebot_chatter>=0) d3_append(s,sizeof(s),T(EDIT_BOT_PROPERTIES));
        for (i=0; i<D3_GUIDEBOT_POWERUPS; i++) {
            if (o.guidebot_counts[i]<0) continue;
            d3_append(s,sizeof(s),T(EDIT_BOT_SUPPLIES));
            break;
        }
    }
    d3_append(s,4096,T(OUTPUT_REREAD));text(review,s);set_status(T(VERIFIED));return 1;
}
static void save_game(int copy){
    char path[260],s[900];D3Buffer out;D3WriteReport report;
    if(!build_output(&out))return;d3_string(path,260,opened_path);
    if(copy){d3_append(path,260,".edited");if(!dialog_path(path,1,T(COPY_TITLE),T(ALL_FILES_FILTER))){d3_free(out.data);return;}}
    if(!d3_write_file(opened_path,original_hash,path,&out,&report)){
        d3_string(s,900,report.error);if(report.recovery[0]){d3_append(s,900,"\r\nOriginal-Backup: ");d3_append(s,900,report.recovery);}error_box(s);d3_free(out.data);return;
    }
    /* Install the already validated output snapshot without discarding edits. */
    d3_free(opened.data);opened=out;d3_parse(game,opened.data,opened.size);d3_string(opened_path,260,path);d3_sha256(opened.data,opened.size,original_hash);load_values();
    d3_string(s,900,T(SAVED));if(report.backup[0]){d3_append(s,900," Backup: ");d3_append(s,900,report.backup);}set_status(s);
}
static void choose_limits(void){
    char path[260];D3U16 parsed[21];const char *error;if(!game)return;path[0]=0;
    if(!dialog_path(path,0,T(LIMITS_TITLE),T(GAME_DATA_FILTER)))return;
    if(!d3_load_limits_file(path,d3_ship(game),parsed,&error)){error_box(error);return;}d3_copy(caps,parsed,42);d3_string(caps_ship,128,d3_ship(game));caps_loaded=1;refresh_weapons(0);set_status(T(LIMITS_LOADED));
}
static void maximum_one(int i,int storage){char s[24];d3_format_u32(s,(!storage && use_caps())?caps[i]:65535);text(ammo[i],s);changed();}
static void select_all(int first,int on){int i;for(i=first;i<first+10;i++){check(owned[i],on);if(!on)text(ammo[i],"0");}refresh_weapons(1);changed();}
static void max_all(int first,int storage){int i;if(!storage && !use_caps()){error_box(T(LOAD_LIMITS_FIRST));return;}for(i=first;i<first+10;i++)if(checked(owned[i]) && uses_ammo(i))maximum_one(i,storage);}
static void label_tabs(UINT message) {
    TCITEMA item;
    int i;
    d3_zero(&item,sizeof(item));
    item.mask=TCIF_TEXT;
    for (i=0; i<(int)(sizeof(page_names)/sizeof(page_names[0])); i++) {
        item.pszText=(LPSTR)d3_text(page_names[i]);
        SendMessageA(tab,message,(WPARAM)i,(LPARAM)&item);
    }
}
static void refresh_weapon_choices(HWND combo,int first) {
    int i;
    LRESULT selection=SendMessageA(combo,CB_GETCURSEL,0,0);
    SendMessageA(combo,CB_RESETCONTENT,0,0);
    if (first) SendMessageA(combo,CB_ADDSTRING,0,(LPARAM)T(NO_SECONDARY));
    for (i=first; i<first+10; i++) SendMessageA(combo,CB_ADDSTRING,0,(LPARAM)d3_text(weapon_names[i]));
    SendMessageA(combo,CB_SETCURSEL,(WPARAM)selection,0);
}
static void refresh_countermeasure_choices(void) {
    int i,selected=(int)SendMessageA(countermeasure_active,CB_GETCURSEL,0,0);
    int choice=selected>=0 && selected<countermeasure_choice_count ? countermeasure_choices[selected] : -1;
    load_countermeasure_choices();
    for (i=0; i<countermeasure_choice_count; i++) {
        if (countermeasure_choices[i]!=choice) continue;
        SendMessageA(countermeasure_active,CB_SETCURSEL,(WPARAM)i,0);
        break;
    }
}
static void apply_language(int english) {
    int i;
    d3_set_english(english);
    loading=1;
    label_tabs(TCM_SETITEMA);
    for (i=0; i<widget_count; i++)
        if (widgets[i].caption!=D3_TEXT_NONE) text(widgets[i].hwnd,d3_text(widgets[i].caption));
    refresh_weapon_choices(primary,0);
    refresh_weapon_choices(secondary,10);
    if (game) {
        refresh_countermeasure_choices();
        if (!game->inventory.count) {
            SendMessageA(saved_inventory,LB_RESETCONTENT,0,0);
            SendMessageA(saved_inventory,LB_ADDSTRING,0,(LPARAM)T(INVENTORY_EMPTY));
        }
        refresh_ship_labels();
        refresh_guidebot_text();
        refresh_map_text();
        refresh_effect_timers();
        refresh_weapons(0);
        refresh_review_text();
        set_status(dirty ? T(DIRTY) : T(LOADED));
    } else {
        text(info,T(NO_SAVE));
        text(review,T(REVIEW_HINT));
        set_status(T(READY));
    }
    loading=0;
}
static void open_language_menu(void) {
    HMENU menu=CreatePopupMenu();
    RECT anchor;
    int chosen=0,english=d3_is_english();
    if (!menu) return;
    if (GetWindowRect(language_button,&anchor) &&
        AppendMenuA(menu,MF_STRING | (english ? 0 : MF_CHECKED),1,"&Deutsch") &&
        AppendMenuA(menu,MF_STRING | (english ? MF_CHECKED : 0),2,"&English"))
        chosen=TrackPopupMenu(menu,TPM_LEFTALIGN|TPM_RETURNCMD,anchor.left,anchor.bottom,0,main_window,0);
    DestroyMenu(menu);
    if (chosen) apply_language(chosen==2);
}
static void help_page(void) {
    local_label(D3_TEXT_HELP_OPEN,
          38,167,681,54,HELP_PAGE);
    local_label(D3_TEXT_HELP_LIMITS,38,253,681,42,HELP_PAGE);
    local_label(D3_TEXT_HELP_TIMERS,38,326,681,37,HELP_PAGE);
    local_label(D3_TEXT_HELP_PYRO_AND_BOT,38,394,681,53,HELP_PAGE);

    local_group(D3_TEXT_HELP_SAVES,24,146,710,83,HELP_PAGE);
    local_group(D3_TEXT_HELP_LOADOUT,24,232,710,70,HELP_PAGE);
    local_group(D3_TEXT_HELP_EFFECTS,24,305,710,65,HELP_PAGE);
    local_group(D3_TEXT_HELP_SHIPS_AND_BOT,24,373,710,83,HELP_PAGE);
}
static void player_page(void) {
    const int page=PLAYER_PAGE, value_x=138, value_width=96;
    const int label_width=96, normal_x=240, high_x=302;

    local_label(D3_TEXT_ENERGY,38,170,label_width,18,page);
    energy=edit(value_x,166,value_width,ID_ENERGY,page,0);
    button("Max 200",normal_x,165,56,ID_ENERGY_MAX,page,1);
    local_button(D3_TEXT_PLAYER_HIGH_VALUE,high_x,165,63,ID_ENERGY_HIGH,page,1);

    local_label(D3_TEXT_SHIELDS,38,198,label_width,18,page);
    shields=edit(value_x,194,value_width,ID_SHIELDS,page,0);
    button("Max 200",normal_x,193,56,ID_SHIELD_MAX,page,1);
    local_button(D3_TEXT_PLAYER_HIGH_VALUE,high_x,193,63,ID_SHIELD_HIGH,page,1);

    local_label(D3_TEXT_AFTERBURNER,38,226,label_width,18,page);
    afterburner=edit(value_x,222,value_width,ID_AB,page,0);
    local_button(D3_TEXT_FILL_AFTERBURNER,normal_x,221,56,ID_AB_MAX,page,1);
    local_button(D3_TEXT_PLAYER_HIGH_VALUE,high_x,221,63,ID_AB_HIGH,page,1);

    local_label(D3_TEXT_LIVES,38,254,label_width,18,page);
    lives=edit(value_x,250,value_width,ID_LIVES,page,1);
    local_label(D3_TEXT_SCORE,38,282,label_width,18,page);
    score=edit(value_x,278,value_width,ID_SCORE,page,0);

    cooler=local_checkbox(D3_TEXT_COOLER,38,356,300,ID_COOLER,page);
    converter=local_checkbox(D3_TEXT_CONVERTER,38,384,300,ID_CONVERTER,page);
    quad=local_checkbox(D3_TEXT_QUAD_LASERS,38,412,300,ID_QUAD,page);

    god=local_checkbox(D3_TEXT_INVULNERABILITY,392,167,213,ID_GOD,page);
    god_renew=local_button(D3_TEXT_EXTEND_EFFECT,610,165,109,ID_GOD_RENEW,page,1);
    god_time=label("",392,191,327,18,page);
    cloak=local_checkbox(D3_TEXT_CLOAK,392,226,213,ID_CLOAK,page);
    cloak_renew=local_button(D3_TEXT_EXTEND_EFFECT,610,224,109,ID_CLOAK_RENEW,page,1);
    cloak_time=label("",392,250,327,18,page);
    rapid_fire=local_checkbox(D3_TEXT_RAPID_FIRE,392,274,327,ID_RAPID_FIRE,page);

    full_map=local_checkbox(D3_TEXT_REVEAL_MAP,392,331,325,ID_MAP,page);
    map_info=label("",392,365,326,55,page);

    local_group(D3_TEXT_SUPPLIES,24,146,347,181,page);
    local_group(D3_TEXT_EQUIPMENT,24,335,347,119,page);
    local_group(D3_TEXT_EFFECTS,378,146,356,151,page);
    local_group(D3_TEXT_MAP,378,308,356,146,page);
}
static void weapon_page(int first,int page){
    int i,y;HWND combo;local_label(D3_TEXT_ACTIVE_WEAPON,28,151,100,18,page);
    combo=control("COMBOBOX","",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,131,147,230,200,first?ID_SECONDARY:ID_PRIMARY,page,1);
    if(first)secondary=combo;else primary=combo;
    refresh_weapon_choices(combo,first);
    local_label(D3_TEXT_OWNED,28,181,75,18,page);local_label(D3_TEXT_WEAPON,110,181,180,18,page);local_label(D3_TEXT_AMMO,298,181,115,18,page);label("Max",423,181,65,18,page);local_label(D3_TEXT_SHIP_LIMIT,512,181,185,18,page);
    for(i=first;i<first+10;i++){
        y=201+(i-first)*23;owned[i]=checkbox("",51,y,35,ID_OWN+i,page);local_label(weapon_names[i],110,y+2,180,18,page);
        ammo[i]=edit(298,y-1,106,ID_AMMO+i,page,1);SendMessageA(ammo[i],EM_LIMITTEXT,5,0);
        max_buttons[i]=control("BUTTON","Max",BS_PUSHBUTTON|WS_TABSTOP,420,y-1,70,22,ID_MAX+i,page,1);cap_labels[i]=local_label(D3_TEXT_LIMIT_NOT_LOADED,512,y+2,185,18,page);
    }
    local_button(D3_TEXT_SELECT_ALL,28,437,130,first?ID_ALL_SEC:ID_ALL_PRI,page,1);local_button(D3_TEXT_CLEAR_ALL,166,437,130,first?ID_NONE_SEC:ID_NONE_PRI,page,1);
    local_button(D3_TEXT_FILL_SHIP_LIMIT,304,437,204,first?ID_CAP_SEC:ID_CAP_PRI,page,1);local_button(D3_TEXT_FILL_STORAGE,516,437,204,first?ID_STORE_SEC:ID_STORE_PRI,page,1);
}
static void countermeasure_page(void) {
    int page=COUNTERMEASURE_PAGE,i,y;
    char value[24];
    local_label(D3_TEXT_ACTIVE_COUNTERMEASURE,28,151,153,18,page);
    countermeasure_active=control("COMBOBOX","",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,
        180,147,230,235,ID_COUNTERMEASURE_ACTIVE,page,1);
    local_label(D3_TEXT_OWNED,28,181,75,18,page);local_label(D3_TEXT_COUNTERMEASURE,110,181,180,18,page);
    local_label(D3_TEXT_COUNT,298,181,115,18,page);label("Max",423,181,65,18,page);local_label(D3_TEXT_GAME_LIMIT,512,181,185,18,page);
    for (i=0; i<D3_COUNTERMEASURE_TYPES; i++) {
        y=201+i*23;
        countermeasure_owned[i]=checkbox("",51,y,35,ID_COUNTERMEASURE_OWN+i,page);
        local_label(countermeasure_names[i],110,y+2,180,18,page);
        countermeasure_counts[i]=edit(298,y-1,106,ID_COUNTERMEASURE_COUNT+i,page,1);
        SendMessageA(countermeasure_counts[i],EM_LIMITTEXT,7,0);
        countermeasure_max[i]=control("BUTTON","Max",BS_PUSHBUTTON|WS_TABSTOP,420,y-1,70,22,ID_COUNTERMEASURE_MAX+i,page,1);
        d3_format_u32(value,(D3U32)d3_countermeasure_types[i].maximum);
        label(value,512,y+2,185,18,page);
    }
    local_button(D3_TEXT_SELECT_ALL,28,326,130,ID_ALL_COUNTERMEASURES,page,1);
    local_button(D3_TEXT_CLEAR_ALL,166,326,130,ID_NONE_COUNTERMEASURES,page,1);
    local_button(D3_TEXT_FILL_GAME_LIMIT,304,326,204,ID_CAP_COUNTERMEASURES,page,1);
    local_label(D3_TEXT_COUNTERMEASURE_HINT,28,359,701,18,page);
    extra_countermeasures=control("LISTBOX","",LBS_NOINTEGRALHEIGHT|WS_BORDER|WS_VSCROLL|WS_HSCROLL|WS_TABSTOP,
        28,383,701,73,0,page,1);
    SendMessageA(extra_countermeasures,LB_SETHORIZONTALEXTENT,2800,0);
}
static void inventory_page(int page) {
    int i,y;
    for (i=0; i<D3_CARGO_TYPES; i++) {
        y=172+i*25;
        label(d3_cargo_items[i].name,38,y+2,172,18,page);
        cargo_counts[i]=edit(217,y,130,ID_CARGO_COUNT+i,page,1);
        SendMessageA(cargo_counts[i],EM_LIMITTEXT,7,0);
    }
    local_label(D3_TEXT_CARGO_HINT,38,232,309,40,page);
    for (i=0; i<D3_STANDARD_SHIPS; i++)
        unlock_ships[i]=checkbox(d3_standard_ships[i],392,166+i*17,325,ID_SHIP_UNLOCK+i,page);
    unlock_black_pyro=checkbox(D3_BLACK_PYRO,392,217,325,ID_BLACK_PYRO_UNLOCK,page);
    prefer_black_pyro_button=local_button(D3_TEXT_BLACK_PYRO_ONLY,392,240,198,ID_BLACK_PYRO_PREFER,page,1);
    local_label(D3_TEXT_BLACK_PYRO_HINT,392,270,325,64,page);
    local_label(D3_TEXT_SAVED_ENTRIES,28,344,697,18,page);
    saved_inventory=control("LISTBOX","",LBS_NOINTEGRALHEIGHT|WS_BORDER|WS_VSCROLL|WS_HSCROLL|WS_TABSTOP,28,362,701,94,0,page,1);
    SendMessageA(saved_inventory,LB_SETHORIZONTALEXTENT,2800,0);
    local_group(D3_TEXT_ITEMS,24,146,338,181,page);
    local_group(D3_TEXT_SHIP_PERMISSIONS,378,146,356,190,page);
}
static void guidebot_page(void) {
    const D3Text supplies[]={D3_TEXT_BOT_EXTINGUISHER,D3_TEXT_BOT_WINGNUT,D3_TEXT_BOT_CONTROL,
        D3_TEXT_BOT_GUARDIAN,D3_TEXT_BOT_ANTIVIRUS};
    int i,y,page=GUIDEBOT_PAGE;
    guidebot_info=label("",28,150,701,31,page);
    label("Name",38,214,124,18,page);
    guidebot_name=edit(171,210,172,ID_GUIDEBOT_NAME,page,0);
    SendMessageA(guidebot_name,EM_LIMITTEXT,7,0);
    guidebot_chatter=local_checkbox(D3_TEXT_BOT_CHATTER,38,243,303,ID_GUIDEBOT_CHATTER,page);
    guidebot_shield=label("",38,271,303,34,page);
    for (i=0; i<D3_GUIDEBOT_POWERUPS; i++) {
        y=210+i*25;
        local_label(supplies[i],392,y+4,198,18,page);
        guidebot_supplies[i]=edit(606,y,111,ID_GUIDEBOT_SUPPLY+i,page,1);
        SendMessageA(guidebot_supplies[i],EM_LIMITTEXT,3,0);
    }
    local_group(D3_TEXT_PROPERTIES,24,186,338,161,page);
    local_group(D3_TEXT_BOT_SUPPLIES,378,186,356,161,page);
}
static void make_ui(void){
    HWND heading;
    font=(HFONT)GetStockObject(DEFAULT_GUI_FONT);title_font=CreateFontA(-18,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_SWISS,"MS Sans Serif");
    heading=label("DESCENT 3  -  SAVEGAME EDITOR / WINDOWS 98",12,10,560,25,-1);SendMessageA(heading,WM_SETFONT,(WPARAM)title_font,TRUE);
    local_button(D3_TEXT_OPEN,12,42,88,ID_OPEN,-1,0);local_button(D3_TEXT_SAVE,106,42,134,ID_SAVE,-1,1);local_button(D3_TEXT_COPY,246,42,128,ID_COPY,-1,1);
    local_button(D3_TEXT_RESET,380,42,100,ID_RESET,-1,1);local_button(D3_TEXT_LOAD_LIMITS,486,42,144,ID_LIMITS,-1,1);
    language_button=local_button(D3_TEXT_LANGUAGE,579,10,112,ID_LANGUAGE,-1,0);
    control("STATIC","v" D3_VERSION_TEXT,SS_RIGHT|WS_DISABLED,697,15,51,18,0,-1,0);
    info=label(T(NO_SAVE),12,76,736,34,-1);
    player_page();weapon_page(0,PRIMARY_PAGE);weapon_page(10,SECONDARY_PAGE);countermeasure_page();inventory_page(INVENTORY_PAGE);guidebot_page();help_page();
    local_button(D3_TEXT_CHECK_CHANGES,28,147,178,ID_CHECK,REVIEW_PAGE,1);
    review=control("EDIT",T(REVIEW_HINT),ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL|WS_HSCROLL,28,180,701,276,0,REVIEW_PAGE,0);
    hint=label("",12,475,736,16,-1);status=local_label(D3_TEXT_READY,12,495,736,32,-1);

    /* The tab background sits behind every page control. */
    tab=control(WC_TABCONTROLA,"",WS_TABSTOP,12,114,736,355,ID_TAB,-1,0);
    label_tabs(TCM_INSERTITEMA);
    enable_controls();show_page(PLAYER_PAGE);
}
static LRESULT CALLBACK window_proc(HWND hwnd,UINT message,WPARAM w,LPARAM l){
    int id,code,i,selected;D3Buffer out;NMHDR *notify;
    switch(message){
    case WM_CREATE:main_window=hwnd;make_ui();return 0;
    case WM_NOTIFY:
        notify=(NMHDR*)l;
        if (notify->hwndFrom==tab && notify->code==TCN_SELCHANGE)
            show_page((int)SendMessageA(tab,TCM_GETCURSEL,0,0));
        return 0;
    case WM_COMMAND:
        id=LOWORD(w);code=HIWORD(w);if(loading)return 0;
        if(id==ID_OPEN){choose_open();return 0;}if(id==ID_LANGUAGE){open_language_menu();return 0;}if(!game)return 0;
        if(code==EN_CHANGE){
            if(id>=ID_COUNTERMEASURE_COUNT && id<ID_COUNTERMEASURE_COUNT+D3_COUNTERMEASURE_TYPES){refresh_countermeasures(1);changed();}
            if((id>=ID_ENERGY && id<=ID_SCORE) || (id>=ID_AMMO && id<ID_AMMO+20) ||
                (id>=ID_CARGO_COUNT && id<ID_CARGO_COUNT+D3_CARGO_TYPES) || id==ID_GUIDEBOT_NAME ||
                (id>=ID_GUIDEBOT_SUPPLY && id<ID_GUIDEBOT_SUPPLY+D3_GUIDEBOT_POWERUPS))changed();
            return 0;
        }
        if(id==ID_COUNTERMEASURE_ACTIVE && code==CBN_SELCHANGE){select_countermeasure();return 0;}
        if(id==ID_PRIMARY && code==CBN_SELCHANGE){selected=(int)SendMessageA(primary,CB_GETCURSEL,0,0);if(selected>=0 && selected<10)check(owned[selected],1);refresh_weapons(1);changed();return 0;}
        if(id==ID_SECONDARY && code==CBN_SELCHANGE){selected=(int)SendMessageA(secondary,CB_GETCURSEL,0,0);if(selected==0)select_all(10,0);else if(selected>0){check(owned[selected+9],1);refresh_weapons(1);changed();}return 0;}
        if(code!=BN_CLICKED)return 0;
        if(id>=ID_COUNTERMEASURE_OWN && id<ID_COUNTERMEASURE_OWN+D3_COUNTERMEASURE_TYPES){i=id-ID_COUNTERMEASURE_OWN;set_countermeasure_count(i,checked(countermeasure_owned[i]));return 0;}
        if(id>=ID_COUNTERMEASURE_MAX && id<ID_COUNTERMEASURE_MAX+D3_COUNTERMEASURE_TYPES){i=id-ID_COUNTERMEASURE_MAX;set_countermeasure_count(i,d3_countermeasure_types[i].maximum);return 0;}
        if(id>=ID_SHIP_UNLOCK && id<ID_SHIP_UNLOCK+D3_STANDARD_SHIPS){refresh_ship_controls();changed();return 0;}
        if(id==ID_BLACK_PYRO_PREFER){select_black_pyro();return 0;}
        if(id==ID_BLACK_PYRO_UNLOCK){changed();return 0;}
        if(id>=ID_OWN && id<ID_OWN+20){i=id-ID_OWN;if(!checked(owned[i]))text(ammo[i],"0");refresh_weapons(1);changed();return 0;}
        if(id>=ID_MAX && id<ID_MAX+20){maximum_one(id-ID_MAX,0);return 0;}
        switch(id){
        case ID_SAVE:save_game(0);break;case ID_COPY:save_game(1);break;case ID_RESET:load_values();set_status(T(VALUES_RESET));break;case ID_LIMITS:choose_limits();break;
        case ID_CHECK:if(build_output(&out))d3_free(out.data);break;
        case ID_ENERGY_MAX:text(energy,"200");changed();break;case ID_SHIELD_MAX:text(shields,"200");changed();break;case ID_AB_MAX:text(afterburner,"5");changed();break;
        case ID_ENERGY_HIGH:text(energy,high_player_value);changed();break;
        case ID_SHIELD_HIGH:text(shields,high_player_value);changed();break;
        case ID_AB_HIGH:text(afterburner,high_player_value);changed();break;
        case ID_GOD:case ID_GOD_RENEW:god_request=checked(god);refresh_effect_timers();changed();break;
        case ID_CLOAK:case ID_CLOAK_RENEW:cloak_request=checked(cloak);refresh_effect_timers();changed();break;
        case ID_RAPID_FIRE:rapid_fire_request=checked(rapid_fire);changed();break;
        case ID_COOLER:cooler_request=checked(cooler);changed();break;case ID_CONVERTER:converter_request=checked(converter);changed();break;case ID_QUAD:quad_request=checked(quad);changed();break;case ID_MAP:changed();break;
        case ID_ALL_PRI:select_all(0,1);break;case ID_NONE_PRI:select_all(0,0);break;case ID_ALL_SEC:select_all(10,1);break;case ID_NONE_SEC:select_all(10,0);break;
        case ID_CAP_PRI:max_all(0,0);break;case ID_STORE_PRI:max_all(0,1);break;case ID_CAP_SEC:max_all(10,0);break;case ID_STORE_SEC:max_all(10,1);break;
        case ID_ALL_COUNTERMEASURES:change_all_countermeasures(1);break;
        case ID_NONE_COUNTERMEASURES:change_all_countermeasures(0);break;
        case ID_CAP_COUNTERMEASURES:change_all_countermeasures(2);break;
        case ID_GUIDEBOT_CHATTER:changed();break;
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
