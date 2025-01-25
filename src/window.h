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

#ifndef __UNN_WINBUF_H_
#define __UNN_WINBUF_H_

#include <wchar.h>
#include <string.h>

#include <pthread.h>

#include "misc.h"
#include "flags.h"
#include "list.h"
#include "bind.h"
#include "buffer.h"
#include "colors.h"

#define BUFFER_PROMPT 1

#define WINDOW_LINES 1
#define WINDOW_LONG_MARKS 2
#define WINDOW_DEFAULT (WINDOW_LINES | WINDOW_LONG_MARKS)

typedef struct rect {
    int y1, x1;
    int y2, x2;
} rect;

typedef struct offset {
    union {
        int index, y;
    };
    union {
        int pos, x;
    };
    line *l;
} offset;

typedef struct window {
    struct window *prev, *next;

    rect loc, pos; // grid, plane
    offset view, cur;
    buffer *buff;

    colors cl;

    int flags;
    int last_pos; // for cursor vertical movement features
    int dc; // digits count for line numbers

    callback on_destroy;
} window;

typedef struct grid {
    int windows_count;
    window *first, *last;

    int height, width;
} grid;

window *window_with_buffer(buffer *buff) {
    window *win = (window *)calloc(1, sizeof(*win));

    if(!win) return NULL;

    win->buff = buff;
    win->cur = (offset) {
        .index = 0,
        .pos = 0,
        .l = buff->first,
    };
    win->view = win->cur;
    win->flags = WINDOW_DEFAULT;

    buff->current_window = win;

    return win;
}

inline static window *window_empty(wchar_t *buff_name) {
    return window_with_buffer(buffer_empty(buff_name));
}

int grid_fit(grid *g, rect pos) {
    if(!g) return -1;

    if(!g->height) return -2;
    if(!g->width) return -2;

    int pos_height = pos.y2 - pos.y1 + 1;
    int pos_width = pos.x2 - pos.x1 + 1;

    int avg_height = pos_height / g->height;
    int avg_width = pos_width / g->width;

    int h_rem = pos_height % g->height;
    int w_rem = pos_width % g->width;

    int y_last = g->height;
    int x_last = g->width;

    for(window *win = g->first; win != NULL; win = win->next) {
        rect loc = win->loc;
        win->pos = (rect) {
            .y1 = loc.y1 * avg_height,
            .x1 = loc.x1 * avg_width,
            .y2 = loc.y2 * avg_height + ((loc.y2 == y_last) ? h_rem : 0) - 1,
            .x2 = loc.x2 * avg_width + ((loc.x2 == x_last) ? w_rem : 0) - 1,
        };

    }

    return 0;
}

int grid_insert(grid *g, window *w) {
    window *bef = g->first;

    if(bef) {
        for(; bef->next != NULL; bef = bef->next) {
            if(bef->loc.y1 < w->loc.y1) continue;
            if(bef->loc.x1 <= w->loc.x1) continue;
            break;
        }

        if(bef->prev == NULL || bef->next == NULL)
            list_insert_after((list *)g, (node *)bef, (node *)w);
        else
            list_insert_before((list *)g, (node *)bef, (node *)w);
    } else {
        g->first = w;
        g->last = w;
    }

    g->windows_count++;

    int possible_height = w->loc.y2;
    int possible_width = w->loc.x2;

    if(g->height < possible_height) g->height = possible_height;
    if(g->width < possible_width) g->width = possible_width;

    return 0;
}

inline static int grid_insert_loc(grid *g, window *w, rect loc) {
    w->loc = loc;
    return grid_insert(g, w);
}

int grid_remove(grid *g, window *w) {
    if(!g) return -1;
    if(!w) return -1;

    list_remove((list *)g, (node *)w);

    int test_height = w->loc.y2;
    int test_width = w->loc.x2;

    if(g->width == test_width || g->height == test_height) {
        int max_height = 0;
        int max_width = 0;

        for(window *win = g->first; win != NULL; win = win->next) {
            int possible_height = win->loc.y2;
            int possible_width = win->loc.x2;

            if(possible_height > max_height) max_height = possible_height;
            if(possible_width > max_width) max_width = possible_width;
        }

        g->width = max_width;
        g->height = max_height;
    }

    return 0;
}

void grid_free(grid *g) {
    if(!g) return;

    node_free_nexts((node *)g->first, free);
    free(g);
}

#endif