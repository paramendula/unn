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

#ifndef __UNN_WSTR_H_
#define __UNN_WSTR_H_

#include <stdlib.h>
#include <wchar.h>
#include <string.h>

// wcs is never NULL
typedef struct wstr {
    int len, cap;
    wchar_t *wcs;
} wstr;

void wstr_free(wstr *w) {
    if(!w) return;
    free(w->wcs);
    free(w);
}

wstr *wstr_new(int cap) {
    if(cap <= 0) return NULL;

    wstr *w = (wstr *)malloc(sizeof(*w));

    if(!w) return NULL;

    wchar_t *wcs = (wchar_t *)malloc(sizeof(*wcs) * (cap + 1));

    if(!wcs) {
        free(w);
        return NULL;
    }

    wcs[0] = 0;

    *w = (wstr) {
        .len = 0,
        .cap = cap,
        .wcs = wcs,
    };

    return w;
}

// fit w->wcs for inserting to_add new wchars
// doesn't check if w is NULL
// doesn't change w->len, only w->cap
// returns -1 if not enough memory
inline static int _wstr_check(wstr *w, int to_add) {
    int new_len = w->len + to_add;
    int new_cap = w->cap;

    if(new_len > new_cap) {
        while(new_len > new_cap) {
            new_cap *= 2;
        }

        wchar_t *new_wcs = (wchar_t *)realloc(w->wcs, sizeof(*new_wcs) * (new_cap + 1));

        if(!new_wcs)
            return -1;

        w->cap = new_cap;
        w->wcs = new_wcs;
    }

    return 0;
}

// idx == w->len if append
int wstr_insert(wstr *w, int idx, wchar_t ch) {
    if(!w) return -1;
    if(idx > w->len) return -3;
    if(_wstr_check(w, 1)) return -2;

    wchar_t *wcs = w->wcs;

    int to_shift = w->len - idx;

    if(to_shift)
        memmove(wcs + idx + 1, wcs + idx, to_shift * sizeof(*wcs));

    wcs[idx] = ch;
    wcs[++w->len] = 0;

    return 0;
}

int wstr_insert_multi(wstr *w, int idx, wchar_t *buff, int buff_len) {
    if(!w) return -1;
    if(idx > w->len) return -3;
    if(buff_len < 0) return -4;
    if(buff_len == 0) return 0;
    if(!buff) return -1;
    if(_wstr_check(w, buff_len)) return -2;

    wchar_t *wcs = w->wcs;

    int to_shift = w->len - idx;

    if(to_shift)
        memmove(wcs + idx + buff_len, wcs + idx, to_shift * sizeof(*wcs));

    memcpy(wcs + idx, buff, sizeof(*wcs) * buff_len);

    w->len += buff_len;

    wcs[w->len] = 0;

    return 0;
}

inline static int wstr_insert_wcs(wstr *w, int idx, wchar_t *wcs) {
    if(!wcs) return -1;

    return wstr_insert_multi(w, idx, wcs, wcslen(wcs));
}

inline static int wstr_insert_wstr(wstr *w, int idx, wstr *w2) {
    if(!w2) return -1;

    return wstr_insert_multi(w, idx, w2->wcs, w2->len);
}

inline static int wstr_append(wstr *w, wchar_t ch) {
    if(!w) return -1;
    
    return wstr_insert(w, w->len, ch);
}

inline static int wstr_append_multi(wstr *w, wchar_t *buff, int buff_len) {
    if(!w) return -1;

    return wstr_insert_multi(w, w->len, buff, buff_len);
}

inline static int wstr_append_wstr(wstr *w, wstr *w2) {
    if(!w) return -1;
    if(!w2) return -1;

    return wstr_insert_multi(w, w->len, w2->wcs, w2->len);
}

int wstr_remove(wstr *w, int idx) {
    if(!w) return -1;
    if(idx >= w->len) return -3;

    wchar_t *wcs = w->wcs;

    int to_shift = w->len - idx - 1;

    if(to_shift)
        memmove(wcs + idx, wcs + idx + 1, to_shift * sizeof(*wcs));

    wcs[--w->len] = 0;

    return 0;
}

int wstr_remove_safe(wstr *w, int idx) {
    if(!w) return -1;
    if(w->len == 0) return 0;
    if(idx >= w->len) return 0;

    wchar_t *wcs = w->wcs;

    int to_shift = w->len - idx - 1;

    if(to_shift)
        memmove(wcs + idx, wcs + idx + 1, to_shift * sizeof(*wcs));

    wcs[--w->len] = 0;

    return 0;
}

int wstr_remove_multi(wstr *w, int idx, int amount) {
    if(!w) return -1;
    if(amount < 0) return -4;
    if(amount == 0) return 0;
    if(idx >= w->len) return -3;

    wchar_t *wcs = w->wcs;

    int to_shift = w->len - idx - amount;

    if(to_shift)
        memmove(wcs + idx, wcs + idx + amount, to_shift * sizeof(*wcs));

    w->len -= amount;

    wcs[w->len] = 0;

    return 0;
}

int wstr_remove_multi_safe(wstr *w, int idx, int amount) {
    if(!w) return -1;
    if(amount < 0) return -4;
    if(amount == 0) return 0;
    if(idx >= w->len) return -3;
    if(w->len == 0) return 0;

    wchar_t *wcs = w->wcs;

    int new_len = w->len - amount;
    int left_border = idx + amount;
    int to_shift = w->len - left_border;

    if(w->len <= left_border) {
        int diff = left_border - w->len;

        to_shift -= diff;
        new_len -= diff;
    }

    if(to_shift)
        memmove(wcs + idx, wcs + idx + amount, to_shift * sizeof(*wcs));

    w->len = new_len;

    wcs[w->len] = 0;

    return 0;
}

inline static int wstr_pop(wstr *w) {
    if(!w) return -1;

    return wstr_remove(w, w->len - 1);
}

inline static int wstr_pop_safe(wstr *w) {
    if(!w) return -1;

    return wstr_remove_safe(w, w->len - 1);
}

inline static int wstr_pop_multi(wstr *w, int amount) {
    if(!w) return -1;
    
    return wstr_remove_multi(w, w->len - amount, amount);
}

inline static int wstr_pop_multi_safe(wstr *w, int amount) {
    if(!w) return -1;

    int beg = w->len - amount;
    int am = amount;

    if(beg < 0) {
        amount += beg;
        beg = 0;
    }

    return wstr_remove_multi(w, beg, am);
}

#endif