/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../src/d3core.h"
#include "../src/d3files.h"
extern int d3_test_move_count,d3_test_fail_move_a,d3_test_fail_move_b;
static int assertions;
static void check(int ok,const char *message){if(!ok){fprintf(stderr,"FAIL: %s\n",message);exit(1);}assertions++;printf("PASS: %s\n",message);}
static D3Buffer read_file(const char *path){D3Buffer b;const char *error;if(!d3_read_file(path,&b,&error)){fprintf(stderr,"%s: %s\n",path,error);exit(1);}return b;}
static D3Game *parse(const D3Buffer *b){D3Game *g=d3_alloc(sizeof(*g));if(!d3_parse(g,b->data,b->size)){fprintf(stderr,"Parse: %s\n",g->error);exit(1);}return g;}
static void write_bytes(const char *p,const D3U8 *bytes,D3U32 n){FILE *f=fopen(p,"wb");if(!f || fwrite(bytes,1,n,f)!=n || fclose(f))exit(2);}
static void hash_hex(const D3U8 hash[32],char out[65]){int i;for(i=0;i<32;i++)sprintf(out+2*i,"%02x",hash[i]);out[64]=0;}
static void hash_test(void){
    D3U8 hash[32];char text[65];D3U8 *million=d3_alloc(1000000);
    d3_sha256((const D3U8*)"",0,hash);hash_hex(hash,text);check(!strcmp(text,"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"),"SHA-256 empty vector");
    d3_sha256((const D3U8*)"abc",3,hash);hash_hex(hash,text);check(!strcmp(text,"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"),"SHA-256 abc vector");
    memset(million,'a',1000000);d3_sha256(million,1000000,hash);hash_hex(hash,text);check(!strcmp(text,"cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"),"SHA-256 million-byte vector");d3_free(million);
}
static void set_options(D3Options *o){int i;o->energy=200;o->shields=200;o->afterburner=5;o->lives=99;o->score=123456;o->god=o->cloak=o->cooler=o->quad=o->reveal_map=1;o->converter=0;o->primary=9;o->secondary=19;for(i=0;i<20;i++){o->owned[i]=1;o->ammo[i]=(D3U16)(i<10?i*17:65535);}}
static void fixture(const char *path,const char *golden){
    D3Buffer b=read_file(path),out,again,off;D3Game *g=parse(&b),*edited,*disabled;D3Options o;int i;D3U8 *changed;
    check(g->player>0 && d3_string_equal(g->callsign,"Testpilot") && d3_string_equal(d3_ship(g),"Pyro-GL"),"historical player header and ship decoded");
    d3_current(g,&o);check(o.energy==100.125f && o.shields==87.25f && o.ammo[10]==4,"historical values match independent C++ generator");
    check(d3_edit(g,&o,&out) && out.size==b.size && d3_equal(out.data,b.data,b.size),"unchanged file is byte-identical");d3_free(out.data);
    set_options(&o);check(d3_edit(g,&o,&out),"all requested edits accepted");edited=parse(&out);d3_current(edited,&o);
    check(o.energy==200 && o.shields==200 && o.afterburner==5 && o.lives==99 && o.score==123456,"player values changed");
    check(d3_god(edited) && d3_float(out.data+edited->player+116)==1e9f && d3_cloak(edited) && d3_float(out.data+edited->object.effect+12)==1e9f,"effect flags and long timers set");
    for(i=0;i<20;i++)check(o.owned[i] && o.ammo[i]==(D3U16)(i<10?i*17:65535),"weapon ownership and count");
    check(o.primary==9 && o.secondary==19,"active primary and secondary set");
    check(d3_has_item(edited,edited->cooler_id) && !d3_has_item(edited,edited->converter_id) && d3_has_item(edited,edited->quad_id) && d3_quad(edited),"equipment and Quad batteries agree");
    check((d3_u32(out.data+edited->object.battery+5*108+104)&16)!=0,"Super Laser Quad battery set");
    check((d3_u32(out.data+edited->player+168)&0x80100000u)==0x80100000u && d3_u32(out.data+edited->player+468)==7,"flare and unrelated raw fields preserved");
    for(i=0;i<(int)edited->map_count;i++)if(out.data[edited->map+i]!=1)break;check(i==(int)edited->map_count,"entire automap revealed");
    check(d3_equal(out.data+out.size-275,b.data+b.size-275,275),"opaque engine tail retained");
    check(d3_edit(edited,&o,&again) && again.size==out.size && d3_equal(again.data,out.data,out.size),"edited file unchanged round-trip");d3_free(again.data);
    if(golden){D3Buffer reference=read_file(golden);check(out.size==reference.size && d3_equal(out.data,reference.data,out.size),"native output exactly matches independently implemented C# editor");d3_free(reference.data);}
    o.god=o.cloak=o.cooler=o.quad=0;check(d3_edit(edited,&o,&off),"disable edits accepted");disabled=parse(&off);
    check(!d3_god(disabled) && !d3_cloak(disabled) && !d3_quad(disabled) && !d3_has_item(disabled,disabled->cooler_id),"effects and equipment disabled");
    changed=d3_alloc(b.size);d3_copy(changed,b.data,b.size);d3_put16(changed+32,3);check(!d3_parse(disabled,changed,b.size),"unknown version rejected");
    d3_copy(changed,b.data,b.size);d3_put16(changed+g->player-2,536);check(!d3_parse(disabled,changed,b.size),"modern 64-bit player ABI rejected");
    d3_copy(changed,b.data,b.size);d3_put16(changed+g->player+136,13);check(!d3_parse(disabled,changed,b.size),"object/player cross-link mismatch rejected");
    d3_copy(changed,b.data,b.size);d3_put32(changed+g->player+312,2);check(!d3_parse(disabled,changed,b.size),"inventory counts must agree");
    d3_copy(changed,b.data,b.size);changed[g->map]=2;check(!d3_parse(disabled,changed,b.size),"unknown map encoding rejected");
    check(!d3_parse(disabled,b.data,g->player+300),"truncated player rejected");
    d3_current(g,&o);o.owned[0]=0;check(!d3_edit(g,&o,&again),"last primary cannot be removed");
    d3_current(g,&o);o.energy=d3_float((const D3U8*)"\x00\x00\xc0\x7f");check(!d3_edit(g,&o,&again),"NaN inputs rejected");
    d3_copy(changed,b.data,b.size);d3_put32(changed+g->player+84,4097);d3_put_float(changed+g->player+116,12.5f);check(d3_parse(disabled,changed,b.size),"finite saved power-up accepted");
    d3_current(disabled,&o);check(d3_edit(disabled,&o,&again) && again.size==b.size && d3_equal(again.data,changed,b.size),"untouched finite power-up preserved");d3_free(again.data);
    d3_free(changed);d3_free(g);d3_free(edited);d3_free(disabled);d3_free(b.data);d3_free(out.data);d3_free(off.data);
}
static void file_tests(const char *path,const char *dir){
    D3Buffer b=read_file(path),out,read;D3Game *g=parse(&b);D3Options o;D3U8 hash[32];char source[260],copy[260],target[260],backup[260];const char *error;D3WriteReport report;
    snprintf(source,260,"%s/saveg000",dir);snprintf(copy,260,"%s/copy",dir);snprintf(target,260,"%s/rollback",dir);
    write_bytes(source,b.data,b.size);d3_sha256(b.data,b.size,hash);d3_current(g,&o);o.energy=200;check(d3_edit(g,&o,&out),"prepare valid file output");
    check(d3_write_file(source,hash,copy,&out,&report) && !report.backup[0],"copy write needs no existing destination");read=read_file(source);check(d3_equal(read.data,b.data,b.size),"copy preserves original file");d3_free(read.data);
    check(d3_write_file(source,hash,source,&out,&report) && report.backup[0],"replacement creates timestamped backup");d3_string(backup,260,report.backup);read=read_file(backup);check(read.size==b.size && d3_equal(read.data,b.data,b.size),"backup contains exact original bytes");d3_free(read.data);
    check(!d3_write_file(source,hash,source,&out,&report),"external changes prevent overwrite");
    write_bytes(target,b.data,b.size);d3_test_move_count=0;d3_test_fail_move_a=2;d3_test_fail_move_b=0;
    check(!d3_write_file(target,hash,target,&out,&report) && !report.recovery[0],"failed replacement triggers successful rollback");read=read_file(target);check(read.size==b.size && d3_equal(read.data,b.data,b.size),"rollback restores exact original file");d3_free(read.data);
    d3_test_move_count=0;d3_test_fail_move_a=2;d3_test_fail_move_b=3;
    check(!d3_write_file(target,hash,target,&out,&report) && report.recovery[0],"failed rollback reports original recovery backup");read=read_file(report.recovery);check(read.size==b.size && d3_equal(read.data,b.data,b.size),"original remains recoverable after both rename failures");d3_free(read.data);unlink(report.recovery);
    d3_test_fail_move_a=d3_test_fail_move_b=d3_test_move_count=0;
    {D3Buffer invalid;invalid.data=(D3U8*)"bad";invalid.size=3;check(!d3_write_file(source,hash,copy,&invalid,&report),"invalid output rejected before file operations");}
    read=read_file(copy);check(read.size==out.size && d3_equal(read.data,out.data,out.size),"existing destination survives failed validation");d3_free(read.data);
    check(!d3_read_file("/this/file/does/not/exist",&read,&error),"missing file handled without writing");
    unlink(source);unlink(copy);unlink(backup);d3_free(g);d3_free(b.data);d3_free(out.data);
}
static D3Buffer make_table(void){
    D3Buffer b;D3U32 p=5;int i,j;b.data=d3_alloc(8192);b.data[0]=6;d3_put16(b.data+p,6);p+=2;d3_copy(b.data+p,"Pyro-GL",8);p+=8;for(i=0;i<6;i++)b.data[p++]=0;p+=8+68+12;
    for(i=0;i<21;i++){b.data[p++]=0;for(j=0;j<3;j++)b.data[p++]=0;d3_put32(b.data+p,(D3U32)(i*7));p+=4+211;for(j=0;j<16;j++)b.data[p++]=0;}
    d3_put32(b.data+1,p-1);b.size=p;return b;
}
static void table_tests(const char *dir){
    D3Buffer table=make_table(),hog;D3U16 caps[21];int found=0;const char *error;char path[260];
    check(d3_limits_table(table.data,table.size,"pyro-gl",caps,&found,&error) && found && caps[10]==70 && caps[19]==133,"ship limits decoded by saved ship name");
    snprintf(path,260,"%s/Table.gam",dir);write_bytes(path,table.data,table.size);check(d3_load_limits_file(path,"Pyro-GL",caps,&error) && caps[19]==133,"plain ship-table file loaded");unlink(path);
    hog.size=116+table.size;hog.data=d3_alloc(hog.size);d3_copy(hog.data,"HOG2",4);d3_put32(hog.data+4,1);d3_put32(hog.data+8,116);d3_copy(hog.data+68,"Table.gam",10);d3_put32(hog.data+108,table.size);d3_copy(hog.data+116,table.data,table.size);
    snprintf(path,260,"%s/test.hog",dir);write_bytes(path,hog.data,hog.size);check(d3_load_limits_file(path,"Pyro-GL",caps,&error) && caps[10]==70,"HOG2 archive table loaded by offsets");
    d3_put32(hog.data+108,0x7fffffffu);write_bytes(path,hog.data,hog.size);check(!d3_load_limits_file(path,"Pyro-GL",caps,&error),"archive entry outside file rejected");unlink(path);
    d3_put32(table.data+1,0x7fffffffu);check(!d3_limits_table(table.data,table.size,"Pyro-GL",caps,&found,&error),"table length outside file rejected");d3_free(table.data);d3_free(hog.data);
}
static void number_tests(void){
    float f;D3I32 n;char s[64];check(d3_number("100,125",&f) && f==100.125f,"decimal comma accepted");check(d3_number("200.5",&f) && f==200.5f,"decimal point accepted");
    check(!d3_number("1,2.3",&f) && !d3_number("NaN",&f) && !d3_number("1000000001",&f),"malformed or excessive decimal inputs rejected");
    check(d3_integer("-2147483648",(-2147483647-1),2147483647,&n) && n==(-2147483647-1),"signed minimum score accepted");
    check(!d3_integer("2147483648",(-2147483647-1),2147483647,&n) && !d3_integer("65536",0,65535,&n),"integer overflow rejected");
    d3_format_float(s,100.125f);check(!strcmp(s,"100.125"),"decimal formatting");
}
int main(int argc,char **argv){
    const char *names[]={"fixture98","fixture98-noeffects","fixture98-guided","fixture98-snapshot","fixture98-v1","fixture98-v0"};char path[260],work[260];int i;
    if(argc<2)return 2;hash_test();number_tests();
    for(i=0;i<6;i++){snprintf(path,260,"%s/%s",argv[1],names[i]);fixture(path,i==0 && argc>2?argv[2]:0);}
    snprintf(work,260,"%s/io-%ld",argv[1],(long)getpid());if(mkdir(work,0700))return 2;
    snprintf(path,260,"%s/fixture98",argv[1]);file_tests(path,work);table_tests(work);check(rmdir(work)==0,"all temporary file-test artifacts cleaned");
    printf("\n%d assertions passed. Windows 98 runtime and in-game loading remain untested.\n",assertions);return 0;
}
