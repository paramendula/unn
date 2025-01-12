#ifndef __UNN_LIST_H_
#define __UNN_LIST_H_

#include <stdlib.h>

typedef struct node {
    struct node *prev, *next;
} node;

typedef struct list {
    int count;
    node *first, *last;
} list;

typedef void(*free_func)(void*);

void node_free_nexts(node *n, free_func f) {
    node *tmp;

    for(; n != NULL; n = tmp) {
        tmp = n->next;
        f(n);
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