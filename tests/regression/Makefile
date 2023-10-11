

# set1
#	tests psf generation, swtruct() and the parser.

# set2
#	specifically tests the parser.
PASS="                                              PASSED"
MYSHELL=/bin/bash

fileset4 = $(prefix1)/testing/lib/rpm/rh-4.0/b*.rpm
fileset5 = $(prefix1)/testing/lib/rpm/rh-5.0/b*.rpm
fileset6 = $(prefix1)/testing/lib/rpm/rh-6.0/b*.rpm

all: 
	$(MAKE) -s alli 1>/usr/tmp/make.stdout.out  2>/usr/tmp/make.stderr.out
	diff /usr/tmp/make.stdout.out baseline/make.stdout.out
	diff /usr/tmp/make.stderr.out baseline/make.stderr.out

alli: set1 set2

depend:

set1:	test001 \
		test002 \
		test003 \
		test004 \
		test005 \
		test006 \
		test007 \
		test010 \
		test011 \
		test012 \
		test013 \
		test014 \
		test015 \
		test016 \
		test017 \
		test018 \
		test019 \
		test020 

set2:	test008 test009


nrrpm:	\
		test008 \
		test009 \
		test014 \
		test015 \
		test016 \
		test017 \
		test018 \
		test019 \
		test020 \
		test021 \
		test022 \
		test023 \
		test024 \
		test025 \
		test026 \
		test027 \
		test028 \
		test029

test001:
	echo "====================================== running test 001"
	find  $(fileset4) | $(MYSHELL) ./test001.sh
	find  $(fileset5) | $(MYSHELL) ./test001.sh
	find  $(fileset6) | $(MYSHELL) ./test001.sh
	echo "finished ============================="
	echo "finished ============================= test001" 1>&2

test002:
	echo "====================================== running test 002"
	find  $(fileset4) | $(MYSHELL) ./test002.sh
	find  $(fileset5) | $(MYSHELL) ./test002.sh
	find  $(fileset6) | $(MYSHELL) ./test002.sh
	echo "finished ============================="
	echo "finished ============================= test002" 1>&2

test003:
	echo "====================================== running test 003"
	find  $(fileset5) | $(MYSHELL) ./test003.sh
	echo "finished ============================="
	echo "finished ============================= test003" 1>&2

test004:
	echo "====================================== running test 004"
	find  $(fileset5) | $(MYSHELL) ./test004.sh
	echo "finished ============================="
	echo "finished ============================= test004" 1>&2

test005:
	echo "====================================== running test 005"
	find  $(fileset5) | $(MYSHELL) ./test005.sh
	echo "finished ============================="
	echo "finished ============================= test005" 1>&2

test006:
	echo "====================================== running test 006"
	find  $(fileset4) | $(MYSHELL) ./test006.sh
	find  $(fileset5) | $(MYSHELL) ./test006.sh
	find  $(fileset6) | $(MYSHELL) ./test006.sh
	echo "finished ============================="
	echo "finished ============================= test006" 1>&2

test007:
	echo "====================================== running test 007"
	find  $(fileset5) | $(MYSHELL) ./test007.sh
	echo "finished ============================="
	echo "finished ============================= test007" 1>&2

test008:
	./test008.sh

test009:
	./test009.sh

test010:
	echo "====================================== running test 010"
	find  $(fileset5) | $(MYSHELL) ./test010.sh
	echo "finished ============================="
	echo "finished ============================= test010" 1>&2

test011:
	echo "====================================== running test 011"
	find  $(fileset5) | $(MYSHELL) ./test011.sh
	echo "finished ============================="
	echo "finished ============================= test011" 1>&2

test012:
	echo "====================================== running test 012"
	find  $(fileset4) | $(MYSHELL) ./test012.sh
	find  $(fileset5) | $(MYSHELL) ./test012.sh
	echo "finished ============================="
	echo "finished ============================= test012" 1>&2

test013:
	echo "====================================== running test 013"
	find  $(fileset4) | $(MYSHELL) ./test013.sh
	find  $(fileset5) | $(MYSHELL) ./test013.sh
	echo "finished ============================="
	echo "finished ============================= test013" 1>&2

test014:
	echo "====================================== running $@"
	$(MYSHELL) ./$@.sh
	echo "finished ============================="
	echo "finished ============================= $@" 1>&2

test015:
	echo "====================================== running $@"
	$(MYSHELL) ./$@.sh
	echo "finished ============================="
	echo "finished ============================= $@" 1>&2

test016:
	echo "====================================== running test 016"
	$(MYSHELL) ./test016.sh
	echo "finished ============================="
	echo "finished ============================= test016" 1>&2

test017:
	echo "====================================== running $@"
	$(MYSHELL) ./$@.sh
	echo "finished ============================="
	echo "finished ============================= $@" 1>&2

test018:
	echo "====================================== running $@"
	$(MYSHELL) ./$@.sh
	echo "finished ============================="
	echo "finished ============================= $@" 1>&2

test019:
	echo "================================= running $@"
	$(MYSHELL) ./$@.sh
	echo "finished ========================"
	echo "finished ======================== $@" 1>&2

test020:
	echo "====================================== running $@"
	$(MYSHELL) ./$@.sh
	echo "finished ============================="
	echo "finished ============================= $@" 1>&2

test021:
	echo "====================================== running $@"
	$(MYSHELL) ./$@.sh
	echo "finished ============================="
	echo "finished ============================= $@" 1>&2
test022:
	echo "====================================== running $@"
	$(MYSHELL) ./$@.sh
	echo "finished ============================="
	echo "finished ============================= $@" 1>&2

test023:
	echo "================================== running $@"
	$(MYSHELL) ./$@.sh
	echo "finished ========================="
	echo "finished ========================= $@" 1>&2

test024:
	echo "running $@"
	(cd ../../; $(MYSHELL) tests/regression/$@.sh && echo $(PASS))
	echo "finished $@" 1>&2

test025:
	echo "running $@"
	(cd ../../; $(MYSHELL) tests/regression/$@.sh && echo $(PASS))
	echo "finished $@" 1>&2

test026:
	echo "running $@"
	(cd ../../; $(MYSHELL) tests/regression/$@.sh && echo $(PASS))
	echo "finished $@" 1>&2

test027:
	echo "running $@"
	(cd ../../; $(MYSHELL) tests/regression/$@.sh && echo $(PASS))
	echo "finished $@" 1>&2

test028:
	echo "running $@"
	(cd ../../; $(MYSHELL) tests/regression/$@.sh && echo $(PASS))
	echo "finished $@" 1>&2

test029:
	echo "running $@"
	(cd ../../; $(MYSHELL) tests/regression/$@.sh && echo $(PASS))
	echo "finished $@" 1>&2

test030:
	echo "running $@"
	(cd ../../; $(MYSHELL) tests/regression/$@.sh && echo $(PASS))
	echo "finished $@" 1>&2

test031:
	echo "running $@"
	(cd ../../; $(MYSHELL) tests/regression/$@.sh; case $$? in 0) echo $(PASS); ;; 1) echo FAIL; ;; 2) echo TEST SKIPPED; ;; esac)
	echo "finished $@" 1>&2

test032:
	echo "running $@"
	(cd ../../; $(MYSHELL) tests/regression/$@.sh; case $$? in 0) echo $(PASS); ;; 1) echo FAIL; ;; 2) echo TEST SKIPPED; ;; esac)
	echo "finished $@" 1>&2

test033:
	echo "running $@"
	(cd ../../; $(MYSHELL) tests/regression/$@.sh; case $$? in 0) echo $(PASS); ;; 1) echo FAIL; ;; 2) echo TEST SKIPPED; ;; esac)
	echo "finished $@" 1>&2

test034:
	echo "running $@"
	(cd ../../; $(MYSHELL) tests/regression/$@.sh; case $$? in 0) echo $(PASS); ;; 1) echo FAIL; ;; 2) echo TEST SKIPPED; ;; esac)
	echo "finished $@" 1>&2

