/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef D3_FILES_H
#define D3_FILES_H
#include "d3core.h"
#define D3_PATH 260
typedef struct {char backup[D3_PATH],recovery[D3_PATH];const char *error;} D3WriteReport;
int d3_read_file(const char *path,D3Buffer *out,const char **error);
int d3_write_file(const char *source,const D3U8 expected_hash[32],const char *destination,const D3Buffer *out,D3WriteReport *report);
int d3_load_limits_file(const char *path,const char *ship,D3U16 caps[21],const char **error);
#endif
