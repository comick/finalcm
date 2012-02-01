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

#include <string.h>

#include "finalwm.h"
#include "xfixes.h"

void fwm_xfixes_init() {
    xcb_xfixes_query_version_reply_t *reply2;
    xcb_query_extension_reply_t *reply1;

    reply1 = xcb_query_extension_reply(gd.conn,
            xcb_query_extension(gd.conn, strlen("XFIXES"), "XFIXES"),
            NULL);
    if (!reply1 || !reply1->present) {
        FWM_ERR("xfixes extension not found");
    }

    reply2 = xcb_xfixes_query_version_reply(gd.conn,
            xcb_xfixes_query_version(gd.conn, XCB_XFIXES_MAJOR_VERSION, XCB_XFIXES_MINOR_VERSION),
            NULL);
    if (!reply2 || !reply2->response_type) {
        FWM_ERR("xfixes extension problems");
    }

}

/* Returns a new region containing the root rectangle. */
xcb_xfixes_region_t fwm_xfixes_copy_root_region() {
    xcb_rectangle_t root_rect;
    xcb_xfixes_region_t r = xcb_generate_id(gd.conn);
    root_rect.x = root_rect.y = 0;
    root_rect.width = gd.def_screen->width_in_pixels;
    root_rect.height = gd.def_screen->height_in_pixels;
    xcb_xfixes_create_region(gd.conn, r, 1, &root_rect);
    return r;
}

/* Returns an empty region. */
xcb_xfixes_region_t fwm_xfixes_copy_empty_region() {
    xcb_xfixes_region_t r = xcb_generate_id(gd.conn);
    xcb_xfixes_create_region(gd.conn, r, 0, NULL);
    return r;
}