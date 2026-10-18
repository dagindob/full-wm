// Includes
#include <X11/Xlib.h>
#include <X11/Xatom.h>

// X11 globals
static Display *display;
static Window root, focused;
static Atom net_wm_state, fullscreen_atom, active_atom;

static int screen_width, screen_height;

// Check if a window is manageble
static int is_manageable_window(Window window, XWindowAttributes *attributes)
{
	return XGetWindowAttributes(display, window, attributes) && !attributes->override_redirect && attributes->class != InputOnly;
}

// Make a window fullscreen
static void fullscreen(Window window)
{
	// Set the fullscreen state
	unsigned long window_state = fullscreen_atom;
	XChangeProperty(display, window, net_wm_state, XA_ATOM, 32, PropModeReplace, (unsigned char *)&window_state, 1);

	// Make the window fill the screen
	XMoveResizeWindow(display, window, 0, 0, screen_width, screen_height);
}

// Focus a window and bring it to the front
static void focus(Window window)
{
	XWindowAttributes attributes;
	unsigned long window_id = window;

	// Make sure that the window is manageble and visible
	if (!is_manageable_window(window, &attributes) || attributes.map_state != IsViewable) return;

	// Bring the window to the front
	XRaiseWindow(display, window);

	// Give the window keyboard focus
	XSetInputFocus(display, window, RevertToPointerRoot, CurrentTime);

	// Set the active window property
	XChangeProperty(display, root, active_atom, XA_WINDOW, 32, PropModeReplace, (unsigned char *)&window_id, 1);

	focused = window;
}

// Focus the topmost manageble visible window when the current focused window closes
static void focus_top(void)
{
	Window tree_root, parent, *windows;
	unsigned int count;

	// Clear the active window
	focused = None;
	XDeleteProperty(display, root, active_atom);

	// Get the windows on the root window
	if (XQueryTree(display, root, &tree_root, &parent, &windows, &count)) {
		while (count--) {
			Window window = windows[count];
			XWindowAttributes attributes;

			// Find the first manageable visible window
			if (is_manageable_window(window, &attributes) && attributes.map_state == IsViewable) {
				XFree(windows);
				focus(window);
				return;
			}
		}

		XFree(windows);
	}

	// If no windows are avaliable, fall back to PointerRoot
	XSetInputFocus(display, PointerRoot, RevertToPointerRoot, CurrentTime);
}

int main(void)
{
	XEvent event;
	Window tree_root, parent, *windows;
	unsigned int count;

	// Connect to the X server
	display = XOpenDisplay(NULL);
	if (!display) return 1;

	// Get the root window and the screen dimensions
	root =          DefaultRootWindow(display);
	screen_width =  DisplayWidth(display, DefaultScreen(display));
	screen_height = DisplayHeight(display, DefaultScreen(display));

	// Get the atoms used by the window manager
	net_wm_state =    XInternAtom(display, "_NET_WM_STATE", False);
	fullscreen_atom = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
	active_atom =     XInternAtom(display, "_NET_ACTIVE_WINDOW", False);

	// Advertise supported window properties and states
	Atom supported_atom = XInternAtom(display, "_NET_SUPPORTED", False);
	Atom supported_atoms[] = {net_wm_state, fullscreen_atom, active_atom};
	XChangeProperty(display, root, supported_atom, XA_ATOM, 32, PropModeReplace, (unsigned char *)supported_atoms, 3);

	// Listen for window creation, mapping, and configuration requests
	XSelectInput(display, root, SubstructureRedirectMask | SubstructureNotifyMask);
	XSync(display, False);

	// Handle existing windows
	if (!XQueryTree(display, root, &tree_root, &parent, &windows, &count)) {
		XCloseDisplay(display);
		return 1;
	}

	// Fullscreen all existing windows
	for (unsigned int i = 0; i < count; i++) {
		Window window = windows[i];
		XWindowAttributes attributes;

		if (!is_manageable_window(window, &attributes)) continue;

		fullscreen(window);
		XMapWindow(display, window);
	}

	XFree(windows);
	XSync(display, False);
	focus_top();

	// Handle X11 events
	for (;;) {
		XNextEvent(display, &event);

		switch (event.type) {
		case MapRequest: {
			Window window = event.xmaprequest.window;

			fullscreen(window);
			XMapWindow(display, window);
			break;
		}

		case MapNotify:
			focus(event.xmap.window);
			break;

		case ConfigureRequest:
			fullscreen(event.xconfigurerequest.window);
			break;

		case DestroyNotify:
			if (event.xdestroywindow.window == focused)
				focus_top();
			break;
		}
	}
}
