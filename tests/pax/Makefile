
SWBPNAME=pax
SWBPVER=3.0
SWBPREL=

SWBPFNAME=$(SWBPNAME)-$(SWBPVER)$(SWBPREL).tar.bz2
SWBPFDIR=pax-3.0
SWBBUILDDIR=$(SWBPFDIR).local


all: build-all

build-all: build-$(SWBPNAME)

build-$(SWBPNAME): $(SWBBUILDDIR)

install: build-all
	make  install-$(SWBPNAME)
	
$(SWBBUILDDIR): 
	make  unpack-$(SWBPNAME)
	make  configure-$(SWBPNAME)
	make  compile-$(SWBPNAME)

unpack-$(SWBPNAME):
	/bin/rm -fr ./$(SWBPFDIR)
	bzcat <$(SWBPFNAME) | tar xpf -
	/bin/mv -i $(SWBPFDIR) $(SWBBUILDDIR)
	/usr/bin/patch -p1 -d $(SWBBUILDDIR) <rdev_initialization.patch

configure-$(SWBPNAME):
	( \
		cd $(SWBBUILDDIR) && ./configure \
	)

compile-$(SWBPNAME):
	( \
		cd $(SWBBUILDDIR) && make \
	)
	
clean-$(SWBPNAME):
	( \
		cd $(SWBBUILDDIR) && make clean \
	)

distclean-$(SWBPNAME):
	( \
		cd $(SWBBUILDDIR) && make distclean \
	)

install-$(SWBPNAME):
	( \
		cd $(SWBBUILDDIR) && make install \
	)

depend:

clean:
	/bin/rm -fr ./$(SWBPFDIR)
	/bin/rm -fr ./$(SWBBUILDDIR)

distclean:
	/bin/rm -fr ./$(SWBPFDIR)
	/bin/rm -fr ./$(SWBBUILDDIR)


