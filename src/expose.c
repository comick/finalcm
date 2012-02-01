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

#include <math.h>
#include <stdlib.h>

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

#include "finalwm.h"
#include "window.h"
#include "expose.h"
#include "render.h"
#include "event.h"
#include "damage.h"
#include "composite.h"
#include "animan.h"

#define _TRANS_FRAMES 5

#define _COORD_STEP(ORIG_C, FINAL_C, STEPS) ((ORIG_C > FINAL_C) ? -(ORIG_C - FINAL_C) / STEPS : (FINAL_C - ORIG_C) / STEPS)
#define _DIM_STEP(ORIG_DIM, FINAL_DIM, STEPS) ((FINAL_DIM - ORIG_DIM) / STEPS)

#define _CONTAINS(RECT, EV) (EV->event_x > RECT.x && EV->event_x < RECT.x + RECT.width && \
                EV->event_y > RECT.y && EV->event_y < RECT.y + RECT.height)

typedef enum {
    _IDLE,
    _PLACING,
    _UNPLACING,
    _READY
} _expose_state;

typedef struct _exposed_window {
    fwm_mwin_t *managed_win;
    xcb_rectangle_t final_cell, temp_cell;
    uint16_t final_opacity, temp_opacity; /* Questo forse non è il tipo giusto. */
    struct _exposed_window *next, *prev;
} _exposed_win_t;

static struct {
    uint8_t wait_tab;
    _expose_state state;
    xcb_window_t win;
    xcb_pixmap_t win_buffer;
    xcb_render_picture_t win_buffer_pic;
    _exposed_win_t *bottom, *top; /* We paint bottom->up. */
    uint8_t remaining_steps;
} _this;

/* Copy a rectangular part of the buffer into our out window. */
static void _swap_buffer(xcb_rectangle_t *clip_rect) {
    xcb_rectangle_t clip_r;
    if (clip_rect == NULL) {
        clip_r.x = clip_r.y = 0;
        clip_r.height = gd.def_screen->height_in_pixels;
        clip_r.width = gd.def_screen->width_in_pixels;
    } else {
        clip_r = *clip_rect;
    }
    xcb_copy_area(gd.conn, _this.win_buffer, _this.win, gd.default_gc,
            clip_r.x, clip_r.y,
            clip_r.x, clip_r.y,
            clip_r.width, clip_r.height);
    xcb_flush(gd.conn);
}

/* Clean (with the root tile) a rectangular area. */
static void _clean_rect(const xcb_rectangle_t *r) {
    xcb_render_composite(gd.conn,
            XCB_RENDER_PICT_OP_SRC,
            fwm_composite_get_root_tile_pic(), 0, _this.win_buffer_pic,
            r->x, r->y,
            0, 0,
            r->x, r->y,
            r->width, r->height
            );
}

/* Draw the intermediate step placing windows. */
static void _paint_place_step(void *p) {
    _exposed_win_t *tmp_exposed_win = _this.bottom;
    _this.remaining_steps--;
    /* Clean old thumbs rects. */
    while (tmp_exposed_win != NULL) {
        _clean_rect(&tmp_exposed_win->temp_cell);
        tmp_exposed_win = tmp_exposed_win->next;
    }
    tmp_exposed_win = _this.bottom;
    /* Draw new thumbs. */
    while (tmp_exposed_win != NULL) {
        tmp_exposed_win->temp_cell.x += _COORD_STEP(tmp_exposed_win->temp_cell.x,
                tmp_exposed_win->final_cell.x, _this.remaining_steps);
        tmp_exposed_win->temp_cell.y += _COORD_STEP(tmp_exposed_win->temp_cell.y,
                tmp_exposed_win->final_cell.y, _this.remaining_steps);
        tmp_exposed_win->temp_cell.width += _DIM_STEP(tmp_exposed_win->temp_cell.width,
                tmp_exposed_win->final_cell.width, _this.remaining_steps);
        tmp_exposed_win->temp_cell.height += _DIM_STEP(tmp_exposed_win->temp_cell.height,
                tmp_exposed_win->final_cell.height, _this.remaining_steps);

        fwm_render_draw_thumb(tmp_exposed_win->managed_win,
                _this.win_buffer_pic, &(tmp_exposed_win->temp_cell), NULL);
        tmp_exposed_win = tmp_exposed_win->next;
    }
    _swap_buffer(NULL);
}

/* Draw the final step placing windows. */
static void _paint_final_place_step(void *p) {
    _exposed_win_t *ew = _this.bottom;
    fwm_mwin_t *mw;
    while (ew != NULL) {
        mw = ew->managed_win;
        fwm_render_draw_thumb(mw, _this.win_buffer_pic,
                &(ew->final_cell), NULL);
        ew->temp_cell = ew->final_cell;
        ew->final_cell.x = mw->rect.x;
        ew->final_cell.y = mw->rect.y;
        ew->final_cell.width = mw->rect.width;
        ew->final_cell.height = mw->rect.height;
        ew = ew->next;
    }
    _swap_buffer(NULL);
    _this.state = _READY;
    _this.remaining_steps = _TRANS_FRAMES;
}

/* Free client side and server size resources. */
static void _clean_scene(void *p) {
    _exposed_win_t *next_ew, *free_ew = _this.bottom;
    while (free_ew != NULL) {
        next_ew = free_ew->next;
        free(free_ew);
        free_ew = next_ew;
    }
    _this.state = _IDLE;
    fwm_composite_release_effects_window();
    xcb_render_free_picture(gd.conn, _this.win_buffer_pic);
    xcb_free_pixmap(gd.conn, _this.win_buffer);
    xcb_flush(gd.conn);
}

/*
 * Find a good integer solution of the system:
 *
 * rows / cols = p
 * rows * cols = count
 */
static void _find_cells(uint16_t root_h, uint16_t root_w, int count, int *rows, int *cols) {
    xcb_point_t candidates[4];
    /* Defaults to the bigger matrix (factors both ceiled). */
    uint8_t i, best_i = 3;
    double temp_ratio;
    double temp_ratio_offset;

    double best_ratio_offset = 999999; /* Inf. */
    double screen_ratio = (double) root_h / (double) root_w;
    double d_cols = sqrt((double) count / screen_ratio);
    double d_rows = screen_ratio* d_cols;

    candidates[0].x = floor(d_cols);
    candidates[0].y = floor(d_rows);

    candidates[1].x = floor(d_cols);
    candidates[1].y = ceil(d_rows);

    candidates[2].x = ceil(d_cols);
    candidates[2].y = floor(d_rows);

    candidates[3].x = ceil(d_cols);
    candidates[3].y = ceil(d_rows);

    for (i = 0; i < 4; i++) {
        temp_ratio = (double) candidates[i].x / (double) candidates[i].y;
        temp_ratio_offset = temp_ratio > screen_ratio ? temp_ratio - screen_ratio : screen_ratio - temp_ratio;
        if (candidates[i].x * candidates[i].y >= count && temp_ratio_offset < best_ratio_offset) {
            best_i = i;
            best_ratio_offset = temp_ratio_offset;
        }
    }
    *rows = candidates[best_i].y;
    *cols = candidates[best_i].x;
}

/* After a thumb has been selected, prepare inverse animation steps. */
static void _hide() {
    int j;
    _this.state = _UNPLACING;
    for (j = 1; j < _TRANS_FRAMES; j++) {
        fwm_animan_append_action(_paint_place_step, NULL, j);
    }
    fwm_animan_append_action(_clean_scene, NULL, j + 1);
}

/* Prepare a single "exposed window" before the animation starts. */
static void _setup_exposed_win(_exposed_win_t *ew, xcb_rectangle_t out_rect, int col_i, int row_i) {
    float scale_x, scale_y, scale;
    int16_t translate_x, translate_y;
    scale_x = (float) (out_rect.width) / (float) ew->managed_win->rect.width;
    scale_y = (float) (out_rect.height) / (float) ew->managed_win->rect.height;
    scale = (scale_x < scale_y) ? scale_x : scale_y;
    translate_x = 0;
    translate_y = 0;
    if (scale_x > scale_y) {
        translate_x = (float) (out_rect.width - (ew->managed_win->rect.width * scale)) / 2.0f;
    }
    if (scale_y > scale_x) {
        translate_y = (float) (out_rect.height - (ew->managed_win->rect.height * scale)) / 2.0f;
    }
    ew->final_cell.x = out_rect.x + (col_i * (out_rect.width + 20) + 10) + translate_x;
    ew->final_cell.y = (row_i * (out_rect.height + 20)) + translate_y;
    ew->final_cell.width = ew->managed_win->rect.width*scale;
    ew->final_cell.height = ew->managed_win->rect.height*scale;

    /* The cell starts with full dimension. */
    ew->temp_cell.x = ew->managed_win->rect.x;
    ew->temp_cell.y = ew->managed_win->rect.y;
    ew->temp_cell.width = ew->managed_win->rect.width;
    ew->temp_cell.height = ew->managed_win->rect.height;
}

/* Says if a window is higher (the more y is near to zero, the more the window is higher) than mw2. */
int _is_higher_than(const void *ew1, const void *ew2) {
    return ((_exposed_win_t *) *(_exposed_win_t **) ew1)->managed_win->rect.y -
            ((_exposed_win_t *) *(_exposed_win_t **) ew2)->managed_win->rect.y;
}

/* Says if a window is higher (the more y is near to zero, the more the window is higher) than mw2. */
int _is_more_left_placed_than(const void *ew1, const void *ew2) {
    return ((_exposed_win_t *) *(_exposed_win_t **) ew1)->managed_win->rect.x -
            ((_exposed_win_t *) *(_exposed_win_t **) ew2)->managed_win->rect.x;
}

/* Prepare the stack of "exposed windows" and init the placing animation frames. */
static void _show() {
    uint8_t i;
    int rows, cols;
    int row_i, col_i;
    int row_cols;
    xcb_rectangle_t generic_cell, root_r;
    xcb_params_cw_t p;
    uint32_t m;
    _exposed_win_t *tmp_ew, *last_exp_win;
    fwm_mwin_t *tmp_man_win;
    _exposed_win_t **ordered_exp_wins;

    /* Get effects window and create buffer + picture. */
    if ((_this.win = fwm_composite_get_effects_window()) == XCB_NONE) {
        return; /* The effects window is busy! */
    }
    XCB_AUX_ADD_PARAM(&m, &p, event_mask, XCB_EVENT_MASK_BUTTON_PRESS);
    xcb_aux_change_window_attributes(gd.conn, _this.win, m, &p);
    _this.win_buffer = xcb_generate_id(gd.conn);
    xcb_create_pixmap(gd.conn, gd.def_screen->root_depth, _this.win_buffer, _this.win,
            gd.def_screen->width_in_pixels, gd.def_screen->height_in_pixels);
    _this.win_buffer_pic = fwm_render_create_picture(_this.win_buffer, XCB_NONE, NULL);

    root_r.x = root_r.y = 0;
    root_r.width = gd.def_screen->width_in_pixels;
    root_r.height = gd.def_screen->height_in_pixels;
    _clean_rect(&root_r);

    /* Decide how to place window thumbs. */
    _find_cells(gd.def_screen->height_in_pixels, gd.def_screen->width_in_pixels, gd.win_count, &rows, &cols);

    generic_cell.width = (gd.def_screen->width_in_pixels / cols) - 20;
    generic_cell.height = (gd.def_screen->height_in_pixels / rows) - 20;

    /* Start preparing exposed windows. */
    ordered_exp_wins = malloc(sizeof (_exposed_win_t *) * gd.win_count);
    tmp_man_win = gd.ws->top->next;
    last_exp_win = malloc(sizeof (_exposed_win_t));
    last_exp_win->managed_win = tmp_man_win;
    last_exp_win->prev = NULL;
    _this.bottom = last_exp_win;
    ordered_exp_wins[0] = last_exp_win;
    tmp_man_win = tmp_man_win->next;
    i = 1;
    while (tmp_man_win != gd.ws->top->next) {
        tmp_ew = malloc(sizeof (_exposed_win_t));
        tmp_ew->managed_win = tmp_man_win;
        tmp_ew->prev = last_exp_win;
        tmp_ew->next = NULL;
        last_exp_win->next = tmp_ew;
        last_exp_win = tmp_ew;
        ordered_exp_wins[i++] = tmp_ew;
        tmp_man_win = tmp_man_win->next;
    }
    _this.top = last_exp_win;
    qsort(ordered_exp_wins, gd.win_count, sizeof (_exposed_win_t *),
            _is_higher_than);

    col_i = row_i = 0;
    generic_cell.x = 0;
    for (row_i = 0; row_i < rows; row_i++) {
        /* Treats last line in a different way to center if it was not full. */
        if (row_i == rows - 1) {
            row_cols = gd.win_count - row_i * cols;
            if (row_cols < cols) {
                generic_cell.x = ((cols - row_cols) * generic_cell.width + 60) / 2;
            }
        } else {
            row_cols = cols;
        }
        qsort(&ordered_exp_wins[row_i * cols], row_cols, sizeof (_exposed_win_t *),
                _is_more_left_placed_than);
        for (col_i = 0; col_i < row_cols; col_i++) {
            _setup_exposed_win(ordered_exp_wins[row_i * cols + col_i],
                    generic_cell, col_i, row_i);
        }
    }
    free(ordered_exp_wins);
    xcb_flush(gd.conn);

    _this.state = _PLACING;
    _this.remaining_steps = _TRANS_FRAMES;
    for (i = 1; i < _TRANS_FRAMES; i++) {
        fwm_animan_append_action(_paint_place_step, NULL, i);
    }
    fwm_animan_append_action(_paint_final_place_step, NULL, i + 1);
}

/* We start the exposition when the user touch triangle reactangle at the top left. */
static int _handle_motion_notify(xcb_motion_notify_event_t *e) {
    if (e->root_x + e->root_y < 20 && _this.state == _IDLE && gd.win_count >= 2) {
        _show();
    } else {
        xcb_ungrab_pointer(gd.conn, XCB_TIME_CURRENT_TIME);
        e->time = XCB_CURRENT_TIME;
        xcb_send_event(gd.conn, 1, e->child, 0, (char *) e);
        xcb_grab_pointer(gd.conn, 1, gd.def_screen->root,
                XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_PRESS,
                XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_CURRENT_TIME);
        xcb_flush(gd.conn);
    }
    return 1;
}

/* We want offer real time changes to our exposed thumbs. */
static int _handle_damage_notify(xcb_damage_notify_event_t *e) {
    _exposed_win_t *exposed_win;

    if (_this.state != _READY) {
        return 1;
    }
    exposed_win = _this.bottom;
    while (exposed_win != NULL) {
        if (exposed_win->managed_win->win == e->drawable) {
            fwm_render_draw_thumb(exposed_win->managed_win, _this.win_buffer_pic, &(exposed_win->temp_cell), NULL);
            _swap_buffer(&(exposed_win->temp_cell));
            break;
        } else {
            exposed_win = exposed_win->next;
        }
    }
    return 1;
}

/* Pick correlation to discover if (and whitch) window selected. */
static int _handle_button_press(xcb_button_press_event_t *e) {
    _exposed_win_t *ew;
    /*

        if (_this.state != _READY) {
            xcb_ungrab_pointer(gd.conn, XCB_TIME_CURRENT_TIME);
            e->time = XCB_CURRENT_TIME;
            xcb_send_event(gd.conn, 1, e->event, 0, (char *) e);
            xcb_grab_pointer(gd.conn, 1, gd.def_screen->root,
                    XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_PRESS,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_CURRENT_TIME);
            xcb_flush(gd.conn);
            return 1;
        }
     */
    ew = _this.bottom;
    while (ew != NULL) {
        if (_CONTAINS(ew->temp_cell, e)) {
            /* Adjust exposed windows stacking order to reflect the change animating back. */
            fwm_window_stack_above(ew->managed_win);
            if (ew == _this.bottom) {
                _this.bottom = ew->next;
                _this.bottom->prev = NULL;
                ew->prev = _this.top;
                _this.top->next = ew;
                ew->next = NULL;
            } else if (ew == _this.top) {
            } else {
                ew->prev->next = ew->next;
                ew->next->prev = ew->prev;
                ew->next = NULL;
                _this.top->next = ew;
            }
            _hide();
            break;
        } else {
            ew = ew->next;
        }
    }
    return 1;
}

#define _CTRL 37

static int _handle_key_press(xcb_key_press_event_t *e) {
    if (_this.state == _IDLE && e->detail == _CTRL) {
        _show();
    }
}

/* Init resources and other. */
void fwm_expose_init() {
    /* TODO mettere un handler di geometry notify e altro... per qunado una finestra cambia forma mentre esposta. */
    //    fwm_event_push_motion_notify_handler(_handle_motion_notify, FWM_EVENT_HANDLER_PRIORITY_BEFORE);
    fwm_event_push_button_press_handler(_handle_button_press, FWM_EVENT_HANDLER_PRIORITY_BEFORE);
    fwm_event_push_damage_notify_handler(_handle_damage_notify, FWM_EVENT_HANDLER_PRIORITY_BEFORE);
    fwm_event_push_key_press_handler(_handle_key_press, FWM_EVENT_HANDLER_PRIORITY_BEFORE);

    fwm_event_set_root_mask(XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE
            | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_PRESS);
    xcb_grab_key(gd.conn, 0, gd.def_screen->root, XCB_MOD_MASK_ANY, _CTRL,
            XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    /*

            xcb_grab_pointer(gd.conn, 1, gd.def_screen->root,
                    XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_PRESS,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_CURRENT_TIME);
     
     */
    _this.win = _this.win_buffer = _this.win_buffer_pic = XCB_NONE;
    _this.state = _IDLE;
    _this.bottom = NULL;
    _this.wait_tab = 0; /* Last time to have keyboard control. */
    xcb_flush(gd.conn);
    FWM_MSG("expose plugin initialized");
}
