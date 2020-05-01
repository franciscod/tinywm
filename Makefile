PREFIX?=/usr/X11R6
CFLAGS?=-Os -pedantic -Wall

tinywm: tinywm.c
	$(CC) $(CFLAGS) -I$(PREFIX)/include tinywm.c -L$(PREFIX)/lib -I/usr/include/cairo -lX11 -lXext -lXfixes -lcairo -o tinywm

clean:
	rm -f tinywm
