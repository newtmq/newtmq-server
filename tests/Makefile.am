check_PROGRAMS = kazusa_test

kazusa_test_SOURCES = test.c unit/test_optparse.c unit/test_config.c unit/test_stomp.c
kazusa_test_LDADD = -lcunit -l_kazusa -lconfuse
kazusa_test_CFLAGS = -I../src/include/
kazusa_test_LDFLAGS = -L../src/lib/

check:
	./kazusa_test
