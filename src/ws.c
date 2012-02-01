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
#include <stdio.h>

#include "ws.h"
#include "finalwm.h"

/* TODO creare anche la corrispondente destroy
/ FIXME forse è il caso di fare una struct contenitrice anche per i singoli elementi della coda...
 */

fwm_ws_t *fwm_ws_init() {
    fwm_ws_t *tmp = malloc(sizeof (fwm_ws_t));
    tmp->top = NULL;
    return tmp;
}

int fwm_ws_push(fwm_ws_t *ws, fwm_mwin_t *managed_win, fwm_ws_put_mode_t mode) {
    /* Update stack vision. */
    if (ws->top == NULL) {
        ws->top = managed_win->next = managed_win->prev = managed_win;
    } else {
        /* Put the node between top and its follower. */
        managed_win->prev = ws->top;
        managed_win->next = ws->top->next;
        managed_win->next->prev = managed_win;
        managed_win->prev->next = managed_win;
        switch (mode) {
            case FWM_WS_PUT_TOP:
                ws->top = managed_win;
            case FWM_WS_PUT_BOTTOM:
                break;
            default:
                return -1;
        }
    }
    return 1;
}


/* TODO forse è meglio ritornare interi.*/

fwm_mwin_t *fwm_ws_rotate(fwm_ws_t *ws, fwm_ws_rotate_mode_t mode) {
    if (ws->top != NULL) {
        switch (mode) {
            case FWM_WS_ROTATE_LEFT:
                ws->top = ws->top->next;
                break;
            case FWM_WS_ROTATE_RIGHT:
                ws->top = ws->top->prev;
                break;
            default:
                return NULL;
        }
    }
    return ws->top;
}

void fwm_ws_detach(fwm_ws_t *ws, fwm_mwin_t *managed_win) {
    if (ws->top == managed_win) {
        ws->top = (managed_win != managed_win->next) ? managed_win->next : NULL;
    }
    managed_win->prev->next = managed_win->next;
    managed_win->next->prev = managed_win->prev;
}
