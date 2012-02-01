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

#include "wt.h"

#define _WT_HEAD(WT, K) (&(WT->elements[K % WT->size]))

static void _fwm_free_list(fwm_node_t* n) {
    n->next != NULL ? _fwm_free_list(n->next) : free(n);
}

fwm_wt_t *fwm_wt_init(uintmax_t size) {
    fwm_wt_t *wt = malloc(sizeof (fwm_wt_t));
    int i;
    wt->size = size;
    wt->elements = malloc(sizeof (fwm_head_t) * size);
    for (i = 0; i < size; i++) {
        wt->elements[i].first = NULL;
    }
    return wt;
}

void fwm_wt_free(fwm_wt_t *wt) {
    fwm_head_t *tmp_head;
    int i;
    for (i = 0; i < wt->size; tmp_head = wt->elements + sizeof (fwm_head_t) * i++) {
        if (tmp_head->first != NULL) {
            _fwm_free_list(tmp_head->first);
        }
    }
    free(wt->elements);
    free(wt);
}

int fwm_wt_put(fwm_wt_t *wt, xcb_window_t k, fwm_mwin_t *managed_win) {
    fwm_node_t *node = NULL;
    fwm_head_t *head = _WT_HEAD(wt, k);
    fwm_node_t *tmp_node;
    int found = 0;
    if (head->first == NULL) {
        head->first = node = malloc(sizeof (fwm_node_t));
    } else {
        tmp_node = head->first;
        while (!found && node == NULL) {
            if (tmp_node->key == k) {
                found++;
            } else if (tmp_node->next == NULL) {
                tmp_node->next = node = malloc(sizeof (fwm_node_t));
            }
            tmp_node = tmp_node->next;
        }
    }
    if (found) {
        return found;
    }
    /* Update node. */
    node->next = NULL;
    node->key = k;
    node->managed_win = managed_win;
    return found;
}

fwm_mwin_t *fwm_wt_get(fwm_wt_t *wt, xcb_window_t k) {
    fwm_node_t *node = _WT_HEAD(wt, k)->first;
    fwm_mwin_t *managed_win = NULL;
    while (managed_win == NULL && node != NULL) {
        if (node->key == k) {
            managed_win = node->managed_win;
        }
        node = node->next;
    }
    return managed_win;
}

fwm_mwin_t *fwm_wt_remove(fwm_wt_t *wt, xcb_window_t k) {
    fwm_head_t *head = _WT_HEAD(wt, k);
    fwm_node_t *pred = NULL;
    fwm_node_t *node = head->first;
    fwm_mwin_t *managed_win = NULL;
    while (managed_win == NULL && node != NULL) {
        if (node->key == k) {
            if (pred == NULL) { // Found the only element.
                head->first = NULL;
            } else { // Found the seconthir... element having a predecessor.
                pred->next = node->next;
            }
            managed_win = node->managed_win;
            free(node);
            break;
        } else {
            pred = node;
            node = node->next;
        }
    }
    return managed_win;
}
