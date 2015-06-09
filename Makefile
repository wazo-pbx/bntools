all:
	make -C src $@ DESTDIR=$(DESTDIR)
	make -C bnfos $@ DESTDIR=$(DESTDIR)
	make -C bnrps $@ DESTDIR=$(DESTDIR)
	@echo
	@echo "Success. Install now with: make install"

clean:
	make -C src $@ || true
	make -C bnfos $@ || true
	make -C bnrps $@ || true
	make -C res_bnfos $@ || true

install: all
	make -C src $@ DESTDIR=$(DESTDIR)
	make -C bnfos $@ DESTDIR=$(DESTDIR)
	make -C bnrps $@ DESTDIR=$(DESTDIR)
	@echo
	@echo "Installation successful."
	
res_bnfos:
	@if ! test -e /usr/include/asterisk/channel.h; then echo "Asterisk Headers not found! Aborting."; exit -1; fi
	make -C src all DESTDIR=$(DESTDIR)
	make -C res_bnfos all DESTDIR=$(DESTDIR)
	@echo
	@echo "Success. Install now with: make res_bnfos_install"

res_bnfos_install: res_bnfos
	make -C res_bnfos install DESTDIR=$(DESTDIR)
	@echo
	@echo "Installation successful."

package:
	if [ -e bntools ]; then rm -rf bntools; fi
	if [ -e bntools.tar.gz ]; then rm -f bntools.tar.gz; fi
	mkdir -p bntools bntools/bnfos bntools/bnrps bntools/src bntools/res_bnfos bntools/res_bnfos/conf bntools/res_bnfos/init
	cp -f bnfos/Makefile bnfos/*.c bntools/bnfos/
	cp -f bnrps/Makefile bnrps/*.c bntools/bnrps/
	cp -rf src/beronet src/Makefile src/*.[ch] bntools/src/
	cp -f res_bnfos/*.[ch] res_bnfos/Makefile res_bnfos/README bntools/res_bnfos/
	cp -f res_bnfos/conf/*.conf bntools/res_bnfos/conf/
	cp -f res_bnfos/init/* bntools/res_bnfos/init/
	cp -f Makefile README bntools/
	tar czf bntools.tar.gz bntools
	rm -rf bntools

debian-package:
	dpkg-buildpackage -rfakeroot

.PHONY: all clean install res_bnfos res_bnfos_install package debian-package
