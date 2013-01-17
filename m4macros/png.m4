dnl
dnl AC_PATH_PNG()
dnl
dnl Defines PNG_INCDIR and PNG_LIBDIR if they exist, and leaves them undefined
dnl otherwise.
dnl
dnl 13 July 2011
dnl David W. Pierce
dnl Climate Research Division
dnl Scripps Institution of Oceanography
dnl dpierce@ucsd.edu
dnl
dnl *******************************************************************************
dnl This code is in the public domain, and can be used for any purposes whatsoever. 
dnl *******************************************************************************
dnl 
dnl
AC_DEFUN([AC_PATH_PNG],[
AC_ARG_WITH( png_incdir, [  --with-png_incdir=dir directory containing png includes],  PNG_INCDIR=$withval)
dnl
dnl
dnl =================================================================================
dnl check for png include directory
dnl
err=0
if test x$PNG_INCDIR != x; then
        AC_CHECK_HEADER( $PNG_INCDIR/png.h,
          echo "Using user-specified png include dir=$PNG_INCDIR",
          err=1 )
	if test $err -eq 1; then
		echo "--------------------------------------------------------------------------------"
		echo "Error: You specified that the pnginclude directory is $PNG_INCDIR"
		echo "but that directory does NOT have required png include file png.h."
		echo "You must specify a directory that has the png include files"
		echo "--------------------------------------------------------------------------------"
		exit -1
	fi
fi
if test $err -eq 1; then
        echo "Error: user specified png include directory does not have png.h!"
        exit -1
fi
if test x$PNG_INCDIR = x; then
        AC_CHECK_HEADER( png.h, PNG_INCDIR=. )
fi
if test x$PNG_INCDIR = x; then
        AC_CHECK_HEADER( /usr/local/include/png.h, PNG_INCDIR=/usr/local/include )
fi
if test x$PNG_INCDIR = x; then
        AC_CHECK_HEADER( /usr/include/png.h, PNG_INCDIR=/usr/include )
fi
if test x$PNG_INCDIR = x; then
        AC_CHECK_HEADER( $HOME/include/png.h, PNG_INCDIR=$HOME/include )
fi
if test x$PNG_INCDIR = x; then
        AC_CHECK_HEADER( /sw/include/png.h, PNG_INCDIR=/sw/include )
fi
dnl
PNG_INC_PRESENT=no
if test x$PNG_INCDIR != x; then
	PNG_INC_PRESENT=yes
        PNG_CPPFLAGS=-I$PNG_INCDIR
else
	echo "** Could not find the png.h file, so -frames support will not be included  **"
	echo "** Install the PNG library (and development headers) to fix this           **"
fi
dnl =================================================================================
dnl check for png lib directory
dnl
PNG_LIBNAME=libpng.so
dnl
AC_ARG_WITH( png_libdir, [  --with-png_libdir=dir directory containing png library],  PNG_LIBDIR=$withval)
err=0
if test x$PNG_LIBDIR != x; then
        AC_CHECK_FILE( $PNG_LIBDIR/$PNG_LIBNAME,
          echo "Using user-specified png library dir=$PNG_LIBDIR",
          err=1 )
fi
if test $err -eq 1; then
        echo "Error: user specified png library directory does not have $PNG_LIBNAME !"
        exit -1
fi
if test x$PNG_LIBDIR = x; then
        AC_CHECK_LIB( png, png_write_png, PNG_LIBDIR=. )
fi
if test x$PNG_LIBDIR = x; then
        AC_CHECK_FILE( /usr/local/lib/$PNG_LIBNAME, PNG_LIBDIR=/usr/local/lib )
fi
if test x$PNG_LIBDIR = x; then
        AC_CHECK_FILE( /usr/lib/$PNG_LIBNAME, PNG_LIBDIR=/usr/lib )
fi
if test x$PNG_LIBDIR = x; then
        AC_CHECK_FILE( /lib/$PNG_LIBNAME, PNG_LIBDIR=/lib )
fi
if test x$PNG_LIBDIR = x; then
        AC_CHECK_FILE( $HOME/lib/$PNG_LIBNAME, PNG_LIBDIR=$HOME/lib )
fi
if test x$PNG_LIBDIR = x; then
        AC_CHECK_FILE( /sw/lib/$PNG_LIBNAME, PNG_LIBDIR=/sw/lib )
fi
PNG_LIBNAME=`echo $PNG_LIBNAME | sed s/lib// | sed s/\.so//`
dnl
PNG_PRESENT=no
if test $PNG_INC_PRESENT = yes; then
	if test x$PNG_LIBDIR != x; then
		PNG_PRESENT=yes
		PNG_LIBS=-l$PNG_LIBNAME
		PNG_LDFLAGS="-L$PNG_LIBDIR $PNG_LIBS"
		AC_DEFINE([HAVE_PNG],1,[Define if you have PNG library])
	fi
fi
dnl
dnl Export our variables
dnl
AC_SUBST(PNG_CPPFLAGS)
AC_SUBST(PNG_LDFLAGS)
AC_SUBST(PNG_LIBS)
dnl
])
