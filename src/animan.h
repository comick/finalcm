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

#ifndef _ANIMAN_H
#define	_ANIMAN_H

#include "managed_win.h"

int fwm_animan_init(uint8_t fps);

int fwm_animan_shutdown();

void fwm_animan_insert(void *func, void *param);

void fwm_animan_append_action(void (*handler) (void *), void *param, uint8_t msec_delta);

#endif

