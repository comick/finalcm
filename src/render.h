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

#ifndef _RENDER_H
#define	_RENDER_H

#include <xcb/render.h>
#include <xcb/xcb_renderutil.h>

#include "finalwm.h"

#define d2f(f) ((xcb_render_fixed_t) ((f) * 65536))
#define f2d(f) (((double)(f)) / 65536)

enum fwm_render_mask_type_t {
    FWM_RENDER_MASK_COLOR,
    FWM_RENDER_MASK_PICTURE
};

typedef struct fwm_render_mask {
    enum fwm_render_mask_type_t type;

    union {
        xcb_render_color_t color;
        xcb_render_picture_t picture;
    } value;
} fwm_render_mask_t;

const xcb_render_query_pict_formats_reply_t *fwm_render_get_pict_formats();
int fwm_render_init();
xcb_render_pictforminfo_t *fwm_render_get_pictforminfo(xcb_window_t origin);
void fwm_render_draw_thumb(fwm_mwin_t *managed_win, xcb_render_picture_t dest, xcb_rectangle_t *r, fwm_render_mask_t *rmask);
xcb_render_picture_t fwm_render_create_picture(xcb_drawable_t drw, uint32_t value_mask, const uint32_t *value_list);

#endif
