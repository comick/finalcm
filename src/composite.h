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

#ifndef _COMPOSITE_H
#define	_COMPOSITE_H

#include <xcb/xcb.h>

#include "finalwm.h"

void fwm_composite_init();
void fwm_composite_init_managed_win(fwm_mwin_t *mwin);
void fwm_composite_finalize();

xcb_window_t fwm_composite_get_effects_window();
void fwm_composite_release_effects_window();

const xcb_render_picture_t fwm_composite_get_root_tile_pic();
const xcb_render_picture_t fwm_composite_get_root_buffer_pic();
const xcb_pixmap_t fwm_composite_get_root_buffer_pixmap();
#endif
