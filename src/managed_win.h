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

#ifndef _MANAGED_WIN_H
#define	_MANAGED_WIN_H

#include <xcb/render.h>
#include <xcb/damage.h>

typedef struct fwm_mwin {
    xcb_window_t win;
    int name_len;
    char *name;
    xcb_pixmap_t icon_pixmap;
    xcb_rectangle_t rect;
    xcb_atom_t type;
    uint8_t useful;
    uint32_t state;
    uint16_t border_width;
    /* Stack part. */
    struct fwm_mwin *next;
    struct fwm_mwin *prev;
    /* Plugins data. */
    /* Composite plugin. */
    xcb_xfixes_region_t region;
    /* Damage plugin. */
    xcb_damage_damage_t damage;
    /* Opacity plugin. */
    uint32_t opacity;
    /* Render plugin. */
    xcb_render_picture_t pic;
} fwm_mwin_t;

#endif
