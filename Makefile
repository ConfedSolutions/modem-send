DESTDIR=
BINDIR=/usr/bin

all: modem-send

modem-send: modem-send.c
	$(CC) -O3 -o modem-send modem-send.c -lm

clean:
	-rm modem-send

install: modem-send
	install -d $(DESTDIR)/$(BINDIR)
	install -m 755 $< $(DESTDIR)/$(BINDIR)

.PHONY: all clean install
