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

#ifndef __UNN_LIST_H_
#define __UNN_LIST_H_

#include <stdlib.h>

#include "misc.h"

typedef struct node {
    struct node *prev, *next;
} node;

typedef struct list {
    int count;
    node *first, *last;
} list;

void node_free_nexts(node *n, free_func f) {
    node *tmp;

    while(1) {
        if(!n) break;

        tmp = n->next;
        
        f(n);

        if(tmp) n = tmp;
        else break;
    }
}

int list_append(list *l, node *n) {
    if(l->last) {
        l->last->next = n;
        n->prev = l->last;
        l->last = n;
    } else {
        l->last = l->first = n;
    }

    l->count++;

    return 0;
}

int list_prepend(list *l, node *n) {
    if(l->first) {
        l->first->prev = n;
        n->next = l->first;
        l->first = n;
    } else {
        l->first = l->last = n;
    }

    l->count++;

    return 0;
}

int list_remove(list *l, node *n) {
    if (n->prev) {
        n->prev->next = n->next;
    } else {
        l->first = n->next;
    }

    if(n->next) {
        n->next->prev = n->prev;
    } else {
        l->last = n->prev;
    }

    l->count--;

    return 0;
}

int list_insert_before(list *l, node *bef, node *n) {
    if(bef->prev == NULL) {
        l->first = n;
    } else {
        bef->prev->next = n;
        n->prev = bef->prev;
    }

    bef->prev = n;
    n->next = bef;

    l->count++;

    return 0;
}

int list_insert_after(list *l, node *aft, node *n) {
    if(aft->next == NULL) {
        l->last = n;
    } else {
        aft->next->prev = n;
        n->next = aft->next;
    }

    aft->next = n;
    n->prev = aft;

    l->count++;

    return 0;
}

#endif