/* SPDX-License-Identifier: GPL-3.0-or-later */
#define WIN32_LEAN_AND_MEAN
#include "launcher_animation.h"
#include "d3core.h"

enum { FRAME_COUNT = 9, FRAME_SIZE = 64, FRAME_INTERVAL_MS = 100, ANIMATION_TIMER_ID = 1 };

static HWND image;
static HBITMAP frames[FRAME_COUNT];
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

static void release_frames(void) {
    int index;
    for (index = 0; index < FRAME_COUNT; index++) {
        if (frames[index]) {
            DeleteObject(frames[index]);
            frames[index] = NULL;
        }
    }
}

static void show_frame(HBITMAP bitmap) {
    HBITMAP previous = (HBITMAP)SendMessageA(image, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmap);
    /* Some versions of the static control copy bitmaps containing alpha. */
    if (previous && previous != frames[current_frame]) DeleteObject(previous);
}

int launcher_animation_start(HWND window, HINSTANCE instance, int x, int y) {
    if (!load_frames()) goto failed;

    image = CreateWindowExA(0, "STATIC", "", WS_CHILD | SS_BITMAP | SS_CENTERIMAGE,
                            x, y, FRAME_SIZE, FRAME_SIZE, window, NULL, instance, NULL);
    if (!image || !SetTimer(window, ANIMATION_TIMER_ID, FRAME_INTERVAL_MS, NULL)) goto failed;

    current_frame = 0;
    show_frame(frames[current_frame]);
    ShowWindow(image, SW_SHOW);
    return 1;

failed:
    if (image) DestroyWindow(image);
    image = NULL;
    release_frames();
    return 0;
}

int launcher_animation_tick(UINT_PTR timer_id) {
    int next;
    if (!image || timer_id != ANIMATION_TIMER_ID) return 0;

    next = (current_frame + 1) % FRAME_COUNT;
    show_frame(frames[next]);
    current_frame = next;
    return 1;
}

void launcher_animation_stop(HWND window) {
    if (image) {
        KillTimer(window, ANIMATION_TIMER_ID);
        show_frame(NULL);
        image = NULL;
    }
    release_frames();
}
