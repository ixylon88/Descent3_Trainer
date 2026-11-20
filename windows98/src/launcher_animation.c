/* SPDX-License-Identifier: GPL-3.0-or-later */
#define WIN32_LEAN_AND_MEAN
#include "launcher_animation.h"
#include "d3core.h"

enum { FRAME_COUNT = 9, FRAME_SIZE = 64, FRAME_INTERVAL_MS = 100, ANIMATION_TIMER_ID = 1 };

static HDC frame_dc;
static HRGN frame_clip;
static HBITMAP frames[FRAME_COUNT];
static int frame_left, frame_top;
static int current_frame;

static int load_frames(void) {
    char path[MAX_PATH];
    DWORD length = GetModuleFileNameA(NULL, path, sizeof(path));
    int index;

    if (!length || length >= sizeof(path)) return 0;
    while (length && path[length - 1] != '\\') length--;
    if (!length || length + sizeof("Anim0.bmp") > sizeof(path)) return 0;
    d3_copy(path + length, "Anim0.bmp", sizeof("Anim0.bmp"));

    for (index = 0; index < FRAME_COUNT; index++) {
        BITMAP size;
        path[length + 4] = (char)('0' + index);
        frames[index] = (HBITMAP)LoadImageA(NULL, path, IMAGE_BITMAP, 0, 0,
                                          LR_LOADFROMFILE | LR_CREATEDIBSECTION);
        if (!frames[index] || GetObjectA(frames[index], sizeof(size), &size) != (int)sizeof(size) ||
            size.bmWidth != FRAME_SIZE || size.bmHeight != FRAME_SIZE)
            return 0;
    }
    return 1;
}

static void release_animation(void) {
    int index;
    if (frame_dc) DeleteDC(frame_dc);
    frame_dc = NULL;
    if (frame_clip) DeleteObject(frame_clip);
    frame_clip = NULL;

    for (index = 0; index < FRAME_COUNT; index++) {
        if (frames[index]) {
            DeleteObject(frames[index]);
            frames[index] = NULL;
        }
    }
}

int launcher_animation_start(HWND window, int x, int y) {
    const int clip_inset = 2;

    if (!load_frames()) goto failed;

    frame_clip = CreateEllipticRgn(x + clip_inset, y + clip_inset,
                                  x + FRAME_SIZE - clip_inset, y + FRAME_SIZE - clip_inset);
    if (!frame_clip) goto failed;

    frame_dc = CreateCompatibleDC(NULL);
    if (!frame_dc || !SetTimer(window, ANIMATION_TIMER_ID, FRAME_INTERVAL_MS, NULL)) goto failed;
    frame_left = x;
    frame_top = y;
    current_frame = 0;
    return 1;

failed:
    release_animation();
    return 0;
}

int launcher_animation_tick(HWND window, UINT_PTR timer_id) {
    if (!frame_dc || timer_id != ANIMATION_TIMER_ID) return 0;

    current_frame = (current_frame + 1) % FRAME_COUNT;
    InvalidateRgn(window, frame_clip, FALSE);
    return 1;
}

int launcher_animation_paint(HWND window) {
    PAINTSTRUCT paint;
    HDC target;
    if (!frame_dc) return 0;

    target = BeginPaint(window, &paint);
    /* The paint DC still observes the window's update and visible regions. */
    if (target && SelectClipRgn(target, frame_clip) != ERROR) {
        HGDIOBJ previous = SelectObject(frame_dc, frames[current_frame]);
        if (previous) {
            BitBlt(target, frame_left, frame_top, FRAME_SIZE, FRAME_SIZE, frame_dc, 0, 0, SRCCOPY);
            SelectObject(frame_dc, previous);
        }
        SelectClipRgn(target, NULL);
    }
    EndPaint(window, &paint);
    return 1;
}

void launcher_animation_stop(HWND window) {
    if (frame_dc) KillTimer(window, ANIMATION_TIMER_ID);
    release_animation();
}
