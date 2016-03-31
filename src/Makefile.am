SUBDIRS = lib

bin_PROGRAMS = kazusad

kazusad_LDFLAGS = -L./lib
kazusad_LDADD = -l_kazusa

kazusad_CFLAGS =  -I ./lib

kazusad_DEPENDENCIES = ./lib/lib_kazusa.a
kazusad_SOURCES = kazusad.c
