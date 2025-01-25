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

#ifndef __UNN_COLORS_H_
#define __UNN_COLORS_H_

#include <notcurses/notcurses.h>

#include <wchar.h>

#include "list.h"
#include "buffer.h"

// header for text styling, TODO

// overall unn theming. 
// theme overriding by separate windows/buffers
// params
// bg color RGB
// fg color RGB

typedef struct rgb {
    unsigned char r, g, b;
} rgb;

#define RGB(rr, gg, bb) ((rgb) { .r = rr, .g = gg, .b = bb })

typedef struct rgb_pair {
    rgb fg, bg;
} rgb_pair;

#define RGB_PAIR(r, g, b, r1, g1, b1) ((rgb_pair) { .fg = RGB(r, g, b), .bg = RGB(r1, g1, b1) })

typedef struct colors {
    rgb_pair gen; // general text
    rgb_pair cur; // cursor/selection
    rgb_pair cur_line; // line with a cursor
    rgb_pair gen_unfocused; // same as above, but if unfocused
    rgb_pair cur_unfocused;
    rgb_pair cur_line_unfocused;
} colors;

// I'm not sure if this is efficient
typedef struct dchar {
    wchar_t wch;
    rgb_pair color;
    // TODO: underline, bold, italic, etc.
} dchar;

typedef struct dline {
    struct dline *prev, *next;

    int len, cap;
    dchar *dstr;
} dline;

// buffer with colored, decorated text
typedef struct cbuffer {
    buffer buff;

    int dlines_count;
    dline *first, *last;
} cbuffer; 

#define CBUFFER_LIST(cbuff) (&((cbuff)->dlines_count))

#endif