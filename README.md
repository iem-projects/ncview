# Ncview

The original home of Ncview is http://meteora.ucsd.edu/~pierce/ncview_home_page.html

This is a fork from Ncview 2.1.2. Ncview is (C)opyright by David W. Pierce and released under the GNU GPL v1 (see file `COPYING`).

## Building on OS X 10.6

### Preparatory step: installing HDF5

Grap the source from http://www.hdfgroup.org/ftp/HDF5/current/src/hdf5-1.8.10.tar.bz2

    $ ./configure
    $ make
    $ make install

### Preparatory step: installing NetCDF

Grap the source from http://www.unidata.ucar.edu/downloads/netcdf/ftp/netcdf-4.2.1.1.tar.gz

    $ export HDF_PATH=<prefix>/hdf5-1.8.10/hdf5/
    $ export CPPFLAGS=-I$HDF_PATH/include
    $ export LDFLAGS=-L$HDF_PATH/lib
    $ export LD_LIBRARY_PATH=$HDF_PATH/lib
    $ ./configure
    $ make
    $ sudo make install

### Building NCView

    $ ./configure CC=/usr/bin/gcc-4.2 --x-libraries=/usr/X11/lib --x-includes=/usr/X11/include

This will build without support for udunits and png. The executable will be `/usr/local/bin/ncview`.

