dnl integrit - file integrity verification system	-*- m4 -*-
dnl Copyright (C) 2006 Ed L. Cashin
dnl 
dnl This program is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU General Public License
dnl as published by the Free Software Foundation; either version 2
dnl of the License, or (at your option) any later version.
dnl 
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
dnl 

dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.53)
AC_INIT(hashtbl.c)
AC_CONFIG_HEADER(config.h)

dnl AC_CONFIG_AUX_DIR(..)

# ----- you can override these values by setting environment variables
# CC=my-compiler ./configure
# CFLAGS="-g foo -x bar" ./configure
CFLAGS=${CFLAGS:-"-g -Wall -O2"}
STATIC=${STATIC:-"-static"}

dnl Checks for programs.
AC_PROG_CC
AC_PROGRAM_PATH(RM, rm , rm)
dnl syncing is the job of the O.S., and users can always do "make; sync"
dnl AC_PROGRAM_PATH(SYNC, sync, sync)
AM_PROG_AR
AC_PROG_MAKE_SET
AC_PROG_RANLIB

dnl for compilers that do not provide __FUNCTION__
AC_MSG_CHECKING(for __FUNCTION__ symbol)
AC_TRY_COMPILE([#include <stdio.h>] , [printf ("%s\n", __FUNCTION__);] , 
  [HAVE__FUNCTION__=1
   AC_MSG_RESULT(yes)], 
  [AC_MSG_RESULT(no)
   CFLAGS="$CFLAGS -D__FUNCTION__=__FILE__"
   HAVE__FUNCTION__=0])

dnl support compilers that do not support inline keyword (standard since C99)
AC_C_INLINE

dnl ---------- let users override the install that configure finds
AC_ARG_WITH(install,
[  --with-install=INSTALL  manually specify a BSD-compatable install program])
if test "$with_install" != ""; then
	if test "`echo $with_install | sed 's/\(.\).*/\1/'`" != "/"; then
		echo configure Error: install program '"'$with_install'"' is not a full path 1>&2
		exit 1
	fi
	echo setting install program with $with_install
	INSTALL=$with_install
else
	AC_PROG_INSTALL
fi

dnl Checks for libraries.

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(unistd.h limits.h)
AC_CHECK_HEADERS(stdint.h inttypes.h)
dnl see below for more work after type sizes are checked

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

AC_CHECK_SIZEOF(unsigned short, 2)
AC_CHECK_SIZEOF(unsigned int, 4)
AC_CHECK_SIZEOF(unsigned long, 4)
AC_CHECK_SIZEOF(unsigned long long, 0)
AC_CHECK_HEADER(stdint.h, have_stdint_h=yes, have_stdint_h=no)
AC_CHECK_HEADER(inttypes.h, have_inttypes_h=yes, have_inttypes_h=no)
if test "$have_stdint_h" = "no" && test "$have_inttypes_h" = "no"; then
  echo no system stdint.h or inttypes.h.  using stdint.h.in instead.
  cp stdint.h.in stdint.h
  AC_DEFINE(HAVE_STDINT_H,, [use stdint.h.in instead of stdint.h])
fi

dnl Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNC(strerror,, [cat <<ELCEOF

        Your system doesn't have strerror. exiting.

ELCEOF
exit 1
])

dnl check for dictionary file for hashtest
AC_MSG_CHECKING(for system dictionary)
system_dict=no
for f in /usr/dict/words /usr/share/dict/words; do 
  if test -r $f; then
    system_dict=$f
    break
  fi
done
AC_MSG_RESULT($system_dict)
if test "$system_dict" = "no"; then
  system_dict="no_dictionary_found"
fi
AC_DEFINE_UNQUOTED(SYSTEM_DICTIONARY,"$system_dict",the system dictionary)

dnl ---------------- make hashtbl not error out on large files if the
dnl                  system supports it. 
dnl     
dnl                  user can say "--disable-largefile" to turn large file
dnl                  support off.
dnl
AC_SYS_LARGEFILE

dnl ----------let the user do ./configure --enable-debug
dnl 
AC_ARG_ENABLE(debug,
	[  --enable-debug          compile with runtime diagnostics enabled], 
	AC_DEFINE(DEBUG,, [enable debugging]))

dnl ----------let ambitious users do ./configure --with-leakfind
dnl 
AC_ARG_WITH(leakfind,
[  --with-leakfind=DIR     compile with Boehm gc runtime memory-leak debugging (for developers)], 
	AC_DEFINE(ELC_FIND_LEAKS,, [enable leakfind]))
dnl -----------... and add to the appropriate variables
if test "$with_leakfind" != ""; then
	LIBS="$LIBS -lgc"
	CPPFLAGS="$CPPFLAGS -I$with_leakfind"
	LDFLAGS="$LDFLAGS -L$with_leakfind"
fi

dnl if they want debugging, then don't strip the binaries on install
dnl
if test "$enable_debug" != ""; then
	STRIP=
else
	STRIP=-s
fi

AC_SUBST(CC)
AC_SUBST(RM)
AC_SUBST(AR)
AC_SUBST(RANLIB)
AC_SUBST(EXTRA_DEFS)
AC_SUBST(STRIP)
AC_OUTPUT(Makefile)
