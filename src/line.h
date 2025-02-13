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

#ifndef __UNN_LINE_H_
#define __UNN_LINE_H_

#include <wchar.h>

#include "colors.h"

#define DCHAR_COLORED 1
#define DCHAR_BOLD 2
#define DCHAR_ITALIC 4
#define DCHAR_DIM 8

// I'm not sure if this is efficient
typedef struct dchar {
    wchar_t wch;
    int flags;
    rgb_pair color;
} dchar;

inline static dchar *dch_empty() {
    dchar *dch = (dchar *)calloc(1, sizeof(*dch));

    if(!dch) {
        return NULL;
    }

    return dch;
}

inline static dchar *dch_new(wchar_t wch, int flags, rgb_pair color) {
    dchar *dch = (dchar *)malloc(sizeof(*dch));

    if(!dch) {
        return NULL;
    }

    *dch = (dchar) {
        .wch = wch,
        .flags = flags,
        .color = color,
    };

    return dch;
}

#define DCH(_wch) ((dchar) { .wch = _wch, .flags = 0, .color = RGB_PAIR(0, 0, 0, 0, 0, 0) })

typedef struct line {
    struct line *prev, *next;

    int len, cap;
    dchar *dstr;
} line;

// cap >= 1
line *line_empty(int cap) {
    line *dl = (line *)malloc(sizeof(*dl));

    if(!dl) return NULL;

    dchar *dstr = (dchar *)malloc(sizeof(*dstr) * cap);

    if(!dstr) {
        free(dl);
        return NULL;
    }

    *dl = (line) {
        .prev = NULL,
        .next = NULL,
        .len = 0,
        .cap = cap,
        .dstr = dstr,
    };

    return dl;
}

void line_free(line *dl) {
    if(!dl) return;
    
    free(dl->dstr);
    free(dl);
}

inline static int _line_check(line *dl, int amount) {
    if((dl->len + amount) <= dl->cap) return 0;

    int new_cap = dl->cap * 2;
    while((dl->len + amount) > new_cap) new_cap *= 2;

    dchar *dstr = (dchar *)realloc(dl->dstr, new_cap * sizeof(*dstr));

    if(!dstr) return -1;

    dl->cap = new_cap;
    dl->dstr = dstr;

    return 0;
}

int line_insert(line *dl, dchar ch, int index) {
    if(!dl) return -1;
    if(_line_check(dl, 1)) return -2;

    int to_move = dl->len - index;

    if(to_move)
        memmove(dl->dstr + index + 1, dl->dstr + index, sizeof(*dl->dstr) * to_move);

    dl->dstr[index] = ch;
    dl->len++;

    return 0;
}

int line_insert_multi(line *dl, dchar *dbuff, int len, int index) {
    if(!dl) return -1;
    if(!dbuff) return -1;
    if(_line_check(dl, len)) return -2;

    int to_move = dl->len - index;

    if(to_move)
        memmove(dl->dstr + index + 1, dl->dstr + index, sizeof(*dl->dstr) * to_move);

    memcpy(dl->dstr + index, dbuff, sizeof(*dbuff) * len);

    dl->len += len;

    return 0;
}

int line_insert_wcs(line *l, wchar_t *wcs, int idx) {
    if(!l) return -1;
    if(!wcs) return -1;

    int len = l->len;
    int wcs_len = wcslen(wcs);
    int new_len = l->len + wcs_len * sizeof(dchar);

    if(_line_check(l, new_len)) return -2;

    dchar *dstr = l->dstr;

    for(int i = 0; i < wcs_len; i++) {
        // shift whose place is taken by those whose place is taken to untaken place
        dstr[len + idx] = dstr[idx + wcs_len + i];
        // shift whose place is taken
        dstr[idx + wcs_len + i] = dstr[idx + i];
        // take place
        dstr[idx + i] = DCH(wcs[i]);
    }

    l->len = new_len;

    return 0;
}

inline static int line_append(line *dl, dchar ch) {
    return line_insert(dl, ch, dl->len);
}

inline static int line_append_multi(line *dl, dchar *dchs, int len) {
    return line_insert_multi(dl, dchs, len, dl->len);
}

int line_remove(line *dl, int index, dchar *buff) {
    if(!dl) return -1;

    dchar ch = dl->dstr[index];

    int to_move = sizeof(*dl->dstr) * (dl->len - index - 1);

    if(to_move)
        memmove(dl->dstr + index, dl->dstr + index + 1, to_move);

    dl->len--;

    if(buff)
        *buff = ch;

    return 0;
}

int line_remove_multi(line *dl, int index, int amount, dchar *buff) {
    if(!dl) return -1;

    if(buff) {
        for(int i = index; i < index + amount; i++) {
            *buff = dl->dstr[i];
        }
    }

    int to_move = sizeof(*dl->dstr) * (dl->len - index - amount);

    if(to_move)
        memmove(dl->dstr + index, dl->dstr + index + amount, to_move);

    dl->len -= amount;

    return 0;
}

int line_to_wstr(line *l, wchar_t **buff) {
    if(!l) return -1;
    if(!buff) return -1;

    wchar_t *s = (wchar_t *)malloc(sizeof(*s) * (l->len + 1));

    if(!s) {
        return -2;
    }

    for(int i = 0; i < l->len; i++) {
        s[i] = l->dstr[i].wch;
    }

    s[l->len] = 0;

    *buff = s;

    return 0;
}

int line_to_str(line *l, char **buff, int *blen, int *bcap) {
    if(!l) return -1;
    if(!buff) return -1;

    int len = 0;
    int cap = 4 + l->len;

    char *str = (char *)malloc(sizeof(*str) * (cap + 1));

    if(!str) {
        return -2;
    }

    char buffer[32] = { 0 };

    for(int i = 0; i < l->len; i++) {
        int c = wctomb(buffer, l->dstr[i].wch);

        if(c < 1) {
            buffer[0] = '?';
            buffer[1] = 0;
            c = 1;
        }
        
        int new_len = len + c;

        if(new_len >= cap) {
            while(new_len >= cap) {
                cap *= 2;
            }

            char *new_str = (char *)realloc(str, sizeof(*new_str) * (cap + 1));

            if(!new_str) {
                free(str);
                return -2;
            }

            for(int i = 0; i < c; i++) {
                new_str[len + i] = buffer[i];
            }
            
            str = new_str;
        }

        len = new_len;
    }

    str[len] = 0;

    if(blen)
        *blen = len;

    if(bcap)
        *bcap = cap;

    return 0;
}

int line_to_str_cap(line *l, char *buff, int max, int *blen) {
    if(!l) return -1;
    if(!buff) return -1;

    int len = 0;

    char tmp[32] = { 0 };

    for(int i = 0; i < l->len; i++) {
        int c = wctomb(tmp, l->dstr[i].wch);

        if(c < 1) {
            tmp[0] = '?';
            tmp[1] = 0;
            c = 1;
        }

        int new_len = len + c;

        if(new_len >= max) {
            return -2;
        }

        for(int i = 0; i < c; i++) {
            buff[len + i] = tmp[i];
        }

        len = new_len;
    }

    buff[len] = 0;

    return 0;
}

#endif