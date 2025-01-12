#ifndef __UNN_ERR_H_
#define __UNN_ERR_H_

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "panic.h"

typedef struct err {
    int code;
    char message[512];
} err;

int err_set(err *e, int code, const char *message) {
    if(!e) return code;

    e->code = code;
    strncpy(e->message, message, sizeof(e->message) / sizeof(*e->message) - sizeof(*e->message));

    return code;
}

int err_printf(err *e, int code, const char *fmt, ...) {
    if(!e) return code;

    va_list ap; va_start(ap, fmt);

    vsnprintf(e->message, sizeof(e->message) / sizeof(*e->message) - sizeof(*e->message), fmt, ap);

    va_end(ap);

    return code;
}

void err_display(err *e) {
    if(!e) return;

    printf("ERROR[%d]: %s\n", e->code, e->message);
}

void err_fatal(err *e) {
    if(!e) return;
    if(e->code == 0) return;

    err_display(e);
    panic();
}

#endif