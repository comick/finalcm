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
#include <stdlib.h>

#include <xcb/xcb_atom.h>
#include <xcb/composite.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_aux.h>

#include "render.h"
#include "damage.h"
#include "event.h"
#include "composite.h"

#include "../config.h"

static struct {
    xcb_render_picture_t root_tile_pic, root_buffer_pic;
    xcb_pixmap_t root_tile, root_buffer;

    xcb_window_t overlay_win;
    xcb_render_picture_t overlay_win_buffer_pic;
    xcb_pixmap_t overlay_win_buffer;

    xcb_gcontext_t gc; /* Used to mantain clip region. */

    uint8_t effects_win_avail;
    xcb_xfixes_region_t bad_region;
} _this;

/* Add rectangles to the damaged area to be redrawn. */
static void _add_damage(xcb_rectangle_t *r, uint32_t nr) {
    xcb_xfixes_region_t tmp_region;

    tmp_region = xcb_generate_id(gd.conn);
    xcb_xfixes_create_region(gd.conn, tmp_region, nr, r);
    if (_this.bad_region == XCB_NONE) {
        _this.bad_region = tmp_region;
    } else {
        xcb_xfixes_union_region(gd.conn, tmp_region, _this.bad_region, _this.bad_region);
        xcb_xfixes_destroy_region(gd.conn, tmp_region);
    }
}

/* Ok this is shit, redraw the whole root. But the project deadline is near... */
static void _draw_windows_bottom_up() {
    xcb_pixmap_t win_pixmap;
    fwm_mwin_t *bottom;
    xcb_void_cookie_t c;

    bottom = gd.ws->top->next;
    xcb_xfixes_set_gc_clip_region(gd.conn, _this.gc, _this.bad_region, 0, 0);
    xcb_copy_area(gd.conn,
            _this.root_tile, _this.root_buffer,
            _this.gc,
            0, 0,
            0, 0,
            gd.def_screen->width_in_pixels,
            gd.def_screen->height_in_pixels
            );

    do {
        if (bottom->rect.x + bottom->rect.width < 1 || bottom->rect.y + bottom->rect.height < 1 ||
                bottom->rect.x >= gd.def_screen->width_in_pixels ||
                bottom->rect.y >= gd.def_screen->height_in_pixels ||
                bottom->rect.width <= 1 || bottom->rect.width <= 1) {
            FWM_ERR("CONTINUO");
            bottom = bottom->next;
            if (bottom != gd.ws->top->next) {
                continue;
            } else {
                break;
            }
        }
        win_pixmap = xcb_generate_id(gd.conn);
        xcb_composite_name_window_pixmap(gd.conn, bottom->win, win_pixmap);
        xcb_copy_area(gd.conn,
                win_pixmap, _this.root_buffer,
                _this.gc,
                0, 0, // TODO è qui che servono i dati aggiornati, fare gli handler nel core!!!
                bottom->rect.x, bottom->rect.y,
                bottom->rect.width, bottom->rect.height
                );
        xcb_free_pixmap(gd.conn, win_pixmap);
       // printf("DISEGNATA %s (%d)\n", bottom->name, bottom->win);
        fflush(stdout);
        bottom = bottom->next;
    } while (bottom != gd.ws->top->next);
#ifdef _COMP_MANUAL
    xcb_copy_area(gd.conn,
            _this.root_buffer, gd.def_screen->root,
            _this.gc,
            0, 0,
            0, 0,
            gd.def_screen->width_in_pixels,
            gd.def_screen->height_in_pixels
            );
#endif
}

/* Handle damage notify repainting intersted area. */
static int _handle_damage_notify(xcb_damage_notify_event_t *e) {
//    FWM_ERR("### damage notify in composite.c");
    if (e->drawable != _this.root_buffer) {
        _add_damage(&e->geometry, 1);
    }
    return FWM_EVENT_SUCCESS;
}

/* Handle map notify to print out a mapped window. */
static int _handle_map_notify(xcb_map_notify_event_t *e) {
    fwm_mwin_t *mwin;
    FWM_ERR("### map notify in composite.c");
    mwin = fwm_wt_get(gd.wt, e->window);
    if (mwin != NULL) {
        printf("map: x %d, y %d, width %d, height %d", mwin->rect.x, mwin->rect.y, mwin->rect.width, mwin->rect.height);
        fflush(stdout);
        _add_damage(&mwin->rect, 1);
        /* TODO add fade...*/
    }
    return FWM_EVENT_SUCCESS;
}

/* Handle root expose events to damage intersted parts for future redraw. */
static int _handle_expose(xcb_expose_event_t *e) {
    xcb_rectangle_t r;
    if (/*!e->count && */e->window == gd.def_screen->root) {
        FWM_ERR("esposta la root!!");
        r.x = e->x;
        r.y = e->y;
        r.width = e->width;
        r.height = e->height;
        _add_damage(&r, 1);
    }
    return FWM_EVENT_SUCCESS;
}

/* Handle unmap notify to hide a window. */
static int _handle_unmap_notify(xcb_unmap_notify_event_t *e) {
    fwm_mwin_t *mwin;

    mwin = fwm_wt_get(gd.wt, e->window);
    if (mwin != NULL) {
        _add_damage(&mwin->rect, 1);
        /* TODO add fade...*/
    }
    return FWM_EVENT_SUCCESS;
}

/* Handle configure notify event to redraw intersted parts. */
static int _handle_configure_notify(xcb_configure_notify_event_t *e) {
    xcb_xfixes_region_t new_reg;
    fwm_mwin_t *mwin;

    mwin = fwm_wt_get(gd.wt, e->window);
    if (mwin == NULL) {
        return FWM_EVENT_FAILURE;
    }
    new_reg = xcb_generate_id(gd.conn);
    xcb_xfixes_create_region_from_window(gd.conn, new_reg, e->event, 0);
    xcb_xfixes_union_region(gd.conn, _this.bad_region, new_reg, _this.bad_region);
    xcb_xfixes_union_region(gd.conn, _this.bad_region, mwin->region, _this.bad_region);

    xcb_xfixes_destroy_region(gd.conn, mwin->region);
    mwin->region = new_reg;
    return FWM_EVENT_SUCCESS;
}

static void _init_root_tile_pic() {
    int i;
    uint8_t *data;
    xcb_get_property_cookie_t cookie;
    xcb_get_property_reply_t * reply;
    xcb_atom_t atom;
    const char *props[] = {"_XROOTPMAP_ID", "_XSETROOT_ID"};

    _this.root_tile = XCB_NONE;

    for (i = 0; i < 2; i++) {
        atom = xcb_atom_get(gd.conn, props[i]);
        cookie = xcb_get_property(gd.conn,
                0,
                gd.def_screen->root,
                atom,
                PIXMAP,
                0, sizeof (xcb_pixmap_t)
                );
        reply = xcb_get_property_reply(gd.conn, cookie, NULL);
        if (reply && reply->type == PIXMAP && reply->length == 1) {
            data = xcb_get_property_value(reply);
            _this.root_tile = *((xcb_pixmap_t *) data);
        }
        free(reply);
    }
    _this.root_tile_pic = fwm_render_create_picture(_this.root_tile, XCB_NONE, NULL);
}

xcb_window_t fwm_composite_get_effects_window() {
    xcb_composite_get_overlay_window_reply_t *overlay_win_reply;

    if (!_this.effects_win_avail) {
        return XCB_NONE;
    }
    overlay_win_reply = xcb_composite_get_overlay_window_reply(gd.conn,
            xcb_composite_get_overlay_window(gd.conn, gd.def_screen->root),
            NULL);
    _this.effects_win_avail = 0;
    return overlay_win_reply->overlay_win;
}

void fwm_composite_release_effects_window() {
    xcb_composite_release_overlay_window(gd.conn, gd.def_screen->root);
    xcb_flush(gd.conn);
    _this.effects_win_avail = 1;
}

const xcb_render_picture_t fwm_composite_get_root_buffer_pic() {
    return _this.root_buffer_pic;
}

const xcb_pixmap_t fwm_composite_get_root_buffer_pixmap() {
    return _this.root_buffer;
}

const xcb_render_picture_t fwm_composite_get_root_tile_pic() {
    return _this.root_tile_pic;
}

void fwm_composite_init_managed_win(fwm_mwin_t *managed_win) {
    uint32_t value_mask = XCB_RENDER_CP_SUBWINDOW_MODE;
    const uint32_t value_list[1] = {1};
#ifdef _COMP_MANUAL
    xcb_composite_redirect_window(gd.conn, managed_win->win, XCB_COMPOSITE_REDIRECT_MANUAL);
#else
    xcb_composite_redirect_window(gd.conn, managed_win->win, XCB_COMPOSITE_REDIRECT_AUTOMATIC);
#endif
    managed_win->opacity = 1.0;
    managed_win->pic = fwm_render_create_picture(
            managed_win->win,
            value_mask, value_list);
    managed_win->region = xcb_generate_id(gd.conn);
    xcb_xfixes_create_region_from_window(gd.conn, managed_win->region, managed_win->win, 0);
    xcb_flush(gd.conn);
}

/* segfaults :) i have no time to inspect why :P */

/*
static int _register() {
    xcb_window_t owner_win;
    xcb_atom_t cm_atom;
    static char *cm_atom_name = "_NET_WM_CM_Sxx";

    snprintf(cm_atom_name, sizeof (cm_atom_name), "_NET_WM_CM_S%d", gd.def_screen_index);
    cm_atom = (xcb_intern_atom_reply(gd.conn,
            xcb_intern_atom(gd.conn, 0, strlen(cm_atom_name), cm_atom_name),
            NULL))->atom;
    owner_win = (xcb_get_selection_owner_reply(gd.conn,
            xcb_get_selection_owner(gd.conn, cm_atom),
            NULL))->owner;
    if (owner_win != XCB_NONE) {
        FWM_ERR("another composite manager already running");
        return 0;
    }
    owner_win = xcb_generate_id(gd.conn);
    xcb_create_window(gd.conn, gd.def_screen->root_depth, owner_win,
            gd.def_screen->root, 0, 0, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
            gd.def_screen->root_visual, 0, NULL);
    xcb_set_wm_name(gd.conn, owner_win, XAutf8, strlen(PACKAGE), PACKAGE);
    xcb_set_wm_icon_name(gd.conn, owner_win, utf8, strlen(PACKAGE), PACKAGE);
    xcb_set_selection_owner(gd.conn, owner_win, cm_atom, XCB_TIME_CURRENT_TIME);
    return 1;
}
 */

void fwm_composite_init() {
    uint32_t m;
    xcb_params_gc_t p;
    xcb_rectangle_t r;
    const xcb_query_extension_reply_t *query_ext_reply;

    query_ext_reply = xcb_get_extension_data(gd.conn, &xcb_composite_id);
    if (!query_ext_reply->present) {
        FWM_ERR("composite extension not found");
        return 0;
    }
    if (0/*!_register()*/) {
        FWM_ERR("cannot become the composite manager");
        return 0;
    }

    _init_root_tile_pic();
    /* Init an empty root buffer (only bg tile). */
    _this.root_buffer = xcb_generate_id(gd.conn);
    xcb_create_pixmap(gd.conn, gd.def_screen->root_depth, _this.root_buffer, gd.def_screen->root, gd.def_screen->width_in_pixels, gd.def_screen->height_in_pixels);
    /* Root buffer damage if needed (for example from the zoom plugin). */
    xcb_damage_create(gd.conn, xcb_generate_id(gd.conn), _this.root_buffer, XCB_DAMAGE_REPORT_LEVEL_NON_EMPTY);
    /* Our private gc, to mantain the clip area/region. */
    m = 0;
    _this.gc = xcb_generate_id(gd.conn);
    XCB_AUX_ADD_PARAM(&m, &p, background, gd.def_screen->black_pixel);
    XCB_AUX_ADD_PARAM(&m, &p, foreground, gd.def_screen->white_pixel);
    XCB_AUX_ADD_PARAM(&m, &p, graphics_exposures, 0);
    xcb_aux_create_gc(gd.conn, _this.gc, gd.def_screen->root, m, &p);

    /* Create and fill root buffer with the root tile. */
    _this.root_buffer_pic = fwm_render_create_picture(_this.root_buffer, XCB_NONE, NULL);
    /* Draw windows on the buffer a first time. */
    r.x = r.y = 0;
    r.width = gd.def_screen->width_in_pixels;
    r.height = gd.def_screen->height_in_pixels;
    _this.bad_region = XCB_NONE;
    _add_damage(&r, 1);
    _draw_windows_bottom_up();
    xcb_flush(gd.conn);

    _this.effects_win_avail = 1;
    /* Event handlers stuff. */
    fwm_event_set_root_mask(XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY);

    fwm_event_push_expose_handler(_handle_expose, FWM_EVENT_HANDLER_PRIORITY_BEFORE);
    fwm_event_push_map_notify_handler(_handle_map_notify, FWM_EVENT_HANDLER_PRIORITY_BEFORE);
    fwm_event_push_unmap_notify_handler(_handle_unmap_notify, FWM_EVENT_HANDLER_PRIORITY_AFTER);
    fwm_event_push_configure_notify_handler(_handle_configure_notify, FWM_EVENT_HANDLER_PRIORITY_AFTER);
    fwm_event_push_damage_notify_handler(_handle_damage_notify, FWM_EVENT_HANDLER_PRIORITY_BEFORE);

    FWM_MSG("composite plugin successfully initialized");
}

/* To be executed after the events queue has been emptied. */
void fwm_composite_finalize() {
    if (_this.bad_region != XCB_NONE) {
        _draw_windows_bottom_up();
        xcb_xfixes_destroy_region(gd.conn, _this.bad_region);
        _this.bad_region = XCB_NONE;
    }
}
