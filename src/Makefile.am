SUBDIRS = lib

bin_PROGRAMS = newtd

newtd_LDFLAGS = -L./lib
newtd_LDADD   = -l_newt -lconfuse -lpthread -lmsgpack
newtd_CFLAGS  = -I./include

newtd_DEPENDENCIES = ./lib/lib_newt.a
newtd_SOURCES = newtd.c
