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

#ifndef __UNN_MEM_H_
#define __UNN_MEM_H_

#include <stdlib.h>

#include "panic.h"

// the single possible sane way xD joke

// I'm doing this because this UNN's C version is temporary
// and it's highly unlikely that I'll not have enough memory
// (also, not like in it's current state UNN would be able to 
//  function in a memory-low environment)

// Will be abrogated after post-MVP refactoring
// OR unneeded because UNN will speak with Lisp.

void *unn_malloc(size_t bytes) {
    void *ptr = malloc(bytes);

    if (!ptr) panic();

    return ptr;
}

void *unn_realloc(void *ptr, size_t bytes) {
    void *new_ptr = realloc(ptr, bytes);

    if (!new_ptr) panic();

    return new_ptr;
}

void *unn_calloc(size_t n, size_t bytes) {
    void *ptr = calloc(n, bytes);

    if (!ptr) panic();

    return ptr;
}

#endif