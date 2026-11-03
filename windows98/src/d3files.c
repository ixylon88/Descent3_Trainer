/* SPDX-License-Identifier: GPL-3.0-or-later
 * Windows implementation imports only ANSI Win32 APIs available on Win9x.
 * Replacement uses two checked MoveFileA calls plus rollback, not MoveFileEx,
 * ReplaceFile, Unicode APIs or a claim of transactional FAT writes.
 */
#include "d3files.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
typedef HANDLE F;
#define BAD_FILE INVALID_HANDLE_VALUE
static F open_read(const char *p){return CreateFileA(p,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);}
static void close_file(F f){if(f!=BAD_FILE)CloseHandle(f);}
static int file_size(F f,D3U32 *n){DWORD high=0,low=GetFileSize(f,&high);if(high || low==0xffffffffu || low>0x7fffffffu)return 0;*n=(D3U32)low;return 1;}
static int read_at(F f,D3U32 p,D3U8 *d,D3U32 n){DWORD got;D3U32 done=0;if(SetFilePointer(f,(LONG)p,0,FILE_BEGIN)==0xffffffffu)return 0;while(done<n){if(!ReadFile(f,d+done,n-done,&got,0) || !got)return 0;done+=got;}return 1;}
static int exists(const char *p){DWORD x=GetFileAttributesA(p);return x==0xffffffffu ? 0 : (x&FILE_ATTRIBUTE_DIRECTORY)?2:1;}
static int platform_move_file(const char *a,const char *b){return MoveFileA(a,b)!=0;}
static void remove_file(const char *p){DeleteFileA(p);}
static int same_file_name(const char *a,const char *b){return lstrcmpiA(a,b)==0;}
static int make_temp(const char *dir,char out[260]){return GetTempFileNameA(dir,"D3E",0,out)!=0;}
static int write_temp(const char *path,const D3Buffer *out){
    F f=CreateFileA(path,GENERIC_WRITE,0,0,TRUNCATE_EXISTING,FILE_ATTRIBUTE_NORMAL,0);D3U32 done=0;DWORD put;int ok=1;
    if(f==BAD_FILE)return 0;while(done<out->size){if(!WriteFile(f,out->data+done,out->size-done,&put,0) || !put){ok=0;break;}done+=put;}if(ok && !FlushFileBuffers(f))ok=0;close_file(f);return ok;
}
static void stamp(char *out){SYSTEMTIME t;GetLocalTime(&t);wsprintfA(out,"%04u%02u%02u-%02u%02u%02u",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond);}
#else
/* The same replacement policy is exercised by host tests without Windows. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
typedef FILE *F;
#define BAD_FILE ((FILE*)0)
static F open_read(const char *p){return fopen(p,"rb");}
static void close_file(F f){if(f)fclose(f);}
static int file_size(F f,D3U32 *n){long s;if(fseek(f,0,SEEK_END))return 0;s=ftell(f);if(s<0 || s>2147483647L)return 0;*n=(D3U32)s;return 1;}
static int read_at(F f,D3U32 p,D3U8 *d,D3U32 n){return !fseek(f,(long)p,SEEK_SET) && fread(d,1,n,f)==n;}
static int exists(const char *p){struct stat s;if(stat(p,&s))return 0;return S_ISDIR(s.st_mode)?2:1;}
static int platform_move_file(const char *a,const char *b){if(link(a,b))return 0;if(unlink(a)){unlink(b);return 0;}return 1;}
static void remove_file(const char *p){unlink(p);}
static int same_file_name(const char *a,const char *b){return d3_string_equal(a,b);}
static int make_temp(const char *dir,char out[260]){int fd;d3_string(out,260,dir);d3_append(out,260,"/d3-edit-XXXXXX");fd=mkstemp(out);if(fd<0)return 0;close(fd);return 1;}
static int write_temp(const char *path,const D3Buffer *out){FILE *f=fopen(path,"wb");int ok;if(!f)return 0;ok=fwrite(out->data,1,out->size,f)==out->size;if(fflush(f) || fsync(fileno(f)))ok=0;if(fclose(f))ok=0;return ok;}
static void stamp(char *out){struct tm *t;time_t now=time(0);t=localtime(&now);strftime(out,32,"%Y%m%d-%H%M%S",t);}
#endif

#ifdef D3_TESTING
int d3_test_move_count,d3_test_fail_move_a,d3_test_fail_move_b;
#endif
static int move_file(const char *a,const char *b){
#ifdef D3_TESTING
    d3_test_move_count++;
    if(d3_test_move_count==d3_test_fail_move_a || d3_test_move_count==d3_test_fail_move_b)return 0;
#endif
    return platform_move_file(a,b);
}

int d3_read_file(const char *path,D3Buffer *out,const char **error){
    F f=open_read(path);D3U32 size;out->data=0;out->size=0;*error=0;
    if(f==BAD_FILE){*error="Datei kann nicht geoeffnet werden.";return 0;}
    if(!file_size(f,&size) || size>D3_MAX_SAVE || size<600){close_file(f);*error="Spielstandsdatei hat keine unterstuetzte Groesse (600 Byte bis 32 MiB).";return 0;}
    out->data=(D3U8*)d3_alloc(size);if(!out->data){close_file(f);*error="Nicht genug Arbeitsspeicher.";return 0;}
    if(!read_at(f,0,out->data,size)){d3_free(out->data);out->data=0;close_file(f);*error="Datei konnte nicht vollstaendig gelesen werden.";return 0;}
    out->size=size;close_file(f);return 1;
}
static int original_matches(const char *source,const D3U8 hash[32]){
    D3Buffer now;D3U8 digest[32];const char *error;if(!d3_read_file(source,&now,&error))return 0;d3_sha256(now.data,now.size,digest);d3_free(now.data);return d3_equal(digest,hash,32);
}
int d3_write_file(const char *source,const D3U8 hash[32],const char *destination,const D3Buffer *out,D3WriteReport *report){
    char temp[260],dir[260],date[32],suffix[16];D3U32 len,i;int replacing,same,backed=0;D3Game *check;
    d3_zero(report,sizeof(*report));temp[0]=0;
    check=(D3Game*)d3_alloc(sizeof(*check));if(!check){report->error="Nicht genug Arbeitsspeicher.";return 0;}
    if(!d3_parse(check,out->data,out->size)){report->error="Ungueltige Ausgabe. Es wurde nichts geschrieben.";d3_free(check);return 0;}d3_free(check);
    len=d3_length(destination);if(!len || len>200){report->error="Der Zielpfad ist zu lang (hoechstens 200 ANSI-Zeichen inklusive Dateiname).";return 0;}
    same=same_file_name(source,destination);if(same && !original_matches(source,hash)){report->error="Das Original wurde inzwischen veraendert. Bitte erneut oeffnen.";return 0;}
    replacing=exists(destination);if(replacing==2){report->error="Das Ziel ist ein Verzeichnis.";return 0;}
    d3_string(dir,260,destination);for(i=len;i>0;i--)if(dir[i-1]=='/' || dir[i-1]=='\\')break;
    if(i){if(i==1 || (i==3 && dir[1]==':'))dir[i]=0;else dir[i-1]=0;}else d3_string(dir,260,".");
    if(!make_temp(dir,temp)){report->error="Temporaere Datei im Zielordner konnte nicht angelegt werden.";return 0;}
    if(!write_temp(temp,out)){report->error="Neue Datei konnte nicht vollstaendig geschrieben und geflusht werden.";goto failure;}
    /* The original is checked again immediately before its first rename. */
    if(same && !original_matches(source,hash)){report->error="Das Original wurde waehrend der Bearbeitung veraendert. Bitte erneut oeffnen.";goto failure;}
    if(replacing){
        stamp(date);
        for(i=0;i<1000;i++){
            d3_string(report->backup,260,destination);d3_append(report->backup,260,".bak-");d3_append(report->backup,260,date);d3_append(report->backup,260,"-");d3_format_u32(suffix,i);d3_append(report->backup,260,suffix);
            if(!exists(report->backup))break;
        }
        if(i==1000 || !move_file(destination,report->backup)){report->error="Original konnte nicht als Backup umbenannt werden. Original bleibt erhalten.";report->backup[0]=0;goto failure;}backed=1;
    }
    if(!move_file(temp,destination)){
        if(backed && !move_file(report->backup,destination)){d3_string(report->recovery,260,report->backup);report->error="Austausch und Ruecksetzen fehlgeschlagen. Original liegt im angegebenen Backup.";}
        else{report->backup[0]=0;report->error=backed?"Austausch fehlgeschlagen. Das Original wurde wiederhergestellt.":"Neue Datei konnte nicht am Ziel bereitgestellt werden. Vorhandene Dateien blieben erhalten.";}
        goto failure;
    }
    return 1;
failure:
    if(temp[0])remove_file(temp);return 0;
}

static int table_at(F file,D3U32 pos,D3U32 size,const char *ship,D3U16 caps[21],int *found,const char **error){
    D3U8 *bytes;int ok;
    if(size>64u*1024u*1024u){*error="Spieldatentabelle ist groesser als 64 MiB.";return 0;}
    bytes=(D3U8*)d3_alloc(size);if(!bytes){*error="Nicht genug Arbeitsspeicher.";return 0;}
    if(!read_at(file,pos,bytes,size)){d3_free(bytes);*error="Spieldatentabelle konnte nicht vollstaendig gelesen werden.";return 0;}
    ok=d3_limits_table(bytes,size,ship,caps,found,error);d3_free(bytes);return ok;
}
int d3_load_limits_file(const char *path,const char *ship,D3U16 caps[21],const char **error){
    F file=open_read(path);D3U32 size,count,offset,index,i,pos,length,n;D3U8 head[12],entry[48];D3U16 parsed[21];char name[37];int pass,base,found=0,ok=0;
    *error=0;if(file==BAD_FILE){*error="Spieldaten konnten nicht geoeffnet werden.";return 0;}
    if(!file_size(file,&size) || size<12 || !read_at(file,0,head,12)){*error="Spieldatendatei ist abgeschnitten oder zu gross.";goto done;}
    if(!d3_equal(head,"HOG2",4)){
        if(!table_at(file,0,size,ship,parsed,&found,error))goto done;
    }else{
        count=d3_u32(head+4);offset=d3_u32(head+8);
        if(count>1000000 || offset>size || count*48>offset){*error="Ungueltiger HOG2-Kopf.";goto done;}index=offset-count*48;
        if(index!=68 && index!=72){*error="Unbekannter HOG2-Index.";goto done;}
        for(pass=0;pass<2;pass++){
            pos=offset;
            for(i=0;i<count;i++){
                if(!read_at(file,index+i*48,entry,48)){*error="HOG2-Index ist abgeschnitten.";goto done;}
                d3_copy(name,entry,36);name[36]=0;length=d3_u32(entry+40);if(pos>size || length>size-pos){*error="HOG2-Eintrag ist ungueltig.";goto done;}
                n=d3_length(name);base=d3_string_iequal(name,"Table.gam");
                if(n>=4 && d3_string_iequal(name+n-4,".gam") && (pass==0 ? base : !base)){
                    if(!table_at(file,pos,length,ship,parsed,&found,error))goto done;
                }pos+=length;
            }
        }
    }
    if(!found){*error="Keine passende Schiffsdefinition gefunden (Version 1 bis 6).";goto done;}
    d3_copy(caps,parsed,42);ok=1;
done:
    close_file(file);return ok;
}
