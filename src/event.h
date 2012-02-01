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

#ifndef _EVENT_H
#define	_EVENT_H

#include <xcb/xcb_event.h>

#include "finalwm.h"

#define FWM_EVENT_SUCCESS 1
#define FWM_EVENT_FAILURE 0

typedef enum {
    FWM_EVENT_HANDLER_PRIORITY_BEFORE,
    FWM_EVENT_HANDLER_PRIORITY_AFTER
} fwm_event_handler_priority_t;

typedef int (*fwm_event_handler_t)(void *data);

void fwm_event_init();

/* Callbacks handling procedures and resources. */
void fwm_event_push_handler(int type, fwm_event_handler_t handler, fwm_event_handler_priority_t priority);
#define FWM_EVENT_PUSH_HANDLER(lkind, ukind) \
static inline void fwm_event_push_##lkind##_handler(int (*handler)(xcb_##lkind##_event_t *), fwm_event_handler_priority_t priority) \
{ \
    fwm_event_push_handler(XCB_##ukind, (fwm_event_handler_t)handler, priority); \
}

FWM_EVENT_PUSH_HANDLER(key_press, KEY_PRESS)
FWM_EVENT_PUSH_HANDLER(key_release, KEY_RELEASE)
FWM_EVENT_PUSH_HANDLER(button_press, BUTTON_PRESS)
FWM_EVENT_PUSH_HANDLER(button_release, BUTTON_RELEASE)
FWM_EVENT_PUSH_HANDLER(motion_notify, MOTION_NOTIFY)
FWM_EVENT_PUSH_HANDLER(enter_notify, ENTER_NOTIFY)
FWM_EVENT_PUSH_HANDLER(leave_notify, LEAVE_NOTIFY)
FWM_EVENT_PUSH_HANDLER(focus_in, FOCUS_IN)
FWM_EVENT_PUSH_HANDLER(focus_out, FOCUS_OUT)
FWM_EVENT_PUSH_HANDLER(keymap_notify, KEYMAP_NOTIFY)
FWM_EVENT_PUSH_HANDLER(expose, EXPOSE)
FWM_EVENT_PUSH_HANDLER(graphics_exposure, GRAPHICS_EXPOSURE)
FWM_EVENT_PUSH_HANDLER(no_exposure, NO_EXPOSURE)
FWM_EVENT_PUSH_HANDLER(visibility_notify, VISIBILITY_NOTIFY)
FWM_EVENT_PUSH_HANDLER(create_notify, CREATE_NOTIFY)
FWM_EVENT_PUSH_HANDLER(destroy_notify, DESTROY_NOTIFY)
FWM_EVENT_PUSH_HANDLER(unmap_notify, UNMAP_NOTIFY)
FWM_EVENT_PUSH_HANDLER(map_notify, MAP_NOTIFY)
FWM_EVENT_PUSH_HANDLER(map_request, MAP_REQUEST)
FWM_EVENT_PUSH_HANDLER(reparent_notify, REPARENT_NOTIFY)
FWM_EVENT_PUSH_HANDLER(configure_notify, CONFIGURE_NOTIFY)
FWM_EVENT_PUSH_HANDLER(configure_request, CONFIGURE_REQUEST)
FWM_EVENT_PUSH_HANDLER(gravity_notify, GRAVITY_NOTIFY)
FWM_EVENT_PUSH_HANDLER(resize_request, RESIZE_REQUEST)
FWM_EVENT_PUSH_HANDLER(circulate_notify, CIRCULATE_NOTIFY)
FWM_EVENT_PUSH_HANDLER(circulate_request, CIRCULATE_REQUEST)
FWM_EVENT_PUSH_HANDLER(property_notify, PROPERTY_NOTIFY)
FWM_EVENT_PUSH_HANDLER(selection_clear, SELECTION_CLEAR)
FWM_EVENT_PUSH_HANDLER(selection_request, SELECTION_REQUEST)
FWM_EVENT_PUSH_HANDLER(selection_notify, SELECTION_NOTIFY)
FWM_EVENT_PUSH_HANDLER(colormap_notify, COLORMAP_NOTIFY)
FWM_EVENT_PUSH_HANDLER(client_message, CLIENT_MESSAGE)
FWM_EVENT_PUSH_HANDLER(mapping_notify, MAPPING_NOTIFY)

/* To set an handler for any possible generic error event, */
void fwm_event_error_set_catch_all(xcb_generic_error_handler_t error_handler);

void fwm_event_set_root_mask(uint32_t mask);
void fwm_event_root_mask_remove(uint32_t mask);
const uint32_t fwm_event_root_mask_get();

#endif
