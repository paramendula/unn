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

#define RGB(rr, gg, bb) ((rgb) { .r = rr, .g = gg, .b = bb })

typedef struct rgb_pair {
    rgb fg, bg;
} rgb_pair;

#define RGB_PAIR(r, g, b, r1, g1, b1) ((rgb_pair) { .fg = RGB(r, g, b), .bg = RGB(r1, g1, b1) })

typedef struct colors {
    rgb_pair gen;
    rgb_pair cur;
} colors;

#endif