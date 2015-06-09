ifeq (,$(DESTDIR))
	DESTDIR=
endif

SRCS		= main.c
OBJS		= main.o

ifneq (,$(findstring CYGWIN,$(shell uname)))
	LIBEXT = .dll
	BINEXT = .exe
	LDFLAGS += /usr/lib/libcurl.a -static
	CONFFILE = bnrps.conf
else
	LIBEXT = .so
	BINEXT =
	CONFFILE = $(DESTDIR)/etc/bnrps.conf
endif

CFLAGS		+= -c -Wall -ggdb -I../src -DDESTDIR=\"$(DESTDIR)\" -DCONFFILE=\"$(CONFFILE)\"
LDFLAGS 	+= -L../src ../src/libbnrps.a `curl-config --libs`

all: bnrps$(BINEXT)

%.o: %.c ../src/beronet/bnrps.h ../src/beronet/confmap_rps.h
	$(CC) $(CFLAGS) -o $@ $<

bnrps$(BINEXT): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

clean: 
	rm -rf *.a *.o *.so core* .depend *~ bnrps$(BINEXT)

install:
	mkdir -p $(DESTDIR)/usr/bin
	cp bnrps $(DESTDIR)/usr/bin
