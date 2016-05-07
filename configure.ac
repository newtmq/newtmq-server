#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([project], [0.0.1], [user.localhost2000@gmail.com])
AC_CONFIG_SRCDIR([src/newtd.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign])

# Checks for programs.
AC_CHECK_LIB(pthread, pthread_create, [], echo "Error: Required library pthread not found. Install its development package and try again" && exit 1)
AC_CHECK_LIB(confuse, cfg_init, [], echo "Error: Required library confuse not found. Install ints development package  and try again" && exit 1)
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_CC_C_O
AC_PROG_RANLIB

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_OUTPUT(Makefile \
          src/Makefile \
          src/lib/Makefile \
          tests/Makefile)
