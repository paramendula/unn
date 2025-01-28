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

#ifndef __UNN_ERR_H_
#define __UNN_ERR_H_

#include <wchar.h>

// we can't make 'message' a pointer because we'd have to handle
// it's freeing, and we don't know if it's static or not
// And we want 'err' to be as simple as possible
typedef struct err {
    int code;
    wchar_t message[512];
} err;

inline static int err_set(err *e, int code, const wchar_t *message) {
    if(!e) return code;

    e->code = code;
    wcsncpy(e->message, message, sizeof(e->message) / sizeof(*e->message) - sizeof(*e->message));

    return code;
}

#endif