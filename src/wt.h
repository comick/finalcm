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

#ifndef _WINDOWS_TABLE_H
#define	_WINDOWS_TABLE_H

#include <stdint.h>

#include <xcb/xcb.h>

#include "managed_win.h"

#define FWM_WT_DEFAULT_SIZE 17

typedef struct fwm_node {
    struct fwm_node *next;
    xcb_window_t key;
    fwm_mwin_t *managed_win;
} fwm_node_t;

typedef struct {
    fwm_node_t *first;
} fwm_head_t;

typedef struct {
    uint32_t size;
    fwm_head_t *elements;
} fwm_wt_t;

fwm_wt_t *fwm_wt_init(uintmax_t s);
int fwm_wt_put(fwm_wt_t *wt, xcb_window_t k, fwm_mwin_t *v);
fwm_mwin_t *fwm_wt_get(fwm_wt_t *wt, xcb_window_t k);
fwm_mwin_t *fwm_wt_remove(fwm_wt_t *wt, xcb_window_t k);
void fwm_wt_free(fwm_wt_t *wt);


#endif

