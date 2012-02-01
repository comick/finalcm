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
#include <assert.h>

#include <xcb/xcb.h>
#include <string.h>

#include "pointer.h"
#include "event.h"
#include "window.h"
#include "composite.h"

#define _QUEUE_SIZE 126

typedef struct fwm_eq_node {
    fwm_event_handler_t handler;
    struct fwm_eq_node *next;
    fwm_event_handler_priority_t priority;
} fwm_eq_node_t;

typedef struct fwm_eq_head {
    fwm_eq_node_t *first;
    fwm_eq_node_t *last;
} fwm_eq_head_t;

struct {
    uint32_t root_event_mask;
    xcb_event_handlers_t *evenths;
    fwm_eq_head_t * evenths_queue[_QUEUE_SIZE];
} _this;

/* Handle a single event executing the handlers within its queue.
 * Prints an event dumb in case of empty queue. */
static void _handle(xcb_generic_event_t *e) {
    uint8_t sendEvent;
    uint16_t seq_num;
    fwm_eq_node_t *next_node;
    uint8_t response_type;
    fwm_eq_head_t *head;
    xcb_generic_error_t *er;

    /* Iterate queued handlers, if any. */
    response_type = XCB_EVENT_RESPONSE_TYPE(e);
    if (response_type) {
        head = _this.evenths_queue[response_type - 2];
        if (head != NULL) {
            next_node = head->first;
            while (next_node != NULL) {
                ((fwm_event_handler_t) (next_node->handler))(e);
                next_node = next_node->next;
            }
        }
    } else {
        /* Or format event if not to be handled. */
        sendEvent = response_type ? 1 : 0;
        seq_num = *((uint16_t *) e + 1);
        switch (response_type) {
            case 0:
                er = (xcb_generic_error_t *) e;
                printf("Error (%s) on sequence number %d.\n",
                        xcb_event_get_error_label(er->error_code), seq_num);
                break;
            default:
                printf("Unhandler event (%s) following seqnum %d.\n",
                        xcb_event_get_label(response_type), seq_num);
                break;
        }
        fflush(stdout);
    }
}

/* This implements something like an Xlib.
 * do{
 *   threat_event(e);
 * } while (!Qlength);
 * It is a good way to optimize compositing performance.
 */
static int _handle_until_empty_queue(void *data, xcb_connection_t *conn, xcb_generic_event_t *e) {
    do {
        _handle(e);
        e = xcb_poll_for_event(gd.conn);
    } while (e != NULL);
    /* To be something hookable.. */
    fwm_composite_finalize();
    /* stop */
    xcb_flush(gd.conn);
    return 1;
}

/* Push an event handler in the queue to be executed for the specified event. */
void fwm_event_push_handler(int kind, fwm_event_handler_t handler, fwm_event_handler_priority_t priority) {
    fwm_eq_head_t *head;
    fwm_eq_node_t *tmp_node;
    assert(_this.evenths != NULL);
    tmp_node = malloc(sizeof (fwm_eq_node_t));
    tmp_node->handler = handler;
    tmp_node->next = NULL;
    tmp_node->priority = priority;
    if (_this.evenths_queue[kind - 2] == NULL) {
        head = malloc(sizeof (fwm_eq_head_t));
        head->first = head->last = tmp_node;
        _this.evenths_queue[kind - 2] = head;
    } else {
        head = _this.evenths_queue[kind - 2];
        switch (priority) {
            case FWM_EVENT_HANDLER_PRIORITY_BEFORE:
                tmp_node->next = head->first;
                head->first = tmp_node;
                break;
            case FWM_EVENT_HANDLER_PRIORITY_AFTER:
                head->last->next = tmp_node;
                head->last = tmp_node;
                break;
        }
    }
}

void fwm_event_error_set_catch_all(xcb_generic_error_handler_t error_handler) {
    int i;
    for (i = 0; i < 256; i++) {
        xcb_event_set_error_handler(&(gd.evenths), i, error_handler, NULL);
    }
}

/* Subscribe to some more event from the root window. */
void fwm_event_set_root_mask(uint32_t mask) {
    _this.root_event_mask |= mask;
    xcb_change_window_attributes(gd.conn, gd.def_screen->root, XCB_CW_EVENT_MASK, &_this.root_event_mask);
    xcb_flush(gd.conn);
}

/* Get the present event mask of the root window. */
const uint32_t fwm_event_root_mask_get() {
    return (const uint32_t) _this.root_event_mask;
}

void fwm_event_root_mask_remove(uint32_t mask) {
    _this.root_event_mask &= ~mask;
    xcb_change_window_attributes(gd.conn, gd.def_screen->root, XCB_CW_EVENT_MASK, &_this.root_event_mask);
    xcb_flush(gd.conn);
}

/* Init the events handling queue setting default handler. */
void fwm_event_init() {
    int i;
    _this.evenths = &(gd.evenths);
    xcb_event_handlers_init(gd.conn, _this.evenths);
    for (i = 0; i < _QUEUE_SIZE; i++) {
        xcb_event_set_handler(_this.evenths, i + 2, _handle_until_empty_queue, NULL);
        _this.evenths_queue[i] = NULL;
    }
    FWM_MSG("event handling stuff initializated");
}
