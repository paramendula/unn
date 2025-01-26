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

#include "window.h"
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
inline static void draw_window_line(window *w, offset view, 
                                    offset cur, int y1, int x1, int width, int dc, char is_mark, colors cl) {
    int len = (view.l) ? (view.l->len - view.pos) : 0;

    char is_curline = view.l == cur.l;

    rgb_pair colcur = cl.cur;
    rgb_pair colcline = cl.cur_line;

    if(is_curline) {
        ncplane_set_bg_rgb8(S.p, colcline.bg.r, colcline.bg.g, colcline.bg.b);
        ncplane_set_fg_rgb8(S.p, colcline.fg.r, colcline.fg.g, colcline.fg.b);
    }

    for(int i = x1; i < x1 + width + is_mark; i++) {
        ncplane_putchar_yx(S.p, y1, i, ' ');
    }

    ncplane_cursor_move_yx(S.p, y1, x1 - ((dc) ? (dc + 1) : 0));

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
            ncplane_putchar(S.p, ' ');
        }
        ncplane_putstr(S.p, buff);
        ncplane_putchar(S.p, ' ');
    }

    // empty line
    if(len <= 0) {
        if(cur.l == view.l) { // if we have cursor at an empty line
            nccell c = { 0 };
            c.gcluster = L' ';

            nccell_set_bg_rgb8(&c, colcur.bg.r, colcur.bg.g, colcur.bg.b);
            nccell_set_fg_rgb8(&c, colcur.fg.r, colcur.fg.g, colcur.fg.b);

            ncplane_putc_yx(S.p, y1, x1, &c);
        }

        if(is_curline) { // go back
            ncplane_set_bg_rgb8(S.p, cl.gen.bg.r, cl.gen.bg.g, cl.gen.bg.b);
            ncplane_set_fg_rgb8(S.p, cl.gen.fg.r, cl.gen.fg.g, cl.gen.fg.b);
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

    ncplane_putwstr(S.p, wcstr);

    if(is_curline) { // go back
        ncplane_set_bg_rgb8(S.p, cl.gen.bg.r, cl.gen.bg.g, cl.gen.bg.b);
        ncplane_set_fg_rgb8(S.p, cl.gen.fg.r, cl.gen.fg.g, cl.gen.fg.b);
    }

    // draw cursor
    if(is_curline) {
        if(cur.pos >= view.pos) {
            nccell c = { 0 };

            // L' ' if cursor is at the end, after the last char
            wchar_t ch = (cur.pos < view.l->len) ? cur.l->str[cur.pos] : L' ';

            nccell_load_ucs32(S.p, &c, ch);

            nccell_set_bg_rgb8(&c, colcur.bg.r, colcur.bg.g, colcur.bg.b);
            nccell_set_fg_rgb8(&c, colcur.fg.r, colcur.fg.g, colcur.fg.b);

            if((cur.pos <= (view.pos + width - 1))) {
                ncplane_putc_yx(S.p, y1, x1 + cur.pos - view.pos, &c);
            } 
        }
    }

    if(not_fully) {
        wcstr[width] = temp;

        if(is_mark) {
            nccell c = { 0 };

            nccell_set_bg_rgb8(&c, colcur.bg.r, colcur.bg.g, colcur.bg.b);
            nccell_set_fg_rgb8(&c, colcur.fg.r, colcur.fg.g, colcur.fg.b);

            c.gcluster = L'>';

            ncplane_putc_yx(S.p, y1, x1 + width, &c);
        }
    }
}

void draw_window(window *w) {
    if(!w) return;

    rect pos = w->pos;
    offset cur = w->cur;
    offset view = w->view;

    char is_prompt = flag_is_on(w->buff->flags, BUFFER_PROMPT);
    char is_mark = !!flag_is_on(w->flags, WINDOW_LONG_MARKS);
    char is_focused = S.current_window == w;

    colors cl = (is_focused) ? w->cl.focused : w->cl.unfocused;

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
            w->dc = 0;
            flag_off(w->flags, WINDOW_LINES);
        } else {
            width = n_width;
            pos.x1 += dc + 1;
            w->dc = dc;
        }
    } else {
        w->dc = 0;
    }

    logg("Drawing window: %d %d %d %d - %d %d\n",
    pos.y1, pos.x1, pos.y2, pos.x2, height, width);

    ncplane_set_bg_rgb8(S.p, cl.gen.bg.r, cl.gen.bg.g, cl.gen.bg.b);
    ncplane_set_fg_rgb8(S.p, cl.gen.fg.r, cl.gen.fg.g, cl.gen.fg.b);

    if(!w->buff) return;

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
        draw_window_line(w, view, cur, y, pos.x1, width, dc, is_mark, cl);
        if(!view.l->next) break;
        view.l = view.l->next;
    }
    y++;

    for(; y <= pos.y2; y++) { // lets finish the empty space
        draw_window_line(w, 
        (offset) { .index = 0, .pos = 0, .l = NULL }, 
        cur, y, pos.x1, width, dc, is_mark, cl);
    }
    
    return;
}

int draw_grid(struct ncplane *p, grid *g, char blocking) {
    if(!g) return -1;

    for(window *w = g->first; w != NULL; w = w->next) {
        if(blocking) {
            pthread_mutex_lock(&w->buff->block);
        } else {
            if(pthread_mutex_trylock(&w->buff->block)) continue;
        }

        buffer *b = w->buff;
        if(b) {
            draw_func draw = b->draw;
            if(draw) {
                draw(w);
            }
        }

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

inline static void empty_at_yx(int y, int x, int amount, rgb_pair cl) {
    nccell c = { 0 };

    nccell_set_bg_rgb8(&c, cl.bg.r, cl.bg.g, cl.bg.b);
    nccell_set_fg_rgb8(&c, cl.fg.r, cl.fg.g, cl.fg.b);

    nccell_load_ucs32(S.p, &c, L' ');

    int last_x = amount +x;

    for(; x < last_x; x++) {
        ncplane_putc_yx(S.p, y, x, &c);
    }
}

inline static void dchar_put_yx(dchar dch, int y, int x, colors cl) {
    nccell c = { 0 };

    rgb_pair col = cl.gen;

    if(flag_is_on(dch.flags, DCHAR_COLORED)) {
        col = dch.color;
    }

    nccell_set_bg_rgb8(&c, col.bg.r, col.bg.g, col.bg.b);
    nccell_set_fg_rgb8(&c, col.fg.r, col.fg.g, col.fg.b);

    nccell_load_ucs32(S.p, &c, dch.wch);

    ncplane_putc_yx(S.p, y, x, &c);
}

inline static void dline_put_yx(dline *dl, int y, int x, colors cl) {
    if(!dl) return;

    ncplane_cursor_move_yx(S.p, y, x);

    dchar *dstr = dl->dstr;

    for(int i = 0; i < dl->len; i++) {
        dchar_put_yx(dstr[i], y, x + i, cl);
    }
}

void draw_window_colored(window *w) {
    if(!w) return;
    if(!w->buff) return;

    cbuffer *b = (cbuffer *)w->buff;

    if(flag_is_off(b->buff.flags, BUFFER_COLORED)) return; // we don't like drawing you!!

    char is_focused = (S.current_window == w);

    colors cl = (is_focused) ? w->cl.focused : w->cl.unfocused;

    // int height = w->pos.y2 - w->pos.y1 + 1;
    int width = w->pos.x2 - w->pos.x1 + 1;

    int current_line_y = w->pos.y1;
    int last_line_y = w->pos.y2;

    dline *current_line = b->first;

    // draw existing lines
    for(; current_line_y <= last_line_y; current_line_y++) {
        if(!current_line) break;

        dline_put_yx(current_line, current_line_y, w->pos.x1, cl);

        // fill the left space with colored spaces
        empty_at_yx(current_line_y, w->pos.x1 + current_line->len, width - current_line->len, cl.gen);

        if(current_line == w->cur.dl) {
            if(w->view.pos <= w->cur.pos) {
                wchar_t ch = (current_line->len) ? (
                    (w->cur.pos < current_line->len) ? current_line->dstr[w->cur.pos].wch : L' '
                ) : L' ';
                dchar_put_yx((dchar) { .wch = ch, 
                                       .flags = DCHAR_COLORED,
                                       .color = cl.cur,
                 }, current_line_y, w->cur.pos, cl);
            }
        }

        current_line = current_line->next;
    }

    // draw empty space
    for(; current_line_y <= last_line_y; current_line_y++) {
        empty_at_yx(current_line_y, w->pos.x1, width, cl.gen);
    }
}

#endif