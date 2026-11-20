/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef D3_LAUNCHER_ANIMATION_H
#define D3_LAUNCHER_ANIMATION_H

#include <windows.h>

int launcher_animation_start(HWND window, int x, int y);
int launcher_animation_tick(HWND window, UINT_PTR timer_id);
int launcher_animation_paint(HWND window);
void launcher_animation_stop(HWND window);

#endif
