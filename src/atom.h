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

#ifndef _ATOM_H
#define	_ATOM_H

#include <xcb/xcb.h>

extern xcb_atom_t fwm_atom_win_type;
extern xcb_atom_t fwm_atom_win_type_desktop;
extern xcb_atom_t fwm_atom_win_type_dock;
extern xcb_atom_t fwm_atom_win_type_toolbar;
extern xcb_atom_t fwm_atom_win_type_menu;
extern xcb_atom_t fwm_atom_win_type_util;
extern xcb_atom_t fwm_atom_win_type_splash;
extern xcb_atom_t fwm_atom_win_type_dialog;
extern xcb_atom_t fwm_atom_win_type_normal;

extern xcb_atom_t fwm_atom_client_list_stacking;
extern xcb_atom_t fwm_atom_restack_window;

void fwm_atom_init();

#endif
