/*
 * nullcursor - simple utility which makes X11 root window cursor invisible
 *
 * Copyright © 2010 Mikhail Gusarov <dottedmag@dottedmag.net>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <err.h>
#include <stdlib.h>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_aux.h>

static void
err_xcb(int retcode, const char *msg, xcb_generic_error_t *xerr)
{
    err(retcode, "%s: %d:%d", msg, xerr->minor_code, xerr->major_code);
}

static xcb_pixmap_t
create_1x1x1_pixmap(xcb_connection_t *conn, xcb_screen_t *screen)
{
    uint32_t pixmap_id = xcb_generate_id(conn);

    xcb_void_cookie_t c
        = xcb_create_pixmap(conn, 1, pixmap_id, screen->root, 1, 1);

    xcb_generic_error_t *e = xcb_request_check(conn, c);
    if (e)
        err_xcb(1, "xcb_create_pixmap", e);

    return pixmap_id;
}

static xcb_gc_t
create_gc_for_pixmap(xcb_connection_t *conn, xcb_pixmap_t pixmap)
{
    xcb_gc_t gc_id = xcb_generate_id(conn);

    xcb_void_cookie_t c = xcb_create_gc (conn, gc_id, pixmap, 0, 0);

    xcb_generic_error_t *e = xcb_request_check(conn, c);
    if (e)
        err_xcb(1, "xcb_create_gc", e);

    return gc_id;
}

static void
fill_pixmap(xcb_connection_t *conn, xcb_pixmap_t pixmap,
            xcb_gc_t gc, uint32_t pixel)
{
    xcb_void_cookie_t c = xcb_put_image(conn, XCB_IMAGE_FORMAT_XY_BITMAP,
                                        pixmap, gc, 1, 1, 0, 0, 0, 1, 4,
                                        (uint8_t*)&pixel);

    xcb_generic_error_t* e = xcb_request_check(conn, c);
    if (e)
        err_xcb(1, "xcb_put_image", e);
}

static xcb_rgb_t
query_white_color(xcb_connection_t *conn, xcb_screen_t *screen)
{
    uint32_t pixels[1] = { screen->white_pixel };

    xcb_query_colors_cookie_t qcc =
        xcb_query_colors(conn, screen->default_colormap, 1, pixels);

    xcb_generic_error_t *e;
    xcb_query_colors_reply_t *colors_rep =
        xcb_query_colors_reply(conn, qcc, &e);
    if (e)
        err_xcb(1, "xcb_query_colors", e);

    return xcb_query_colors_colors(colors_rep)[0];
}

static xcb_cursor_t
create_null_cursor(xcb_connection_t *conn, xcb_screen_t *screen)
{
    xcb_pixmap_t pixmap = create_1x1x1_pixmap(conn, screen);
    xcb_gc_t gc = create_gc_for_pixmap(conn, pixmap);
    fill_pixmap(conn, pixmap, gc, screen->white_pixel);
    xcb_rgb_t color = query_white_color(conn, screen);

    uint32_t cursor_id = xcb_generate_id(conn);

    xcb_void_cookie_t c
        = xcb_create_cursor(conn, cursor_id, pixmap, pixmap,
                            color.red, color.green, color.blue,
                            color.red, color.green, color.blue, 0, 0);

    xcb_generic_error_t *e = xcb_request_check(conn, c);
    if (e)
        err_xcb(1, "xcb_create_cursor", e);

    xcb_free_gc_checked(conn, gc);
    xcb_free_pixmap_checked(conn, pixmap);

    return cursor_id;
}

static void
set_root_cursor(xcb_connection_t *conn, xcb_screen_t *screen,
                xcb_cursor_t cursor)
{
    uint32_t cw_mask = XCB_CW_CURSOR;
    uint32_t cw_list[] = { cursor };

    xcb_void_cookie_t c
        = xcb_change_window_attributes(conn, screen->root, cw_mask, cw_list);
    xcb_generic_error_t *e = xcb_request_check(conn, c);
    if (e)
        err_xcb(1, "xcb_change_window_attributes", e);
}

int
main(int argc, char *argv[])
{
    int default_screen;
    xcb_connection_t *conn;
    xcb_screen_t *screen;
    conn = xcb_connect(NULL, &default_screen);
    if (xcb_connection_has_error(conn))
        errx(1, "nullcursor: cannot open display %s", getenv("DISPLAY"));
    if (!(screen = xcb_aux_get_screen(conn, default_screen)))
        errx(1, "nullcursor: cannot obtain default screen");

    xcb_cursor_t cursor = create_null_cursor(conn, screen);
    set_root_cursor(conn, screen, cursor);
    xcb_free_cursor_checked(conn, cursor);

    xcb_disconnect(conn);
    return 0;
}
