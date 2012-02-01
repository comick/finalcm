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
#include <stdlib.h>
#include <string.h>

#include "wt.h"
#include "property.h"
#include "window.h"
#include "finalwm.h"

int fwm_handle_wmname_change(void *data, xcb_connection_t *c, uint8_t state, xcb_window_t window, xcb_atom_t atom, xcb_get_property_reply_t *prop) {
    printf("WM_NAME change for 0x%08x: ", window);
    fwm_mwin_t *managed_window = fwm_wt_get(gd.wt, window);
    if (!managed_window) {
        printf("is not being managed.\n");
        return 0;
    }
    if (managed_window->name) {
        printf("was named \"%.*s\"; now ", managed_window->name_len, managed_window->name);
        free(managed_window->name);
    }
    if (!prop) {
        managed_window->name_len = 0;
        managed_window->name = NULL;
        printf("has no name.\n");
        return 1;
    }
    managed_window->name_len = xcb_get_property_value_length(prop);
    managed_window->name = malloc(managed_window->name_len);
    strncpy(managed_window->name, xcb_get_property_value(prop), managed_window->name_len);
    printf("is named \"%.*s\".\n", managed_window->name_len, managed_window->name);
    return 1;
}

/* Forse qua volevo vedere quando la root cambia sfondo... */
int fwm_property_handle_default(void *data, xcb_connection_t *c, uint8_t state, xcb_window_t window, xcb_atom_t atom, xcb_get_property_reply_t *prop) {
/*
    if (prop->type == PIXMAP) {
        FWM_ERR("cambiata prop ed è una pixmap\n");
    } else FWM_ERR("cambiata una prop e che cazzo è???");
*/
    return 1;
}

int fwm_handle_client_list_stacking_ghange(void *date, xcb_connection_t *c, uint8_t state, xcb_window_t window, xcb_atom_t atom, xcb_get_property_reply_t *prop){
    FWM_ERR("CAMBIATO LO STACKING ORDER E LE FINESTRE ETC ETC ETC!!!!");
}