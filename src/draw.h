/*
    UNN - text editor with high ambitions and far-fetched goals
    Copyright (C) 2025  Sergei Igolnikov

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef __UNN_DRAW_H_
#define __UNN_DRAW_H_

#include <wchar.h>

#include <notcurses/notcurses.h>
#include <pthread.h>

#include "winbuf.h"
#include "state.h"
#include "colors.h"

int digits_count(int number) {
    int d = 1;

    number = number / 10;

    while(number) {
        d++;
        number = number / 10;
    }

    return d;
}

// ugly code >:(
inline static void draw_window_line(struct ncplane *p, window *w, offset view, 
                                    offset cur, int y1, int x1, int width, int dc, char is_mark) {
    int len = (view.l) ? (view.l->len - view.pos) : 0;

    colors cl = w->cl;

    for(int i = x1; i < x1 + width + is_mark; i++) {
        ncplane_putchar_yx(p, y1, i, ' ');
    }
    ncplane_cursor_move_yx(p, y1, x1 - ((dc) ? (dc + 1) : 0));

    if(dc) { // if we number lines
        char buff[16] = { 0 };

        int c;

        if(view.l) {
           c = snprintf(buff, sizeof(buff) - 1, "%d", view.index + y1 + 1);
        } else {
            c = dc;
            for(int i = 0; i < dc; i++) {
                buff[i] = '-';
            }
        }
        int indent = dc - c;

        for(int i = 0; i < indent; i++) {
            ncplane_putchar(p, ' ');
        }
        ncplane_putstr(p, buff);
        ncplane_putchar(p, ' ');
    }

    // empty line
    if(len <= 0) {
        if(cur.l == view.l) { // if we have cursor at an empty line
            nccell c = { 0 };
            c.gcluster = L' ';

            nccell_set_bg_rgb8(&c, cl.cur.bg.r, cl.cur.bg.g, cl.cur.bg.b);
            nccell_set_fg_rgb8(&c, cl.cur.fg.r, cl.cur.fg.g, cl.cur.fg.b);

            ncplane_putc_yx(S.p, y1, x1, &c);
        }
        return;
    }
    
    // if window is not wide enough for the line
    char not_fully = len > width;

    wchar_t *wcstr = view.l->str + view.pos;

    wchar_t temp;
    if(not_fully) {
        temp = wcstr[width];
        wcstr[width] = 0;
    }

    ncplane_putwstr(p, wcstr);

    // draw cursor
    if(cur.l == view.l) {
        if(cur.pos >= view.pos) {
            nccell c = { 0 };

            // L' ' if cursor is at the end, after the last char
            wchar_t ch = (cur.pos < view.l->len) ? cur.l->str[cur.pos] : L' ';

            nccell_load_ucs32(p, &c, ch);

            nccell_set_bg_rgb8(&c, cl.cur.bg.r, cl.cur.bg.g, cl.cur.bg.b);
            nccell_set_fg_rgb8(&c, cl.cur.fg.r, cl.cur.fg.g, cl.cur.fg.b);

            if((cur.pos <= (view.pos + width - 1))) {
                ncplane_putc_yx(S.p, y1, x1 + cur.pos - view.pos, &c);
            } 
        }
    }

    if(not_fully) {
        wcstr[width] = temp;

        if(is_mark) {
            nccell c = { 0 };

            nccell_set_bg_rgb8(&c, cl.cur.bg.r, cl.cur.bg.g, cl.cur.bg.b);
            nccell_set_fg_rgb8(&c, cl.cur.fg.r, cl.cur.fg.g, cl.cur.fg.b);

            c.gcluster = L'>';

            ncplane_putc_yx(S.p, y1, x1 + width, &c);
        }
    }
}

int draw_window(struct ncplane *p, window *w) {
    if(!w) return -1;

    rect pos = w->pos;
    offset cur = w->cur;
    offset view = w->view;

    colors cl = w->cl;

    char is_prompt = flag_is_on(w->buff->flags, BUFFER_PROMPT);
    char is_mark = !!flag_is_on(w->flags, WINDOW_LONG_MARKS);

    int dc = 0;

    int height = pos.y2 - pos.y1 + 1;
    int width = pos.x2 - pos.x1 + 1;

    if(flag_is_on(w->flags, WINDOW_LONG_MARKS)) {
        int n_width = width - 1; // for side markers
        if(n_width < 1) flag_off(w->flags, WINDOW_LONG_MARKS);
        else width = n_width;
    }

    if(flag_is_on(w->flags, WINDOW_LINES) && !is_prompt) {
        dc = digits_count(view.index + height); // how many cells the most big num will take
        int n_width = width - dc - 1;
        if(n_width < 1) {
            dc = 0;
            flag_off(w->flags, WINDOW_LINES);
        } else {
            width = n_width;
            pos.x1 += dc + 1;
        }
    }

    logg("Drawing window: %d %d %d %d - %d %d\n",
    pos.y1, pos.x1, pos.y2, pos.x2, height, width);

    ncplane_set_bg_rgb8(S.p, cl.gen.bg.r, cl.gen.bg.g, cl.gen.bg.b);
    ncplane_set_fg_rgb8(S.p, cl.gen.fg.r, cl.gen.fg.g, cl.gen.fg.b);

    // TODO: draw unfocused windows differently
    // char is_focused = S.current_window == w;

    if(!w->buff) return 0;

    // if prompt window, write the prompt string at the beginning and offset the cursor pos
    if(is_prompt) {
        if(w->buff->path) {
            int path_len = wcslen(w->buff->path);
            int nw = width - path_len;

            if(nw > 0) {
                ncplane_putwstr_yx(S.p, pos.y1, pos.x1, w->buff->path);
                pos.x1 += path_len;
                width = nw;
            }
        }
    }

    int y = pos.y1;
    for(; y <= pos.y2; y++) {
        draw_window_line(p, w, view, cur, y, pos.x1, width, dc, is_mark);
        if(!view.l->next) break;
        view.l = view.l->next;
    }
    y++;

    for(; y <= pos.y2; y++) { // lets finish the empty space
        draw_window_line(p, w, 
        (offset) { .index = 0, .pos = 0, .l = NULL }, 
        cur, y, pos.x1, width, dc, is_mark);
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

    logg("Drawing status at %d %d\n", y, x);

    ncplane_set_fg_rgb8(S.p, S.colors_status.fg.r, S.colors_status.fg.g, S.colors_status.fg.b);
    ncplane_set_bg_rgb8(S.p, S.colors_status.bg.r, S.colors_status.bg.g, S.colors_status.bg.b);

    logg("%d %d %d\n", S.colors_status.fg.r, S.colors_status.fg.g, S.colors_status.fg.b);

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
    L"unn <[%d]%ls> %s [%s] %ls",
    // buffer index
    ((S.current_window) ?
        (((S.current_window->buff) ?
            S.current_window->buff->index : 
            -1)) : 
        -1),
    // buffer name
    ((S.current_window) ?
        (((S.current_window->buff) ?
            S.current_window->buff->name : 
            L"*NO BUFFER*")) : 
        L"*NO WINDOW*"),
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

    return 0;
}

#endif