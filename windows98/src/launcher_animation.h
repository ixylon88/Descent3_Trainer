/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef D3_LAUNCHER_ANIMATION_H
#define D3_LAUNCHER_ANIMATION_H

#include <windows.h>

int launcher_animation_start(HWND window, HINSTANCE instance, int x, int y);
int launcher_animation_tick(UINT_PTR timer_id);
void launcher_animation_stop(HWND window);

#endif
