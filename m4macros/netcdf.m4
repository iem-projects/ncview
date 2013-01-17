dnl
dnl AC_PATH_NETCDF()
dnl
dnl This macro looks for version 4 of the netcdf library.  Specifically, it looks for a
dnl version-4 netcdf library that includes the script "nc-config", which is queried
dnl for the information.  "nc-config" first appeared in netcdf release 4.1-beta2, which
dnl is what this is tested with.  
dnl
dnl Defines NETCDF_LIBS, NETCDF_CPPFLAGS, and NETCDF_LDFLAGS, or exits with an error message.
dnl I.e., the intent is to have this used when the application REQUIRES the netcdf v4 library.
dnl
dnl version 1.0    
dnl 21 Jan 2010
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
AC_DEFUN([AC_PATH_NETCDF],[
AC_ARG_WITH( nc_config, [  --with-nc-config=path to nc-config script from netcdf v4 package],  NC_CONFIG=$withval)
dnl
dnl
dnl =================================================================================
dnl check for netcdf include directory
dnl
err=0
if test x$NC_CONFIG = x; then
	NC_CONFIG_SHORT=nc-config
	NC_CONFIG_FULLQUAL=nc-config
	NC_CONFIG_PATH=$PATH
else
	echo "user specified nc-config is $NC_CONFIG"
	NC_CONFIG_SHORT=`basename $NC_CONFIG`
	NC_CONFIG_FULLQUAL=$NC_CONFIG
	NC_CONFIG_PATH=`dirname $NC_CONFIG`
fi
AC_CHECK_PROG( HAS_NC_CONFIG, [$NC_CONFIG_SHORT], [yes], [no], [$NC_CONFIG_PATH] )
if test x$HAS_NC_CONFIG = xno; then
	echo "-----------------------------------------------------------------------------------"
	echo "Error, nc-config not found or not executable.  This is a script that comes with the"
	echo "netcdf library, version 4.1-beta2 or later, and must be present for configuration"
	echo "to succeed."
	echo " "
	echo "If you installed the netcdf library (and nc-config) in a standard location, nc-config"
	echo "should be found automatically.  Otherwise, you can specify the full path and name of"
	echo "the nc-config script by passing the --with-nc-config=/full/path/nc-config argument"
	echo "flag to the configure script.  For example:"
	echo " "
	echo "./configure --with-nc-config=/sw/dist/netcdf4/nc-config"
	echo " "
	echo "Special note for R users:"
	echo "-------------------------"
	echo "To pass the configure flag to R, use something like this:"
	echo " "
	echo "R CMD INSTALL --configure-args=\"--with-nc-config=/home/joe/bin/nc-config\" ncdf4"
	echo " "
	echo "where you should replace /home/joe/bin etc. with the location where you have"
	echo "installed the nc-config script that came with the netcdf 4 distribution."
	echo "-----------------------------------------------------------------------------------"
	exit -1
fi
dnl
NETCDF_CC=`$NC_CONFIG_FULLQUAL --cc`
NETCDF_LDFLAGS=`$NC_CONFIG_FULLQUAL --libs`
NETCDF_CPPFLAGS=`$NC_CONFIG_FULLQUAL --cflags`
NETCDF_VERSION=`$NC_CONFIG_FULLQUAL --version`
dnl
dnl The following will be either "yes" or "no"
NETCDF_V4=`$NC_CONFIG_FULLQUAL --has-nc4`
dnl
dnl If we get here, we assume that netcdf exists.  It might not if, for example,
dnl the package was installed and nc-config is present, but then the libraries
dnl were erased.  Assume such deliberately broken behavior is not the case.
dnl
AC_DEFINE([HAVE_NETCDF],1,[Define if you have the NETCDF library, either v3 or v4])
dnl
if test x$NETCDF_V4 = xyes; then
	AC_DEFINE([HAVE_NETCDF4],1,[Define if you have version 4 of the NETCDF library])
fi
dnl -----------------------------------------------------------------------------------
dnl At this piont, $NETCDF_V4 will be either "yes" or "no"
dnl
echo "Netcdf library version: $NETCDF_VERSION"
echo "Netcdf library has version 4 interface present: $NETCDF_V4"
echo "Netcdf library was compiled with C compiler: $NETCDF_CC"
dnl
dnl Export our variables
dnl
AC_SUBST(NETCDF_CPPFLAGS)
AC_SUBST(NETCDF_LDFLAGS)
AC_SUBST(NETCDF_LIBS)
AC_SUBST(NETCDF_VERSION)
AC_SUBST(NETCDF_CC)
dnl
])
