ifneq (,$(findstring CYGWIN,$(shell uname)))
LIBEXT 		= .dll
BINEXT 		= .exe
ACFLAGS		=
else
LIBEXT 		= .so
BINEXT 		=
ACFLAGS		=
endif

CFLAGS		= -pipe -c -Wall -ggdb $(ACFLAGS) -I. `curl-config --cflags`
LDFLAGS     = `curl-config --libs`
FOSOBJS		= bnfos.o gbl4.o
RPSOBJS		= bnrps.o gbl4.o


#all: libbnfos$(LIBEXT) libbnrps$(LIBEXT) mkconf_fos$(BINEXT) mkconf_rps$(BINEXT)
all: libbnfos.a libbnrps.a mkconf_fos$(BINEXT) mkconf_rps$(BINEXT)

%.o: %.c beronet/bnfos.h beronet/bnrps.h beronet/gbl4.h beronet/confmap.h
	$(CC) $(CFLAGS) -o $@ $<

libbnfos.a: $(FOSOBJS)
	ar rc libbnfos.a $(FOSOBJS)

libbnrps.a: $(RPSOBJS)
	ar rc libbnrps.a $(RPSOBJS)

libbnfos$(LIBEXT): $(FOSOBJS)
	$(CC) -shared -o $@ $(FOSOBJS) $(LDFLAGS)

libbnrps$(LIBEXT): $(RPSOBJS)
	$(CC) -shared -o $@ $(RPSOBJS) $(LDFLAGS)

mkconf_fos$(BINEXT): mkconf_fos.o
	$(CC) mkconf_fos.o -o $@

mkconf_rps$(BINEXT): mkconf_rps.o
	$(CC) mkconf_rps.o -o $@

clean: 
	rm -rf *.a *.o *.so core* .depend mkconf_fos$(BINEXT) mkconf_rps$(BINEXT) *~ libbnfos$(LIBEXT) libbnrps$(LIBEXT)

install:
#	mkdir -p $(DESTDIR)/usr/lib $(DESTDIR)/usr/include/beronet
#	cp libbnfos.so libbnrps.so $(DESTDIR)/usr/lib
#	cp -r beronet/*.h $(DESTDIR)/usr/include/beronet/
