// Based on TinyWM, written by Nick Welch <nick@incise.org> in 2005 & 2011.

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>

#include <cairo.h>
#include <cairo-xlib.h>

#define MOD Mod4Mask

#define MAX(a, b) ((a) > (b) ? (a) : (b))

Display *dpy;
XVisualInfo vinfo;

cairo_t *latest_cr;

Atom WM_PROTOCOLS;
Atom WM_DELETE_WINDOW;

Bool other_wm = False;
int other_wm_handler(Display *dpy, XErrorEvent *e)
{
	if (e->error_code == BadAccess) other_wm = True;
	return 0;
}

void raise_win(Window w)  {
	XRaiseWindow(dpy, w);
	XSetInputFocus(dpy, w, RevertToParent, CurrentTime);
}

void delete_win(Window w) {
	XEvent ev;
	ev.xclient.type = ClientMessage;
	ev.xclient.message_type = WM_PROTOCOLS;
	ev.xclient.display = dpy;
	ev.xclient.window = w;
	ev.xclient.format = 32; // ???
	ev.xclient.data.l[0] = WM_DELETE_WINDOW;
	ev.xclient.data.l[1] = CurrentTime;

	XSendEvent(dpy, w, False, 0, &ev);
}


void add_corner(Window win) {
	XSetWindowAttributes attrs;
	attrs.override_redirect = 1;

	attrs.colormap = XCreateColormap(dpy, win, vinfo.visual, AllocNone);
	attrs.background_pixel = 0;
	attrs.border_pixel = 0;


	Window overlay = XCreateWindow(
			dpy, win,
			0, 0, 20, 20, 0,
			vinfo.depth, InputOutput,
			vinfo.visual,
			CWOverrideRedirect | CWColormap | CWBackPixel | CWBorderPixel, &attrs
			);


	// ignore input
	XserverRegion region = XFixesCreateRegion (dpy, NULL, 0);
	XFixesSetWindowShapeRegion (dpy, overlay, ShapeBounding, 0, 0, 0);
	XFixesSetWindowShapeRegion (dpy, overlay, ShapeInput, 0, 0, region);
	XFixesDestroyRegion (dpy, region);


	XMapWindow(dpy, overlay);

	const int sz = 20;

	Pixmap shape = XCreatePixmap(dpy, overlay, sz, sz, 1);

	cairo_surface_t* surf = cairo_xlib_surface_create(dpy, overlay,
			vinfo.visual,
			sz, sz);
	cairo_surface_t *surfs = cairo_xlib_surface_create_for_bitmap(dpy, shape,
			DefaultScreenOfDisplay(dpy),
			sz, sz);
	cairo_t* cr = cairo_create(surf);
	cairo_t *crs = cairo_create(surfs);

	latest_cr = cr;

	cairo_set_operator(crs, CAIRO_OPERATOR_CLEAR);
	cairo_rectangle(crs, 0, 0, sz, sz);
	cairo_fill(crs);

	cairo_set_source_rgba(cr, 1.0, 0.0, 1.0, 1.0);
	cairo_set_operator(crs, CAIRO_OPERATOR_OVER);

	cairo_move_to(cr, 0, 0);
	cairo_move_to(crs, 0, 0);

	cairo_rel_line_to(cr, sz, 0);
	cairo_rel_line_to(crs, sz, 0);
	cairo_rel_line_to(cr, -sz, sz);
	cairo_rel_line_to(crs, -sz, sz);

	cairo_fill(cr);
	cairo_fill(crs);

	XShapeCombineMask(dpy, overlay, ShapeBounding, 0, 0,
			cairo_xlib_surface_get_drawable(surfs), ShapeSet);

	XFlush(dpy);

	// TODO: take care of this eventually?

	// cairo_destroy(cr);
	// cairo_destroy(crs);
	// cairo_surface_destroy(surf);
	// cairo_surface_destroy(surfs);
	// XUnmapWindow(d, overlay);
}



void on_map_request(XMapRequestEvent *e) {
	XMapWindow(dpy, e->window);
	add_corner(e->window);
}

void on_configure_request(XConfigureRequestEvent *e) {
	XWindowChanges c;
	c.x = e->x;
	c.y = e->y;
	c.width = e->width;
	c.height = e->height;
	c.border_width = e->border_width;
	c.sibling = e->above;
	c.stack_mode = e->detail;

	XConfigureWindow(dpy, e->window, e->value_mask, &c);
}

int main(void)
{
	if (!(dpy = XOpenDisplay(0x0))) return 1;

	WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);
	WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

	Window root = DefaultRootWindow(dpy);

	// become a wm
	XSetErrorHandler(other_wm_handler);
	XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask);
	XSync(dpy, False);
	if (other_wm) return 2;

	if (!XMatchVisualInfo(dpy, DefaultScreen(dpy), 32, TrueColor, &vinfo)) {
		return 3;
	}

	XGrabButton(dpy, 1, MOD, root, True,
			ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
			GrabModeAsync, GrabModeAsync, None, None);

	XGrabButton(dpy, 3, MOD, root, True,
			ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
			GrabModeAsync, GrabModeAsync, None, None);

	XGrabKey(dpy, XKeysymToKeycode(dpy, XK_q), MOD | ShiftMask,
			root, True, GrabModeAsync,
			GrabModeAsync);

	XWindowAttributes attr;
	XButtonEvent start;
	start.subwindow = None;

	for(;;)
	{
		XEvent ev;
		XNextEvent(dpy, &ev);
		KeySym keysym;
		int x;
		int y;

		switch (ev.type) {
			case KeyPress:
				keysym = XKeycodeToKeysym(dpy, ev.xkey.keycode, 0);

				if (ev.xkey.state == (MOD | ShiftMask)) {
					if (keysym == XK_q) {
						delete_win(ev.xkey.subwindow);
						break;
					}
				}

				break;
			case ButtonPress:
				if (!ev.xbutton.subwindow) continue;
				raise_win(ev.xbutton.subwindow);

				start = ev.xbutton;
				XGetWindowAttributes(dpy, ev.xbutton.subwindow, &attr);
				x = ev.xbutton.x - attr.x;
				y = ev.xbutton.y - attr.y;
				if (x+y < 20) {
					printf("docking widgets should appear\n");
					cairo_t *cr = latest_cr;
					cairo_set_source_rgba(cr, 0.0, 1.0, 1.0, 1.0);
					cairo_move_to(cr, 0, 0);
					cairo_rel_line_to(cr, 20, 0);
					cairo_rel_line_to(cr, -20, 20);
					cairo_fill(cr);
				}
				break;
			case ButtonRelease:
				XGetWindowAttributes(dpy, ev.xbutton.subwindow, &attr);
				x = ev.xbutton.x - attr.x;
				y = ev.xbutton.y - attr.y;
				if (x+y < 20) {
					printf("docking widgets should be gone\n");
					cairo_t *cr = latest_cr;
					cairo_set_source_rgba(cr, 1.0, 0.0, 1.0, 1.0);
					cairo_move_to(cr, 0, 0);
					cairo_rel_line_to(cr, 20, 0);
					cairo_rel_line_to(cr, -20, 20);
					cairo_fill(cr);
				}
				start.subwindow = None;
				break;
			case MotionNotify:
				if (!start.subwindow) continue;
				int xdiff = ev.xbutton.x_root - start.x_root;
				int ydiff = ev.xbutton.y_root - start.y_root;

				XMoveResizeWindow(dpy, start.subwindow,
						attr.x + (start.button==1 ? xdiff : 0),
						attr.y + (start.button==1 ? ydiff : 0),
						MAX(1, attr.width  + (start.button==3 ? xdiff : 0)),
						MAX(1, attr.height + (start.button==3 ? ydiff : 0)));
				break;
			case CreateNotify:
				break; // TODO ?
			case DestroyNotify:
				break; // TODO ?
			case UnmapNotify:
				break; // TODO ?
			case MapNotify:
				raise_win(ev.xmap.window);
				break;
			case MapRequest:
				on_map_request(&ev.xmaprequest);
				break;
			case ConfigureNotify:
				break; // TODO ?
			case ConfigureRequest:
				on_configure_request(&ev.xconfigurerequest);
				break;
			default:
				printf("unhandled event, type %d\n", ev.type);
		}
	}
}

