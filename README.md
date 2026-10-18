# full-wm

A minimal (and dumb) fullscreen-only window manager for X11, written in C using Xlib.

## Features

- Automatically fullscreen windows.
- Focus and raise mapped windows.
- Track the active window using `_NET_ACTIVE_WINDOW`.
- Focus another visible window when the focused window is destroyed.

## Dependencies

- Xlib development headers and library

## Build

Compile the source code:

```sh
gcc -Wall -Wextra -O2 full-wm.c -o full-wm -lX11
```

## Usage

Run `full-wm` inside an X session without another window manager running on that display:

```sh
./full-wm
```

To select `full-wm` or another session when starting X, use this `.xinitrc`:

```sh
#!/bin/sh

# Default session
session="i3"

if [ "$#" -gt 0 ]; then
  session="$1"
fi

exec "$session"
```

Start the desired session by passing its command to `startx`:

```sh
startx ~/.xinitrc full-wm
```

The session command must be available in your `PATH`, or you must provide its absolute path.

## Limitations

This is an experimental, minimal window manager rather than a complete EWMH-compliant implementation.

- Windows are forced to fullscreen dimensions.
- Window configuration requests are overridden.
- Focus behavior is based on X11 mapping and destruction events.
- It does not provide workspaces, decorations, tiling, or a built-in keybinding system.
- Only one window manager can control a given X server's root window at a time.

Use a separate X server when testing it alongside an existing window manager.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
