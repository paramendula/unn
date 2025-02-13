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

// header for text styling, TODO

// overall unn theming. 
// theme overriding by separate windows/buffers
// params
// bg color RGB
// fg color RGB

typedef struct rgb {
    unsigned char r, g, b;
} rgb;

#define RGB(_rr, _gg, _bb) ((rgb) { .r = _rr, .g = _gg, .b = _bb })
#define RGB_INVERSE(_rgb) ((rgb) { .r = ~((_rgb).r), ~((_rgb).g), ~((_rgb).b)})

typedef struct rgb_pair {
    rgb fg, bg;
} rgb_pair;

#define RGB_PAIR(_r, _g, _b, _r1, _g1, _b1) ((rgb_pair) { .fg = RGB(_r, _g, _b), .bg = RGB(_r1, _g1, _b1) })
#define RGB_PAIR_INVERSE(_rgbp) ((rgb_pair) { .fg = RGB_INVERSE((_rgbp).fg), .bg = RGB_INVERSE((_rgbp).bg)})

typedef struct colors {
    rgb_pair gen; // general text
    rgb_pair cur; // cursor/selection
    rgb_pair cur_line; // line with a cursor
} colors;

typedef struct win_colors {
    colors focused;
    colors unfocused;
} win_colors;

#endif