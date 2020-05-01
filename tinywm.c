// Based on TinyWM, written by Nick Welch <nick@incise.org> in 2005 & 2011.

#include <stdio.h>
#include <X11/Xlib.h>

#define MOD Mod4Mask

#define MAX(a, b) ((a) > (b) ? (a) : (b))

Display * dpy;

Bool other_wm = False;
int other_wm_handler(Display *dpy, XErrorEvent *e)
{
	if (e->error_code == BadAccess) other_wm = True;
	return 0;
}

void on_map_request(XMapRequestEvent *e) {
	XMapWindow(dpy, e->window);
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
	Window root = DefaultRootWindow(dpy);

	// become a wm
	XSetErrorHandler(other_wm_handler);
	XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask);
	XSync(dpy, False);
	if (other_wm) return 2;


	XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("F1")), MOD,
			root, True, GrabModeAsync,
			GrabModeAsync);
	XGrabButton(dpy, 1, MOD, root, True,
			ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
			GrabModeAsync, GrabModeAsync, None, None);
	XGrabButton(dpy, 3, MOD, root, True,
			ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
			GrabModeAsync, GrabModeAsync, None, None);

	XWindowAttributes attr;
	XButtonEvent start;
	start.subwindow = None;

	for(;;)
	{
		XEvent ev;
		XNextEvent(dpy, &ev);
		switch (ev.type) {
			case KeyPress:
				if (!ev.xkey.subwindow) continue;
				XRaiseWindow(dpy, ev.xkey.subwindow);
				break;
			case ButtonPress:
				if (!ev.xbutton.subwindow) continue;
				XGetWindowAttributes(dpy, ev.xbutton.subwindow, &attr);
				start = ev.xbutton;
				break;
			case ButtonRelease:
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
			case CreateNotify: break; // TODO ?
			case DestroyNotify: break; // TODO ?
			case UnmapNotify: break; // TODO ?
			case MapNotify: break; // TODO ?
			case MapRequest:
				on_map_request(&ev.xmaprequest);
				break;
			case ConfigureNotify: break; // TODO ?
			case ConfigureRequest:
				on_configure_request(&ev.xconfigurerequest);
				break;
			default:
				printf("unhandled event, type %d\n", ev.type);
		}
	}
}

