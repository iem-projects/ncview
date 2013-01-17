bin_PROGRAMS=ncview
noinst_PROGRAMS=geteuid
geteuid_SOURCES=geteuid.c
ncview_SOURCES=$(headers) $(sources)
ncview_LDADD=$(PNG_LIBS) $(UDUNITS2_LDFLAGS) -lm $(NETCDF_LDFLAGS) $(XAW_LIBS) $(X_PRE_LIBS) $(X_LIBS) $(X11_LIBS) $(X_EXTRA_LIBS) -lpng

headers = ncview.bitmaps.h ncview.includes.h             \
          ncview.defines.h ncview.protos.h               \
          utCalendar2_cal.h SciPlot.h SciPlotP.h 	 \
          colormaps_3gauss.h colormaps_bw.h              \
          colormaps_detail.h colormaps_helix2.h          \
          colormaps_hotres.h colormaps_3saw.h            \
          colormaps_default.h colormaps_extrema.h        \
          colormaps_helix.h colormaps_ssec.h             \
          overlay_coasts_p08deg.h overlay_coasts_p8deg.h \
          overlay_usa.h interface/closed_circle_bitmap.h \
          interface/open_circle_bitmap.h \
          interface/fallback_resources.h interface/RadioWidget.h \
          interface/helvR08.h interface/widgets.h 	\
	  calcalcs.h Stringlist.h
sources = ncview.c file.c util.c do_buttons.c             \
          file_netcdf.c   view.c do_print.c               \
          epic_time.c interface/interface.c               \
          interface/x_interface.c interface/dataedit.c    \
          interface/display_info.c interface/plot_xy.c    \
          interface/utils.c interface/range.c             \
          interface/printer_options.c overlay.c           \
          interface/filesel.c interface/set_options.c     \
          interface/plot_range.c udu.c SciPlot.c          \
          interface/RadioWidget.c interface/cbar.c	  \
	  utCalendar2_cal.c calcalcs.c 			  \
	  interface/colormap_funcs.c interface/make_tc_data.c \
	  stringlist.c handle_rc_file.c

AM_CPPFLAGS=-DNCVIEW_LIB_DIR=\"$(pkgdatadir)\" $(PNG_CPPFLAGS) $(UDUNITS2_CPPFLAGS) $(NETCDF_CPPFLAGS)
AM_CFLAGS=$(X_CFLAGS)
AM_LDFLAGS=$(PNG_LDFLAGS) $(UDUNITS2_LDFLAGS) $(NETCDF_LDFLAGS) $(X_PRE_LIBS) $(X_LIBS) $(X11_LIBS) $(X_EXTRA_LIBS) $(RPATH_FLAGS)
