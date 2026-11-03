/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "d3core.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
void *d3_alloc(D3U32 bytes) { return HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,bytes ? bytes : 1); }
void d3_free(void *p) { if(p)HeapFree(GetProcessHeap(),0,p); }
#else
#include <stdlib.h>
void *d3_alloc(D3U32 bytes) { return calloc(1,bytes ? bytes : 1); }
void d3_free(void *p) { free(p); }
#endif

const char *d3_weapon_names[20]={"Laser","Vauss","Microwave","Plasma","Fusion","Super Laser","Mass Driver","Napalm","EMD Gun","Omega","Concussion","Homing","Impact Mortar","Smart","Mega","Frag","Guided","Napalm Rocket","Cyclone","Black Shark"};
void d3_zero(void *p,D3U32 n) { D3U8 *d=(D3U8*)p;while(n--)*d++=0; }
void d3_copy(void *dest,const void *source,D3U32 n) { D3U8 *d=(D3U8*)dest;const D3U8 *s=(const D3U8*)source;while(n--)*d++=*s++; }
int d3_equal(const void *a,const void *b,D3U32 n) { const D3U8 *x=(const D3U8*)a,*y=(const D3U8*)b;while(n--)if(*x++!=*y++)return 0;return 1; }
D3U32 d3_length(const char *s) { D3U32 n=0;while(s[n])n++;return n; }
int d3_string_equal(const char *a,const char *b) { while(*a && *a==*b){a++;b++;}return *a==*b; }
static int lower(int c) { return c>='A' && c<='Z' ? c+32 : c; }
int d3_string_iequal(const char *a,const char *b) { while(*a && lower((D3U8)*a)==lower((D3U8)*b)){a++;b++;}return *a==*b; }
void d3_string(char *d,D3U32 cap,const char *s) { if(!cap)return;while(--cap && *s)*d++=*s++;*d=0; }
void d3_append(char *d,D3U32 cap,const char *s) { D3U32 n=d3_length(d);if(n<cap)d3_string(d+n,cap-n,s); }
D3U16 d3_u16(const D3U8 *p) { return (D3U16)(p[0]|((D3U16)p[1]<<8)); }
D3U32 d3_u32(const D3U8 *p) { return (D3U32)p[0]|((D3U32)p[1]<<8)|((D3U32)p[2]<<16)|((D3U32)p[3]<<24); }
float d3_float(const D3U8 *p) { union {D3U32 u;float f;} v;v.u=d3_u32(p);return v.f; }
void d3_put16(D3U8 *p,D3U16 n){p[0]=(D3U8)n;p[1]=(D3U8)(n>>8);}
void d3_put32(D3U8 *p,D3U32 n){int i;for(i=0;i<4;i++){p[i]=(D3U8)n;n>>=8;}}
void d3_put_float(D3U8 *p,float f){union{D3U32 u;float f;}v;v.f=f;d3_put32(p,v.u);}
int d3_finite(float f){union{D3U32 u;float f;}v;v.f=f;return (v.u&0x7f800000u)!=0x7f800000u;}

typedef struct {const D3U8 *d;D3U32 n,p;int ok;} Reader;
static Reader reader(const D3U8 *d,D3U32 n,D3U32 p){Reader r;r.d=d;r.n=n;r.p=p;r.ok=p<=n;return r;}
static int need(Reader *r,D3U32 n){if(!r->ok || r->p>r->n || n>r->n-r->p){r->ok=0;return 0;}return 1;}
static void skip(Reader *r,D3U32 n){if(need(r,n))r->p+=n;}
static int u8(Reader *r){if(!need(r,1))return 0;return r->d[r->p++];}
static int u16(Reader *r){int v;if(!need(r,2))return 0;v=d3_u16(r->d+r->p);r->p+=2;return v;}
static int i16(Reader *r){int v=u16(r);return v>32767 ? v-65536 : v;}
static D3I32 i32(Reader *r){D3U32 v;if(!need(r,4))return 0;v=d3_u32(r->d+r->p);r->p+=4;return (D3I32)v;}
static float f32(Reader *r){float v;if(!need(r,4))return 0;v=d3_float(r->d+r->p);r->p+=4;return v;}
static int boolean(Reader *r){int v=u8(r);if(v>1)r->ok=0;return v;}
static int z(Reader *r,char *dest,D3U32 cap){D3U32 n=0;int c;if(cap)dest[0]=0;while(r->ok && n<512){c=u8(r);if(!r->ok)break;if(c==0){if(cap)dest[n<cap?n:cap-1]=0;return 1;}if(cap && n<cap-1)dest[n]=(char)c;n++;}r->ok=0;return 0;}
static void sized(Reader *r){int n=i16(r);if(n<0)r->ok=0;else skip(r,(D3U32)n);}
static int fail(D3Game *g,const char *error){g->error=error;return 0;}

static int inventory(Reader *r,D3Inventory *inv){
    int i;D3Item *x;inv->start=r->p;inv->count=i32(r);
    if(inv->count<0 || inv->count>10)return 0;
    for(i=0;i<inv->count;i++){
        x=&inv->items[i];x->type=i32(r);x->original_type=i32(r);x->id=i32(r);x->original_id=i32(r);x->flags=i32(r);x->count=i32(r);
        if(x->count<1 || x->count>1000000)return 0;
        z(r,x->description,512);z(r,x->icon,512);z(r,x->name,512);x->iflags=i32(r);
    }
    inv->selected=i32(r);inv->end=r->p;
    return r->ok && inv->selected>=0 && inv->selected<(inv->count ? inv->count : 1);
}
static int object(const D3U8 *data,D3U32 size,D3U32 pos,int version,D3Object *o){
    Reader r=reader(data,size,pos);int render,n,allocated;float shield;
    d3_zero(o,sizeof(*o));o->start=pos;
    if((D3U32)i32(&r)!=0x00BADB0Bu)return 0;
    o->type=u8(&r);if(o->type==255){o->end=r.p;return r.ok;}
    if(o->type>25)return 0;
    u8(&r);if(boolean(&r))skip(&r,52);
    o->handle=(D3U32)i32(&r);u8(&r);skip(&r,4+12+12+36);n=u8(&r);skip(&r,(D3U32)n);
    o->id=i16(&r);skip(&r,4);u8(&r);u8(&r);render=u8(&r);skip(&r,2+4);
    o->shield=r.p;shield=f32(&r);if(!d3_finite(shield))return 0;
    skip(&r,3+12+4+8);n=i32(&r);if(n<0 || n>1024)return 0;
    if(n){allocated=version>=2?i32(&r):1;if(allocated<0 || allocated>1)return 0;if(allocated)skip(&r,(D3U32)n*4);}
    skip(&r,1+2+4+24+20);n=u8(&r);skip(&r,(D3U32)n);n=u8(&r);skip(&r,(D3U32)n);
    skip(&r,2);sized(&r);sized(&r);if(boolean(&r))sized(&r);sized(&r);skip(&r,4);if(render==1)skip(&r,24);
    o->batteries=u8(&r);if(o->batteries>21)return 0;o->battery=r.p;skip(&r,(D3U32)o->batteries*108);
    o->effect_record=r.p;o->has_effect=boolean(&r);
    if(o->has_effect){n=i16(&r);if(n!=164)return 0;o->effect=r.p;skip(&r,164);}
    o->end=r.p;return r.ok;
}
static int snapshot(Reader *r){
    int id,cmap,type,mips,width,height,m,run;D3U32 pixels,total,w,h;
    id=u8(r);cmap=u8(r);type=u8(r);if(cmap || (type!=121 && type!=122 && type!=123))return 0;
    z(r,0,0);mips=u8(r);skip(r,9);width=u16(r);height=u16(r);skip(r,2);skip(r,(D3U32)id);
    if(width<1 || width>8192 || height<1 || height>8192 || mips<1 || mips>16)return 0;
    for(m=0;m<mips;m++){
        w=(D3U32)width>>m;h=(D3U32)height>>m;if(!w)w=1;if(!h)h=1;total=w*h;pixels=0;
        while(pixels<total && r->ok){run=u8(r);if(run==1 || run>250)return 0;if(!run)run=1;if((D3U32)run>total-pixels)return 0;skip(r,2);pixels+=(D3U32)run;}
    }
    return r->ok;
}
static int header(D3Game *g){
    Reader r=reader(g->data,g->size,0);char text[512];int table,n,i,index,doors,active,weather;float time;
    for(i=0;i<32;i++)if(g->data[i]==0)break;if(i==32)return 0;
    d3_string(g->description,32,(const char*)g->data);skip(&r,32);g->version=i16(&r);if(g->version<0 || g->version>2)return 0;
    if(boolean(&r) && !snapshot(&r))return 0;
    g->cooler_id=g->converter_id=g->quad_id=-1;
    for(table=0;table<6;table++){
        n=i16(&r);if(n<0 || n>16384 || (table==3 && n>128))return 0;
        if(table==3)g->ship_count=n;
        for(i=0;i<n;i++){
            if(!z(&r,text,512))return 0;
            if(table==0){if(d3_string_iequal(text,"Afterburner"))g->cooler_id=i;if(d3_string_iequal(text,"Converter"))g->converter_id=i;if(d3_string_iequal(text,"QuadLaser"))g->quad_id=i;}
            if(table==3){if(d3_length(text)>=128)return 0;d3_string(g->ships[i],128,text);}
        }
    }
    for(i=0;i<16385;i++){index=i16(&r);if(!z(&r,text,512))return 0;if(index==-1){if(text[0])return 0;break;}if(index<0)return 0;}if(i==16385)return 0;
    g->level=i16(&r);z(&r,g->mission,512);if(g->level<0 || g->level>10000 || !g->mission[0])return 0;
    skip(&r,4);time=f32(&r);if(!d3_finite(time))return 0;skip(&r,4+4+2);weather=i32(&r);if(weather<0 || weather>65536)return 0;skip(&r,(D3U32)weather);
    doors=i32(&r);active=i32(&r);if(doors<0 || doors>65536 || active<0 || active>doors)return 0;skip(&r,(D3U32)doors*4);
    g->highest_room=i16(&r);n=i16(&r);if(n<1 || n>16384 || g->highest_room< -1 || g->highest_room>=n)return 0;
    g->map_count=(D3U32)n;g->map=r.p;for(i=0;i<n;i++)if(u8(&r)>1)return 0;
    return r.ok;
}
static int valid_player(D3Game *g,D3U32 p,const D3Object *o){
    Reader r;int i,ship,primary,secondary,matcens;float energy,ab,value;
    if(p>g->size || D3_PLAYER_SIZE>g->size-p)return 0;
    for(i=0;i<20;i++)if(g->data[p+64+i]==0)break;if(i==20)return 0;
    ship=(D3I32)d3_u32(g->data+p+60);if(ship<0 || ship>=g->ship_count || !g->ships[ship][0])return 0;
    energy=d3_float(g->data+p+104);ab=d3_float(g->data+p+348);
    if(!d3_finite(energy) || energy<0 || energy>1e9f || !d3_finite(ab) || ab<0 || ab>1e6f)return 0;
    if(d3_u16(g->data+p+136)!=(o->handle&0x7ffu))return 0;
    primary=(D3I32)d3_u32(g->data+p+216);secondary=(D3I32)d3_u32(g->data+p+228);
    if(primary<0 || primary>9 || secondary<10 || secondary>19)return 0;
    for(i=0;i<6;i++){value=d3_float(g->data+p+432+4*i);if(!d3_finite(value) || value<0 || value>1e6f)return 0;}
    r=reader(g->data,g->size,p+496);if(d3_u32(g->data+p+420))skip(&r,4);
    if(!inventory(&r,&g->inventory) || !inventory(&r,&g->countermeasures))return 0;
    if((D3I32)d3_u32(g->data+p+312)!=g->inventory.count || (D3I32)d3_u32(g->data+p+324)!=g->countermeasures.count)return 0;
    matcens=i32(&r);return r.ok && matcens>=0 && matcens<=4096;
}
int d3_parse(D3Game *g,const D3U8 *data,D3U32 size){
    D3Object o,next,best;D3U32 p,end,best_player=0;int expected,matches=0,candidates=0;
    d3_zero(g,sizeof(*g));g->data=data;g->size=size;
    if(!data || size<600 || size>D3_MAX_SAVE)return fail(g,"Keine unterstuetzte Datei (600 Byte bis 32 MiB).");
    if(!header(g))return fail(g,"Unbekannter oder beschaedigter Kopf-, Bild- oder Kartenblock (Format 0 bis 2 erwartet).");
    for(p=g->map+g->map_count;p+7<size;p++){
        if(data[p]!=11 || data[p+1]!=219 || data[p+2]!=186 || data[p+3]!=0 || data[p+4]!=4)continue;
        if(!object(data,size,p,g->version,&o) || o.id!=0 || (o.handle&0x7ffu)>=1500 || (o.batteries!=0 && o.batteries!=21))continue;
        if(++candidates>64)return fail(g,"Zu viele moegliche Spielerobjekte. Datei wird nicht bearbeitet.");
        end=o.end;expected=(int)(o.handle&0x7ffu)+1;
        while(end+5<size && d3_u32(data+end)==0xBADB0Bu && expected<1500){
            if(!object(data,size,end,g->version,&next) || (next.type!=255 && (next.handle&0x7ffu)!=(D3U32)expected)){end=size;break;}
            end=next.end;expected++;
        }
        if(end+2+496>size || d3_u16(data+end)!=496)continue;
        if(valid_player(g,end+2,&o)){matches++;best=o;best_player=end+2;}
    }
    if(matches!=1)return fail(g,matches ? "Mehrere passende Spielerbloecke. Bearbeitung abgelehnt." : "Kein passender historischer 32-Bit-Spielerblock (496 Byte) gefunden.");
    if(!valid_player(g,best_player,&best))return fail(g,"Spielerblock konnte nicht erneut bestaetigt werden.");
    g->player=best_player;g->object=best;d3_string(g->callsign,20,(const char*)data+best_player+64);g->error=0;return 1;
}
const char *d3_ship(const D3Game *g){return g->ships[d3_u32(g->data+g->player+60)];}
int d3_has_item(const D3Game *g,int id){int i;if(id<0)return 0;for(i=0;i<g->inventory.count;i++)if(g->inventory.items[i].type==7 && g->inventory.items[i].id==id)return 1;return 0;}
int d3_god(const D3Game *g){return (d3_u32(g->data+g->player+84)&1)!=0;}
int d3_cloak(const D3Game *g){return g->object.has_effect && (d3_u32(g->data+g->object.effect)&1025)!=0;}
int d3_quad(const D3Game *g){return g->object.batteries==21 && (d3_u32(g->data+g->object.battery+104)&16)!=0;}
void d3_current(const D3Game *g,D3Options *o){
    int i;D3U32 mask;d3_zero(o,sizeof(*o));
    o->energy=d3_float(g->data+g->player+104);o->shields=d3_float(g->data+g->object.shield);o->afterburner=d3_float(g->data+g->player+348);
    o->score=(D3I32)d3_u32(g->data+g->player+88);o->lives=g->data[g->player+108];
    o->primary=(D3I32)d3_u32(g->data+g->player+216);o->secondary=(D3I32)d3_u32(g->data+g->player+228);
    mask=d3_u32(g->data+g->player+168);for(i=0;i<20;i++){o->owned[i]=(D3U8)((mask>>i)&1);o->ammo[i]=d3_u16(g->data+g->player+172+2*i);}
    o->god=o->cloak=o->cooler=o->converter=o->quad=-1;
}

typedef struct {D3U32 start,length,size;const D3U8 *bytes;} Patch;
static void add_patch(Patch *p,int *n,D3U32 start,D3U32 length,const D3U8 *bytes,D3U32 size){p[*n].start=start;p[*n].length=length;p[*n].bytes=bytes;p[*n].size=size;(*n)++;}
static int equipment(D3Inventory *inv,int id,const char *name,int flags,int request){
    int i,j;if(request<0)return 1;if(id<0)return 0;
    for(i=0;i<inv->count;i++)if(inv->items[i].type==7 && inv->items[i].id==id)break;
    if(request && i==inv->count){
        D3Item *x;if(inv->count==10)return 0;x=&inv->items[inv->count++];d3_zero(x,sizeof(*x));
        x->type=x->original_type=7;x->id=x->original_id=id;x->count=1;x->iflags=flags;d3_string(x->name,512,name);
    }else if(!request && i<inv->count){
        if(inv->selected==i)inv->selected=0;else if(inv->selected>i)inv->selected--;
        for(j=i;j+1<inv->count;j++)d3_copy(&inv->items[j],&inv->items[j+1],sizeof(D3Item));inv->count--;
    }
    return 1;
}
static void write32(D3U8 *bytes,D3U32 *p,D3U32 value){d3_put32(bytes+*p,value);*p+=4;}
static void write_z(D3U8 *bytes,D3U32 *p,const char *text){D3U32 n=d3_length(text)+1;d3_copy(bytes+*p,text,n);*p+=n;}
static D3U32 inventory_bytes(const D3Inventory *inv,D3U8 *dest){
    D3U32 p=0;int i;const D3Item *x;write32(dest,&p,(D3U32)inv->count);
    for(i=0;i<inv->count;i++){
        x=&inv->items[i];write32(dest,&p,(D3U32)x->type);write32(dest,&p,(D3U32)x->original_type);write32(dest,&p,(D3U32)x->id);write32(dest,&p,(D3U32)x->original_id);write32(dest,&p,(D3U32)x->flags);write32(dest,&p,(D3U32)x->count);
        write_z(dest,&p,x->description);write_z(dest,&p,x->icon);write_z(dest,&p,x->name);write32(dest,&p,(D3U32)x->iflags);
    }write32(dest,&p,(D3U32)inv->selected);return p;
}
int d3_edit(D3Game *g,const D3Options *o,D3Buffer *out){
    D3U8 raw[496],shield[4],effect[167],quad0[4],quad5[4];D3U8 *inv_bytes=0,*map=0;D3Inventory *inv=0;D3Game *verify=0;
    Patch patches[8],temp;int n=0,i,j,primary,secondary,primaries=0;D3U32 mask,oldmask,flags,inv_size,total,source_pos,dest_pos;
    const char *error=0;out->data=0;out->size=0;
    if(!d3_finite(o->energy) || o->energy<0 || o->energy>1e9f || !d3_finite(o->shields) || o->shields<0 || o->shields>1e9f || !d3_finite(o->afterburner) || o->afterburner<0 || o->afterburner>1e6f || o->lives<0 || o->lives>255)return fail(g,"Ungueltiger Spielerwert.");
    oldmask=d3_u32(g->data+g->player+168);for(i=0;i<10;i++)if(o->owned[i])primaries++;
    if(!primaries && (oldmask&1023))return fail(g,"Mindestens eine Primaerwaffe muss vorhanden bleiben.");
    inv=(D3Inventory*)d3_alloc(sizeof(*inv));inv_bytes=(D3U8*)d3_alloc(16384);verify=(D3Game*)d3_alloc(sizeof(*verify));
    if(!inv || !inv_bytes || !verify){error="Nicht genug Arbeitsspeicher.";goto cleanup;}
    d3_copy(inv,&g->inventory,sizeof(*inv));
    if(!equipment(inv,g->cooler_id,"AB Cooler",128,o->cooler) || !equipment(inv,g->converter_id,"Energy to Shield Converter",131,o->converter)){
        error="Ausruestungsdefinition fehlt oder Inventar ist voll (maximal 10 Arten).";goto cleanup;
    }
    if(o->quad>=0){
        if(g->object.batteries!=21 || !equipment(inv,g->quad_id,"Quad Laser",132,o->quad)){error="Quad-Laser nicht zuweisbar: Definition/Batterien fehlen oder Inventar voll.";goto cleanup;}
        flags=d3_u32(g->data+g->object.battery+104)&~16u;if(o->quad)flags|=16;d3_put32(quad0,flags);add_patch(patches,&n,g->object.battery+104,4,quad0,4);
        flags=d3_u32(g->data+g->object.battery+5*108+104)&~16u;if(o->quad)flags|=16;d3_put32(quad5,flags);add_patch(patches,&n,g->object.battery+5*108+104,4,quad5,4);
    }
    d3_copy(raw,g->data+g->player,496);d3_put_float(raw+104,o->energy);d3_put_float(raw+348,o->afterburner);d3_put32(raw+88,(D3U32)o->score);raw[108]=(D3U8)o->lives;
    mask=oldmask&~0xfffffu;for(i=0;i<20;i++){if(o->owned[i])mask|=1u<<i;d3_put16(raw+172+2*i,o->ammo[i]);}d3_put32(raw+168,mask);
    primary=o->primary;secondary=o->secondary;
    if(primary<0 || primary>9 || (!o->owned[primary] && (mask!=oldmask || primary!=(D3I32)d3_u32(raw+216)))){for(primary=0;primary<10;primary++)if(o->owned[primary])break;if(primary==10)primary=0;}
    if(secondary<10 || secondary>19 || (!o->owned[secondary] && (mask!=oldmask || secondary!=(D3I32)d3_u32(raw+228)))){for(secondary=10;secondary<20;secondary++)if(o->owned[secondary])break;if(secondary==20)secondary=10;}
    d3_put32(raw+216,(D3U32)primary);d3_put32(raw+228,(D3U32)secondary);
    if(o->god>=0){flags=d3_u32(raw+84)&~(1u|(1u<<22));if(o->god)flags|=1;d3_put32(raw+84,flags);d3_put_float(raw+116,o->god?1e9f:0);}
    d3_put32(raw+312,(D3U32)inv->count);inv_size=inventory_bytes(inv,inv_bytes);
    if(inv_size!=g->inventory.end-g->inventory.start || !d3_equal(inv_bytes,g->data+g->inventory.start,inv_size))add_patch(patches,&n,g->inventory.start,g->inventory.end-g->inventory.start,inv_bytes,inv_size);
    if(o->cloak>=0 && (g->object.has_effect || o->cloak)){
        d3_zero(effect,167);effect[0]=1;d3_put16(effect+1,164);
        if(g->object.has_effect)d3_copy(effect+3,g->data+g->object.effect,164);
        else{d3_put_float(effect+3+4,1);d3_put32(effect+3+56,0xffffffffu);d3_put_float(effect+3+140,1);d3_put32(effect+3+144,0xffffffffu);d3_put32(effect+3+148,0xffffffffu);}
        flags=d3_u32(effect+3)&~(1u|512u|1024u|4096u);if(o->cloak)flags|=1;d3_put32(effect+3,flags);d3_put_float(effect+3+12,o->cloak?1e9f:0);d3_put_float(effect+3+36,0);d3_put_float(effect+3+40,0);
        add_patch(patches,&n,g->object.effect_record,g->object.has_effect?167:1,effect,167);
    }
    if(o->reveal_map){map=(D3U8*)d3_alloc(g->map_count);if(!map){error="Nicht genug Arbeitsspeicher.";goto cleanup;}for(i=0;i<(int)g->map_count;i++)map[i]=1;add_patch(patches,&n,g->map,g->map_count,map,g->map_count);}
    d3_put_float(shield,o->shields);add_patch(patches,&n,g->object.shield,4,shield,4);add_patch(patches,&n,g->player,496,raw,496);
    for(i=0;i<n;i++)for(j=i+1;j<n;j++)if(patches[j].start<patches[i].start){temp=patches[i];patches[i]=patches[j];patches[j]=temp;}
    total=g->size;source_pos=0;
    for(i=0;i<n;i++){
        if(patches[i].start<source_pos || patches[i].start>g->size || patches[i].length>g->size-patches[i].start){error="Ueberlappende Aenderungen.";goto cleanup;}
        source_pos=patches[i].start+patches[i].length;total=total-patches[i].length+patches[i].size;
    }
    if(total>D3_MAX_SAVE){error="Die Ausgabedatei ist zu gross.";goto cleanup;}
    out->data=(D3U8*)d3_alloc(total);if(!out->data){error="Nicht genug Arbeitsspeicher.";goto cleanup;}out->size=total;source_pos=dest_pos=0;
    for(i=0;i<n;i++){
        d3_copy(out->data+dest_pos,g->data+source_pos,patches[i].start-source_pos);dest_pos+=patches[i].start-source_pos;
        d3_copy(out->data+dest_pos,patches[i].bytes,patches[i].size);dest_pos+=patches[i].size;source_pos=patches[i].start+patches[i].length;
    }d3_copy(out->data+dest_pos,g->data+source_pos,g->size-source_pos);
    if(!d3_parse(verify,out->data,out->size)){error="Bearbeitete Ausgabe konnte nicht erneut eingelesen werden.";goto cleanup;}
cleanup:
    d3_free(inv);d3_free(inv_bytes);d3_free(verify);d3_free(map);
    if(error){d3_free(out->data);out->data=0;out->size=0;return fail(g,error);}g->error=0;return 1;
}

int d3_limits_table(const D3U8 *data,D3U32 size,const char *ship,D3U16 caps[21],int *found,const char **error){
    Reader r=reader(data,size,0),s;D3U32 start,length,end;int type,version,i,j,cap;char name[512];D3U16 values[21];*error=0;
    while(r.p<r.n && r.ok){
        type=u8(&r);start=r.p;length=(D3U32)i32(&r);if(length<4 || start>size || length>size-start){*error="Ungueltige Laenge einer Spieldatenseite.";return 0;}end=start+length;
        if(type==6){
            s=reader(data,end,r.p);version=i16(&s);if(version<1 || version>6){*error="Unbekannte Version einer Schiffsdefinition.";return 0;}
            z(&s,name,512);for(i=0;i<6;i++)z(&s,0,0);skip(&s,8+68+12);
            for(i=0;i<21;i++){
                u8(&s);for(j=0;j<3;j++)z(&s,0,0);cap=i32(&s);if(cap<0 || cap>65535){*error="Unbekannte Munitionsgrenze.";return 0;}values[i]=(D3U16)cap;
                skip(&s,version>=6?211:210);for(j=0;j<16;j++)z(&s,0,0);
            }
            if(!s.ok || s.p!=end){*error="Beschaedigte Schiffsdefinition.";return 0;}
            if(d3_string_iequal(name,ship)){d3_copy(caps,values,sizeof(values));*found=1;}
        }r.p=end;
    }
    if(!r.ok){*error="Spieldatentabelle ist abgeschnitten.";return 0;}return 1;
}

int d3_number(const char *text,float *value){
    double n=0,scale=1;int digits=0,point=0,negative=0;const char *p=text;
    while(*p==' ' || *p=='\t')p++;if(*p=='+' || *p=='-'){negative=*p=='-';p++;}
    for(;*p && *p!=' ' && *p!='\t';p++){
        if(*p=='.' || *p==','){if(point)return 0;point=1;continue;}
        if(*p<'0' || *p>'9' || ++digits>20)return 0;
        if(point){scale*=10;n+=(*p-'0')/scale;}else{n=n*10+(*p-'0');if(n>1e9)return 0;}
    }
    while(*p==' ' || *p=='\t')p++;if(*p || !digits || n>1e9)return 0;*value=(float)(negative?-n:n);return d3_finite(*value);
}
int d3_integer(const char *text,D3I32 minimum,D3I32 maximum,D3I32 *value){
    const char *p=text;int negative=0,digits=0;D3U32 n=0,limit;D3I32 v;
    while(*p==' ' || *p=='\t')p++;if(*p=='+' || *p=='-'){negative=*p=='-';p++;}limit=negative?2147483648u:2147483647u;
    while(*p>='0' && *p<='9'){if(n>(limit-(D3U32)(*p-'0'))/10)return 0;n=n*10+(D3U32)(*p++-'0');digits++;}
    while(*p==' ' || *p=='\t')p++;if(*p || !digits)return 0;v=negative?(D3I32)(0u-n):(D3I32)n;
    if(v<minimum || v>maximum)return 0;*value=v;return 1;
}
void d3_format_u32(char *dest,D3U32 n){char tmp[16];int count=0,i;do{tmp[count++]=(char)('0'+n%10);n/=10;}while(n);for(i=0;i<count;i++)dest[i]=tmp[count-i-1];dest[count]=0;}
void d3_format_i32(char *dest,D3I32 n){if(n<0){*dest++='-';d3_format_u32(dest,0u-(D3U32)n);}else d3_format_u32(dest,(D3U32)n);}
void d3_format_float(char *dest,float f){
    D3U32 whole,fraction;char *p;int i;
    if(!d3_finite(f)){d3_string(dest,32,"ungueltig");return;}
    if(f>1e9f || f< -1e9f){d3_string(dest,32,f<0?"<-1e9":">1e9");return;}
    if(f<0){*dest++='-';f=-f;}whole=(D3U32)f;fraction=(D3U32)(((double)f-whole)*1000+0.5);if(fraction==1000){whole++;fraction=0;}
    d3_format_u32(dest,whole);if(!fraction)return;p=dest+d3_length(dest);*p++='.';
    for(i=100;i>=1;i/=10){*p++=(char)('0'+fraction/(D3U32)i);fraction%=(D3U32)i;}*p=0;while(p>dest && p[-1]=='0')*--p=0;
}
