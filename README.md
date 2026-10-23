# full-wm

A minimal (and dumb) fullscreen-only window manager for X11, written in C using Xlib.

## Features

- Automatically fullscreen windows.
- Focus and raise new windows.
- Focus another visible window when the focused window is destroyed or unmapped.
- Keep windows fullscreen when their size or position is changed.

## Dependencies

- Xlib development headers and library.

## Build

Using make:

```sh
make
```

Or compile manually:

```sh
cc -Wall -Wextra -O2 full-wm.c -o full-wm -lX11
```

## Install

Install to `/usr/local/bin`:

```sh
sudo make install
```

## Usage

Run `full-wm` (if it's in your `PATH`) or specify its absolute path
inside an X session without another window manager running on that display:

```sh
./full-wm
```

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
