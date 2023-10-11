
all: catalog

unpack: all

pack: 
	tar cf - --owner=root --group=root catalog | gzip -1 >catalog.tar.gz

catalog: catalog.tar.gz
	gzip -cd <catalog.tar.gz | tar xpf - ; true
	touch catalog

clean:
	rm -fr catalog

distclean: clean

