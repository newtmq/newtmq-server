SUBDIRS = lib

bin_PROGRAMS = program

program_LDFLAGS = -L./lib 
program_LDADD = -llib_stomp

program_CFLAGS =  -I ./lib 

program_DEPENDENCIES = ./lib/liblib_stomp.a
program_SOURCES = kazusad.c
