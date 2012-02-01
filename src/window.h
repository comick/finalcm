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

#ifndef _WINDOW_H
#define	_WINDOW_H

#include <xcb/xcb_atom.h>

#include "finalwm.h"

typedef struct {
    enum xcb_atom_fast_tag_t tag;

    union {
        xcb_get_window_attributes_cookie_t cookie;
        uint8_t override_redirect;
    } value;
} fwm_wattr_t;

fwm_mwin_t *fwm_window_manage(xcb_window_t window, fwm_wattr_t wattr, xcb_rectangle_t *r, uint16_t border_width);
fwm_mwin_t *fwm_window_prepare(xcb_window_t child, xcb_rectangle_t geom_r, uint16_t border_width);
void fwm_window_stack_above(fwm_mwin_t *managed_win);

/* Basic event handlers to keep up-to-date internal structures. */
int fwm_handle_configure_notify(xcb_configure_notify_event_t *e);
int fwm_handle_destroy_notify(xcb_destroy_notify_event_t *e);
int fwm_handle_create_notify(xcb_create_notify_event_t *e);
int fwm_handle_circulate_notify(xcb_circulate_notify_event_t *e);

#endif
