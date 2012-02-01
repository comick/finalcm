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
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <xcb/composite.h>
#include <xcb/xcb_event.h>
#include <xcb/xfixes.h>
#include <xcb/xcb_aux.h>

#include "finalwm.h"
#include "zoom.h"
#include "composite.h"
#include "render.h"
#include "event.h"
#include "pointer.h"
#include "animan.h"
#include "damage.h"

#define _NONE 1.0f
#define _MAX_ZOOM 2.0f
#define _STEP 0.02f
#define _OUT_STEPS 5

#define _WIN 133
#define _PLUS 61
#define _MINUS 35

typedef enum {
    _IDLE,
    _READY,
    _ZOOMING
} _zoom_state;

static struct {
    float zoom_level;
    _zoom_state state;
    xcb_window_t win;
    xcb_pixmap_t win_buffer;
    uint8_t out_steps;
    xcb_render_picture_t win_buffer_pic;
    xcb_xfixes_region_t good_region;
} _this;

/*
 TODO c'è da usare lo stato zooming per evitare tanti begli errorini del protocollo
 */
static xcb_rectangle_t _full_out_rect() {
    xcb_rectangle_t r;
    int16_t mouse_x, mouse_y;

    fwm_pointer_getxy(&mouse_x, &mouse_y);
    r.x = floor((mouse_x * _this.zoom_level - mouse_x) / _this.zoom_level);
    r.y = floor((mouse_y * _this.zoom_level - mouse_y) / _this.zoom_level);
    r.height = ceil((float) gd.def_screen->height_in_pixels / _this.zoom_level);
    r.width = ceil((float) gd.def_screen->width_in_pixels / _this.zoom_level);
    return r;
}

static void _draw(xcb_rectangle_t *r, uint32_t r_len) {
    xcb_render_picture_t orig_pic = fwm_composite_get_root_buffer_pic();
    uint32_t i;
    int16_t mouse_x, mouse_y;
    fwm_pointer_getxy(&mouse_x, &mouse_y);
    xcb_render_transform_t transform;
    transform.matrix11 = d2f(1.0f);
    transform.matrix12 = 0;
    transform.matrix13 = 0;
    transform.matrix21 = 0;
    transform.matrix22 = d2f(1.0f);
    transform.matrix23 = 0;
    transform.matrix31 = 0;
    transform.matrix32 = 0;
    transform.matrix33 = d2f(_this.zoom_level);
    xcb_render_set_picture_transform(gd.conn, orig_pic, transform);
    xcb_render_set_picture_filter(gd.conn, orig_pic, strlen("fast"), "fast", 0, NULL);


    //xcb_render_set_picture_clip_rectangles(global_data.conn, _this.win_buffer_pic, 0, 0, r_len, r);



    for (i = 0; i < r_len; i++) {
        xcb_render_composite(gd.conn,
                XCB_RENDER_PICT_OP_SRC,
                orig_pic, 0, _this.win_buffer_pic,
                floor((float) r[i].x * _this.zoom_level),
                floor((float) r[i].y * _this.zoom_level),
                0, 0,
                floor((float) r[i].x * _this.zoom_level),
                floor((float) r[i].y * _this.zoom_level),
                ceil((float) r[i].width * _this.zoom_level),
                ceil((float) r[i].height * _this.zoom_level)
                );
    }
}

// TODO sta cagata va pensata per benino.

static void _swap_buffer(xcb_rectangle_t *r) {
    xcb_rectangle_t full_r;
    int16_t mouse_x, mouse_y, dest_x, dest_y, vpart_x, vpart_y;
    fwm_pointer_getxy(&mouse_x, &mouse_y);
    vpart_x = mouse_x * _this.zoom_level - mouse_x;
    vpart_y = mouse_y * _this.zoom_level - mouse_y;
    if (r == NULL) {
        /* Follow the pointer position. */
        full_r.x = vpart_x;
        full_r.y = vpart_y;
        full_r.width = gd.def_screen->width_in_pixels;
        full_r.height = gd.def_screen->height_in_pixels;
        r = &full_r;
        dest_x = dest_y = 0;
    } else {
        r->x *= _this.zoom_level;
        r->x = max(r->x, vpart_x);

        r->y *= _this.zoom_level;
        r->y = max(r->y, vpart_y);

        dest_x = r->x * _this.zoom_level;
        dest_y = r->y * _this.zoom_level;
        r->width *= _this.zoom_level;
        r->width *= _this.zoom_level;
    }
    xcb_copy_area(gd.conn, _this.win_buffer, _this.win, gd.default_gc,
            r->x, r->y,
            dest_x, dest_y,
            r->width, r->height);
}

static int _handle_damage_notify(xcb_damage_notify_event_t *e) {
    if (_this.state != _IDLE) {
        _draw(&(e->geometry), 1);
        _swap_buffer(NULL);
    }
    return 1;
}

static int _handle_motion_notify(xcb_motion_notify_event_t *e) {
    xcb_rectangle_t full_out_rect;
    xcb_rectangle_t *refresh_rects = NULL;
    xcb_xfixes_fetch_region_reply_t *reply;
    int nrects;
    xcb_xfixes_region_t full_out_region, refresh_region;
    switch (_this.state) {
        case _READY:
            full_out_region = xcb_generate_id(gd.conn);
            refresh_region = xcb_generate_id(gd.conn);
            full_out_rect = _full_out_rect();
            xcb_xfixes_create_region(gd.conn, full_out_region, 1, &full_out_rect);
            xcb_xfixes_create_region(gd.conn, refresh_region, 0, NULL);
            xcb_xfixes_subtract_region(gd.conn, full_out_region, _this.good_region, refresh_region);
            reply = xcb_xfixes_fetch_region_reply(
                    gd.conn,
                    xcb_xfixes_fetch_region(gd.conn, refresh_region),
                    NULL);
            refresh_rects = xcb_xfixes_fetch_region_rectangles(reply);
            if ((nrects = xcb_xfixes_fetch_region_rectangles_length(reply))) {
                _draw(refresh_rects, nrects);
            }
            _swap_buffer(NULL);
            xcb_xfixes_union_region(gd.conn, _this.good_region, refresh_region, _this.good_region);

            xcb_xfixes_destroy_region(gd.conn, full_out_region);
            xcb_xfixes_destroy_region(gd.conn, refresh_region);

            xcb_flush(gd.conn);
            free(reply);
            break;
        default:
            break;
    }
    return 1;
}

static void _out_step() {
    xcb_rectangle_t r;

    _this.zoom_level -= (_this.zoom_level - _NONE) / _this.out_steps;
    r = _full_out_rect();
    _draw(&r, 1);
    _swap_buffer(NULL);
    _this.out_steps--;
    xcb_flush(gd.conn);
}

static void _clean_scene() {
    fwm_composite_release_effects_window();
    xcb_render_free_picture(gd.conn, _this.win_buffer_pic);
    xcb_free_pixmap(gd.conn, _this.win_buffer);
    xcb_flush(gd.conn);
    _this.state = _IDLE;
    _this.win = XCB_NONE;
    _this.zoom_level = _NONE;
}

static int _handle_key_release(xcb_key_release_event_t *e) {
    uint8_t i;
    switch (_this.state) {
        case _READY:
            if (e->detail == _WIN) {
                _this.out_steps = (_this.zoom_level - _NONE) / _STEP;
                _this.out_steps = _OUT_STEPS;
                for (i = 1; i <= _this.out_steps; i++) {
                    fwm_animan_append_action(_out_step, NULL, i);
                }
                fwm_animan_append_action(_clean_scene, NULL, i);
            }
        case _IDLE:
            break;
        default:
            break;
    }
    return 1;
}

static int _handle_key_press(xcb_key_press_event_t *e) {
    xcb_params_cw_t p;
    uint32_t m;
    xcb_rectangle_t r;
    float tmp_zoom_level = _this.zoom_level;
    switch (_this.state) {
        case _IDLE:
            if (e->detail == _WIN) {
                _this.state = _READY;
            };
            break;
        case _READY:
            if (e->detail == _PLUS || e->detail == _MINUS) {
                if (_this.win == XCB_NONE) {
                    _this.win = fwm_composite_get_effects_window();
                    XCB_AUX_ADD_PARAM(&m, &p, event_mask, XCB_EVENT_MASK_POINTER_MOTION);
                    xcb_aux_change_window_attributes(gd.conn, _this.win, m, &p);

                    _this.win_buffer = xcb_generate_id(gd.conn);
                    xcb_create_pixmap(gd.conn, gd.def_screen->root_depth, _this.win_buffer, _this.win,
                            gd.def_screen->width_in_pixels * _MAX_ZOOM,
                            gd.def_screen->height_in_pixels * _MAX_ZOOM
                            );
                    _this.win_buffer_pic = fwm_render_create_picture(_this.win_buffer, XCB_NONE, NULL);
                }

                switch (e->detail) {
                    case _PLUS:
                        tmp_zoom_level -= _STEP;
                        if (tmp_zoom_level >= _NONE) {
                            _this.zoom_level = tmp_zoom_level;
                            r = _full_out_rect();
                            _draw(&r, 1);
                            _swap_buffer(NULL);
                        }
                        break;
                    case _MINUS:
                        tmp_zoom_level += _STEP;
                        if (tmp_zoom_level <= _MAX_ZOOM) {
                            _this.zoom_level = tmp_zoom_level;
                            r = _full_out_rect();
                            _draw(&r, 1);
                            _swap_buffer(NULL);
                        }
                        break;
                }
                /* Init good region after zoom level change. */
                if (_this.good_region == XCB_NONE) {
                    xcb_xfixes_destroy_region(gd.conn, _this.good_region);
                }
                _this.good_region = xcb_generate_id(gd.conn);
                xcb_xfixes_create_region(gd.conn, _this.good_region, 1, &r);
            }
        default:
            break;
    }
    xcb_flush(gd.conn);
    return 1;
}

int fwm_zoom_init() {
    _this.win = _this.good_region = XCB_NONE;
    _this.state = _IDLE;
    _this.zoom_level = _NONE;
    /* Needs to be processed after composite has processed itself. */
    fwm_event_push_damage_notify_handler(_handle_damage_notify, FWM_EVENT_HANDLER_PRIORITY_AFTER);
    fwm_event_push_key_press_handler(_handle_key_press, FWM_EVENT_HANDLER_PRIORITY_AFTER);
    fwm_event_push_key_release_handler(_handle_key_release, FWM_EVENT_HANDLER_PRIORITY_AFTER);
    fwm_event_push_motion_notify_handler(_handle_motion_notify, FWM_EVENT_HANDLER_PRIORITY_AFTER);
    xcb_grab_key(gd.conn, 0, gd.def_screen->root, XCB_MOD_MASK_ANY, _WIN,
            XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_grab_key(gd.conn, 0, gd.def_screen->root, XCB_MOD_MASK_1, _PLUS,
            XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_grab_key(gd.conn, 0, gd.def_screen->root, XCB_MOD_MASK_1, _MINUS,
            XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    fwm_event_set_root_mask(XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE
            | XCB_EVENT_MASK_POINTER_MOTION);
    FWM_MSG("zoom plugin initialized.");
    return 1;
}
