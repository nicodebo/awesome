/*
 * tasklist.c - task list widget
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
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

#include <stdio.h>
#include "util.h"
#include "widget.h"
#include "client.h"
#include "focus.h"
#include "xutil.h"
#include "screen.h"
#include "event.h"
#include "ewmh.h"
#include "rules.h"

#define ISVISIBLE_ON_TB(c, screen) (client_isvisible(c, screen) && !c->skiptb)

extern AwesomeConf globalconf;

typedef struct
{
    Bool show_icons;
    Alignment align;
    XColor fg_sel;
    XColor bg_sel;
    XColor fg;
    XColor bg;
} Data;

static int
tasklist_draw(Widget *widget, DrawCtx *ctx, int offset, int used)
{
    Client *c;
    Data *d = widget->data;
    Client *sel = focus_get_current_client(widget->statusbar->screen);
    Rule *r;
    Area area;
    int n = 0, i = 0, box_width = 0, icon_width = 0;
    NetWMIcon *icon;

    for(c = globalconf.clients; c; c = c->next)
        if(ISVISIBLE_ON_TB(c, widget->statusbar->screen))
            n++;
    
    if(!n)
    {
        widget->area.width = 0;
        return widget->area.width;
    }

    box_width = (widget->statusbar->width - used) / n;

    if(!widget->user_supplied_x)
        widget->area.x = widget_calculate_offset(widget->statusbar->width,
                                                 0,
                                                 offset,
                                                 widget->alignment);

    if(!widget->user_supplied_y)
        widget->area.y = widget->area.y = 0;

    for(c = globalconf.clients; c; c = c->next)
        if(ISVISIBLE_ON_TB(c, widget->statusbar->screen))
        {
            icon_width = 0;

            if(d->show_icons)
            {
                for(r = globalconf.rules; r; r = r->next)
                    if(r->icon && client_match_rule(c, r))
                    {
                        area = draw_get_image_size(r->icon);
                        icon_width = ((double) widget->statusbar->height / (double) area.height) * area.width;
                        draw_image(ctx,
                                   widget->area.x + box_width * i,
                                   widget->area.y,
                                   widget->statusbar->height,
                                   r->icon);
                    }

                if(!icon_width && (icon = ewmh_get_window_icon(c->win)))
                {
                    icon_width = ((double) widget->statusbar->height / (double) icon->height)
                        * icon->width;
                    draw_image_from_argb_data(ctx,
                                              widget->area.x + box_width * i,
                                              widget->area.y,
                                              icon->width, icon->height,
                                              widget->statusbar->height, icon->image);
                    p_delete(&icon->image);
                    p_delete(&icon);
                }
            }

            if(sel == c)
            {
                draw_text(ctx, widget->area.x + icon_width + box_width * i,
                          widget->area.y,
                          box_width - icon_width,
                          widget->statusbar->height,
                          d->align,
                          widget->font->height / 2, widget->font, c->name,
                          d->fg_sel, d->bg_sel);
            }
            else
                draw_text(ctx, widget->area.x + icon_width + box_width * i,
                          widget->area.y,
                          box_width - icon_width,
                          widget->statusbar->height,
                          d->align,
                          widget->font->height / 2, widget->font, c->name,
                          d->fg, d->bg);
            if(c->isfloating)
                draw_circle(ctx, widget->area.x + icon_width + box_width * i,
                            widget->area.y,
                            (widget->font->height + 2) / 4,
                            c->ismax, d->fg);
            i++;
        }

    widget->area.width = widget->statusbar->width - used;
    widget->area.height = widget->statusbar->height;

    return widget->area.width;
}

static void
tasklist_button_press(Widget *widget, XButtonPressedEvent *ev)
{
    Button *b;
    Client *c;
    int n = 0, box_width = 0, i = 0, ci = 0;

    /* button1 give focus */
    if(ev->button == Button1 && CLEANMASK(ev->state) == NoSymbol)
    {
        for(c = globalconf.clients; c; c = c->next)
            if(ISVISIBLE_ON_TB(c, widget->statusbar->screen))
                n++;
    
        if(!n)
            return;

        box_width = widget->area.width / n;

        if(ev->button == Button1 && CLEANMASK(ev->state) == NoSymbol)
        {
            switch(widget->statusbar->position)
            {
              case Top:
              case Bottom:
                ci = (ev->x - widget->area.x) / box_width;
                break;
              case Right:
                ci = (ev->y - widget->area.x) / box_width;
                break;
              default:
                ci = ((widget->statusbar->width - ev->y) - widget->area.x) / box_width;
                break;
            }
            /* found first visible client */
            for(c = globalconf.clients;
                c && !ISVISIBLE_ON_TB(c, widget->statusbar->screen);
                c = c->next);
            /* found ci-th visible client */
            for(; c && i < ci; c = c->next)
                if(ISVISIBLE_ON_TB(c, widget->statusbar->screen))
                    i++;

            focus(c, True, widget->statusbar->screen);
            return;
        }
    }

    for(b = widget->buttons; b; b = b->next)
        if(ev->button == b->button && CLEANMASK(ev->state) == b->mod && b->func)
        {
            b->func(widget->statusbar->screen, b->arg);
            return;
        }
}

Widget *
tasklist_new(Statusbar *statusbar, cfg_t *config)
{
    Widget *w;
    Data *d;
    char *buf;
    int phys_screen = get_phys_screen(statusbar->screen);

    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);
    w->draw = tasklist_draw;
    w->button_press = tasklist_button_press;
    w->alignment = AlignFlex;
    w->data = d = p_new(Data, 1);

    if((buf = cfg_getstr(config, "fg")))
        d->fg = initxcolor(phys_screen, buf);
    else
        d->fg = globalconf.screens[statusbar->screen].colors_normal[ColFG];

    if((buf = cfg_getstr(config, "bg")))
        d->bg = initxcolor(phys_screen, buf);
    else
        d->bg = globalconf.screens[statusbar->screen].colors_normal[ColBG];

    if((buf = cfg_getstr(config, "focus_bg")))
        d->bg_sel = initxcolor(phys_screen, buf);
    else
        d->bg_sel = globalconf.screens[statusbar->screen].colors_selected[ColBG];

    if((buf = cfg_getstr(config, "focus_fg")))
        d->fg_sel = initxcolor(phys_screen, buf);
    else
        d->fg_sel = globalconf.screens[statusbar->screen].colors_selected[ColFG];

    d->align = draw_get_align(cfg_getstr(config, "align"));
    d->show_icons = cfg_getbool(config, "show_icons");

    if((buf = cfg_getstr(config, "font")))
        w->font = XftFontOpenName(globalconf.display, get_phys_screen(statusbar->screen), buf);

    if(!w->font)
        w->font = globalconf.screens[statusbar->screen].font;

    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
