CC      ?= cc
CFLAGS  ?= -Wall -Wextra -O2
TARGET   = full-wm
SOURCE   = full-wm.c
PREFIX  ?= /usr/local
BINDIR   = $(PREFIX)/bin

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE) -lX11

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all install uninstall clean
