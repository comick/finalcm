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
#include <unistd.h>
#include <stdlib.h>

#include <xcb/xcb_event.h>
#include <string.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_icccm.h>
#include <xcb/composite.h>
#include <xcb/xcb_property.h>

#include "xfixes.h"
#include "damage.h"
#include "expose.h"
#include "property.h"
#include "event.h"
#include "finalwm.h"
#include "window.h"
#include "animan.h"
#include "coverswitch.h"
#include "damage.h"
#include "composite.h"
#include "pointer.h"
#include "zoom.h"

#include "../config.h"
#include "atom.h"

fwm_data_t gd;

void fwm_check_error(xcb_void_cookie_t c) {
    xcb_generic_error_t *e;
    FWM_ERR("mi chiamano");
    e = xcb_request_check(gd.conn, c);
    if (e != NULL) {
        printf("Error (sequence %d): %s\n", c.sequence, xcb_event_get_error_label(e->error_code));
    } else {
        printf("Request (sequence %d) OK.\n", c.sequence);
    }
    fflush(stdout);
}

/* Iterates through top level windows and hopefully manage them. */
static void fwm_manage_existing_windows() {
    xcb_get_property_reply_t *rep;
    xcb_window_t *wins;
    int wins_len;
    xcb_get_window_attributes_cookie_t *wattr_cookies;
    fwm_wattr_t attr;
    int i;
    fwm_mwin_t *mwin;
    xcb_query_tree_reply_t *tree;

    rep = xcb_get_property_reply(gd.conn,
            xcb_get_property(gd.conn, 0, gd.def_screen->root, fwm_atom_client_list_stacking, XCB_ATOM_WINDOW, 0, FWM_PROPERTY_MAX_LEN),
            NULL);
    wins = (xcb_window_t *) xcb_get_property_value(rep);
    wins_len = xcb_get_property_value_length(rep);
    /* Send requests to clients taking advantage from xcb asyncronous model. */
    wattr_cookies = malloc(sizeof (xcb_get_window_attributes_cookie_t) * wins_len);

    for (i = 0; i < wins_len; ++i) {
        tree = xcb_query_tree_reply(gd.conn, xcb_query_tree(gd.conn, wins[i]),NULL);
        if (tree != NULL) {
            wins[i] = tree->parent;
        }
        wattr_cookies[i] = xcb_get_window_attributes(gd.conn, wins[i]);
    }
    for (i = 0; i < wins_len; ++i) {
        attr.tag = TAG_COOKIE;
        attr.value.cookie = wattr_cookies[i];
        mwin = fwm_window_manage(wins[i], attr, NULL, XCB_NONE);
    }
    free(wattr_cookies);
}

int main(int argc, char** argv) {
    uint32_t m;
    xcb_params_gc_t p;
    char *display_name;
    xcb_screen_iterator_t screen_iterator;

    /* Connect to the X server and fills global data. */
    display_name = (argc == 1) ? argv[1] : NULL;
    gd.conn = xcb_connect(display_name, &(gd.def_screen_index));
    if (xcb_connection_has_error(gd.conn)) {
        FWM_ERR("unable to connect to the X server");
        exit(EXIT_FAILURE);
    }
    /* Not real world software :P care only about default screen. */
    screen_iterator = xcb_setup_roots_iterator(xcb_get_setup(gd.conn));
    gd.def_screen = &(screen_iterator.data[gd.def_screen_index]);
    gd.default_gc = xcb_generate_id(gd.conn);
    gd.win_count = 0;
    /* Prepare a default gc. */
    m = 0;
    XCB_AUX_ADD_PARAM(&m, &p, background, gd.def_screen->black_pixel);
    XCB_AUX_ADD_PARAM(&m, &p, foreground, gd.def_screen->white_pixel);
    XCB_AUX_ADD_PARAM(&m, &p, graphics_exposures, 0);
    xcb_aux_create_gc(gd.conn, gd.default_gc, gd.def_screen->root, m, &p);
    /* Init event handling stuff. */
    fwm_event_init(&gd);
    /* Init properties handlers table. */
    xcb_property_handlers_init(&(gd.prophs), &(gd.evenths));
    /* Set property handlers. */
    // TODO aggiungere il nome finestra cambiato handler
    xcb_property_set_default_handler(&(gd.prophs), FWM_PROPERTY_MAX_LEN, fwm_property_handle_default, NULL);
    gd.wt = fwm_wt_init(FWM_WT_DEFAULT_SIZE);
    gd.ws = fwm_ws_init();

    xcb_flush(gd.conn);

    /* Init plugin stuff. */
    fwm_xfixes_init();
    fwm_pointer_init();
    fwm_animan_init(40);
    fwm_damage_init();
    /* Never put these before animan init. */
    fwm_expose_init();
    fwm_coverswitch_init();
    fwm_zoom_init();
    //   fwm_opacity_init();

    xcb_grab_server(gd.conn);
    fwm_atom_init();
    fwm_manage_existing_windows();
    xcb_property_set_handler(&(gd.prophs), fwm_atom_client_list_stacking, FWM_PROPERTY_MAX_LEN, fwm_handle_client_list_stacking_ghange, NULL);
    xcb_property_changed(&(gd.prophs), XCB_PROPERTY_NEW_VALUE, gd.def_screen->root, fwm_atom_client_list_stacking);
    fwm_event_set_root_mask(XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_STRUCTURE_NOTIFY |
            XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_EXPOSURE);

    /* Core related event handlers. */
    fwm_event_push_create_notify_handler(fwm_handle_create_notify, FWM_EVENT_HANDLER_PRIORITY_BEFORE);
    fwm_event_push_configure_notify_handler(fwm_handle_configure_notify, FWM_EVENT_HANDLER_PRIORITY_BEFORE);
    fwm_event_push_destroy_notify_handler(fwm_handle_destroy_notify, FWM_EVENT_HANDLER_PRIORITY_BEFORE);
    fwm_event_push_circulate_notify_handler(fwm_handle_circulate_notify, FWM_EVENT_HANDLER_PRIORITY_BEFORE);

    xcb_ungrab_server(gd.conn);

    /* Here, need the existing windows inited. */
    fwm_composite_init();

    /* Main loop. */
    xcb_event_wait_for_event_loop(&(gd.evenths));

    fwm_animan_shutdown();
    /* TODO mettere shutdown per le altre cose. */

    xcb_disconnect(gd.conn);
    FWM_MSG("I've done my hard work, see you later... :)");
    return (EXIT_SUCCESS);
}
