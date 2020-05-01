PREFIX?=/usr/X11R6
CFLAGS?=-Os -pedantic -Wall

	$(CC) $(CFLAGS) -I$(PREFIX)/include tinywm.c -L$(PREFIX)/lib -lX11 -o tinywm
tinywm: tinywm.c

clean:
	rm -f tinywm
