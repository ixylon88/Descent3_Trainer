/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef D3_TEXT_H
#define D3_TEXT_H

typedef enum {
    D3_TEXT_NONE,
#define D3_TEXT(key, de, en) D3_TEXT_##key,
#include "d3strings.inc"
#undef D3_TEXT
} D3Text;

const char *d3_text(D3Text key);
int d3_is_english(void);
void d3_set_english(int english);

#define T(key) d3_text(D3_TEXT_##key)

#endif
