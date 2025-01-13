#ifndef __UNN_DRAW_H_
#define __UNN_DRAW_H_

#include <wchar.h>

#include <notcurses/notcurses.h>
#include <pthread.h>

#include "winbuf.h"
#include "state.h"

inline static void draw_window_line(struct ncplane *p, window *w, offset view, offset cur, int y1, int x1, int width) {
    int len = view.l->len - view.pos;

    if(len <= 0) {
        if(cur.l == view.l) { // if we have cursor at an empty line
            nccell c = { 0 };
            c.gcluster = L' ';

            nccell_set_bg_rgb8(&c, 0, 0, 0);
            nccell_set_fg_rgb8(&c, 255, 255, 255);

            ncplane_putc_yx(S.p, y1, 0, &c);
        }
        return;
    }
    
    char not_fully = len > width;

    wchar_t *wcstr = view.l->str + view.pos;

    wchar_t temp;
    if(not_fully) {
        temp = wcstr[width];
        wcstr[width] = 0;
    }

    ncplane_cursor_move_yx(p, y1, x1);
    ncplane_putwstr(p, wcstr);

    if(cur.l == view.l && (cur.pos >= view.pos)) { // if we have cursor at an empty line
        nccell c = { 0 };

        // L' ' if cursor is at the end, after the last char
        c.gcluster = (cur.pos < view.l->len) ? cur.l->str[cur.pos] : L' ';

        nccell_set_bg_rgb8(&c, 0, 0, 0);
        nccell_set_fg_rgb8(&c, 255, 255, 255);

        ncplane_putc_yx(S.p, y1, x1 + cur.pos - view.pos, &c);
    }

    if(not_fully)
        wcstr[width] = temp;
}

int draw_window(struct ncplane *p, window *w) {
    if(!w) return -1;

    rect pos = w->pos;
    offset cur = w->cur;
    offset view = w->view;

    int width = pos.x2 - pos.x1;
    int height = pos.y2 - pos.y1;

    ncplane_cursor_move_yx(p, pos.y1, pos.x1);
    ncplane_erase_region(p, pos.y1, pos.x1, height, width);

    if(!w->buff) return 0;

    for(int y = pos.y1; y <= pos.y2; y++) {
        if(!view.l) break;
        draw_window_line(p, w, view, cur, y, pos.x1, width);
        view.l = view.l->next;
    }

    return 0;
}

int draw_grid(struct ncplane *p, grid *g, char blocking) {
    if(!g) return -1;

    for(window *w = g->first; w != NULL; w = w->next) {
        if(blocking) {
            pthread_mutex_lock(&w->buff->block);
        } else {
            if(pthread_mutex_trylock(&w->buff->block)) continue;
        }

        draw_window(p, w);

        pthread_mutex_unlock(&w->buff->block);
    }

    return 0;
}

int draw_status(struct ncplane *p) {
    unsigned int max_x, max_y;
    ncplane_dim_yx(p, &max_y, &max_x);

    int y = max_y - 1;
    int x = 0;

    ncplane_set_fg_rgb8(S.p, 255, 255, 255);
    ncplane_set_bg_rgb8(S.p, 0, 0, 0);

    for(int i = 0; i < max_x; i++) {
        ncplane_putchar_yx(S.p, y, x + i, ' ');
    }

    wchar_t buffer[512] = { 0 };

    wchar_t *msg = L"";

    char locked = 0;

    if(!pthread_mutex_trylock(&S.status_message_block)) {
        locked = 1;
        msg = S.status_message;
    }

    swprintf(buffer, sizeof(buffer) - 1, 
    L"unn [%s] %s [%s] %ls",
    // buffer name
    ((S.current_window) ?
        (((S.current_window->buff) ?
            S.current_window->buff->name : 
            "NO BUFFER")) : 
        "NO WINDOW"),
    // mode
    (flag_is_on(S.flags, FLAG_EDIT) ?
        "EDIT" : 
        "MOVE"),
    // input
    S.input_buffer,
    // message
    msg);
    
    if(locked) {
        pthread_mutex_unlock(&S.status_message_block);
    }

    ncplane_putwstr_yx(p, y, x, buffer);

    ncplane_set_bg_rgb8(S.p, 255, 255, 255);
    ncplane_set_fg_rgb8(S.p, 0, 0, 0);

    return 0;
}

#endif