// Includes
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>

// X11 globals
static Display *display;
static Window root, focused;
static Atom net_wm_state, fullscreen_atom, active_atom, close_atom, supported_atom;

static int screen_width, screen_height;
static int startup_done = 0;

// Ignore X errors from clients
static int on_x_error(Display *dpy, XErrorEvent *e)
{
	(void)dpy;
	(void)e;
	return 0;
}

// Exit if the server goes away
static int on_io_error(Display *dpy)
{
	(void)dpy;
	exit(1);
}

// Check if a window is manageable
static int is_manageable_window(Window window, XWindowAttributes *attributes)
{
	if (window == None) return 0;
	if (!XGetWindowAttributes(display, window, attributes)) return 0;

	return !attributes->override_redirect && attributes->class != InputOnly;
}

// Make a window fullscreen
static void fullscreen(Window window)
{
	unsigned long window_state = fullscreen_atom;

	XChangeProperty(display, window, net_wm_state, XA_ATOM, 32, PropModeReplace, (unsigned char *)&window_state, 1);

	XSetWindowBorderWidth(display, window, 0);
	XMoveResizeWindow(display, window, 0, 0, screen_width, screen_height);
}

// Focus a window and raise it
static void focus(Window window)
{
	XWindowAttributes attributes;
	unsigned long window_id = window;

	if (!is_manageable_window(window, &attributes)) return;
	if (attributes.map_state != IsViewable) return;

	XRaiseWindow(display, window);
	XSetInputFocus(display, window, RevertToParent, CurrentTime);

	XChangeProperty(display, root, active_atom, XA_WINDOW, 32, PropModeReplace, (unsigned char *)&window_id, 1);

	focused = window;
}

// Focus the topmost visible window, or PointerRoot if none
static void focus_top(void)
{
	Window tree_root, parent, *windows = NULL;
	unsigned int count = 0;

	focused = None;
	XDeleteProperty(display, root, active_atom);

	// XQueryTree returns bottom-to-top, so walk backwards
	if (XQueryTree(display, root, &tree_root, &parent, &windows, &count)) {
		for (unsigned int i = count; i-- > 0; ) {
			XWindowAttributes attributes;

			if (is_manageable_window(windows[i], &attributes) && attributes.map_state == IsViewable) {
				Window top = windows[i];

				XFree(windows);
				focus(top);

				return;
			}
		}
		if (windows) XFree(windows);
	}

	XSetInputFocus(display, PointerRoot, RevertToPointerRoot, CurrentTime);
}

int main(void)
{
	XEvent event;
	Window tree_root, parent, *windows = NULL;
	unsigned int count = 0;
	Atom supported_atoms[3];

	// Install error handlers first
	XSetErrorHandler(on_x_error);
	XSetIOErrorHandler(on_io_error);

	// Connect to X
	display = XOpenDisplay(NULL);
	if (!display) return 1;

	root =          DefaultRootWindow(display);
	screen_width =  DisplayWidth(display, DefaultScreen(display));
	screen_height = DisplayHeight(display, DefaultScreen(display));

	// Atoms
	net_wm_state    = XInternAtom(display, "_NET_WM_STATE", False);
	fullscreen_atom = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
	active_atom     = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
	close_atom      = XInternAtom(display, "_NET_CLOSE_WINDOW", False);
	supported_atom  = XInternAtom(display, "_NET_SUPPORTED", False);

	// Become the WM. Fails and is ignored if another WM is running
	XSelectInput(display, root, SubstructureRedirectMask | SubstructureNotifyMask | FocusChangeMask);
	XSync(display, False);

	// Advertise supported atoms
	supported_atoms[0] = net_wm_state;
	supported_atoms[1] = fullscreen_atom;
	supported_atoms[2] = active_atom;
	XChangeProperty(display, root, supported_atom, XA_ATOM, 32, PropModeReplace, (unsigned char *)supported_atoms, 3);

	// Manage existing windows
	if (XQueryTree(display, root, &tree_root, &parent, &windows, &count)) {
		for (unsigned int i = 0; i < count; i++) {
			XWindowAttributes attributes;

			if (!is_manageable_window(windows[i], &attributes)) continue;

			fullscreen(windows[i]);
			XMapWindow(display, windows[i]);
		}
		XFree(windows);
	}
	XSync(display, False);
	focus_top();

	// Startup finished. Focus changes on MapNotify from here on
	startup_done = 1;

	// Event loop
	for (;;) {
		XNextEvent(display, &event);

		switch (event.type) {
		case MapRequest: {
			Window window = event.xmaprequest.window;
			XWindowAttributes attributes;

			if (!is_manageable_window(window, &attributes)) {
				XMapWindow(display, window);
				break;
			}
			fullscreen(window);
			XMapWindow(display, window);

			break;
		}

		case MapNotify: {
			// Ignore popups and tooltips, and the startup maps
			if (!startup_done) break;

			XWindowAttributes attributes;
			Window window = event.xmap.window;

			if (is_manageable_window(window, &attributes) && attributes.map_state == IsViewable) {
				focus(window);
			}

			break;
		}

		case ConfigureRequest: {
			// Honor the request, then re-fullscreen
			XConfigureRequestEvent *req = &event.xconfigurerequest;
			XWindowChanges changes = {
				.x = req->x,
				.y = req->y,
				.width = req->width,
				.height = req->height,
				.border_width = req->border_width,
				.sibling = req->above,
				.stack_mode = req->detail,
			};

			XConfigureWindow(display, req->window, req->value_mask, &changes);
			fullscreen(req->window);

			break;
		}

		case UnmapNotify:
			if (event.xunmap.window == focused)
				focus_top();
			break;

		case DestroyNotify:
			if (event.xdestroywindow.window == focused)
				focus_top();
			break;

		case ClientMessage:
			// Handle standard close requests
			if ((Atom)event.xclient.message_type == close_atom && event.xclient.format == 32) {
				XDestroyWindow(display, (Window)event.xclient.data.l[0]);
			}
			break;

		default:
			break;
		}
	}
}
