#include <stdio.h>
#include "../src/lisp-mode.h"

void print_space(int space) {
    for(int i = 0; i < space; i++) {
        fputc(' ', stdout);
    }
}

datum *parents[32] = { 0 };
int depth = 0;

void print_datum(datum *d, int space) {
    if(!d) return;

    // print_space(space);
    // printf("[%x %d]\n", d, d->t);

    print_space(space);

    switch(d->t) {
        case tByteVector:
        case tVector:
        case tList:
            fputs("list (", stdout);

            if(d->list) {
                printf("\n");

                // check if circ ref

                for(int i = 0; i < depth; i++) {
                    if(parents[i] == d) {
                        printf("CIRCULAR REFERENCE, BUG\n");
                        return;
                    }
                }

                parents[depth++] = d;

                for(datum *dt = d->list->first; dt != NULL; dt = dt->next) {
                    print_datum(dt, space + 2);
                }

                depth--;
            } else {
                printf("empty");
            }

            print_space(space); fputs(")\n", stdout);

            break;
        
        case tComBlock:
        case tChar:
        case tComDatum:
        case tComSingle:
        case tDirective:
        case tString:
        case tNumber:
        case tIdent:
        case tLabel:
        case tLabelRef:
            printf("[%d] ^%ls^\n", d->t, (d->str) ? d->str : L"[EMPTY]");
    }
}

int main(int argc, char **argv) {
    if(argc != 2) {
        printf("./a.out file.scm\n");
        return 0;
    }

    frdata d = { 0 };

    FILE *fp = fopen(argv[1], "rb");

    if(!fp) {
        return -1;
    }

    d.fp = fp;

    lparse *lp = (lparse *)calloc(1, sizeof(*lp));

    if(!lp) {
        fclose(fp);
        return -2;
    }

    err e = { 0 };

    lp_parse(lp, (read_func)read_func_file, &d, &e);

    fclose(fp);

    printf("[%d]: %ls\n", e.code, (e.code) ? e.message : L"Success");

    if(!e.code) {
        print_datum(lp->first, 0);

        for(perr_node *perr = lp->pe.first; perr != NULL; perr = perr->next) {
            printf("Error: (line %d; char %d) [%d] %ls\n", perr->line, perr->symbol, perr->code, perr->message);
        }

        lp_deinit(lp);
    }

    free(lp);

    return 0;
}