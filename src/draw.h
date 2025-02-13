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

inline static void empty_at_yx(int y, int x, int amount) {
    int last_x = amount + x;

    for(; x < last_x; x++) {
        ncplane_putwc_yx(S.p, y, x, L' ');
    }
}

inline static void dchar_put_yx(dchar dch, int y, int x, rgb_pair col) {
    nccell c = { 0 };

    if(flag_is_on(dch.flags, DCHAR_COLORED)) {
        col = dch.color;
    }

    nccell_set_bg_rgb8(&c, col.bg.r, col.bg.g, col.bg.b);
    nccell_set_fg_rgb8(&c, col.fg.r, col.fg.g, col.fg.b);

    nccell_load_ucs32(S.p, &c, dch.wch);

    ncplane_putc_yx(S.p, y, x, &c);
}

inline static void dstr_put_yx(dchar *dstr, int y, int x, int amount, rgb_pair col) {
    if(!dstr) return;

    ncplane_cursor_move_yx(S.p, y, x);

    for(int i = 0; i < amount; i++) {
        dchar_put_yx(dstr[i], y, x + i, col);
    }
}

void draw_window(window *w) {
    if(!w) return;
    if(!w->buff) return;

    // buffer *b = w->buff;

    char is_focused = (S.current_window == w);
    char is_numbered = flag_is_on(S.current_window->flags, WINDOW_LINES);
    char is_marked = !!flag_is_on(S.current_window->flags, WINDOW_LONG_MARKS);

    colors cl = (is_focused) ? w->cl.focused : w->cl.unfocused;

    ncplane_set_bg_rgb8(S.p, cl.gen.bg.r, cl.gen.bg.g, cl.gen.bg.b);
    ncplane_set_fg_rgb8(S.p, cl.gen.fg.r, cl.gen.fg.g, cl.gen.fg.b);

    int height = w->pos.y2 - w->pos.y1 + 1;
    int width = w->pos.x2 - w->pos.x1 + 1;

    int dc = 0;
    int left_border = w->pos.x1;
    int right_border = w->pos.x2;

    if(is_numbered) {
        dc = digits_count(w->view.index + height + 1);
        left_border += dc + 1;
        w->dc = dc;
    } else {
        w->dc = 0;
    }

    if(is_marked) {
        right_border -= 1;
    }

    int current_line_y = w->pos.y1;
    int last_line_y = w->pos.y2;

    line *current_line = w->view.l;

    char buff[32] = { 0 };

    // draw existing lines
    for(; current_line_y <= last_line_y; current_line_y++) {
        if(!current_line) break;

        // print line numbers
        if(dc) {
            int indent = dc - snprintf(buff, sizeof(buff) - 1, "%d", w->view.index + 1 + current_line_y - w->pos.y1);
            empty_at_yx(current_line_y, w->pos.x1, indent);
            ncplane_putstr_yx(S.p, current_line_y, w->pos.x1 + indent, buff);
            ncplane_putwc_yx(S.p, current_line_y, w->pos.x1 + dc, L' ');
            buff[0] = 0;
        }

        // print decorated text
        // with account for right border

        int line_length = current_line->len - S.current_window->view.pos;
        int withhold = 0;
        int to_be_printed = 0;
        int line_right_border = left_border + line_length;

        if(line_length > 0) {
            if(line_right_border > right_border) {
                withhold = line_right_border - right_border - 1;
            }

            to_be_printed = line_length - withhold;

            dstr_put_yx(current_line->dstr + w->view.pos, current_line_y, left_border, to_be_printed,
                (current_line == w->cur.l) ? cl.cur_line : cl.gen);
        }

        if(current_line == w->cur.l) {
            if(line_right_border <= right_border) {
                ncplane_set_bg_rgb8(S.p, cl.cur_line.bg.r, cl.cur_line.bg.g, cl.cur_line.bg.b);
                ncplane_set_fg_rgb8(S.p, cl.cur_line.fg.r, cl.cur_line.fg.g, cl.cur_line.fg.b);

                empty_at_yx(current_line_y, left_border + to_be_printed, right_border - to_be_printed);

                ncplane_set_bg_rgb8(S.p, cl.gen.bg.r, cl.gen.bg.g, cl.gen.bg.b);
                ncplane_set_fg_rgb8(S.p, cl.gen.fg.r, cl.gen.fg.g, cl.gen.fg.b);
            }

            if(w->view.pos <= w->cur.pos) {
                wchar_t ch = (current_line->len) ? (
                    (w->cur.pos < current_line->len) ? current_line->dstr[w->cur.pos].wch : L' '
                ) : L' ';
                dchar_put_yx((dchar) { .wch = ch, 
                                       .flags = 0,
                 }, current_line_y, w->cur.pos + left_border - w->view.pos, cl.cur);
            }
        } else {
            if(line_right_border < right_border) {
                empty_at_yx(current_line_y, left_border + to_be_printed, right_border - to_be_printed);
            }
        }

        if(is_marked) {
            if(withhold) {
                dchar_put_yx(DCH(L'>'),
                    current_line_y, w->pos.x2, cl.cur);
            }
        }

        current_line = current_line->next;
    }

    // draw empty space
    for(; current_line_y <= last_line_y; current_line_y++) {
        empty_at_yx(current_line_y, w->pos.x1, width);

        if(dc) {
            for(int i = 0; i < dc; i++) {
                ncplane_putwc_yx(S.p, current_line_y, w->pos.x1 + i, L'-');
            }
            ncplane_putwc_yx(S.p, current_line_y, w->pos.x1 + dc, L' ');
        }
    }
}

#endif