include Makefile.inc.in

# To process, run:
# aclocal; autoconf; autoheader; automake

SUBDIRS = swsupplib swstructlib swprogs tests/devel doc
CSUBDIRS = swsupplib 
CPPSUBDIRS = swstructlib swprogs tests/devel
INSTSUBDIRS = lib @PO@

TOPTARGET=depend everything

everything: make-subdirs $(PROGS) $(LIB) 

all:
	$(MAKE) $(TOPTARGET)
	@echo ""
	swprogs/intsizes
	@echo ""
	@echo "Testing is recommended (after installing)."
	@echo "      make check"
	@echo "      make installcheck  # After installing"
	@echo ""

make-subdirs:
	(mkdir tmp/ 2>/dev/null; exit 0)
	(mkdir swsupplib/tmp/ 2>/dev/null; exit 0)
	(mkdir swstructlib/src/tmp 2>/dev/null; exit 0)
	(mkdir swstructlib/src/lib 2>/dev/null; exit 0)
	(mkdir tests/tmp 2>/dev/null; exit 0)
	for d in $(SUBDIRS); do \
		(cd $$d; $(MAKE)) \
		  || case "$(MFLAGS)" in *k*) fail=yes;; *) exit 1;; esac;\
	done && test -z "$$fail"

#
# At present, the dist-hook target is abused to copy all the files
# into distdir instead of allowing the generated Makefiles do this
# job, and furthermore the shell script bin/copyfordist.sh takes
# the file list in catalog/dfiles/files as representing the
# files to be packaged.  Use of dist-hook is package dependent
# and using it in this way or at all is not required for using
# swbis to create signed tarballs.  See the target 'dist-swbis'
# below for the general rule that will work for any package.
#
dist-hook:
	echo "$(VERSION)" > VERSION
	echo "$(VERSION)" > $(distdir)/VERSION
	$(MAKE) -s version_files
	sh bin/copyfordist.sh $(distdir)

check:
	$(MAKE) $(TARGET) testsuite1
	for d in swsupplib/shell_lib; do \
		(cd $$d; $(MAKE) $@) ;\
	done

installcheck: check testall

testall:
	$(MAKE) $(TARGET) testsuite1 testsuite2

snapshot:
	( cd ..; cat swbis/catalog/dfiles/files | \
	tar --no-recursion cf - --files-from=- | \
	bzip2 -c -9 >swbis-`date -u +%Y%m%d"T"%H%M"Z"`.tar.bz2; \
	)

showtests_:
	echo "shorttest     test the md5 and sha1 digest code (short)."
	echo "shorttest2    test the md5 and sha1 digest code (short)."
	echo "digesttest    test the md5 and sha1 digest code (5 min.)."
	echo "digesttest2   test the md5 and sha1 digest code (5 min.)."
	echo "parsertest    parser regression test008 (short)."	
	echo "parsertest2   parser regression test009 (short)."	
	echo "archivetest0  run the make_archive script (short)."	
	echo "swigntest1    swign regression test024, asks for passphrase  (short)."	
	echo "swigntest2    swign regression test026 (short)."	
	echo "swigntest3    swign regression test027 (short)."	
	echo "swigntest4    swign regression test028 (short)."	

testsuite1: \
	testsuite1info \
	shorttest \
	shorttest2 \
	parsertest \
	parsertest2 \
	tartest1

testsuite1info:
	@echo ""
	@echo "Test the parser, md5, and sha1 digest code"
	@echo ""
	@echo "This test should only fail due to missing sha1sum, md5sum"
	@echo "or openssl utilities."
	@echo ""


testsuite2: \
	testsuite2info \
	swigntest2 \
	swigntest3 \
	swigntest4 \
	swigntest5 \
	tartest1

testsuite2info:
	@echo ""
	@echo "Test swpackage and swverify"
	@echo ""
	@echo "This test may fail if gpg is an old verion, if not running on"
	@echo "a GNU/Linux system, or if you don't have GNU tar."
	@echo ""

shorttest:
	(echo; cd swsupplib/tests; $(SHELL) ./testdig.sh --short; exit 0)

shorttest2:
	(echo; cd swsupplib/tests; $(SHELL) ./testdig2.sh --short; exit 0)

digesttest:
	(cd swsupplib/tests;  $(SHELL) ./testdig.sh)

digesttest2:
	(cd swsupplib/tests;  $(SHELL) ./testdig2.sh)

parsertest:
	(cd tests/regression; $(MAKE) -s test008 2>/dev/null; exit $?)

parsertest2:
	(cd tests/regression; $(MAKE) -s test009 2>/dev/null; exit $?)

archivetest0:
	(AUTHOR="Test User" ./make_archive --test-release; exit $?)

swigntest1:
	(cd tests/regression; $(MAKE) -s test024)

swigntest2:
	(cd tests/regression; $(MAKE) -s test026)

swigntest3:
	(cd tests/regression; $(MAKE) -s test027)

swigntest4:
	(cd tests/regression; $(MAKE) -s test028)

swigntest5:
	(cd tests/regression; $(MAKE) -s test029)

Xtartest1:
	sh bin/testforgnutar.sh; case  $$? in 0) \
	(cd tests/regression; $(MAKE) -s test030); ;; \
	*) echo GNU tar not found, test030 not run, exiting with status 0; exit 0; ;; esac

tartest1:
	$(MAKE) -s Xtartest1

make-include_links:
	find  *.h  swprogs swstructlib swsupplib -name '*.h' | \
	sed -e 's@^\./@@' | \
	egrep -v -e '/config.h' | egrep -v -e swuser_config.h | egrep -v -e /um_rpm/rpm- | \
	(while read file; do (cd include; ln -s  ../$$file `basename $$file`); done; exit 0)
	(${MAKE} -s make-final_include_links; exit 0)
	if [ "$(DO_UM_RPM)" ]; then \
		(${MAKE} -s make-um-rpm_include_links; exit 0); \
	fi

VERSION:
	echo "$(VERSION)" > VERSION

version_files: swprog_versions.h version.sgml version.texi

swprog_versions.h:
	sed -e s/__VERSION__/$$(cat VERSION)/ <include/$@.in >include/$@

version.sgml:
	sed -e s/__VERSION__/$$(cat VERSION)/ <doc/source/$@.in >doc/source/$@

version.texi:
	sed -e s/__VERSION__/$$(cat VERSION)/ <doc/source/$@.in >doc/source/$@

maketmpdirs:
	(mkdir swstructlib/src/tmp; exit 0)
	(mkdir swsupplib/um_rpm/rpm-3.0.5/tmp; exit 0)
	(mkdir swsupplib/tmp; exit 0)
	(mkdir tests/tmp; exit 0)
	(mkdir tmp; exit 0)

cleantmpdirs:
	/bin/rm -f swstructlib/src/tmp/*
	/bin/rm -f swsupplib/um_rpm/rpm-3.0.5/tmp/*
	/bin/rm -f swsupplib/tmp/*
	/bin/rm -f tests/tmp/*
	/bin/rm -f tmp/*

removetmpdirs:
	(rmdir swstructlib/src/tmp; exit 0)
	(rmdir swsupplib/um_rpm/rpm-3.0.5/tmp; exit 0)
	(rmdir swsupplib/tmp; exit 0)
	(rmdir tests/tmp; exit 0)
	(rmdir tmp; exit 0)

make-um-rpm_include_links:
	cat swsupplib/um_rpm/linklist | \
	sed -e 's@^\./@@' | \
	egrep -v -e '/config.h' | egrep -v -e swuser_config.h  | \
	while read file; do (cd include; ln -s  ../$$file `basename $$file`); done

make-final_include_links:
	echo swsupplib/sw_parse/swparse.tab.h | \
	sed -e 's@^\./@@' | \
	egrep -v -e '/config.h' | egrep -v -e swuser_config.h | egrep -v -e /um_rpm/rpm- |\
	while read file; do (cd include; ln -s  ../$$file `basename $$file`); done
	\
	echo 'swsupplib/sw_parse/y.tab.h' | \
	while read file; do (cd include; ln -s  ../$$file `basename $$file`); done
	\
	echo 'swstructlib/src/misc/swptrlist.cxx' | \
	sed -e 's@^\./@@' | \
	egrep -v -e /config.h | \
	egrep -v -e swuser_config.h | \
	while read file; do (cd include; ln -s  ../$$file `basename $$file`); done
	\
	(cd include && cp /dev/null popt.h;)
	(cd include && /bin/rm config.h; ln -s ../swsupplib/config.h config.h;)

.depend: 
	$(MAKE) maketmpdirs
	$(MAKE) make-include_links
	for d in $(CSUBDIRS); do \
		(cd $$d; $(MAKE) depend) ;\
	done
	for d in $(CPPSUBDIRS); do \
		(cd $$d; $(MAKE) depend) ;\
	done
	( > .depend )
	$(MAKE) -s version_files
	
# Environment Controls for swign:
#   To enter passphrase via gpg-agent: SWPACKAGEPASSFD="agent"
#   To enter passphrase via the tty:   SWPACKAGEPASSFD="tty" or unset, because this is the default
#   To specify the GPG User:    export GNUPGNAME="Your-GPG-Name"
#   To specify the GPG homedir: export GNUPGHOME="Your-GPG-homedir"      

# Provide am__remove_distdir ourselves since am__remove_distdir may be a
# private automake variable.
sw_am__remove_distdir = \
  { test ! -d $(distdir) \
    || { find $(distdir) -type d ! -perm -200 -exec chmod u+w {} ';' \
         && /bin/rm -fr $(distdir); }; }
dist-swbis: distdir
	(cd $(distdir) && swign -s PSF.in --name-version=$(distdir) @-) | GZIP=$(GZIP_ENV) gzip -c >$(distdir).tar.gz
	$(sw_am__remove_distdir)

depend: .depend 

clean:
	for d in $(SUBDIRS); do \
		(cd $$d; $(MAKE) $@) ;\
	done
	/bin/rm -f *.S *.a *.o core *~ $(PROGS) rpm.shared 
	/bin/rm -f include/*LINKS
	/bin/rm -f include/LINKS
	find . -name "*.orig" -exec /bin/rm {} \;
	find . -name core -exec /bin/rm {} \;
	/bin/rm -fr tmp/*
	$(MAKE) -s cleantmpdirs

.PHONY:		distclean
distclean: clean
	for d in $(SUBDIRS); do \
		(cd $$d; $(MAKE) $@) ;\
	done
	$(MAKE) -s removetmpdirs
	/bin/rm -f *.a *.o core *~ $(PROGS) rpm.shared librpmrc find-provides.sh find-requires.sh design.ps design.eps
	/bin/rm -f .depend config.h config.h.in~ config.cache config.status config.log lib-rpmrc *.bak
	/bin/rm -f CHANGES.rej*
	/bin/rm -fr autom4te.cache
	/bin/rm -fr swsupplib/autom4te.cache
	(cd include; find . -type l -exec /bin/rm -f {} \;)
	(find . -name '.#*' -exec /bin/rm -f {} \;)
	(cd include; /bin/rm -f popt.h \;)
	(find . -name gmon.out -exec /bin/rm -f {} \;)
	(find . -name .depend -exec /bin/rm -f {} \;)
	/bin/rm -f stamp-h1
	/bin/rm -f tags TAGS ctags CTAGS
	/bin/rm -fr tmp/*
	/bin/rm -f .[a-z]*
	/bin/rm -f Makefile Makefile.inc

.PHONY:		depend

