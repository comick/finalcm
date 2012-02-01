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

#include <xcb/xcb_event.h>

#include "event.h"
#include "damage.h"
#include "finalwm.h"


static uint8_t _damage_event = 0;

void fwm_event_push_damage_notify_handler(int (*handler)(xcb_damage_notify_event_t *), fwm_event_handler_priority_t priority) {
    fwm_event_push_handler(_damage_event, (void *) handler, priority);
}

static int _handle_damage_notify(xcb_damage_notify_event_t *e) {
    xcb_damage_subtract(gd.conn, e->damage, 0, 0);
    xcb_flush(gd.conn);
    return 1;
}

int fwm_damage_init() {
    const xcb_query_extension_reply_t *query_ext_reply =
            xcb_get_extension_data(gd.conn, &xcb_damage_id);
    if (!query_ext_reply->present) {
        FWM_ERR("damage extension not found");
        return -1;
    }
    xcb_damage_query_version_cookie_t dcok = xcb_damage_query_version_unchecked(gd.conn, 1, 1);
    xcb_damage_query_version_reply_t *drep = xcb_damage_query_version_reply(gd.conn,
            dcok,
            NULL);
    if (drep) {
        FWM_MSG("damage plugin successfully initializated");
        free(drep);
    }
    _damage_event = query_ext_reply->first_event;
    fwm_event_push_damage_notify_handler(_handle_damage_notify, FWM_EVENT_HANDLER_PRIORITY_AFTER);
}

void fwm_damage_init_managed_win(fwm_mwin_t *mwin) {
    mwin->damage = xcb_generate_id(gd.conn);
    xcb_damage_create(gd.conn, mwin->damage, mwin->win, XCB_DAMAGE_REPORT_LEVEL_NON_EMPTY);
    xcb_flush(gd.conn);
}
