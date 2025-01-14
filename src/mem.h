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