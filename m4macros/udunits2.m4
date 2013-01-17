dnl
dnl AC_PATH_UDUNITS2()
dnl
dnl Defines UDUNITS2_INCDIR and UDUNITS2_LIBDIR if they exist, and leaves them undefined
dnl otherwise.
dnl
dnl version 0.2    
dnl 21 Feb 2010
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
AC_DEFUN([AC_PATH_UDUNITS2],[
AC_ARG_WITH( udunits2_incdir, [  --with-udunits2_incdir=dir directory containing udunits2 includes],  UDUNITS2_INCDIR=$withval)
dnl
dnl
dnl =================================================================================
dnl check for udunits2 include directory
dnl
err=0
if test x$UDUNITS2_INCDIR != x; then
        AC_CHECK_HEADER( $UDUNITS2_INCDIR/converter.h,
          echo "Using user-specified udunits2 include dir=$UDUNITS2_INCDIR",
          err=1 )
	if test $err -eq 1; then
		echo "--------------------------------------------------------------------------------"
		echo "Error: You specified that the udunits-2 include directory is $UDUNITS2_INCDIR"
		echo "but that directory does NOT have required udunits-2 include file converter.h."
		echo "You must specify a directory that has ALL the udunits-2 include files"
		echo "(converter.h as well as udunits2.h)."
		echo "--------------------------------------------------------------------------------"
		exit -1
	fi
        AC_CHECK_HEADER( $UDUNITS2_INCDIR/udunits2.h,
          echo "Using user-specified udunits2 include dir=$UDUNITS2_INCDIR",
          err=1 )
fi
if test $err -eq 1; then
        echo "Error: user specified udunits2 include directory does not have udunits2.h!"
        exit -1
fi
if test x$UDUNITS2_INCDIR = x; then
        AC_CHECK_HEADER( udunits2.h, UDUNITS2_INCDIR=. )
fi
if test x$UDUNITS2_INCDIR = x; then
        AC_CHECK_HEADER( /usr/local/include/udunits2.h, UDUNITS2_INCDIR=/usr/local/include )
fi
if test x$UDUNITS2_INCDIR = x; then
        AC_CHECK_HEADER( /usr/include/udunits2.h, UDUNITS2_INCDIR=/usr/include )
fi
if test x$UDUNITS2_INCDIR = x; then
        AC_CHECK_HEADER( $HOME/include/udunits2.h, UDUNITS2_INCDIR=$HOME/include )
fi
if test x$UDUNITS2_INCDIR = x; then
        AC_CHECK_HEADER( /sw/include/udunits2.h, UDUNITS2_INCDIR=/sw/include )
fi
dnl
UDUNITS2_INC_PRESENT=no
if test x$UDUNITS2_INCDIR != x; then
	UDUNITS2_INC_PRESENT=yes
        UDUNITS2_CPPFLAGS=-I$UDUNITS2_INCDIR
else
	echo "** Could not find the udunits2.h file, udunits support will not be included **"
fi
dnl =================================================================================
dnl check for udunits2 lib directory
dnl
UDUNITS2_LIBNAME=libudunits2.a
dnl
AC_ARG_WITH( udunits2_libdir, [  --with-udunits2_libdir=dir directory containing udunits2 library],  UDUNITS2_LIBDIR=$withval)
err=0
if test x$UDUNITS2_LIBDIR != x; then
        AC_CHECK_FILE( $UDUNITS2_LIBDIR/$UDUNITS2_LIBNAME,
          echo "Using user-specified udunits2 library dir=$UDUNITS2_LIBDIR",
          err=1 )
fi
if test $err -eq 1; then
        echo "Error: user specified udunits2 library directory does not have $UDUNITS2_LIBNAME !"
        exit -1
fi
if test x$NETCDF_LIBDIR = x; then
        AC_CHECK_LIB( udunits2, ut_read_xml, UDUNITS2_LIBDIR=. )
fi
if test x$UDUNITS2_LIBDIR = x; then
        AC_CHECK_FILE( /usr/local/lib/$UDUNITS2_LIBNAME, UDUNITS2_LIBDIR=/usr/local/lib )
fi
if test x$UDUNITS2_LIBDIR = x; then
        AC_CHECK_FILE( /usr/lib/$UDUNITS2_LIBNAME, UDUNITS2_LIBDIR=/usr/lib )
fi
if test x$UDUNITS2_LIBDIR = x; then
        AC_CHECK_FILE( /lib/$UDUNITS2_LIBNAME, UDUNITS2_LIBDIR=/lib )
fi
if test x$UDUNITS2_LIBDIR = x; then
        AC_CHECK_FILE( $HOME/lib/$UDUNITS2_LIBNAME, UDUNITS2_LIBDIR=$HOME/lib )
fi
if test x$UDUNITS2_LIBDIR = x; then
        AC_CHECK_FILE( /sw/lib/$UDUNITS2_LIBNAME, UDUNITS2_LIBDIR=/sw/lib )
fi
UDUNITS2_LIBNAME=`echo $UDUNITS2_LIBNAME | sed s/lib// | sed s/\.a//`
dnl
UDUNITS2_PRESENT=no
if test $UDUNITS2_INC_PRESENT = yes; then
	if test x$UDUNITS2_LIBDIR != x; then
		UDUNITS2_PRESENT=yes
		UDUNITS2_LIBS=-l$UDUNITS2_LIBNAME
		UDUNITS2_LDFLAGS="-L$UDUNITS2_LIBDIR $UDUNITS2_LIBS"
		AC_DEFINE([HAVE_UDUNITS2],1,[Define if you have udunits-2 library])
	fi
fi
dnl
dnl ------------------------------------------------------------------------
dnl New feature of udunits2: it now depends on the expat library.  Add that
dnl to the flags as well.
dnl ------------------------------------------------------------------------
if test $UDUNITS2_PRESENT = yes; then
	haveExpat=yes
	LIBSsave=$LIBS
	CFLAGSsave=$CFLAGS
	LIBS=$UDUNITS2_LDFLAGS
	AC_MSG_CHECKING([for expat library (required by udunits2)])
	AC_MSG_RESULT()
	AC_CHECK_LIB(expat,XML_GetBase,[],[haveExpat=no])
	if test x"$haveExpat"x != "xyesx"; then
	  AC_MSG_ERROR([Could not find the expat library despite the fact that the udunits2 library is installed.  This probably means some sort of configuration file error. Please consult with the package maintainer.])
	  exit -1
	fi
	EXPAT_LIBS=$LIBS
	echo "Expat libraries (needed by udunits2): $EXPAT_LIBS"
	UDUNITS2_LDFLAGS="$UDUNITS2_LDFLAGS $LIBS"
	LIBS=$LIBSsave
	CFLAGS=$CFLAGSsave
fi

dnl Export our variables
dnl
AC_SUBST(UDUNITS2_CPPFLAGS)
AC_SUBST(UDUNITS2_LDFLAGS)
AC_SUBST(UDUNITS2_LIBS)
dnl
])
