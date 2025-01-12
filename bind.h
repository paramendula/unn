#ifndef __UNN_BIND_H_
#define __UNN_BIND_H_

#include <stdlib.h>

#include "mem.h"

// continuations are virtual #(:-3>-<--< (somewhat)

typedef void(*tusk)(); // thunk. but my sleepy head at that moment misspelled it

#define TUSK_CONT ((tusk) 1)

typedef struct ubind {
    char is_cont;
    const char *seq;
    union {
        tusk cb;
        struct ubind *cont;
    };
} ubind;

typedef struct bucket {
    char bind[16]; // that's enough for now... :()
    tusk cb;
    struct bucket *next;
} bucket;

typedef struct binds {
    int flags;
    size_t len, cap;
    bucket **bucks;
} binds;

size_t bad_hash(const char *str) {
    if(!str) return 0;

    size_t hash = 0;

    for(int i = 0; ; i++) {
        char ch = str[i];
        if(ch == 0) break;
        hash += ch;
    }

    return hash * 31;
}

void binds_free(binds *b) {
    for(size_t i = 0; i < b->len; i++) {
        free(b->bucks[i]);
    }
    free(b->bucks);
    free(b);
}

binds *binds_empty() {
    binds *b = (binds *)unn_malloc(sizeof(*b));
    bucket **bucks = (bucket **)unn_calloc(256, sizeof(*bucks));

    b->flags = 0;
    b->len = 0;
    b->cap = 256;
    b->bucks = bucks;

    return b;
}

inline static void _binds_add(binds *b, size_t hash, size_t index, char *seq, tusk cb) {
    bucket **buck_ref = b->bucks + index;

    bucket *buck = (bucket *)malloc(sizeof(*buck));

    logg("%s %zu\n", seq, cb);

    memcpy(buck->bind, seq, 16);
    buck->cb = cb;

    if(!*buck_ref) {
        *buck_ref = buck;
        buck->next = NULL;
    } else {
        bucket *bk = *buck_ref;
        while(1) {
            if(!strcmp(seq, bk->bind)) {
                free(buck);
                bk->cb = cb;
                return;
            }

            if(bk->next == NULL) {
                bk->next = buck;
                break;
            }

            bk = bk->next;
        }
    }

    b->len++;
}

int binds_set(binds *b, const char *prefix, ubind *u) {
    char seq[16] = { 0 };
    if(prefix) strcpy(seq, prefix);
    strcat(seq, u->seq);

    logg("SET %s\n", seq);

    size_t hash = bad_hash(seq);
    size_t index = hash % b->cap;

    int useq_len = strlen(u->seq);
    int prefix_len = (prefix) ? strlen(prefix) : 0;
    int seq_len = useq_len + prefix_len;

    for(int i = prefix_len;;) {
        if(i == seq_len) {
            if(!u->is_cont) {
                _binds_add(b, hash, index, seq, u->cb);
            }
            break;
        }

        if(seq[i++] == '(') {
            char ch1 = seq[i];
            char ch2 = seq[i + 1];
            if(ch1 > ch2) {
                seq[i] = ch2;
                seq[i + 1] = ch1;
            }
            i += 3;
        }

        if((u->is_cont) || (i != seq_len)) {
            char tmp = seq[i];

            seq[i] = 0;

            size_t h = bad_hash(seq);
            size_t idx = h % b->cap;

            _binds_add(b, h, idx, seq, TUSK_CONT);

            seq[i] = tmp;
        }
    }

    if(u->is_cont) {
        for(ubind *ub = u->cont; ub->seq != NULL; ub++) {
            binds_set(b, seq, ub);
        }
    }

    return 0;
}

tusk binds_get(binds *b, char *seq) {
    size_t hash = bad_hash(seq);
    size_t index = hash % b->cap;

    bucket *buck = b->bucks[index];

    if (buck) {
        for(; buck != NULL; buck = buck->next) {
            if(!strcmp(buck->bind, seq)) {
                return buck->cb;
            }
        }
    }

    return NULL;
}

#endif