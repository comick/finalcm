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

#ifndef _DAMAGE_H
#define	_DAMAGE_H

#include <xcb/damage.h>

#include "finalwm.h"
#include "event.h"

void fwm_event_push_damage_notify_handler(int (*handler)(xcb_damage_notify_event_t *), fwm_event_handler_priority_t priority);
int fwm_damage_init();
void fwm_damage_init_managed_win(fwm_mwin_t *managed_win);

#endif
