/*
 * Ncview by David W. Pierce.  A visual netCDF file viewer.
 * Copyright (C) 1993 through 2010 David W. Pierce
 *
 * This program  is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, Version 3, as 
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * David W. Pierce
 * 6259 Caminito Carrena
 * San Diego, CA  92122
 * pierce@cirrus.ucsd.edu
 */

#include "ncview.includes.h"
#include "ncview.defines.h"
#include "ncview.protos.h"

/* These hold data for our colormaps */

/* A. Shchepetkin: new colormaps are added here */
#include "colormaps_bright.h"
#include "colormaps_banded.h"
#include "colormaps_rainbow.h"
#include "colormaps_jaisnb.h"
#include "colormaps_jaisnc.h"
#include "colormaps_jaisnd.h"
#include "colormaps_blu_red.h"
#include "colormaps_manga.h"
#include "colormaps_jet.h"
#include "colormaps_wheel.h"

/* the following are original colormaps from ncview */
#include "colormaps_3gauss.h"
#include "colormaps_3saw.h"
#include "colormaps_bw.h"
#include "colormaps_default.h"
#include "colormaps_detail.h"
#include "colormaps_extrema.h"
#include "colormaps_helix.h"
#include "colormaps_helix2.h"
#include "colormaps_hotres.h"
#include "colormaps_ssec.h"

/* Program defaults in a easy-to-find place */
#define DEFAULT_INVERT_PHYSICAL	FALSE
#define DEFAULT_INVERT_COLORS	FALSE
#define DEFAULT_BLOWUP		1
#define DEFAULT_MIN_MAX_METHOD	MIN_MAX_METHOD_FAST
#define DEFAULT_N_COLORS	200
#define DEFAULT_PRIVATE_CMAP	FALSE
#define DEFAULT_BLOWUP_TYPE	BLOWUP_BILINEAR
#define DEFAULT_SHRINK_METHOD	SHRINK_METHOD_MEAN
#define DEFAULT_SAVEFRAMES	TRUE
#define DEFAULT_NO_AUTOFLIP	FALSE
#define DEFAULT_LISTSEL_MAX	40
#define DEFAULT_COLOR_BY_NDIMS	TRUE
#define DEFAULT_AUTO_OVERLAY	TRUE

Options	  options;
NCVar	  *variables;
ncv_pixel *pixel_transform;
FrameStore framestore;
Stringlist *read_in_state;

static void init_cmaps_from_data();
static void init_cmap_from_data( char *colormap_name, int *data );
static int get_cmaps_from_dir( char *dir_name );

/***********************************************************************************************/
	int
main( int argc, char **argv )
{
	Stringlist *input_files, *state_to_save;
	int	   err, found_state_file;

	/* Initialize misc constants */
	initialize_misc();

	/* Read in our state file from a previous run of ncview 
	 */
	read_in_state = NULL;	/* Note: a global var. Set to null to flag following routine to make a new stringlist */
	err = read_state_from_file( &read_in_state );
	if( err == 0 ) 	
		found_state_file = TRUE;
	else
		found_state_file = FALSE;

	in_parse_args               ( &argc, argv );
	input_files = parse_options ( argc,  argv );
	determine_file_type         ( input_files );

	options.window_title = input_files->string;
	options.blowup       = 1;

	/* this routine sets up the 'variables' structure */
	initialize_file_interface   ( input_files );

	if( n_vars_in_list( variables ) == 0 ) {
		fprintf( stderr, "no displayable variables found!\n" );
		exit( -1 );
		}

	/* If any vars are in groups, we build the interface differently. 
	 * I pass this information through the global "options" struct.
	 */
	if( any_var_in_group( variables )) {
		options.enable_group_sel = TRUE;
		options.varsel_style = VARSEL_MENU;
		}
	else
		options.enable_group_sel = FALSE;

	/* This initializes the colormaps, and then the X widows system */
	if( options.debug ) printf( "Initializing display interface...\n" );
	initialize_display_interface(); 
	if( options.debug ) printf( "Initializing printing subsystem...\n" );
	print_init();
	if( options.debug ) printf( "Initializing overlays...\n" );
	overlay_init();

	/* If there is only one variable, make it the active one */
	if( n_vars_in_list( variables ) == 1 ) {
		/* set_scan_variable     ( variables       ); */
		in_indicate_active_var( variables->name );
		}

	/* If we didn't find a state file (".ncviewrc") when we started up, then
	 * write a new one out now that we are all initialized
	 */
	if( found_state_file == FALSE ) {
		state_to_save = get_persistent_state();
		if( (err = write_state_to_file( state_to_save )) != 0 ) {
			fprintf( stderr, "Error %d while trying to save options file \"$HOME/.ncviewrc\".\n", err );
			}
		stringlist_delete_entire_list( state_to_save );
		}

	process_user_input();

	return(0);
}

/***********************************************************************************************/
	Stringlist *
parse_options( int argc, char *argv[] )
{
	int	i, n;
	Stringlist *file_list = NULL;
	char	bufr[1000], *comma_ptr;

	for( i=1; i<argc; i++ ) {
		if( argv[i][0] == '-' ) {
			/* found an entry that is in option syntax */
			if( strncmp( argv[i], "-min", 4 ) == 0 ) {

				if( i == (argc-1) ) {
					fprintf( stderr, "Error, -minmax argument must be followed by one of these: fast med slow all\n" );
					exit(-1);
					}

				if( strncmp( argv[i+1], "fast", 4 ) == 0 ) {
					options.min_max_method  = MIN_MAX_METHOD_FAST;
					i++;
					}
				else if( strncmp( argv[i+1], "med", 3 ) == 0 ) {
					options.min_max_method  = MIN_MAX_METHOD_MED;
					i++;
					}
				else if( strncmp( argv[i+1], "slow", 4 ) == 0 ) {
					options.min_max_method  = MIN_MAX_METHOD_SLOW;
					i++;
					}
				else if( strncmp( argv[i+1], "exh", 3 ) == 0 ) {
					options.min_max_method  = MIN_MAX_METHOD_EXHAUST;
					i++;
					}
				else if( strncmp( argv[i+1], "all", 3 ) == 0 ) {
					options.min_max_method  = MIN_MAX_METHOD_EXHAUST;
					i++;
					}
				else
					{
					fprintf( stderr, "unrecognizied option: %s %s\n",
						argv[i], argv[i+1] );
					/* doesn't return */
					useage();
					}
				}

			else if( strncmp( argv[i], "-cal", 4 ) == 0 ) {
				options.calendar = (char *)malloc( strlen(argv[i+1] + 2));
				strcpy( options.calendar, argv[i+1] );
				i++;
				}

			else if( strncmp( argv[i], "-w", 2 ) == 0 ) {
				print_no_warranty();
				exit( 0 );
				}

			else if( strncmp( argv[i], "-pri", 4 ) == 0 )
				options.private_colormap = TRUE;

			else if( strncmp( argv[i], "-deb", 4 ) == 0 )
				options.debug = TRUE;

			else if( strncmp( argv[i], "-beep", 5 ) == 0 )
				options.beep_on_restart = TRUE;

			else if( strncmp( argv[i], "-fra", 4 ) == 0 )
				options.dump_frames = TRUE;

			else if( strncmp( argv[i], "-small", 6 ) == 0 )
				options.small = TRUE;

			else if( strncmp( argv[i], "-ext", 4 ) == 0 )
				options.want_extra_info = TRUE;

			else if( strncmp( argv[i], "-mti", 3 ) == 0 ) {
				options.window_title = argv[i+1];
				i++;
				}

			else if( strncmp( argv[i], "-noauto", 7 ) == 0 )
				options.no_autoflip = TRUE;

			else if( strncmp( argv[i], "-no1d", 5 ) == 0 )
				options.no_1d_vars = TRUE;

			else if( strncmp( argv[i], "-show_sel", 9 ) == 0 )
				options.show_sel = TRUE;

			else if( strncmp( argv[i], "-no_char_dim", 12 ) == 0 )
				options.no_char_dims = TRUE;

			else if( strncmp( argv[i], "-notconv", 8 ) == 0 )
				options.t_conv = FALSE;

			else if( strncmp( argv[i], "-shrink_mode", 12) == 0 )
				options.shrink_method = SHRINK_METHOD_MODE;

			else if( strncmp( argv[i], "-c", 2 ) == 0 ) {
				print_copying();
				exit( 0 );
				}

			else if( strncmp( argv[i], "-no_color_ndims", 7 ) == 0 ) {
				options.color_by_ndims = FALSE;
				}

			else if( strncmp( argv[i], "-no_auto_overlay", 7 ) == 0 ) {
				options.auto_overlay = FALSE;
				}

			else if( strncmp( argv[i], "-autoscale", 9 ) == 0 ) {
				options.autoscale = TRUE;
				}

			else if( strncmp( argv[i], "-listsel_max", 7 ) == 0 ) {
				sscanf( argv[i+1], "%d", &(options.listsel_max) );
				i++;
				}

			else if( strncmp( argv[i], "-nc", 3 ) == 0 ) {
				sscanf( argv[i+1], "%d", &(options.n_colors) );
				if( options.n_colors > 255 ) {
					fprintf( stderr, "maximum number of colors is currently 255\n" );
					exit( -1 );
					}
				i++;
				}

			else if( strncmp( argv[i], "-max", 4 ) == 0 ) {

				if( i == (argc-1) ) {
					fprintf( stderr, "Error, -maxsize argument must be followed by either a single integer (pct of screen) or two comma separated integers (max width,max height)\n" );
					exit(-1);
					}
				/* See if there is a comma in the following arg */
				i++;
				if( (comma_ptr = strstr( argv[i], "," )) == NULL ) {
					if( (sscanf( argv[i], "%d", &(options.maxsize_pct) ) != 1 ) ||
					    (options.maxsize_pct < 30) ||
					    (options.maxsize_pct > 100)) {
					    	fprintf( stderr, "Error, when the -maxsize arg is followed by a single number, it must be an integer between 30 and 100\n" );
						exit(-1);
						}
					}
				else
					{
					/* parse a width,height pair of ints separated by a comma */
					options.maxsize_pct = -1;	/* flag using width,height rather than pct */
					if( strlen(argv[i]) > 900 ) {
						fprintf( stderr, "Error, string specified in -maxsize too long!\n" );
						exit(-1);
						}
					n = comma_ptr - argv[i];
					strncpy( bufr, argv[i], n );
					bufr[n] = '\0';
					if( (sscanf( bufr, "%d", &(options.maxsize_width) ) != 1 ) ||
					    (options.maxsize_width < 30) ||
					    (options.maxsize_width > 99999)) {
					    	fprintf( stderr, "Error, specified -maxsize width must be an integer between 30 and 100\n" );
						exit(-1);
						}
					strcpy( bufr, argv[i]+n+1 );
					if( (sscanf( bufr, "%d", &(options.maxsize_height) ) != 1 ) ||
					    (options.maxsize_height < 30) ||
					    (options.maxsize_height > 99999)) {
					    	fprintf( stderr, "Error, specified -maxsize height must be an integer between 30 and 100\n" );
						exit(-1);
						}
					}
				}

			else	/* put other options here */
				{
				fprintf( stderr, "unrecognizied option: %s\n", argv[i] );
				/* doesn't return */
				useage();
				}
			}
		else /* found an entry which is NOT in option syntax -- assume a filename */
			stringlist_add_string( &file_list, argv[i], NULL, SLTYPE_NULL );
		} /* end of i loop through argv's */

	return( file_list );
}

/***********************************************************************************************/
	void
initialize_misc()
{
	print_disclaimer();

	udu_utinit( NULL );
	options.invert_physical  = DEFAULT_INVERT_PHYSICAL;
	options.invert_colors    = DEFAULT_INVERT_COLORS;
	options.blowup           = DEFAULT_BLOWUP;
	options.shrink_method    = DEFAULT_SHRINK_METHOD;
	options.min_max_method   = DEFAULT_MIN_MAX_METHOD;
	options.transform        = TRANSFORM_NONE;
	options.n_colors 	 = DEFAULT_N_COLORS;
	options.n_extra_colors 	 = 10;
	options.private_colormap = DEFAULT_PRIVATE_CMAP;
	options.debug		 = FALSE;
	options.show_sel	 = FALSE;
	options.want_extra_info  = FALSE;
	options.beep_on_restart  = FALSE;
	options.small  		 = FALSE;
	options.blowup_type      = DEFAULT_BLOWUP_TYPE;
	options.save_frames      = DEFAULT_SAVEFRAMES;
	options.no_autoflip      = DEFAULT_NO_AUTOFLIP;
	options.t_conv      	 = TRUE;
	options.varsel_style	 = VARSEL_LIST;
	options.dump_frames	 = FALSE;
	options.listsel_max	 = DEFAULT_LISTSEL_MAX;
	options.color_by_ndims	 = DEFAULT_COLOR_BY_NDIMS;
	options.auto_overlay	 = DEFAULT_AUTO_OVERLAY;
	options.autoscale	 = FALSE;
	options.calendar	 = NULL;

	options.overlay          = (OverlayOptions *)malloc( sizeof( OverlayOptions ));
	options.overlay->doit    = FALSE;
	options.overlay->overlay = NULL;

	options.maxsize_pct	 = 75;	/* maximum size of a window, in percent of screen, before switching to scrollbars */

	framestore.frame = NULL;
	framestore.valid = FALSE;

}

/***********************************************************************************************/
	void
initialize_colormaps()
{
	int	n_colormaps = 0;
	char	*ncview_base_dir;

	/* ncview has a useful set of built-in colormaps.  The set of built-in
	 * colormaps can be augmented by user-specified colormaps that are contained
	 * in simple ASCII files with 256 lines, where each line has 3 entries
	 * (separated by spaces), which indicate the R, B, and G values.  Each
	 * value must be an integer between 0 and 255, inclusive.  User-specified
	 * colormaps are contained in files with the extension of ".ncmap", 
	 * and can live in the following places:
	 *  1) NCVIEW_LIB_DIR, which is determined at installation time.
	 *     	A reasonable choice is "/usr/local/lib/ncview".
	 *  2) In a directory named by the environmental variable
	 *	"NCVIEWBASE".
	 *  3) If there is no environmental variable "NCVIEWBASE", then
	 *     	in $HOME.
	 *  4) In the current working directory.
	 */

	/* Get built-in colormaps */
	init_cmaps_from_data();

	/* Get user-specified colormaps, if any */
#ifdef NCVIEW_LIB_DIR
	n_colormaps  = get_cmaps_from_dir( NCVIEW_LIB_DIR );
#endif
	ncview_base_dir = (char *)getenv( "NCVIEWBASE" );
	if( ncview_base_dir == NULL )
		ncview_base_dir = (char *)getenv( "HOME" );

	if( ncview_base_dir != NULL )
		n_colormaps += get_cmaps_from_dir( ncview_base_dir );

	n_colormaps += get_cmaps_from_dir( "." );
}

/***********************************************************************************************/
	int
get_cmaps_from_dir( char *dir_name )
{
	DIR		*ncdir = NULL;
	struct dirent	*dir_entry;
	int		n_colormaps = 0;

	if( options.debug ) 
		fprintf( stderr, "Getting colormaps from dir >%s<\n", dir_name );

	ncdir     = opendir( dir_name );
	if( ncdir == NULL ) 
		return( 0 );
	dir_entry = readdir( ncdir    );
	while( dir_entry != NULL ) {
		/* Additional allowed filenames as per suggestion by 
		 * Arlindo da Silva for his Windows port
		 */
		if( (strstr( dir_entry->d_name, ".NCM"   ) != NULL) ||
		    (strstr( dir_entry->d_name, ".ncm"   ) != NULL) ||
		    (strstr( dir_entry->d_name, ".ncmap" ) != NULL) ) {
			init_cmap_from_file( dir_name, dir_entry->d_name );
			n_colormaps++;
			}
		dir_entry = readdir( ncdir );
		}
	closedir( ncdir );

	return( n_colormaps );
}

/***********************************************************************************************/
	void
init_cmaps_from_data()
{
/* the following are original colormaps from ncview */

	init_cmap_from_data( "3gauss",  cmap_3gauss  );
	init_cmap_from_data( "detail",  cmap_detail  );
	init_cmap_from_data( "ssec",    cmap_ssec    );

/* A. Shchepetkin: new colormaps are added here */

        init_cmap_from_data( "bright",  cmap_bright  );
        init_cmap_from_data( "banded",  cmap_banded  );
        init_cmap_from_data( "rainbow", cmap_rainbow );
        init_cmap_from_data( "jaisnb",  cmap_jaisnb  );
        init_cmap_from_data( "jaisnc",  cmap_jaisnc  );
        init_cmap_from_data( "jaisnd",  cmap_jaisnd  );
        init_cmap_from_data( "blu_red", cmap_blu_red );
        init_cmap_from_data( "manga",   cmap_manga   );
        init_cmap_from_data( "jet",     cmap_jet     );
        init_cmap_from_data( "wheel",   cmap_wheel   );

/* the following are the rest of the original colormaps from ncview */

	init_cmap_from_data( "3saw",    cmap_3saw    );
	init_cmap_from_data( "bw",      cmap_bw      );
	init_cmap_from_data( "default", cmap_default );
	init_cmap_from_data( "extrema", cmap_extrema );
	init_cmap_from_data( "helix",   cmap_helix   );
	init_cmap_from_data( "helix2",  cmap_helix2  );
	init_cmap_from_data( "hotres",  cmap_hotres  );
}

/***********************************************************************************************/
	void
init_cmap_from_data( char *colormap_name, int *data )
{
	int	i;
	unsigned char r[256], g[256], b[256];

	if( options.debug ) 
		fprintf( stderr, "    ... initting cmap >%s< from supplied data\n", colormap_name );

	for( i=0; i<256; i++ ) {
		r[i] = (unsigned char)data[i*3+0];
		g[i] = (unsigned char)data[i*3+1];
		b[i] = (unsigned char)data[i*3+2];
		}

	in_create_colormap( colormap_name, r, g, b );
}

/***********************************************************************************************/
	void
init_cmap_from_file( char *dir_name, char *file_name )
{
	char 	*colormap_name;
	FILE	*cmap_file;
	int	i, nentries, r_entry, g_entry, b_entry;
	char	line[ 128 ], *long_file_name;
	unsigned char r[256], g[256], b[256];
	size_t	slen;

	if( options.debug ) 
		fprintf( stderr, "    ... initting cmap >%s<\n", file_name );

	/* Colormap name is the file name without the '.ncmap' extension */
	colormap_name = (char *)malloc( strlen(file_name)-5 );
	strncpy( colormap_name, file_name, strlen(file_name)-6 );
	*(colormap_name + strlen(file_name)-6) = '\0';

	/* Make sure this colormap name isn't already known */
	if( x_seen_colormap_name( colormap_name )) {
		if( options.debug ) 
			printf( "Already have a colormap named >%s<, not NOT inittig colormap from file >%s<\n",
				colormap_name, file_name);
		return;
		}

	/* Read in the r, g, b values */
	slen = strlen(file_name) + strlen(dir_name) + 5;  /* add space for intermediate slash and trailing NULL */
	long_file_name = (char *)malloc( sizeof(char)*slen);  /* add space for intermediate slash and trailing NULL */
	snprintf( long_file_name, slen, "%s/%s", dir_name, file_name );
	if( (cmap_file = fopen( long_file_name, "r" )) == NULL ) {
		fprintf( stderr, "ncview.c: init_cmap_from_file: error " );
		fprintf( stderr, "opening file %s\n", long_file_name );
		free( long_file_name );
		return;
		}
	for( i=0; i<256; i++ ) {
		if( fgets( line, 128, cmap_file ) == NULL ) {
			fprintf( stderr, "ncview: init_cmap_from_file: file %s finished ",
					long_file_name );
			fprintf( stderr, "on line %d, but should have 256 lines\n", i+1 );
			exit( -1 );
			}

		nentries = sscanf( line, "%d %d %d", &r_entry, &g_entry, &b_entry );
		if( nentries != 3 ) {
			fprintf( stderr, "ncview: init_cmap_from_file: incorrect number " );
			fprintf( stderr, "of entries on the line.  Should be 3\n" );
			fprintf( stderr, "file %s, line %d\n", long_file_name, i+1 );
			return;
			}

		if( check( r_entry, 0, 255 ) < 0 ) {
			fprintf( stderr, "ncview: init_cmap_from_file: first entry (red) " );
			fprintf( stderr, "is outside valid limits of 0 to 255.\n" );
			fprintf( stderr, "file %s, line %d\n", long_file_name, i+1 );
			return;
			}

		if( check( g_entry, 0, 255 ) < 0 ) {
			fprintf( stderr, "ncview: init_cmap_from_file: second entry (green) " );
			fprintf( stderr, "is outside valid limits of 0 to 255.\n" );
			fprintf( stderr, "file %s, line %d\n", long_file_name, i+1 );
			return;
			}

		if( check( b_entry, 0, 255 ) < 0 ) {
			fprintf( stderr, "ncview: init_cmap_from_file: third entry (blue) " );
			fprintf( stderr, "is outside valid limits of 0 to 255.\n" );
			fprintf( stderr, "file %s, line %d\n", long_file_name, i+1 );
			return;
			}
		r[i] = (unsigned char)r_entry;
		g[i] = (unsigned char)g_entry;
		b[i] = (unsigned char)b_entry;
		}

	in_create_colormap( colormap_name, r, g, b );

	free( long_file_name );
}

/***********************************************************************************************/
	void
initialize_file_interface( Stringlist *input_files )
{
	int	i, idim, nvars, nfiles;
	NCVar	*var;

	if( options.debug ) 
		fprintf( stderr, "Initializing file interface...\n" );

	nfiles = stringlist_len( input_files );

	while( input_files != NULL ) {
		fi_initialize( input_files->string, nfiles );
		input_files = input_files->next;
		}
	if( options.debug ) 
		fprintf( stderr, "...calculating dim min & maxes...\n" );
	calc_dim_minmaxes();

	/* Get the effective dimensionality of all the vars.
	 * Can't do this before we have read in all of the
	 * input file.
	 */
	nvars = 0;
	var = variables;
	while( var != NULL ) {
		nvars++;
		var->effective_dimensionality = 0;
		for( idim=0; idim<var->n_dims; idim++ ) {
			if( *(var->size + idim) > 1 )
				var->effective_dimensionality++;
			if( options.debug ) 
				fprintf( stderr, "var %s has %d dims, dim %d: >%s< len %ld\n",
					var->name, var->n_dims, idim, 
					var->dim[idim]->name, var->dim[idim]->size );
			}
		if( options.debug ) {
			fprintf( stderr, "variable %s had effective_dimensionality of %d\n",
				var->name, var->effective_dimensionality );
			}
		var = var->next;
		}

	/* Now that we have read in all the files, we can
	 * gather any scalar coordinate information (which
	 * might possibly change in each file)
	 */
	cache_scalar_coord_info( variables );

	if( nvars > options.listsel_max )
		options.varsel_style = VARSEL_MENU;

	if( options.debug ) 
		fprintf( stderr, "Done initializing file interface...\n" );
}

/***********************************************************************************************/
	void
initialize_display_interface()
{
	initialize_colormaps();

	/* Make the colormaps in the program congruent in order
	 * and "enabled-ness" with the read-in state
	 */
	x_check_legal_colormap_loaded();

	if( options.debug ) printf( "...initializing X interface\n" );
	in_initialize();
	if( options.debug ) printf( "...done with initializing X interface\n" );
}

/***********************************************************************************************/
	void
process_user_input()
{
	/* This call never returns, as it loops, handling user interface events */
	in_process_user_input();
}

/***********************************************************************************************/
/* Only correct exit point for 'ncview' */
	void
quit_app()
{
	exit( 0 );
}

/***********************************************************************************************/
	int
check( int val, int min, int max )
{
	if( (val >= min) && (val <= max) )
		return( 0 );
	else
		return( -1 );
}

/***********************************************************************************************/
/* Make a quickie colormap in case none is found */
	void
create_default_colormap()
{
	ncv_pixel	r[256], g[256], b[256];
	int		i;

	for( i=0; i<256; i++ ) {
		r[i] = i;
		g[i] = 255 - abs(i-128);
		b[i] = 255-i;
		}

	in_create_colormap( "default", r, g, b );
}

/***********************************************************************************************/
int any_var_in_group( NCVar *var ) {

	NCVar	*cursor;

	cursor = var;
	while( cursor != NULL ) {
		if( count_nslashes( cursor->name ) > 0 ) 
			return( 1 );
		cursor = cursor->next;
		}

	return( 0 );
}

/***********************************************************************************************/
	void
useage()
{
fprintf( stderr, "\nuseage:\n" );
fprintf( stderr, "ncview [options] datafiles\n" );
fprintf( stderr, "\n" );
fprintf( stderr, "Options\n" );
fprintf( stderr, "	-minmax: selects how rapidly minimum and maximum\n" );
fprintf( stderr, "		values in the data files will be determined;\n" );
fprintf( stderr, "		by scanning every third time entry (\"-minmax fast\"),\n" );
fprintf( stderr, "		every fifth time entry (\"-minmax med\"), every tenth\n" );
fprintf( stderr, "		(\"-minmax slow\"), or all entries (\"-minmax all\").\n" );
fprintf( stderr, "	-frames: Dump out PNG images (to make a movie, for instance)\n" );
fprintf( stderr, "	-nc: 	Specify number of colors to use.\n" );
fprintf( stderr, "	-no1d: 	Do NOT allow 1-D variables to be displayed.\n" );
fprintf( stderr, "	-calendar: Specify time calendar to use, overriding value in file. Known: noleap standard gregorian 365_day 360_day.\n" );
fprintf( stderr, "	-private: Use a private colormap.\n" );
fprintf( stderr, "	-debug: Print lots of debugging info.\n" );
fprintf( stderr, "	-beep: 	Ring the bell when the movie restarts at frame zero.\n" );
fprintf( stderr, "	-extra: Put some extra information on the display window.\n" );
fprintf( stderr, "	-mtitle: My title to use on the display window.\n" );
fprintf( stderr, "	-noautoflip: Do not automatically flip image, even\n" );
fprintf( stderr, "		if dimensions indicate that it would make sense.\n" );
fprintf( stderr, "	-w: 	print the lack-of-warranty blurb.\n" );
fprintf( stderr, "	-small: Keep popup window as small as possible by default.\n" );
fprintf( stderr, "	-shrink_mode: Shrink image assuming integer classes, so most common\n" );
fprintf( stderr, "		value in sub-block returned instead of arithmetic mean.\n" );
fprintf( stderr, "	-listsel_max NN: max number of vars allowed before switching to menu selection\n");
fprintf( stderr, "	-no_color_ndims: do NOT color the var selection buttons by their dimensionality\n" );
fprintf( stderr, "	-no_auto_overlay: do NOT automatically put on continental overlays\n" );
fprintf( stderr, "	-autoscale: scale color map of EACH frame to range of that frame. Note: MUCH SLOWER!\n" );
fprintf( stderr, "	-maxsize: specifies max size of window before scrollbars are added. Either a single\n" );
fprintf( stderr, "              integer between 30 and 100 giving percentage, or two integers separated by a\n" );
fprintf( stderr, "              comma giving width and height. Ex: -maxsize 75  or -maxsize 800,600\n" );
fprintf( stderr, "	-c: 	print the copying policy.\n" );
fprintf( stderr, "datafiles:\n" );
fprintf( stderr, "	You can have up to 32 of these.  They must all be in\n" );
fprintf( stderr, "	the same general format, or have different variables in\n" );
fprintf( stderr, "	them.  Ncview tries its best under such circumstances.\n" );

exit( -1 );
}

/***********************************************************************************************/
	void
print_disclaimer()
{
fprintf( stderr, "%s\n", PROGRAM_ID );
fprintf( stderr, "http://meteora.ucsd.edu:80/~pierce/ncview_home_page.html\n" );
fprintf( stderr, "Copyright (C) 1993 through 2011, David W. Pierce\n" );
fprintf( stderr, "Ncview comes with ABSOLUTELY NO WARRANTY; for details type `ncview -w'.\n" );
fprintf( stderr, "This is free software licensed under the Gnu General Public License version 3; type `ncview -c' for redistribution details.\n\n" );
}

/***********************************************************************************************/
	void
print_no_warranty()
{
printf( "\n The program `ncview' is Copyright (C) 1993 through 2010 David W. Pierce, and\n" );
printf( "is subject to the terms and conditions of the Gnu General Public License,\n" );
printf( "Version 3. For information on copying, modifying, or distributing `ncview',\n" );
printf( "type `ncview -c'.\n" );
printf( "\n" );
printf( "  This License Agreement applies to any program or other work which \n" );
printf( "contains a notice placed by the copyright holder saying it may be \n" );
printf( "distributed under the terms of this General Public License.  The\n" );
printf( "\"Program\", below, refers to any such program or work, and a \"work based\n" );
printf( "on the Program\" means either the Program or any work containing the\n" );
printf( "Program or a portion of it, either verbatim or with modifications.  Each\n" );
printf( "licensee is addressed as \"you\".\n" );
printf( "\n" );
printf( "                            NO WARRANTY\n" );
printf( "\n" );
printf( "  BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY\n" );
printf( "FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN\n" );
printf( "OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES\n" );
printf( "PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED\n" );
printf( "OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n" );
printf( "MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS\n" );
printf( "TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE\n" );
printf( "PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,\n" );
printf( "REPAIR OR CORRECTION.\n" );
printf( "\n" );
printf( "  IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING\n" );
printf( "WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR\n" );
printf( "REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,\n" );
printf( "INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING\n" );
printf( "OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED\n" );
printf( "TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY\n" );
printf( "YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER\n" );
printf( "PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE\n" );
printf( "POSSIBILITY OF SUCH DAMAGES.\n" );
}

/***********************************************************************************************/
	void 
print_copying()
{
printf( "  The program `ncview' is Copyright (C) 1993 through 2010, David W. Pierce, and \n" );
printf( "is subject to the terms and conditions of the Gnu General Public License,\n" );
printf( "Version 3.  Ncview comes with NO WARRANTY; for further information, type\n" );
printf( "`ncview -w'.\n" );
printf( "\n" );
printf( "GNU GENERAL PUBLIC LICENSE\n" );
printf( "\n" );
printf( "Version 3, 29 June 2007\n" );
printf( "\n" );
printf( "Copyright (C) 2007 Free Software Foundation, Inc. <http://fsf.org/>\n" );
printf( "\n" );
printf( "Everyone is permitted to copy and distribute verbatim copies of this license\n" );
printf( "document, but changing it is not allowed.\n" );
printf( "\n" );
printf( "Preamble\n" );
printf( "\n" );
printf( "The GNU General Public License is a free, copyleft license for software and\n" );
printf( "other kinds of works.\n" );
printf( "\n" );
printf( "The licenses for most software and other practical works are designed to take\n" );
printf( "away your freedom to share and change the works. By contrast, the GNU General\n" );
printf( "Public License is intended to guarantee your freedom to share and change all\n" );
printf( "versions of a program--to make sure it remains free software for all its users.\n" );
printf( "We, the Free Software Foundation, use the GNU General Public License for most\n" );
printf( "of our software; it applies also to any other work released this way by its\n" );
printf( "authors. You can apply it to your programs, too.\n" );
printf( "\n" );
printf( "When we speak of free software, we are referring to freedom, not price. Our\n" );
printf( "General Public Licenses are designed to make sure that you have the freedom to\n" );
printf( "distribute copies of free software (and charge for them if you wish), that you\n" );
printf( "receive source code or can get it if you want it, that you can change the\n" );
printf( "software or use pieces of it in new free programs, and that you know you can do\n" );
printf( "these things.\n" );
printf( "\n" );
printf( "To protect your rights, we need to prevent others from denying you these rights\n" );
printf( "or asking you to surrender the rights. Therefore, you have certain\n" );
printf( "responsibilities if you distribute copies of the software, or if you modify it:\n" );
printf( "responsibilities to respect the freedom of others.\n" );
printf( "\n" );
printf( "For example, if you distribute copies of such a program, whether gratis or for\n" );
printf( "a fee, you must pass on to the recipients the same freedoms that you received.\n" );
printf( "You must make sure that they, too, receive or can get the source code. And you\n" );
printf( "must show them these terms so they know their rights.\n" );
printf( "\n" );
printf( "Developers that use the GNU GPL protect your rights with two steps: (1) assert\n" );
printf( "copyright on the software, and (2) offer you this License giving you legal\n" );
printf( "permission to copy, distribute and/or modify it.\n" );
printf( "\n" );
printf( "For the developers' and authors' protection, the GPL clearly explains that\n" );
printf( "there is no warranty for this free software. For both users' and authors' sake,\n" );
printf( "the GPL requires that modified versions be marked as changed, so that their\n" );
printf( "problems will not be attributed erroneously to authors of previous versions.\n" );
printf( "\n" );
printf( "Some devices are designed to deny users access to install or run modified\n" );
printf( "versions of the software inside them, although the manufacturer can do so. This\n" );
printf( "is fundamentally incompatible with the aim of protecting users' freedom to\n" );
printf( "change the software. The systematic pattern of such abuse occurs in the area of\n" );
printf( "products for individuals to use, which is precisely where it is most\n" );
printf( "unacceptable. Therefore, we have designed this version of the GPL to prohibit\n" );
printf( "the practice for those products. If such problems arise substantially in other\n" );
printf( "domains, we stand ready to extend this provision to those domains in future\n" );
printf( "versions of the GPL, as needed to protect the freedom of users.\n" );
printf( "\n" );
printf( "Finally, every program is threatened constantly by software patents. States\n" );
printf( "should not allow patents to restrict development and use of software on\n" );
printf( "general-purpose computers, but in those that do, we wish to avoid the special\n" );
printf( "danger that patents applied to a free program could make it effectively\n" );
printf( "proprietary. To prevent this, the GPL assures that patents cannot be used to\n" );
printf( "render the program non-free.\n" );
printf( "\n" );
printf( "The precise terms and conditions for copying, distribution and modification\n" );
printf( "follow.\n" );
printf( "\n" );
printf( "TERMS AND CONDITIONS\n" );
printf( "\n" );
printf( "0. Definitions.\n" );
printf( "\n" );
printf( "\"This License\" refers to version 3 of the GNU General Public License.\n" );
printf( "\n" );
printf( "\"Copyright\" also means copyright-like laws that apply to other kinds of works,\n" );
printf( "such as semiconductor masks.\n" );
printf( "\n" );
printf( "\"The Program\" refers to any copyrightable work licensed under this License.\n" );
printf( "Each licensee is addressed as \"you\". \"Licensees\" and \"recipients\" may be\n" );
printf( "individuals or organizations.\n" );
printf( "\n" );
printf( "To \"modify\" a work means to copy from or adapt all or part of the work in a\n" );
printf( "fashion requiring copyright permission, other than the making of an exact copy.\n" );
printf( "The resulting work is called a \"modified version\" of the earlier work or a work\n" );
printf( "\"based on\" the earlier work.\n" );
printf( "\n" );
printf( "A \"covered work\" means either the unmodified Program or a work based on the\n" );
printf( "Program.\n" );
printf( "\n" );
printf( "To \"propagate\" a work means to do anything with it that, without permission,\n" );
printf( "would make you directly or secondarily liable for infringement under applicable\n" );
printf( "copyright law, except executing it on a computer or modifying a private copy.\n" );
printf( "Propagation includes copying, distribution (with or without modification),\n" );
printf( "making available to the public, and in some countries other activities as well.\n" );
printf( "\n" );
printf( "To \"convey\" a work means any kind of propagation that enables other parties to\n" );
printf( "make or receive copies. Mere interaction with a user through a computer\n" );
printf( "network, with no transfer of a copy, is not conveying.\n" );
printf( "\n" );
printf( "An interactive user interface displays \"Appropriate Legal Notices\" to the\n" );
printf( "extent that it includes a convenient and prominently visible feature that (1)\n" );
printf( "displays an appropriate copyright notice, and (2) tells the user that there is\n" );
printf( "no warranty for the work (except to the extent that warranties are provided),\n" );
printf( "that licensees may convey the work under this License, and how to view a copy\n" );
printf( "of this License. If the interface presents a list of user commands or options,\n" );
printf( "such as a menu, a prominent item in the list meets this criterion.\n" );
printf( "\n" );
printf( "1. Source Code.\n" );
printf( "\n" );
printf( "The \"source code\" for a work means the preferred form of the work for making\n" );
printf( "modifications to it. \"Object code\" means any non-source form of a work.\n" );
printf( "\n" );
printf( "A \"Standard Interface\" means an interface that either is an official standard\n" );
printf( "defined by a recognized standards body, or, in the case of interfaces specified\n" );
printf( "for a particular programming language, one that is widely used among developers\n" );
printf( "working in that language.\n" );
printf( "\n" );
printf( "The \"System Libraries\" of an executable work include anything, other than the\n" );
printf( "work as a whole, that (a) is included in the normal form of packaging a Major\n" );
printf( "Component, but which is not part of that Major Component, and (b) serves only\n" );
printf( "to enable use of the work with that Major Component, or to implement a Standard\n" );
printf( "Interface for which an implementation is available to the public in source code\n" );
printf( "form. A \"Major Component\", in this context, means a major essential component\n" );
printf( "(kernel, window system, and so on) of the specific operating system (if any) on\n" );
printf( "which the executable work runs, or a compiler used to produce the work, or an\n" );
printf( "object code interpreter used to run it.\n" );
printf( "\n" );
printf( "The \"Corresponding Source\" for a work in object code form means all the source\n" );
printf( "code needed to generate, install, and (for an executable work) run the object\n" );
printf( "code and to modify the work, including scripts to control those activities.\n" );
printf( "However, it does not include the work's System Libraries, or general-purpose\n" );
printf( "tools or generally available free programs which are used unmodified in\n" );
printf( "performing those activities but which are not part of the work. For example,\n" );
printf( "Corresponding Source includes interface definition files associated with source\n" );
printf( "files for the work, and the source code for shared libraries and dynamically\n" );
printf( "linked subprograms that the work is specifically designed to require, such as\n" );
printf( "by intimate data communication or control flow between those subprograms and\n" );
printf( "other parts of the work.\n" );
printf( "\n" );
printf( "The Corresponding Source need not include anything that users can regenerate\n" );
printf( "automatically from other parts of the Corresponding Source.\n" );
printf( "\n" );
printf( "The Corresponding Source for a work in source code form is that same work.\n" );
printf( "\n" );
printf( "2. Basic Permissions.\n" );
printf( "\n" );
printf( "All rights granted under this License are granted for the term of copyright on\n" );
printf( "the Program, and are irrevocable provided the stated conditions are met. This\n" );
printf( "License explicitly affirms your unlimited permission to run the unmodified\n" );
printf( "Program. The output from running a covered work is covered by this License only\n" );
printf( "if the output, given its content, constitutes a covered work. This License\n" );
printf( "acknowledges your rights of fair use or other equivalent, as provided by\n" );
printf( "copyright law.\n" );
printf( "\n" );
printf( "You may make, run and propagate covered works that you do not convey, without\n" );
printf( "conditions so long as your license otherwise remains in force. You may convey\n" );
printf( "covered works to others for the sole purpose of having them make modifications\n" );
printf( "exclusively for you, or provide you with facilities for running those works,\n" );
printf( "provided that you comply with the terms of this License in conveying all\n" );
printf( "material for which you do not control copyright. Those thus making or running\n" );
printf( "the covered works for you must do so exclusively on your behalf, under your\n" );
printf( "direction and control, on terms that prohibit them from making any copies of\n" );
printf( "your copyrighted material outside their relationship with you.\n" );
printf( "\n" );
printf( "Conveying under any other circumstances is permitted solely under the\n" );
printf( "conditions stated below. Sublicensing is not allowed; section 10 makes it\n" );
printf( "unnecessary.\n" );
printf( "\n" );
printf( "3. Protecting Users' Legal Rights From Anti-Circumvention Law.\n" );
printf( "\n" );
printf( "No covered work shall be deemed part of an effective technological measure\n" );
printf( "under any applicable law fulfilling obligations under article 11 of the WIPO\n" );
printf( "copyright treaty adopted on 20 December 1996, or similar laws prohibiting or\n" );
printf( "restricting circumvention of such measures.\n" );
printf( "\n" );
printf( "When you convey a covered work, you waive any legal power to forbid\n" );
printf( "circumvention of technological measures to the extent such circumvention is\n" );
printf( "effected by exercising rights under this License with respect to the covered\n" );
printf( "work, and you disclaim any intention to limit operation or modification of the\n" );
printf( "work as a means of enforcing, against the work's users, your or third parties'\n" );
printf( "legal rights to forbid circumvention of technological measures.\n" );
printf( "\n" );
printf( "4. Conveying Verbatim Copies.\n" );
printf( "\n" );
printf( "You may convey verbatim copies of the Program's source code as you receive it,\n" );
printf( "in any medium, provided that you conspicuously and appropriately publish on\n" );
printf( "each copy an appropriate copyright notice; keep intact all notices stating that\n" );
printf( "this License and any non-permissive terms added in accord with section 7 apply\n" );
printf( "to the code; keep intact all notices of the absence of any warranty; and give\n" );
printf( "all recipients a copy of this License along with the Program.\n" );
printf( "\n" );
printf( "You may charge any price or no price for each copy that you convey, and you may\n" );
printf( "offer support or warranty protection for a fee.\n" );
printf( "\n" );
printf( "5. Conveying Modified Source Versions.\n" );
printf( "\n" );
printf( "You may convey a work based on the Program, or the modifications to produce it\n" );
printf( "from the Program, in the form of source code under the terms of section 4,\n" );
printf( "provided that you also meet all of these conditions:\n" );
printf( "\n" );
printf( "    * a) The work must carry prominent notices stating that you modified it,\n" );
printf( "and giving a relevant date.\n" );
printf( "    * b) The work must carry prominent notices stating that it is released\n" );
printf( "under this License and any conditions added under section 7. This requirement\n" );
printf( "modifies the requirement in section 4 to \"keep intact all notices\".\n" );
printf( "    * c) You must license the entire work, as a whole, under this License to\n" );
printf( "anyone who comes into possession of a copy. This License will therefore apply,\n" );
printf( "along with any applicable section 7 additional terms, to the whole of the work,\n" );
printf( "and all its parts, regardless of how they are packaged. This License gives no\n" );
printf( "permission to license the work in any other way, but it does not invalidate\n" );
printf( "such permission if you have separately received it.\n" );
printf( "    * d) If the work has interactive user interfaces, each must display\n" );
printf( "Appropriate Legal Notices; however, if the Program has interactive interfaces\n" );
printf( "that do not display Appropriate Legal Notices, your work need not make them do\n" );
printf( "so.\n" );
printf( "\n" );
printf( "A compilation of a covered work with other separate and independent works,\n" );
printf( "which are not by their nature extensions of the covered work, and which are not\n" );
printf( "combined with it such as to form a larger program, in or on a volume of a\n" );
printf( "storage or distribution medium, is called an \"aggregate\" if the compilation and\n" );
printf( "its resulting copyright are not used to limit the access or legal rights of the\n" );
printf( "compilation's users beyond what the individual works permit. Inclusion of a\n" );
printf( "covered work in an aggregate does not cause this License to apply to the other\n" );
printf( "parts of the aggregate.\n" );
printf( "\n" );
printf( "6. Conveying Non-Source Forms.\n" );
printf( "\n" );
printf( "You may convey a covered work in object code form under the terms of sections 4\n" );
printf( "and 5, provided that you also convey the machine-readable Corresponding Source\n" );
printf( "under the terms of this License, in one of these ways:\n" );
printf( "\n" );
printf( "    * a) Convey the object code in, or embodied in, a physical product\n" );
printf( "(including a physical distribution medium), accompanied by the Corresponding\n" );
printf( "Source fixed on a durable physical medium customarily used for software\n" );
printf( "interchange.\n" );
printf( "    * b) Convey the object code in, or embodied in, a physical product\n" );
printf( "(including a physical distribution medium), accompanied by a written offer,\n" );
printf( "valid for at least three years and valid for as long as you offer spare parts\n" );
printf( "or customer support for that product model, to give anyone who possesses the\n" );
printf( "object code either (1) a copy of the Corresponding Source for all the software\n" );
printf( "in the product that is covered by this License, on a durable physical medium\n" );
printf( "customarily used for software interchange, for a price no more than your\n" );
printf( "reasonable cost of physically performing this conveying of source, or (2)\n" );
printf( "access to copy the Corresponding Source from a network server at no charge.\n" );
printf( "    * c) Convey individual copies of the object code with a copy of the written\n" );
printf( "offer to provide the Corresponding Source. This alternative is allowed only\n" );
printf( "occasionally and noncommercially, and only if you received the object code with\n" );
printf( "such an offer, in accord with subsection 6b.\n" );
printf( "    * d) Convey the object code by offering access from a designated place\n" );
printf( "(gratis or for a charge), and offer equivalent access to the Corresponding\n" );
printf( "Source in the same way through the same place at no further charge. You need\n" );
printf( "not require recipients to copy the Corresponding Source along with the object\n" );
printf( "code. If the place to copy the object code is a network server, the\n" );
printf( "Corresponding Source may be on a different server (operated by you or a third\n" );
printf( "party) that supports equivalent copying facilities, provided you maintain clear\n" );
printf( "directions next to the object code saying where to find the Corresponding\n" );
printf( "Source. Regardless of what server hosts the Corresponding Source, you remain\n" );
printf( "obligated to ensure that it is available for as long as needed to satisfy these\n" );
printf( "requirements.\n" );
printf( "    * e) Convey the object code using peer-to-peer transmission, provided you\n" );
printf( "inform other peers where the object code and Corresponding Source of the work\n" );
printf( "are being offered to the general public at no charge under subsection 6d.\n" );
printf( "\n" );
printf( "A separable portion of the object code, whose source code is excluded from the\n" );
printf( "Corresponding Source as a System Library, need not be included in conveying the\n" );
printf( "object code work.\n" );
printf( "\n" );
printf( "A \"User Product\" is either (1) a \"consumer product\", which means any tangible\n" );
printf( "personal property which is normally used for personal, family, or household\n" );
printf( "purposes, or (2) anything designed or sold for incorporation into a dwelling.\n" );
printf( "In determining whether a product is a consumer product, doubtful cases shall be\n" );
printf( "resolved in favor of coverage. For a particular product received by a\n" );
printf( "particular user, \"normally used\" refers to a typical or common use of that\n" );
printf( "class of product, regardless of the status of the particular user or of the way\n" );
printf( "in which the particular user actually uses, or expects or is expected to use,\n" );
printf( "the product. A product is a consumer product regardless of whether the product\n" );
printf( "has substantial commercial, industrial or non-consumer uses, unless such uses\n" );
printf( "represent the only significant mode of use of the product.\n" );
printf( "\n" );
printf( "\"Installation Information\" for a User Product means any methods, procedures,\n" );
printf( "authorization keys, or other information required to install and execute\n" );
printf( "modified versions of a covered work in that User Product from a modified\n" );
printf( "version of its Corresponding Source. The information must suffice to ensure\n" );
printf( "that the continued functioning of the modified object code is in no case\n" );
printf( "prevented or interfered with solely because modification has been made.\n" );
printf( "\n" );
printf( "If you convey an object code work under this section in, or with, or\n" );
printf( "specifically for use in, a User Product, and the conveying occurs as part of a\n" );
printf( "transaction in which the right of possession and use of the User Product is\n" );
printf( "transferred to the recipient in perpetuity or for a fixed term (regardless of\n" );
printf( "how the transaction is characterized), the Corresponding Source conveyed under\n" );
printf( "this section must be accompanied by the Installation Information. But this\n" );
printf( "requirement does not apply if neither you nor any third party retains the\n" );
printf( "ability to install modified object code on the User Product (for example, the\n" );
printf( "work has been installed in ROM).\n" );
printf( "\n" );
printf( "The requirement to provide Installation Information does not include a\n" );
printf( "requirement to continue to provide support service, warranty, or updates for a\n" );
printf( "work that has been modified or installed by the recipient, or for the User\n" );
printf( "Product in which it has been modified or installed. Access to a network may be\n" );
printf( "denied when the modification itself materially and adversely affects the\n" );
printf( "operation of the network or violates the rules and protocols for communication\n" );
printf( "across the network.\n" );
printf( "\n" );
printf( "Corresponding Source conveyed, and Installation Information provided, in accord\n" );
printf( "with this section must be in a format that is publicly documented (and with an\n" );
printf( "implementation available to the public in source code form), and must require\n" );
printf( "no special password or key for unpacking, reading or copying.\n" );
printf( "\n" );
printf( "7. Additional Terms.\n" );
printf( "\n" );
printf( "\"Additional permissions\" are terms that supplement the terms of this License by\n" );
printf( "making exceptions from one or more of its conditions. Additional permissions\n" );
printf( "that are applicable to the entire Program shall be treated as though they were\n" );
printf( "included in this License, to the extent that they are valid under applicable\n" );
printf( "law. If additional permissions apply only to part of the Program, that part may\n" );
printf( "be used separately under those permissions, but the entire Program remains\n" );
printf( "governed by this License without regard to the additional permissions.\n" );
printf( "\n" );
printf( "When you convey a copy of a covered work, you may at your option remove any\n" );
printf( "additional permissions from that copy, or from any part of it. (Additional\n" );
printf( "permissions may be written to require their own removal in certain cases when\n" );
printf( "you modify the work.) You may place additional permissions on material, added\n" );
printf( "by you to a covered work, for which you have or can give appropriate copyright\n" );
printf( "permission.\n" );
printf( "\n" );
printf( "Notwithstanding any other provision of this License, for material you add to a\n" );
printf( "covered work, you may (if authorized by the copyright holders of that material)\n" );
printf( "supplement the terms of this License with terms:\n" );
printf( "\n" );
printf( "    * a) Disclaiming warranty or limiting liability differently from the terms\n" );
printf( "of sections 15 and 16 of this License; or\n" );
printf( "    * b) Requiring preservation of specified reasonable legal notices or author\n" );
printf( "attributions in that material or in the Appropriate Legal Notices displayed by\n" );
printf( "works containing it; or\n" );
printf( "    * c) Prohibiting misrepresentation of the origin of that material, or\n" );
printf( "requiring that modified versions of such material be marked in reasonable ways\n" );
printf( "as different from the original version; or\n" );
printf( "    * d) Limiting the use for publicity purposes of names of licensors or\n" );
printf( "authors of the material; or\n" );
printf( "    * e) Declining to grant rights under trademark law for use of some trade\n" );
printf( "names, trademarks, or service marks; or\n" );
printf( "    * f) Requiring indemnification of licensors and authors of that material by\n" );
printf( "anyone who conveys the material (or modified versions of it) with contractual\n" );
printf( "assumptions of liability to the recipient, for any liability that these\n" );
printf( "contractual assumptions directly impose on those licensors and authors.\n" );
printf( "\n" );
printf( "All other non-permissive additional terms are considered \"further restrictions\"\n" );
printf( "within the meaning of section 10. If the Program as you received it, or any\n" );
printf( "part of it, contains a notice stating that it is governed by this License along\n" );
printf( "with a term that is a further restriction, you may remove that term. If a\n" );
printf( "license document contains a further restriction but permits relicensing or\n" );
printf( "conveying under this License, you may add to a covered work material governed\n" );
printf( "by the terms of that license document, provided that the further restriction\n" );
printf( "does not survive such relicensing or conveying.\n" );
printf( "\n" );
printf( "If you add terms to a covered work in accord with this section, you must place,\n" );
printf( "in the relevant source files, a statement of the additional terms that apply to\n" );
printf( "those files, or a notice indicating where to find the applicable terms.\n" );
printf( "\n" );
printf( "Additional terms, permissive or non-permissive, may be stated in the form of a\n" );
printf( "separately written license, or stated as exceptions; the above requirements\n" );
printf( "apply either way.\n" );
printf( "\n" );
printf( "8. Termination.\n" );
printf( "\n" );
printf( "You may not propagate or modify a covered work except as expressly provided\n" );
printf( "under this License. Any attempt otherwise to propagate or modify it is void,\n" );
printf( "and will automatically terminate your rights under this License (including any\n" );
printf( "patent licenses granted under the third paragraph of section 11).\n" );
printf( "\n" );
printf( "However, if you cease all violation of this License, then your license from a\n" );
printf( "particular copyright holder is reinstated (a) provisionally, unless and until\n" );
printf( "the copyright holder explicitly and finally terminates your license, and (b)\n" );
printf( "permanently, if the copyright holder fails to notify you of the violation by\n" );
printf( "some reasonable means prior to 60 days after the cessation.\n" );
printf( "\n" );
printf( "Moreover, your license from a particular copyright holder is reinstated\n" );
printf( "permanently if the copyright holder notifies you of the violation by some\n" );
printf( "reasonable means, this is the first time you have received notice of violation\n" );
printf( "of this License (for any work) from that copyright holder, and you cure the\n" );
printf( "violation prior to 30 days after your receipt of the notice.\n" );
printf( "\n" );
printf( "Termination of your rights under this section does not terminate the licenses\n" );
printf( "of parties who have received copies or rights from you under this License. If\n" );
printf( "your rights have been terminated and not permanently reinstated, you do not\n" );
printf( "qualify to receive new licenses for the same material under section 10.\n" );
printf( "\n" );
printf( "9. Acceptance Not Required for Having Copies.\n" );
printf( "\n" );
printf( "You are not required to accept this License in order to receive or run a copy\n" );
printf( "of the Program. Ancillary propagation of a covered work occurring solely as a\n" );
printf( "consequence of using peer-to-peer transmission to receive a copy likewise does\n" );
printf( "not require acceptance. However, nothing other than this License grants you\n" );
printf( "permission to propagate or modify any covered work. These actions infringe\n" );
printf( "copyright if you do not accept this License. Therefore, by modifying or\n" );
printf( "propagating a covered work, you indicate your acceptance of this License to do\n" );
printf( "so.\n" );
printf( "\n" );
printf( "10. Automatic Licensing of Downstream Recipients.\n" );
printf( "\n" );
printf( "Each time you convey a covered work, the recipient automatically receives a\n" );
printf( "license from the original licensors, to run, modify and propagate that work,\n" );
printf( "subject to this License. You are not responsible for enforcing compliance by\n" );
printf( "third parties with this License.\n" );
printf( "\n" );
printf( "An \"entity transaction\" is a transaction transferring control of an\n" );
printf( "organization, or substantially all assets of one, or subdividing an\n" );
printf( "organization, or merging organizations. If propagation of a covered work\n" );
printf( "results from an entity transaction, each party to that transaction who receives\n" );
printf( "a copy of the work also receives whatever licenses to the work the party's\n" );
printf( "predecessor in interest had or could give under the previous paragraph, plus a\n" );
printf( "right to possession of the Corresponding Source of the work from the\n" );
printf( "predecessor in interest, if the predecessor has it or can get it with\n" );
printf( "reasonable efforts.\n" );
printf( "\n" );
printf( "You may not impose any further restrictions on the exercise of the rights\n" );
printf( "granted or affirmed under this License. For example, you may not impose a\n" );
printf( "license fee, royalty, or other charge for exercise of rights granted under this\n" );
printf( "License, and you may not initiate litigation (including a cross-claim or\n" );
printf( "counterclaim in a lawsuit) alleging that any patent claim is infringed by\n" );
printf( "making, using, selling, offering for sale, or importing the Program or any\n" );
printf( "portion of it.\n" );
printf( "\n" );
printf( "11. Patents.\n" );
printf( "\n" );
printf( "A \"contributor\" is a copyright holder who authorizes use under this License of\n" );
printf( "the Program or a work on which the Program is based. The work thus licensed is\n" );
printf( "called the contributor's \"contributor version\".\n" );
printf( "\n" );
printf( "A contributor's \"essential patent claims\" are all patent claims owned or\n" );
printf( "controlled by the contributor, whether already acquired or hereafter acquired,\n" );
printf( "that would be infringed by some manner, permitted by this License, of making,\n" );
printf( "using, or selling its contributor version, but do not include claims that would\n" );
printf( "be infringed only as a consequence of further modification of the contributor\n" );
printf( "version. For purposes of this definition, \"control\" includes the right to grant\n" );
printf( "patent sublicenses in a manner consistent with the requirements of this\n" );
printf( "License.\n" );
printf( "\n" );
printf( "Each contributor grants you a non-exclusive, worldwide, royalty-free patent\n" );
printf( "license under the contributor's essential patent claims, to make, use, sell,\n" );
printf( "offer for sale, import and otherwise run, modify and propagate the contents of\n" );
printf( "its contributor version.\n" );
printf( "\n" );
printf( "In the following three paragraphs, a \"patent license\" is any express agreement\n" );
printf( "or commitment, however denominated, not to enforce a patent (such as an express\n" );
printf( "permission to practice a patent or covenant not to sue for patent\n" );
printf( "infringement). To \"grant\" such a patent license to a party means to make such\n" );
printf( "an agreement or commitment not to enforce a patent against the party.\n" );
printf( "\n" );
printf( "If you convey a covered work, knowingly relying on a patent license, and the\n" );
printf( "Corresponding Source of the work is not available for anyone to copy, free of\n" );
printf( "charge and under the terms of this License, through a publicly available\n" );
printf( "network server or other readily accessible means, then you must either (1)\n" );
printf( "cause the Corresponding Source to be so available, or (2) arrange to deprive\n" );
printf( "yourself of the benefit of the patent license for this particular work, or (3)\n" );
printf( "arrange, in a manner consistent with the requirements of this License, to\n" );
printf( "extend the patent license to downstream recipients. \"Knowingly relying\" means\n" );
printf( "you have actual knowledge that, but for the patent license, your conveying the\n" );
printf( "covered work in a country, or your recipient's use of the covered work in a\n" );
printf( "country, would infringe one or more identifiable patents in that country that\n" );
printf( "you have reason to believe are valid.\n" );
printf( "\n" );
printf( "If, pursuant to or in connection with a single transaction or arrangement, you\n" );
printf( "convey, or propagate by procuring conveyance of, a covered work, and grant a\n" );
printf( "patent license to some of the parties receiving the covered work authorizing\n" );
printf( "them to use, propagate, modify or convey a specific copy of the covered work,\n" );
printf( "then the patent license you grant is automatically extended to all recipients\n" );
printf( "of the covered work and works based on it.\n" );
printf( "\n" );
printf( "A patent license is \"discriminatory\" if it does not include within the scope of\n" );
printf( "its coverage, prohibits the exercise of, or is conditioned on the non-exercise\n" );
printf( "of one or more of the rights that are specifically granted under this License.\n" );
printf( "You may not convey a covered work if you are a party to an arrangement with a\n" );
printf( "third party that is in the business of distributing software, under which you\n" );
printf( "make payment to the third party based on the extent of your activity of\n" );
printf( "conveying the work, and under which the third party grants, to any of the\n" );
printf( "parties who would receive the covered work from you, a discriminatory patent\n" );
printf( "license (a) in connection with copies of the covered work conveyed by you (or\n" );
printf( "copies made from those copies), or (b) primarily for and in connection with\n" );
printf( "specific products or compilations that contain the covered work, unless you\n" );
printf( "entered into that arrangement, or that patent license was granted, prior to 28\n" );
printf( "March 2007.\n" );
printf( "\n" );
printf( "Nothing in this License shall be construed as excluding or limiting any implied\n" );
printf( "license or other defenses to infringement that may otherwise be available to\n" );
printf( "you under applicable patent law.\n" );
printf( "\n" );
printf( "12. No Surrender of Others' Freedom.\n" );
printf( "\n" );
printf( "If conditions are imposed on you (whether by court order, agreement or\n" );
printf( "otherwise) that contradict the conditions of this License, they do not excuse\n" );
printf( "you from the conditions of this License. If you cannot convey a covered work so\n" );
printf( "as to satisfy simultaneously your obligations under this License and any other\n" );
printf( "pertinent obligations, then as a consequence you may not convey it at all. For\n" );
printf( "example, if you agree to terms that obligate you to collect a royalty for\n" );
printf( "further conveying from those to whom you convey the Program, the only way you\n" );
printf( "could satisfy both those terms and this License would be to refrain entirely\n" );
printf( "from conveying the Program.\n" );
printf( "\n" );
printf( "13. Use with the GNU Affero General Public License.\n" );
printf( "\n" );
printf( "Notwithstanding any other provision of this License, you have permission to\n" );
printf( "link or combine any covered work with a work licensed under version 3 of the\n" );
printf( "GNU Affero General Public License into a single combined work, and to convey\n" );
printf( "the resulting work. The terms of this License will continue to apply to the\n" );
printf( "part which is the covered work, but the special requirements of the GNU Affero\n" );
printf( "General Public License, section 13, concerning interaction through a network\n" );
printf( "will apply to the combination as such.\n" );
printf( "\n" );
printf( "14. Revised Versions of this License.\n" );
printf( "\n" );
printf( "The Free Software Foundation may publish revised and/or new versions of the GNU\n" );
printf( "General Public License from time to time. Such new versions will be similar in\n" );
printf( "spirit to the present version, but may differ in detail to address new problems\n" );
printf( "or concerns.\n" );
printf( "\n" );
printf( "Each version is given a distinguishing version number. If the Program specifies\n" );
printf( "that a certain numbered version of the GNU General Public License .or any later\n" );
printf( "version. applies to it, you have the option of following the terms and\n" );
printf( "conditions either of that numbered version or of any later version published by\n" );
printf( "the Free Software Foundation. If the Program does not specify a version number\n" );
printf( "of the GNU General Public License, you may choose any version ever published by\n" );
printf( "the Free Software Foundation.\n" );
printf( "\n" );
printf( "If the Program specifies that a proxy can decide which future versions of the\n" );
printf( "GNU General Public License can be used, that proxy's public statement of\n" );
printf( "acceptance of a version permanently authorizes you to choose that version for\n" );
printf( "the Program.\n" );
printf( "\n" );
printf( "Later license versions may give you additional or different permissions.\n" );
printf( "However, no additional obligations are imposed on any author or copyright\n" );
printf( "holder as a result of your choosing to follow a later version.\n" );
printf( "\n" );
printf( "15. Disclaimer of Warranty.\n" );
printf( "\n" );
printf( "THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE\n" );
printf( "LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER\n" );
printf( "PARTIES PROVIDE THE PROGRAM .AS IS. WITHOUT WARRANTY OF ANY KIND, EITHER\n" );
printf( "EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n" );
printf( "MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE\n" );
printf( "QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE\n" );
printf( "DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR\n" );
printf( "CORRECTION.\n" );
printf( "\n" );
printf( "16. Limitation of Liability.\n" );
printf( "\n" );
printf( "IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL ANY\n" );
printf( "COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS THE PROGRAM AS\n" );
printf( "PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY GENERAL, SPECIAL,\n" );
printf( "INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE\n" );
printf( "THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED\n" );
printf( "INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE\n" );
printf( "PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY\n" );
printf( "HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.\n" );
printf( "\n" );
printf( "17. Interpretation of Sections 15 and 16.\n" );
printf( "\n" );
printf( "If the disclaimer of warranty and limitation of liability provided above cannot\n" );
printf( "be given local legal effect according to their terms, reviewing courts shall\n" );
printf( "apply local law that most closely approximates an absolute waiver of all civil\n" );
printf( "liability in connection with the Program, unless a warranty or assumption of\n" );
printf( "liability accompanies a copy of the Program in return for a fee.\n" );
printf( "\n" );
printf( "END OF TERMS AND CONDITIONS\n" );
printf( "\n" );
printf( "How to Apply These Terms to Your New Programs\n" );
printf( "\n" );
printf( "If you develop a new program, and you want it to be of the greatest possible\n" );
printf( "use to the public, the best way to achieve this is to make it free software\n" );
printf( "which everyone can redistribute and change under these terms.\n" );
printf( "\n" );
printf( "To do so, attach the following notices to the program. It is safest to attach\n" );
printf( "them to the start of each source file to most effectively state the exclusion\n" );
printf( "of warranty; and each file should have at least the .copyright. line and a\n" );
printf( "pointer to where the full notice is found.\n" );
printf( "\n" );
printf( "    <one line to give the program's name and a brief idea of what it does.>\n" );
printf( "    Copyright (C) <year>  <name of author>\n" );
printf( "\n" );
printf( "    This program is free software: you can redistribute it and/or modify\n" );
printf( "    it under the terms of the GNU General Public License as published by\n" );
printf( "    the Free Software Foundation, either version 3 of the License, or\n" );
printf( "    (at your option) any later version.\n" );
printf( "\n" );
printf( "    This program is distributed in the hope that it will be useful,\n" );
printf( "    but WITHOUT ANY WARRANTY; without even the implied warranty of\n" );
printf( "    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n" );
printf( "    GNU General Public License for more details.\n" );
printf( "\n" );
printf( "    You should have received a copy of the GNU General Public License\n" );
printf( "    along with this program.  If not, see <http://www.gnu.org/licenses/>.\n" );
printf( "\n" );
printf( "Also add information on how to contact you by electronic and paper mail.\n" );
printf( "\n" );
printf( "If the program does terminal interaction, make it output a short notice like\n" );
printf( "this when it starts in an interactive mode:\n" );
printf( "\n" );
printf( "    <program>  Copyright (C) <year>  <name of author>\n" );
printf( "    This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.\n" );
printf( "    This is free software, and you are welcome to redistribute it\n" );
printf( "    under certain conditions; type `show c' for details.\n" );
printf( "\n" );
printf( "The hypothetical commands `show w' and `show c' should show the appropriate\n" );
printf( "parts of the General Public License. Of course, your program's commands might\n" );
printf( "be different; for a GUI interface, you would use an \"about box\".\n" );
printf( "\n" );
printf( "You should also get your employer (if you work as a programmer) or school, if\n" );
printf( "any, to sign a \"copyright disclaimer\" for the program, if necessary. For more\n" );
printf( "information on this, and how to apply and follow the GNU GPL, see\n" );
printf( "<http://www.gnu.org/licenses/>.\n" );
printf( "\n" );
printf( "The GNU General Public License does not permit incorporating your program into\n" );
printf( "proprietary programs. If your program is a subroutine library, you may consider\n" );
printf( "it more useful to permit linking proprietary applications with the library. If\n" );
printf( "this is what you want to do, use the GNU Lesser General Public License instead\n" );
printf( "of this License. But first, please read\n" );
printf( "<http://www.gnu.org/philosophy/why-not-lgpl.html>.\n" );
printf( "\n" );
}
