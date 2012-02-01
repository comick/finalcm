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

#include <stdio.h>

#include "finalwm.h"
#include "event.h"
#include "window.h"
#include "pointer.h"

static struct {
    int16_t root_x, root_y;
} _this;

/* Get the pointer position. */
void fwm_pointer_getxy(int16_t *x, int16_t *y) {
    xcb_query_pointer_reply_t *r;

    r = xcb_query_pointer_reply(gd.conn,
            xcb_query_pointer(gd.conn, gd.def_screen->root),
            NULL);
    *x = r->root_x;
    *y = r->root_y;
    /* Tiis implies grabbing pointer, but not doing so by now.
     * x = _this.root_x;
     * y = _this.root_y;
     */
}

/* Keep track of the pointer position to be accessibile event in non-motion related handlers. */
static int _handle_motion_notify(xcb_motion_notify_event_t *e) {
    _this.root_x = e->root_x;
    _this.root_y = e->root_y;
    return 1;
}

void fwm_pointer_init() {
    fwm_event_push_motion_notify_handler(_handle_motion_notify, FWM_EVENT_HANDLER_PRIORITY_BEFORE);
    fwm_event_set_root_mask(XCB_EVENT_MASK_BUTTON_MOTION);

    /*
        xcb_grab_pointer(gd.conn, 0, gd.def_screen->root, XCB_EVENT_MASK_POINTER_MOTION,
                XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_CURRENT_TIME);
     */

    xcb_flush(gd.conn);
}
