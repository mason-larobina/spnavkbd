/*
 * spnavkbd.c - 3d Connexion Space-Navigator to keyboard emulator
 *
 * Copyright © 2012 Mason Larobina <mason.larobina@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>

#include <X11/Xlib.h>
#include <spnav.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

XKeyEvent event;

void
sig(int s)
{
    spnav_close();
    exit(0);
}

bool
lua_loadrc(lua_State *L, const char *confpath)
{
    printf("loading rc: %s\n", confpath);
    if (!luaL_loadfile(L, confpath)) {
        if (lua_pcall(L, 0, LUA_MULTRET, 0))
            fprintf(stderr, "%s\n", lua_tostring(L, -1));
        else
            return true;
    } else
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
    return false;
}

bool
lua_dofunction(lua_State *L, const char *fname, int nargs)
{
    lua_getglobal(L, fname);
    if (!lua_isfunction(L, -1)) {
        fprintf(stderr, "'%s' not a function", fname);
        lua_pop(L, 1);
        return false;
    }

    /* move function above args */
    lua_insert(L, -(nargs+1));

    if (!lua_pcall(L, nargs, 0, 0))
        return true;

    fprintf(stderr, "%s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
    return false;
}

/* return current time with nanosecond precision (overkill) */
int
lua_gettime(lua_State *L)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    lua_pushnumber(L, ts.tv_sec + (ts.tv_nsec / 1e9));
    return 1;
}

int
lua_send_keyev(lua_State *L)
{
    /* event.window = currently focused window */
    Window win_focus;
    int revert;
    XGetInputFocus(event.display, &win_focus, &revert);
    event.window = win_focus;

    event.keycode = luaL_checkint(L, 1);
    event.state = !lua_isnoneornil(L, 2) ? luaL_checkint(L, 2) : 0;

    //printf("send_keyev (%4d, %4d)\n", event.keycode, event.state);

    event.type = KeyPress;
    XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent*)&event);
    event.type = KeyRelease;
    XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent*)&event);

    return 0;
}

void
lua_init(lua_State **L)
{
    *L = luaL_newstate();
    luaL_openlibs(*L);

    lua_pushcfunction(*L, lua_gettime);
    lua_setglobal(*L, "gettime");

    lua_pushcfunction(*L, lua_send_keyev);
    lua_setglobal(*L, "send_keyev");

    if (!lua_loadrc(*L, "spnavkbd.lua"))
        exit(1);
}

int main(void)
{
    Display *dpy;
    Window win;
    unsigned long bpix;

    spnav_event sev;

    signal(SIGINT, sig);

    if(!(dpy = XOpenDisplay(0))) {
        fprintf(stderr, "failed to connect to the X server\n");
        return 1;
    }

    bpix = BlackPixel(dpy, DefaultScreen(dpy));
    win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1, 1, 0, bpix, bpix);

    /* This actually registers our window with the driver for receiving
     * motion/button events through the 3dxsrv-compatible X11 protocol.
     */
    if(spnav_x11_open(dpy, win) == -1) {
        fprintf(stderr, "failed to connect to the space navigator daemon\n");
        return 1;
    }

    /* setup key event struct */
    event.display = dpy;
    event.root = XDefaultRootWindow(dpy);
    event.subwindow = None;
    event.time = CurrentTime;
    event.x = 1;
    event.y = 1;
    event.x_root = 1;
    event.y_root = 1;
    event.same_screen = True;
    event.state = 0;

    /* setup lua state & load users config */
    lua_State *L;
    lua_init(&L);

    while(spnav_wait_event(&sev)) {
        if(sev.type == SPNAV_EVENT_MOTION) {
            lua_pushnumber(L, sev.motion.x);
            lua_pushnumber(L, sev.motion.y);
            lua_pushnumber(L, sev.motion.z);
            lua_pushnumber(L, sev.motion.rx);
            lua_pushnumber(L, sev.motion.ry);
            lua_pushnumber(L, sev.motion.rz);
            lua_dofunction(L, "motion_event", 6);

        } else { /* SPNAV_EVENT_BUTTON */
            lua_pushstring(L, (sev.button.press ? "press" : "release"));
            lua_pushnumber(L, sev.button.bnum);
            lua_dofunction(L, "button_event", 2);
        }
    }

    spnav_close();
    return 0;
}
