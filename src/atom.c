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
#include <string.h>

#include <xcb/xcb_atom.h>

#include "finalwm.h"
#include "atom.h"

#define _WIN_TYPE "_NET_WM_WINDOW_TYPE"
#define _WIN_TYPE_DESKTOP "_NET_WM_WINDOW_TYPE_DESKTOP"
#define _WIN_TYPE_DOCK "_NET_WM_WINDOW_TYPE_DOCK"
#define _WIN_TYPE_TOOLBAR "_NET_WM_WINDOW_TYPE_TOOLBAR"
#define _WIN_TYPE_MENU "_NET_WM_WINDOW_TYPE_MENU"
#define _WIN_TYPE_UTILITY "_NET_WM_WINDOW_TYPE_UTILITY"
#define _WIN_TYPE_SPLASH "_NET_WM_WINDOW_TYPE_SPLASH"
#define _WIN_TYPE_DIALOG "_NET_WM_WINDOW_TYPE_DIALOG"
#define _WIN_TYPE_NORMAL "_NET_WM_WINDOW_TYPE_NORMAL"


#define _CLIENT_LIST_STACKING "_NET_CLIENT_LIST_STACKING"
#define _RESTACK_WINDOW "_NET_RESTACK_WINDOW"

xcb_atom_t fwm_atom_win_type = XCB_NONE;
xcb_atom_t fwm_atom_win_type_desktop = XCB_NONE;
xcb_atom_t fwm_atom_win_type_dock = XCB_NONE;
xcb_atom_t fwm_atom_win_type_toolbar = XCB_NONE;
xcb_atom_t fwm_atom_win_type_menu = XCB_NONE;
xcb_atom_t fwm_atom_win_type_util = XCB_NONE;
xcb_atom_t fwm_atom_win_type_splash = XCB_NONE;
xcb_atom_t fwm_atom_win_type_dialog = XCB_NONE;
xcb_atom_t fwm_atom_win_type_normal = XCB_NONE;

xcb_atom_t fwm_atom_client_list_stacking = XCB_NONE;
xcb_atom_t fwm_atom_restack_window =XCB_NONE;

void fwm_atom_init() {
    xcb_atom_fast_cookie_t c[16];

    c[0] = xcb_atom_get_fast(gd.conn, 1, strlen(_WIN_TYPE), _WIN_TYPE);
    c[1] = xcb_atom_get_fast(gd.conn, 1, strlen(_WIN_TYPE_DESKTOP), _WIN_TYPE_DESKTOP);
    c[2] = xcb_atom_get_fast(gd.conn, 1, strlen(_WIN_TYPE_DOCK), _WIN_TYPE_DOCK);
    c[3] = xcb_atom_get_fast(gd.conn, 1, strlen(_WIN_TYPE_TOOLBAR), _WIN_TYPE_TOOLBAR);
    c[4] = xcb_atom_get_fast(gd.conn, 1, strlen(_WIN_TYPE_MENU), _WIN_TYPE_MENU);
    c[5] = xcb_atom_get_fast(gd.conn, 1, strlen(_WIN_TYPE_UTILITY), _WIN_TYPE_UTILITY);
    c[6] = xcb_atom_get_fast(gd.conn, 1, strlen(_WIN_TYPE_SPLASH), _WIN_TYPE_SPLASH);
    c[7] = xcb_atom_get_fast(gd.conn, 1, strlen(_WIN_TYPE_DIALOG), _WIN_TYPE_DIALOG);
    c[8] = xcb_atom_get_fast(gd.conn, 1, strlen(_WIN_TYPE_NORMAL), _WIN_TYPE_NORMAL);


    c[9] = xcb_atom_get_fast(gd.conn, 1, strlen(_CLIENT_LIST_STACKING), _CLIENT_LIST_STACKING);
    c[10] = xcb_atom_get_fast(gd.conn, 1, strlen(_RESTACK_WINDOW), _RESTACK_WINDOW);

    fwm_atom_win_type = xcb_atom_get_fast_reply(gd.conn, c[0], NULL);
    printf("_WIN_TYPE: %d\n", fwm_atom_win_type);

    fwm_atom_win_type_desktop = xcb_atom_get_fast_reply(gd.conn, c[1], NULL);
    printf("_WIN_TYPE_DESKTOP: %d\n", fwm_atom_win_type_desktop);

    fwm_atom_win_type_dock = xcb_atom_get_fast_reply(gd.conn, c[2], NULL);
    printf("_WIN_TYPE_DOCK: %d\n", fwm_atom_win_type_dock);

    fwm_atom_win_type_toolbar = xcb_atom_get_fast_reply(gd.conn, c[3], NULL);
    printf("_WIN_TYPE_TOOLBAR: %d\n", fwm_atom_win_type_toolbar);

    fwm_atom_win_type_menu = xcb_atom_get_fast_reply(gd.conn, c[4], NULL);
    printf("_WIN_TYPE_MENU: %d\n", fwm_atom_win_type_menu);

    fwm_atom_win_type_util = xcb_atom_get_fast_reply(gd.conn, c[5], NULL);
    printf("_WIN_TYPE_UTIL: %d\n", fwm_atom_win_type_util);

    fwm_atom_win_type_splash = xcb_atom_get_fast_reply(gd.conn, c[6], NULL);
    printf("_WIN_TYPE_SPLASH: %d\n", fwm_atom_win_type_splash);

    fwm_atom_win_type_dialog = xcb_atom_get_fast_reply(gd.conn, c[7], NULL);
    printf("_WIN_TYPE_DIALOG: %d\n", fwm_atom_win_type_dialog);

    fwm_atom_win_type_normal = xcb_atom_get_fast_reply(gd.conn, c[8], NULL);
    printf("_WIN_TYPE_NORMAL: %d\n", fwm_atom_win_type_normal);


    fwm_atom_client_list_stacking = xcb_atom_get_fast_reply(gd.conn, c[9], NULL);
    printf("_CLIENT_LIST: %d\n", fwm_atom_client_list_stacking);

    fwm_atom_restack_window = xcb_atom_get_fast_reply(gd.conn, c[10], NULL);
    printf("_RESTACK_WINDOW: %d\n", fwm_atom_restack_window);
    FWM_MSG("atom stuff initializated");
}
