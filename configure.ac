dnl integrit - file integrity verification system    -*- m4 -*-
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
AC_INIT(elcwft.c)
AC_CONFIG_HEADER(config.h)

EXTRA_DEFS=${EXTRA_DEFS-""}

# ----- you can override these values by setting environment variables
# CC=my-compiler ./configure
# CFLAGS="-g foo -x bar" ./configure
CFLAGS=${CFLAGS-"-g -Wall -O2"}
STATIC=${STATIC-"-static"}

dnl
dnl These object files are the ones that don't go into "libintegrit.a".
dnl
dnl The library, libintegrit.a, is the way that integrit and the auxiliary
dnl tools share object code (as a convenience during the build).
dnl
OBJ="main.o options.o xml.o eachfile.o rules.o checkset.o missing.o xstrdup.o cdb_put.o cdb_get.o elcwft.o cdb.o cdb_make.o cdb_hash.o"
ILIBOBJ="elcerror.o cdb_seq.o hexprint.o xstradd.o show.o rmd160.o istat.o"

dnl ---------------- README IS THE DEFINITIVE PLACE TO SET THE VERSION NUMBER
INTEGRIT_VERSION=`sed -n 1p $srcdir/README | awk '{ print $3 }'`
dnl ---------------- 

AC_DEFINE(PROGNAME, "integrit", [integrit program name's when running])
AC_DEFINE_UNQUOTED(INTEGRIT_VERSION, "$INTEGRIT_VERSION", [integrit version])

dnl Checks for programs.
dnl AC_PROG_CXX
AC_PROG_CC
dnl AC_PROG_LEX
AC_PROG_MAKE_SET
AC_PROG_AWK
AC_PROGRAM_PATH(RM, rm , rm)
dnl syncing is the job of the O.S., and users can always do "make; sync"
dnl AC_PROGRAM_PATH(SYNC, sync, sync)
AM_PROG_AR
AC_PROG_RANLIB

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


dnl At least under OSX static linking cannot be used due to missing static libs.
AC_MSG_CHECKING([whether static linking is possible])
LDFLAGS_ORIG="$LDFLAGS"
LDFLAGS="$LDFLAGS $STATIC"
AC_TRY_LINK(, [int main() { return 0; }], AC_MSG_RESULT(yes),
[
   AC_MSG_RESULT(no)
   AC_MSG_WARN(integrit will be built without static linking)
   STATIC=""
])
LDFLAGS="$LDFLAGS_ORIG"


dnl ---------- let ambitious users do ./configure --with-leakfind
dnl 
AC_ARG_WITH(leakfind,
[  --with-leakfind=DIR     use Boehm gc memory-leak detection (for developers)])

dnl Checks for libraries.

if test "$with_leakfind" != ""; then
	if test "`echo $with_leakfind | sed 's/\(.\).*/\1/'`" != "/"; then
		echo configure Error: gc source directory '"'$with_leakfind'"' is not a full path 1>&2
		exit 1
	fi
	LDFLAGS="$LDFLAGS -L$with_leakfind"
	AC_CHECK_LIB(gc, GC_gcollect, , [cat <<ELCEOF

       libgc - 
         Boehm garbage collection / leak detection library, not found

       Please install libgc.a or tell configure where to find it (see
         INSTALL).  Make sure libgc.a is installed for static linking.

       Otherwise compile without the "--with-leakfind=DIR" configure 
	 option.

       exiting.	

ELCEOF
exit 1
])
fi

AC_ARG_WITH(mmap_max,
[  --with-mmap-max=BYTES   biggest database that will be mmap-ed [BYTES=10485760]])
with_mmap_max=${with_mmap_max:-10485760}
echo using max size for mmap-ing known database: $with_mmap_max bytes
AC_DEFINE_UNQUOTED(CDB_MMAP_MAX, $with_mmap_max, [maximal mmap region aperture])

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
dnl AC_CHECK_HEADERS(fcntl.h limits.h sys/time.h syslog.h unistd.h stdint.h inttypes.h)
AC_CHECK_HEADERS(fcntl.h limits.h sys/time.h syslog.h unistd.h utime.h)
AC_CHECK_HEADERS(stdint.h inttypes.h)
dnl but see below for more work after size of types has been found

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_CHECK_TYPES([long long,unsigned long long])

AC_MSG_CHECKING(for __FUNCTION__ symbol)
AC_TRY_COMPILE([#include <stdio.h>] , [printf ("%s\n", __FUNCTION__);] , 
  [HAVE__FUNCTION__=1
   AC_MSG_RESULT(yes)], 
  [AC_MSG_RESULT(no)
   CFLAGS="$CFLAGS -D__FUNCTION__=__FILE__"
   HAVE__FUNCTION__=0])

AC_CACHE_SAVE

dnl ---------------- make integrit not error out on large files if the
dnl                  system supports it. 
dnl     
dnl                  user can say "--disable-largefile" to turn large file
dnl                  support off.
dnl
AC_SYS_LARGEFILE

dnl ---------------- these low-level checks are needed by gnupg crypto
dnl                  and our own CDB stuff needs to know byte order
dnl
dnl                  the code up to "end gnupg configure.in" is from there
dnl

dnl ---- we need this to make CDB-format databases with little-endian numbers
dnl      the macro below is from gnupg
dnl
dnl GNUPG_CHECK_ENDIAN
dnl define either LITTLE_ENDIAN_HOST or BIG_ENDIAN_HOST
dnl
AC_DEFUN([GNUPG_CHECK_ENDIAN],
  [ if test "$cross_compiling" = yes; then
        AC_MSG_WARN(cross compiling; assuming little endianess)
    fi
    AC_MSG_CHECKING(endianess)
    AC_CACHE_VAL(gnupg_cv_c_endian,
      [ gnupg_cv_c_endian=unknown
        # See if sys/param.h defines the BYTE_ORDER macro.
        AC_TRY_COMPILE([#include <sys/types.h>
        #include <sys/param.h>], [
        #if !BYTE_ORDER || !BIG_ENDIAN || !LITTLE_ENDIAN
         bogus endian macros
        #endif], [# It does; now see whether it defined to BIG_ENDIAN or not.
        AC_TRY_COMPILE([#include <sys/types.h>
        #include <sys/param.h>], [
        #if BYTE_ORDER != BIG_ENDIAN
         not big endian
        #endif], gnupg_cv_c_endian=big, gnupg_cv_c_endian=little)])
        if test "$gnupg_cv_c_endian" = unknown; then
            AC_TRY_RUN([main () {
              /* Are we little or big endian?  From Harbison&Steele.  */
              union
              {
                long l;
                char c[sizeof (long)];
              } u;
              u.l = 1;
              exit (u.c[sizeof (long) - 1] == 1);
              }],
              gnupg_cv_c_endian=little,
              gnupg_cv_c_endian=big,
              gnupg_cv_c_endian=little
            )
        fi
      ])
    AC_MSG_RESULT([$gnupg_cv_c_endian])
    if test "$gnupg_cv_c_endian" = little; then
      AC_DEFINE(LITTLE_ENDIAN_HOST, 1, [host is little endian])
    else
      AC_DEFINE(BIG_ENDIAN_HOST, 1, [host is big endian])
    fi
  ])
GNUPG_CHECK_ENDIAN

dnl GNUPG_CHECK_TYPEDEF(TYPE, HAVE_NAME)
dnl Check whether a typedef exists and create a #define $2 if it exists
dnl
AC_DEFUN([GNUPG_CHECK_TYPEDEF],
  [ AC_MSG_CHECKING(for $1 typedef)
    AC_CACHE_VAL(gnupg_cv_typedef_$1,
    [AC_TRY_COMPILE([#define _GNU_SOURCE 1
    #include <stdlib.h>
    #include <sys/types.h>], [
    #undef $1
    int a = sizeof($1);
    ], gnupg_cv_typedef_$1=yes, gnupg_cv_typedef_$1=no )])
    AC_MSG_RESULT($gnupg_cv_typedef_$1)
    if test "$gnupg_cv_typedef_$1" = yes; then
        AC_DEFINE($2,, [has named typedef])
    fi
  ])
GNUPG_CHECK_TYPEDEF(byte, HAVE_BYTE_TYPEDEF)
GNUPG_CHECK_TYPEDEF(ushort, HAVE_USHORT_TYPEDEF)
GNUPG_CHECK_TYPEDEF(ulong, HAVE_ULONG_TYPEDEF)
GNUPG_CHECK_TYPEDEF(u16, HAVE_U16_TYPEDEF)
GNUPG_CHECK_TYPEDEF(u32, HAVE_U32_TYPEDEF)

AC_CHECK_SIZEOF(unsigned short, 2)
AC_CHECK_SIZEOF(unsigned int, 4)
AC_CHECK_SIZEOF(unsigned long, 4)
AC_CHECK_SIZEOF(long long, 0)
AC_CHECK_SIZEOF(unsigned long long, 0)
AC_CACHE_SAVE

if test "$ac_cv_sizeof_unsigned_short" = "0" \
   || test "$ac_cv_sizeof_unsigned_int" = "0" \
   || test "$ac_cv_sizeof_unsigned_long" = "0"; then
    AC_MSG_WARN([Hmmm, something is wrong with the sizes - using defaults]);
fi
dnl --------------------------------------- "end gnupg configure.in"
AC_CHECK_HEADER(stdint.h, have_stdint_h=yes, have_stdint_h=no)
AC_CHECK_HEADER(inttypes.h, have_inttypes_h=yes, have_inttypes_h=no)
if test "$have_stdint_h" = "no" && test "$have_inttypes_h" = "no"; then
  echo Warning: no system stdint.h or inttypes.h.  using stdint.h.in instead.
  echo "         check stdint.h for correctness on your system."
  cp stdint.h.in stdint.h
  AC_DEFINE(HAVE_STDINT_H,, [use stdint.h.in instead of stdint.h])
fi


dnl Checks for library functions.
AC_FUNC_MEMCMP
AC_FUNC_MMAP
dnl (only examples/integrit-run.c wants this) AC_FUNC_FORK
AC_FUNC_LSTAT
AC_FUNC_MALLOC
AC_FUNC_STRFTIME
AC_TYPE_SIGNAL
AC_FUNC_UTIME_NULL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(mkdir)
AC_CHECK_FUNC(strerror,, [cat <<ELCEOF

        Your system doesn't have strerror. exiting.

ELCEOF
exit 1
])
AC_CHECK_FUNC(strstr,, [cat <<ELCEOF

        Your system doesn't have strstr. exiting.

ELCEOF
exit 1
])
AC_CHECK_FUNCS([memmove memset munmap strchr strerror strrchr strspn strstr utime])

dnl AC_ARG_ENABLE(mmap,
dnl 	    [  --enable-mmap[=ARG]     use mmap for reading databases [ARG=yes]], 
dnl 	    AC_DEFINE(CDB_MMAP_ENABLE))

dnl ----------let the user do ./configure --enable-debug
dnl 
AC_ARG_ENABLE(debug,
	[  --enable-debug[=ARG]    compile with runtime diagnostics enabled [ARG=yes]], 
	AC_DEFINE(DEBUG,, [enable debugging]))

dnl -----------for --with-leakfind (see above)
dnl -----------... and add to the appropriate variables
if test "$with_leakfind" != ""; then
	# LIBS="$LIBS -lgc" # this happens automatically
	CPPFLAGS="$CPPFLAGS -I$with_leakfind"
	LDFLAGS="$LDFLAGS -L$with_leakfind"
	AC_DEFINE(ELC_FIND_LEAKS,, [enable leakfind])
fi

dnl if they want debugging, then don't strip the binaries on install
dnl
if test "$enable_debug" != ""; then
	STRIP=
else
	STRIP=-s
fi

dnl Configure subdirectories
dnl David Doubrava says some ftp software messes up file permissions.
dnl but adding a chmod here is not a general solution (e.g., a user 
dnl using a separate build directory who doesn't have write permission
dnl to the sources).
dnl 
AC_CONFIG_SUBDIRS(hashtbl)

dnl Perform substitutions in the output
AC_SUBST(CC)
AC_SUBST(RM)
dnl AC_SUBST(SYNC)
AC_SUBST(AR)
AC_SUBST(RANLIB)
AC_SUBST(OBJ)
AC_SUBST(ILIBOBJ)
AC_SUBST(CFLAGS)
AC_SUBST(STATIC)
AC_SUBST(EXTRA_DEFS)
AC_SUBST(DEFS)
AC_SUBST(LIBS)
AC_SUBST(CPPFLAGS)
AC_SUBST(STRIP)
AC_SUBST(inc_dir)
AC_SUBST(lib_dir)
AC_SUBST(INTEGRIT_VERSION)
AC_OUTPUT(Makefile utils/Makefile doc/Makefile doc/integrit.1 doc/i-viewdb.1 doc/i-ls.1 doc/integrit.texi)
