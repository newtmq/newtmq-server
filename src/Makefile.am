SUBDIRS = lib

bin_PROGRAMS = program

program_LDFLAGS = -L./lib 
program_LDADD = -l_kazusa

program_CFLAGS =  -I ./lib 

program_DEPENDENCIES = ./lib/lib_kazusa.a
program_SOURCES = kazusad.c
