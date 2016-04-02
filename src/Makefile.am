SUBDIRS = lib

bin_PROGRAMS = kazusad

kazusad_LDFLAGS = -L./lib
kazusad_LDADD   = -l_kazusa -lconfuse
kazusad_CFLAGS  = -I./include

kazusad_DEPENDENCIES = ./lib/lib_kazusa.a
kazusad_SOURCES = kazusad.c
