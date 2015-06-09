ifeq (,$(DESTDIR))
	DESTDIR=
endif

SRCS		= main.c
OBJS		= main.o

ifneq (,$(findstring CYGWIN,$(shell uname)))
	LIBEXT = .dll
	BINEXT = .exe
	LDFLAGS += /usr/lib/libcurl.a -static
	CONFFILE = bnfos.conf
else
	LIBEXT = .so
	BINEXT =
	CONFFILE = $(DESTDIR)/etc/bnfos.conf
endif

CFLAGS		+= -c -Wall -ggdb -I../src -DDESTDIR=\"$(DESTDIR)\" -DCONFFILE=\"$(CONFFILE)\"
LDFLAGS 	+= -L../src ../src/libbnfos.a `curl-config --libs`

all: bnfos$(BINEXT)

%.o: %.c ../src/bnfos/bnfos.h ../src/bnfos/confmap.h
	$(CC) $(CFLAGS) -o $@ $<

bnfos$(BINEXT): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

clean: 
	rm -rf *.a *.o *.so core* .depend *~ bnfos$(BINEXT)

install:
	mkdir -p $(DESTDIR)/usr/bin
	cp bnfos $(DESTDIR)/usr/bin
