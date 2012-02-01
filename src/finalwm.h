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

#ifndef _FINALWM_H
#define	_FINALWM_H

#include <stdio.h>

#include <xcb/render.h>
#include <xcb/xcb_property.h>

#include "wt.h"
#include "ws.h"

#define FWM_MSG(A) printf("MSG: " A ".\n")

#define max(A, B) (A) > (B) ? (A) : (B)
#define min(A, B) (A) < (B) ? (A) : (B)

typedef struct fwm_screen_data {
    xcb_screen_t *def_screen;
    xcb_connection_t *conn;
    int def_screen_index;
    fwm_wt_t *wt;
    fwm_ws_t *ws;
    uint32_t win_count;
    xcb_gcontext_t default_gc;
    xcb_event_handlers_t evenths;
    xcb_property_handlers_t prophs;
} fwm_data_t;

extern fwm_data_t gd;

#define FWM_ERR(A) fprintf(stderr, "ERR: " A ".\n"); fflush(stderr)
void fwm_check_error(xcb_void_cookie_t c);

#endif

