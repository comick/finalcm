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

#ifndef _WS_H
#define	_WS_H

#include <xcb/xcb.h>

#include "managed_win.h"

typedef enum {
    FWM_WS_PUT_TOP,
    FWM_WS_PUT_BOTTOM
} fwm_ws_put_mode_t;

typedef enum {
    FWM_WS_ROTATE_LEFT,
    FWM_WS_ROTATE_RIGHT
} fwm_ws_rotate_mode_t;

typedef struct fwm_ws {
    fwm_mwin_t *top;
} fwm_ws_t;

fwm_ws_t *fwm_ws_init();
int fwm_ws_push(fwm_ws_t *ws, fwm_mwin_t *managed_win, fwm_ws_put_mode_t mode);
void fwm_ws_detach(fwm_ws_t *ws, fwm_mwin_t *managed_win);
fwm_mwin_t *fwm_ws_rotate(fwm_ws_t *ws, fwm_ws_rotate_mode_t mode);

#endif
