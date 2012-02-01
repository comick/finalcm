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

#include <stdlib.h>
#include <stdio.h>

#include <xcb/composite.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_icccm.h>

#include "window.h"
#include "finalwm.h"
#include "ws.h"
#include "event.h"
#include "damage.h"
#include "pointer.h"
#include "composite.h"
#include "atom.h"

/* Try to manage a window. */
fwm_mwin_t *fwm_window_manage(xcb_window_t win, fwm_wattr_t wattr, xcb_rectangle_t *r, uint16_t border_width) {
    xcb_rectangle_t geom_r;
    fwm_mwin_t *mwin = NULL;
    xcb_get_geometry_reply_t *geom_reply = NULL;
    xcb_get_window_attributes_reply_t *wattr_reply = NULL;

    if (wattr.tag == TAG_COOKIE) {
        wattr_reply = xcb_get_window_attributes_reply(gd.conn, wattr.value.cookie, NULL);
        if (wattr_reply == NULL) {
            goto clean;
        }
        wattr.tag = TAG_VALUE;
        wattr.value.override_redirect = wattr_reply->override_redirect;
    }
    /* We do not manage ovverride redirect windows. */
    if (wattr.value.override_redirect) {
        goto clean;
    }
    /* Already managed. */
    if (!wattr.value.override_redirect && fwm_wt_get(gd.wt, win)) {
        goto clean;
    }
    if (r == NULL) {
        geom_reply = xcb_get_geometry_reply(gd.conn,
                xcb_get_geometry(gd.conn, win),
                NULL);
        geom_r.x = geom_reply->x;
        geom_r.y = geom_reply->y;
        geom_r.width = geom_reply->width;
        geom_r.height = geom_reply->height;
        border_width = geom_reply->border_width;
    } else {
        geom_r = *r;
    }
    mwin = fwm_window_prepare(win, geom_r, border_width);
    xcb_property_changed((xcb_property_handlers_t *)&(gd.prophs), XCB_PROPERTY_NEW_VALUE, win, WM_NAME);
clean:
    if (wattr_reply != NULL) {
        free(wattr_reply);
    }
    if (geom_reply != NULL) {
        free(geom_reply);
    }
    xcb_flush(gd.conn);
    return mwin;
}

/* Sets up the managed window structure. */
fwm_mwin_t *fwm_window_prepare(xcb_window_t win, xcb_rectangle_t geom_r, uint16_t border_width) {
    uint32_t mask = 0;
    uint32_t values[3];
    fwm_mwin_t *mwin;
    xcb_get_property_reply_t *pr;
    xcb_wm_hints_t wm_hints;
    xcb_get_text_property_reply_t wm_name;

    mwin = malloc(sizeof (fwm_mwin_t));
    pr = xcb_get_property_reply(gd.conn,
            xcb_get_property(gd.conn, 0, win, fwm_atom_win_type, ATOM, 0, 1),
            NULL);
    if (pr->response_type) {
        mwin->type = *((xcb_atom_t*) xcb_get_property_value(pr));
    } else {
        mwin->type = -1;
    }
    xcb_get_wm_hints_reply(gd.conn,
            xcb_get_wm_hints(gd.conn, win),
            &wm_hints,
            NULL);
    xcb_get_wm_name_reply(gd.conn, xcb_get_wm_name(gd.conn, win), &wm_name, NULL);

    mwin->state = wm_hints.initial_state;
    mwin->win = win;
    mwin->name = wm_name.name;
    mwin->name_len = wm_name.name_len;
    mwin->rect.height = geom_r.height;
    mwin->rect.width = geom_r.width;
    mwin->icon_pixmap = XCB_NONE;
    mwin->rect.x = geom_r.x;
    mwin->rect.y = geom_r.y;
    mwin->border_width = border_width;


    printf("wintype %d (%s) - ", mwin->type, mwin->name);
    printf("state %d\n\n", mwin->state);

    /* TODO to be hookable some day... */
    fwm_composite_init_managed_win(mwin);
    fwm_damage_init_managed_win(mwin);
    fwm_composite_init_managed_win(mwin);

    /* Update global structures. */
    fwm_wt_put(gd.wt, win, mwin);
    fwm_ws_push(gd.ws, mwin, FWM_WS_PUT_TOP);
    gd.win_count++;

    mask = XCB_CW_EVENT_MASK;
    // TODO qui forse andrebbero fuse con la mask originaria o comunque con quelle che ci servono
    values[0] = XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;
    xcb_change_window_attributes(gd.conn, win, mask, values);

    xcb_flush(gd.conn);
    return mwin;
}

/* Handle create notify. */
int fwm_handle_create_notify(xcb_create_notify_event_t *e) {
    fwm_mwin_t *mwin;
    fwm_wattr_t wattr;

    FWM_MSG("### create notify di window.c");
    mwin = fwm_wt_get(gd.wt, e->window);
    if (!mwin) {
        wattr.tag = TAG_COOKIE;
        wattr.value.cookie = xcb_get_window_attributes(gd.conn, e->window);
        mwin = fwm_window_manage(e->window, wattr, NULL, XCB_NONE);
    }
    xcb_flush(gd.conn);
    return 1;
}

/* Unmap a window if managed and sets unmapped. */
int fwm_handle_destroy_notify(xcb_destroy_notify_event_t *e) {
    fwm_mwin_t *mwin;

    FWM_ERR("### destroy notify in window.c");
    mwin = fwm_wt_remove(gd.wt, e->window);
    if (mwin != NULL) {
        fwm_ws_detach(gd.ws, mwin);
        xcb_flush(gd.conn);
        gd.win_count--;
        free(mwin);
    }
    return 1;
}

/* Handle circulate notify to reflect stacking order variations into internal data structures. */
int fwm_handle_circulate_notify(xcb_circulate_notify_event_t *e) {
    fwm_mwin_t *mwin;

    FWM_ERR("hendlo circulate notify");
    mwin = fwm_wt_get(gd.wt, e->window);
    fwm_window_stack_above(mwin);
    return FWM_EVENT_SUCCESS;
}

/* Handle configure notify updating internal data structures. */
int fwm_handle_configure_notify(xcb_configure_notify_event_t *e) {
    fwm_mwin_t *mwin;

    mwin = fwm_wt_get(gd.wt, e->window);
    if (mwin != NULL) {
        mwin->rect.x = e->x;
        mwin->rect.y = e->y;
        mwin->rect.width = e->width;
        mwin->rect.height = e->height;
        mwin->border_width = e->border_width;
    }
    return FWM_EVENT_SUCCESS;
}

/* Stack a window above and refresh internal structures. */
void fwm_window_stack_above(fwm_mwin_t *mwin) {
    uint16_t m = 0;
    xcb_params_configure_window_t p;

    if (mwin != gd.ws->top) {
        mwin->next->prev = mwin->prev;
        mwin->prev->next = mwin->next;

        mwin->prev = gd.ws->top;
        mwin->next = gd.ws->top->next;

        gd.ws->top->next->prev = mwin;
        gd.ws->top->next = mwin;

        gd.ws->top = mwin;

        XCB_AUX_ADD_PARAM(&m, &p, stack_mode, XCB_STACK_MODE_ABOVE);
        xcb_aux_configure_window(gd.conn, mwin->win, m, &p);
        xcb_set_input_focus(gd.conn, XCB_INPUT_FOCUS_NONE, mwin->win, XCB_CURRENT_TIME);
    }
}
