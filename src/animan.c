/* FinalCM - a composite manager for X written using xcb
 * Copyright (C) 2010  Michele Comignano
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>

#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

#include "finalwm.h"
#include "animan.h"

/* To be really an heap with many parallel action sending plugins.
 * A list is ok by now. */
typedef struct _animan_heap_elem {
    uintmax_t target_fps;
    void (*handler) (void *);
    struct _animan_heap_elem *next;
    void *param;
} _animan_heap_elem_t;

typedef struct _animan_heap {
    _animan_heap_elem_t *first;
    _animan_heap_elem_t *last;
} _animan_heap_t;

static struct {
    _animan_heap_t queue;
    uint8_t must_shutdown;
    pthread_t thread;
    pthread_mutex_t queue_mutex;
    uint8_t fps;
    uint32_t frame_msecs;
    uintmax_t fps_counter;
    pthread_cond_t run_cond;
} _this;

/* Append an action to the queue. */
void fwm_animan_append_action(void (*handler) (void *), void *param, uint8_t fps_delta) {
    struct timeval tv;
    _animan_heap_elem_t *elem = malloc(sizeof (_animan_heap_elem_t));
    elem->handler = handler;
    elem->param = param;
    elem->next = NULL;

    pthread_mutex_lock(&(_this.queue_mutex));
    gettimeofday(&tv, NULL);
    elem->target_fps = _this.fps_counter + fps_delta;
    if (_this.queue.first == NULL) {
        _this.queue.first = _this.queue.last = elem;
        pthread_cond_signal(&(_this.run_cond)); /* The loop thread unlock the mutex. */
    } else {
        _this.queue.last->next = elem;
        _this.queue.last = elem;
    }
    pthread_mutex_unlock(&(_this.queue_mutex));
}

/* The main loop executing actions in the queue if any. */
static void *_loop(void *__) {
    struct timeval start_tv, end_tv;
    int32_t sleep_msecs;
    while (!_this.must_shutdown) {
        pthread_mutex_lock(&(_this.queue_mutex));
        if (_this.queue.first == NULL) {
            _this.fps_counter = 0; /* Trying to avoid overflow on long execution. */
            pthread_cond_wait(&(_this.run_cond), &(_this.queue_mutex));
        }
        gettimeofday(&start_tv, NULL);
        while (_this.queue.first != NULL && _this.queue.first->target_fps <= _this.fps_counter) {
            _this.queue.first->handler(_this.queue.first->param);
            _this.queue.first = _this.queue.first->next;
        }
        pthread_mutex_unlock(&(_this.queue_mutex));
        gettimeofday(&end_tv, NULL);
        sleep_msecs = _this.frame_msecs - (
                (end_tv.tv_sec - start_tv.tv_sec) * 1000 +
                (end_tv.tv_usec - start_tv.tv_usec) / 1000
                );
        if (sleep_msecs > 0) {
            usleep((__useconds_t) (sleep_msecs * 1000));
        } else {
            /*      FWM_MSG("animan framerate or load per frame too high"); */
        }
        _this.fps_counter++;
    }
    pthread_exit(0);
}

/* Init resources and prepare for execution. */
int fwm_animan_init(uint8_t fps) {
    if (1000 % fps) {
        FWM_MSG("1000 is not a multiple of fps, this may result in timing errors");
    }
    _this.frame_msecs = 1000 / fps;
    _this.queue.first = NULL;
    _this.queue.last = NULL;
    _this.fps = fps;
    _this.fps_counter = 0;
    _this.must_shutdown = 0;
    pthread_mutex_init(&(_this.queue_mutex), NULL);
    pthread_cond_init(&(_this.run_cond), NULL);
    pthread_create(&(_this.thread), NULL, _loop, NULL);
    FWM_MSG("animan plugin successful initializated");
    return 1;
}

/* Wait thread termination and frees resources. */
int fwm_animan_shutdown() {
    _this.must_shutdown = 1;
    pthread_join(_this.thread, NULL);
    pthread_mutex_destroy(&(_this.queue_mutex));
    pthread_cond_destroy(&(_this.run_cond));
    FWM_MSG("animan plugin shutdown completed");
    return 1;
}
