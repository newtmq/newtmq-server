check_PROGRAMS = kazusa_test

kazusa_test_SOURCES = test.c unit/test_optparse.c
kazusa_test_LDADD = -lcunit -l_kazusa
kazusa_test_CFLAGS = -I../src/lib/
kazusa_test_LDFLAGS = -L../src/lib/

check:
	./kazusa_test
