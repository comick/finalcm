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
#include <assert.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <pthread.h>

#include <xcb/composite.h>
#include <xcb/xcb.h>
#include <xcb/render.h>

#include "window.h"
#include "animan.h"
#include "coverswitch.h"
#include "render.h"
#include "event.h"
#include "ws.h"
#include "damage.h"
#include "composite.h"

#define _ALT 64
#define _TAB 23
#define _SHIFT 50

/*
 * _THUMB_total_size should be % _transition_steps == 0 for maximum realism
 */
#define _THUMB_MAX_SIDE_SIZE 150
#define _THUMB_OUTER_BORDER_SIZE 5
#define _THUMB_TOTAL_SIZE (_THUMB_MAX_SIDE_SIZE + 2 * _THUMB_OUTER_BORDER_SIZE)
#define _TRANSITION_FRAMES 5

#define _BUFFER_W(win_w) (win_w + (_THUMB_TOTAL_SIZE << 1))
#define _GET_TOP_X(win_count) _THUMB_TOTAL_SIZE + ((win_count - (win_count % 2)) / 2) * _THUMB_TOTAL_SIZE + _THUMB_OUTER_BORDER_SIZE

/* Need sync between damage and animan draws. */
#define _SYNCHRONIZED(A) pthread_mutex_lock(&(_this.paint_mutex)); \
    A; \
    pthread_mutex_unlock(&(_this.paint_mutex));

typedef enum {
    _IDLE,
    _READY,
    _SWITCHING,
    _ROTATING
} _coverswitch_states;

static struct {
    _coverswitch_states state;
    fwm_ws_t ws;
    fwm_ws_rotate_mode_t rotate_mode;
    fwm_ws_rotate_mode_t next_rotate_mode;
    xcb_window_t win;
    xcb_render_picture_t win_buffer_pic;
    xcb_pixmap_t win_buffer;
    uint16_t win_height, win_width;
    int16_t win_x, win_y;
    int16_t win_offset;

    uint8_t num_win;
    /* This avoid animan and damage events trying to paint in the same time. */
    pthread_mutex_t paint_mutex;
    fwm_render_mask_t mask;
} _this;

/* Paint the full coverswitch window without swapping buffers. */

/* TODO usare clip_r come rettangolo di clipping può essere necessario usare buffer grande quanto root così da non dover smazzare con le misure. */
static void _paint(xcb_rectangle_t *clip_r) {
    uint16_t top_x;
    xcb_rectangle_t r;
    fwm_mwin_t *last_drawn = _this.ws.top;
    uint8_t win_per_side, i;

    if (_this.state == _IDLE || _this.state == _READY) {
        return; /* Do not draw when the window is not visible. */
    }
    /* Clean buffer. */
    r.x = r.y = 0;
    r.height = _this.win_height;
    r.width = _BUFFER_W(_this.win_width);
    /* Apply the bg. */
    xcb_copy_area(gd.conn, fwm_composite_get_root_buffer_pixmap(),
            _this.win_buffer, gd.default_gc,
            _this.win_x - _THUMB_TOTAL_SIZE - _this.win_offset, _this.win_y,
            r.x, r.y,
            r.width, r.height
            );
    xcb_render_fill_rectangles(gd.conn, XCB_RENDER_PICT_OP_OVER,
            _this.win_buffer_pic, _this.mask.value.color, 1, &r);

    /* Put the top window on center or on right depenging on number of window. */
    top_x = _GET_TOP_X(_this.num_win);
    r.y = _THUMB_OUTER_BORDER_SIZE;
    r.x = top_x;
    r.width = r.height = _THUMB_MAX_SIDE_SIZE;
    /* Draw the cover switching top. */
    fwm_render_draw_thumb(_this.ws.top, _this.win_buffer_pic, &r, NULL);
    win_per_side = (_this.num_win - 1) / 2;
    /* Draw windows on the left. */
    last_drawn = _this.ws.top;
    for (i = 0; i <= win_per_side; i++) {
        r.x -= _THUMB_TOTAL_SIZE;
        last_drawn = last_drawn->prev;
        fwm_render_draw_thumb(last_drawn, _this.win_buffer_pic, &r, &_this.mask);
    }
    /* Draw windows on the right. */
    r.x = top_x;
    last_drawn = _this.ws.top;
    for (i = 0; i <= win_per_side; i++) {
        r.x += _THUMB_TOTAL_SIZE;
        last_drawn = last_drawn->next;
        fwm_render_draw_thumb(last_drawn, _this.win_buffer_pic, &r, &_this.mask);
    }
    xcb_flush(gd.conn);
}

/* Copy the visible part of the buffer into our window. */
static void _swap_buffer() {
    xcb_copy_area(gd.conn, _this.win_buffer, _this.win, gd.default_gc,
            _THUMB_TOTAL_SIZE + _this.win_offset, 0,
            0, 0,
            _this.win_width, _this.win_height);
    xcb_flush(gd.conn);
}

/* Paint the damaged part of the coverswitch window without swapping buffers. */
static void _paint_and_swap_thumb_buffer(xcb_window_t win) {
    xcb_rectangle_t r;
    uint16_t x_if_left, x_if_right;
    fwm_mwin_t *tmp_left, *tmp_right;


    tmp_left = tmp_right = _this.ws.top;
    x_if_left = x_if_right = _GET_TOP_X(_this.num_win);

    while (tmp_left->win != win && tmp_right->win != win) {
        tmp_left = tmp_left->prev;
        tmp_right = tmp_right->next;
        x_if_left -= _THUMB_TOTAL_SIZE;
        x_if_right += _THUMB_TOTAL_SIZE;
    }
    r.x = tmp_left->win == win ? x_if_left : x_if_right;
    r.y = _THUMB_OUTER_BORDER_SIZE;
    r.width = r.height = _THUMB_MAX_SIDE_SIZE;
    fwm_render_draw_thumb(
            tmp_left->win == win ? tmp_left : tmp_right,
            _this.win_buffer_pic, &r,
            (_this.ws.top == tmp_left) ? NULL : (&_this.mask) /* Do not want mask onto the temp top. */
            );
    xcb_copy_area(gd.conn, _this.win_buffer, _this.win, gd.default_gc,
            r.x, r.y,
            r.x - _THUMB_TOTAL_SIZE, r.y,
            r.width, r.height);
    xcb_flush(gd.conn);
}
// TODO mettere sta roba in show window

static void _create_window() {
    /* Create cover switch window. */
    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK;
    uint32_t values[2];
    values[0] = gd.def_screen->white_pixel;
    values[1] = 1;
    values[2] = fwm_event_root_mask_get();
    _this.win = xcb_generate_id(gd.conn);
    xcb_create_window(gd.conn, gd.def_screen->root_depth, _this.win,
            gd.def_screen->root,
            0, 0, 1, 1,
            0, XCB_WINDOW_CLASS_INPUT_OUTPUT, gd.def_screen->root_visual,
            mask, values
            );
    xcb_flush(gd.conn);
}

static void _hide_window() {
    xcb_unmap_window(gd.conn, _this.win);
    xcb_render_free_picture(gd.conn, _this.win_buffer_pic);
    xcb_free_pixmap(gd.conn, _this.win_buffer);
    xcb_flush(gd.conn);
}

static void _show_window() {
    uint16_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
    uint32_t values[4];
    uint32_t *x = &values[0], *y = &values[1], *width = &values[2], *height = &values[3];

    _this.num_win = min(floor(gd.def_screen->width_in_pixels / _THUMB_TOTAL_SIZE), gd.win_count);
    if (!(_this.num_win % 2)) {
        _this.num_win--; /* To have the top centered. */
    }
    *height = _THUMB_TOTAL_SIZE;
    *width = _this.num_win * _THUMB_TOTAL_SIZE;
    *x = gd.def_screen->width_in_pixels / 2 - *width / 2;
    *y = gd.def_screen->height_in_pixels / 2 - *height / 2;
    xcb_configure_window(gd.conn, _this.win, mask, values);
    _this.win_height = *height;
    _this.win_width = *width;
    _this.win_x = *x;
    _this.win_y = *y;

    _this.win_buffer = xcb_generate_id(gd.conn);
    // TODO creare il buffer grande quanyto la root così posso maneggiare meglio i datti alla root maappandoli diretti qui
    // poi quando si swappa si sa win x win y a altro
    xcb_create_pixmap(gd.conn, gd.def_screen->root_depth, _this.win_buffer, _this.win,
            _BUFFER_W(_this.win_width), _THUMB_TOTAL_SIZE
            );
    _this.win_buffer_pic = fwm_render_create_picture(_this.win_buffer, XCB_NONE, NULL);

    mask = XCB_CONFIG_WINDOW_STACK_MODE;
    values[0] = XCB_STACK_MODE_ABOVE;
    xcb_configure_window(gd.conn, _this.win, mask, values);
    xcb_map_window(gd.conn, _this.win);
    xcb_flush(gd.conn);
}

static int _handle_damage_notify(xcb_damage_notify_event_t *e) {
    if (_this.state == _SWITCHING) {
        if (fwm_wt_get(gd.wt, e->drawable) != NULL) {
            _paint_and_swap_thumb_buffer(e->drawable);
            xcb_flush(gd.conn);
        } else if (e->drawable == fwm_composite_get_root_buffer_pixmap()) {
            _paint(&(e->geometry)); // TODO vedere regione della mia iwn, regione danneggiata e se c'è intersezione ridisegnare solo la parte interessata!!!
            _swap_buffer();
            xcb_flush(gd.conn);
        }
    }
    return 1;
}

/* TODO in caso di map e unmap le sytrutture cambiano, potrei semplicemente farla ridisegnare...
 allargando la finestra con una bella paint
 o* cchio però allo stack order
 */

static int _handle_expose(xcb_expose_event_t *e) {
    if (e->count != 0) {
        return 1;
    }
    if (e->window == _this.win) {
        _paint(NULL);
        _swap_buffer();
        xcb_flush(gd.conn);
    }
    return 1;
}

static int _handle_map_notify(xcb_map_notify_event_t *e) {
    if (e->window != _this.win && _this.state == _SWITCHING) {
        _hide_window();
        _this.state = _IDLE;
    }
    return 1;
}

static int _handle_unmap_notify(xcb_unmap_notify_event_t *e) {
    if (e->window != _this.win && _this.state == _SWITCHING) {
        _hide_window();
        _this.state = _IDLE;
    }
    return 1;
}

static int _handle_key_release(xcb_key_release_event_t *e) {
    switch (e->detail) {
        case _ALT:
            /* TODO remove any temporal event from the animan queue associated to the coverswitch.*/
            if (_this.state == _SWITCHING || _this.state == _ROTATING) {
                if (_this.ws.top != NULL && _this.ws.top != gd.ws->top) {
                    fwm_window_stack_above(_this.ws.top);
                }
                _hide_window();
                _this.state = _IDLE;
                _this.rotate_mode = FWM_WS_ROTATE_LEFT;
            }
            break;
        case _SHIFT:
            _this.next_rotate_mode = FWM_WS_ROTATE_LEFT;
            break;
    }
    return 1;
}

/*
 TODO  se non si è visibile inutile disegnare, un giorno ciò sarà ottenuto rimuovendo tutt ala
 * roba coverswitch dallo heap.
 */
static void _coverswitch_animan_mid_handler(void *p) {
    if (_this.state == _IDLE) {
        return;
    }
    _SYNCHRONIZED(
    if (_this.rotate_mode == FWM_WS_ROTATE_LEFT) {
        _this.win_offset += _THUMB_TOTAL_SIZE / _TRANSITION_FRAMES;
    } else {
        _this.win_offset -= _THUMB_TOTAL_SIZE / _TRANSITION_FRAMES;
    }
    _paint(NULL);
            _swap_buffer();
            );
}

static void _coverswitch_animan_end_handler(void *p) {
    if (_this.state == _IDLE) {
        return;
    }
    _SYNCHRONIZED(
            fwm_ws_rotate(&(_this.ws), _this.rotate_mode);
            _this.rotate_mode = _this.next_rotate_mode;
            _this.win_offset = 0;
            _paint(NULL);
            _swap_buffer();
            /* TODO  _handle_alt key release set state to idle, so if the animation is not finished
             * we should set to switching only if not idle.
             */
    if (_this.state == _ROTATING) {
        _this.state = _SWITCHING;
    }
    );
}

static int _handle_key_press(xcb_key_press_event_t *e) {
    int j;
    switch (e->detail) {
        case _SHIFT:
            _this.next_rotate_mode = FWM_WS_ROTATE_RIGHT;
            break;
        case _ALT:
            if (_this.state == _IDLE) {
                _this.state = _READY;
                _this.rotate_mode = FWM_WS_ROTATE_LEFT;
            }
            break;
        case _TAB:
            switch (_this.state) {
                case _READY:
                    if (gd.win_count > 1) {
                        _this.ws.top = gd.ws->top;
                        _this.state = _SWITCHING;
                        _this.win_offset = 0;
                        _show_window();
                        _paint(NULL);
                        _swap_buffer();
                    }
                    break;
                case _SWITCHING:
                    _this.state = _ROTATING;
                    /* TODO cambiare alpha alle finestre senza restackarle. */
                    for (j = 1; j <= _TRANSITION_FRAMES; j++) {
                        fwm_animan_append_action(_coverswitch_animan_mid_handler, NULL, j);
                    }
                    fwm_animan_append_action(_coverswitch_animan_end_handler, NULL, j + 1);
                    break;
                case _ROTATING:
                    /* Do not start a new transition while another is running. */
                    break;
                case _IDLE:
                    break;
            }
            break;
    }
    return 1;
}

/* Create cover switch resources and set needed event handlers. */
int fwm_coverswitch_init() {
    fwm_event_push_key_press_handler(_handle_key_press, FWM_EVENT_HANDLER_PRIORITY_BEFORE);
    fwm_event_push_expose_handler(_handle_expose, FWM_EVENT_HANDLER_PRIORITY_BEFORE);
    fwm_event_push_key_release_handler(_handle_key_release, FWM_EVENT_HANDLER_PRIORITY_BEFORE);
    /* When noticing about mapped/unmapped windows. */
    fwm_event_push_map_notify_handler(_handle_map_notify, FWM_EVENT_HANDLER_PRIORITY_BEFORE);
    fwm_event_push_unmap_notify_handler(_handle_unmap_notify, FWM_EVENT_HANDLER_PRIORITY_BEFORE);
    /* Need some more event. */
    fwm_event_set_root_mask(XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE);
    /* Need to handle damages, AFTER "the composite has handler refreshing the root buffer". */
    fwm_event_push_damage_notify_handler(_handle_damage_notify, FWM_EVENT_HANDLER_PRIORITY_AFTER);
    xcb_grab_key(gd.conn, 0, gd.def_screen->root, XCB_MOD_MASK_ANY, _ALT,
            XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_grab_key(gd.conn, 0, gd.def_screen->root, XCB_MOD_MASK_1, _TAB,
            XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    /* Init resources. */
    _this.mask.type = FWM_RENDER_MASK_COLOR;
    _this.mask.value.color.alpha = 0xaaaa;
    _this.mask.value.color.red = 0xaaaa;
    _this.mask.value.color.green = 0xaaaa;
    _this.mask.value.color.blue = 0xaaaa;

    _this.state = _IDLE;
    _this.ws.top = NULL;
    _this.next_rotate_mode = FWM_WS_ROTATE_LEFT;
    _this.win_buffer = XCB_NONE;
    _this.win_offset = 0;
    pthread_mutex_init(&_this.paint_mutex, NULL);
    _create_window();
    xcb_flush(gd.conn);
    FWM_MSG("coverswitch plugin initialized");
    return 1;
}
