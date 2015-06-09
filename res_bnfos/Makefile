INSTALL_MODPATH=/usr/lib/asterisk/modules

INSTALL_PREFIX:=

CFLAGS+=-g -ggdb -Wall -D_GNU_SOURCE -I. -I../src/
CFLAGS+=-Wno-missing-prototypes -Wno-missing-declarations
CFLAGS+=-fomit-frame-pointer -D_REENTRANT -DINSTALL_PREFIX=$(INSTALL_PREFIX)
CFLAGS+=-Wall
LDFLAGS 	= -L../src -lbnfos `curl-config --libs`

all: res_bnfos.so

res_bnfos.so: res_bnfos.o config.o kick.o cli.o
	$(CC) -shared -Xlinker -x -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.so *.o

install: res_bnfos.so
	install -m 755 res_bnfos.so $(INSTALL_PREFIX)$(INSTALL_MODPATH)/
	if [ ! -f $(INSTALL_PREFIX)/etc/asterisk/bnfos.conf ] ; then cp conf/bnfos.conf $(INSTALL_PREFIX)/etc/asterisk/; fi
	if [ ! -f $(INSTALL_PREFIX)/usr/sbin/safe_asterisk_fos ] ; then install -m 755 init/safe_asterisk_fos $(INSTALL_PREFIX)/usr/sbin/safe_asterisk_fos; fi
	if [ ! -f $(INSTALL_PREFIX)/etc/init.d/asterisk_fos ] ; then install -m 755 init/asterisk_fos $(INSTALL_PREFIX)/etc/init.d/asterisk_fos; fi

force-install: res_bnfos.so
	install -m 755 res_bnfos.so $(INSTALL_PREFIX)$(INSTALL_MODPATH)/
	if [ ! -f $(INSTALL_PREFIX)/etc/asterisk/bnfos.conf ] ; then cp conf/bnfos.conf $(INSTALL_PREFIX)/etc/asterisk/; fi
	install -m 755 init/safe_asterisk_fos $(INSTALL_PREFIX)/usr/sbin/safe_asterisk_fos
	install -m 755 init/asterisk_fos $(INSTALL_PREFIX)/etc/init.d/asterisk_fos
