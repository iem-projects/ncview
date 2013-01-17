/*
 * Ncview by David W. Pierce.  A visual netCDF file viewer.
 * Copyright (C) 1993-2010 David W. Pierce
 *
 * This program  is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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

#include "../ncview.includes.h"
#include "../ncview.defines.h"
#include "../ncview.protos.h"

#define CMAPLIST_MAGIC		378218
#define CMAPLIST_BAD_MAGIC	129802

/********************************************/
extern Stringlist	*read_in_state;
extern Widget 		topLevel;
extern Server_Info	server;
extern Cmaplist		*colormap_list, *current_colormap_list;		/* These are MY structures */
extern Colormap		current_colormap;				/* This is the standard X windows struct */
extern Options		options;
extern ncv_pixel 	*pixel_transform;
/********************************************/

void dump_Cmaplist	( Cmaplist *cml );

static Cmaplist *dup_single_cmaplist( Cmaplist *el );
static void new_cmaplist( Cmaplist **cml );
static Cmaplist *x_find_colormap_from_name( char *name );
static int check_cmaplist_magic( Cmaplist *cml );

/***************************************************************************************************/
/* Given a Cmaplist, this returns a duplicate of it.  This is a little confusing since 
 * a list of these things (Cmaplist stucts) is just a bunch of individual Cmaplist structs.
 * In any event, routine "dup_whole_cmaplist" duplicates an entire list, while routine
 * "dup_single_cmaplist" duplicates a single Cmaplist struct, not the entire chain of
 * Cmaplist structs.
 */
Cmaplist *dup_whole_cmaplist( Cmaplist *src_list )
{
	Cmaplist	*cml, *newel, *prevel=NULL, *list_head=NULL;

	if( src_list == NULL )
		return( NULL );

	cml = src_list;
	while( cml != NULL ) {
		
		if( check_cmaplist_magic( cml ) != 0 ) {
			fprintf( stderr, "Error, trying to duplicate a cmaplist with an invalid element\n" );
			exit(-1);
			}

		newel = dup_single_cmaplist( cml );	/* has NULL for ->next and ->prev elements */
		if( prevel == NULL ) 
			list_head = newel;
		else
			{
			prevel->next = newel;
			newel->prev  = prevel;
			}
		prevel = newel;
		cml = cml->next;
		}

	return( list_head );
}

/***************************************************************************************************/
/* Dumplicates a SINGLE cmaplist element, not an entire chain of them.  Leaves NULL in the
 * ->next and ->prev elements.
 */
static Cmaplist *dup_single_cmaplist( Cmaplist *el )
{
	Cmaplist	*newel;
	int		i;

	/* This allocates space for pixel_transform and color_list */
	new_cmaplist( &newel );

	newel->name = (char *)malloc( sizeof(char)*(strlen(el->name)+1));
	strcpy( newel->name, el->name );

	newel->enabled = el->enabled;

	for( i=0; i<(options.n_colors+options.n_extra_colors); i++ ) {
		newel->pixel_transform[i] = el->pixel_transform[i];
		newel->color_list     [i] = el->color_list[i];
		}

	return( newel );
}

/***************************************************************************************************
 * Return a pointer to the colormap list entry that has this name, or NULL if this name is 
 * not that of a colormap on the list
 */
static Cmaplist *x_find_colormap_from_name( char *name )
{
	Cmaplist	*cml;

        cml = colormap_list;
        while( cml != NULL ) {

		if( check_cmaplist_magic( cml ) != 0 ) {
			fprintf( stderr, "Error, trying to find a colormap from name in a cmaplist with an invalid element\n" );
			exit(-1);
			}

                if( strcmp(cml->name, name) == 0 )
                        return(cml);
                cml = cml->next;
                }

        return(NULL);
}

/***************************************************************************************************/
/* Return 1 if we alrady have a colormap with the name of the passed argument in the specified
 * list of colormaps, and 0 otherwise
 */
int x_seen_colormap_name( char *name )
{
	if( x_find_colormap_from_name(name) == NULL ) 
		return(0);
	else
		return(1);
}

/***************************************************************************************************
 * Given an index into the colormap list, this sets
 * "name" to point to storage for the colormap's name,
 * sets "enabled" to 1 if the colormap is enabled and to
 * 0 otherwise, and sets "color_list" to point to an
 * array of N XColors.
 */
void x_colormap_info( Cmaplist *cmlist, int idx, char **name, int *enabled, XColor **color_list )
{
        Cmaplist        *cml;
        int             i;

        if( idx < 0 ) {
                fprintf( stderr, "Error, bad idx entry in x_colormap_name: %d\n", idx );
                exit(-1);
                }

        if( cmlist == NULL ) {
                fprintf( stderr, "Error, x_colormap_name called but no colormaps are on the list" );
                exit(-1);
                }

        cml = cmlist;
        for( i=0; i<idx; i++ ) {

		if( check_cmaplist_magic( cml ) != 0 ) {
			fprintf( stderr, "Error, trying to find colormap information in a cmaplist with an invalid element\n" );
			exit(-1);
			}

                if( cml->next == NULL ) {
                        fprintf( stderr, "Error, x_colormap_name called with idx=%d but n_colormaps=%d\n",
                                idx, x_n_colormaps( cmlist ) );
                        exit(-1);
                        }
                cml = cml->next;
                }

        *name           = cml->name;
        *enabled        = cml->enabled;
        *color_list     = cml->color_list;
}

/***************************************************************************************************/
int x_n_colormaps( Cmaplist *cmlist )
{
        Cmaplist        *cml;
        int             n;

        if( cmlist == NULL )
                return( 0 );

        n = 1;
        cml = cmlist;
        while( cml->next != NULL ) {

		if( check_cmaplist_magic( cml ) != 0 ) {
			fprintf( stderr, "Error, trying to compute colormap length in a cmaplist with an invalid element\n" );
			exit(-1);
			}

                cml = cml->next;
                n++;
                }

        return(n);
}

/***************************************************************************************************/
void x_check_legal_colormap_loaded()
{
	Cmaplist	*cm, **list_to_use, *tcm;
	int		n_enabled, n_cm_saved, n_cm_prog, n_cm_max, i, i_cursor, n_list_to_use=0, *cmap_used_already;
	Stringlist	*sl;
	char		cm_name[1000];

	/* We have the following tasks:
	 * 	1) Make the order of the in-program colormaps be the same
	 *	   as the order in the save file.
	 *	2) Make the enabled-ness of the colormaps in the program
	 *	   be the same as the save file.
	 *	3) Make sure current_colormap_list is set to a valid colormap 
	 * Any colormaps that are in the program but not in the save file
	 * are enabled and included at the end of the colormap list.
	 */
	n_cm_saved = stringlist_len( read_in_state );
	n_cm_prog  = x_n_colormaps( colormap_list );
	n_cm_max   = n_cm_saved + n_cm_prog;		/* max possible colormaps we could have */
	if( n_cm_max > 200 ) {
		fprintf( stderr, "Error, too many colormaps -- max is 200\n" );
		exit(-1);
		}
	if( n_cm_max < 1 ) {
		fprintf( stderr, "Internal error -- n_cm_max == 0?!?\n" );
		exit(-1);
		}

	list_to_use = (Cmaplist **)malloc( sizeof( Cmaplist * ) * n_cm_max );
	if( list_to_use == NULL ) {
		fprintf( stderr, "Error allocating a temporary colormap list pointer\n" );
		exit(-1);
		}
	for( i=0; i<n_cm_max; i++ )
		list_to_use[i] = NULL;		/* initialize to "not set" */

	cmap_used_already = (int *)malloc( sizeof(int) * n_cm_saved );
	if( cmap_used_already == NULL ) {
		fprintf( stderr, "Error allocating a temporary colormap array\n" );
		exit(-1);
		}
	for( i=0; i<n_cm_saved; i++ )
		cmap_used_already[i] = 0;

	/* Start with the colormaps in the save file, put those
	 * on the list first
	 */
	sl = read_in_state;
	i_cursor = 0;
	while( sl != NULL ) {
		/* Get name of colormap that we read in from the save file.
		 * Remember that they are prepended with "CMAP_"
		 */
		if( strncmp( sl->string, "CMAP_", 5 ) == 0 ) {
			if( strlen( sl->string ) > 900 ) {
				fprintf( stderr, "Error, saved colormap name too long (remove $HOME/.ncviewrc file if stuck)\n" );
				exit(-1);
				}
			/* This is a saved colormap, extract its name */
			strcpy( cm_name, sl->string+5 );

			/* Look for this colormap on the program's colormap list */
			tcm = x_find_colormap_from_name( cm_name );

			/* If we found this colormap, save it to our list */
			if( tcm == NULL ) 
				printf( "Note: Program does not know about saved colormap \"%s\" -- ignoring it\n", cm_name );
			else
				{
				if( cmap_used_already[i_cursor] == 0 ) {
					list_to_use[ n_list_to_use++ ] = tcm;
					cmap_used_already[i_cursor] = 1;
					}
				}
			}
		sl = sl->next;
		i_cursor++;
		}

	/* Now add on any colormaps that the program knows about but
	 * which were not in the save file
	 */
	cm = colormap_list;
	while( cm != NULL ) {
		if( strlen(cm->name) > 900 ) {
			fprintf( stderr, "Error, colormap name is too long: %s\n", cm->name );
			exit(-1);
			}
		snprintf( cm_name, 999, "CMAP_%s", cm->name );
		sl = stringlist_match_string_exact( read_in_state, cm_name );
		if( sl == NULL ) 
			list_to_use[ n_list_to_use++ ] = cm;

		cm = cm->next;
		}

	if( n_list_to_use == 0 ) {
		fprintf( stderr, "Strange internal error, no colormaps known?!?\n" );
		exit(-1);
		}

	/* Make sure at least one colormap is enabled. If not,
	 * set the first one to be enabled
	 */
	n_enabled = 0;
	for( i=0; i<n_list_to_use; i++ )
		n_enabled += list_to_use[i]->enabled;
	if( n_enabled == 0 )
		list_to_use[0]->enabled = 1;

	/* Set our current colormap to the first enabled one */
	for( i=0; i<n_list_to_use; i++ )
		if( list_to_use[i]->enabled ) {
			current_colormap_list = list_to_use[i];
			break;
			}

	/* Set up our next and prev pointers */
	for( i=0; i<n_list_to_use; i++ ) {
		if( i == 0 ) 
			list_to_use[i]->prev = NULL;
		else
			list_to_use[i]->prev = list_to_use[i-1];
		if( i == (n_list_to_use-1))
			list_to_use[i]->next = NULL;
		else
			list_to_use[i]->next = list_to_use[i+1];
		}

	/* Set to head of our list */
	colormap_list = list_to_use[0];

	/* check to make sure all is kosher */
	cm = colormap_list;
	while( cm != NULL ) {
		if( check_cmaplist_magic( cm ) != 0 ) {
			fprintf( stderr, "Error, generated a cmaplist with an invalid element\n" );
			exit(-1);
			}
		cm = cm->next;
		}

	free( list_to_use );
	free( cmap_used_already );
}

/***************************************************************************************************/
void x_create_colormap( char *name, unsigned char r[256], unsigned char g[256], unsigned char b[256] )
{
        Colormap        orig_colormap, new_colormap;
        Display         *display;
        int             i, status=0, enabled;
        XColor          *color;
        unsigned long   plane_masks[1], pixels[1];
        Cmaplist        *cmaplist, *cml;
        static int      first_time_through = FALSE;
	char		sl_cmap_name[1020];
	Stringlist	*sl;

	/*
	printf( "----------------------\n" );
	printf( "in x_create_colormap, here is state stringlist:\n" );
	stringlist_dump( read_in_state );
	printf( "----------------------\n" );
	*/

        display = XtDisplay( topLevel );

        if( colormap_list == NULL )
                first_time_through = TRUE;
        else
                first_time_through = FALSE;

        /* Make a new Cmaplist structure, and link it into the list */
        new_cmaplist( &cmaplist );
        cmaplist->name = (char *)malloc( (strlen(name)+1)*sizeof(char) );
        strcpy( cmaplist->name, name );

	/* Figure out if, according to our saved state, this colormap
	 * should be enabled or not. If we don't find this colormap 
	 * in the list, default to 'Enabled' 
	 */
	if( strlen(name) > 1000 ) {
		fprintf( stderr, "Error, colormap name is too long: %s\n", name );
		exit(-1);
		}
	snprintf( sl_cmap_name, 1018, "CMAP_%s", name );
	sl = stringlist_match_string_exact( read_in_state, sl_cmap_name );
	if( sl == NULL ) {
		/* People seem to find this message confusing
		printf( "did not find color map %s in state file: enabling by default\n", sl_cmap_name );
		*/
	 	cmaplist->enabled = 1;
		}
	else
		{
		if( sl->sltype != SLTYPE_INT ) {
			fprintf( stderr, "Error, saved TYPE for colormap %s is not an int\n", sl_cmap_name );
			enabled = 1;
			}
		else
			enabled = *((int *)(sl->aux));
		cmaplist->enabled = enabled;
		}

        if( first_time_through )
                colormap_list = cmaplist;
        else
                {
                cml = colormap_list;
                while( cml->next != NULL )
                        cml = cml->next;
                cml->next      = cmaplist;
                cmaplist->prev = cml;
                }

        if( first_time_through ) {

                /* First time through, allocate the read/write entries in the colormap.
                 * We always do ten more than the number of colors requested so that
                 * black can indicate 'fill_value' entries, and the other color entries
                 * (which are the window elements) can be colored as desired.
                 */
                current_colormap_list = cmaplist;

                /* TrueColor device */
                if( options.display_type == TrueColor ) {
                        for( i=0; i<(options.n_colors+options.n_extra_colors); i++ ) {
                                (cmaplist->color_list+i)->pixel = (long)i;
                                *(cmaplist->pixel_transform+i)  = (ncv_pixel)i;
                                }
                        }

                /* Shared colormap, PseudoColor device */
                if( (!options.private_colormap) && (options.display_type == PseudoColor) ) {
                        /* Try the standard colormap, and see if
                         * it has enough available colorcells.
                         */
                        orig_colormap    = DefaultColormap( display, 0 );
                        current_colormap = orig_colormap;
                        i      = 0;
                        status = 1;
                        while( (i < (options.n_colors+options.n_extra_colors)) && (status != 0) ) {
                                status = XAllocColorCells( display, orig_colormap, True,
                                        plane_masks, 0, pixels, 1 );
                                (cmaplist->color_list+i)->pixel = *pixels;
                                *(cmaplist->pixel_transform+i)   = (ncv_pixel)*pixels;
                                i++;
                                }
                        }

                /* standard colormap failed for PseudoColor, allocate a private colormap */
                if( (options.display_type == PseudoColor) &&
                                ((status == 0) || (options.private_colormap)) ) {
                        printf( "Using private colormap...\n" );
                        new_colormap  = XCreateColormap(
                                display,
                                RootWindow( display, DefaultScreen( display ) ),
                                XDefaultVisualOfScreen( XtScreen( topLevel ) ),
                                AllocNone );
                        current_colormap = new_colormap;
                        for( i=0; i<(options.n_colors+options.n_extra_colors); i++ )
                                {
                                status = XAllocColorCells( display, new_colormap,
                                                True, plane_masks,
                                                0, pixels, 1 );
                                if( status == 0 ) {
                                        fprintf( stderr, "ncview: x_create_colormap: couldn't allocate \n" );
                                        fprintf( stderr, "requested number of colorcells in a private colormap\n" );
                                        fprintf( stderr, "Try requesting fewer colors with the -nc option\n" );
                                        exit( -1 );
                                        }
                                (cmaplist->color_list+i)->pixel = *pixels;
                                *(cmaplist->pixel_transform+i)  = (ncv_pixel)*pixels;
                                }
                        }
                pixel_transform = cmaplist->pixel_transform;
                }
        else
                {
                /* if NOT the first time through, just set the pixel values */
                for( i=0; i<options.n_colors+options.n_extra_colors; i++ ) {
                        (cmaplist->color_list+i)->pixel =
                                (current_colormap_list->color_list+i)->pixel;
                        *(cmaplist->pixel_transform+i)   =
                                (ncv_pixel)(current_colormap_list->color_list+i)->pixel;
                        }
                }

        /* Set the first ten colors including black, the color used for "Fill_Value" entries */
        for( i=0; i<options.n_extra_colors; i++ ) {
                color        = cmaplist->color_list+i;
                color->flags = DoRed | DoGreen | DoBlue;
                color->red   = 256*(unsigned int)255;
                color->green = 256*(unsigned int)255;
                color->blue  = 256*(unsigned int)255;
                }
        color        = cmaplist->color_list+1;
        color->flags = DoRed | DoGreen | DoBlue;
        color->red   = 256*(unsigned int)0;
        color->green = 256*(unsigned int)0;
        color->blue  = 256*(unsigned int)0;

        for( i=options.n_extra_colors; i<options.n_colors+options.n_extra_colors; i++ )
                {
                color        = cmaplist->color_list+i;
                color->flags = DoRed | DoGreen | DoBlue;
                color->red   = (unsigned int)
                                (256*interp( i-options.n_extra_colors, options.n_colors, r, 256 ));
                color->green = (unsigned int)
                                (256*interp( i-options.n_extra_colors, options.n_colors, g, 256 ));
                color->blue  = (unsigned int)
                                (256*interp( i-options.n_extra_colors, options.n_colors, b, 256 ));
                }

        if( (options.display_type == PseudoColor) && first_time_through )
                XStoreColors( XtDisplay(topLevel), current_colormap, current_colormap_list->color_list,
                        options.n_colors+options.n_extra_colors );
}

/****************************************************************************************************
 * This returns the name of the newly installed colormap.  If do_widgets_flag
 * is false, don't install colormaps on the widgets, just on the topLevel.
 */
char *x_change_colormap( int delta, int do_widgets_flag )
{
	int	dump_cmap_list, n_enabled;
	Cmaplist *cursor;

	dump_cmap_list = 0;	/* for debugging */

	if( dump_cmap_list ) {
		printf( "---- on entry to x_change_colormap, here is colormap_list\n" );
		dump_Cmaplist( colormap_list );
		}

	/* Make sure at least one colormap is enabled. As a failsafe, is none
	 * are enabled, enable the first one on the list
	 */
	cursor = colormap_list;
	if( check_cmaplist_magic( cursor ) != 0 ) {
		fprintf( stderr, "Error, trying to change colormaps in a cmaplist with an invalid element\n" );
		exit(-1);
		}
	n_enabled = colormap_list->enabled;
	while( cursor->next != NULL ) {
		if( check_cmaplist_magic( cursor ) != 0 ) {
			fprintf( stderr, "Error, trying to change colormaps in a cmaplist with an invalid element\n" );
			exit(-1);
			}
		cursor = cursor->next;
		n_enabled = n_enabled + cursor->enabled;
		}
	if( n_enabled == 0 ) {
		printf( "Warning, no colormaps enabled in x_change_colormap -- using first on list\n" );
		colormap_list->enabled = 1;
		}

        if( delta > 0 ) {
		/* Advance to next enabled colormap. Might have to wrap
		 * around to the start to encounter an enabled one.
		 */
	 	if( current_colormap_list->next == NULL )
			current_colormap_list = colormap_list;	/* go to head of list */
		else
			current_colormap_list = current_colormap_list->next;
		while( current_colormap_list->enabled == 0 ) {
			if( current_colormap_list->next == NULL )
				current_colormap_list = colormap_list;
			else
				current_colormap_list = current_colormap_list->next;
			}
                }

        else if( delta < 0 ) {
		/* delta < 0, go backwards on the list */
                if( current_colormap_list->prev == NULL ) {
			/* Go to last colormap */
                        while( current_colormap_list->next != NULL )
                                current_colormap_list = current_colormap_list->next;
			}
		else
			current_colormap_list = current_colormap_list->prev;

		/* Now go to an enabled colormap */
		while( current_colormap_list->enabled == 0 ) {
			if( current_colormap_list->prev == NULL ) {
				/* Go to last colormap */
				while( current_colormap_list->next != NULL )
					current_colormap_list = current_colormap_list->next;
				}
			else
				current_colormap_list = current_colormap_list->prev;
			}
                }
	else
		{
		;	/* We can be called with delta=0, if we changed to a new colormap in the options selection widget */
		}

        if( options.display_type == PseudoColor )
                XStoreColors( XtDisplay(topLevel), current_colormap,
                        current_colormap_list->color_list,
                        options.n_colors+1 );
        pixel_transform = current_colormap_list->pixel_transform;

        return( current_colormap_list->name );
}

/*************************************************************************************************/
static void new_cmaplist( Cmaplist **cml )
{
	(*cml) = (Cmaplist *)malloc( sizeof( Cmaplist ) );
	(*cml)->color_list = (XColor *)malloc( (options.n_colors+options.n_extra_colors)*sizeof(XColor) );
	(*cml)->next       = NULL;
	(*cml)->prev       = NULL;
	(*cml)->name       = NULL;
	(*cml)->enabled    = 1;		/* start out enabled by default */
	(*cml)->pixel_transform = (ncv_pixel *)malloc( (options.n_colors+options.n_extra_colors)*sizeof(ncv_pixel) );
	(*cml)->magic      = CMAPLIST_MAGIC;		/* indicate a valid list element */
}

/*************************************************************************************************
 * Returns -1 unless the passed thing is a valid Cmaplist * pointing to a valid element
 */
	static int
check_cmaplist_magic( Cmaplist *cml )
{
	if( cml == NULL ) {
		fprintf( stderr, "Error, NULL cmaplist pointer!\n" );
		return( -1 );
		}

	if( cml->magic != CMAPLIST_MAGIC ) {
		if( cml->magic == CMAPLIST_BAD_MAGIC ) {
			fprintf( stderr, "Error, using a previously deleted Cmaplist (bad magic number found)\n" );
			return(-1);
			}
		else
			{
			fprintf( stderr, "Error, trying to use an invalid or malformed Cmaplist (missing valid magic number) \n" );
			return(-1);
			}
		}

	return(0);
}

/*************************************************************************************************/
void delete_cmaplist( Cmaplist *cml )
{
	Cmaplist	*cursor, *tt;

	if( cml == NULL )
		return;

	if( check_cmaplist_magic( cml ) != 0 ) {
		fprintf( stderr, "delete_cmaplist: coding error, trying to delete an invalid Cmaplist\n" );
		exit(-1);
		}

	/* Go to head of list */
	cursor = cml;
	while( cml->prev != NULL )
		cursor = cursor->prev;

	/* Delete all subsequent elements ... */
	while( cursor != NULL ) {
		if( check_cmaplist_magic( cursor ) != 0 ) {
			fprintf( stderr, "delete_cmaplist: coding error, trying to delete an invalid Cmaplist\n" );
			exit(-1);
			}
		if( cursor->color_list != NULL )
			free( cursor->color_list );
		if( cursor->pixel_transform != NULL )
			free( cursor->pixel_transform );

		cursor->magic = CMAPLIST_BAD_MAGIC;

		tt = cursor->next;
		free( cursor );
		cursor = tt;
		}
}

/********************************************************************************************************/
void dump_Cmaplist( Cmaplist *cml ) 
{
	Cmaplist	*cursor, *next, *prev;

	cursor = cml;
	printf( "========================== Cmaplist: ===========\n" );
	while( cursor != NULL ) {
		printf( "ADDR=%ld MAGIC=%d PREV=%ld NEXT=%ld name:%s enabled:%d ", 
			(long)cursor, cursor->magic, (long)cursor->prev, (long)cursor->next,
			cursor->name, cursor->enabled );

		prev = (Cmaplist *)(cursor->prev);
		if( prev == NULL )
			printf( "name_PREV:NULL ");
		else
			printf( "name_PREV:%s ", prev->name );

		next = (Cmaplist *)(cursor->next);
		if( next == NULL )
			printf( "name_NEXT:NULL ");
		else
			printf( "name_NEXT:%s ", next->name );
		printf("\n" );

		cursor = next;
		}
}

/********************************************************************************************************
 * Put the information about what colormaps we want enabled, and their order, into a stringlist 
 * Returns 0 on success, != 0 on error
 */
	int
colormap_options_to_stringlist( Stringlist **sl ) 
{
	Cmaplist	*cursor;
	int		err, enabled;
	char		cmap_name_ext[1024];	/* "extended" because it has a "CMAP_" prepended */

	cursor = colormap_list;
	while( cursor != NULL ) {
		if( check_cmaplist_magic( cursor ) != 0 ) {
			fprintf( stderr, "Error, trying to find information about a cmaplist with an invalid element\n" );
			exit(-1);
			}
		if( strlen( cursor->name ) > 1000 ) {
			fprintf( stderr, "Error, colormap name too long -- max is 1000 chars (routine colormap_options_to_stringlist)\n" );
			exit(-1);
			}
		snprintf( cmap_name_ext, 1022, "CMAP_%s", cursor->name );
		enabled = cursor->enabled;
		if( (err = stringlist_add_string( sl, cmap_name_ext, &enabled, SLTYPE_INT )) != 0 ) {
			fprintf( stderr, "colormap_options_to_stringlist: Error creating save state stringlist\n" );
			return( err );
			}

		cursor = cursor->next;
		}

	return( 0 );
}

