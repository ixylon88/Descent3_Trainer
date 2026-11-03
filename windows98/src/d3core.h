/* SPDX-License-Identifier: GPL-3.0-or-later
 * Format: DescentDevelopers/Descent3 d3-historical
 * commit 84e25b0b411284bad57b34f50fd976f1a8a2367f.
 * Format/declaration provenance: Copyright (C) 2024 Parallax Software.
 */
#ifndef D3_CORE_H
#define D3_CORE_H

typedef unsigned char D3U8;
typedef unsigned short D3U16;
typedef unsigned int D3U32;
typedef int D3I32;
typedef char D3AssertU32[(sizeof(D3U32)==4) ? 1 : -1];
typedef char D3AssertFloat[(sizeof(float)==4) ? 1 : -1];

#define D3_MAX_SAVE (32u*1024u*1024u)
#define D3_MAX_ITEMS 10
#define D3_STRING 512
#define D3_PLAYER_SIZE 496

typedef struct {
    D3I32 type, original_type, id, original_id, flags, count, iflags;
    char description[D3_STRING], icon[D3_STRING], name[D3_STRING];
} D3Item;
typedef struct {
    D3U32 start, end;
    int count, selected;
    D3Item items[D3_MAX_ITEMS];
} D3Inventory;
typedef struct {
    D3U32 start, end, handle, shield, effect_record, effect, battery;
    int id, type, has_effect, batteries;
} D3Object;
typedef struct {
    const D3U8 *data;
    D3U32 size, player, map, map_count;
    int version, level, highest_room, ship_count;
    int cooler_id, converter_id, quad_id;
    char description[32], mission[D3_STRING], callsign[20];
    char ships[128][128];
    D3Object object;
    D3Inventory inventory, countermeasures;
    const char *error;
} D3Game;
typedef struct {
    float energy, shields, afterburner;
    D3I32 score;
    int lives, primary, secondary;
    D3U8 owned[20];
    D3U16 ammo[20];
    /* -1 keep existing; 0 disable; 1 enable with a long timer. */
    int god, cloak, cooler, converter, quad, reveal_map;
} D3Options;
typedef struct {
    D3U8 *data;
    D3U32 size;
} D3Buffer;

extern const char *d3_weapon_names[20];
void *d3_alloc(D3U32 bytes);
void d3_free(void *pointer);
void d3_zero(void *pointer,D3U32 bytes);
void d3_copy(void *dest,const void *source,D3U32 bytes);
int d3_equal(const void *a,const void *b,D3U32 bytes);
D3U32 d3_length(const char *text);
int d3_string_equal(const char *a,const char *b);
int d3_string_iequal(const char *a,const char *b);
void d3_string(char *dest,D3U32 capacity,const char *source);
void d3_append(char *dest,D3U32 capacity,const char *source);
D3U16 d3_u16(const D3U8 *data);
D3U32 d3_u32(const D3U8 *data);
float d3_float(const D3U8 *data);
void d3_put16(D3U8 *data,D3U16 value);
void d3_put32(D3U8 *data,D3U32 value);
void d3_put_float(D3U8 *data,float value);
int d3_finite(float value);
int d3_parse(D3Game *game,const D3U8 *data,D3U32 size);
void d3_current(const D3Game *game,D3Options *options);
int d3_has_item(const D3Game *game,int id);
const char *d3_ship(const D3Game *game);
int d3_god(const D3Game *game);
int d3_cloak(const D3Game *game);
int d3_quad(const D3Game *game);
int d3_edit(D3Game *game,const D3Options *options,D3Buffer *out);
/* A table can be valid without containing the requested ship. */
int d3_limits_table(const D3U8 *data,D3U32 size,const char *ship,D3U16 caps[21],int *found,const char **error);
void d3_sha256(const D3U8 *data,D3U32 size,D3U8 digest[32]);
int d3_number(const char *text,float *value);
int d3_integer(const char *text,D3I32 minimum,D3I32 maximum,D3I32 *value);
void d3_format_u32(char *dest,D3U32 value);
void d3_format_i32(char *dest,D3I32 value);
void d3_format_float(char *dest,float value);
#endif
