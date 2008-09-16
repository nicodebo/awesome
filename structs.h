/*
 * structs.h - basic structs header
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef AWESOME_STRUCTS_H
#define AWESOME_STRUCTS_H

#include <xcb/xcb_event.h>
#include <ev.h>

#include "lua.h"
#include "layout.h"
#include "common/xutil.h"
#include "common/draw.h"
#include "common/swindow.h"
#include "common/xscreen.h"
#include "common/xembed.h"
#include "common/refcount.h"

/** Stacking layout layers */
typedef enum
{
    LAYER_DESKTOP = 1,
    LAYER_BELOW,
    LAYER_TILE,
    LAYER_FLOAT,
    LAYER_ABOVE,
    LAYER_FULLSCREEN,
    LAYER_MODAL,
    LAYER_OUTOFSPACE
} layer_t;

/** Cursors */
enum
{
    CurNormal, CurResize, CurResizeH, CurResizeV, CurMove,
    CurTopLeft, CurTopRight, CurBotLeft, CurBotRight, CurLast
};

typedef struct button_t button_t;
typedef struct widget_t widget_t;
typedef struct widget_node_t widget_node_t;
typedef struct statusbar_t statusbar_t;
typedef struct client_t client_t;
typedef struct titlebar_t titlebar_t;
typedef struct client_node_t client_node_t;
typedef struct _tag_t tag_t;
typedef struct tag_client_node_t tag_client_node_t;
typedef area_t (floating_placement_t)(client_t *);
typedef widget_t *(widget_constructor_t)(alignment_t);
typedef void (widget_destructor_t)(widget_t *);
typedef struct awesome_t awesome_t;

/** Mouse buttons bindings */
struct button_t
{
    /** Ref count */
    int refcount;
    /** Key modifiers */
    unsigned long mod;
    /** Mouse button number */
    unsigned int button;
    /** Lua function to execute. */
    luaA_ref fct;
    /** Next and previous buttons */
    button_t *prev, *next;
};

DO_SLIST(button_t, button, p_delete)
DO_RCNT(button_t, button, p_delete)

/** Widget */
struct widget_t
{
    /** Ref count */
    int refcount;
    /** widget_t name */
    char *name;
    /** Widget type is constructor */
    widget_constructor_t *type;
    /** Widget destructor */
    widget_destructor_t *destructor;
    /** Widget detach function */
    void (*detach)(widget_t *, void *);
    /** Draw function */
    int (*draw)(draw_context_t *, int, widget_node_t *, int, int, void *, awesome_type_t);
    /** Index function */
    int (*index)(lua_State *, awesome_token_t);
    /** Newindex function */
    int (*newindex)(lua_State *, awesome_token_t);
    /** ButtonPressedEvent handler */
    void (*button_press)(widget_node_t *, xcb_button_press_event_t *, int, void *, awesome_type_t);
    /** Alignement */
    alignment_t align;
    /** Misc private data */
    void *data;
    /** Button bindings */
    button_t *buttons;
    /** Cache flags */
    int cache_flags;
    /** True if the widget is visible */
    bool isvisible;
};

/** Delete a widget structure.
 * \param widget The widget to destroy.
 */
static inline void
widget_delete(widget_t **widget)
{
    if((*widget)->destructor)
        (*widget)->destructor(*widget);
    button_list_wipe(&(*widget)->buttons);
    p_delete(&(*widget)->name);
    p_delete(widget);
}

DO_RCNT(widget_t, widget, widget_delete)

struct widget_node_t
{
    /** The widget */
    widget_t *widget;
    /** The area where the widget was drawn */
    area_t area;
    /** Next and previous widget in the list */
    widget_node_t *prev, *next;
};

/** Delete a widget node structure.
 * \param node The node to destroy.
 */
static inline void
widget_node_delete(widget_node_t **node)
{
    widget_unref(&(*node)->widget);
    p_delete(node);
}

DO_SLIST(widget_node_t, widget_node, widget_node_delete)

/** Titlebar template structure */
struct titlebar_t
{
    /** Ref count */
    int refcount;
    /** Position */
    position_t position, oldposition;
    /** Alignment on window */
    alignment_t align;
    /** Widgets */
    widget_node_t *widgets;
    /** Width and height */
    int width, height;
    /** Titlebar window */
    simple_window_t *sw;
    /** Default colors */
    struct
    {
        xcolor_t fg, bg;
    } colors;
    /** Border */
    struct
    {
        xcolor_t color;
        int width;
    } border;
    /** Need update */
    bool need_update;
};

/** Delete a titlebar structure.
 * \param t The titlebar to destroy.
 */
static inline void
titlebar_delete(titlebar_t **t)
{
    widget_node_list_wipe(&(*t)->widgets);
    p_delete(t);
}

DO_RCNT(titlebar_t, titlebar, titlebar_delete)

/** Status bar */
struct statusbar_t
{
    /** Ref count */
    int refcount;
    /** Window */
    simple_window_t *sw;
    /** statusbar_t name */
    char *name;
    /** Bar width */
    int width;
    /** Bar height */
    int height;
    /** True if user specified width */
    bool width_user;
    /** Bar position */
    position_t position;
    /** Alignment */
    alignment_t align;
    /** Screen */
    int screen;
    /** Physical screen id */
    int phys_screen;
    /** Widget list */
    widget_node_t *widgets;
    /** Draw context */
    draw_context_t *ctx;
    /** Need update */
    bool need_update;
    /** Default colors */
    struct
    {
        xcolor_t fg, bg;
    } colors;
    /** Next and previous statusbars */
    statusbar_t *prev, *next;
};

/** Netwm icon */
typedef struct
{
    int height;
    int width;
    unsigned char *image;
} netwm_icon_t;

/** client_t type */
struct client_t
{
    /** Ref counter */
    int refcount;
    /** Valid, or not ? */
    bool invalid;
    /** Client name */
    char *name;
    /** Window geometry */
    area_t geometry;
    /** Floating window geometry */
    area_t f_geometry;
    /** Max window geometry */
    area_t m_geometry;
    /* Size hints */
    int basew, baseh, incw, inch, maxw, maxh, minw, minh;
    int minax, maxax, minay, maxay;
    bool hassizehints;
    /** Respect resize hints */
    bool honorsizehints;
    int border, oldborder;
    xcolor_t border_color;
    /** True if the client does not want any border */
    bool noborder;
    /** Has urgency hint */
    bool isurgent;
    /** Store previous floating state before maximizing */
    bool wasfloating;
    /** true if the window is floating */
    bool isfloating;
    /** true if the window is fixed */
    bool isfixed;
    /** true if the window is maximized */
    bool ismax;
    /** true if the client must be skipped from client list */
    bool skip;
    /** true if the client is moving */
    bool ismoving;
    /** True if the client is hidden */
    bool ishidden;
    /** true if the client must be skipped from task bar client list */
    bool skiptb;
    /** Window of the client */
    xcb_window_t win;
    /** Client logical screen */
    int screen;
    /** Client physical screen */
    int phys_screen;
    /** Layer in the stacking order */
    layer_t layer, oldlayer;
    /** Path to an icon */
    char *icon_path;
    /** Titlebar */
    titlebar_t *titlebar;
    /** Button bindings */
    button_t *buttons;
    /** Floating window placement algo */
    floating_placement_t *floating_placement;
    /** Icon */
    netwm_icon_t *icon;
    /** Next and previous clients */
    client_t *prev, *next;
};

static void
client_delete(client_t **c)
{
    button_list_wipe(&(*c)->buttons);
    p_delete(&(*c)->icon_path);
    p_delete(&(*c)->name);
    p_delete(c);
}

DO_ARRAY(client_t *, client, DO_NOTHING)
DO_RCNT(client_t, client, client_delete)

struct client_node_t
{
    /** The client */
    client_t *client;
    /** Next and previous client_nodes */
    client_node_t *prev, *next;
};

/** Tag type */
struct _tag_t
{
    /** Ref count */
    int refcount;
    /** Tag name */
    char *name;
    /** Screen */
    int screen;
    /** true if selected */
    bool selected;
    /** Current tag layout */
    layout_t *layout;
    /** Master width factor */
    double mwfact;
    /** Number of master windows */
    int nmaster;
    /** Number of columns in tile layout */
    int ncol;
    /** clients in this tag */
    client_array_t clients;
};
ARRAY_TYPE(tag_t *, tag)

/** Padding type */
typedef struct
{
    /** Padding at top */
    int top;
    /** Padding at bottom */
    int bottom;
    /** Padding at left */
    int left;
    /** Padding at right */
    int right;
} padding_t;

typedef struct
{
    /** Screen index */
    int index;
    /** Screen geometry */
    area_t geometry;
    /** true if we need to arrange() */
    bool need_arrange;
    /** Tag list */
    tag_array_t tags;
    /** Status bar */
    statusbar_t *statusbar;
    /** Padding */
    padding_t padding;
    /** Window that contains the systray */
    struct
    {
        xcb_window_t window;
        /** Systray window parent */
        xcb_window_t parent;
    } systray;
    /** Focused client */
    client_t *client_focus;
} screen_t;

/** Main configuration structure */
struct awesome_t
{
    /** Connection ref */
    xcb_connection_t *connection;
    /** Event and error handlers */
    xcb_event_handlers_t evenths;
    /** Default screen number */
    int default_screen;
    /** Keys symbol table */
    xcb_key_symbols_t *keysyms;
    /** Logical screens */
    screen_t *screens;
    /** Screens info */
    screens_info_t *screens_info;
    /** Mouse bindings list */
    struct
    {
           button_t *root;
    } buttons;
    /** Numlock mask */
    unsigned int numlockmask;
    /** Numlock mask */
    unsigned int shiftlockmask;
    /** Numlock mask */
    unsigned int capslockmask;
    /** Check for XRandR extension */
    bool have_randr;
    /** Cursors */
    xcb_cursor_t cursor[CurLast];
    /** Clients list */
    client_t *clients;
    /** Embedded windows */
    xembed_window_t *embedded;
    /** Path to config file */
    char *configpath;
    /** Stack client history */
    client_node_t *stack;
    /** Command line passed to awesome */
    char *argv;
    /** Last XMotionEvent coords */
    int pointer_x, pointer_y;
    /** Lua VM state */
    lua_State *L;
    /** Default colors */
    struct
    {
        xcolor_t fg, bg;
    } colors;
    /** Default font */
    font_t *font;
    struct
    {
        /** Command to execute when spawning a new client */
        luaA_ref manage;
        /** Command to execute when unmanaging client */
        luaA_ref unmanage;
        /** Command to execute when giving focus to a client */
        luaA_ref focus;
        /** Command to execute when removing focus to a client */
        luaA_ref unfocus;
        /** Command to run when mouse is over */
        luaA_ref mouseover;
        /** Command to run on arrange */
        luaA_ref arrange;
        /** Command to run on title change */
        luaA_ref titleupdate;
        /** Command to run on urgent flag */
        luaA_ref urgent;
        /** Command to run on time */
        luaA_ref timer;
    } hooks;
    /** The event loop */
    struct ev_loop *loop;
    /** The timeout after which we need to stop select() */
    struct ev_timer timer;
    /** The key grabber function */
    luaA_ref keygrabber;
    /** Focused screen */
    screen_t *screen_focus;
};

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
