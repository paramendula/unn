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

#include "mem.h"
#include "flags.h"
#include "list.h"
#include "bind.h"

#define BUFFER_PROMPT 1

#define WINDOW_LINES 1

typedef void(*callback)(void*);

typedef struct line {
    struct line *prev, *next;

    int len, cap;
    wchar_t *str;
} line;

typedef struct buffer {
    struct buffer *prev, *next;

    int lines_count;
    line *first, *last;

    wchar_t *path, *name;
    int index;
    int flags;

    pthread_mutex_t block;

    callback on_destroy;

    binds *move_binds;
    binds *edit_binds;

    void *current_window;

    void *userdata;
} buffer;

typedef struct int_node {
    struct int_node *prev, *next;
    int val;
} int_node;

typedef struct int_list {
    int count;
    int_node *first, *last;
} int_list;

typedef struct buffer_list {
    int buffers_count;
    buffer *first, *last;

    int_list indexes;
    int last_index;
} buffer_list;

typedef struct rect {
    int y1, x1;
    int y2, x2;
} rect;

typedef struct offset {
    int index;
    int pos;
    line *l;
} offset;

typedef struct window {
    struct window *prev, *next;

    rect loc, pos; // grid, plane
    offset view, cur;
    buffer *buff;

    int flags;

    int last_pos; // for cursor vertical movement features

    callback on_destroy;
} window;

typedef struct grid {
    int windows_count;
    window *first, *last;

    int height, width;
} grid;

line *line_empty(int cap) {
    line *l = (line *)unn_calloc(1, sizeof(*l));

    wchar_t *str = (wchar_t *)unn_malloc((sizeof(*str) + 1) * cap);

    str[0] = 0;
    l->cap = cap;
    l->str = str;

    return l;
}

void line_free(line *l) {
    if(!l) return;

    free(l->str);
    free(l);
}

inline static list *buffer_as_list(buffer *b) {
    return (list *)&(b->lines_count);
}


buffer *buffer_from_lines(const wchar_t *name, line *first, line *last, int line_count) {
    buffer *b = (buffer *)unn_calloc(1, sizeof(*b));

    wchar_t *name_copy = (wchar_t *)unn_malloc((wcslen(name) + 1) * sizeof(*name_copy));
    wcscpy(name_copy, name);

    b->first = first;
    b->last = last;
    b->lines_count = line_count;
    b->name = name_copy;
    b->on_destroy = NULL;
    b->current_window = NULL;

    b->edit_binds = NULL;
    b->move_binds = NULL;
    
    pthread_mutex_init(&b->block, NULL);

    return b;
}

inline static buffer *buffer_empty(const wchar_t *name) {
    line *l = line_empty(4);
    return buffer_from_lines(name, l, l, 1);
}

void buffer_free(buffer *b) {
    if(!b) return;

    node_free_nexts((node *)b->first, (free_func)line_free);
    free(b->path);
    free(b->name);
    pthread_mutex_destroy(&b->block);

    binds_free(b->edit_binds);
    binds_free(b->move_binds);

    free(b);
}

int blist_insert(buffer_list *blist, buffer *b) {
    list_append((list *)blist, (node *)b);

    if(blist->indexes.count) {
        int_node *n = blist->indexes.first;

        int i = n->val;
        b->index = i;
        
        list_remove((list *)&(blist->indexes), (node *)n);
        free(n);
    } else {
        b->index = ++blist->last_index;
    }

    return 0;
}

int blist_remove(buffer_list *blist, buffer *b) {
    int i = b->index;

    int_node *ni = (int_node *)unn_calloc(1, sizeof(*ni));
    ni->val = i;

    list_remove((list *)blist, (node *)b);

    int_node *n;
    for(n = blist->indexes.first; n != NULL && n->next != NULL; n = n->next) {
        if(n->val > i) break;
    }

    if(!n) {
        blist->indexes.first = ni;
        blist->indexes.last = ni;
        blist->indexes.count++;
        return 0;
    }

    if(n->prev == NULL && (n->val < i)) {
        list_insert_after((list *)&(blist->indexes), (node *)n, (node *)ni);
    } else
        list_insert_before((list *)&(blist->indexes), (node *)n, (node *)ni);

    return 0;
}

void blist_free(buffer_list *b) {
    node_free_nexts((node *)b->first, (free_func)buffer_free);
    node_free_nexts((node *)b->indexes.first, free);
    free(b);
}

window *window_with_buffer(buffer *buff) {
    window *win = (window *)unn_calloc(1, sizeof(*win));

    win->buff = buff;
    win->cur = (offset) {
        .index = 0,
        .pos = 0,
        .l = buff->first,
    };
    win->view = win->cur;

    return win;
}

inline static window *window_empty(wchar_t *buff_name) {
    return window_with_buffer(buffer_empty(buff_name));
}

inline static list *grid_as_list(grid *g) {
    return (list *)&(g->windows_count);
}

int grid_fit(grid *g, rect pos) {
    int pos_height = pos.y2 - pos.y1;
    int pos_width = pos.x2 - pos.x1;

    int avg_height = pos_height / g->height;
    int avg_width = pos_width / g->width;

    int h_rem = pos_height % g->height;
    int w_rem = pos_width % g->width;

    int x_last = g->height;
    int y_last = g->width;

    for(window *win = g->first; win != NULL; win = win->next) {
        rect loc = win->loc;
        win->pos = (rect) {
            .y1 = loc.y1 * avg_height,
            .x1 = loc.x1 * avg_width,
            .y2 = loc.y2 * avg_height + ((loc.y2 == y_last) ? h_rem : 0),
            .x2 = loc.x2 * avg_width + ((loc.x2 == x_last) ? w_rem : 0),
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