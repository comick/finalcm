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
#include <string.h>

#include <xcb/composite.h>
#include <xcb/xcb_renderutil.h>

#include "render.h"

#define _FILTER "fast"

static xcb_render_query_pict_formats_reply_t *_pict_formats = NULL;

const xcb_render_query_pict_formats_reply_t *fwm_render_get_pict_formats() {
    if (_pict_formats == NULL) {
        _pict_formats = (xcb_render_query_pict_formats_reply_t *) xcb_render_util_query_formats(gd.conn);
    }
    return _pict_formats;
}

xcb_render_pictforminfo_t *fwm_render_get_pictforminfo(xcb_window_t window) {
    xcb_render_pictforminfo_t template;
    xcb_get_window_attributes_reply_t *attr = xcb_get_window_attributes_reply(
            gd.conn,
            xcb_get_window_attributes(gd.conn, window),
            NULL);
    const xcb_render_query_pict_formats_reply_t *pict_formats = fwm_render_get_pict_formats();
    template.id = xcb_render_util_find_visual_format(pict_formats, attr->visual)->format;
    return xcb_render_util_find_format(pict_formats, XCB_PICT_FORMAT_ID, &template, 0);
}

xcb_render_picture_t fwm_render_create_picture(xcb_drawable_t drw, uint32_t value_mask, const uint32_t *value_list) {
    const xcb_render_query_pict_formats_reply_t *pict_formats;
    xcb_render_pictvisual_t *pictvisual;
    xcb_render_picture_t pic = xcb_generate_id(gd.conn);
    pict_formats = fwm_render_get_pict_formats();
    pictvisual = xcb_render_util_find_visual_format(
            pict_formats, gd.def_screen->root_visual);
    xcb_render_create_picture(gd.conn, pic, drw, pictvisual->format, value_mask, value_list);
    return pic;
}

/* Draw a single window thumb withing thecified rectangle inside the dest picture. */
void fwm_render_draw_thumb(fwm_mwin_t *mwin, xcb_render_picture_t dest, xcb_rectangle_t *out_rect, fwm_render_mask_t *rmask) {
    float scale_x, scale_y, scale = 1.0f;
    int16_t translate_x = 0, translate_y = 0;
    xcb_render_transform_t transform;
    xcb_render_picture_t thumb_pic = xcb_generate_id(gd.conn);
    xcb_render_pictforminfo_t *pictforminfo = fwm_render_get_pictforminfo(mwin->win);
    uint32_t mask = XCB_RENDER_CP_SUBWINDOW_MODE;
    uint32_t values[] = {1};
    xcb_pixmap_t window_pixmap = xcb_generate_id(gd.conn);

    xcb_composite_name_window_pixmap(gd.conn, mwin->win, window_pixmap);
    xcb_render_create_picture(gd.conn,
            thumb_pic,
            window_pixmap,
            pictforminfo->id,
            mask,
            values
            );

    if (out_rect != NULL) {
        scale_x = (float) (out_rect->width) / (float) mwin->rect.width;
        scale_y = (float) (out_rect->height) / (float) mwin->rect.height;
        scale = (scale_x < scale_y) ? scale_x : scale_y;
        if (scale_x > scale_y) {
            translate_x = (float) (out_rect->width - (mwin->rect.width * scale)) / 2.0f;
        }
        if (scale_y > scale_x) {
            translate_y = (float) (out_rect->height - (mwin->rect.height * scale)) / 2.0f;
        }
        if (scale != 1.0) {
            transform.matrix11 = d2f(1.0f);
            transform.matrix12 = 0;
            transform.matrix13 = 0;
            transform.matrix21 = 0;
            transform.matrix22 = d2f(1.0f);
            transform.matrix23 = 0;
            transform.matrix31 = 0;
            transform.matrix32 = 0;
            transform.matrix33 = d2f(scale);
            xcb_render_set_picture_transform(gd.conn, thumb_pic, transform);
        }
    }

    xcb_render_set_picture_filter(gd.conn, thumb_pic, strlen(_FILTER), _FILTER, 0, NULL);
    
    xcb_render_composite(gd.conn,
            XCB_RENDER_PICT_OP_SRC,
            thumb_pic, 0, dest,
            0, 0,
            0, 0,
            out_rect->x + translate_x, out_rect->y + translate_y,
            mwin->rect.width*scale, mwin->rect.height * scale
            );
    if (rmask != NULL && rmask->type == FWM_RENDER_MASK_COLOR) {
        xcb_rectangle_t resized_r;
        resized_r.x = out_rect->x + translate_x;
        resized_r.y = out_rect->y + translate_y;
        resized_r.width = mwin->rect.width*scale;
        resized_r.height = mwin->rect.height * scale;
        xcb_render_fill_rectangles(gd.conn, XCB_RENDER_PICT_OP_OVER,
                dest, rmask->value.color, 1, &resized_r);
    }

    xcb_render_free_picture(gd.conn, thumb_pic);
    xcb_free_pixmap(gd.conn, window_pixmap);
}

int fwm_render_init() {
    const xcb_query_extension_reply_t *query_ext_reply;
    query_ext_reply = xcb_get_extension_data(gd.conn, &xcb_render_id);
    if (!query_ext_reply->present) {
        FWM_ERR("render extension not found");
        return -1;
    }
    return 0;
}
