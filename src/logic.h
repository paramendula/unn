#ifndef __UNN_LOGIC_H_
#define __UNN_LOGIC_H_

#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

#include "state.h"
#include "draw.h"

inline static void state_flag_on(int f) {
    pthread_mutex_lock(&S.state_flags_block);

    S.flags |= f;

    pthread_mutex_unlock(&S.state_flags_block);
}

inline static void state_flag_off(int f) {
    pthread_mutex_lock(&S.state_flags_block);

    S.flags ^= (S.flags & f);

    pthread_mutex_unlock(&S.state_flags_block);
}

inline static void state_flag_toggle(int f) {
    pthread_mutex_lock(&S.state_flags_block);

    S.flags ^= f;

    pthread_mutex_unlock(&S.state_flags_block);
}

inline static int state_flag_is_on(int f) {
    pthread_mutex_lock(&S.state_flags_block);

    int r = S.flags & f;

    pthread_mutex_unlock(&S.state_flags_block);

    return r;
}

inline static int state_flag_is_off(int f) {
    return !state_flag_is_on(f);
}

#define FLAG_DRAW_WINDOW 1
#define FLAG_DRAW_STATUS 2
#define FLAG_DRAW_GRID 4
#define FLAG_DRAW_ALL 8

inline static void order_draw(int f) {
    pthread_mutex_lock(&S.draw_flags_block);

    S.draw_flags |= f;

    int val;
    sem_getvalue(&S.draw_request, &val);

    if(!val) sem_post(&S.draw_request);

    pthread_mutex_unlock(&S.draw_flags_block);
}

inline static void order_draw_window(window *w) {
    pthread_mutex_lock(&S.draw_flags_block);

    for(int i = 0; i < S.draw_windows_count; i++) {  // bad
        if(S.draw_windows[i] == w) {
            pthread_mutex_unlock(&S.draw_flags_block);
            return;
        }
    }

    S.draw_windows[S.draw_windows_count++] = w;
    S.draw_flags |= FLAG_DRAW_WINDOW;

    pthread_mutex_unlock(&S.draw_flags_block);
}

inline static void order_draw_status() {
    return order_draw(FLAG_DRAW_STATUS);
}

inline static void order_draw_grid() {
    return order_draw(FLAG_DRAW_GRID);
}

inline static void order_draw_all() {
    return order_draw(FLAG_DRAW_ALL);
}

void *draw_loop(void*) {
    while (1) {
        if(state_flag_is_on(FLAG_EXIT)) {
            break;
        }

        sem_wait(&S.draw_request);

        pthread_mutex_lock(&S.draw_block);
        pthread_mutex_lock(&S.draw_flags_block);

        char d_a = flag_is_on(S.draw_flags, FLAG_DRAW_ALL);
        char d_g = flag_is_on(S.draw_flags, FLAG_DRAW_GRID);
        char d_w = flag_is_on(S.draw_flags, FLAG_DRAW_WINDOW);
        char d_s = flag_is_on(S.draw_flags, FLAG_DRAW_STATUS);
        S.draw_flags = 0;

        if(d_a || d_g) {
            S.draw_windows_count = 0;
        } else if(d_w) {
            S.draw_windows_count2 = S.draw_windows_count;
            S.draw_windows_count = 0;

            window **tmp = S.draw_windows;
            S.draw_windows = S.draw_windows2;
            S.draw_windows2 = tmp;
        }

        pthread_mutex_unlock(&S.draw_flags_block);

        if(d_a) {
            draw_status(S.p);
            draw_grid(S.p, S.grid, 0);

            notcurses_render(S.nc);
            pthread_mutex_unlock(&S.draw_block);
            continue;
        }    

        if(d_s) {
            draw_status(S.p);
        }

        if(d_g) {
            draw_grid(S.p, S.grid, 0);

            pthread_mutex_unlock(&S.draw_block);
            notcurses_render(S.nc);
            continue;
        }

        if(d_w) {
            for(int i = 0; i < S.draw_windows_count2; i++) {
                window *w = S.draw_windows2[i];
                if(pthread_mutex_trylock(&w->buff->block)) continue;

                draw_window(S.p, w);

                pthread_mutex_unlock(&w->buff->block);
            }

            pthread_mutex_unlock(&S.draw_block);
            notcurses_render(S.nc);
        } else if(d_s) {
            pthread_mutex_unlock(&S.draw_block);
            notcurses_render(S.nc);
        }
    }

    pthread_exit(NULL);
    return NULL;
}

void status_set_message(wchar_t *fmt, ...) {
    va_list ap; va_start(ap, fmt);

    pthread_mutex_lock(&S.status_message_block);
    vswprintf(S.status_message, sizeof(S.status_message) - 1, fmt, ap);
    pthread_mutex_unlock(&S.status_message_block);

    va_end(ap);

    order_draw_status();
}

void grid_fit_full() {
    unsigned int max_y, max_x;
    ncplane_dim_yx(S.p, &max_y, &max_x);

    if(S.prompt_window) {
        max_y -= 1;
    }

    grid_fit(S.grid, (rect) {
        .y1 = 0,
        .x1 = 0,
        .y2 = max_y - 2, // -1 for status
        .x2 = max_x - 1,
    });
}

void on_resize() {
    pthread_mutex_lock(&S.draw_block);

    grid_fit_full();

    order_draw_all();

    pthread_mutex_unlock(&S.draw_block);
}

inline static void _process_input(wchar_t wch1, wchar_t wch2) {
    char ch1 = (wch1 > 255) ? '?' : (char)wch1;
    char ch2 = (wch2 > 255) ? '?' : (char)wch2;

    if(ch2) {
        if(ch1 > ch2) {
            char tmp = ch2;
            ch2 = ch1;
            ch1 = tmp;
        }
    }

    if(ch2) {
        S.input_buffer[S.input_buffer_len++] = '(';
        S.input_buffer[S.input_buffer_len++] = ch1;
        S.input_buffer[S.input_buffer_len++] = ch2;
        S.input_buffer[S.input_buffer_len++] = ')';
    } else {
        S.input_buffer[S.input_buffer_len++] = ch1;
    }

    S.input_buffer[S.input_buffer_len] = 0;

    char is_edit = state_flag_is_on(FLAG_EDIT);

    buffer *buff = (S.current_window) ? S.current_window->buff : NULL;

    binds *binds;
    
    if(!is_edit) {
        binds = (buff) ? ((buff->move_binds) ? buff->move_binds : S.binds_move) : S.binds_move;
    } else {
        binds = (buff) ? ((buff->edit_binds) ? buff->edit_binds : S.binds_edit) : S.binds_edit; 
    }

    tusk att = binds_get(binds, S.input_buffer);

    if(!att) {
        if(!is_edit) {
            status_set_message(L"| Unknown command <%s>", S.input_buffer);
        }

        S.input_buffer[0] = 0;
        S.input_buffer_len = 0;


        return;
    }

    if(att == TUSK_CONT) {
        order_draw_status();
        return;
    }

    S.input_buffer[0] = 0;
    S.input_buffer_len = 0;

    order_draw_status();

    att();
}

void *input_loop(void*) {
    static struct timeval start, stop;
    static char flag_sim = 0;
    static wchar_t ch, last_char;

    while (1) {
        if(flag_sim) {
            ch = notcurses_get_nblock(S.nc, NULL);

            if(ch == NCKEY_RESIZE) {
                on_resize();
                continue;
            }

            gettimeofday(&stop, NULL);

            suseconds_t diff = ((stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec);

            if(diff > S.sim_cap) {
                flag_sim = 0;
                _process_input(last_char, 0);
            } else {
                if(ch) {
                    flag_sim = 0;
                    _process_input(last_char, ch);
                }
            }

            continue;
        }
        
        if(state_flag_is_on(FLAG_EXIT)) break;

        wchar_t nch = notcurses_get_blocking(S.nc, NULL);

        if(nch == NCKEY_RESIZE) {
            on_resize();
            continue;
        }

        last_char = nch;
        flag_sim = 1;
        gettimeofday(&start, NULL);
    }

    pthread_exit(NULL);
    return NULL;
}

#endif