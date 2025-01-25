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

#ifndef __UNN_BUFFER_H_
#define __UNN_BUFFER_H_

#include <stdlib.h>

#include <pthread.h>

#include "bind.h"
#include "list.h"
#include "misc.h"

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

#define BUFFER_LIST(buff) ((list *)(&((buff)->lines_count)))

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


line *line_empty(int cap) {
    line *l = (line *)calloc(1, sizeof(*l));

    if(!l) return NULL;

    wchar_t *str = (wchar_t *)malloc((sizeof(*str) + 1) * cap);

    if(!str) {
        free(l);
        return NULL;
    }

    str[0] = 0;
    l->cap = cap;
    l->str = str;

    return l;
}

void line_free(line *l) {
    if(!l) return;

    if(l->str)
        free(l->str);

    free(l);
}

buffer *buffer_from_lines(const wchar_t *name, line *first, line *last, int line_count) {
    buffer *b = (buffer *)calloc(1, sizeof(*b));

    if(!b) {
        return NULL;
    }

    int name_len = wcslen(name);

    wchar_t *name_copy = (wchar_t *)malloc((name_len + 1) * sizeof(*name_copy));

    if(!name_copy) {
        free(b);
        return NULL;
    }

    wcsncpy(name_copy, name, name_len);

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
    if(!l) return NULL;

    return buffer_from_lines(name, l, l, 1);
}

void buffer_free(buffer *b) {
    if(!b) return;

    node_free_nexts((node *)b->first, (free_func)line_free);

    if(b->path)
        free(b->path);

    if(b->name)
        free(b->name);
    
    pthread_mutex_destroy(&b->block);

    if(b->edit_binds)
        binds_free(b->edit_binds);
    
    if(b->move_binds)
        binds_free(b->move_binds);

    free(b);
}

int blist_insert(buffer_list *blist, buffer *b) {
    if(!blist) return -1;
    if(!b) return -1;

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
    if(!blist) return -1;
    if(!b) return -1;

    int_node *ni = (int_node *)calloc(1, sizeof(*ni));

    if(!ni) return -2;

    int i = b->index;

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
    if(!b) return;
    
    node_free_nexts((node *)b->first, (free_func)buffer_free);
    node_free_nexts((node *)b->indexes.first, free);
    free(b);
}

#endif